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

        public static int ExecuteMsbuildCommand(string Command, BuildFlags Flags, out string OutputString)
        {
            string file = GetMsbuildFilePath(Flags);

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

        public static bool SetCurrentDirectoryParent(string FileName)
        {
            try
            {
                DirectoryInfo info = new DirectoryInfo(".");

                while (info.Parent?.Parent != null)
                {
                    info = info.Parent;

                    if (File.Exists($"{info.FullName}\\{FileName}"))
                    {
                        Directory.SetCurrentDirectory(info.FullName);
                        break;
                    }
                }
            }
            catch (Exception ex)
            {
                Program.PrintColorMessage($"Unable to find directory: {ex}", ConsoleColor.Red);
                return false;
            }

            return File.Exists(FileName);
        }

        public static string GetOutputDirectoryPath(string FileName)
        {
            return Path.Join([Build.BuildWorkingFolder, FileName]);
        }

        public static void CreateOutputDirectory()
        {
            if (string.IsNullOrWhiteSpace(Build.BuildOutputFolder))
                return;
            if (File.Exists(Build.BuildOutputFolder))
                return;

            try
            {
                Directory.CreateDirectory(Build.BuildOutputFolder);
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
                [
                    "%ProgramFiles%\\Microsoft Visual Studio\\Installer\\vswhere.exe",
                    "%ProgramFiles(x86)%\\Microsoft Visual Studio\\Installer\\vswhere.exe",
                    "%ProgramW6432%\\Microsoft Visual Studio\\Installer\\vswhere.exe"
                ];

                foreach (string path in vswherePathArray)
                {
                    string file = Utils.ExpandFullPath(path);

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

        private static string GetMsbuildFilePath(BuildFlags Flags)
        {
            string MsBuildFilePath = null;
            List<string> MsBuildPath = 
            [
                "\\MSBuild\\Current\\Bin\\amd64\\MSBuild.exe",
                "\\MSBuild\\Current\\Bin\\MSBuild.exe",
                "\\MSBuild\\15.0\\Bin\\MSBuild.exe"
            ];

            if (Flags.HasFlag(BuildFlags.BuildArm64bit) && RuntimeInformation.OSArchitecture == Architecture.Arm64)
            {
                MsBuildPath.Insert(0, "\\MSBuild\\Current\\Bin\\amd64\\MSBuild.exe");
            }

            VisualStudioInstance instance = VisualStudio.GetVisualStudioInstance();

            if (instance != null)
            {
                foreach (string path in MsBuildPath)
                {
                    string file = Path.Join([instance.Path, path]);

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
                    "-latest -prerelease -products * -requiresAny -requires Microsoft.Component.MSBuild -property installationPath"
                    );

                if (!string.IsNullOrWhiteSpace(vswhereResult))
                {
                    foreach (string path in MsBuildPath)
                    {
                        string file = Path.Join([vswhereResult, path]);

                        if (File.Exists(file))
                        {
                            MsBuildFilePath = file;
                            break;
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
                [
                    "%ProgramFiles%\\Git\\bin\\git.exe",
                    "%ProgramFiles(x86)%\\Git\\bin\\git.exe",
                    "%ProgramW6432%\\Git\\bin\\git.exe"
                ];

                foreach (string path in GitPathArray)
                {
                    string file = Utils.ExpandFullPath(path);

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
            string kitsPath = Utils.ExpandFullPath(Path.Join([kitsRoot, "\\Include"]));

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
            string kitsPath = Utils.ExpandFullPath(Path.Join([kitsRoot, "\\bin"]));

            if (Directory.Exists(kitsPath))
            {
                var windowsKitsDirectory = Directory.EnumerateDirectories(kitsPath);

                foreach (string path in windowsKitsDirectory)
                {
                    var name = Path.GetFileName(path);

                    if (Version.TryParse(name, out var version))
                    {
                        versionList.Add(new KeyValuePair<Version, string>(version, path));
                    }
                }

                versionList.Sort((first, second) => first.Key.CompareTo(second.Key));

                if (versionList.Count > 0)
                {
                    var result = versionList[^1];

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
            List<KeyValuePair<Version, string>> versionList = new List<KeyValuePair<Version, string>>();
            string kitsRoot = Win32.GetKeyValue(true, "Software\\Microsoft\\Windows Kits\\Installed Roots", "KitsRoot10", "%ProgramFiles(x86)%\\Windows Kits\\10\\");
            string kitsPath = Utils.ExpandFullPath(Path.Join([kitsRoot, "\\bin"]));

            if (Directory.Exists(kitsPath))
            {
                var windowsKitsDirectory = Directory.EnumerateDirectories(kitsPath);

                foreach (string path in windowsKitsDirectory)
                {
                    var name = Path.GetFileName(path.AsSpan());

                    if (Version.TryParse(name, out var version))
                    {
                        versionList.Add(new KeyValuePair<Version, string>(version, path));
                    }
                }

                versionList.Sort((first, second) => first.Key.CompareTo(second.Key));

                if (versionList.Count > 0)
                {
                    var result = versionList[^1];
                    var value = result.Key.ToString();

                    if (!string.IsNullOrWhiteSpace(value))
                    {
                        return value;
                    }
                }
            }

            return null;
        }

        public static string GetVisualStudioVersion(BuildFlags Flags)
        {
            string msbuild = GetMsbuildFilePath(Flags);

            if (string.IsNullOrWhiteSpace(msbuild))
                return string.Empty;

            try
            {
                var directory = Path.GetDirectoryName(msbuild);

                if (string.IsNullOrWhiteSpace(directory))
                    return string.Empty;

                DirectoryInfo info = new DirectoryInfo(directory);

                while (info.Parent != null && info.Parent.Parent != null)
                {
                    info = info.Parent;

                    if (File.Exists(Path.Join([info.FullName, "\\Common7\\IDE\\devenv.exe"])))
                    {
                        FileVersionInfo currentInfo = FileVersionInfo.GetVersionInfo(Path.Join([info.FullName, "\\Common7\\IDE\\devenv.exe"]));
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

            string makeAppxPath = Path.Join([windowsSdkPath, "\\x64\\MakeAppx.exe"]);

            if (string.IsNullOrWhiteSpace(makeAppxPath))
                return null;

            if (!File.Exists(makeAppxPath))
            {
                string sdkRootPath = Win32.GetKeyValue(true, "Software\\Microsoft\\Windows Kits\\Installed Roots", "WdkBinRootVersioned", null);

                if (string.IsNullOrWhiteSpace(sdkRootPath))
                    return null;

                makeAppxPath = Utils.ExpandFullPath(Path.Join([sdkRootPath, "\\x64\\MakeAppx.exe"]));

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

            string signToolPath = Path.Join([windowsSdkPath, "\\x64\\SignTool.exe"]);

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

        public static bool IsSpanNullOrWhiteSpace(ReadOnlySpan<char> value)
        {
            if (value == null)
                return true;
            if (value.IsEmpty)
                return true;
            if (value.IsWhiteSpace())
                return true;

            return false;
        }

        public static string ExpandFullPath(string Name)
        {
            string value = Environment.ExpandEnvironmentVariables(Name);

            value = Path.GetFullPath(value);

            return value;
        }
    }

    public class BuildUpdateRequest
    {
        [JsonPropertyName("build_id")] public string BuildId { get; set; }
        [JsonPropertyName("build_display")] public string BuildDisplay { get; set; }
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

    public class GithubAuthor
    {
        [JsonPropertyName("name")]
        public string name { get; set; }

        [JsonPropertyName("email")]
        public string email { get; set; }

        [JsonPropertyName("date")]
        public DateTime date { get; set; }

        [JsonPropertyName("login")]
        public string login { get; set; }

        [JsonPropertyName("id")]
        public long id { get; set; }

        [JsonPropertyName("node_id")]
        public string node_id { get; set; }

        [JsonPropertyName("avatar_url")]
        public string avatar_url { get; set; }

        [JsonPropertyName("gravatar_id")]
        public string gravatar_id { get; set; }

        [JsonPropertyName("url")]
        public string url { get; set; }

        [JsonPropertyName("html_url")]
        public string html_url { get; set; }

        [JsonPropertyName("followers_url")]
        public string followers_url { get; set; }

        [JsonPropertyName("following_url")]
        public string following_url { get; set; }

        [JsonPropertyName("gists_url")]
        public string gists_url { get; set; }

        [JsonPropertyName("starred_url")]
        public string starred_url { get; set; }

        [JsonPropertyName("subscriptions_url")]
        public string subscriptions_url { get; set; }

        [JsonPropertyName("organizations_url")]
        public string organizations_url { get; set; }

        [JsonPropertyName("repos_url")]
        public string repos_url { get; set; }

        [JsonPropertyName("events_url")]
        public string events_url { get; set; }

        [JsonPropertyName("received_events_url")]
        public string received_events_url { get; set; }

        [JsonPropertyName("type")]
        public string type { get; set; }

        [JsonPropertyName("site_admin")]
        public bool site_admin { get; set; }
    }

    public class GithubCommit
    {
        [JsonPropertyName("author")]
        public GithubAuthor author { get; set; }

        [JsonPropertyName("committer")]
        public GithubCommitAuthor committer { get; set; }

        [JsonPropertyName("message")]
        public string message { get; set; }

        [JsonPropertyName("tree")]
        public GithubCommitTree tree { get; set; }

        [JsonPropertyName("url")]
        public string url { get; set; }

        [JsonPropertyName("comment_count")]
        public int comment_count { get; set; }

        [JsonPropertyName("verification")]
        public GithubCommitVerification verification { get; set; }
    }

    public class GithubCommitAuthor
    {
        [JsonPropertyName("name")]
        public string name { get; set; }

        [JsonPropertyName("email")]
        public string email { get; set; }

        [JsonPropertyName("date")]
        public DateTime date { get; set; }

        [JsonPropertyName("login")]
        public string login { get; set; }

        [JsonPropertyName("id")]
        public int id { get; set; }

        [JsonPropertyName("node_id")]
        public string node_id { get; set; }

        [JsonPropertyName("avatar_url")]
        public string avatar_url { get; set; }

        [JsonPropertyName("gravatar_id")]
        public string gravatar_id { get; set; }

        [JsonPropertyName("url")]
        public string url { get; set; }

        [JsonPropertyName("html_url")]
        public string html_url { get; set; }

        [JsonPropertyName("followers_url")]
        public string followers_url { get; set; }

        [JsonPropertyName("following_url")]
        public string following_url { get; set; }

        [JsonPropertyName("gists_url")]
        public string gists_url { get; set; }

        [JsonPropertyName("starred_url")]
        public string starred_url { get; set; }

        [JsonPropertyName("subscriptions_url")]
        public string subscriptions_url { get; set; }

        [JsonPropertyName("organizations_url")]
        public string organizations_url { get; set; }

        [JsonPropertyName("repos_url")]
        public string repos_url { get; set; }

        [JsonPropertyName("events_url")]
        public string events_url { get; set; }

        [JsonPropertyName("received_events_url")]
        public string received_events_url { get; set; }

        [JsonPropertyName("type")]
        public string type { get; set; }

        [JsonPropertyName("site_admin")]
        public bool site_admin { get; set; }
    }

    //public class GithubFile
    //{
    //    [JsonPropertyName("sha")]
    //    public string sha { get; set; }
    //
    //    [JsonPropertyName("filename")]
    //    public string filename { get; set; }
    //
    //    [JsonPropertyName("status")]
    //    public string status { get; set; }
    //
    //    [JsonPropertyName("additions")]
    //    public int additions { get; set; }
    //
    //    [JsonPropertyName("deletions")]
    //    public int deletions { get; set; }
    //
    //    [JsonPropertyName("changes")]
    //    public int changes { get; set; }
    //
    //    [JsonPropertyName("blob_url")]
    //    public string blob_url { get; set; }
    //
    //    [JsonPropertyName("raw_url")]
    //    public string raw_url { get; set; }
    //
    //    [JsonPropertyName("contents_url")]
    //    public string contents_url { get; set; }
    //
    //    [JsonPropertyName("patch")]
    //    public string patch { get; set; }
    //}
    //
    //public class GithubParent
    //{
    //    [JsonPropertyName("sha")]
    //    public string sha { get; set; }
    //
    //    [JsonPropertyName("url")]
    //    public string url { get; set; }
    //
    //    [JsonPropertyName("html_url")]
    //    public string html_url { get; set; }
    //}

    public class GithubCommitResponse
    {
        [JsonPropertyName("sha")]
        public string sha { get; set; }

        [JsonPropertyName("node_id")]
        public string node_id { get; set; }

        [JsonPropertyName("commit")]
        public GithubCommit commit { get; set; }

        [JsonPropertyName("url")]
        public string url { get; set; }

        [JsonPropertyName("html_url")]
        public string html_url { get; set; }

        [JsonPropertyName("comments_url")]
        public string comments_url { get; set; }

        [JsonPropertyName("author")]
        public GithubAuthor author { get; set; }

        [JsonPropertyName("committer")]
        public GithubCommitAuthor committer { get; set; }

        //[JsonPropertyName("parents")]
        //public List<GithubParent> parents { get; set; }

        [JsonPropertyName("stats")]
        public GithubCommitStats stats { get; set; }

        //[JsonPropertyName("files")]
        //public List<GithubFile> files { get; set; }
    }

    public class GithubCommitStats
    {
        [JsonPropertyName("total")]
        public int total { get; set; }

        [JsonPropertyName("additions")]
        public int additions { get; set; }

        [JsonPropertyName("deletions")]
        public int deletions { get; set; }
    }

    public class GithubCommitTree
    {
        [JsonPropertyName("sha")]
        public string sha { get; set; }

        [JsonPropertyName("url")]
        public string url { get; set; }
    }

    public class GithubCommitVerification
    {
        [JsonPropertyName("verified")]
        public bool verified { get; set; }

        [JsonPropertyName("reason")]
        public string reason { get; set; }

        [JsonPropertyName("signature")]
        public string signature { get; set; }

        [JsonPropertyName("payload")]
        public string payload { get; set; }
    }

    [JsonSerializable(typeof(BuildUpdateRequest))]
    [JsonSourceGenerationOptions(DefaultIgnoreCondition = JsonIgnoreCondition.WhenWritingNull, GenerationMode = JsonSourceGenerationMode.Serialization)]
    public partial class BuildUpdateRequestContext : JsonSerializerContext
    {

    }

    [JsonSerializable(typeof(GithubReleasesRequest))]
    [JsonSourceGenerationOptions(DefaultIgnoreCondition = JsonIgnoreCondition.WhenWritingNull, GenerationMode = JsonSourceGenerationMode.Serialization)]
    public partial class GithubReleasesRequestContext : JsonSerializerContext
    {

    }

    [JsonSerializable(typeof(GithubReleasesResponse))]
    [JsonSourceGenerationOptions(DefaultIgnoreCondition = JsonIgnoreCondition.WhenWritingNull, GenerationMode = JsonSourceGenerationMode.Serialization)]
    public partial class GithubReleasesResponseContext : JsonSerializerContext
    {

    }

    [JsonSerializable(typeof(GithubAssetsResponse))]
    [JsonSourceGenerationOptions(DefaultIgnoreCondition = JsonIgnoreCondition.WhenWritingNull, GenerationMode = JsonSourceGenerationMode.Serialization)]
    public partial class GithubAssetsResponseContext : JsonSerializerContext
    {

    }

    [JsonSerializable(typeof(GithubCommitResponse))]
    [JsonSourceGenerationOptions(DefaultIgnoreCondition = JsonIgnoreCondition.WhenWritingNull, GenerationMode = JsonSourceGenerationMode.Serialization)]
    public partial class GithubCommitResponseContext : JsonSerializerContext
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

        Debug = Build32bit | Build64bit | BuildArm64bit | BuildDebug | BuildApi | BuildVerbose,
        Release = Build32bit | Build64bit | BuildArm64bit | BuildRelease | BuildApi | BuildVerbose,
        All = Build32bit | Build64bit | BuildArm64bit | BuildDebug | BuildRelease | BuildApi | BuildVerbose,
    }

    /// <summary>
    /// https://learn.microsoft.com/en-us/dotnet/csharp/whats-new/tutorials/interpolated-string-handler
    /// </summary>
    [InterpolatedStringHandler]
    public readonly ref struct LogInterpolatedStringHandler
    {
        public readonly StringBuilder builder;

        public LogInterpolatedStringHandler(int literalLength, int formattedCount)
        {
            builder = new StringBuilder(literalLength);
        }

        public void AppendLiteral(string s)
        {
            builder.Append(s.AsSpan());
        }

        public void AppendFormatted<T>(T t)
        {
            builder.Append(t?.ToString());
        }

        public void AppendFormatted<T>(T t, string format) where T : IFormattable
        {
            builder.Append(t?.ToString(format, null));
        }

        internal string GetFormattedText()
        {
            return builder.ToString();
        }
    }
}
