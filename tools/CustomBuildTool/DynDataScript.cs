/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     jxy-s   2026
 *
 */

namespace CustomBuildTool
{
    /// <summary>
    /// Builds the script files from the script manifest: one compiled script with its name, limits and routines.
    /// </summary>
    public static class DynDataScript
    {
        /// <summary>
        /// The script manifest, relative to the build working folder.
        /// </summary>
        private const string ManifestFile = "kphlib\\kphscript.xml";

        /// <summary>
        /// The folder script files in the manifest are relative to.
        /// </summary>
        private const string ScriptFolder = "kphlib";

        /// <summary>
        /// The folder of an output directory that holds the script files.
        /// </summary>
        private const string OutputFolder = "scripts";

        /// <summary>
        /// The header that names the Lua release of the tree, relative to the build working folder.
        /// </summary>
        private const string LuaHeaderFile = "tools\\thirdparty\\lua\\src\\lua.h";

        /// <summary>
        /// The script file version. The driver loads only files of its own version.
        /// </summary>
        private const UInt32 Version = 1;

        /// <summary>
        /// The Lua release scripts are compiled and run with, by the script file version that began using it.
        /// The driver cannot tell the release of a chunk, so a new release takes a new row with a new Version,
        /// and the driver rebuilt and shipped.
        /// </summary>
        private static readonly (UInt32 Version, string Release)[] LuaReleases =
        [
            (1, "5.5.1"),
        ];

        private const int RoutineSize = 8;

        /// <summary>
        /// The architectures a script can be built for, by their names in the manifest.
        /// </summary>
        private static readonly string[] Architectures = ["amd64", "arm64"];
        private const int ChunkAlignment = 8;

        // The layout of a script file. KSystemInformer\include\script.h asserts the same numbers.
        private const UInt32 FileMagic = 0x5349534B;
        private const int FileHeaderSize = 72;

        /// <summary>
        /// The client levels a routine can require, by their names in the manifest.
        /// </summary>
        private static readonly (string Name, UInt16 Value)[] Levels =
        [
            ("low", 0),
            ("medium", 1),
            ("high", 2),
            ("maximum", 3),
        ];

        /// <summary>
        /// The C header content for the script file version and the script levels.
        /// </summary>
        public static readonly string HeaderC =
$$"""

#define KPH_SCRIPT_VERSION                      ((ULONG){{Version}})

#define KPH_SCRIPT_LEVEL_LOW                    ((USHORT){{Levels[0].Value}})
#define KPH_SCRIPT_LEVEL_MEDIUM                 ((USHORT){{Levels[1].Value}})
#define KPH_SCRIPT_LEVEL_HIGH                   ((USHORT){{Levels[2].Value}})
#define KPH_SCRIPT_LEVEL_MAXIMUM                ((USHORT){{Levels[3].Value}})
""";

        /// <summary>
        /// A routine a script exports.
        /// </summary>
        private sealed class Routine
        {
            public string Name;
            public UInt16 Level;
        }

        /// <summary>
        /// A script of the manifest.
        /// </summary>
        private sealed class Script
        {
            public string Name;
            public string File;
            public string[] Architectures;
            public UInt16 Level;
            public Limits? Limits;
            public List<Routine> Routines = [];
            public byte[] Chunk;
        }

        /// <summary>
        /// The limits of the manifest.
        /// </summary>
        private struct Limits
        {
            public UInt64 Instructions;
            public UInt64 CleanupInstructions;
            public UInt32 Milliseconds;
            public UInt32 CleanupMilliseconds;
            public UInt32 Memory;
            public UInt32 CleanupMemory;
        }

        /// <summary>
        /// Rounds a length up to a multiple of an alignment.
        /// </summary>
        /// <param name="Value">The length.</param>
        /// <param name="Alignment">The alignment, a power of two.</param>
        /// <returns>The rounded length.</returns>
        public static int AlignUp(int Value, int Alignment)
        {
            return (Value + Alignment - 1) & ~(Alignment - 1);
        }

