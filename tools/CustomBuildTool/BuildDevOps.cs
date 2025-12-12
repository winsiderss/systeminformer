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
    public static class BuildDevOps
    {
        private static readonly string BaseBuild;
        private static readonly string BaseToken;
        private static readonly string BaseName;
        private static readonly string BaseUrl;

        static BuildDevOps()
        {
            BaseBuild = Win32.GetEnvironmentVariable("BUILD_BUILDID");
            BaseToken = Win32.GetEnvironmentVariable("SYSTEM_ACCESSTOKEN");
            BaseName = Win32.GetEnvironmentVariable("SYSTEM_TEAMPROJECT");
            BaseUrl = Win32.GetEnvironmentVariable("SYSTEM_COLLECTIONURI");
        }

        public static DateTime BuildQueryQueueTime()
        {
            if (string.IsNullOrWhiteSpace(BaseBuild) ||
                string.IsNullOrWhiteSpace(BaseToken) ||
                string.IsNullOrWhiteSpace(BaseName) ||
                string.IsNullOrWhiteSpace(BaseUrl))
            {
                Program.PrintColorMessage("One or more required environment variables are missing.", ConsoleColor.Red);
                return DateTime.MinValue;
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
                        return DateTime.MinValue;
                    }

                    var content = JsonSerializer.Deserialize(httpResult, BuildInfoResponseContext.Default.BuildInfo);
                    if (content == null)
                    {
                        Program.PrintColorMessage($"[ERROR] Failed to deserialize the response.", ConsoleColor.Red);
                        return DateTime.MinValue;
                    }

                    var offset = new DateTimeOffset(DateTime.SpecifyKind(content.QueueTime, DateTimeKind.Utc));

                    Program.PrintColorMessage($"Build QueueTime: {content.QueueTime}", ConsoleColor.Green);
                    Program.PrintColorMessage($"Build StartTime: {content.StartTime}", ConsoleColor.Green);
                    Program.PrintColorMessage($"Build Elapsed: {(DateTimeOffset.UtcNow - offset)}", ConsoleColor.Green);

                    return offset.DateTime;
                }
            }
            catch (Exception ex)
            {
                Program.PrintColorMessage($"[ERROR] {ex}", ConsoleColor.Red);
                return DateTime.MinValue;
            }
        }
    }

    [JsonSourceGenerationOptions(DefaultIgnoreCondition = JsonIgnoreCondition.WhenWritingNull, GenerationMode = JsonSourceGenerationMode.Default)]
    [JsonSerializable(typeof(BuildInfo))]
    public partial class BuildInfoResponseContext : JsonSerializerContext
    {

    }

    public class BuildInfo
    {
        [JsonPropertyName("_links")]
        public RepoLinks Links { get; set; }

        [JsonPropertyName("id")]
        public int Id { get; set; }

        [JsonPropertyName("buildNumber")]
        public string BuildNumber { get; set; }

        [JsonPropertyName("status")]
        public string Status { get; set; }

        [JsonPropertyName("queueTime")]
        public DateTime QueueTime { get; set; }

        [JsonPropertyName("startTime")]
        public DateTime StartTime { get; set; }

        [JsonPropertyName("url")]
        public string Url { get; set; }

        [JsonPropertyName("definition")]
        public Definition Definition { get; set; }

        [JsonPropertyName("project")]
        public Project Project { get; set; }

        [JsonPropertyName("uri")]
        public string Uri { get; set; }

        [JsonPropertyName("sourceBranch")]
        public string SourceBranch { get; set; }

        [JsonPropertyName("sourceVersion")]
        public string SourceVersion { get; set; }

        [JsonPropertyName("priority")]
        public string Priority { get; set; }

        [JsonPropertyName("reason")]
        public string Reason { get; set; }

        [JsonPropertyName("requestedFor")]
        public Identity RequestedFor { get; set; }

        [JsonPropertyName("requestedBy")]
        public Identity RequestedBy { get; set; }

        [JsonPropertyName("lastChangedDate")]
        public DateTime LastChangedDate { get; set; }

        [JsonPropertyName("lastChangedBy")]
        public Identity LastChangedBy { get; set; }

        [JsonPropertyName("parameters")]
        public string Parameters { get; set; }

        [JsonPropertyName("logs")]
        public Logs Logs { get; set; }

        [JsonPropertyName("repository")]
        public Repository Repository { get; set; }

        [JsonPropertyName("retainedByRelease")]
        public bool RetainedByRelease { get; set; }

        [JsonPropertyName("appendCommitMessageToRunName")]
        public bool AppendCommitMessageToRunName { get; set; }

        //[JsonPropertyName("orchestrationPlan")]
        //public OrchestrationPlan OrchestrationPlan { get; set; }

        //[JsonPropertyName("plans")]
        //public List<Plan> Plans { get; set; }
    }

    public class RepoLinks
    {
        [JsonPropertyName("self")]
        public Link Self { get; set; }

        [JsonPropertyName("web")]
        public Link Web { get; set; }

        [JsonPropertyName("sourceVersionDisplayUri")]
        public Link SourceVersionDisplayUri { get; set; }

        [JsonPropertyName("timeline")]
        public Link Timeline { get; set; }

        [JsonPropertyName("badge")]
        public Link Badge { get; set; }
    }

    public class Link
    {
        [JsonPropertyName("href")]
        public string Href { get; set; }
    }

    public class Definition
    {
        [JsonPropertyName("id")]
        public int Id { get; set; }

        [JsonPropertyName("name")]
        public string Name { get; set; }

        [JsonPropertyName("url")]
        public string Url { get; set; }

        [JsonPropertyName("uri")]
        public string Uri { get; set; }

        [JsonPropertyName("path")]
        public string Path { get; set; }

        [JsonPropertyName("type")]
        public string Type { get; set; }

        [JsonPropertyName("queueStatus")]
        public string QueueStatus { get; set; }

        [JsonPropertyName("revision")]
        public int Revision { get; set; }

        [JsonPropertyName("project")]
        public Project Project { get; set; }
    }

    public class Project
    {
        [JsonPropertyName("id")]
        public string Id { get; set; }

        [JsonPropertyName("name")]
        public string Name { get; set; }

        [JsonPropertyName("url")]
        public string Url { get; set; }

        [JsonPropertyName("state")]
        public string State { get; set; }

        [JsonPropertyName("revision")]
        public int Revision { get; set; }

        [JsonPropertyName("visibility")]
        public string Visibility { get; set; }

        [JsonPropertyName("lastUpdateTime")]
        public DateTime LastUpdateTime { get; set; }
    }

    public class Identity
    {
        [JsonPropertyName("displayName")]
        public string DisplayName { get; set; }

        [JsonPropertyName("url")]
        public string Url { get; set; }

        [JsonPropertyName("id")]
        public string Id { get; set; }

        [JsonPropertyName("uniqueName")]
        public string UniqueName { get; set; }

        [JsonPropertyName("imageUrl")]
        public string ImageUrl { get; set; }

        [JsonPropertyName("descriptor")]
        public string Descriptor { get; set; }
    }

    public class Logs
    {
        [JsonPropertyName("id")]
        public int Id { get; set; }

        [JsonPropertyName("type")]
        public string Type { get; set; }

        [JsonPropertyName("url")]
        public string Url { get; set; }
    }

    public class Repository
    {
        [JsonPropertyName("id")]
        public string Id { get; set; }

        [JsonPropertyName("type")]
        public string Type { get; set; }

        [JsonPropertyName("name")]
        public string Name { get; set; }

        [JsonPropertyName("url")]
        public string Url { get; set; }

        [JsonPropertyName("clean")]
        public bool? Clean { get; set; }

        [JsonPropertyName("checkoutSubmodules")]
        public bool CheckoutSubmodules { get; set; }
    }
}
