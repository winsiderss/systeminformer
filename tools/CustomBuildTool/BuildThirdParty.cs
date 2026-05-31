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
    /// Provides methods for checking the versions of vendored thirdparty libraries against their latest GitHub releases.
    /// </summary>
    public static partial class BuildThirdParty
    {
        private static readonly string ThirdPartyDirectory = "tools\\thirdparty";
        private static readonly ThirdPartyLibrary[] Libraries =
        [
            new("detours",    "microsoft",       "Detours",       "detours\\detours.h",          @"#define\s+DETOURS_VERSION\s+(0x[0-9a-fA-F]+)"),
            new("jsonc",      "json-c",          "json-c",        "jsonc\\json_c_version.h",     @"#define\s+JSON_C_VERSION\s+""([^""]+)"""),
            new("maxminddb",  "maxmind",         "libmaxminddb",  "maxminddb\\maxminddb.h",      @"#define\s+PACKAGE_VERSION\s+""([^""]+)"""),
            new("miniz",      "richgel999",      "miniz",         "miniz\\miniz.h",              @"miniz\.c\s+(\d+(?:\.\d+)+)"),
            new("mxml",       "michaelrsweet",   "mxml",          "mxml\\config.h",              @"#\s*define\s+MXML_VERSION\s+""Mini-XML\s+v([^""]+)"""),
            new("pcre2",      "PCRE2Project",    "pcre2",         "pcre\\pcre2.h",               @"#define\s+PCRE2_MAJOR\s+(\d+)"),
            new("ssdeep",     "ssdeep-project",  "ssdeep",        "ssdeep\\fuzzy.h",             null),
            new("tlsh",       "trendmicro",      "tlsh",          "tlsh\\tlsh_win_version.h",    @"#define\s+VERSION_MAJOR\s+(\d+)"),
            new("xxhash",     "Cyan4973",        "xxHash",        "xxhash\\xxhash.h",            @"#define\s+XXH_VERSION_MAJOR\s+(\d+)"),
            new("zydis",      "zyantific",       "zydis",         "zydis\\Zydis.h",              @"#define\s+ZYDIS_VERSION\s+\(ZyanU64\)(0x[0-9a-fA-F]+)"),
        ];

        /// <summary>
        /// Checks all vendored thirdparty libraries against their latest GitHub releases and reports any that are out of date.
        /// </summary>
        public static async Task CheckThirdPartyVersions()
        {
            Program.PrintColorMessage("Checking thirdparty library versions...", ConsoleColor.Cyan);

            using var httpClient = BuildHttpClient.CreateHttpClient();
            httpClient.DefaultRequestHeaders.Accept.Add(new MediaTypeWithQualityHeaderValue("application/vnd.github+json"));
            httpClient.DefaultRequestHeaders.TryAddWithoutValidation("X-GitHub-Api-Version", "2022-11-28");

            string githubToken = Win32.GetEnvironmentVariable("GITHUB_TOKEN");
            if (!string.IsNullOrWhiteSpace(githubToken))
            {
                httpClient.DefaultRequestHeaders.Authorization = new AuthenticationHeaderValue("Bearer", githubToken);
            }

            bool anyOutdated = false;
            bool anyUnknown = false;

            foreach (var lib in Libraries)
            {
                string localVersion = GetLocalVersion(lib);
                string latestTag = await GetLatestGithubTag(httpClient, lib.GithubOwner, lib.GithubRepo);

                if (string.IsNullOrWhiteSpace(latestTag))
                {
                    anyUnknown = true;
                    Console.WriteLine($"{VT.YELLOW}{lib.Name,-14} local={localVersion ?? "unknown",-20} latest=fetch failed{VT.RESET}");
                    continue;
                }

                string latestVersion = NormalizeVersion(latestTag);

                if (string.IsNullOrWhiteSpace(localVersion))
                {
                    anyUnknown = true;
                    Console.WriteLine($"{VT.YELLOW}{lib.Name,-14} local=unknown        latest={latestTag}{VT.RESET}");
                    continue;
                }

                bool upToDate = IsUpToDate(localVersion, latestVersion);

                if (upToDate)
                {
                    Console.WriteLine($"{VT.GREEN}{lib.Name,-14} {localVersion,-20} up to date ({latestTag}){VT.RESET}");
                }
                else
                {
                    anyOutdated = true;
                    Console.WriteLine($"{VT.RED}{lib.Name,-14} {localVersion,-20} OUTDATED -> {latestTag}{VT.RESET}");
                }
            }

            Console.WriteLine();

            if (anyOutdated)
                Console.WriteLine($"{VT.YELLOW}One or more thirdparty libraries are out of date.{VT.RESET}");
            else if (anyUnknown)
                Console.WriteLine($"{VT.YELLOW}One or more thirdparty library versions could not be checked.{VT.RESET}");
            else
                Console.WriteLine($"{VT.GREEN}All thirdparty libraries are up to date.{VT.RESET}");
        }

        /// <summary>
        /// Retrieves the latest release tag from a GitHub repository using the GitHub API.
        /// </summary>
        /// <param name="HttpClient">The HTTP client used to send requests.</param>
        /// <param name="Owner">The GitHub repository owner.</param>
        /// <param name="Repo">The GitHub repository name.</param>
        /// <returns>The latest release tag, or null if not found.</returns>
        private static async Task<string> GetLatestGithubTag(HttpClient HttpClient, string Owner, string Repo)
        {
            try
            {
                using (var request = new HttpRequestMessage(HttpMethod.Get, $"https://api.github.com/repos/{Owner}/{Repo}/releases/latest"))
                using (var response = await BuildHttpClient.SendMessageResponse(HttpClient, request))
                {
                    if (response?.IsSuccessStatusCode == true)
                    {
                        await using var stream = await response.Content.ReadAsStreamAsync();
                        var release = await JsonSerializer.DeserializeAsync(stream, GithubResponseContext.Default.GithubReleasesResponse);

                        if (!string.IsNullOrWhiteSpace(release?.TagName))
                            return release.TagName;
                    }
                }

                using (var request = new HttpRequestMessage(HttpMethod.Get, $"https://api.github.com/repos/{Owner}/{Repo}/tags?per_page=1"))
                using (var response = await BuildHttpClient.SendMessageResponse(HttpClient, request))
                {
                    if (response?.IsSuccessStatusCode == true)
                    {
                        await using var stream = await response.Content.ReadAsStreamAsync();
                        using var document = await JsonDocument.ParseAsync(stream);

                        if (
                            document.RootElement.ValueKind == JsonValueKind.Array &&
                            document.RootElement.GetArrayLength() > 0 &&
                            document.RootElement[0].TryGetProperty("name", out var tagName) &&
                            tagName.ValueKind == JsonValueKind.String
                            )
                        {
                            return tagName.GetString();
                        }
                    }
                }
            }
            catch (Exception ex)
            {
                Console.WriteLine($"{VT.RED}GetLatestGithubTag: {ex}{VT.RESET}");
            }

            return null;
        }

        /// <summary>
        /// Retrieves the local version of a specified third-party library based on its version pattern.
        /// </summary>
        /// <param name="Lib">The third-party library for which the local version is being retrieved.</param>
        /// <returns>The local version as a string, or null if the version cannot be determined.</returns>
        private static string GetLocalVersion(ThirdPartyLibrary Lib)
        {
            if (string.IsNullOrWhiteSpace(Lib.VersionPattern))
                return null;

            string headerPath = Path.Combine(ThirdPartyDirectory, Lib.HeaderFile);
            if (!File.Exists(headerPath))
                return null;

            string content = Utils.ReadAllText(headerPath);
            if (string.IsNullOrWhiteSpace(content))
                return null;

            var match = Regex.Match(content, Lib.VersionPattern, RegexOptions.Multiline);
            if (!match.Success || match.Groups.Count < 2)
                return null;

            string value = match.Groups[1].Value.Trim();

            // detours encode version as 0xMAJORcMINORcPATCH where 'c' is the hex digit 0xC as separator
            // e.g., 0x4c0c1 → "4" "c" "0" "c" "1" → 4.0.1
            if (
                Lib.Name.Equals("detours", StringComparison.OrdinalIgnoreCase) && 
                value.StartsWith("0x", StringComparison.OrdinalIgnoreCase)
                )
            {
                ReadOnlySpan<char> hex = value.AsSpan(2);
                int major = -1, minor = -1, patch = -1;
                int partIndex = 0;

                while (!hex.IsEmpty)
                {
                    int nextC = hex.IndexOf('C');
                    ReadOnlySpan<char> part = nextC >= 0 ? hex[..nextC] : hex;

                    if (partIndex == 0)
                    {
                        if (!uint.TryParse(part, NumberStyles.HexNumber, null, out uint val)) break;
                        major = (int)val;
                    }
                    else if (partIndex == 1)
                    {
                        if (!uint.TryParse(part, NumberStyles.HexNumber, null, out uint val)) break;
                        minor = (int)val;
                    }
                    else if (partIndex == 2)
                    {
                        if (!uint.TryParse(part, NumberStyles.HexNumber, null, out uint val)) break;
                        patch = (int)val;
                    }

                    if (nextC < 0) break;
                    hex = hex[(nextC + 1)..];
                    partIndex++;
                }

                if (major != -1 && minor != -1 && patch != -1)
                {
                    return $"{major}.{minor}.{patch}";
                }
            }

            if (
                Lib.Name.Equals("zydis", StringComparison.OrdinalIgnoreCase) &&
                value.StartsWith("0x", StringComparison.OrdinalIgnoreCase) &&
                ulong.TryParse(value.AsSpan(2), NumberStyles.HexNumber, null, out ulong zydisVersion)
                )
            {
                var major = (zydisVersion & 0xFFFF000000000000) >> 48;
                var minor = (zydisVersion & 0x0000FFFF00000000) >> 32;
                var patch = (zydisVersion & 0x00000000FFFF0000) >> 16;
                var build = zydisVersion & 0x000000000000FFFF;

                if (build != 0)
                    return $"{major}.{minor}.{patch}.{build}";

                return $"{major}.{minor}.{patch}";
            }

            // pcre2 only captures major; read minor from next line
            if (Lib.Name.Equals("pcre2", StringComparison.OrdinalIgnoreCase))
            {
                var minorMatch = PcreVersionRegex().Match(content);
                if (minorMatch.Success)
                    return $"{value}.{minorMatch.Groups[1].Value}";
            }

            // tlsh encodes as separate MAJOR/MINOR/PATCH defines
            if (Lib.Name.Equals("tlsh", StringComparison.OrdinalIgnoreCase))
            {
                var minorMatch = TlshVersionMinorRegex().Match(content);
                var patchMatch = TlshVersionPatchRegex().Match(content);
                if (minorMatch.Success && patchMatch.Success)
                    return $"{value}.{minorMatch.Groups[1].Value}.{patchMatch.Groups[1].Value}";
            }

            // xxhash encodes as separate MAJOR/MINOR/RELEASE defines
            if (Lib.Name.Equals("xxhash", StringComparison.OrdinalIgnoreCase))
            {
                var minorMatch = XxHashVersionMinorRegex().Match(content);
                var releaseMatch = XxHashVersionReleaseRegex().Match(content);
                if (minorMatch.Success && releaseMatch.Success)
                    return $"{value}.{minorMatch.Groups[1].Value}.{releaseMatch.Groups[1].Value}";
            }

            return value;
        }

        /// <summary>
        /// Determines whether the local version is up to date with the latest version.
        /// </summary>
        /// <param name="LocalVersion">The local version string to compare.</param>
        /// <param name="LatestVersion">The latest available version string.</param>
        /// <returns>true if the local version is up to date; otherwise, false.</returns>
        private static bool IsUpToDate(string LocalVersion, string LatestVersion)
        {
            if (string.Equals(LocalVersion, LatestVersion, StringComparison.OrdinalIgnoreCase))
                return true;

            string local = NormalizeVersion(LocalVersion);
            string latest = NormalizeVersion(LatestVersion);

            if (string.Equals(local, latest, StringComparison.OrdinalIgnoreCase))
                return true;

            if (Version.TryParse(local, out var localVer) && Version.TryParse(latest, out var latestVer))
                return localVer >= latestVer;

            return false;
        }

        /// <summary>
        /// Normalizes a version tag by removing a leading 'v' and extracting the version number.
        /// </summary>
        /// <param name="Tag">The version tag to normalize.</param>
        /// <returns>The normalized version string.</returns>
        private static string NormalizeVersion(string Tag)
        {
            if (string.IsNullOrWhiteSpace(Tag))
                return Tag;

            Tag = Tag.Trim();

            if (Tag.StartsWith('v') && Tag.Length > 1 && char.IsDigit(Tag[1]))
                Tag = Tag[1..];

            var match = VersionNumberRegex().Match(Tag);
            return match.Success ? match.Groups[1].Value : Tag;
        }

        [GeneratedRegex(@"#define\s+PCRE2_MINOR\s+(\d+)", RegexOptions.Multiline)]
        private static partial Regex PcreVersionRegex();
        [GeneratedRegex(@"#define\s+VERSION_MINOR\s+(\d+)", RegexOptions.Multiline)]
        private static partial Regex TlshVersionMinorRegex();
        [GeneratedRegex(@"#define\s+XXH_VERSION_RELEASE\s+(\d+)", RegexOptions.Multiline)]
        private static partial Regex XxHashVersionReleaseRegex();
        [GeneratedRegex(@"#define\s+XXH_VERSION_MINOR\s+(\d+)", RegexOptions.Multiline)]
        private static partial Regex XxHashVersionMinorRegex();
        [GeneratedRegex(@"#define\s+VERSION_PATCH\s+(\d+)", RegexOptions.Multiline)]
        private static partial Regex TlshVersionPatchRegex();
        [GeneratedRegex(@"(\d+(?:\.\d+)+)", RegexOptions.Multiline)]
        private static partial Regex VersionNumberRegex();
    }

    public sealed record ThirdPartyLibrary(
        string Name,
        string GithubOwner,
        string GithubRepo,
        string HeaderFile,
        string VersionPattern
        );
}
