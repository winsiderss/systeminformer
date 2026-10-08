namespace CustomBuildTool
{
    /// <summary>
    /// Generates the per-driver SPDX 3.0 SBOM and VEX statement required for WHCP driver submissions
    /// (Windows 25H2/26H1 and Windows Server 2025, starting 03/2027).
    /// </summary>
    /// <remarks>
    /// WHCP requires one SBOM and one VEX per INF per driver folder, named <c>{inf_name}.spdx.json</c> and
    /// <c>{inf_name}.vex.json</c> and placed in a subfolder of the driver package folder. Both files must
    /// be COSE signed afterwards (WDK sbom-tool); this command only produces the unsigned documents.
    /// </remarks>
    public static partial class BuildDriverSbom
    {
        private const string SpdxContext = "https://spdx.org/rdf/3.0.1/spdx-context.jsonld";
        private const string SpdxSpecVersion = "3.0.1";
        private const string CreationInfoId = "_:creationinfo";
        private const string InfName = "KSystemInformer";
        private const string Supplier = "Winsider Seminars & Solutions, Inc.";
        private const string RepositoryUrl = "https://github.com/winsiderss/systeminformer";

        public static Command CreateDriverSbomCommand()
        {
            var command = new Command("-sbom-driver", "Generates the WHCP SPDX 3.0 SBOM and VEX for the driver package.");
            var repo = new Argument<string>("repo") { Description = "Repository root directory" };
            var arch = new Argument<string>("arch") { Description = "Driver package architecture (x64, arm64)" };
            var binaries = new Argument<string>("binaries") { Description = "Directory containing systeminformer.sys and ksi.dll" };
            var output = new Argument<string>("output") { Description = "Driver package folder (files are written to <output>\\sbom)" };
            var pqc = new Option<bool>("--pqc") { Description = "Driver was built with KphEnablePqc (links SymCrypt)" };
            command.Add(repo); command.Add(arch); command.Add(binaries); command.Add(output); command.Add(pqc);
            command.SetAction(parse => Generate(
                parse.GetValue(repo),
                parse.GetValue(arch),
                parse.GetValue(binaries),
                parse.GetValue(output),
                parse.GetValue(pqc)
                ) ? 0 : 1);
            return command;
        }

        public static Command CreateDriverSbomSignCommand()
        {
            var command = new Command("-sbom-driver-sign", "COSE-signs the driver SBOM and VEX documents (detached) using the WDK CoseSignTool.");
            var sbomdir = new Argument<string>("sbomdir") { Description = "Directory containing the SBOM/VEX documents (the <output>\\sbom folder written by -sbom-driver)" };
            var pfx = new Option<string>("--pfx") { Description = "Path to a signing certificate (.pfx)." };
            var password = new Option<string>("--pw") { Description = "Password for the .pfx file, if any." };
            var thumbprint = new Option<string>("--thumbprint") { Description = "SHA-1 thumbprint of a signing certificate in a local certificate store." };
            var storeName = new Option<string>("--store-name") { DefaultValueFactory = _ => "My", Description = "Certificate store name for --thumbprint (default: My)." };
            var storeLocation = new Option<string>("--store-location") { DefaultValueFactory = _ => "CurrentUser", Description = "Certificate store location for --thumbprint (default: CurrentUser)." };
            var validate = new Option<bool>("--validate") { Description = "Validate each detached signature after signing." };
            command.Add(sbomdir); command.Add(pfx); command.Add(password); command.Add(thumbprint); command.Add(storeName); command.Add(storeLocation); command.Add(validate);
            command.SetAction(parse => Sign(
                parse.GetValue(sbomdir),
                parse.GetValue(pfx),
                parse.GetValue(password),
                parse.GetValue(thumbprint),
                parse.GetValue(storeName),
                parse.GetValue(storeLocation),
                parse.GetValue(validate)
                ) ? 0 : 1);
            return command;
        }

        /// <summary>
        /// COSE-signs (detached) the driver SBOM and VEX documents produced by <see cref="Generate"/>.
        /// </summary>
        /// <remarks>
        /// WHCP requires each <c>{inf}.spdx.json</c> / <c>{inf}.vex.json</c> to carry a detached COSE signature. A
        /// detached signature is written next to the payload as <c>{payload}.cose</c> and validates by hash match, so
        /// the unsigned documents remain human-readable. Exactly one certificate source must be supplied: a <c>.pfx</c>
        /// file (with optional password) or a certificate-store thumbprint (with optional store name/location).
        /// </remarks>
        private static bool Sign(string sbomdir, string pfx, string password, string thumbprint, string storeName, string storeLocation, bool validate)
        {
            if (!Directory.Exists(sbomdir))
            {
                Console.Error.WriteLine($"Missing SBOM directory: {sbomdir}");
                return false;
            }

            bool hasPfx = !string.IsNullOrWhiteSpace(pfx);
            bool hasThumbprint = !string.IsNullOrWhiteSpace(thumbprint);

            if (hasPfx == hasThumbprint)
            {
                Console.Error.WriteLine("Specify exactly one certificate source: --pfx <file> [--pw <password>] or --thumbprint <sha1> [--store-name <name>] [--store-location <location>].");
                return false;
            }

            if (hasPfx && !File.Exists(pfx))
            {
                Console.Error.WriteLine($"Missing signing certificate: {pfx}");
                return false;
            }

            string coseSignTool = Utils.GetCoseSignToolPath();

            if (string.IsNullOrWhiteSpace(coseSignTool) || !File.Exists(coseSignTool))
            {
                Console.Error.WriteLine("CoseSignTool.exe was not found in the Windows SDK/WDK Tools directory.");
                return false;
            }

            foreach (string name in new[] { InfName + ".spdx.json", InfName + ".vex.json" })
            {
                string payload = Path.Join([sbomdir, name]);

                if (!File.Exists(payload))
                {
                    Console.Error.WriteLine($"Missing SBOM document: {payload}");
                    return false;
                }

                string signature = payload + ".cose";

                var signArgs = new List<string> { "sign", "-payload", payload, "-sf", signature };

                if (hasPfx)
                {
                    signArgs.Add("-pfx"); signArgs.Add(pfx);

                    if (!string.IsNullOrWhiteSpace(password))
                    {
                        signArgs.Add("-pw"); signArgs.Add(password);
                    }
                }
                else
                {
                    signArgs.Add("-th"); signArgs.Add(thumbprint);

                    if (!string.IsNullOrWhiteSpace(storeName))
                    {
                        signArgs.Add("-sn"); signArgs.Add(storeName);
                    }

                    if (!string.IsNullOrWhiteSpace(storeLocation))
                    {
                        signArgs.Add("-sl"); signArgs.Add(storeLocation);
                    }
                }

                if (Win32.CreateProcess(coseSignTool, signArgs, out _, false, false) != 0)
                {
                    Console.Error.WriteLine($"CoseSignTool sign failed for {name}.");
                    return false;
                }

                Console.WriteLine($"Signed: {signature}");

                if (validate)
                {
                    var validateArgs = new List<string> { "validate", "-sf", signature, "-payload", payload };

                    if (Win32.CreateProcess(coseSignTool, validateArgs, out _, false, false) != 0)
                    {
                        Console.Error.WriteLine($"CoseSignTool validate failed for {name}.");
                        return false;
                    }

                    Console.WriteLine($"Validated: {signature}");
                }
            }

            return true;
        }

        private static bool Generate(string repo, string arch, string binaries, string output, bool pqc)
        {
            repo = Path.GetFullPath(repo);
            binaries = Path.GetFullPath(binaries);

            string driverPath = Path.Join([binaries, "systeminformer.sys"]);
            string ksiPath = Path.Join([binaries, "ksi.dll"]);

            foreach (var file in new[] { driverPath, ksiPath })
            {
                if (!File.Exists(file))
                {
                    Console.Error.WriteLine($"Missing driver binary: {file}");
                    return false;
                }
            }

            string version = FileVersion(driverPath);
            string created = DateTime.UtcNow.ToString("yyyy-MM-ddTHH:mm:ssZ");
            string baseId = $"{RepositoryUrl}/spdx/{InfName}/{arch}/{version}/{Guid.NewGuid()}";
            string packagePurl = $"pkg:github/winsiderss/systeminformer@{version}#KSystemInformer?arch={arch}";
            string sbomDir = Path.Join([Path.GetFullPath(output), "sbom"]);

            Directory.CreateDirectory(sbomDir);

            //
            // SBOM
            //

            var graph = new List<SpdxElement>();
            var ids = new SpdxIds(baseId);

            AddCreationInfo(graph, ids, created);

            string packageId = ids.Next("Package-" + InfName);
            graph.Add(Package(packageId, InfName, version, packagePurl, RepositoryUrl, "deviceDriver"));

            string driverId = ids.Next("File-systeminformer.sys");
            graph.Add(FileElement(driverId, driverPath, "deviceDriver"));
            string ksiId = ids.Next("File-ksi.dll");
            graph.Add(FileElement(ksiId, ksiPath, "library"));

            var dependencies = new List<string>();

            //
            // First-party static inputs linked into systeminformer.sys.
            //

            string kphlibId = ids.Next("Package-kphlib");
            graph.Add(Package(kphlibId, "kphlib", version, $"pkg:github/winsiderss/systeminformer@{version}#kphlib", RepositoryUrl, "library"));
            dependencies.Add(kphlibId);

            string phntId = ids.Next("Package-phnt");
            graph.Add(Package(phntId, "phnt", version, $"pkg:github/winsiderss/systeminformer@{version}#phnt", RepositoryUrl, "source"));
            dependencies.Add(phntId);

            if (pqc)
            {
                string symcryptVersion = SymCryptVersion(repo);

                if (string.IsNullOrWhiteSpace(symcryptVersion))
                {
                    Console.Error.WriteLine("Unable to determine the SymCrypt version.");
                    return false;
                }

                string symcryptId = ids.Next("Package-SymCrypt");
                graph.Add(Package(symcryptId, "SymCrypt", symcryptVersion, $"pkg:github/microsoft/SymCrypt@v{symcryptVersion}", "https://github.com/microsoft/SymCrypt", "library"));
                dependencies.Add(symcryptId);
            }

            //
            // Build tools.
            //

            var tools = new List<string>();
            AddTool(graph, ids, tools, "MSVC", Environment.GetEnvironmentVariable("VCToolsVersion"), "pkg:generic/microsoft/msvc@");
            AddTool(graph, ids, tools, "Windows Driver Kit", Environment.GetEnvironmentVariable("WindowsSDKVersion")?.TrimEnd('\\'), "pkg:generic/microsoft/wdk@");

            graph.Add(Relationship(ids.Next("Relationship-contains"), packageId, "contains", [driverId, ksiId]));
            graph.Add(Relationship(ids.Next("Relationship-dependsOn"), driverId, "dependsOn", dependencies));
            if (tools.Count > 0)
                graph.Add(Relationship(ids.Next("Relationship-usesTool"), packageId, "usesTool", tools));

            string licenseId = ids.Next("License-MIT");
            graph.Add(new SpdxElement
            {
                Type = "simplelicensing_LicenseExpression",
                SpdxId = licenseId,
                CreationInfo = CreationInfoId,
                LicenseExpression = "MIT"
            });
            graph.Add(Relationship(ids.Next("Relationship-hasDeclaredLicense"), packageId, "hasDeclaredLicense", [licenseId]));

            graph.Add(Document(ids.Next("Document"), $"{InfName} ({arch}) SBOM", packageId, ["core", "software", "simpleLicensing"]));

            //
            // VEX
            //

            var vexGraph = new List<SpdxElement>();
            var vexIds = new SpdxIds(baseId + "/vex");

            AddCreationInfo(vexGraph, vexIds, created);

            // Reference the assessed package by its SBOM spdxId.
            vexGraph.Add(Package(packageId, InfName, version, packagePurl, RepositoryUrl, "deviceDriver"));

            if (!AddVexStatements(vexGraph, vexIds, Path.Join([repo, "KSystemInformer", "vex.json"]), packageId, created))
                return false;

            vexGraph.Add(Document(vexIds.Next("Document"), $"{InfName} ({arch}) VEX", packageId, ["core", "software", "security"]));

            string sbomPath = Path.Join([sbomDir, InfName + ".spdx.json"]);
            string vexPath = Path.Join([sbomDir, InfName + ".vex.json"]);

            Write(sbomPath, graph);
            Write(vexPath, vexGraph);

            Console.WriteLine($"SBOM: {sbomPath}");
            Console.WriteLine($"VEX:  {vexPath}");
            return true;
        }

        /// <summary>
        /// Adds VEX assessments from the optional vex.json input.
        /// </summary>
        /// <remarks>
        /// Input format: [ { "cve": "CVE-YYYY-NNNN", "status": "not_affected|affected|fixed|under_investigation",
        /// "justification": "vulnerableCodeNotPresent|...", "statement": "..." } ]. A missing file means no
        /// known open CVEs apply, which yields a VEX document with no assessments.
        /// </remarks>
        private static bool AddVexStatements(List<SpdxElement> graph, SpdxIds ids, string inputPath, string packageId, string created)
        {
            if (!File.Exists(inputPath))
                return true;

            List<VexInputEntry> entries;

            try
            {
                using var stream = File.OpenRead(inputPath);
                entries = JsonSerializer.Deserialize(stream, BuildDriverSbomJsonContext.Default.ListVexInputEntry);
            }
            catch (JsonException ex)
            {
                Console.Error.WriteLine($"Invalid VEX input {inputPath}: {ex.Message}");
                return false;
            }

            if (entries == null)
            {
                Console.Error.WriteLine($"Invalid VEX input {inputPath}: expected a JSON array.");
                return false;
            }

            foreach (var entry in entries)
            {
                if (entry == null || string.IsNullOrWhiteSpace(entry.Cve) || string.IsNullOrWhiteSpace(entry.Status))
                {
                    Console.Error.WriteLine($"Invalid VEX entry in {inputPath}: 'cve' and 'status' are required.");
                    return false;
                }

                string cve = entry.Cve;
                string statement = string.IsNullOrWhiteSpace(entry.Statement) ? null : entry.Statement;
                string justification = string.IsNullOrWhiteSpace(entry.Justification) ? null : entry.Justification;

                string vulnId = ids.Next("Vulnerability-" + cve);
                graph.Add(new SpdxElement
                {
                    Type = "security_Vulnerability",
                    SpdxId = vulnId,
                    CreationInfo = CreationInfoId,
                    Name = cve,
                    ExternalIdentifier =
                    [
                        new SpdxExternalIdentifier
                        {
                            Type = "ExternalIdentifier",
                            ExternalIdentifierType = "cve",
                            Identifier = cve,
                            IdentifierLocator = [$"https://www.cve.org/CVERecord?id={cve}"]
                        }
                    ]
                });

                (string type, string relationshipType) = entry.Status switch
                {
                    "not_affected" => ("security_VexNotAffectedVulnAssessmentRelationship", "doesNotAffect"),
                    "affected" => ("security_VexAffectedVulnAssessmentRelationship", "affects"),
                    "fixed" => ("security_VexFixedVulnAssessmentRelationship", "fixedIn"),
                    "under_investigation" => ("security_VexUnderInvestigationVulnAssessmentRelationship", "underInvestigationFor"),
                    _ => (null, null)
                };

                if (string.IsNullOrWhiteSpace(type))
                {
                    Console.Error.WriteLine($"Invalid VEX status '{entry.Status}' for {cve}.");
                    return false;
                }

                var assessment = Relationship(ids.Next("VexAssessment-" + cve), vulnId, relationshipType, [packageId]);
                assessment.Type = type;
                assessment.AssessedElement = packageId;
                assessment.PublishedTime = created;

                switch (entry.Status)
                {
                    case "not_affected":
                        if (string.IsNullOrWhiteSpace(justification) && string.IsNullOrWhiteSpace(statement))
                        {
                            Console.Error.WriteLine($"VEX not_affected for {cve} requires a 'justification' or 'statement'.");
                            return false;
                        }
                        assessment.JustificationType = justification;
                        assessment.ImpactStatement = statement;
                        break;
                    case "affected":
                        assessment.ActionStatement = statement ?? "No action statement provided.";
                        break;
                    default:
                        assessment.Comment = statement;
                        break;
                }

                graph.Add(assessment);
            }

            return true;
        }

        private static void AddCreationInfo(List<SpdxElement> graph, SpdxIds ids, string created)
        {
            string orgId = ids.Next("Organization-Winsider");
            string toolId = ids.Next("Tool-CustomBuildTool");

            graph.Add(new SpdxElement
            {
                Type = "CreationInfo",
                NodeId = CreationInfoId,
                SpecVersion = SpdxSpecVersion,
                Created = created,
                CreatedBy = [orgId],
                CreatedUsing = [toolId]
            });
            graph.Add(new SpdxElement
            {
                Type = "Organization",
                SpdxId = orgId,
                CreationInfo = CreationInfoId,
                Name = Supplier
            });
            graph.Add(new SpdxElement
            {
                Type = "Tool",
                SpdxId = toolId,
                CreationInfo = CreationInfoId,
                Name = "CustomBuildTool"
            });
        }

        private static void AddTool(List<SpdxElement> graph, SpdxIds ids, List<string> tools, string name, string version, string purlPrefix)
        {
            if (string.IsNullOrWhiteSpace(version))
            {
                Console.WriteLine($"SBOM: {name} version unavailable (run from a configured build environment); omitted from SBOM.");
                return;
            }

            string id = ids.Next("Package-" + name.Replace(' ', '-'));
            graph.Add(Package(id, name, version, purlPrefix + version, "NOASSERTION", "application"));
            tools.Add(id);
        }

        private static SpdxElement Package(string id, string name, string version, string purl, string downloadLocation, string purpose)
        {
            return new SpdxElement
            {
                Type = "software_Package",
                SpdxId = id,
                CreationInfo = CreationInfoId,
                Name = name,
                PackageVersion = version,
                PackageUrl = purl,
                DownloadLocation = downloadLocation,
                PrimaryPurpose = purpose,
                ExternalIdentifier =
                [
                    new SpdxExternalIdentifier
                    {
                        Type = "ExternalIdentifier",
                        ExternalIdentifierType = "packageUrl",
                        Identifier = purl
                    }
                ]
            };
        }

        private static SpdxElement FileElement(string id, string path, string purpose)
        {
            using var stream = File.OpenRead(path);

            return new SpdxElement
            {
                Type = "software_File",
                SpdxId = id,
                CreationInfo = CreationInfoId,
                Name = Path.GetFileName(path),
                PrimaryPurpose = purpose,
                VerifiedUsing =
                [
                    new SpdxHash
                    {
                        Type = "Hash",
                        Algorithm = "sha256",
                        HashValue = Convert.ToHexStringLower(SHA256.HashData(stream))
                    }
                ]
            };
        }

        private static SpdxElement Relationship(string id, string from, string type, List<string> to)
        {
            return new SpdxElement
            {
                Type = "Relationship",
                SpdxId = id,
                CreationInfo = CreationInfoId,
                From = from,
                RelationshipType = type,
                To = to
            };
        }

        private static SpdxElement Document(string id, string name, string rootElement, List<string> profiles)
        {
            return new SpdxElement
            {
                Type = "SpdxDocument",
                SpdxId = id,
                CreationInfo = CreationInfoId,
                Name = name,
                ProfileConformance = profiles,
                RootElement = [rootElement]
            };
        }

        private static void Write(string path, List<SpdxElement> graph)
        {
            var document = new SpdxGraphDocument
            {
                Context = SpdxContext,
                Graph = graph
            };

            using var stream = File.Create(path);
            JsonSerializer.Serialize(stream, document, BuildDriverSbomJsonContext.Default.SpdxGraphDocument);
        }

        private static string FileVersion(string path)
        {
            var info = FileVersionInfo.GetVersionInfo(path);
            return $"{info.FileMajorPart}.{info.FileMinorPart}.{info.FileBuildPart}.{info.FilePrivatePart}";
        }

        private static string SymCryptVersion(string repo)
        {
            string header = Path.Join([repo, "tools", "thirdparty", "SymCrypt", "inc", "symcrypt_internal_shared.inc"]);

            if (!File.Exists(header))
                return null;

            string api = null, minor = null, patch = null;

            foreach (var line in File.ReadLines(header))
            {
                var parts = line.Split((char[])null, StringSplitOptions.RemoveEmptyEntries);

                if (parts.Length < 3 || parts[0] != "#define")
                    continue;

                switch (parts[1])
                {
                    case "SYMCRYPT_CODE_VERSION_API": api = parts[2]; break;
                    case "SYMCRYPT_CODE_VERSION_MINOR": minor = parts[2]; break;
                    case "SYMCRYPT_CODE_VERSION_PATCH": patch = parts[2]; break;
                }
            }

            return !string.IsNullOrWhiteSpace(api) && !string.IsNullOrWhiteSpace(minor) && !string.IsNullOrWhiteSpace(patch) ? $"{api}.{minor}.{patch}" : null;
        }

        /// <summary>
        /// Generates unique SPDX element identifiers under a document base URI.
        /// </summary>
        private sealed class SpdxIds(string baseId)
        {
            private int counter;

            public string Next(string name)
            {
                return $"{baseId}/{Uri.EscapeDataString(name)}-{++this.counter}";
            }
        }

        /// <summary>
        /// Source generation context for the SPDX 3.0 SBOM/VEX documents and the VEX input file.
        /// </summary>
        [JsonSourceGenerationOptions(DefaultIgnoreCondition = JsonIgnoreCondition.WhenWritingNull, GenerationMode = JsonSourceGenerationMode.Default, WriteIndented = true)]
        [JsonSerializable(typeof(SpdxGraphDocument))]
        [JsonSerializable(typeof(List<VexInputEntry>))]
        internal partial class BuildDriverSbomJsonContext : JsonSerializerContext;

        public class SpdxGraphDocument
        {
            [JsonPropertyName("@context")]
            public string Context { get; set; }

            [JsonPropertyName("@graph")]
            public List<SpdxElement> Graph { get; set; }
        }

        /// <summary>
        /// Flattened SPDX 3.0 graph element; unused properties are null and omitted on write.
        /// </summary>
        public class SpdxElement
        {
            [JsonPropertyName("type")]
            public string Type { get; set; }

            [JsonPropertyName("@id")]
            public string NodeId { get; set; }

            [JsonPropertyName("spdxId")]
            public string SpdxId { get; set; }

            [JsonPropertyName("creationInfo")]
            public string CreationInfo { get; set; }

            [JsonPropertyName("specVersion")]
            public string SpecVersion { get; set; }

            [JsonPropertyName("created")]
            public string Created { get; set; }

            [JsonPropertyName("createdBy")]
            public List<string> CreatedBy { get; set; }

            [JsonPropertyName("createdUsing")]
            public List<string> CreatedUsing { get; set; }

            [JsonPropertyName("name")]
            public string Name { get; set; }

            [JsonPropertyName("comment")]
            public string Comment { get; set; }

            [JsonPropertyName("software_packageVersion")]
            public string PackageVersion { get; set; }

            [JsonPropertyName("software_packageUrl")]
            public string PackageUrl { get; set; }

            [JsonPropertyName("software_downloadLocation")]
            public string DownloadLocation { get; set; }

            [JsonPropertyName("software_primaryPurpose")]
            public string PrimaryPurpose { get; set; }

            [JsonPropertyName("externalIdentifier")]
            public List<SpdxExternalIdentifier> ExternalIdentifier { get; set; }

            [JsonPropertyName("verifiedUsing")]
            public List<SpdxHash> VerifiedUsing { get; set; }

            [JsonPropertyName("from")]
            public string From { get; set; }

            [JsonPropertyName("relationshipType")]
            public string RelationshipType { get; set; }

            [JsonPropertyName("to")]
            public List<string> To { get; set; }

            [JsonPropertyName("simplelicensing_licenseExpression")]
            public string LicenseExpression { get; set; }

            [JsonPropertyName("profileConformance")]
            public List<string> ProfileConformance { get; set; }

            [JsonPropertyName("rootElement")]
            public List<string> RootElement { get; set; }

            [JsonPropertyName("security_assessedElement")]
            public string AssessedElement { get; set; }

            [JsonPropertyName("security_publishedTime")]
            public string PublishedTime { get; set; }

            [JsonPropertyName("security_justificationType")]
            public string JustificationType { get; set; }

            [JsonPropertyName("security_impactStatement")]
            public string ImpactStatement { get; set; }

            [JsonPropertyName("security_actionStatement")]
            public string ActionStatement { get; set; }
        }

        public class SpdxExternalIdentifier
        {
            [JsonPropertyName("type")]
            public string Type { get; set; }

            [JsonPropertyName("externalIdentifierType")]
            public string ExternalIdentifierType { get; set; }

            [JsonPropertyName("identifier")]
            public string Identifier { get; set; }

            [JsonPropertyName("identifierLocator")]
            public List<string> IdentifierLocator { get; set; }
        }

        public class SpdxHash
        {
            [JsonPropertyName("type")]
            public string Type { get; set; }

            [JsonPropertyName("algorithm")]
            public string Algorithm { get; set; }

            [JsonPropertyName("hashValue")]
            public string HashValue { get; set; }
        }

        public class VexInputEntry
        {
            [JsonPropertyName("cve")]
            public string Cve { get; set; }

            [JsonPropertyName("status")]
            public string Status { get; set; }

            [JsonPropertyName("justification")]
            public string Justification { get; set; }

            [JsonPropertyName("statement")]
            public string Statement { get; set; }
        }
    }
}