        /// <summary>
        /// Reads an attribute that must be present.
        /// </summary>
        private static string RequiredAttribute(XmlNode Node, string Name)
        {
            string value = Node.Attributes?.GetNamedItem(Name)?.Value;

            if (string.IsNullOrWhiteSpace(value))
                throw new Exception($"{ManifestFile}: <{Node.Name}> needs a {Name} attribute");

            return value;
        }

        /// <summary>
        /// Refuses attributes and child elements the manifest does not define, so a misspelt one is not ignored.
        /// </summary>
        private static void CheckNode(XmlNode Node, string[] Attributes, string[] Children)
        {
            if (Node.Attributes != null)
            {
                foreach (XmlAttribute attribute in Node.Attributes)
                {
                    if (attribute.Name.Equals("desc", StringComparison.Ordinal))
                        continue;
                    if (!Attributes.Contains(attribute.Name, StringComparer.Ordinal))
                        throw new Exception($"{ManifestFile}: <{Node.Name}> has no attribute {attribute.Name}");
                }
            }

            foreach (XmlNode child in Node.ChildNodes)
            {
                if (child.NodeType != XmlNodeType.Element)
                    continue;
                if (!Children.Contains(child.Name, StringComparer.Ordinal))
                    throw new Exception($"{ManifestFile}: <{Node.Name}> has no element <{child.Name}>");
            }
        }

        /// <summary>
        /// Checks a script or routine name: ASCII letters, digits, '_' and '-'.
        /// </summary>
        private static bool IsValidName(string Name)
        {
            if (Name.Length == 0 || Name.Length > UInt16.MaxValue)
                return false;

            foreach (char c in Name)
            {
                if (!(char.IsAsciiLetterOrDigit(c) || c == '_' || c == '-'))
                    return false;
            }

            return true;
        }

        /// <summary>
        /// Reads a limit, which must be a number above zero.
        /// </summary>
        private static UInt64 ParseLimit(XmlNode Node, string Name, UInt64 Maximum)
        {
            string text = RequiredAttribute(Node, Name);

            if (!UInt64.TryParse(text, NumberStyles.None, CultureInfo.InvariantCulture, out UInt64 value) ||
                value == 0 ||
                value > Maximum)
            {
                throw new Exception($"{ManifestFile}: limit {Name}=\"{text}\" is not a number from 1 to {Maximum}");
            }

            return value;
        }

        /// <summary>
        /// Reads a limits element: all six limits.
        /// </summary>
        private static Limits ParseLimits(XmlNode Node)
        {
            CheckNode(
                Node,
                ["instructions", "milliseconds", "memory", "cleanup-instructions", "cleanup-milliseconds", "cleanup-memory"],
                []);

            return new Limits
            {
                Instructions = ParseLimit(Node, "instructions", UInt64.MaxValue),
                CleanupInstructions = ParseLimit(Node, "cleanup-instructions", UInt64.MaxValue),
                Milliseconds = (UInt32)ParseLimit(Node, "milliseconds", UInt32.MaxValue),
                CleanupMilliseconds = (UInt32)ParseLimit(Node, "cleanup-milliseconds", UInt32.MaxValue),
                Memory = (UInt32)ParseLimit(Node, "memory", UInt32.MaxValue),
                CleanupMemory = (UInt32)ParseLimit(Node, "cleanup-memory", UInt32.MaxValue),
            };
        }

