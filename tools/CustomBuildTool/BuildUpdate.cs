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
    public static class NugetMetadataClient
    {
        private const string ServiceIndexUri = "https://api.nuget.org/v3/index.json";
        private const string FlatContainerBaseUri = "https://api.nuget.org/v3-flatcontainer";

        /// <summary>
        /// Gets the latest package version from NuGet using service index discovery.
        /// Query order: RegistrationsBaseUrl -> SearchQueryService -> flat-container fallback.
        /// </summary>
        /// <param name="PackageId">Package ID.</param>
        /// <param name="CancellationToken">Cancellation token.</param>
        /// <returns>Latest stable version if available; otherwise latest prerelease; null if not found.</returns>
        public static async Task<string> GetLatestVersion(string PackageId, CancellationToken CancellationToken = default)
        {
            if (string.IsNullOrWhiteSpace(PackageId))
                return null;

            using var httpClient = BuildHttpClient.CreateHttpClient();
            var versions = new HashSet<string>(StringComparer.OrdinalIgnoreCase);
            string registrationsBase = null;
            string searchQueryService = null;

            try
            {
                using var request = new HttpRequestMessage(HttpMethod.Get, ServiceIndexUri);
                using var httpResponse = await httpClient.SendAsync(request, CancellationToken);

                if (httpResponse.IsSuccessStatusCode)
                {
                    await using var stream = await httpResponse.Content.ReadAsStreamAsync(CancellationToken);
                    var doc = JsonSerializer.Deserialize(stream, NugetJsonContext.Default.NugetServiceIndexResponse);

                    if (doc.Resources != null)
                    {
                        foreach (var resource in doc.Resources)
                        {
                            var id = resource.Id;

                            if (string.IsNullOrWhiteSpace(id))
                                continue;

                            if (ResourceTypeContains(resource.Type, "RegistrationsBaseUrl") && string.IsNullOrWhiteSpace(registrationsBase))
                                registrationsBase = id;

                            if (ResourceTypeContains(resource.Type, "SearchQueryService") && string.IsNullOrWhiteSpace(searchQueryService))
                                searchQueryService = id;
                        }
                    }
                }
            }
            catch
            {
                // ignore
            }

            if (!string.IsNullOrWhiteSpace(registrationsBase))
            {
                await AddVersionsFromRegistrations(httpClient, registrationsBase, PackageId, versions, CancellationToken);
            }

            if (versions.Count == 0 && !string.IsNullOrWhiteSpace(searchQueryService))
            {
                await AddVersionsFromSearch(httpClient, searchQueryService, PackageId, versions, CancellationToken);
            }

            if (versions.Count == 0)
            {
                await AddVersionsFromFlatContainer(httpClient, PackageId, versions, CancellationToken);
            }

            return SelectBestVersion(versions);
        }

        /// <summary>
        /// Convenience wrapper for Microsoft.Windows.SDK.CPP.x64 package.
        /// </summary>
        public static Task<string> GetLatestBuildToolsVersion(CancellationToken CancellationToken = default)
        {
            return GetLatestVersion("Microsoft.Windows.SDK.BuildTools", CancellationToken);
        }

        /// <summary>
        /// Convenience wrapper for Microsoft.Windows.SDK.CPP.x64 package.
        /// </summary>
        public static Task<string> GetLatestWindowsSdkVersion(CancellationToken CancellationToken = default)
        {
            return GetLatestVersion("Microsoft.Windows.SDK.CPP", CancellationToken);
        }

        /// <summary>
        /// Convenience wrapper for Microsoft.Windows.SDK.CPP.x64 package.
        /// </summary>
        public static Task<string> GetLatestWindowsSdkx64Version(CancellationToken CancellationToken = default)
        {
            return GetLatestVersion("Microsoft.Windows.SDK.CPP.x64", CancellationToken);
        }

        /// <summary>
        /// Queries NuGet registrations API to retrieve all versions of a package.
        /// </summary>
        /// <param name="HttpClient">HTTP client for making requests.</param>
        /// <param name="RegistrationsBase">Base URL for registrations API.</param>
        /// <param name="PackageId">Package ID to query.</param>
        /// <param name="Versions">Set to populate with found versions.</param>
        /// <param name="CancellationToken">Cancellation token.</param>
        private static async Task AddVersionsFromRegistrations(
            HttpClient HttpClient,
            string RegistrationsBase,
            string PackageId,
            HashSet<string> Versions,
            CancellationToken CancellationToken)
        {
            var registrationIndexUri = $"{RegistrationsBase.TrimEnd('/')}/{PackageId.ToLowerInvariant()}/index.json";
            NugetRegistrationIndexResponse index = null;

            try
            {
                using var request = new HttpRequestMessage(HttpMethod.Get, registrationIndexUri);
                using var httpResponse = await HttpClient.SendAsync(request, CancellationToken);

                if (httpResponse.IsSuccessStatusCode)
                {
                    await using var stream = await httpResponse.Content.ReadAsStreamAsync(CancellationToken);
                    index = JsonSerializer.Deserialize(stream, NugetJsonContext.Default.NugetRegistrationIndexResponse);
                }
                else
                {
                    index = null;
                }
            }
            catch
            {
                index = null;
            }

            if (index?.Items == null)
                return;

            foreach (var page in index?.Items)
            {
                if (page.Items != null && page.Items.Length != 0)
                {
                    AddVersions(Versions, page.Items.Select(i => i.CatalogEntry.Version));
                    continue;
                }

                if (string.IsNullOrWhiteSpace(page.Id))
                    continue;

                NugetRegistrationPageResponse pageResponse = null;
                try
                {
                    using var request = new HttpRequestMessage(HttpMethod.Get, page.Id);
                    using var httpResponse = await HttpClient.SendAsync(request, CancellationToken);

                    if (httpResponse.IsSuccessStatusCode)
                    {
                        await using var stream = await httpResponse.Content.ReadAsStreamAsync(CancellationToken);
                        pageResponse = JsonSerializer.Deserialize(stream, NugetJsonContext.Default.NugetRegistrationPageResponse);
                    }
                    else
                    {
                        pageResponse = null;
                    }
                }
                catch
                {
                    pageResponse = null;
                }

                AddVersions(Versions, pageResponse?.Items?.Select(i => i.CatalogEntry.Version));
            }
        }

        /// <summary>
        /// Queries NuGet search API to retrieve all versions of a package.
        /// </summary>
        /// <param name="HttpClient">HTTP client for making requests.</param>
        /// <param name="SearchQueryService">Base URL for search API.</param>
        /// <param name="PackageId">Package ID to query.</param>
        /// <param name="Versions">Set to populate with found versions.</param>
        /// <param name="CancellationToken">Cancellation token.</param>
        private static async Task AddVersionsFromSearch(
            HttpClient HttpClient,
            string SearchQueryService,
            string PackageId,
            HashSet<string> Versions,
            CancellationToken CancellationToken)
        {
            var searchUri = $"{SearchQueryService.TrimEnd('/')}?q=packageid:{Uri.EscapeDataString(PackageId)}&prerelease=true&take=20";
            NugetSearchResponse search = null;

            try
            {
                using var request = new HttpRequestMessage(HttpMethod.Get, searchUri);
                using var httpResponse = await HttpClient.SendAsync(request, CancellationToken);

                if (httpResponse.IsSuccessStatusCode)
                {
                    await using var stream = await httpResponse.Content.ReadAsStreamAsync(CancellationToken);
                    search = JsonSerializer.Deserialize(stream, NugetJsonContext.Default.NugetSearchResponse);
                }
                else
                {
                    search = null;
                }
            }
            catch
            {
                search = null;
            }

            if (search?.Data == null)
                return;

            foreach (var package in search?.Data)
            {
                if (!string.Equals(package.Id, PackageId, StringComparison.OrdinalIgnoreCase))
                    continue;

                AddVersions(Versions, package.Versions?.Select(v => v.Version));
                AddVersions(Versions, new[] { package.Version });
            }
        }

        /// <summary>
        /// Queries NuGet flat container API to retrieve all versions of a package.
        /// </summary>
        /// <param name="HttpClient">HTTP client for making requests.</param>
        /// <param name="PackageId">Package ID to query.</param>
        /// <param name="Versions">Set to populate with found versions.</param>
        /// <param name="CancellationToken">Cancellation token.</param>
        private static async Task AddVersionsFromFlatContainer(
            HttpClient HttpClient,
            string PackageId,
            HashSet<string> Versions,
            CancellationToken CancellationToken)
        {
            NugetVersionResponse response;

            try
            {
                using var request = new HttpRequestMessage(HttpMethod.Get, $"{FlatContainerBaseUri}/{PackageId.ToLowerInvariant()}/index.json");
                using var httpResponse = await HttpClient.SendAsync(request, CancellationToken);

                if (!httpResponse.IsSuccessStatusCode)
                {
                    response = null;
                }
                else
                {
                    await using var stream = await httpResponse.Content.ReadAsStreamAsync(CancellationToken);
                    response = JsonSerializer.Deserialize(stream, NugetJsonContext.Default.NugetVersionResponse);
                }
            }
            catch
            {
                response = null;
            }

            AddVersions(Versions, response?.Versions);
        }

        /// <summary>
        /// Adds non-empty version strings from source collection to target set.
        /// </summary>
        /// <param name="Target">Target set to add versions to.</param>
        /// <param name="Source">Source collection of version strings.</param>
        private static void AddVersions(HashSet<string> Target, IEnumerable<string> Source)
        {
            if (Target == null || Source == null)
                return;

            foreach (var version in Source)
            {
                if (!string.IsNullOrWhiteSpace(version))
                {
                    Target.Add(version);
                }
            }
        }

        /// <summary>
        /// Retrieves the endpoint URL for a specific resource type from the NuGet service index.
        /// </summary>
        /// <param name="ServiceIndex">NuGet service index response.</param>
        /// <param name="ResourceType">Resource type to find.</param>
        /// <returns>Resource endpoint URL; null if not found.</returns>
        private static string GetResourceEndpoint(NugetServiceIndexResponse ServiceIndex, string ResourceType)
        {
            if (ServiceIndex.Resources == null)
                return null;

            foreach (var resource in ServiceIndex.Resources)
            {
                if (!string.IsNullOrWhiteSpace(resource.Id) && ResourceTypeContains(resource.Type, ResourceType))
                    return resource.Id;
            }

            return null;
        }

        /// <summary>
        /// Checks if a JSON element (string or array of strings) contains a specific value.
        /// </summary>
        /// <param name="Type">JSON element to check.</param>
        /// <param name="Value">Value to search for.</param>
        /// <returns>True if value is found; false otherwise.</returns>
        private static bool ResourceTypeContains(string Type, string Value)
        {
            return Type?.Contains(Value, StringComparison.OrdinalIgnoreCase) == true;
        }

        /// <summary>
        /// Selects the best version from a collection, preferring the latest stable version or latest prerelease if no stable version exists.
        /// </summary>
        /// <param name="Versions">Collection of version strings.</param>
        /// <returns>Best version string; null if collection is empty.</returns>
        private static string SelectBestVersion(IEnumerable<string> Versions)
        {
            var ordered = Versions
                .Where(v => !string.IsNullOrWhiteSpace(v))
                .Select(v => NugetSemVer.TryParse(v, out var semVer) ? semVer : null)
                .Where(v => v != null)
                .OrderBy(v => v)
                .ToList();

            if (ordered.Count == 0)
                return null;

            var stable = ordered.LastOrDefault(v => !v.IsPrerelease);
            return (stable ?? ordered[^1]).Original;
        }

        public sealed class NugetSemVer : IComparable<NugetSemVer>
        {
            public string Original { get; init; }
            public int Major { get; init; }
            public int Minor { get; init; }
            public int Patch { get; init; }
            public int[] ExtraNumbers { get; init; }
            public string Prerelease { get; init; }
            public bool IsPrerelease => !string.IsNullOrEmpty(Prerelease);

            /// <summary>
            /// Attempts to parse a semantic version string into a NugetSemVer object.
            /// </summary>
            /// <param name="Value">Version string to parse.</param>
            /// <param name="Result">Parsed result if successful; null otherwise.</param>
            /// <returns>True if parsing succeeded; false otherwise.</returns>
            public static bool TryParse(string Value, out NugetSemVer Result)
            {
                Result = null;

                if (string.IsNullOrWhiteSpace(Value))
                    return false;

                var plusIndex = Value.IndexOf('+');
                var trimmed = plusIndex >= 0 ? Value[..plusIndex] : Value;
                var dashIndex = trimmed.IndexOf('-');
                var numeric = dashIndex >= 0 ? trimmed[..dashIndex] : trimmed;
                var prerelease = dashIndex >= 0 ? trimmed[(dashIndex + 1)..] : null;

                var parts = numeric.Split('.', StringSplitOptions.RemoveEmptyEntries);
                if (parts.Length < 2)
                    return false;

                if (!int.TryParse(parts[0], out var major))
                    return false;
                if (!int.TryParse(parts[1], out var minor))
                    return false;

                var patch = 0;
                if (parts.Length >= 3 && !int.TryParse(parts[2], out patch))
                    return false;

                var extra = Array.Empty<int>();
                if (parts.Length > 3)
                {
                    extra = new int[parts.Length - 3];
                    for (var i = 3; i < parts.Length; i++)
                    {
                        if (!int.TryParse(parts[i], out extra[i - 3]))
                            return false;
                    }
                }

                Result = new NugetSemVer
                {
                    Original = Value,
                    Major = major,
                    Minor = minor,
                    Patch = patch,
                    ExtraNumbers = extra,
                    Prerelease = prerelease
                };

                return true;
            }

            /// <summary>
            /// Compares this version to another version using semantic versioning rules.
            /// Stable versions are considered greater than prerelease versions with the same base version.
            /// </summary>
            /// <param name="Other">Version to compare to.</param>
            /// <returns>Negative if this is less than other; zero if equal; positive if greater.</returns>
            public int CompareTo(NugetSemVer Other)
            {
                if (Other == null)
                    return 1;

                var cmp = Major.CompareTo(Other.Major);
                if (cmp != 0) return cmp;

                cmp = Minor.CompareTo(Other.Minor);
                if (cmp != 0) return cmp;

                cmp = Patch.CompareTo(Other.Patch);
                if (cmp != 0) return cmp;

                var max = Math.Max(ExtraNumbers.Length, Other.ExtraNumbers.Length);
                for (var i = 0; i < max; i++)
                {
                    var left = i < ExtraNumbers.Length ? ExtraNumbers[i] : 0;
                    var right = i < Other.ExtraNumbers.Length ? Other.ExtraNumbers[i] : 0;
                    cmp = left.CompareTo(right);
                    if (cmp != 0) return cmp;
                }

                if (!IsPrerelease && Other.IsPrerelease) return 1;
                if (IsPrerelease && !Other.IsPrerelease) return -1;

                return string.Compare(Prerelease, Other.Prerelease, StringComparison.OrdinalIgnoreCase);
            }

            public override string ToString()
            {
                return this.Original;
            }
        }
    }

    [JsonSourceGenerationOptions(DefaultIgnoreCondition = JsonIgnoreCondition.WhenWritingNull, GenerationMode = JsonSourceGenerationMode.Default)]
    [JsonSerializable(typeof(OpenAiResponse))]
    [JsonSerializable(typeof(GeminiResponse))]
    [JsonSerializable(typeof(NugetVersionResponse))]
    [JsonSerializable(typeof(NugetServiceIndexResponse))]
    [JsonSerializable(typeof(NugetRegistrationIndexResponse))]
    [JsonSerializable(typeof(NugetRegistrationPageResponse))]
    [JsonSerializable(typeof(NugetSearchResponse))]
    internal partial class NugetJsonContext : JsonSerializerContext
    {

    }

    internal class OpenAiResponse
    {
        [JsonPropertyName("choices")]
        public OpenAiChoice[] Choices { get; init; }
    }

    internal class OpenAiChoice
    {
        [JsonPropertyName("message")]
        public OpenAiMessage Message { get; init; }
    }

    internal class OpenAiMessage
    {
        [JsonPropertyName("content")]
        public string Content { get; init; }
    }

    internal class GeminiResponse
    {
        [JsonPropertyName("candidates")]
        public GeminiCandidate[] Candidates { get; init; }
    }

    internal class GeminiCandidate
    {
        [JsonPropertyName("content")]
        public GeminiContent Content { get; init; }
    }

    internal class GeminiContent
    {
        [JsonPropertyName("parts")]
        public GeminiPart[] Parts { get; init; }
    }

    internal class GeminiPart
    {
        [JsonPropertyName("text")]
        public string Text { get; init; }
    }

    internal class NugetSearchVersionResponse
    {
        [JsonPropertyName("version")]
        public string Version { get; init; }
    }

    internal class NugetRegistrationCatalogEntryResponse
    {
        [JsonPropertyName("@id")]
        public string Id { get; init; }

        [JsonPropertyName("id")]
        public string Id2 { get; init; }

        [JsonPropertyName("@type")]
        public string Type { get; init; }

        [JsonPropertyName("packageContent")]
        public string PackageContent { get; init; }

        [JsonPropertyName("published")]
        public string Published { get; init; }

        [JsonPropertyName("version")]
        public string Version { get; init; }

        public override string ToString()
        {
            return $"{this.Id} [{this.Version}] [{this.Published}]";
        }
    }

    internal class NugetServiceIndexResource
    {
        [JsonPropertyName("@id")]
        public string Id { get; init; }

        [JsonPropertyName("@type")]
        public string Type { get; init; }

        public override string ToString()
        {
            return $"{this.Id} [{this.Type}]";
        }
    }

    internal class NugetRegistrationLeafResponse
    {
        [JsonPropertyName("@id")]
        public string Id { get; init; }

        [JsonPropertyName("commitId")]
        public string CommitId { get; init; }

        [JsonPropertyName("commitTimeStamp")]
        public string CommitTimeStamp { get; init; }

        [JsonPropertyName("catalogEntry")]
        public NugetRegistrationCatalogEntryResponse CatalogEntry { get; init; }

        public override string ToString()
        {
            return $"{this.Id} [{this.CommitTimeStamp}] [{this.CommitId}]";
        }
    }

    internal class NugetSearchPackageResponse
    {
        [JsonPropertyName("id")]
        public string Id { get; init; }

        [JsonPropertyName("version")]
        public string Version { get; init; }

        [JsonPropertyName("versions")]
        public NugetSearchVersionResponse[] Versions { get; init; }

        public override string ToString()
        {
            return $"{this.Id} [{this.Version}]";
        }
    }

    internal class NugetServiceIndexResponse
    {
        [JsonPropertyName("resources")]
        public NugetServiceIndexResource[] Resources { get; init; }
    }

    internal class NugetRegistrationIndexResponse
    {
        [JsonPropertyName("items")]
        public NugetRegistrationPageResponse[] Items { get; init; }
    }

    internal class NugetRegistrationPageResponse
    {
        [JsonPropertyName("@id")]
        public string Id { get; init; }

        [JsonPropertyName("commitId")]
        public string CommitId { get; init; }

        [JsonPropertyName("commitTimeStamp")]
        public string CommitTimeStamp { get; init; }

        [JsonPropertyName("items")]
        public NugetRegistrationLeafResponse[] Items { get; init; }

        public override string ToString()
        {
            return $"{this.Id} [{this.CommitTimeStamp}] [{this.CommitId}]";
        }
    }

    internal class NugetSearchResponse
    {
        [JsonPropertyName("data")]
        public NugetSearchPackageResponse[] Data { get; init; }
    }
}
