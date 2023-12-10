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
    public static class Github
    {
        private static readonly string BaseUrl;
        private static readonly string BaseToken;

        static Github()
        {
            BaseUrl = Win32.GetEnvironmentVariable("GITHUB_MIRROR_API");
            BaseToken = Win32.GetEnvironmentVariable("GITHUB_MIRROR_KEY");
        }

        /// <summary>
        /// This endpoint creates a new Github release and triggers notifications.
        /// </summary>
        /// <param name="Version">The unique identifier of the release.</param>
        /// <returns>HTTP response status.</returns>
        public static GithubReleasesResponse CreateRelease(string Version)
        {
            if (string.IsNullOrWhiteSpace(BaseUrl))
                return null;
            if (string.IsNullOrWhiteSpace(BaseToken))
                return null;

            try
            {
                GithubCreateReleaseRequest buildUpdateRequest = new GithubCreateReleaseRequest()
                {
                    ReleaseTag = Version,
                    Name = Version,
                    Draft = true,
                    Prerelease = false,
                    GenerateReleaseNotes = true
                };

                byte[] buildPostString = JsonSerializer.SerializeToUtf8Bytes(buildUpdateRequest, GithubCreateReleaseRequestContext.Default.GithubCreateReleaseRequest);

                if (buildPostString.LongLength == 0)
                    return null;

                using (HttpClient httpClient = Http.CreateGithubClient(BaseToken))
                {
                    string result = Http.PostString(httpClient, $"{BaseUrl}/releases", buildPostString);

                    if (string.IsNullOrWhiteSpace(result))
                    {
                        Program.PrintColorMessage("[CreateRelease-PostString]", ConsoleColor.Red);
                        return null;
                    }

                    GithubReleasesResponse response = JsonSerializer.Deserialize(result, GithubReleasesResponseContext.Default.GithubReleasesResponse);

                    if (response == null)
                    {
                        Program.PrintColorMessage("[CreateRelease-GithubReleasesResponse]", ConsoleColor.Red);
                        return null;
                    }

                    if (string.IsNullOrWhiteSpace(response.UploadUrl))
                    {
                        Program.PrintColorMessage("[CreateRelease-upload_url]", ConsoleColor.Red);
                        return null;
                    }

                    return response;
                }
            }
            catch (Exception ex)
            {
                Program.PrintColorMessage("[CreateRelease] " + ex, ConsoleColor.Red);
            }

            return null;
        }

        /// <summary>
        /// This endpoint deletes a release from Github.
        /// </summary>
        /// <param name="Version">The unique identifier of the release.</param>
        /// <returns>HTTP response status.</returns>
        public static bool DeleteRelease(string Version)
        {
            if (string.IsNullOrWhiteSpace(BaseUrl))
                return false;
            if (string.IsNullOrWhiteSpace(BaseToken))
                return false;

            try
            {
                using (HttpClient httpClient = Http.CreateGithubClient(BaseToken))
                {
                    var httpTask = httpClient.GetAsync($"{BaseUrl}/releases/tags/{Version}");
                    httpTask.Wait();

                    if (!httpTask.Result.IsSuccessStatusCode)
                    {
                        if (httpTask.Result.StatusCode == HttpStatusCode.NotFound)
                        {
                            return true; // tag doesn't exist
                        }
                        else
                        {
                            Program.PrintColorMessage("[DeleteRelease-GetAsync] " + httpTask.Result, ConsoleColor.Red);
                            return false;
                        }
                    }

                    var httpResult = httpTask.Result.Content.ReadAsStringAsync();
                    httpResult.Wait();

                    if (!httpResult.IsCompletedSuccessfully || string.IsNullOrWhiteSpace(httpResult.Result))
                    {
                        Program.PrintColorMessage("[DeleteRelease-ReadAsStringAsync]", ConsoleColor.Red);
                        return false;
                    }

                    GithubReleasesResponse response = JsonSerializer.Deserialize(httpResult.Result, GithubReleasesResponseContext.Default.GithubReleasesResponse);

                    if (response == null)
                    {
                        Program.PrintColorMessage("[DeleteRelease-GithubReleasesResponse]", ConsoleColor.Red);
                        return false;
                    }

                    if (string.IsNullOrWhiteSpace(response.ReleaseId))
                    {
                        Program.PrintColorMessage("[DeleteRelease-ReleaseId]", ConsoleColor.Red);
                        return false;
                    }

                    // Delete the release (required)
                    var httpDeleteReleaseTask = httpClient.DeleteAsync($"{BaseUrl}/releases/{response.ReleaseId}");
                    httpDeleteReleaseTask.Wait();

                    if (!httpDeleteReleaseTask.IsCompletedSuccessfully)
                    {
                        Program.PrintColorMessage("[DeleteRelease-DeleteAsync]", ConsoleColor.Red);
                        return false;
                    }

                    // Delete the release tag (optional)
                    var httpDeleteTagTask = httpClient.DeleteAsync($"{BaseUrl}/git/refs/tags/{Version}");
                    httpDeleteTagTask.Wait();

                    return true;
                }
            }
            catch (Exception ex)
            {
                Program.PrintColorMessage("[DeleteRelease] " + ex, ConsoleColor.Red);
            }

            return false;
        }

        /// <summary>
        /// This endpoint updates an existing release on Github.
        /// </summary>
        /// <param name="ReleaseId">The unique identifier of the release.</param>
        /// <returns>HTTP response status.</returns>
        public static GithubReleasesResponse UpdateRelease(string ReleaseId)
        {
            if (string.IsNullOrWhiteSpace(BaseUrl))
                return null;
            if (string.IsNullOrWhiteSpace(BaseToken))
                return null;

            try
            {
                var buildUpdateRequest = new GithubCreateReleaseRequest
                {
                    Draft = false,
                    Prerelease = true
                };

                byte[] buildPostString = JsonSerializer.SerializeToUtf8Bytes(buildUpdateRequest, GithubCreateReleaseRequestContext.Default.GithubCreateReleaseRequest);

                if (buildPostString.LongLength == 0)
                    return null;

                using (HttpClient httpClient = Http.CreateGithubClient(BaseToken))
                {
                    string result = Http.PostString(httpClient, $"{BaseUrl}/releases/{ReleaseId}", buildPostString);

                    if (string.IsNullOrWhiteSpace(result))
                    {
                        Program.PrintColorMessage("[UpdateRelease-ReadAsStringAsync]", ConsoleColor.Red);
                        return null;
                    }

                    GithubReleasesResponse response = JsonSerializer.Deserialize(result, GithubReleasesResponseContext.Default.GithubReleasesResponse);

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

        /// <summary>
        /// This endpoint uploads binaries for a pending Github release.
        /// </summary>
        /// <param name="Version">The unique identifier of the release.</param>
        /// <param name="FileName">The name of the file.</param>
        /// <param name="UploadUrl">The unique Github asset location for this upload.</param>
        /// <returns>HTTP response status.</returns>
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
                using (HttpClient httpClient = Http.CreateGithubClient(BaseToken))
                {
                    var result = Http.UploadFile(httpClient, upload_url, FileName);

                    if (string.IsNullOrWhiteSpace(result))
                    {
                        Program.PrintColorMessage("[UploadAssets-ReadAsStringAsync]", ConsoleColor.Red);
                        return null;
                    }

                    GithubAssetsResponse response = JsonSerializer.Deserialize(result, GithubAssetsResponseContext.Default.GithubAssetsResponse);

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
        public string ReleaseId;
        public List<GithubReleaseAsset> Files;

        public GithubRelease()
        {
            this.ReleaseId = string.Empty;
            this.Files = new List<GithubReleaseAsset>();
        }

        public bool Valid
        {
            get { return !string.IsNullOrWhiteSpace(this.ReleaseId) && this.Files.Count > 0; }
        }

        public string GetFileUrl(string FileName)
        {
            foreach (GithubReleaseAsset entry in this.Files)
            {
                if (!string.IsNullOrWhiteSpace(entry.DownloadUrl))
                {
                    if (string.Equals(FileName, entry.Filename, StringComparison.OrdinalIgnoreCase))
                    {
                        return entry.DownloadUrl;
                    }
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
            if (obj is not GithubRelease file)
                return false;

            return this.ReleaseId.Equals(file.ReleaseId, StringComparison.OrdinalIgnoreCase);
        }

        public bool Equals(GithubRelease other)
        {
            if (other == null)
                return false;

            return this.ReleaseId.Equals(other.ReleaseId, StringComparison.OrdinalIgnoreCase);
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

    public class GithubReleaseAsset : IComparable, IComparable<GithubReleaseAsset>, IEquatable<GithubReleaseAsset>
    {
        public string Filename { get; }
        public string DownloadUrl { get; }

        public GithubReleaseAsset(string Filename, string DownloadUrl)
        {
            this.Filename = Filename;
            this.DownloadUrl = DownloadUrl;
        }

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
            if (obj is not GithubReleaseAsset file)
                return false;

            return this.Filename.Equals(file.Filename, StringComparison.OrdinalIgnoreCase);
        }

        public bool Equals(GithubReleaseAsset other)
        {
            if (other == null)
                return false;

            return this.Filename.Equals(other.Filename, StringComparison.OrdinalIgnoreCase);
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
