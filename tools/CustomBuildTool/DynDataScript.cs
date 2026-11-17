/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     jxy-s
 *
 */

namespace CustomBuildTool
{
    /// <summary>
    /// Builds the script section of the dynamic data from the script manifest: the limits, a directory of
    /// scripts with their routines, and the compiled chunks.
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
        /// The Lua release scripts are compiled and run with, by the dynamic data version that began using it.
        /// The driver cannot tell the release of a chunk, so a new release takes a new row with a new
        /// DynData.Version, and the driver rebuilt and shipped.
        /// </summary>
        private static readonly (UInt32 Version, string Release)[] LuaReleases =
        [
            (17, "5.5.1"),
        ];

        // The layout of the section. The generated header asserts the same numbers.
        private const int LimitsSize = 32;
        private const int ScriptsOffset = 36;
        private const int ScriptSize = 20;
        private const int RoutineSize = 8;
        private const int ChunkAlignment = 8;

        // The layout of a script file. KSystemInformer\include\script.h asserts the same numbers.
        private const UInt32 FileMagic = 0x5349534B;
        private const int FileHeaderSize = 64;

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
        /// The C header content for the script section.
        /// </summary>
        public static readonly string HeaderC =
$$"""

#define KPH_DYN_SCRIPT_LEVEL_LOW                ((USHORT){{Levels[0].Value}})
#define KPH_DYN_SCRIPT_LEVEL_MEDIUM             ((USHORT){{Levels[1].Value}})
#define KPH_DYN_SCRIPT_LEVEL_HIGH               ((USHORT){{Levels[2].Value}})
#define KPH_DYN_SCRIPT_LEVEL_MAXIMUM            ((USHORT){{Levels[3].Value}})

typedef struct _KPH_DYN_SCRIPT_LIMITS
{
    ULONG64 Instructions;
    ULONG64 CleanupInstructions;
    ULONG Milliseconds;
    ULONG CleanupMilliseconds;
    ULONG Memory;
    ULONG CleanupMemory;
} KPH_DYN_SCRIPT_LIMITS, *PKPH_DYN_SCRIPT_LIMITS;

typedef struct _KPH_DYN_SCRIPT_ROUTINE
{
    ULONG NameOffset;
    USHORT NameLength;
    USHORT Level;
} KPH_DYN_SCRIPT_ROUTINE, *PKPH_DYN_SCRIPT_ROUTINE;

typedef struct _KPH_DYN_SCRIPT
{
    ULONG NameOffset;
    USHORT NameLength;
    USHORT RoutineCount;
    ULONG RoutinesOffset;
    ULONG ChunkOffset;
    ULONG ChunkLength;
} KPH_DYN_SCRIPT, *PKPH_DYN_SCRIPT;

typedef struct _KPH_DYN_SCRIPT_SECTION
{
    KPH_DYN_SCRIPT_LIMITS Limits;
    ULONG Count;
    KPH_DYN_SCRIPT Scripts[ANYSIZE_ARRAY];
    // KPH_DYN_SCRIPT_ROUTINE Routines[ANYSIZE_ARRAY];
    // CHAR Names[ANYSIZE_ARRAY];
    // BYTE Chunks[ANYSIZE_ARRAY];
} KPH_DYN_SCRIPT_SECTION, *PKPH_DYN_SCRIPT_SECTION;

C_ASSERT(sizeof(KPH_DYN_SCRIPT_LIMITS) == {{LimitsSize}});
C_ASSERT(sizeof(KPH_DYN_SCRIPT_ROUTINE) == {{RoutineSize}});
C_ASSERT(sizeof(KPH_DYN_SCRIPT) == {{ScriptSize}});
C_ASSERT(FIELD_OFFSET(KPH_DYN_SCRIPT_SECTION, Scripts) == {{ScriptsOffset}});
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
        /// Generates the script section from the script manifest.
        /// </summary>
        /// <param name="Version">The dynamic data version.</param>
        /// <param name="ScriptCount">Receives the number of scripts.</param>
        /// <param name="RoutineCount">Receives the number of routines of all scripts.</param>
        /// <returns>The section, or null when the tree has no script manifest.</returns>
        public static byte[] Generate(UInt32 Version, out int ScriptCount, out int RoutineCount)
        {
            string manifestFile = Path.Join([Build.BuildWorkingFolder, ManifestFile]);

            ScriptCount = 0;
            RoutineCount = 0;

            if (!File.Exists(manifestFile))
                return null;

            CheckLuaRelease(Version);

            ParseManifest(manifestFile, out Limits limits, out List<Script> scripts);

            foreach (Script script in scripts)
            {
                script.Chunk = Compile(script);
                RoutineCount += script.Routines.Count;
            }

            ScriptCount = scripts.Count;

            return Layout(limits, scripts, RoutineCount);
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
                CheckNode(scriptNode, ["name", "file", "level"], ["routine", "limits"]);

                var script = new Script
                {
                    Name = RequiredAttribute(scriptNode, "name"),
                    File = RequiredAttribute(scriptNode, "file"),
                };

                if (!IsValidName(script.Name))
                    throw new Exception($"{ManifestFile}: script name \"{script.Name}\": a name is ASCII letters, digits, '_' and '-'");
                if (Scripts.Any(s => s.Name.Equals(script.Name, StringComparison.Ordinal)))
                    throw new Exception($"{ManifestFile}: script \"{script.Name}\" is listed twice");

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

            // The driver searches scripts and routines by name with a binary search.
            Scripts.Sort((a, b) => string.CompareOrdinal(a.Name, b.Name));
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
        /// Checks the Lua release of the tree and of the script compiler against the pinned release.
        /// </summary>
        /// <param name="Version">The dynamic data version.</param>
        private static void CheckLuaRelease(UInt32 Version)
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

            if (!release.Equals(pinnedRelease, StringComparison.Ordinal) || Version < pinnedVersion)
            {
                throw new Exception(
                    $"{LuaHeaderFile} is Lua {release} and the dynamic data version is {Version}, but " +
                    $"DynDataScript.LuaReleases pins Lua {pinnedRelease} from version {pinnedVersion}. A new Lua " +
                    "release needs a higher DynData.Version, a new row in LuaReleases, and the driver rebuilt and shipped.");
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
        /// Lays out the section: the limits, the scripts, their routines, the names, then the chunks.
        /// </summary>
        /// <param name="ManifestLimits">The limits.</param>
        /// <param name="Scripts">The scripts, compiled.</param>
        /// <param name="RoutineCount">The number of routines of all scripts.</param>
        /// <returns>The section.</returns>
        private static byte[] Layout(Limits ManifestLimits, List<Script> Scripts, int RoutineCount)
        {
            int routinesOffset = ScriptsOffset + (Scripts.Count * ScriptSize);
            int namesOffset = routinesOffset + (RoutineCount * RoutineSize);
            int namesLength = Scripts.Sum(s => s.Name.Length + s.Routines.Sum(r => r.Name.Length));
            int length = AlignUp(namesOffset + namesLength, ChunkAlignment);

            foreach (Script script in Scripts)
            {
                length = AlignUp(length + script.Chunk.Length, ChunkAlignment);
            }

            byte[] section = new byte[length];
            Span<byte> span = section;

            BinaryPrimitives.WriteUInt64LittleEndian(span[0..], ManifestLimits.Instructions);
            BinaryPrimitives.WriteUInt64LittleEndian(span[8..], ManifestLimits.CleanupInstructions);
            BinaryPrimitives.WriteUInt32LittleEndian(span[16..], ManifestLimits.Milliseconds);
            BinaryPrimitives.WriteUInt32LittleEndian(span[20..], ManifestLimits.CleanupMilliseconds);
            BinaryPrimitives.WriteUInt32LittleEndian(span[24..], ManifestLimits.Memory);
            BinaryPrimitives.WriteUInt32LittleEndian(span[28..], ManifestLimits.CleanupMemory);
            BinaryPrimitives.WriteUInt32LittleEndian(span[LimitsSize..], (UInt32)Scripts.Count);

            int scriptAt = ScriptsOffset;
            int routineAt = routinesOffset;
            int nameAt = namesOffset;
            int chunkAt = AlignUp(namesOffset + namesLength, ChunkAlignment);

            foreach (Script script in Scripts)
            {
                BinaryPrimitives.WriteUInt32LittleEndian(span[scriptAt..], (UInt32)nameAt);
                BinaryPrimitives.WriteUInt16LittleEndian(span[(scriptAt + 4)..], (UInt16)script.Name.Length);
                BinaryPrimitives.WriteUInt16LittleEndian(span[(scriptAt + 6)..], (UInt16)script.Routines.Count);
                BinaryPrimitives.WriteUInt32LittleEndian(span[(scriptAt + 8)..], (UInt32)routineAt);
                BinaryPrimitives.WriteUInt32LittleEndian(span[(scriptAt + 12)..], (UInt32)chunkAt);
                BinaryPrimitives.WriteUInt32LittleEndian(span[(scriptAt + 16)..], (UInt32)script.Chunk.Length);
                scriptAt += ScriptSize;

                nameAt += Encoding.ASCII.GetBytes(script.Name, span[nameAt..]);

                foreach (Routine routine in script.Routines)
                {
                    BinaryPrimitives.WriteUInt32LittleEndian(span[routineAt..], (UInt32)nameAt);
                    BinaryPrimitives.WriteUInt16LittleEndian(span[(routineAt + 4)..], (UInt16)routine.Name.Length);
                    BinaryPrimitives.WriteUInt16LittleEndian(span[(routineAt + 6)..], routine.Level);
                    routineAt += RoutineSize;

                    nameAt += Encoding.ASCII.GetBytes(routine.Name, span[nameAt..]);
                }

                script.Chunk.CopyTo(span[chunkAt..]);
                chunkAt = AlignUp(chunkAt + script.Chunk.Length, ChunkAlignment);
            }

            return section;
        }

        /// <summary>
        /// Generates one script file image for each script of the manifest.
        /// </summary>
        /// <returns>The scripts by name with their images; empty when the tree has no script manifest.</returns>
        public static List<(string Name, byte[] Image)> GenerateFiles()
        {
            string manifestFile = Path.Join([Build.BuildWorkingFolder, ManifestFile]);
            var files = new List<(string Name, byte[] Image)>();

            if (!File.Exists(manifestFile))
                return files;

            ParseManifest(manifestFile, out Limits limits, out List<Script> scripts);

            foreach (Script script in scripts)
            {
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
            BinaryPrimitives.WriteUInt32LittleEndian(span[4..], (UInt32)length);
            BinaryPrimitives.WriteUInt64LittleEndian(span[8..], ScriptLimits.Instructions);
            BinaryPrimitives.WriteUInt64LittleEndian(span[16..], ScriptLimits.CleanupInstructions);
            BinaryPrimitives.WriteUInt32LittleEndian(span[24..], ScriptLimits.Milliseconds);
            BinaryPrimitives.WriteUInt32LittleEndian(span[28..], ScriptLimits.CleanupMilliseconds);
            BinaryPrimitives.WriteUInt32LittleEndian(span[32..], ScriptLimits.Memory);
            BinaryPrimitives.WriteUInt32LittleEndian(span[36..], ScriptLimits.CleanupMemory);
            BinaryPrimitives.WriteUInt32LittleEndian(span[40..], (UInt32)namesOffset);
            BinaryPrimitives.WriteUInt32LittleEndian(span[44..], (UInt32)routinesOffset);
            BinaryPrimitives.WriteUInt32LittleEndian(span[48..], (UInt32)chunkOffset);
            BinaryPrimitives.WriteUInt32LittleEndian(span[52..], (UInt32)Target.Chunk.Length);
            BinaryPrimitives.WriteUInt16LittleEndian(span[56..], (UInt16)Target.Name.Length);
            BinaryPrimitives.WriteUInt16LittleEndian(span[58..], (UInt16)Target.Routines.Count);
            BinaryPrimitives.WriteUInt32LittleEndian(span[60..], Target.Level);

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
        /// <param name="StrictChecks">Whether to enforce strict signature checks.</param>
        /// <returns>True if successful; otherwise, false.</returns>
        public static bool Execute(string OutDir, bool StrictChecks)
        {
            string folder = Path.Join([OutDir, OutputFolder]);

            Win32.CreateDirectory(folder);

            foreach ((string name, byte[] image) in GenerateFiles())
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