        /// <summary>
        /// Reads the script manifest.
        /// </summary>
        /// <param name="FileName">The manifest file.</param>
        /// <param name="ManifestLimits">Receives the limits.</param>
        /// <param name="Scripts">Receives the scripts, in the order of the manifest.</param>
        private static void ParseManifest(string FileName, out Limits ManifestLimits, out List<Script> Scripts)
        {
            var xml = new XmlDocument();
            var xmlSettings = new XmlReaderSettings
            {
                DtdProcessing = DtdProcessing.Prohibit,
                IgnoreWhitespace = true,
                IgnoreComments = true
            };
            using (var xmlReader = XmlReader.Create(FileName, xmlSettings))
            {
                xml.Load(xmlReader);
            }

            XmlNode root = xml.SelectSingleNode("/scripts") ??
                throw new Exception($"{ManifestFile}: the root element is not <scripts>");

            CheckNode(root, [], ["limits", "script"]);

            XmlNodeList limitsNodes = root.SelectNodes("limits");

            if (limitsNodes == null || limitsNodes.Count != 1)
                throw new Exception($"{ManifestFile}: one <limits> element is needed");

            ManifestLimits = ParseLimits(limitsNodes[0]);

            Scripts = [];

            foreach (XmlNode scriptNode in root.SelectNodes("script"))
            {
                CheckNode(scriptNode, ["name", "file", "arch", "level"], ["routine", "limits"]);

                var script = new Script
                {
                    Name = RequiredAttribute(scriptNode, "name"),
                    File = RequiredAttribute(scriptNode, "file"),
                    Architectures = RequiredAttribute(scriptNode, "arch").Split(','),
                };

                if (!IsValidName(script.Name))
                    throw new Exception($"{ManifestFile}: script name \"{script.Name}\": a name is ASCII letters, digits, '_' and '-'");

                foreach (string arch in script.Architectures)
                {
                    if (!Architectures.Contains(arch, StringComparer.Ordinal))
                        throw new Exception($"{ManifestFile}: script \"{script.Name}\": arch \"{arch}\" is not one of {string.Join(", ", Architectures)}");
                    if (script.Architectures.Count(a => a.Equals(arch, StringComparison.Ordinal)) > 1)
                        throw new Exception($"{ManifestFile}: script \"{script.Name}\": arch \"{arch}\" is listed twice");
                }

                if (Scripts.Any(s => s.Name.Equals(script.Name, StringComparison.Ordinal) &&
                                     s.Architectures.Intersect(script.Architectures, StringComparer.Ordinal).Any()))
                    throw new Exception($"{ManifestFile}: script \"{script.Name}\" is listed twice for one arch");

                foreach (XmlNode routineNode in scriptNode.SelectNodes("routine"))
                {
                    CheckNode(routineNode, ["name", "level"], []);

                    string name = RequiredAttribute(routineNode, "name");
                    string level = RequiredAttribute(routineNode, "level");
                    int index = Array.FindIndex(Levels, l => l.Name.Equals(level, StringComparison.Ordinal));

                    if (!IsValidName(name))
                        throw new Exception($"{ManifestFile}: routine name \"{name}\" of script \"{script.Name}\": a name is ASCII letters, digits, '_' and '-'");
                    if (index < 0)
                        throw new Exception($"{ManifestFile}: routine \"{name}\" of script \"{script.Name}\": level \"{level}\" is not one of {string.Join(", ", Levels.Select(l => l.Name))}");
                    if (script.Routines.Any(r => r.Name.Equals(name, StringComparison.Ordinal)))
                        throw new Exception($"{ManifestFile}: routine \"{name}\" of script \"{script.Name}\" is listed twice");

                    script.Routines.Add(new Routine { Name = name, Level = Levels[index].Value });
                }

                if (script.Routines.Count > UInt16.MaxValue)
                    throw new Exception($"{ManifestFile}: script \"{script.Name}\" lists too many routines");

                if (script.Routines.Count == 0)
                    throw new Exception($"{ManifestFile}: script \"{script.Name}\" lists no routine");

                string scriptLevel = scriptNode.Attributes?.GetNamedItem("level")?.Value;

                if (scriptLevel == null)
                {
                    script.Level = script.Routines.Min(r => r.Level);
                }
                else
                {
                    int index = Array.FindIndex(Levels, l => l.Name.Equals(scriptLevel, StringComparison.Ordinal));

                    if (index < 0)
                        throw new Exception($"{ManifestFile}: script \"{script.Name}\": level \"{scriptLevel}\" is not one of {string.Join(", ", Levels.Select(l => l.Name))}");

                    script.Level = Levels[index].Value;

                    Routine below = script.Routines.FirstOrDefault(r => r.Level < script.Level);

                    if (below != null)
                        throw new Exception($"{ManifestFile}: routine \"{below.Name}\" of script \"{script.Name}\" asks for a lower level than the script's \"{scriptLevel}\"");
                }

                XmlNodeList ownLimits = scriptNode.SelectNodes("limits");

                if (ownLimits.Count > 1)
                    throw new Exception($"{ManifestFile}: script \"{script.Name}\" has more than one <limits> element");
                if (ownLimits.Count == 1)
                    script.Limits = ParseLimits(ownLimits[0]);

                script.Routines.Sort((a, b) => string.CompareOrdinal(a.Name, b.Name));

                Scripts.Add(script);
            }

            Scripts.Sort((a, b) => string.CompareOrdinal(a.Name, b.Name));
        }

