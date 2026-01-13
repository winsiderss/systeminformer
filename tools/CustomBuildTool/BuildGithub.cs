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
    /// Provides static methods for managing GitHub releases and assets via the GitHub API.
    /// </summary>
    public static class BuildGithub
    {
        /// <summary>
        /// The GitHub API token used for authentication.
        /// </summary>
        private static readonly string BaseToken;

        /// <summary>
        /// The base URL for the GitHub API.
        /// </summary>
        private static readonly string BaseUrl;

        /// <summary>
        /// Initializes static members of the <see cref="BuildGithub"/> class by reading environment variables.
        /// </summary>
        static BuildGithub()
        {
            BaseToken = Win32.GetEnvironmentVariable("GITHUB_MIRROR_KEY");
            BaseUrl = Win32.GetEnvironmentVariable("GITHUB_MIRROR_API");
        }

        /// <summary>
        /// Queries the Github Action Run API for the current build's queue time.
        /// </summary>
        /// <returns>
        /// The <see cref="DateTime"/> representing the queue time of the build, or <see cref="DateTime.MinValue"/> if an error occurs.
        /// </returns>
        public static bool BuildQueryQueueTime(out DateTime DateTime)
        {
            string repo = Win32.GetEnvironmentVariable("GITHUB_REPOSITORY");
            string runId = Win32.GetEnvironmentVariable("GITHUB_RUN_ID");
            string token = Win32.GetEnvironmentVariable("GITHUB_TOKEN");

            if (string.IsNullOrWhiteSpace(repo) ||
                string.IsNullOrWhiteSpace(runId) ||
                string.IsNullOrWhiteSpace(token))
            {
                DateTime = DateTime.UtcNow.Subtract(TimeSpan.FromMilliseconds(Environment.TickCount64));

                Console.WriteLine($"Build StartTime: {VT.YELLOW}{DateTime}{VT.RESET} (Failed)");
                Console.WriteLine($"Build Elapsed: {VT.YELLOW}{(DateTimeOffset.UtcNow - DateTime)}{VT.RESET} (Failed)");
                return true;
            }

            try
            {
                using (HttpRequestMessage requestMessage = new HttpRequestMessage(HttpMethod.Get, $"https://api.github.com/repos/{repo}/actions/runs/{runId}"))
                {
                    requestMessage.Headers.Authorization = new AuthenticationHeaderValue("Bearer", token);
                    requestMessage.Headers.Accept.Add(new MediaTypeWithQualityHeaderValue("application/vnd.github+json"));
                    requestMessage.Headers.TryAddWithoutValidation("X-GitHub-Api-Version", "2022-11-28");

                    var httpResult = BuildHttpClient.SendMessage(requestMessage);

                    if (string.IsNullOrWhiteSpace(httpResult))
                    {
                        Console.WriteLine($"{VT.RED}[ERROR] Received empty response.{VT.RESET}");
                        ArgumentNullException.ThrowIfNull(httpResult);
                    }

                    var content = JsonSerializer.Deserialize(httpResult, GithubResponseContext.Default.GithubActionRun);
                    if (content == null)
                    {
                        Console.WriteLine($"{VT.RED}[ERROR] Failed to deserialize the response.{VT.RESET}");
                        ArgumentNullException.ThrowIfNull(content);
                    }

                    var offset = new DateTimeOffset(DateTime.SpecifyKind(content.CreatedAt, DateTimeKind.Utc));

                    Console.WriteLine($"Build Created: {VT.GREEN}{content.CreatedAt}{VT.RESET}");
                    Console.WriteLine($"Build Started: {VT.GREEN}{content.RunStartedAt}{VT.RESET}");
                    Console.WriteLine($"Build Elapsed: {VT.GREEN}{(DateTimeOffset.UtcNow - offset)}{VT.RESET}");

                    DateTime = offset.DateTime;
                    return true;
                }
            }
            catch (Exception ex)
            {
                Console.WriteLine($"{VT.RED}[ERROR] {ex}{VT.RESET}");
                DateTime = DateTime.UtcNow.Subtract(TimeSpan.FromMilliseconds(Environment.TickCount64));
                return false;
            }
        }

        /// <summary>
        /// Retrieves a GitHub release by its release ID.
        /// </summary>
        /// <param name="ReleaseId">The unique identifier of the release.</param>
        /// <returns>
        /// A <see cref="GithubReleaseQueryResponse"/> object containing release details, or <c>null</c> if not found or on error.
        /// </returns>
        public static GithubReleaseQueryResponse GetRelease(ulong ReleaseId)
        {
            if (string.IsNullOrWhiteSpace(BaseUrl))
                return null;
            if (string.IsNullOrWhiteSpace(BaseToken))
                return null;

            try
            {
                using (HttpRequestMessage requestMessage = new HttpRequestMessage(HttpMethod.Get, $"{BaseUrl}/releases/{ReleaseId}"))
                {
                    requestMessage.Headers.Accept.Add(new MediaTypeWithQualityHeaderValue("application/vnd.github+json"));
                    requestMessage.Headers.Authorization = new AuthenticationHeaderValue("Token", BaseToken);
                    requestMessage.Headers.Add("X-GitHub-Api-Version", "2022-11-28");

                    var httpResult = BuildHttpClient.SendMessage(requestMessage, GithubResponseContext.Default.GithubReleaseQueryResponse);

                    if (httpResult == null)
                    {
                        Program.PrintColorMessage("[GetRelease-GithubReleaseQueryResponse]", ConsoleColor.Red);
                        return null;
                    }

                    return httpResult;
                }
            }
            catch (Exception ex)
            {
                Program.PrintColorMessage($"[GetRelease] {ex}", ConsoleColor.Red);
            }

            return null;
        }

        /// <summary>
        /// Creates a new GitHub release.
        /// </summary>
        /// <param name="Version">The version tag for the release.</param>
        /// <param name="Draft">Indicates whether the release is a draft.</param>
        /// <param name="Prerelease">Indicates whether the release is a prerelease.</param>
        /// <param name="GenerateReleaseNotes">Indicates whether to auto-generate release notes.</param>
        /// <returns>
        /// A <see cref="GithubReleasesResponse"/> object containing release details, or <c>null</c> on error.
        /// </returns>
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

        /// <summary>
        /// Deletes a GitHub release and its associated tag.
        /// </summary>
        /// <param name="Version">The version tag of the release to delete.</param>
        /// <returns>
        /// <c>true</c> if the release and tag were deleted or did not exist; <c>false</c> on error.
        /// </returns>
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

                if (githubResponseMessage.ReleaseId == 0)
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

        /// <summary>
        /// Updates an existing GitHub release.
        /// </summary>
        /// <param name="ReleaseId">The unique identifier of the release to update.</param>
        /// <param name="Draft">Indicates whether the release is a draft.</param>
        /// <param name="Prerelease">Indicates whether the release is a prerelease.</param>
        /// <returns>
        /// A <see cref="GithubReleasesResponse"/> object containing updated release details, or <c>null</c> on error.
        /// </returns>
        public static GithubReleasesResponse UpdateRelease(ulong ReleaseId, bool Draft = false, bool Prerelease = false)
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

                using (HttpRequestMessage requestMessage = new HttpRequestMessage(HttpMethod.Patch, $"{BaseUrl}/releases/{ReleaseId}"))
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

        /// <summary>
        /// Uploads an asset file to a GitHub release.
        /// </summary>
        /// <param name="ReleaseAssetUrl">The upload URL for the release asset.</param>
        /// <param name="FileName">The path to the file to upload.</param>
        /// <param name="Name">The name of the asset.</param>
        /// <param name="Label">The label for the asset.</param>
        /// <returns>
        /// A <see cref="GithubAssetsResponse"/> object containing asset upload details, or <c>null</c> on error.
        /// </returns>
        public static GithubAssetsResponse UploadAsset(string ReleaseAssetUrl, string FileName, string Name, string Label)
        {
            if (string.IsNullOrWhiteSpace(BaseToken))
                return null;
            if (!ReleaseAssetUrl.Contains("{?name,label}", StringComparison.OrdinalIgnoreCase))
                return null;

            string upload_url = ReleaseAssetUrl.Replace(
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

    /// <summary>
    /// Represents a GitHub release with associated assets.
    /// </summary>
    public class GithubRelease : IComparable, IComparable<GithubRelease>, IEquatable<GithubRelease>
    {
        /// <summary>
        /// Gets the unique identifier of the release.
        /// </summary>
        public readonly ulong ReleaseId;

        /// <summary>
        /// Gets the list of assets associated with the release.
        /// </summary>
        public List<GithubReleaseAsset> Files;

        /// <summary>
        /// Initializes a new instance of the <see cref="GithubRelease"/> class with default values.
        /// </summary>
        public GithubRelease()
        {
            this.ReleaseId = 0;
            this.Files = new List<GithubReleaseAsset>();
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="GithubRelease"/> class with a specified release ID.
        /// </summary>
        /// <param name="ReleaseId">The unique identifier of the release.</param>
        public GithubRelease(ulong ReleaseId)
        {
            this.ReleaseId = ReleaseId;
            this.Files = new List<GithubReleaseAsset>();
        }

        /// <summary>
        /// Gets a value indicating whether the release is valid (has a non-zero ID and at least one asset).
        /// </summary>
        public bool Valid
        {
            get { return this.ReleaseId != 0 && this.Files.Count > 0; }
        }

        /// <summary>
        /// Gets the download URL for a specified file name in the release assets.
        /// </summary>
        /// <param name="FileName">The name of the file to find.</param>
        /// <returns>The download URL if found; otherwise, an empty string.</returns>
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

        /// <summary>
        /// Gets deployment information for a specified asset name.
        /// </summary>
        /// <param name="Name">The name of the deploy file.</param>
        /// <returns>
        /// The <see cref="GithubReleaseAsset"/> if found; otherwise, <c>null</c>.
        /// </returns>
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

        /// <inheritdoc/>
        public override string ToString()
        {
            return this.ReleaseId.ToString();
        }

        /// <inheritdoc/>
        public override int GetHashCode()
        {
            return this.ReleaseId.GetHashCode();
        }

        /// <inheritdoc/>
        public override bool Equals(object obj)
        {
            return obj is GithubRelease file && this.ReleaseId == file.ReleaseId;
        }

        /// <inheritdoc/>
        public bool Equals(GithubRelease other)
        {
            return other != null && this.ReleaseId == other.ReleaseId;
        }

        /// <inheritdoc/>
        public int CompareTo(object obj)
        {
            if (obj == null)
                return 1;

            if (obj is GithubRelease package)
                return string.Compare(this.ReleaseId.ToString(), package.ReleaseId.ToString(), StringComparison.OrdinalIgnoreCase);
            else
                return 1;
        }

        /// <inheritdoc/>
        public int CompareTo(GithubRelease obj)
        {
            if (obj == null)
                return 1;

            return string.Compare(this.ReleaseId.ToString(), obj.ReleaseId.ToString(), StringComparison.OrdinalIgnoreCase);
        }

        /// <summary>
        /// Determines whether two <see cref="GithubRelease"/> instances are equal.
        /// </summary>
        public static bool operator ==(GithubRelease left, GithubRelease right)
        {
            if (ReferenceEquals(left, null))
            {
                return ReferenceEquals(right, null);
            }

            return left.Equals(right);
        }

        /// <summary>
        /// Determines whether two <see cref="GithubRelease"/> instances are not equal.
        /// </summary>
        public static bool operator !=(GithubRelease left, GithubRelease right)
        {
            return !(left == right);
        }

        /// <summary>
        /// Determines whether one <see cref="GithubRelease"/> is less than another.
        /// </summary>
        public static bool operator <(GithubRelease left, GithubRelease right)
        {
            return ReferenceEquals(left, null) ? !ReferenceEquals(right, null) : left.CompareTo(right) < 0;
        }

        /// <summary>
        /// Determines whether one <see cref="GithubRelease"/> is less than or equal to another.
        /// </summary>
        public static bool operator <=(GithubRelease left, GithubRelease right)
        {
            return ReferenceEquals(left, null) || left.CompareTo(right) <= 0;
        }

        /// <summary>
        /// Determines whether one <see cref="GithubRelease"/> is greater than another.
        /// </summary>
        public static bool operator >(GithubRelease left, GithubRelease right)
        {
            return !ReferenceEquals(left, null) && left.CompareTo(right) > 0;
        }

        /// <summary>
        /// Determines whether one <see cref="GithubRelease"/> is greater than or equal to another.
        /// </summary>
        public static bool operator >=(GithubRelease left, GithubRelease right)
        {
            return ReferenceEquals(left, null) ? ReferenceEquals(right, null) : left.CompareTo(right) >= 0;
        }
    }

    /// <summary>
    /// Represents an asset in a GitHub release.
    /// </summary>
    /// <param name="Filename">The name of the asset file.</param>
    /// <param name="DownloadUrl">The download URL for the asset.</param>
    /// <param name="DeployFile">The deployment file metadata.</param>
    public class GithubReleaseAsset(string Filename, string DownloadUrl, DeployFile DeployFile)
        : IComparable, IComparable<GithubReleaseAsset>, IEquatable<GithubReleaseAsset>
    {
        /// <summary>
        /// Gets the filename of the asset.
        /// </summary>
        public string Filename { get; } = Filename;

        /// <summary>
        /// Gets the download URL for the asset.
        /// </summary>
        public string DownloadUrl { get; } = DownloadUrl;

        /// <summary>
        /// Gets the deployment file metadata.
        /// </summary>
        public DeployFile DeployFile { get; } = DeployFile;

        /// <inheritdoc/>
        public override string ToString()
        {
            return this.Filename;
        }

        /// <inheritdoc/>
        public override int GetHashCode()
        {
            return this.Filename.GetHashCode(StringComparison.OrdinalIgnoreCase);
        }

        /// <inheritdoc/>
        public override bool Equals(object obj)
        {
            return obj is GithubReleaseAsset file && this.Filename.Equals(file.Filename, StringComparison.OrdinalIgnoreCase);
        }

        /// <inheritdoc/>
        public bool Equals(GithubReleaseAsset other)
        {
            return other != null && this.Filename.Equals(other.Filename, StringComparison.OrdinalIgnoreCase);
        }

        /// <inheritdoc/>
        public int CompareTo(object obj)
        {
            if (obj == null)
                return 1;

            if (obj is GithubReleaseAsset package)
                return string.Compare(this.Filename, package.Filename, StringComparison.OrdinalIgnoreCase);
            else
                return 1;
        }

        /// <inheritdoc/>
        public int CompareTo(GithubReleaseAsset obj)
        {
            if (obj == null)
                return 1;

            return string.Compare(this.Filename, obj.Filename, StringComparison.OrdinalIgnoreCase);
        }

        /// <summary>
        /// Determines whether two <see cref="GithubReleaseAsset"/> instances are equal.
        /// </summary>
        public static bool operator ==(GithubReleaseAsset left, GithubReleaseAsset right)
        {
            if (ReferenceEquals(left, null))
            {
                return ReferenceEquals(right, null);
            }

            return left.Equals(right);
        }

        /// <summary>
        /// Determines whether two <see cref="GithubReleaseAsset"/> instances are not equal.
        /// </summary>
        public static bool operator !=(GithubReleaseAsset left, GithubReleaseAsset right)
        {
            return !(left == right);
        }

        /// <summary>
        /// Determines whether one <see cref="GithubReleaseAsset"/> is less than another.
        /// </summary>
        public static bool operator <(GithubReleaseAsset left, GithubReleaseAsset right)
        {
            return ReferenceEquals(left, null) ? !ReferenceEquals(right, null) : left.CompareTo(right) < 0;
        }

        /// <summary>
        /// Determines whether one <see cref="GithubReleaseAsset"/> is less than or equal to another.
        /// </summary>
        public static bool operator <=(GithubReleaseAsset left, GithubReleaseAsset right)
        {
            return ReferenceEquals(left, null) || left.CompareTo(right) <= 0;
        }

        /// <summary>
        /// Determines whether one <see cref="GithubReleaseAsset"/> is greater than another.
        /// </summary>
        public static bool operator >(GithubReleaseAsset left, GithubReleaseAsset right)
        {
            return !ReferenceEquals(left, null) && left.CompareTo(right) > 0;
        }

        /// <summary>
        /// Determines whether one <see cref="GithubReleaseAsset"/> is greater than or equal to another.
        /// </summary>
        public static bool operator >=(GithubReleaseAsset left, GithubReleaseAsset right)
        {
            return ReferenceEquals(left, null) ? ReferenceEquals(right, null) : left.CompareTo(right) >= 0;
        }
    }

    /// <summary>
    /// Represents a deployable file with metadata including name, filename, hash, signature, and length.
    /// </summary>
    /// <remarks>
    /// This class stores deployment file information and provides methods to update download details
    /// from GitHub API responses. The hash code is based on the file name.
    /// </remarks>
    /// <param name="Name">The name of the deploy file.</param>
    /// <param name="Filename">The filename of the deploy file.</param>
    /// <param name="FileHash">The hash value of the deploy file.</param>
    /// <param name="FileSignature">The digital signature of the deploy file.</param>
    /// <param name="FileLength">The file size/length of the deploy file.</param>
    public class DeployFile(string Name, string Filename, string FileHash, string FileSignature, string FileLength)
    {
        /// <summary>
        /// Gets the name of the deploy file.
        /// </summary>
        public readonly string Name = Name;

        /// <summary>
        /// Gets the filename of the deploy file.
        /// </summary>
        public readonly string FileName = Filename;

        /// <summary>
        /// Gets the hash value of the deploy file.
        /// </summary>
        public readonly string FileHash = FileHash;

        /// <summary>
        /// Gets the digital signature of the deploy file.
        /// </summary>
        public readonly string FileSignature = FileSignature;

        /// <summary>
        /// Gets the file size/length of the deploy file.
        /// </summary>
        public readonly string FileLength = FileLength;

        /// <summary>
        /// Gets or sets the hash value from the download source.
        /// </summary>
        public string DownloadHash { get; private set; }

        /// <summary>
        /// Gets or sets the download URL from the download source.
        /// </summary>
        public string DownloadLink { get; private set; }

        /// <summary>
        /// Updates the download hash and link from a GitHub assets response.
        /// </summary>
        /// <param name="response">The GitHub assets response containing hash and download URL.</param>
        /// <returns>True if the response was successfully processed; otherwise, false.</returns>
        /// <remarks>
        /// Returns false if the response is null, the hash value is empty, or the download URL is empty.
        /// </remarks>
        public bool UpdateAssetsResponse(GithubAssetsResponse response)
        {
            if (response == null)
            {
                Program.PrintColorMessage("[UpdateAssetsResponse] response null.", ConsoleColor.Red);
                return false;
            }
            if (string.IsNullOrWhiteSpace(response.HashValue))
            {
                Program.PrintColorMessage("[UpdateAssetsResponse] HashValue null.", ConsoleColor.Red);
                return false;
            }
            if (string.IsNullOrWhiteSpace(response.DownloadUrl))
            {
                Program.PrintColorMessage("[UpdateAssetsResponse] DownloadUrl null.", ConsoleColor.Red);
                return false;
            }

            this.DownloadHash = response.HashValue;
            this.DownloadLink = response.DownloadUrl;
            return true;
        }

        /// <inheritdoc/>
        public override string ToString()
        {
            return this.Name;
        }

        /// <inheritdoc/>
        public override int GetHashCode()
        {
            return this.Name.GetHashCode();
        }
    }
}
