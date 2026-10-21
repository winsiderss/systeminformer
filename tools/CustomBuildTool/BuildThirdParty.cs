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
        // Libraries with a null GithubOwner/GithubRepo have no upstream GitHub releases or tags to compare against.
        private static readonly ThirdPartyLibrary[] Libraries =
        [
            new("7zip",       "ip7z",            "7zip",          "7zip\\C\\7zVersion.h",                          @"#define\s+MY_VERSION_NUMBERS\s+""([^""]+)"""),
            new("ahash",      null,              null,            "ahash\\ahash.c",                                null),
            new("detours",    "microsoft",       "Detours",       "detours\\detours.h",                            @"#define\s+DETOURS_VERSION\s+(0x[0-9a-fA-F]+)"),
            new("fuzzy",      null,              null,            "fuzzy\\fts_fuzzy_match.h",                      @"^//\s+(\d+(?:\.\d+)+)\s+\("),
            new("jsonc",      "json-c",          "json-c",        "jsonc\\json_c_version.h",                       @"#define\s+JSON_C_VERSION\s+""([^""]+)"""),
            new("maxminddb",  "maxmind",         "libmaxminddb",  "maxminddb\\maxminddb.h",                        @"#define\s+PACKAGE_VERSION\s+""([^""]+)"""),
            new("md5",        null,              null,            "md5\\md5.h",                                    null),
            new("miniz",      "richgel999",      "miniz",         "miniz\\miniz.h",                                @"miniz\.c\s+(\d+(?:\.\d+)+)"),
            new("mxml",       "michaelrsweet",   "mxml",          "mxml\\config.h",                                @"#\s*define\s+MXML_VERSION\s+""Mini-XML\s+v([^""]+)"""),
            new("pcre2",      "PCRE2Project",    "pcre2",         "pcre\\pcre2.h",                                 @"#define\s+PCRE2_MAJOR\s+(\d+)"),
            new("sha",        null,              null,            "sha\\sha.h",                                    null),
            new("sha256",     null,              null,            "sha256\\sha256.h",                              null),
            new("ssdeep",     "ssdeep-project",  "ssdeep",        "ssdeep\\fuzzy.h",                               null),
            new("symcrypt",   "microsoft",       "SymCrypt",      "SymCrypt\\inc\\symcrypt_internal_shared.inc",   @"#define\s+SYMCRYPT_CODE_VERSION_API\s+(\d+)"),
            new("tlsh",       "trendmicro",      "tlsh",          "tlsh\\tlsh_win_version.h",                      @"#define\s+VERSION_MAJOR\s+(\d+)"),
            new("winsdk",     null,              null,            "winsdk\\dia2.h",                                null),
            new("xxhash",     "Cyan4973",        "xxHash",        "xxhash\\xxhash.h",                              @"#define\s+XXH_VERSION_MAJOR\s+(\d+)"),
            new("zydis",      "zyantific",       "zydis",         "zydis\\Zydis.h",                                @"#define\s+ZYDIS_VERSION\s+\(ZyanU64\)(0x[0-9a-fA-F]+)"),
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
            var outdatedLibraries = new List<(ThirdPartyLibrary Library, string LocalVersion, string LatestTag)>();

            foreach (var lib in Libraries)
            {
                string localVersion = GetLocalVersion(lib);

                if (string.IsNullOrWhiteSpace(lib.GithubOwner) || string.IsNullOrWhiteSpace(lib.GithubRepo))
                {
                    if (!File.Exists(Path.Join([ThirdPartyDirectory, lib.HeaderFile])))
                    {
                        anyUnknown = true;
                        Console.WriteLine($"{VT.YELLOW}{lib.Name,-14} missing ({lib.HeaderFile}){VT.RESET}");
                        continue;
                    }

                    Console.WriteLine($"{VT.GRAY}{lib.Name,-14} {localVersion ?? "vendored",-20} no upstream release tracking{VT.RESET}");
                    continue;
                }

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
                    outdatedLibraries.Add((lib, localVersion, latestTag));
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

            foreach (var (lib, localVersion, latestTag) in outdatedLibraries)
            {
                await PrintThirdPartyChangelog(httpClient, lib, localVersion, latestTag);
            }
        }

        /// <summary>
        /// Prints the GitHub release notes of a library for every release newer than the local version, up to and including the latest release.
        /// </summary>
        /// <param name="HttpClient">The HTTP client used to send requests.</param>
        /// <param name="Lib">The outdated third-party library.</param>
        /// <param name="LocalVersion">The vendored version of the library.</param>
        /// <param name="LatestTag">The latest release tag of the library.</param>
        private static async Task PrintThirdPartyChangelog(HttpClient HttpClient, ThirdPartyLibrary Lib, string LocalVersion, string LatestTag)
        {
            Console.WriteLine();
            Console.WriteLine($"{VT.CYAN}==== {Lib.Name} changelog ({LocalVersion} -> {LatestTag}) ===={VT.RESET}");

            if (!TryParseVersion(LocalVersion, out var localVer) || !TryParseVersion(LatestTag, out var latestVer))
            {
                Console.WriteLine($"{VT.YELLOW}Unable to compare versions, see https://github.com/{Lib.GithubOwner}/{Lib.GithubRepo}/releases{VT.RESET}");
                return;
            }

            var releases = await GetGithubReleasesBetween(HttpClient, Lib.GithubOwner, Lib.GithubRepo, localVer, latestVer);

            if (releases.Count == 0)
            {
                Console.WriteLine($"{VT.YELLOW}No release notes found, see https://github.com/{Lib.GithubOwner}/{Lib.GithubRepo}/tags{VT.RESET}");
                return;
            }

            // Oldest first so the notes read forward from the vendored version.
            releases.Sort((a, b) => a.Version.CompareTo(b.Version));

            foreach (var release in releases)
            {
                Console.WriteLine();

                if (string.IsNullOrWhiteSpace(release.Name) || string.Equals(release.Name, release.TagName, StringComparison.OrdinalIgnoreCase))
                    Console.WriteLine($"{VT.GREEN}{release.TagName}{VT.RESET} {VT.GRAY}({release.Published}) {release.HtmlUrl}{VT.RESET}");
                else
                    Console.WriteLine($"{VT.GREEN}{release.TagName} - {release.Name}{VT.RESET} {VT.GRAY}({release.Published}) {release.HtmlUrl}{VT.RESET}");

                if (string.IsNullOrWhiteSpace(release.Body))
                    Console.WriteLine($"{VT.GRAY}(no release notes){VT.RESET}");
                else
                    Console.WriteLine(release.Body.ReplaceLineEndings().Trim());
            }
        }

        /// <summary>
        /// Retrieves the published (non-draft, non-prerelease) GitHub releases with a version greater than the local version and less than or equal to the latest version.
        /// </summary>
        /// <param name="HttpClient">The HTTP client used to send requests.</param>
        /// <param name="Owner">The GitHub repository owner.</param>
        /// <param name="Repo">The GitHub repository name.</param>
        /// <param name="LocalVersion">The exclusive lower version bound.</param>
        /// <param name="LatestVersion">The inclusive upper version bound.</param>
        /// <returns>The matching releases, or an empty list if none were found.</returns>
        private static async Task<List<ThirdPartyRelease>> GetGithubReleasesBetween(HttpClient HttpClient, string Owner, string Repo, Version LocalVersion, Version LatestVersion)
        {
            var releases = new List<ThirdPartyRelease>();

            try
            {
                // Releases are returned newest first; stop paging once we reach the local version.
                for (int page = 1; page <= 5; page++)
                {
                    using var request = new HttpRequestMessage(HttpMethod.Get, $"https://api.github.com/repos/{Owner}/{Repo}/releases?per_page=100&page={page}");
                    using var response = await BuildHttpClient.SendRequestMessage(HttpClient, request);

                    if (response?.IsSuccessStatusCode != true)
                        break;

                    await using var stream = await response.Content.ReadAsStreamAsync();
                    using var document = await JsonDocument.ParseAsync(stream);

                    if (document.RootElement.ValueKind != JsonValueKind.Array || document.RootElement.GetArrayLength() == 0)
                        break;

                    bool reachedLocal = false;

                    foreach (var element in document.RootElement.EnumerateArray())
                    {
                        if (GetJsonBoolean(element, "draft") || GetJsonBoolean(element, "prerelease"))
                            continue;

                        string tagName = GetJsonString(element, "tag_name");

                        if (!TryParseVersion(tagName, out var version))
                            continue;

                        if (version <= LocalVersion)
                        {
                            reachedLocal = true;
                            continue;
                        }

                        if (version > LatestVersion)
                            continue;

                        string published = GetJsonString(element, "published_at");

                        if (DateTimeOffset.TryParse(published, CultureInfo.InvariantCulture, DateTimeStyles.AssumeUniversal, out var publishedDate))
                            published = publishedDate.ToString("yyyy-MM-dd", CultureInfo.InvariantCulture);

                        releases.Add(new ThirdPartyRelease(
                            tagName,
                            GetJsonString(element, "name"),
                            published,
                            GetJsonString(element, "html_url"),
                            GetJsonString(element, "body"),
                            version
                            ));
                    }

                    if (reachedLocal || document.RootElement.GetArrayLength() < 100)
                        break;
                }
            }
            catch (Exception ex)
            {
                Console.WriteLine($"{VT.RED}GetGithubReleasesBetween: {ex}{VT.RESET}");
            }

            return releases;
        }

        private static string GetJsonString(JsonElement Element, string Name)
        {
            return Element.TryGetProperty(Name, out var value) && value.ValueKind == JsonValueKind.String ? value.GetString() : null;
        }

        private static bool GetJsonBoolean(JsonElement Element, string Name)
        {
            return Element.TryGetProperty(Name, out var value) && value.ValueKind == JsonValueKind.True;
        }

        /// <summary>
        /// Parses a version string or release tag into a <see cref="Version"/>, accepting single-component versions.
        /// </summary>
        /// <param name="Value">The version string or release tag.</param>
        /// <param name="Result">The parsed version.</param>
        /// <returns>true if the value was parsed; otherwise, false.</returns>
        private static bool TryParseVersion(string Value, out Version Result)
        {
            string normalized = NormalizeVersion(Value);

            if (!string.IsNullOrWhiteSpace(normalized) && !normalized.Contains('.'))
                normalized += ".0";

            return Version.TryParse(normalized, out Result);
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
                using (var response = await BuildHttpClient.SendRequestMessage(HttpClient, request))
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
                using (var response = await BuildHttpClient.SendRequestMessage(HttpClient, request))
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

            string headerPath = Path.Join([ThirdPartyDirectory, Lib.HeaderFile]);
            if (!File.Exists(headerPath))
                return null;

            Match match = Match.Empty;
            Match minorMatch = Match.Empty;
            Match patchMatch = Match.Empty;
            foreach (string line in File.ReadLines(headerPath))
            {
                if (!match.Success)
                    match = Regex.Match(line, Lib.VersionPattern);
                if (Lib.Name is "pcre2" or "symcrypt" or "tlsh" or "xxhash")
                {
                    if (!minorMatch.Success)
                        minorMatch = Lib.Name switch
                        {
                            "pcre2" => PcreVersionRegex().Match(line),
                            "symcrypt" => SymCryptVersionMinorRegex().Match(line),
                            "tlsh" => TlshVersionMinorRegex().Match(line),
                            _ => XxHashVersionMinorRegex().Match(line)
                        };
                    if (!patchMatch.Success)
                        patchMatch = Lib.Name switch
                        {
                            "symcrypt" => SymCryptVersionPatchRegex().Match(line),
                            "tlsh" => TlshVersionPatchRegex().Match(line),
                            "xxhash" => XxHashVersionReleaseRegex().Match(line),
                            _ => Match.Empty
                        };
                }
            }
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
                if (minorMatch.Success)
                    return $"{value}.{minorMatch.Groups[1].Value}";
            }

            // symcrypt encodes as separate API/MINOR/PATCH defines
            if (Lib.Name.Equals("symcrypt", StringComparison.OrdinalIgnoreCase))
            {
                if (minorMatch.Success && patchMatch.Success)
                    return $"{value}.{minorMatch.Groups[1].Value}.{patchMatch.Groups[1].Value}";
            }

            // tlsh encodes as separate MAJOR/MINOR/PATCH defines
            if (Lib.Name.Equals("tlsh", StringComparison.OrdinalIgnoreCase))
            {
                if (minorMatch.Success && patchMatch.Success)
                    return $"{value}.{minorMatch.Groups[1].Value}.{patchMatch.Groups[1].Value}";
            }

            // xxhash encodes as separate MAJOR/MINOR/RELEASE defines
            if (Lib.Name.Equals("xxhash", StringComparison.OrdinalIgnoreCase))
            {
                if (minorMatch.Success && patchMatch.Success)
                    return $"{value}.{minorMatch.Groups[1].Value}.{patchMatch.Groups[1].Value}";
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
        [GeneratedRegex(@"#define\s+SYMCRYPT_CODE_VERSION_MINOR\s+(\d+)", RegexOptions.Multiline)]
        private static partial Regex SymCryptVersionMinorRegex();
        [GeneratedRegex(@"#define\s+SYMCRYPT_CODE_VERSION_PATCH\s+(\d+)", RegexOptions.Multiline)]
        private static partial Regex SymCryptVersionPatchRegex();
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

    public sealed record ThirdPartyRelease(
        string TagName,
        string Name,
        string Published,
        string HtmlUrl,
        string Body,
        Version Version
        );
}