        /// <summary>
        /// Converts a build platform to the architecture it builds for.
        /// </summary>
        /// <param name="Platform">The platform: x64, ARM64 or Win32.</param>
        /// <returns>The architecture, by its name in the manifest; x86 for Win32, which no script is built for.</returns>
        public static string GetArchitecture(string Platform)
        {
            if (Platform.Equals("x64", StringComparison.OrdinalIgnoreCase))
                return "amd64";
            if (Platform.Equals("ARM64", StringComparison.OrdinalIgnoreCase))
                return "arm64";
            if (Platform.Equals("Win32", StringComparison.OrdinalIgnoreCase))
                return "x86";

            throw new Exception($"platform \"{Platform}\" is not x64, ARM64 or Win32");
        }

        /// <summary>
        /// Locates the script compiler built for the architecture this tool runs as.
        /// </summary>
        /// <returns>The path of the compiler.</returns>
        private static string GetCompilerPath()
        {
            string folder = RuntimeInformation.ProcessArchitecture switch
            {
                Architecture.X86 => "Release32",
                Architecture.X64 => "Release64",
                Architecture.Arm64 => "ReleaseARM64",
                _ => throw new Exception($"no script compiler for {RuntimeInformation.ProcessArchitecture}")
            };

            string path = Path.Join([Build.BuildWorkingFolder, $"tools\\kphluac\\bin\\{folder}\\kphluac.exe"]);

            if (!File.Exists(path))
                throw new Exception($"The script compiler is not built ({path}). Run build\\build_tools.cmd.");

            return path;
        }

        /// <summary>
        /// Checks that the script compiler was built from the Lua release of the tree.
        /// </summary>
        private static void CheckLuaRelease()
        {
            string header = File.ReadAllText(Path.Join([Build.BuildWorkingFolder, LuaHeaderFile]));
            List<string> numbers = [];

            foreach (string part in new[] { "MAJOR", "MINOR", "RELEASE" })
            {
                Match match = Regex.Match(header, $@"^#define\s+LUA_VERSION_{part}_N\s+(\d+)\s*$", RegexOptions.Multiline);

                if (!match.Success)
                    throw new Exception($"{LuaHeaderFile}: LUA_VERSION_{part}_N was not found");

                numbers.Add(match.Groups[1].Value);
            }

            string release = string.Join('.', numbers);

            for (int i = 1; i < LuaReleases.Length; i++)
            {
                if (LuaReleases[i].Version <= LuaReleases[i - 1].Version)
                    throw new Exception("DynDataScript.LuaReleases: a row needs a higher version than the row before it");
            }

            (UInt32 pinnedVersion, string pinnedRelease) = LuaReleases[^1];

            if (!release.Equals(pinnedRelease, StringComparison.Ordinal) || Version != pinnedVersion)
            {
                throw new Exception(
                    $"{LuaHeaderFile} is Lua {release} and the script version is {Version}, but " +
                    $"DynDataScript.LuaReleases pins Lua {pinnedRelease} to version {pinnedVersion}. A new Lua " +
                    "release needs a higher DynDataScript.Version, a new row in LuaReleases, and the driver rebuilt and shipped.");
            }

            List<string> arguments = ["-v"];
            int exitCode = Win32.CreateProcess(GetCompilerPath(), arguments, out string compilerRelease, false);

            if (exitCode != 0 || !compilerRelease.Trim().Equals(release, StringComparison.Ordinal))
                throw new Exception($"The script compiler was not built from Lua {release}. Run build\\build_tools.cmd.");
        }

