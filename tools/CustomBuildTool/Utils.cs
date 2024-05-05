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
    public static class Utils
    {
        public static readonly Encoding UTF8NoBOM = new UTF8Encoding(encoderShouldEmitUTF8Identifier: false, throwOnInvalidBytes: true);
        private static string GitFilePath;
        private static string MsBuildFilePath;
        private static string VsWhereFilePath;

        /// <summary>
        /// Splits a string into key-value pairs.
        /// </summary>
        public static Dictionary<string, string> ParseArgs(string[] args)
        {
            var dict = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase);
            string argPending = null;

            foreach (string s in args)
            {
                if (s.StartsWith("-", StringComparison.OrdinalIgnoreCase))
                {
                    dict.TryAdd(s, string.Empty);

                    argPending = s;
                }
                else
                {
                    if (argPending != null)
                    {
                        dict[argPending] = s;
                        argPending = null;
                    }
                    else
                    {
                        dict.TryAdd(string.Empty, s);
                    }
                }
            }

            return dict;
        }

        /// <summary>
        /// Splits a string into a dictionary.
        /// </summary>
        public static Dictionary<string, string> ParseInput(string[] args)
        {
            var dict = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase);

            foreach (string s in args)
            {
                dict.TryAdd(s, string.Empty);
            }

            return dict;
        }

        public static int ExecuteMsbuildCommand(string Command, out string OutputString)
        {
            string file = GetMsbuildFilePath();

            if (string.IsNullOrWhiteSpace(file))
            {
                OutputString = "[ExecuteMsbuildCommand] MsbuildFilePath is invalid.";
                return 3; // file not found.
            }

            return Win32.CreateProcess(file, Command, out OutputString, false);
        }

        public static string ExecuteVsWhereCommand(string Command)
        {
            string file = GetVswhereFilePath();

            if (string.IsNullOrWhiteSpace(file))
            {
                Program.PrintColorMessage("[ExecuteVsWhereCommand] VswhereFilePath is invalid.", ConsoleColor.Red);
                return null;
            }

            string output = Win32.ShellExecute(file, Command, false);
            output = output.Trim();
            return output;
        }

        public static string GetOutputDirectoryPath()
        {
            return $"{Build.BuildWorkingFolder}\\build\\output";
        }

        public static void CreateOutputDirectory()
        {
            string folder = GetOutputDirectoryPath();

            if (File.Exists(folder))
                return;

            try
            {
                Directory.CreateDirectory(folder);
            }
            catch (Exception ex)
            {
                Program.PrintColorMessage($"Error creating output directory: {ex}", ConsoleColor.Red);
            }
        }

        public static string GetVswhereFilePath()
        {
            if (string.IsNullOrWhiteSpace(VsWhereFilePath))
            {
                string[] vswherePathArray =
                {
                    "%ProgramFiles%\\Microsoft Visual Studio\\Installer\\vswhere.exe",
                    "%ProgramFiles(x86)%\\Microsoft Visual Studio\\Installer\\vswhere.exe",
                    "%ProgramW6432%\\Microsoft Visual Studio\\Installer\\vswhere.exe"
                };

                foreach (string path in vswherePathArray)
                {
                    string file = Win32.ExpandEnvironmentVariables(path);

                    if (File.Exists(file))
                    {
                        VsWhereFilePath = file;
                        break;
                    }
                }

                if (string.IsNullOrWhiteSpace(VsWhereFilePath))
                {
                    string file = Win32.SearchPath("vswhere.exe");

                    if (File.Exists(file))
                    {
                        VsWhereFilePath = file;
                    }
                }
            }

            return VsWhereFilePath;
        }

        private static string GetMsbuildFilePath()
        {
            if (string.IsNullOrWhiteSpace(MsBuildFilePath))
            {
                string[] MsBuildPathArray =
                {
                    "\\MSBuild\\Current\\Bin\\amd64\\MSBuild.exe",
                    "\\MSBuild\\Current\\Bin\\MSBuild.exe",
                    "\\MSBuild\\15.0\\Bin\\MSBuild.exe"
                };

                VisualStudioInstance instance = VisualStudio.GetVisualStudioInstance();

                if (instance != null)
                {
                    foreach (string path in MsBuildPathArray)
                    {
                        string file = instance.Path + path;

                        if (File.Exists(file))
                        {
                            MsBuildFilePath = file;
                            break;
                        }
                    }
                }

                if (string.IsNullOrWhiteSpace(MsBuildFilePath))
                {
                    // -latest -requires Microsoft.Component.MSBuild -find "MSBuild\**\Bin\MSBuild.exe"
                    string vswhereResult = ExecuteVsWhereCommand(
                        "-latest " +
                        "-prerelease " +
                        "-products * " +
                        "-requiresAny " +
                        "-requires Microsoft.Component.MSBuild " +
                        "-property installationPath "
                        );

                    if (!string.IsNullOrWhiteSpace(vswhereResult))
                    {
                        foreach (string path in MsBuildPathArray)
                        {
                            string file = vswhereResult + path;

                            if (File.Exists(file))
                            {
                                MsBuildFilePath = file;
                                break;
                            }
                        }
                    }
                }
            }

            return MsBuildFilePath;
        }

        public static string GetGitFilePath()
        {
            if (string.IsNullOrWhiteSpace(GitFilePath))
            {
                string[] GitPathArray =
                {
                    "%ProgramFiles%\\Git\\bin\\git.exe",
                    "%ProgramFiles(x86)%\\Git\\bin\\git.exe",
                    "%ProgramW6432%\\Git\\bin\\git.exe"
                };

                foreach (string path in GitPathArray)
                {
                    string file = Win32.ExpandEnvironmentVariables(path);

                    if (File.Exists(file))
                    {
                        GitFilePath = file;
                        break;
                    }
                }

                if (string.IsNullOrWhiteSpace(GitFilePath))
                {
                    string file = Win32.SearchPath("git.exe");

                    if (File.Exists(file))
                    {
                        GitFilePath = file;
                    }
                }
            }

            return GitFilePath;
        }

        private static string GetGitWorkPath(string DirectoryPath)
        {
            if (string.IsNullOrWhiteSpace(DirectoryPath))
                return null;

            if (!Directory.Exists($"{DirectoryPath}\\.git"))
                return null;

            return $"--git-dir=\"{DirectoryPath}\\.git\" --work-tree=\"{DirectoryPath}\" ";
        }

        public static string ExecuteGitCommand(string WorkingFolder, string Command)
        {
            string currentGitDirectory = GetGitWorkPath(WorkingFolder);

            if (string.IsNullOrWhiteSpace(currentGitDirectory))
            {
                Program.PrintColorMessage("[ExecuteGitCommand] WorkingFolder is invalid.", ConsoleColor.Red);
                return null;
            }

            string currentGitPath = GetGitFilePath();

            if (string.IsNullOrWhiteSpace(currentGitPath))
            {
                Program.PrintColorMessage("[ExecuteGitCommand] GitFilePath is invalid.", ConsoleColor.Red);
                return null;
            }

            string output = Win32.ShellExecute(currentGitPath, $"{currentGitDirectory} {Command}", false);
            output = output.Trim();
            return output;
        }

        public static string GetWindowsSdkIncludePath()
        {
            List<KeyValuePair<Version, string>> versionList = new List<KeyValuePair<Version, string>>();
            string kitsRoot = Win32.GetKeyValue(true, "Software\\Microsoft\\Windows Kits\\Installed Roots", "KitsRoot10", "%ProgramFiles(x86)%\\Windows Kits\\10\\");
            string kitsPath = Win32.ExpandEnvironmentVariables($"{kitsRoot}\\Include");

            if (Directory.Exists(kitsPath))
            {
                var windowsKitsDirectory = Directory.EnumerateDirectories(kitsPath);

                foreach (string path in windowsKitsDirectory)
                {
                    string name = Path.GetFileName(path);

                    if (Version.TryParse(name, out var version))
                    {
                        versionList.Add(new KeyValuePair<Version, string>(version, path));
                    }
                }

                versionList.Sort((first, second) => first.Key.CompareTo(second.Key));

                if (versionList.Count > 0)
                {
                    var result = versionList[versionList.Count - 1];

                    if (!string.IsNullOrWhiteSpace(result.Value))
                    {
                        return result.Value;
                    }
                }
            }

            return null;
        }

        public static string GetWindowsSdkPath()
        {
            List<KeyValuePair<Version, string>> versionList = new List<KeyValuePair<Version, string>>();
            string kitsRoot = Win32.GetKeyValue(true, "Software\\Microsoft\\Windows Kits\\Installed Roots", "KitsRoot10", "%ProgramFiles(x86)%\\Windows Kits\\10\\");
            string kitsPath = Win32.ExpandEnvironmentVariables($"{kitsRoot}\\bin");

            if (Directory.Exists(kitsPath))
            {
                var windowsKitsDirectory = Directory.EnumerateDirectories(kitsPath);

                foreach (string path in windowsKitsDirectory)
                {
                    string name = Path.GetFileName(path);

                    if (Version.TryParse(name, out var version))
                    {
                        versionList.Add(new KeyValuePair<Version, string>(version, path));
                    }
                }

                versionList.Sort((first, second) => first.Key.CompareTo(second.Key));

                if (versionList.Count > 0)
                {
                    var result = versionList[versionList.Count - 1];

                    if (!string.IsNullOrWhiteSpace(result.Value))
                    {
                        return result.Value;
                    }
                }
            }

            return null;
        }

        public static string GetWindowsSdkVersion()
        {
            List<string> versions = new List<string>();
            string kitsRoot = Win32.GetKeyValue(true, "Software\\Microsoft\\Windows Kits\\Installed Roots", "KitsRoot10", "%ProgramFiles(x86)%\\Windows Kits\\10\\");
            string kitsPath = Win32.ExpandEnvironmentVariables($"{kitsRoot}\\bin");

            if (Directory.Exists(kitsPath))
            {
                var windowsKitsDirectory = Directory.EnumerateDirectories(kitsPath);

                foreach (string path in windowsKitsDirectory)
                {
                    string name = Path.GetFileName(path);

                    if (Version.TryParse(name, out _))
                    {
                        versions.Add(name);
                    }
                }

                versions.Sort((p1, p2) =>
                {
                    if (Version.TryParse(p1, out Version v1) && Version.TryParse(p2, out Version v2))
                        return v1.CompareTo(v2);
                    else
                        return 1;
                });

                if (versions.Count > 0)
                {
                    string result = versions[^1];

                    if (!string.IsNullOrWhiteSpace(result))
                    {
                        return result;
                    }
                }
            }

            return null;
        }

        public static string GetVisualStudioVersion()
        {
            string msbuild = GetMsbuildFilePath();

            if (string.IsNullOrWhiteSpace(msbuild))
                return string.Empty;

            try
            {
                DirectoryInfo info = new DirectoryInfo(Path.GetDirectoryName(msbuild));

                while (info.Parent != null && info.Parent.Parent != null)
                {
                    info = info.Parent;

                    if (File.Exists($"{info.FullName}\\Common7\\IDE\\devenv.exe"))
                    {
                        FileVersionInfo currentInfo = FileVersionInfo.GetVersionInfo($"{info.FullName}\\Common7\\IDE\\devenv.exe");
                        return currentInfo.ProductVersion ?? string.Empty;
                    }
                }
            }
            catch (Exception ex)
            {
                Program.PrintColorMessage($"Unable to find devenv directory: {ex}", ConsoleColor.Red);
            }

            return string.Empty;
        }

        public static string GetMakeAppxPath()
        {
            string windowsSdkPath = Utils.GetWindowsSdkPath();

            if (string.IsNullOrWhiteSpace(windowsSdkPath))
                return null;

            string makeAppxPath = $"{windowsSdkPath}\\x64\\MakeAppx.exe";

            if (string.IsNullOrWhiteSpace(makeAppxPath))
                return null;

            if (!File.Exists(makeAppxPath))
            {
                string sdkRootPath = Win32.GetKeyValue(true, "Software\\Microsoft\\Windows Kits\\Installed Roots", "WdkBinRootVersioned", null);

                if (string.IsNullOrWhiteSpace(sdkRootPath))
                    return null;

                makeAppxPath = Win32.ExpandEnvironmentVariables($"{sdkRootPath}\\x64\\MakeAppx.exe");

                if (!File.Exists(makeAppxPath))
                    return null;
            }

            return makeAppxPath;
        }

        public static string ExecuteMsixCommand(string Command)
        {
            string currentMisxPath = GetMakeAppxPath();

            if (string.IsNullOrWhiteSpace(currentMisxPath))
            {
                Program.PrintColorMessage("[ExecuteMsixCommand] File is invalid.", ConsoleColor.Red);
                return null;
            }

            string output = Win32.ShellExecute(currentMisxPath, Command, false);
            output = output.Trim();
            return output;
        }

        public static string GetSignToolPath()
        {
            string windowsSdkPath = Utils.GetWindowsSdkPath();

            if (string.IsNullOrWhiteSpace(windowsSdkPath))
                return string.Empty;

            string signToolPath = $"{windowsSdkPath}\\x64\\SignTool.exe";

            if (string.IsNullOrWhiteSpace(signToolPath))
                return string.Empty;

            if (!File.Exists(signToolPath))
                return string.Empty;

            return signToolPath;
        }

        //public static string GetMsbuildFilePath()
        //{
        //    VisualStudioInstance instance;
        //    string vswhere;
        //    string vswhereResult;
        //
        //    instance = GetVisualStudioInstance();
        //
        //    if (instance != null)
        //    {
        //        foreach (string path in MsBuildPathArray)
        //        {
        //            string file = instance.Path + path;
        //
        //            if (File.Exists(file))
        //            {
        //                return file;
        //            }
        //        }
        //    }
        //
        //    vswhere = GetVswhereFilePath();
        //
        //    if (string.IsNullOrWhiteSpace(vswhere))
        //        return null;
        //
        //    vswhereResult = Win32.ShellExecute(
        //        vswhere,
        //        "-latest " +
        //        "-prerelease " +
        //        "-products * " +
        //        "-requiresAny " +
        //        "-requires Microsoft.Component.MSBuild " +
        //        "-property installationPath "
        //        );
        //
        //    if (string.IsNullOrWhiteSpace(vswhereResult))
        //        return null;
        //
        //    foreach (string path in MsBuildPathArray)
        //    {
        //        string file = vswhereResult + path;
        //
        //        if (File.Exists(file))
        //        {
        //            return file;
        //        }
        //    }
        //
        //    return null;
        //}

        public static string ReadAllText(string FileName)
        {
            FileStreamOptions options = new FileStreamOptions
            {
                Mode = FileMode.Open,
                Access = FileAccess.Read,
                Share = FileShare.Read | FileShare.Delete,
                Options = FileOptions.SequentialScan,
                BufferSize = 0x1000
            };

            using (StreamReader sr = new StreamReader(FileName, Utils.UTF8NoBOM, detectEncodingFromByteOrderMarks: true, options))
            {
                return sr.ReadToEnd();
            }
        }

        public static void WriteAllText(string FileName, string Content)
        {
            FileStreamOptions options = new FileStreamOptions
            {
                Mode = FileMode.Create,
                Access = FileAccess.Write,
                Share = FileShare.None,
                Options = FileOptions.SequentialScan,
                BufferSize = 0x1000
            };

            using (StreamWriter sw = new StreamWriter(FileName, Utils.UTF8NoBOM, options))
            {
                sw.Write(Content);
            }
        }

        public static byte[] ReadAllBytes(string FileName)
        {
            FileStreamOptions options = new FileStreamOptions
            {
                Mode = FileMode.Open,
                Access = FileAccess.Read,
                Share = FileShare.Read | FileShare.Delete,
                Options = FileOptions.SequentialScan,
                BufferSize = 0x1000
            };

            long length = Win32.GetFileSize(FileName);

            byte[] buffer = new byte[length];

            using (MemoryStream stream = new MemoryStream(buffer, true))
            using (FileStream sr = new FileStream(FileName, options))
            {
                sr.CopyTo(stream);
            }

            return buffer;
        }

        public static void WriteAllBytes(string FileName, byte[] Buffer)
        {
            FileStreamOptions options = new FileStreamOptions
            {
                Mode = FileMode.Create,
                Access = FileAccess.Write,
                Share = FileShare.Write | FileShare.Delete,
                Options = FileOptions.SequentialScan,
                PreallocationSize = Buffer.LongLength,
                BufferSize = 0x1000,
            };

            using (MemoryStream stream = new MemoryStream(Buffer, true))
            using (FileStream filestream = new FileStream(FileName, options))
            {
                stream.CopyTo(filestream);
            }
        }
    }

    public class BuildUpdateRequest
    {
        [JsonPropertyName("build_id")] public string BuildId { get; set; }
        [JsonPropertyName("build_version")] public string BuildVersion { get; set; }
        [JsonPropertyName("build_commit")] public string BuildCommit { get; set; }
        [JsonPropertyName("build_updated")] public string BuildUpdated { get; set; }

        [JsonPropertyName("bin_url")] public string BinUrl { get; set; }
        [JsonPropertyName("bin_length")] public string BinLength { get; set; }
        [JsonPropertyName("bin_hash")] public string BinHash { get; set; }
        [JsonPropertyName("bin_sig")] public string BinSig { get; set; }

        [JsonPropertyName("setup_url")] public string SetupUrl { get; set; }
        [JsonPropertyName("setup_length")] public string SetupLength { get; set; }
        [JsonPropertyName("setup_hash")] public string SetupHash { get; set; }
        [JsonPropertyName("setup_sig")] public string SetupSig { get; set; }


        [JsonPropertyName("release_bin_url")] public string ReleaseBinUrl { get; set; }
        [JsonPropertyName("release_bin_length")] public string ReleaseBinLength { get; set; }
        [JsonPropertyName("release_bin_hash")] public string ReleaseBinHash { get; set; }
        [JsonPropertyName("release_bin_sig")] public string ReleaseBinSig { get; set; }

        [JsonPropertyName("release_setup_url")] public string ReleaseSetupUrl { get; set; }
        [JsonPropertyName("release_setup_length")] public string ReleaseSetupLength { get; set; }
        [JsonPropertyName("release_setup_hash")] public string ReleaseSetupHash { get; set; }
        [JsonPropertyName("release_setup_sig")] public string ReleaseSetupSig { get; set; }
    }

    public class GithubReleasesRequest
    {
        [JsonPropertyName("tag_name")] public string ReleaseTag { get; set; }
        [JsonPropertyName("target_commitish")] public string Branch { get; set; }
        [JsonPropertyName("name")] public string Name { get; set; }
        [JsonPropertyName("body")] public string Description { get; set; }

        [JsonPropertyName("draft")] public bool Draft { get; set; }
        [JsonPropertyName("prerelease")] public bool Prerelease { get; set; }
        [JsonPropertyName("generate_release_notes")] public bool GenerateReleaseNotes { get; set; }

        public override string ToString()
        {
            return this.ReleaseTag;
        }
    }

    public class GithubReleasesResponse
    {
        [JsonPropertyName("id")] public long ID { get; set; }
        [JsonPropertyName("upload_url")] public string UploadUrl { get; set; }
        [JsonPropertyName("html_url")] public string HtmlUrl { get; set; }
        [JsonPropertyName("assets")] public List<GithubAssetsResponse> Assets { get; set; }
        [JsonIgnore] public string ReleaseId { get { return this.ID.ToString(); } }

        public override string ToString()
        {
            return this.ReleaseId;
        }
    }

    public class GithubAssetsResponse
    {
        [JsonPropertyName("name")] public string Name { get; set; }
        [JsonPropertyName("label")] public string Label { get; set; }
        [JsonPropertyName("size")] public long Size { get; set; }
        [JsonPropertyName("state")] public string State { get; set; }
        [JsonPropertyName("browser_download_url")] public string DownloadUrl { get; set; }

        [JsonIgnore]
        public bool Uploaded
        {
            get
            {
                return !string.IsNullOrWhiteSpace(this.State) && string.Equals(this.State, "uploaded", StringComparison.OrdinalIgnoreCase);
            }
        }

        public override string ToString()
        {
            return this.Name;
        }
    }

    [JsonSourceGenerationOptions(DefaultIgnoreCondition = JsonIgnoreCondition.WhenWritingNull, GenerationMode = JsonSourceGenerationMode.Serialization)]
    [JsonSerializable(typeof(BuildUpdateRequest))]
    public partial class BuildUpdateRequestContext : JsonSerializerContext
    {

    }

    [JsonSourceGenerationOptions(DefaultIgnoreCondition = JsonIgnoreCondition.WhenWritingNull, GenerationMode = JsonSourceGenerationMode.Serialization)]
    [JsonSerializable(typeof(GithubReleasesRequest))]
    public partial class GithubReleasesRequestContext : JsonSerializerContext
    {

    }

    [JsonSourceGenerationOptions(DefaultIgnoreCondition = JsonIgnoreCondition.WhenWritingNull, GenerationMode = JsonSourceGenerationMode.Metadata)]
    [JsonSerializable(typeof(GithubReleasesResponse))]
    public partial class GithubReleasesResponseContext : JsonSerializerContext
    {

    }

    [JsonSourceGenerationOptions(DefaultIgnoreCondition = JsonIgnoreCondition.WhenWritingNull, GenerationMode = JsonSourceGenerationMode.Metadata)]
    [JsonSerializable(typeof(GithubAssetsResponse))]
    public partial class GithubAssetsResponseContext : JsonSerializerContext
    {

    }

    public static class Extextensions
    {
        private const long OneKb = 1024;
        private const long OneMb = OneKb * 1024;
        private const long OneGb = OneMb * 1024;
        private const long OneTb = OneGb * 1024;

        public static string ToPrettySize(this int value, int decimalPlaces = 0)
        {
            return ((long)value).ToPrettySize(decimalPlaces);
        }

        public static string ToPrettySize(this long value, int decimalPlaces = 0)
        {
            double asTb = Math.Round((double)value / OneTb, decimalPlaces);
            double asGb = Math.Round((double)value / OneGb, decimalPlaces);
            double asMb = Math.Round((double)value / OneMb, decimalPlaces);
            double asKb = Math.Round((double)value / OneKb, decimalPlaces);
            string chosenValue = asTb > 1 ? $"{asTb}Tb"
                : asGb > 1 ? $"{asGb}Gb"
                : asMb > 1 ? $"{asMb}Mb"
                : asKb > 1 ? $"{asKb}Kb"
                : $"{Math.Round((double)value, decimalPlaces)}B";
            return chosenValue;
        }
    }

    [Flags]
    public enum BuildFlags
    {
        None,
        Build32bit = 1,
        Build64bit = 2,
        BuildArm64bit = 4,
        BuildDebug = 8,
        BuildRelease = 16,
        BuildVerbose = 32,
        BuildApi = 64,
        BuildMsix = 128,
    }
}
