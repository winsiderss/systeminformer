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
    /// Provides methods for retrieving NuGet package metadata, including the latest available versions, using NuGet v3 APIs.
    /// </summary>
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
                    var doc = await JsonSerializer.DeserializeAsync(stream, NugetJsonContext.Default.NugetServiceIndexResponse, CancellationToken);

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
                await foreach (var version in GetVersionsFromRegistrations(httpClient, registrationsBase, PackageId, CancellationToken))
                {
                    versions.Add(version);
                }
            }

            if (versions.Count == 0 && !string.IsNullOrWhiteSpace(searchQueryService))
            {
                await foreach (var version in GetVersionsFromSearch(httpClient, searchQueryService, PackageId, CancellationToken))
                {
                    versions.Add(version);
                }
            }

            if (versions.Count == 0)
            {
                await foreach (var version in GetVersionsFromFlatContainer(httpClient, PackageId, CancellationToken))
                {
                    versions.Add(version);
                }
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
        private static async IAsyncEnumerable<string> GetVersionsFromRegistrations(
            HttpClient HttpClient,
            string RegistrationsBase,
            string PackageId,
            [EnumeratorCancellation] CancellationToken CancellationToken)
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
                    index = await JsonSerializer.DeserializeAsync(stream, NugetJsonContext.Default.NugetRegistrationIndexResponse, CancellationToken);
                }
            }
            catch
            {
                index = null;
            }

            if (index?.Items == null)
                yield break;

            foreach (var page in index.Items)
            {
                if (page.Items != null && page.Items.Length != 0)
                {
                    foreach (var item in page.Items)
                    {
                        if (!string.IsNullOrWhiteSpace(item.CatalogEntry?.Version))
                            yield return item.CatalogEntry.Version.Trim();
                    }
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
                        pageResponse = await JsonSerializer.DeserializeAsync(stream, NugetJsonContext.Default.NugetRegistrationPageResponse, CancellationToken);
                    }
                }
                catch
                {
                    pageResponse = null;
                }

                if (pageResponse?.Items != null)
                {
                    foreach (var item in pageResponse.Items)
                    {
                        if (!string.IsNullOrWhiteSpace(item.CatalogEntry?.Version))
                            yield return item.CatalogEntry.Version.Trim();
                    }
                }
            }
        }

        private static async IAsyncEnumerable<string> GetVersionsFromSearch(
            HttpClient HttpClient,
            string SearchQueryService,
            string PackageId,
            [EnumeratorCancellation] CancellationToken CancellationToken)
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
                    search = await JsonSerializer.DeserializeAsync(stream, NugetJsonContext.Default.NugetSearchResponse, CancellationToken);
                }
            }
            catch
            {
                search = null;
            }

            if (search?.Data == null)
                yield break;

            foreach (var package in search.Data)
            {
                if (!string.Equals(package.Id, PackageId, StringComparison.OrdinalIgnoreCase))
                    continue;

                if (package.Versions != null)
                {
                    foreach (var v in package.Versions)
                        if (!string.IsNullOrWhiteSpace(v.Version))
                            yield return v.Version.Trim();
                }

                if (!string.IsNullOrWhiteSpace(package.Version))
                    yield return package.Version.Trim();
            }
        }

        private static async IAsyncEnumerable<string> GetVersionsFromFlatContainer(
            HttpClient HttpClient,
            string PackageId,
            [EnumeratorCancellation] CancellationToken CancellationToken)
        {
            var versionsUri = $"{NugetMetadataClient.FlatContainerBaseUri}/{PackageId.ToLowerInvariant()}/index.json";
            NugetFlatContainerResponse container = null;

            try
            {
                using var request = new HttpRequestMessage(HttpMethod.Get, versionsUri);
                using var httpResponse = await HttpClient.SendAsync(request, CancellationToken);

                if (httpResponse.IsSuccessStatusCode)
                {
                    await using var stream = await httpResponse.Content.ReadAsStreamAsync(CancellationToken);
                    container = await JsonSerializer.DeserializeAsync(stream, NugetJsonContext.Default.NugetFlatContainerResponse, CancellationToken);
                }
            }
            catch
            {
                container = null;
            }

            if (container?.Versions == null)
                yield break;

            foreach (var version in container.Versions)
            {
                if (!string.IsNullOrWhiteSpace(version))
                    yield return version.Trim();
            }
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

        /// <summary>
        /// Represents a NuGet-compatible semantic version, supporting parsing and comparison according to semantic
        /// versioning rules.
        /// </summary>
        public sealed class NugetSemVer : IComparable<NugetSemVer>
        {
            private static readonly SearchValues<char> SemVerDelimiters = SearchValues.Create(".+-");

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

                ReadOnlySpan<char> span = Value.AsSpan();
                int major = 0, minor = 0, patch = 0;
                string prerelease = null;
                var extra = new List<int>();

                int partIndex = 0;
                while (!span.IsEmpty)
                {
                    int nextDelimiter = span.IndexOfAny(SemVerDelimiters);
                    ReadOnlySpan<char> part = nextDelimiter >= 0 ? span[..nextDelimiter] : span;
                    char delimiter = nextDelimiter >= 0 ? span[nextDelimiter] : '\0';

                    if (partIndex == 0)
                    {
                        if (!int.TryParse(part, out major)) return false;
                    }
                    else if (partIndex == 1)
                    {
                        if (!int.TryParse(part, out minor)) return false;
                    }
                    else if (partIndex == 2)
                    {
                        if (!int.TryParse(part, out patch)) return false;
                    }
                    else if (partIndex > 2 && delimiter == '.')
                    {
                        if (int.TryParse(part, out int ex)) extra.Add(ex);
                    }

                    if (delimiter == '-')
                    {
                        span = span[(nextDelimiter + 1)..];
                        int buildIndex = span.IndexOf('+');
                        prerelease = (buildIndex >= 0 ? span[..buildIndex] : span).ToString();
                        break;
                    }

                    if (delimiter == '+') break;

                    if (nextDelimiter < 0) break;
                    span = span[(nextDelimiter + 1)..];
                    partIndex++;
                }

                if (partIndex < 1) return false;

                Result = new NugetSemVer
                {
                    Original = Value,
                    Major = major,
                    Minor = minor,
                    Patch = patch,
                    ExtraNumbers = extra.ToArray(),
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
    [JsonSerializable(typeof(NugetFlatContainerResponse))]
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