        /// <summary>
        /// Compiles a script and checks it against its manifest entry.
        /// </summary>
        /// <param name="Target">The script.</param>
        /// <returns>The chunk.</returns>
        private static byte[] Compile(Script Target)
        {
            string compiler = GetCompilerPath();
            string source = Path.GetFullPath(Path.Join([Build.BuildWorkingFolder, ScriptFolder, Target.File]));
            string output = Path.Join([Path.GetTempPath(), $"kphluac-{Environment.ProcessId}-{Target.Name}.luac"]);

            if (!File.Exists(source))
                throw new Exception($"{ManifestFile}: script \"{Target.Name}\": {source} does not exist");

            List<string> arguments = ["-n", Target.Name, "-i", source, "-o", output];

            foreach (Routine routine in Target.Routines)
            {
                arguments.Add("-r");
                arguments.Add(routine.Name);
            }

            try
            {
                int exitCode = Win32.CreateProcess(compiler, arguments, out string messages, false);

                if (exitCode != 0)
                {
                    // The compiler's messages already name the file and the line.
                    if (!string.IsNullOrWhiteSpace(messages))
                        Program.PrintColorMessage(messages, ConsoleColor.Red);

                    throw new Exception($"script \"{Target.Name}\" was not built (kphluac exit code {exitCode})");
                }

                return Utils.ReadAllBytes(output);
            }
            finally
            {
                if (File.Exists(output))
                    File.Delete(output);
            }
        }

        /// <summary>
        /// Generates one script file image for each script of the manifest built for an architecture.
        /// </summary>
        /// <param name="Architecture">The architecture.</param>
        /// <returns>The scripts by name with their images; empty when the tree has no script manifest.</returns>
        public static List<(string Name, byte[] Image)> GenerateFiles(string Architecture)
        {
            string manifestFile = Path.Join([Build.BuildWorkingFolder, ManifestFile]);
            var files = new List<(string Name, byte[] Image)>();

            if (!File.Exists(manifestFile))
                return files;

            CheckLuaRelease();

            ParseManifest(manifestFile, out Limits limits, out List<Script> scripts);

            foreach (Script script in scripts)
            {
                if (!script.Architectures.Contains(Architecture, StringComparer.Ordinal))
                    continue;

                script.Chunk = Compile(script);
                files.Add((script.Name, LayoutFile(script.Limits ?? limits, script)));
            }

            return files;
        }

        /// <summary>
        /// Lays out one script file: the header, the routines, the names, then the chunk.
        /// </summary>
        /// <param name="ScriptLimits">The limits of the script.</param>
        /// <param name="Target">The script, compiled.</param>
        /// <returns>The file image.</returns>
        private static byte[] LayoutFile(Limits ScriptLimits, Script Target)
        {
            int routinesOffset = FileHeaderSize;
            int namesOffset = routinesOffset + (Target.Routines.Count * RoutineSize);
            int namesLength = Target.Name.Length + Target.Routines.Sum(r => r.Name.Length);
            int chunkOffset = AlignUp(namesOffset + namesLength, ChunkAlignment);
            int length = chunkOffset + Target.Chunk.Length;

            byte[] image = new byte[length];
            Span<byte> span = image;

            BinaryPrimitives.WriteUInt32LittleEndian(span[0..], FileMagic);
            BinaryPrimitives.WriteUInt32LittleEndian(span[4..], Version);
            BinaryPrimitives.WriteUInt32LittleEndian(span[8..], (UInt32)length);
            BinaryPrimitives.WriteUInt32LittleEndian(span[12..], Target.Level);
            BinaryPrimitives.WriteUInt64LittleEndian(span[16..], ScriptLimits.Instructions);
            BinaryPrimitives.WriteUInt64LittleEndian(span[24..], ScriptLimits.CleanupInstructions);
            BinaryPrimitives.WriteUInt32LittleEndian(span[32..], ScriptLimits.Milliseconds);
            BinaryPrimitives.WriteUInt32LittleEndian(span[36..], ScriptLimits.CleanupMilliseconds);
            BinaryPrimitives.WriteUInt32LittleEndian(span[40..], ScriptLimits.Memory);
            BinaryPrimitives.WriteUInt32LittleEndian(span[44..], ScriptLimits.CleanupMemory);
            BinaryPrimitives.WriteUInt32LittleEndian(span[48..], (UInt32)namesOffset);
            BinaryPrimitives.WriteUInt32LittleEndian(span[52..], (UInt32)routinesOffset);
            BinaryPrimitives.WriteUInt32LittleEndian(span[56..], (UInt32)chunkOffset);
            BinaryPrimitives.WriteUInt32LittleEndian(span[60..], (UInt32)Target.Chunk.Length);
            BinaryPrimitives.WriteUInt16LittleEndian(span[64..], (UInt16)Target.Name.Length);
            BinaryPrimitives.WriteUInt16LittleEndian(span[66..], (UInt16)Target.Routines.Count);

            int nameAt = namesOffset + Encoding.ASCII.GetBytes(Target.Name, span[namesOffset..]);
            int routineAt = routinesOffset;

            foreach (Routine routine in Target.Routines)
            {
                BinaryPrimitives.WriteUInt32LittleEndian(span[routineAt..], (UInt32)nameAt);
                BinaryPrimitives.WriteUInt16LittleEndian(span[(routineAt + 4)..], (UInt16)routine.Name.Length);
                BinaryPrimitives.WriteUInt16LittleEndian(span[(routineAt + 6)..], routine.Level);
                routineAt += RoutineSize;

                nameAt += Encoding.ASCII.GetBytes(routine.Name, span[nameAt..]);
            }

            Target.Chunk.CopyTo(span[chunkOffset..]);

            return image;
        }

