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
    /// Provides methods for interacting with Azure DevOps build information using environment variables and HTTP requests.
    /// </summary>
    public static class BuildDevOps
    {
        /// <summary>
        /// The build ID from the environment variable <c>BUILD_BUILDID</c>.
        /// </summary>
        private static readonly string BaseBuild;

        /// <summary>
        /// The system access token from the environment variable <c>SYSTEM_ACCESSTOKEN</c>.
        /// </summary>
        private static readonly string BaseToken;

        /// <summary>
        /// The team project name from the environment variable <c>SYSTEM_TEAMPROJECT</c>.
        /// </summary>
        private static readonly string BaseName;

        /// <summary>
        /// The collection URI from the environment variable <c>SYSTEM_COLLECTIONURI</c>.
        /// </summary>
        private static readonly string BaseUrl;

        /// <summary>
        /// Initializes static members of the <see cref="BuildDevOps"/> class by reading required environment variables.
        /// </summary>
        static BuildDevOps()
        {
            BaseBuild = Win32.GetEnvironmentVariable("BUILD_BUILDID");
            BaseToken = Win32.GetEnvironmentVariable("SYSTEM_ACCESSTOKEN");
            BaseName = Win32.GetEnvironmentVariable("SYSTEM_TEAMPROJECT");
            BaseUrl = Win32.GetEnvironmentVariable("SYSTEM_COLLECTIONURI");
        }

        /// <summary>
        /// Queries the Azure DevOps build API for the current build's queue time.
        /// </summary>
        /// <returns>
        /// The <see cref="DateTime"/> representing the queue time of the build, or <see cref="DateTime.MinValue"/> if an error occurs.
        /// </returns>
        public static bool BuildQueryQueueTime(out DateTime DateTime)
        {
            if (string.IsNullOrWhiteSpace(BaseBuild) ||
                string.IsNullOrWhiteSpace(BaseToken) ||
                string.IsNullOrWhiteSpace(BaseName) ||
                string.IsNullOrWhiteSpace(BaseUrl))
            {
                Program.PrintColorMessage("One or more required environment variables are missing.", ConsoleColor.Red);
                DateTime = DateTime.UtcNow.Subtract(TimeSpan.FromMilliseconds(Environment.TickCount64));
                return true;
            }

            try
            {
                using (HttpRequestMessage requestMessage = new HttpRequestMessage(HttpMethod.Get, $"{BaseUrl}/{BaseName}/_apis/build/builds/{BaseBuild}?api-version=7.1"))
                {
                    requestMessage.Headers.Authorization = new AuthenticationHeaderValue("Bearer", BaseToken);
                    requestMessage.Headers.Accept.Add(new MediaTypeWithQualityHeaderValue("application/json"));

                    var httpResult = BuildHttpClient.SendMessage(requestMessage);

                    if (string.IsNullOrWhiteSpace(httpResult))
                    {
                        Program.PrintColorMessage($"[ERROR] Received empty response.", ConsoleColor.Red);
                        ArgumentNullException.ThrowIfNull(httpResult);
                    }

                    var content = JsonSerializer.Deserialize(httpResult, BuildInfoResponseContext.Default.BuildInfo);
                    if (content == null)
                    {
                        Program.PrintColorMessage($"[ERROR] Failed to deserialize the response.", ConsoleColor.Red);
                        ArgumentNullException.ThrowIfNull(content);
                    }

                    var offset = new DateTimeOffset(DateTime.SpecifyKind(content.QueueTime, DateTimeKind.Utc));

                    Program.PrintColorMessage($"Build QueueTime: {content.QueueTime}", ConsoleColor.Green);
                    Program.PrintColorMessage($"Build StartTime: {content.StartTime}", ConsoleColor.Green);
                    Program.PrintColorMessage($"Build Elapsed: {(DateTimeOffset.UtcNow - offset)}", ConsoleColor.Green);

                    DateTime = offset.DateTime;
                    return true;
                }
            }
            catch (Exception ex)
            {
                Program.PrintColorMessage($"[ERROR] {ex}", ConsoleColor.Red);
                DateTime = DateTime.UtcNow.Subtract(TimeSpan.FromMilliseconds(Environment.TickCount64));
                return false;
            }
        }
    }

    /// <summary>
    /// Provides a context for source generation of JSON serialization for <see cref="BuildInfo"/>.
    /// </summary>
    [JsonSourceGenerationOptions(DefaultIgnoreCondition = JsonIgnoreCondition.WhenWritingNull, GenerationMode = JsonSourceGenerationMode.Default)]
    [JsonSerializable(typeof(BuildInfo))]
    public partial class BuildInfoResponseContext : JsonSerializerContext
    {

    }

    /// <summary>
    /// Represents the response from the Azure DevOps build API.
    /// </summary>
    public class BuildInfo
    {
        /// <summary>
        /// Gets or sets the repository links.
        /// </summary>
        [JsonPropertyName("_links")]
        public RepoLinks Links { get; set; }

        /// <summary>
        /// Gets or sets the build ID.
        /// </summary>
        [JsonPropertyName("id")]
        public int Id { get; set; }

        /// <summary>
        /// Gets or sets the build number.
        /// </summary>
        [JsonPropertyName("buildNumber")]
        public string BuildNumber { get; set; }

        /// <summary>
        /// Gets or sets the build status.
        /// </summary>
        [JsonPropertyName("status")]
        public string Status { get; set; }

        /// <summary>
        /// Gets or sets the time the build was queued.
        /// </summary>
        [JsonPropertyName("queueTime")]
        public DateTime QueueTime { get; set; }

        /// <summary>
        /// Gets or sets the time the build started.
        /// </summary>
        [JsonPropertyName("startTime")]
        public DateTime StartTime { get; set; }

        /// <summary>
        /// Gets or sets the build URL.
        /// </summary>
        [JsonPropertyName("url")]
        public string Url { get; set; }

        /// <summary>
        /// Gets or sets the build definition.
        /// </summary>
        [JsonPropertyName("definition")]
        public Definition Definition { get; set; }

        /// <summary>
        /// Gets or sets the project information.
        /// </summary>
        [JsonPropertyName("project")]
        public Project Project { get; set; }

        /// <summary>
        /// Gets or sets the build URI.
        /// </summary>
        [JsonPropertyName("uri")]
        public string Uri { get; set; }

        /// <summary>
        /// Gets or sets the source branch.
        /// </summary>
        [JsonPropertyName("sourceBranch")]
        public string SourceBranch { get; set; }

        /// <summary>
        /// Gets or sets the source version.
        /// </summary>
        [JsonPropertyName("sourceVersion")]
        public string SourceVersion { get; set; }

        /// <summary>
        /// Gets or sets the build priority.
        /// </summary>
        [JsonPropertyName("priority")]
        public string Priority { get; set; }

        /// <summary>
        /// Gets or sets the reason for the build.
        /// </summary>
        [JsonPropertyName("reason")]
        public string Reason { get; set; }

        /// <summary>
        /// Gets or sets the identity of the user the build was requested for.
        /// </summary>
        [JsonPropertyName("requestedFor")]
        public Identity RequestedFor { get; set; }

        /// <summary>
        /// Gets or sets the identity of the user who requested the build.
        /// </summary>
        [JsonPropertyName("requestedBy")]
        public Identity RequestedBy { get; set; }

        /// <summary>
        /// Gets or sets the date the build was last changed.
        /// </summary>
        [JsonPropertyName("lastChangedDate")]
        public DateTime LastChangedDate { get; set; }

        /// <summary>
        /// Gets or sets the identity of the user who last changed the build.
        /// </summary>
        [JsonPropertyName("lastChangedBy")]
        public Identity LastChangedBy { get; set; }

        /// <summary>
        /// Gets or sets the build parameters.
        /// </summary>
        [JsonPropertyName("parameters")]
        public string Parameters { get; set; }

        /// <summary>
        /// Gets or sets the build logs.
        /// </summary>
        [JsonPropertyName("logs")]
        public Logs Logs { get; set; }

        /// <summary>
        /// Gets or sets the repository information.
        /// </summary>
        [JsonPropertyName("repository")]
        public Repository Repository { get; set; }

        /// <summary>
        /// Gets or sets a value indicating whether the build is retained by a release.
        /// </summary>
        [JsonPropertyName("retainedByRelease")]
        public bool RetainedByRelease { get; set; }

        /// <summary>
        /// Gets or sets a value indicating whether the commit message is appended to the run name.
        /// </summary>
        [JsonPropertyName("appendCommitMessageToRunName")]
        public bool AppendCommitMessageToRunName { get; set; }

        // [JsonPropertyName("orchestrationPlan")]
        // public OrchestrationPlan OrchestrationPlan { get; set; }

        // [JsonPropertyName("plans")]
        // public List<Plan> Plans { get; set; }
    }

    /// <summary>
    /// Represents repository-related links in the build information.
    /// </summary>
    public class RepoLinks
    {
        /// <summary>
        /// Gets or sets the self link.
        /// </summary>
        [JsonPropertyName("self")]
        public Link Self { get; set; }

        /// <summary>
        /// Gets or sets the web link.
        /// </summary>
        [JsonPropertyName("web")]
        public Link Web { get; set; }

        /// <summary>
        /// Gets or sets the source version display URI link.
        /// </summary>
        [JsonPropertyName("sourceVersionDisplayUri")]
        public Link SourceVersionDisplayUri { get; set; }

        /// <summary>
        /// Gets or sets the timeline link.
        /// </summary>
        [JsonPropertyName("timeline")]
        public Link Timeline { get; set; }

        /// <summary>
        /// Gets or sets the badge link.
        /// </summary>
        [JsonPropertyName("badge")]
        public Link Badge { get; set; }
    }

    /// <summary>
    /// Represents a hyperlink in the build information.
    /// </summary>
    public class Link
    {
        /// <summary>
        /// Gets or sets the hyperlink reference.
        /// </summary>
        [JsonPropertyName("href")]
        public string Href { get; set; }
    }

    /// <summary>
    /// Represents the build definition details.
    /// </summary>
    public class Definition
    {
        /// <summary>
        /// Gets or sets the definition ID.
        /// </summary>
        [JsonPropertyName("id")]
        public int Id { get; set; }

        /// <summary>
        /// Gets or sets the definition name.
        /// </summary>
        [JsonPropertyName("name")]
        public string Name { get; set; }

        /// <summary>
        /// Gets or sets the definition URL.
        /// </summary>
        [JsonPropertyName("url")]
        public string Url { get; set; }

        /// <summary>
        /// Gets or sets the definition URI.
        /// </summary>
        [JsonPropertyName("uri")]
        public string Uri { get; set; }

        /// <summary>
        /// Gets or sets the definition path.
        /// </summary>
        [JsonPropertyName("path")]
        public string Path { get; set; }

        /// <summary>
        /// Gets or sets the definition type.
        /// </summary>
        [JsonPropertyName("type")]
        public string Type { get; set; }

        /// <summary>
        /// Gets or sets the queue status.
        /// </summary>
        [JsonPropertyName("queueStatus")]
        public string QueueStatus { get; set; }

        /// <summary>
        /// Gets or sets the revision number.
        /// </summary>
        [JsonPropertyName("revision")]
        public int Revision { get; set; }

        /// <summary>
        /// Gets or sets the project associated with the definition.
        /// </summary>
        [JsonPropertyName("project")]
        public Project Project { get; set; }

        /// <inheritdoc/>
        public override string ToString()
        {
            return $"{this.Id} - {this.Name}";
        }
    }

    /// <summary>
    /// Represents a project in Azure DevOps.
    /// </summary>
    public class Project
    {
        /// <summary>
        /// Gets or sets the project ID.
        /// </summary>
        [JsonPropertyName("id")]
        public string Id { get; set; }

        /// <summary>
        /// Gets or sets the project name.
        /// </summary>
        [JsonPropertyName("name")]
        public string Name { get; set; }

        /// <summary>
        /// Gets or sets the project URL.
        /// </summary>
        [JsonPropertyName("url")]
        public string Url { get; set; }

        /// <summary>
        /// Gets or sets the project state.
        /// </summary>
        [JsonPropertyName("state")]
        public string State { get; set; }

        /// <summary>
        /// Gets or sets the project revision number.
        /// </summary>
        [JsonPropertyName("revision")]
        public int Revision { get; set; }

        /// <summary>
        /// Gets or sets the project visibility.
        /// </summary>
        [JsonPropertyName("visibility")]
        public string Visibility { get; set; }

        /// <summary>
        /// Gets or sets the last update time of the project.
        /// </summary>
        [JsonPropertyName("lastUpdateTime")]
        public DateTime LastUpdateTime { get; set; }

        /// <inheritdoc/>
        public override string ToString()
        {
            return $"{this.Id} - {this.Name}";
        }
    }

    /// <summary>
    /// Represents an identity (user or service) in Azure DevOps.
    /// </summary>
    public class Identity
    {
        /// <summary>
        /// Gets or sets the display name.
        /// </summary>
        [JsonPropertyName("displayName")]
        public string Name { get; set; }

        /// <summary>
        /// Gets or sets the URL of the identity.
        /// </summary>
        [JsonPropertyName("url")]
        public string Url { get; set; }

        /// <summary>
        /// Gets or sets the identity ID.
        /// </summary>
        [JsonPropertyName("id")]
        public string Id { get; set; }

        /// <summary>
        /// Gets or sets the unique name.
        /// </summary>
        [JsonPropertyName("uniqueName")]
        public string UniqueName { get; set; }

        /// <summary>
        /// Gets or sets the image URL.
        /// </summary>
        [JsonPropertyName("imageUrl")]
        public string ImageUrl { get; set; }

        /// <summary>
        /// Gets or sets the descriptor.
        /// </summary>
        [JsonPropertyName("descriptor")]
        public string Descriptor { get; set; }

        /// <inheritdoc/>
        public override string ToString()
        {
            return $"{this.Id} - {this.Name}";
        }
    }

    /// <summary>
    /// Represents build logs information.
    /// </summary>
    public class Logs
    {
        /// <summary>
        /// Gets or sets the log ID.
        /// </summary>
        [JsonPropertyName("id")]
        public int Id { get; set; }

        /// <summary>
        /// Gets or sets the log type.
        /// </summary>
        [JsonPropertyName("type")]
        public string Type { get; set; }

        /// <summary>
        /// Gets or sets the log URL.
        /// </summary>
        [JsonPropertyName("url")]
        public string Url { get; set; }

        /// <inheritdoc/>
        public override string ToString()
        {
            return this.Id.ToString();
        }
    }

    /// <summary>
    /// Represents repository information for a build.
    /// </summary>
    public class Repository
    {
        /// <summary>
        /// Gets or sets the repository ID.
        /// </summary>
        [JsonPropertyName("id")]
        public string Id { get; set; }

        /// <summary>
        /// Gets or sets the repository type.
        /// </summary>
        [JsonPropertyName("type")]
        public string Type { get; set; }

        /// <summary>
        /// Gets or sets the repository name.
        /// </summary>
        [JsonPropertyName("name")]
        public string Name { get; set; }

        /// <summary>
        /// Gets or sets the repository URL.
        /// </summary>
        [JsonPropertyName("url")]
        public string Url { get; set; }

        /// <summary>
        /// Gets or sets a value indicating whether the repository is cleaned before build.
        /// </summary>
        [JsonPropertyName("clean")]
        public bool? Clean { get; set; }

        /// <summary>
        /// Gets or sets a value indicating whether submodules are checked out.
        /// </summary>
        [JsonPropertyName("checkoutSubmodules")]
        public bool CheckoutSubmodules { get; set; }

        /// <inheritdoc/>
        public override string ToString()
        {
            return $"{this.Id} - {this.Name}";
        }
    }
}
