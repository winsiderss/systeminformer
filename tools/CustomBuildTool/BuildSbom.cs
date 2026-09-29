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
    /// SBOM generation and explicit GitHub publishing commands used by Azure DevOps.
    /// </summary>
    public static partial class BuildSbom
    {
        public static Command CreateSbomCommand()
        {
            var command = new Command("-sbom", "Generates SPDX and CycloneDX SBOM files.");
            var path = new Argument<string>("path") { Description = "Release staging directory" };
            command.Add(path);
            command.SetAction(parse => Generate(parse.GetValue(path)) ? 0 : 1);
            return command;
        }

        public static Command CreateGithubSubmitSbomCommand()
        {
            var command = new Command("-github-submit-sbom", "Submits the SBOM dependency snapshot to GitHub.");
            var sbom = new Argument<string>("sbom") { Description = "SPDX SBOM path" };
            command.Add(sbom);
            command.SetAction(async parse => await Submit(parse.GetValue(sbom)) ? 0 : 1);
            return command;
        }

        public static Command CreateGithubCreateReleaseCommand()
        {
            var command = new Command("-github-create-release", "Creates or updates a GitHub release and uploads assets.");
            var tag = new Argument<string>("tag");
            var directory = new Argument<string>("directory") { Description = "Directory containing release assets" };
            command.Add(tag); command.Add(directory);
            command.SetAction(async parse => await CreateRelease(parse.GetValue(tag), parse.GetValue(directory)) ? 0 : 1);
            return command;
        }

        public static Command CreateGithubAttestSbomCommand()
        {
            var command = new Command("-github-attest-sbom", "Attests a release binary with an SPDX or CycloneDX SBOM.");
            var binary = new Argument<string>("binary");
            var sbom = new Argument<string>("sbom");
            var format = new Option<string>("--format") { DefaultValueFactory = _ => "spdx" };
            command.Add(binary); command.Add(sbom); command.Add(format);
            command.SetAction(parse => Attest(parse.GetValue(binary), parse.GetValue(sbom), parse.GetValue(format)) ? 0 : 1);
            return command;
        }

        private static List<(string Name, string Version, string Path)> Components(string root)
        {
            var result = new List<(string, string, string)>
            {
                ("System Informer", "source", root)
            };

            var thirdParty = Path.Combine(Directory.GetParent(root)?.Parent?.FullName ?? root, "tools", "thirdparty");

            if (Directory.Exists(thirdParty))
            {
                foreach (var dir in Directory.GetDirectories(thirdParty).OrderBy(x => x, StringComparer.OrdinalIgnoreCase))
                {
                    result.Add((Path.GetFileName(dir), "vendored", dir));
                }
            }

            return result;
        }

        private static bool Generate(string root)
        {
            if (!Directory.Exists(root))
            {
                Console.Error.WriteLine($"Missing release directory: {root}");
                return false;
            }

            var components = Components(Path.GetFullPath(root));

            var bom = new CycloneDxBom
            {
                BomFormat = "CycloneDX",
                SpecVersion = "1.6",
                SerialNumber = "urn:uuid:" + Guid.NewGuid(),
                Version = 1,
                Components = []
            };

            foreach (var item in components)
            {
                bom.Components.Add(new CycloneDxComponent
                {
                    Type = "library",
                    Name = item.Name,
                    Version = item.Version,
                    BomRef = "pkg:generic/" + Uri.EscapeDataString(item.Name) + "@" + item.Version
                });
            }

            var spdx = new SpdxDocument
            {
                SpdxVersion = "SPDX-2.3",
                SpdxId = "SPDXRef-DOCUMENT",
                Name = "System Informer",
                DocumentNamespace = "https://github.com/winsiderss/systeminformer/sbom/" + Guid.NewGuid(),
                CreationInfo = new SpdxCreationInfo
                {
                    Created = DateTime.UtcNow.ToString("O"),
                    Creators = ["Tool: CustomBuildTool"]
                },
                Packages = []
            };

            foreach (var item in components)
            {
                spdx.Packages.Add(new SpdxPackage
                {
                    SpdxId = "SPDXRef-" + item.Name.Replace(' ', '-'),
                    Name = item.Name,
                    VersionInfo = item.Version,
                    DownloadLocation = "NOASSERTION",
                    LicenseConcluded = "NOASSERTION",
                    LicenseDeclared = "NOASSERTION"
                });
            }

            using (var stream = File.Create(Path.Join(root, "SystemInformer.cdx.json")))
                JsonSerializer.Serialize(stream, bom, BuildSbomIndentedJsonContext.Default.CycloneDxBom);
            using (var stream = File.Create(Path.Join(root, "SystemInformer.spdx.json")))
                JsonSerializer.Serialize(stream, spdx, BuildSbomIndentedJsonContext.Default.SpdxDocument);
            return true;
        }

        private static async Task<bool> Submit(string sbomPath)
        {
            var token = Environment.GetEnvironmentVariable("GITHUB_TOKEN");
            var repo = Environment.GetEnvironmentVariable("GITHUB_REPOSITORY");
            var sha = Environment.GetEnvironmentVariable("BUILD_SOURCEVERSION") ?? Environment.GetEnvironmentVariable("GITHUB_SHA");

            if (string.IsNullOrWhiteSpace(token) || string.IsNullOrWhiteSpace(repo) || string.IsNullOrWhiteSpace(sha))
            {
                Console.Error.WriteLine("GITHUB_TOKEN, GITHUB_REPOSITORY and BUILD_SOURCEVERSION/GITHUB_SHA are required.");
                return false;
            }

            var snapshot = new DependencySnapshot
            {
                Version = 0,
                Sha = sha,
                Ref = "refs/heads/" + (Environment.GetEnvironmentVariable("BUILD_SOURCEBRANCHNAME") ?? "main"),
                Job = new DependencyJob
                {
                    Correlator = "custombuildtool-sbom",
                    Id = Environment.GetEnvironmentVariable("BUILD_BUILDID") ?? Guid.NewGuid().ToString()
                },
                Detector = new DependencyDetector
                {
                    Name = "CustomBuildTool",
                    Version = "1.0",
                    Url = "https://github.com/winsiderss/systeminformer"
                },
                Scanned = DateTime.UtcNow.ToString("O"),
                Manifests = new Dictionary<string, DependencyManifest>
                {
                    [Path.GetFileName(sbomPath)] = new DependencyManifest
                    {
                        Name = Path.GetFileName(sbomPath),
                        File = new DependencyManifestFile { SourceLocation = sbomPath },
                        Resolved = new Dictionary<string, DependencyResolved>()
                    }
                }
            };

            using var request = new HttpRequestMessage(HttpMethod.Post, $"https://api.github.com/repos/{repo}/dependency-graph/snapshots");

            GithubHeaders(request, token);

            request.Content = JsonContent.Create(snapshot, BuildSbomJsonContext.Default.DependencySnapshot);

            using var response = await BuildHttpClient.SendWithRetry(BuildGithub.GithubHttpClient, request);

            if (!response.IsSuccessStatusCode)
                Console.Error.WriteLine(await response.Content.ReadAsStringAsync());

            return response.IsSuccessStatusCode;
        }

        private static async Task<bool> CreateRelease(string tag, string directory)
        {
            var token = Environment.GetEnvironmentVariable("GITHUB_TOKEN");
            var repo = Environment.GetEnvironmentVariable("GITHUB_REPOSITORY");

            if (string.IsNullOrWhiteSpace(token) || string.IsNullOrWhiteSpace(repo))
                return false;

            var body = new GithubReleaseRequest
            {
                TagName = tag,
                Name = tag,
                GenerateReleaseNotes = true
            };

            using var request = new HttpRequestMessage(HttpMethod.Post, $"https://api.github.com/repos/{repo}/releases");

            GithubHeaders(request, token);

            request.Content = JsonContent.Create(body, BuildSbomJsonContext.Default.GithubReleaseRequest);

            using var response = await BuildHttpClient.SendWithRetry(BuildGithub.GithubHttpClient, request);

            if (!response.IsSuccessStatusCode)
            {
                Console.Error.WriteLine(await response.Content.ReadAsStringAsync());
                return false;
            }

            await using var stream = await response.Content.ReadAsStreamAsync();
            var release = await JsonSerializer.DeserializeAsync(stream, BuildSbomJsonContext.Default.GithubReleaseResponse);
            var uploadUrl = release?.UploadUrl?.Split('{')[0];

            if (string.IsNullOrWhiteSpace(uploadUrl))
                return false;

            foreach (var file in Directory.EnumerateFiles(directory))
            {
                using var uploadRequest = new HttpRequestMessage(HttpMethod.Post, uploadUrl + "?name=" + Uri.EscapeDataString(Path.GetFileName(file)));

                GithubHeaders(uploadRequest, token);

                uploadRequest.Content = new StreamContent(File.OpenRead(file));
                uploadRequest.Content.Headers.ContentType = new MediaTypeHeaderValue("application/octet-stream");

                using var upload = await BuildHttpClient.SendWithRetry(BuildGithub.GithubHttpClient, uploadRequest);

                if (!upload.IsSuccessStatusCode)
                {
                    Console.Error.WriteLine(await upload.Content.ReadAsStringAsync());
                    return false;
                }
            }

            return true;
        }

        private static bool Attest(string binary, string sbom, string format)
        {
            if (!File.Exists(binary) || !File.Exists(sbom) || (format != "spdx" && format != "cyclonedx"))
                return false;

            Console.WriteLine($"SBOM ready for GitHub attestation: {Path.GetFileName(binary)} ({format}), SHA256={BuildVerify.HashFile(binary)}");
            Console.WriteLine("Use the Azure DevOps OIDC-capable GitHub attestation task to sign this predicate."); return true;
        }

        /// <summary>
        /// Applies the GitHub API authentication and versioning headers to a request.
        /// </summary>
        /// <remarks>The shared <see cref="BuildGithub.GithubHttpClient"/> only supplies the user agent, so
        /// per-request headers are set here rather than on the client defaults.</remarks>
        private static void GithubHeaders(HttpRequestMessage request, string token)
        {
            request.Headers.Authorization = new AuthenticationHeaderValue("Bearer", token);
            request.Headers.Accept.Add(new MediaTypeWithQualityHeaderValue("application/vnd.github+json"));
            request.Headers.TryAddWithoutValidation("X-GitHub-Api-Version", "2022-11-28");
        }

        /// <summary>
        /// Source generation context for the GitHub API payloads.
        /// </summary>
        [JsonSourceGenerationOptions(DefaultIgnoreCondition = JsonIgnoreCondition.WhenWritingNull, GenerationMode = JsonSourceGenerationMode.Default)]
        [JsonSerializable(typeof(DependencySnapshot))]
        [JsonSerializable(typeof(GithubReleaseRequest))]
        [JsonSerializable(typeof(GithubReleaseResponse))]
        internal partial class BuildSbomJsonContext : JsonSerializerContext;

        /// <summary>
        /// Source generation context for the indented SBOM documents written to disk.
        /// </summary>
        [JsonSourceGenerationOptions(DefaultIgnoreCondition = JsonIgnoreCondition.WhenWritingNull, GenerationMode = JsonSourceGenerationMode.Default, WriteIndented = true)]
        [JsonSerializable(typeof(CycloneDxBom))]
        [JsonSerializable(typeof(SpdxDocument))]
        internal partial class BuildSbomIndentedJsonContext : JsonSerializerContext;

        public class CycloneDxBom
        {
            [JsonPropertyName("bomFormat")]
            public string BomFormat { get; set; }

            [JsonPropertyName("specVersion")]
            public string SpecVersion { get; set; }

            [JsonPropertyName("serialNumber")]
            public string SerialNumber { get; set; }

            [JsonPropertyName("version")]
            public int Version { get; set; }

            [JsonPropertyName("components")]
            public List<CycloneDxComponent> Components { get; set; }
        }

        public class CycloneDxComponent
        {
            [JsonPropertyName("type")]
            public string Type { get; set; }

            [JsonPropertyName("name")]
            public string Name { get; set; }

            [JsonPropertyName("version")]
            public string Version { get; set; }

            [JsonPropertyName("bom-ref")]
            public string BomRef { get; set; }
        }

        public class SpdxDocument
        {
            [JsonPropertyName("spdxVersion")]
            public string SpdxVersion { get; set; }

            [JsonPropertyName("SPDXID")]
            public string SpdxId { get; set; }

            [JsonPropertyName("name")]
            public string Name { get; set; }

            [JsonPropertyName("documentNamespace")]
            public string DocumentNamespace { get; set; }

            [JsonPropertyName("creationInfo")]
            public SpdxCreationInfo CreationInfo { get; set; }

            [JsonPropertyName("packages")]
            public List<SpdxPackage> Packages { get; set; }
        }

        public class SpdxCreationInfo
        {
            [JsonPropertyName("created")]
            public string Created { get; set; }

            [JsonPropertyName("creators")]
            public List<string> Creators { get; set; }
        }

        public class SpdxPackage
        {
            [JsonPropertyName("SPDXID")]
            public string SpdxId { get; set; }

            [JsonPropertyName("name")]
            public string Name { get; set; }

            [JsonPropertyName("versionInfo")]
            public string VersionInfo { get; set; }

            [JsonPropertyName("downloadLocation")]
            public string DownloadLocation { get; set; }

            [JsonPropertyName("licenseConcluded")]
            public string LicenseConcluded { get; set; }

            [JsonPropertyName("licenseDeclared")]
            public string LicenseDeclared { get; set; }
        }

        public class DependencySnapshot
        {
            [JsonPropertyName("version")]
            public int Version { get; set; }

            [JsonPropertyName("sha")]
            public string Sha { get; set; }

            [JsonPropertyName("ref")]
            public string Ref { get; set; }

            [JsonPropertyName("job")]
            public DependencyJob Job { get; set; }

            [JsonPropertyName("detector")]
            public DependencyDetector Detector { get; set; }

            [JsonPropertyName("scanned")]
            public string Scanned { get; set; }

            [JsonPropertyName("manifests")]
            public Dictionary<string, DependencyManifest> Manifests { get; set; }
        }

        public class DependencyJob
        {
            [JsonPropertyName("correlator")]
            public string Correlator { get; set; }

            [JsonPropertyName("id")]
            public string Id { get; set; }
        }

        public class DependencyDetector
        {
            [JsonPropertyName("name")]
            public string Name { get; set; }

            [JsonPropertyName("version")]
            public string Version { get; set; }

            [JsonPropertyName("url")]
            public string Url { get; set; }
        }

        public class DependencyManifest
        {
            [JsonPropertyName("name")]
            public string Name { get; set; }

            [JsonPropertyName("file")]
            public DependencyManifestFile File { get; set; }

            [JsonPropertyName("resolved")]
            public Dictionary<string, DependencyResolved> Resolved { get; set; }
        }

        public class DependencyManifestFile
        {
            [JsonPropertyName("source_location")]
            public string SourceLocation { get; set; }
        }

        public class DependencyResolved
        {
            [JsonPropertyName("package_url")]
            public string PackageUrl { get; set; }
        }

        public class GithubReleaseRequest
        {
            [JsonPropertyName("tag_name")]
            public string TagName { get; set; }

            [JsonPropertyName("name")]
            public string Name { get; set; }

            [JsonPropertyName("generate_release_notes")]
            public bool GenerateReleaseNotes { get; set; }
        }

        public class GithubReleaseResponse
        {
            [JsonPropertyName("upload_url")]
            public string UploadUrl { get; set; }
        }
    }
}