        /// <summary>
        /// Checks whether a file is a script file, which the script key signs.
        /// </summary>
        /// <param name="FileName">The path to the file.</param>
        /// <returns>True for a .bin file in a scripts folder; otherwise, false.</returns>
        public static bool IsScriptFile(string FileName)
        {
            string folder = Path.GetFileName(Path.GetDirectoryName(FileName));

            return string.Equals(Path.GetExtension(FileName), ".bin", StringComparison.OrdinalIgnoreCase) &&
                   string.Equals(folder, OutputFolder, StringComparison.OrdinalIgnoreCase);
        }

        /// <summary>
        /// Writes and signs the script files of the manifest in the scripts folder of an output directory.
        /// </summary>
        /// <param name="OutDir">The output directory.</param>
        /// <param name="Platform">The platform the output directory is built for.</param>
        /// <param name="StrictChecks">Whether to enforce strict signature checks.</param>
        /// <returns>True if successful; otherwise, false.</returns>
        public static bool Execute(string OutDir, string Platform, bool StrictChecks)
        {
            string folder = Path.Join([OutDir, OutputFolder]);
            List<(string Name, byte[] Image)> files = GenerateFiles(GetArchitecture(Platform));

            Win32.CreateDirectory(folder);

            foreach (string stale in Directory.EnumerateFiles(folder))
            {
                string extension = Path.GetExtension(stale);
                string name = Path.GetFileNameWithoutExtension(stale);

                if ((extension.Equals(".bin", StringComparison.OrdinalIgnoreCase) ||
                     extension.Equals(".sig", StringComparison.OrdinalIgnoreCase)) &&
                    !files.Any(f => f.Name.Equals(name, StringComparison.OrdinalIgnoreCase)))
                {
                    File.Delete(stale);
                }
            }

            foreach ((string name, byte[] image) in files)
            {
                string file = Path.Join([folder, $"{name}.bin"]);
                string sigFile = Path.ChangeExtension(file, ".sig");

                if (File.Exists(file) &&
                    Utils.ReadAllBytes(file).AsSpan().SequenceEqual(image) &&
                    BuildVerify.IsValidKphSigFile(sigFile))
                {
                    Program.PrintColorMessage($"Script file -> {file} ({image.Length} bytes)", ConsoleColor.Cyan);
                    continue;
                }

                if (File.Exists(file))
                    File.Delete(file);
                if (File.Exists(sigFile))
                    File.Delete(sigFile);

                Utils.WriteAllBytes(file, image);

                if (!BuildVerify.CreateSigFile("script", file, StrictChecks))
                    return false;

                if (!File.Exists(sigFile))
                    Program.PrintColorMessage($"[No script key, nothing signed] ({file})", ConsoleColor.Yellow);

                Program.PrintColorMessage($"Script file -> {file} ({image.Length} bytes)", ConsoleColor.Cyan);
            }

            return true;
        }
    }
}
