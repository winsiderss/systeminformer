using System;
using System.IO;
using System.Net;
using System.Net.Http;
using System.Net.Http.Headers;
using System.Text;
using System.Text.Json;
using System.Collections.Generic;

namespace CustomBuildTool
{
    public static class Github
    {
        private static readonly string BaseUrl = string.Empty;
        private static readonly string BaseToken = string.Empty;

        static Github()
        {
            BaseUrl = Win32.GetEnvironmentVariable("%APPVEYOR_GITHUB_MIRROR_API%");
            BaseToken = Win32.GetEnvironmentVariable("%APPVEYOR_GITHUB_MIRROR_KEY%");
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
                    Prerelease = true
                };

                byte[] buildPostString = JsonSerializer.SerializeToUtf8Bytes(buildUpdateRequest, GithubReleasesRequestContext.Default.GithubReleasesRequest);

                if (buildPostString == null || buildPostString.LongLength == 0)
                    return null;

                using (HttpClientHandler httpClientHandler = new HttpClientHandler())
                {
                    httpClientHandler.AutomaticDecompression = DecompressionMethods.All;

                    using (HttpClient httpClient = new HttpClient(httpClientHandler))
                    {
                        httpClient.DefaultRequestVersion = HttpVersion.Version20;
                        httpClient.DefaultVersionPolicy = HttpVersionPolicy.RequestVersionOrHigher;
                        httpClient.DefaultRequestHeaders.UserAgent.Add(new ProductInfoHeaderValue("CustomBuildTool", "1.0"));
                        httpClient.DefaultRequestHeaders.Accept.Add(new MediaTypeWithQualityHeaderValue("application/json"));
                        httpClient.DefaultRequestHeaders.Authorization = new AuthenticationHeaderValue("Token", BaseToken);

                        using (ByteArrayContent httpContent = new ByteArrayContent(buildPostString))
                        {
                            httpContent.Headers.ContentType = new MediaTypeHeaderValue("application/json");

                            var httpTask = httpClient.PostAsync($"{BaseUrl}/releases", httpContent);
                            httpTask.Wait();

                            if (!httpTask.Result.IsSuccessStatusCode)
                            {
                                Program.PrintColorMessage("[CreateRelease-Post] " + httpTask.Result, ConsoleColor.Red);
                                return null;
                            }

                            var httpResult = httpTask.Result.Content.ReadAsStringAsync();
                            httpResult.Wait();

                            if (!httpResult.IsCompletedSuccessfully || string.IsNullOrWhiteSpace(httpResult.Result))
                            {
                                Program.PrintColorMessage("[CreateRelease-ReadAsStringAsync]", ConsoleColor.Red);
                                return null;
                            }

                            var response = JsonSerializer.Deserialize(httpResult.Result, GithubReleasesResponseContext.Default.GithubReleasesResponse);

                            if (response == null)
                            {
                                Program.PrintColorMessage("[CreateRelease-GithubReleasesResponse]", ConsoleColor.Red);
                                return null;
                            }

                            if (string.IsNullOrWhiteSpace(response.upload_url))
                            {
                                Program.PrintColorMessage("[CreateRelease-upload_url]", ConsoleColor.Red);
                                return null;
                            }

                            return response;
                        }
                    }
                }
            }
            catch (Exception ex)
            {
                Program.PrintColorMessage("[CreateRelease] " + ex, ConsoleColor.Red);
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
                using (HttpClientHandler httpClientHandler = new HttpClientHandler())
                {
                    httpClientHandler.AutomaticDecompression = DecompressionMethods.All;

                    using (HttpClient httpClient = new HttpClient(httpClientHandler))
                    {
                        httpClient.DefaultRequestVersion = HttpVersion.Version20;
                        httpClient.DefaultVersionPolicy = HttpVersionPolicy.RequestVersionOrHigher;
                        httpClient.DefaultRequestHeaders.UserAgent.Add(new ProductInfoHeaderValue("CustomBuildTool", "1.0"));
                        httpClient.DefaultRequestHeaders.Accept.Add(new MediaTypeWithQualityHeaderValue("application/json"));
                        httpClient.DefaultRequestHeaders.Authorization = new AuthenticationHeaderValue("Token", BaseToken);

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

                        var response = JsonSerializer.Deserialize(httpResult.Result, GithubReleasesResponseContext.Default.GithubReleasesResponse);

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
            }
            catch (Exception ex)
            {
                Program.PrintColorMessage("[DeleteRelease] " + ex, ConsoleColor.Red);
            }

            return false;
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
                    Prerelease = true
                };

                byte[] buildPostString = JsonSerializer.SerializeToUtf8Bytes(buildUpdateRequest, GithubReleasesRequestContext.Default.GithubReleasesRequest);

                if (buildPostString == null || buildPostString.LongLength == 0)
                    return null;

                using (HttpClientHandler httpClientHandler = new HttpClientHandler())
                {
                    httpClientHandler.AutomaticDecompression = DecompressionMethods.All;

                    using (HttpClient httpClient = new HttpClient(httpClientHandler))
                    {
                        httpClient.DefaultRequestVersion = HttpVersion.Version20;
                        httpClient.DefaultVersionPolicy = HttpVersionPolicy.RequestVersionOrHigher;
                        httpClient.DefaultRequestHeaders.UserAgent.Add(new ProductInfoHeaderValue("CustomBuildTool", "1.0"));
                        httpClient.DefaultRequestHeaders.Accept.Add(new MediaTypeWithQualityHeaderValue("application/json"));
                        httpClient.DefaultRequestHeaders.Authorization = new AuthenticationHeaderValue("Token", BaseToken);

                        using (ByteArrayContent httpContent = new ByteArrayContent(buildPostString))
                        {
                            httpContent.Headers.ContentType = new MediaTypeHeaderValue("application/json");

                            var httpTask = httpClient.PostAsync($"{BaseUrl}/releases/{ReleaseId}", httpContent);
                            httpTask.Wait();

                            if (!httpTask.Result.IsSuccessStatusCode)
                            {
                                Program.PrintColorMessage("[UpdateRelease-Post] " + httpTask.Result, ConsoleColor.Red);
                                return null;
                            }

                            var httpResult = httpTask.Result.Content.ReadAsStringAsync();
                            httpResult.Wait();

                            if (!httpResult.IsCompletedSuccessfully || string.IsNullOrWhiteSpace(httpResult.Result))
                            {
                                Program.PrintColorMessage("[UpdateRelease-ReadAsStringAsync]", ConsoleColor.Red);
                                return null;
                            }

                            var response = JsonSerializer.Deserialize(httpResult.Result, GithubReleasesResponseContext.Default.GithubReleasesResponse);

                            if (response == null)
                            {
                                Program.PrintColorMessage("[UpdateRelease-GithubReleasesResponse]", ConsoleColor.Red);
                                return null;
                            }

                            if (string.IsNullOrWhiteSpace(response.upload_url))
                            {
                                Program.PrintColorMessage("[UpdateRelease-upload_url]", ConsoleColor.Red);
                                return null;
                            }

                            return response;
                        }
                    }
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
                using (HttpClientHandler httpClientHandler = new HttpClientHandler())
                {
                    httpClientHandler.AutomaticDecompression = DecompressionMethods.All;

                    using (HttpClient httpClient = new HttpClient(httpClientHandler))
                    {
                        httpClient.DefaultRequestVersion = HttpVersion.Version20;
                        httpClient.DefaultVersionPolicy = HttpVersionPolicy.RequestVersionOrHigher;
                        httpClient.DefaultRequestHeaders.UserAgent.Add(new ProductInfoHeaderValue("CustomBuildTool", "1.0"));
                        httpClient.DefaultRequestHeaders.Accept.Add(new MediaTypeWithQualityHeaderValue("application/json"));
                        httpClient.DefaultRequestHeaders.Authorization = new AuthenticationHeaderValue("Token", BaseToken);

                        using (FileStream fileStream = File.OpenRead(FileName))
                        using (BufferedStream bufferedStream = new BufferedStream(fileStream))
                        using (StreamContent streamContent = new StreamContent(bufferedStream))
                        {
                            streamContent.Headers.ContentType = new MediaTypeHeaderValue("application/octet-stream");

                            var httpUpload = httpClient.PostAsync(upload_url, streamContent);
                            httpUpload.Wait();

                            if (!httpUpload.Result.IsSuccessStatusCode)
                            {
                                Program.PrintColorMessage("[UploadAssets-Post] " + httpUpload.Result, ConsoleColor.Red);
                                return null;
                            }

                            var httpUploadResult = httpUpload.Result.Content.ReadAsStringAsync();
                            httpUploadResult.Wait();

                            if (!httpUploadResult.IsCompletedSuccessfully || string.IsNullOrWhiteSpace(httpUploadResult.Result))
                            {
                                Program.PrintColorMessage("[UploadAssets-ReadAsStringAsync]", ConsoleColor.Red);
                                return null;
                            }

                            var response = JsonSerializer.Deserialize(httpUploadResult.Result, GithubAssetsResponseContext.Default.GithubAssetsResponse);

                            if (response == null)
                            {
                                Program.PrintColorMessage("[UploadAssets-GithubAssetsResponse]", ConsoleColor.Red);
                                return null;
                            }

                            if (!response.Uploaded || string.IsNullOrWhiteSpace(response.download_url))
                            {
                                Program.PrintColorMessage("[UploadAssets-download_url]", ConsoleColor.Red);
                                return null;
                            }

                            return response;
                        }
                    }
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
            if (obj == null)
                return false;

            if (obj is not GithubRelease file)
                return false;

            return this.ReleaseId.Equals(file.ReleaseId, StringComparison.OrdinalIgnoreCase);
        }

        public bool Equals(GithubRelease other)
        {
            return this.ReleaseId.Equals(other.ReleaseId, StringComparison.OrdinalIgnoreCase);
        }

        public int CompareTo(object obj)
        {
            if (obj == null)
                return 1;

            if (obj is GithubRelease package)
                return this.ReleaseId.CompareTo(package.ReleaseId);
            else
                return 1;
        }

        public int CompareTo(GithubRelease obj)
        {
            if (obj == null)
                return 1;

            return this.ReleaseId.CompareTo(obj.ReleaseId);
        }
    }

    public class GithubReleaseAsset : IComparable, IComparable<GithubReleaseAsset>, IEquatable<GithubReleaseAsset>
    {
        public string Filename { get; private set; }
        public string DownloadUrl { get; private set; }

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
            if (obj == null)
                return false;

            if (obj is not GithubReleaseAsset file)
                return false;

            return this.Filename.Equals(file.Filename, StringComparison.OrdinalIgnoreCase);
        }

        public bool Equals(GithubReleaseAsset other)
        {
            return this.Filename.Equals(other.Filename, StringComparison.OrdinalIgnoreCase);
        }

        public int CompareTo(object obj)
        {
            if (obj == null)
                return 1;

            if (obj is GithubReleaseAsset package)
                return this.Filename.CompareTo(package.Filename);
            else
                return 1;
        }

        public int CompareTo(GithubReleaseAsset obj)
        {
            if (obj == null)
                return 1;

            return this.Filename.CompareTo(obj.Filename);
        }
    }
}
