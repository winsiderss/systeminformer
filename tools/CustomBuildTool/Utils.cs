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
    public static unsafe class Utils
    {
        private static readonly Dictionary<string, string> EnvironmentBlock = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase);
        public static readonly Encoding UTF8NoBOM = new UTF8Encoding(encoderShouldEmitUTF8Identifier: false, throwOnInvalidBytes: true);
        private static string GitFilePath;
        private static string VsWhereFilePath;

        /// <summary>
        /// Splits a string into key-value pairs.
        /// </summary>
        public static Dictionary<string, string> ParseArgs(string[] args)
        {
            var dict = new Dictionary<string, string>(args.Length, StringComparer.OrdinalIgnoreCase);
            string argPending = null;

            foreach (string s in args)
            {
                if (s.StartsWith('-'))
                {
                    dict[s] = string.Empty;
                    argPending = s;
                }
                else if (!string.IsNullOrWhiteSpace(argPending))
                {
                    dict[argPending] = s;
                    argPending = null;
                }
                else
                {
                    dict[string.Empty] = s;
                }
            }

            return dict;
        }

        /// <summary>
        /// Splits a string into key-value pairs.
        /// </summary>
        public static Dictionary<string, string> ParseArguments(string[] args)
        {
            // 2. Track the current key (string) if it starts with "-".
            // 3. For each string in args:
            //    a. If it starts with "-", set as current key, add to dict if not present.
            //    b. Else, if current key is set, append value to its list.
            //    c. If no current key, ignore or add to a special key (optional).
            // 4. Return the dictionary.

            var kvp = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase);
            var key = string.Empty;

            foreach (string s in args)
            {
                if (s.StartsWith('-'))
                {
                    key = s;

                    if (!kvp.ContainsKey(key))
                    {
                        kvp[key] = string.Empty;
                    }
                }
                else if (!string.IsNullOrWhiteSpace(key))
                {
                    kvp[key] += s;
                }
                // else: ignore values before any key
            }

            return kvp;
        }

        /// <summary>
        /// Parses arguments from a file. Each line should contain a key-value pair in the format: key=value
        /// Lines starting with # are treated as comments and ignored.
        /// </summary>
        public static void ParseArgumentsFromFile(Dictionary<string, string> Kvp, string FileName)
        {
            if (!File.Exists(FileName))
            {
                Program.PrintColorMessage($"Arguments file not found: {FileName}", ConsoleColor.Red);
                return;
            }

            var args = File.ReadAllLines(FileName);
            var key = string.Empty;

            foreach (string s in args)
            {
                if (s.StartsWith('-'))
                {
                    key = s;

                    if (!Kvp.ContainsKey(key))
                    {
                        Kvp[key] = string.Empty;
                    }
                }
                else if (!string.IsNullOrWhiteSpace(key))
                {
                    Kvp[key] += s;
                }
                // else: ignore values before any key
            }
        }

        public static Dictionary<string, string> ParseArgumentsFromFile(string filePath)
        {
            var kvp = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase);

            try
            {
                string jsonContent = File.ReadAllText(filePath);
                var args = JsonSerializer.Deserialize(jsonContent, DictionarySerializerContext.Default.DictionaryStringString);

                foreach (var s in args)
                {
                    string key = s.Key;

                    if (!key.StartsWith('-'))
                        key = "-" + key;

                    kvp[key] = s.Value;
                }
            }
            catch (JsonException ex)
            {
                Program.PrintColorMessage($"Failed to parse JSON arguments file: {ex.Message}", ConsoleColor.Red);
            }

            return kvp;
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

        public static int ExecuteMsbuildCommand(string Command, BuildFlags Flags, out string OutputString, bool RedirectOutput = true)
        {
            string file = null;

            if (string.IsNullOrWhiteSpace(file))
            {
                file = GetMsbuildFilePath(Flags, RuntimeInformation.ProcessArchitecture);
            }

            if (string.IsNullOrWhiteSpace(file))
            {
                OutputString = "[ExecuteMsbuildCommand] MsbuildFilePath is invalid.";
                return 3; // file not found.
            }

            return Win32.CreateProcess(file, Command, out OutputString, false, RedirectOutput);
        }

        public static string ExecuteVsWhereCommand(string Command)
        {
            string file = GetVswhereFilePath();

            if (string.IsNullOrWhiteSpace(file))
            {
                Program.PrintColorMessage("[ExecuteVsWhereCommand] VswhereFilePath is invalid.", ConsoleColor.Red);
                return null;
            }

            Win32.CreateProcess(file, Command, out var outputString, false);

            return outputString.Trim();
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

        private static string GetMsbuildFilePath(BuildFlags Flags, Architecture Architecture)
        {
            var MsBuildPath = new Dictionary<string, Architecture>(3, StringComparer.OrdinalIgnoreCase)
            {
                ["\\MSBuild\\Current\\Bin\\arm64\\MSBuild.exe"] = Architecture.Arm64,
                ["\\MSBuild\\Current\\Bin\\amd64\\MSBuild.exe"] = Architecture.X64,
                ["\\MSBuild\\Current\\Bin\\MSBuild.exe"] = Architecture.X86
            };

            {
                VisualStudioInstance instance = BuildVisualStudio.GetVisualStudioInstance();

                if (instance != null)
                {
                    foreach (var path in MsBuildPath)
                    {
                        if (path.Value == Architecture)
                        {
                            string file = Path.Join([instance.Path, path.Key]);

                            if (File.Exists(file))
                            {
                                return file;
                            }
                        }
                    }
                }
            }

            {
                string vswhereResult = ExecuteVsWhereCommand(
                    "-latest -prerelease -products * -requiresAny -requires Microsoft.Component.MSBuild -property installationPath"
                    );

                if (!string.IsNullOrWhiteSpace(vswhereResult))
                {
                    foreach (var path in MsBuildPath)
                    {
                        if (path.Value == Architecture)
                        {
                            string file = Path.Join([vswhereResult, path.Key]);

                            if (File.Exists(file))
                            {
                                return file;
                            }
                        }
                    }
                }
            }

            {
                string vswhereResult = ExecuteVsWhereCommand(
                    "-latest -prerelease -products * -requiresAny -requires Microsoft.Component.MSBuild -find \"MSBuild\\**\\Bin\\MSBuild.exe"
                    );

                if (!string.IsNullOrWhiteSpace(vswhereResult))
                {
                    foreach (var path in MsBuildPath)
                    {
                        if (path.Value == Architecture)
                        {
                            string file = Path.Join([vswhereResult, path.Key]);

                            if (File.Exists(file))
                            {
                                return file;
                            }
                        }
                    }
                }
            }

            return null;
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

            Win32.CreateProcess(currentGitPath, $"{currentGitDirectory} {Command}", out var outputString, false);

            return outputString.Trim();
        }

        public static List<string> EnumerateDirectory(string FilePath, string[] Extensions, string[] Exclude = null)
        {
            var list = Directory.EnumerateFiles(FilePath, "*", new EnumerationOptions
            {
                RecurseSubdirectories = true,
                ReturnSpecialDirectories = false
            }).Where(s => Extensions.Any(ext => string.Equals(ext, Path.GetExtension(s), StringComparison.OrdinalIgnoreCase))).ToList();

            if (Exclude != null)
            {
                list.RemoveAll(s => Exclude.Any(f => f.Equals(Path.GetFileName(s), StringComparison.OrdinalIgnoreCase)));
            }

            return list;
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

        public static string GetVisualStudioVersion(BuildFlags Flags, Architecture Architecture)
        {
            string msbuild = GetMsbuildFilePath(Flags, Architecture);

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

        public static int ExecuteCMakeCommand(string Command)
        {
            return int.MaxValue;
        }

        public static string ExecuteMsixCommand(string Command)
        {
            string file = GetMakeAppxPath();

            if (string.IsNullOrWhiteSpace(file))
            {
                Program.PrintColorMessage("[ExecuteMsixCommand] File is invalid.", ConsoleColor.Red);
                return null;
            }

            Win32.CreateProcess(file, Command, out var outputString, false);

            return outputString.Replace("\r\n\r\n", "\r\n", StringComparison.OrdinalIgnoreCase).Trim();
        }

        //public static string GetSignToolPath()
        //{
        //    string windowsSdkPath = Utils.GetWindowsSdkPath();
        //
        //    if (string.IsNullOrWhiteSpace(windowsSdkPath))
        //        return string.Empty;
        //
        //    string signToolPath = Path.Join([windowsSdkPath, "\\x64\\SignTool.exe"]);
        //
        //    if (string.IsNullOrWhiteSpace(signToolPath))
        //        return string.Empty;
        //
        //    if (!File.Exists(signToolPath))
        //        return string.Empty;
        //
        //    return signToolPath;
        //}

        public static string GetSymStorePath()
        {
            string kitsRoot = Win32.GetKeyValue(true, "Software\\Microsoft\\Windows Kits\\Installed Roots", "KitsRoot10", "%ProgramFiles(x86)%\\Windows Kits\\10\\");
            if (string.IsNullOrWhiteSpace(kitsRoot))
                return null;

            string kitsPath = Utils.ExpandFullPath(kitsRoot);
            if (string.IsNullOrWhiteSpace(kitsPath))
                return null;

            string symStorePath = Path.Join([kitsPath, "Debuggers\\x64\\symstore.exe"]);
            if (string.IsNullOrWhiteSpace(symStorePath))
                return null;

            return symStorePath;
        }

        public static bool SymStoreExists()
        {
            string file = GetSymStorePath();

            if (string.IsNullOrWhiteSpace(file))
            {
                Program.PrintColorMessage("[ExecuteSymStoreCommand] symstore.exe is invalid.", ConsoleColor.Red);
                return false;
            }

            if (!File.Exists(file))
            {
                Program.PrintColorMessage("[ExecuteSymStoreCommand] symstore.exe does not exist.", ConsoleColor.Red);
                return false;
            }

            return true;
        }

        public static int ExecuteSymStoreCommand(string Command)
        {
            string file = GetSymStorePath();

            if (string.IsNullOrWhiteSpace(file))
            {
                Program.PrintColorMessage("[ExecuteSymStoreCommand] symstore.exe is invalid.", ConsoleColor.Red);
                return int.MaxValue;
            }

            if (!File.Exists(file))
            {
                Program.PrintColorMessage("[ExecuteSymStoreCommand] symstore.exe does not exist.", ConsoleColor.Red);
                return int.MaxValue;
            }

            return Win32.CreateProcess(file, Command, out _, false, false);
        }

        public static string GetDevEnvPath()
        {
            var instance = BuildVisualStudio.GetVisualStudioInstance();
            {
                string file = Path.Join([instance.Path, "Common7\\IDE\\devenv.com"]);

                if (File.Exists(file))
                {
                    return file;
                }
            }

            string vswhere = GetVswhereFilePath();

            if (string.IsNullOrWhiteSpace(vswhere))
                return null;

            int errorcode = Win32.CreateProcess(
                vswhere,
                "-latest " +
                "-prerelease " +
                "-products * " +
                "-requiresAny " +
                "-requires Microsoft.Component.MSBuild " +
                "-property installationPath ",
                out string vswhereResult
                );

            if (errorcode != 0 || string.IsNullOrWhiteSpace(vswhereResult))
                return null;

            {
                string file = Path.Join([vswhereResult, "Common7\\IDE\\devenv.com"]);

                if (File.Exists(file))
                {
                    return file;
                }
            }

            return null;
        }

        public static int ExecuteDevEnvCommand(string Command)
        {
            string currentDevEnvPath = GetDevEnvPath();

            if (string.IsNullOrWhiteSpace(currentDevEnvPath))
            {
                Program.PrintColorMessage("[ExecuteDevEnvCommand] File is invalid.", ConsoleColor.Red);
                return int.MaxValue;
            }

            return Win32.CreateProcess(currentDevEnvPath, Command, out _, false, false);
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
        //    vswhereResult = Win32.CreateProcess(
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

            ulong length = Win32.GetFileSize(FileName);

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

        public static Dictionary<string, string> GetSystemEnvironmentBlock()
        {
            if (EnvironmentBlock.Count == 0)
            {
                if (PInvoke.CreateEnvironmentBlock(out var block, null, false))
                {
                    char* offset = (char*)block;

                    while (offset != null)
                    {
                        string variable = new string(offset);

                        if (string.IsNullOrWhiteSpace(variable))
                            break;

                        string[] parts = variable.Split('=', StringSplitOptions.RemoveEmptyEntries | StringSplitOptions.TrimEntries);
                        EnvironmentBlock.Add(parts[0], parts.Length <= 1 ? string.Empty : parts[1]);

                        //offset = new IntPtr(offset.ToInt64() + (variable.Length + 1) * sizeof(char));
                        offset += variable.Length + 1;
                    }

                    PInvoke.DestroyEnvironmentBlock(block);
                }
            }

            return EnvironmentBlock;
        }

        public static string GetBuildLogPath(string Solution, string Platform, BuildFlags Flags)
        {
            return $"{Path.GetFileNameWithoutExtension(Solution)}{(Flags.HasFlag(BuildFlags.BuildDebug) ? "Debug" : "Release")}{Platform}";
        }

        public static DateTime FileTimeToDateTime(this System.Runtime.InteropServices.ComTypes.FILETIME FileTime)
        {
            long fileTime = ((long)FileTime.dwHighDateTime << 32) + FileTime.dwLowDateTime;

            return DateTime.FromFileTime(fileTime);
        }

        public static long FileTimeFromFileTime(this System.Runtime.InteropServices.ComTypes.FILETIME FileTime)
        {
            long fileTime = ((long)FileTime.dwHighDateTime << 32) + FileTime.dwLowDateTime;

            return fileTime;
        }

        public static bool ValidateImageExports(string FileName)
        {
            LOADED_IMAGE loadedMappedImage = default;
            IMAGE_EXPORT_DIRECTORY* exportDirectory;

            try
            {
                if (!PInvoke.MapAndLoad(FileName, null, out loadedMappedImage, false, true))
                    return false;

                try
                {
                    exportDirectory = (IMAGE_EXPORT_DIRECTORY*)PInvoke.ImageDirectoryEntryToData(
                        loadedMappedImage.MappedAddress, false,
                        IMAGE_DIRECTORY_ENTRY.IMAGE_DIRECTORY_ENTRY_EXPORT, out uint DirectorySize
                        );

                    if (exportDirectory != null)
                    {
                        if (exportDirectory->NumberOfNames == 0)
                            return true;

                        Program.PrintColorMessage("Exported functions missing from module export definition file: ", ConsoleColor.Yellow);

                        uint* exportNameTable = (uint*)PInvoke.ImageRvaToVa(loadedMappedImage.FileHeader, loadedMappedImage.MappedAddress, exportDirectory->AddressOfNames, null);

                        for (uint i = 0; i < exportDirectory->NumberOfNames; i++)
                        {
                            uint nameRva = exportNameTable[i];
                            IntPtr namePtr = (IntPtr)PInvoke.ImageRvaToVa(loadedMappedImage.FileHeader, loadedMappedImage.MappedAddress, nameRva, null);
                            var exportName = Marshal.PtrToStringUTF8(namePtr);

                            Program.PrintColorMessage($"{i}: {exportName}", ConsoleColor.Yellow);
                        }
                    }
                    else
                    {
                        Program.PrintColorMessage("IMAGE_DIRECTORY_ENTRY_EXPORT", ConsoleColor.Red);
                    }
                }
                catch (Exception ex)
                {
                    Program.PrintColorMessage($"ValidateImageExports: {ex}", ConsoleColor.Red);
                }
            }
            finally
            {
                PInvoke.UnMapAndLoad(ref loadedMappedImage);
            }

            return false;
        }
    }

    public class BuildUpdateRequest
    {
        [JsonPropertyName("build_id")] public string BuildId { get; init; }
        [JsonPropertyName("build_display")] public string BuildDisplay { get; init; }
        [JsonPropertyName("build_version")] public string BuildVersion { get; init; }
        [JsonPropertyName("build_commit")] public string BuildCommit { get; init; }
        [JsonPropertyName("build_updated")] public string BuildUpdated { get; init; }
        [JsonPropertyName("build_github_id")] public string BuildGithubId { get; init; }

        [JsonPropertyName("bin_url")] public string BinUrl { get; init; }
        [JsonPropertyName("bin_length")] public string BinLength { get; init; }
        [JsonPropertyName("bin_hash")] public string BinHash { get; init; }
        [JsonPropertyName("bin_sig")] public string BinSig { get; init; }

        [JsonPropertyName("canary_setup_url")] public string CanarySetupUrl { get; init; }
        [JsonPropertyName("canary_setup_length")] public string CanarySetupLength { get; init; }
        [JsonPropertyName("canary_setup_hash")] public string CanarySetupHash { get; init; }
        [JsonPropertyName("canary_setup_sig")] public string CanarySetupSig { get; init; }

        [JsonPropertyName("release_setup_url")] public string ReleaseSetupUrl { get; init; }
        [JsonPropertyName("release_setup_length")] public string ReleaseSetupLength { get; init; }
        [JsonPropertyName("release_setup_hash")] public string ReleaseSetupHash { get; init; }
        [JsonPropertyName("release_setup_sig")] public string ReleaseSetupSig { get; init; }

        public override string ToString()
        {
            return this.BuildId;
        }

        public string SerializeToJson()
        {
            return JsonSerializer.Serialize(this, BuildUpdateRequestContext.Default.BuildUpdateRequest);
        }

        public byte[] SerializeToBytes()
        {
            return JsonSerializer.SerializeToUtf8Bytes(this, BuildUpdateRequestContext.Default.BuildUpdateRequest);
        }
    }

    public class GithubReleasesRequest
    {
        [JsonPropertyName("tag_name")]
        public string ReleaseTag { get; init; }

        [JsonPropertyName("target_commitish")]
        public string Branch { get; init; }

        [JsonPropertyName("name")]
        public string Name { get; init; }

        [JsonPropertyName("body")]
        public string Description { get; init; }

        [JsonPropertyName("draft")]
        public bool Draft { get; init; }

        [JsonPropertyName("prerelease")]
        public bool Prerelease { get; init; }

        [JsonPropertyName("generate_release_notes")]
        public bool GenerateReleaseNotes { get; init; }

        public override string ToString()
        {
            return this.ReleaseTag;
        }

        public string SerializeToJson()
        {
            return JsonSerializer.Serialize(this, GithubResponseContext.Default.GithubReleasesRequest);
        }

        public byte[] SerializeToBytes()
        {
            return JsonSerializer.SerializeToUtf8Bytes(this, GithubResponseContext.Default.GithubReleasesRequest);
        }
    }

    public class GithubReleasesResponse
    {
        [JsonPropertyName("id")]
        public ulong ReleaseId { get; init; }

        [JsonPropertyName("upload_url")]
        public string UploadUrl { get; init; }

        [JsonPropertyName("html_url")]
        public string HtmlUrl { get; init; }

        [JsonPropertyName("assets")]
        public List<GithubAssetsResponse> Assets { get; init; }

        public override string ToString()
        {
            return this.ReleaseId.ToString();
        }

        public string SerializeToJson()
        {
            return JsonSerializer.Serialize(this, GithubResponseContext.Default.GithubReleasesRequest);
        }

        public byte[] SerializeToBytes()
        {
            return JsonSerializer.SerializeToUtf8Bytes(this, GithubResponseContext.Default.GithubReleasesRequest);
        }
    }

    public class GithubReleaseQueryResponse
    {
        [JsonPropertyName("id")]
        public ulong ReleaseId { get; init; }

        [JsonPropertyName("name")]
        public string Name { get; init; }
        [JsonPropertyName("tag_name")]
        public string TagName { get; init; }

        [JsonPropertyName("upload_url")]
        public string UploadUrl { get; init; }
        [JsonPropertyName("html_url")]
        public string HtmlUrl { get; init; }

        [JsonPropertyName("created_at")]
        public DateTimeOffset Created { get; init; }
        [JsonPropertyName("updated_at")]
        public DateTimeOffset Updated { get; init; }
        [JsonPropertyName("published_at")]
        public DateTimeOffset Published { get; init; }

        [JsonPropertyName("draft")]
        public bool Draft { get; init; }
        [JsonPropertyName("prerelease")]
        public bool Prerelease { get; init; }

        [JsonPropertyName("assets")]
        public List<GithubAssetsResponse> Assets { get; init; }

        public override string ToString()
        {
            return this.ReleaseId.ToString();
        }

        public string SerializeToJson()
        {
            return JsonSerializer.Serialize(this, GithubResponseContext.Default.GithubReleasesRequest);
        }

        public byte[] SerializeToBytes()
        {
            return JsonSerializer.SerializeToUtf8Bytes(this, GithubResponseContext.Default.GithubReleasesRequest);
        }
    }

    public class GithubAssetsResponse
    {
        [JsonPropertyName("id")]
        public ulong Id { get; init; }

        [JsonPropertyName("name")]
        public string Name { get; init; }

        [JsonPropertyName("label")]
        public string Label { get; init; }

        [JsonPropertyName("size")]
        public ulong Size { get; init; }

        [JsonPropertyName("state")]
        public string State { get; init; }

        [JsonPropertyName("browser_download_url")]
        public string DownloadUrl { get; init; }

        [JsonPropertyName("digest")]
        public string Digest { get; init; }

        [JsonIgnore]
        public bool Uploaded => string.Equals(this.State, "uploaded", StringComparison.OrdinalIgnoreCase);

        [JsonIgnore]
        public string HashAlgorithm
        {
            get
            {
                if (string.IsNullOrWhiteSpace(this.Digest))
                    return string.Empty;

                if (this.Digest.AsSpan().IndexOf(':') is int idx && idx >= 0)
                {
                    return this.Digest[..idx];
                }

                return string.Empty;
            }
        }

        [JsonIgnore]
        public string HashValue
        {
            get
            {
                if (string.IsNullOrWhiteSpace(this.Digest))
                    return string.Empty;

                if (this.Digest.AsSpan().IndexOf(':') is int idx && idx >= 0)
                {
                    return this.Digest[idx..];
                }

                return string.Empty;
            }
        }

        public override string ToString()
        {
            return this.Name;
        }
    }

    public class GithubUser
    {
        [JsonPropertyName("name")]
        public string Name { get; init; }

        [JsonPropertyName("email")]
        public string Email { get; init; }

        [JsonPropertyName("date")]
        public DateTime Date { get; init; }

        [JsonPropertyName("login")]
        public string Login { get; init; }

        [JsonPropertyName("id")]
        public ulong Id { get; init; }

        [JsonPropertyName("node_id")]
        public string NodeId { get; init; }

        [JsonPropertyName("avatar_url")]
        public string AvatarUrl { get; init; }

        [JsonPropertyName("gravatar_id")]
        public string GravatarId { get; init; }

        [JsonPropertyName("url")]
        public string Url { get; init; }

        [JsonPropertyName("html_url")]
        public string HtmlUrl { get; init; }

        [JsonPropertyName("followers_url")]
        public string FollowersUrl { get; init; }

        [JsonPropertyName("following_url")]
        public string FollowingUrl { get; init; }

        [JsonPropertyName("gists_url")]
        public string GistsUrl { get; init; }

        [JsonPropertyName("starred_url")]
        public string StarredUrl { get; init; }

        [JsonPropertyName("subscriptions_url")]
        public string SubscriptionsUrl { get; init; }

        [JsonPropertyName("organizations_url")]
        public string OrganizationsUrl { get; init; }

        [JsonPropertyName("repos_url")]
        public string ReposUrl { get; init; }

        [JsonPropertyName("events_url")]
        public string EventsUrl { get; init; }

        [JsonPropertyName("received_events_url")]
        public string ReceivedEventsUrl { get; init; }

        [JsonPropertyName("type")]
        public string Type { get; init; }
    }

    public class GithubCommit
    {
        [JsonPropertyName("author")]
        public GithubUser Author { get; init; }

        [JsonPropertyName("committer")]
        public GithubUser Committer { get; init; }

        [JsonPropertyName("message")]
        public string Message { get; init; }

        [JsonPropertyName("tree")]
        public GithubCommitTree Tree { get; init; }

        [JsonPropertyName("url")]
        public string Url { get; init; }

        [JsonPropertyName("comment_count")]
        public ulong CommentCount { get; init; }

        [JsonPropertyName("verification")]
        public GithubCommitVerification Verification { get; init; }
    }

    //public class GithubFile
    //{
    //    [JsonPropertyName("sha")]
    //    public string sha { get; init; }
    //
    //    [JsonPropertyName("filename")]
    //    public string filename { get; init; }
    //
    //    [JsonPropertyName("status")]
    //    public string status { get; init; }
    //
    //    [JsonPropertyName("additions")]
    //    public int additions { get; init; }
    //
    //    [JsonPropertyName("deletions")]
    //    public int deletions { get; init; }
    //
    //    [JsonPropertyName("changes")]
    //    public int changes { get; init; }
    //
    //    [JsonPropertyName("blob_url")]
    //    public string blob_url { get; init; }
    //
    //    [JsonPropertyName("raw_url")]
    //    public string raw_url { get; init; }
    //
    //    [JsonPropertyName("contents_url")]
    //    public string contents_url { get; init; }
    //
    //    [JsonPropertyName("patch")]
    //    public string patch { get; init; }
    //}
    //
    //public class GithubParent
    //{
    //    [JsonPropertyName("sha")]
    //    public string sha { get; init; }
    //
    //    [JsonPropertyName("url")]
    //    public string url { get; init; }
    //
    //    [JsonPropertyName("html_url")]
    //    public string html_url { get; init; }
    //}

    public class GithubCommitResponse
    {
        [JsonPropertyName("sha")]
        public string Sha { get; init; }

        [JsonPropertyName("node_id")]
        public string NodeId { get; init; }

        [JsonPropertyName("commit")]
        public GithubCommit Commit { get; init; }

        [JsonPropertyName("url")]
        public string Url { get; init; }

        [JsonPropertyName("html_url")]
        public string HtmlUrl { get; init; }

        [JsonPropertyName("comments_url")]
        public string CommentsUrl { get; init; }

        [JsonPropertyName("author")]
        public GithubUser Author { get; init; }

        [JsonPropertyName("committer")]
        public GithubUser Committer { get; init; }

        //[JsonPropertyName("parents")]
        //public List<GithubParent> parents { get; init; }

        [JsonPropertyName("stats")]
        public GithubCommitStats Stats { get; init; }

        //[JsonPropertyName("files")]
        //public List<GithubFile> files { get; init; }
    }

    public class GithubCommitStats
    {
        [JsonPropertyName("total")]
        public ulong Total { get; init; }

        [JsonPropertyName("additions")]
        public ulong Additions { get; init; }

        [JsonPropertyName("deletions")]
        public ulong Deletions { get; init; }
    }

    public class GithubCommitTree
    {
        [JsonPropertyName("sha")]
        public string Sha { get; init; }

        [JsonPropertyName("url")]
        public string Url { get; init; }
    }

    public class GithubCommitVerification
    {
        [JsonPropertyName("verified")]
        public bool Verified { get; init; }

        [JsonPropertyName("reason")]
        public string Reason { get; init; }

        [JsonPropertyName("signature")]
        public string Signature { get; init; }

        [JsonPropertyName("payload")]
        public string Payload { get; init; }
    }

    public class GithubHeadCommit
    {
        [JsonPropertyName("id")]
        public string Id { get; set; }

        [JsonPropertyName("tree_id")]
        public string TreeId { get; set; }

        [JsonPropertyName("message")]
        public string Message { get; set; }

        [JsonPropertyName("timestamp")]
        public DateTime Timestamp { get; set; }

        [JsonPropertyName("author")]
        public GithubUser Author { get; set; }

        [JsonPropertyName("committer")]
        public GithubUser Committer { get; set; }
    }

    public class GithubHeadRepository
    {
        [JsonPropertyName("id")]
        public string Id { get; set; }

        [JsonPropertyName("node_id")]
        public string NodeId { get; set; }

        [JsonPropertyName("name")]
        public string Name { get; set; }

        [JsonPropertyName("full_name")]
        public string FullName { get; set; }

        [JsonPropertyName("private")]
        public bool PrivateRepo { get; set; }

        [JsonPropertyName("owner")]
        public GithubUser Owner { get; set; }

        [JsonPropertyName("html_url")]
        public string HtmlUrl { get; set; }

        //[JsonPropertyName("description")]
        //public object description { get; set; }

        [JsonPropertyName("fork")]
        public bool Fork { get; set; }

        [JsonPropertyName("url")]
        public string Url { get; set; }

        [JsonPropertyName("forks_url")]
        public string ForksUrl { get; set; }

        [JsonPropertyName("keys_url")]
        public string KeysUrl { get; set; }

        [JsonPropertyName("collaborators_url")]
        public string CollaboratorsUrl { get; set; }

        [JsonPropertyName("teams_url")]
        public string TeamsUrl { get; set; }

        [JsonPropertyName("hooks_url")]
        public string HooksUrl { get; set; }

        [JsonPropertyName("issue_events_url")]
        public string IssueEventsUrl { get; set; }

        [JsonPropertyName("events_url")]
        public string EventsUrl { get; set; }

        [JsonPropertyName("assignees_url")]
        public string AssigneesUrl { get; set; }

        [JsonPropertyName("branches_url")]
        public string BranchesUrl { get; set; }

        [JsonPropertyName("tags_url")]
        public string TagsUrl { get; set; }

        [JsonPropertyName("blobs_url")]
        public string BlobsUrl { get; set; }

        [JsonPropertyName("git_tags_url")]
        public string GitTagsUrl { get; set; }

        [JsonPropertyName("git_refs_url")]
        public string GitRefsUrl { get; set; }

        [JsonPropertyName("trees_url")]
        public string TreesUrl { get; set; }

        [JsonPropertyName("statuses_url")]
        public string StatusesUrl { get; set; }

        //[JsonPropertyName("languages_url")]
        //public string languages_url { get; set; }

        //[JsonPropertyName("stargazers_url")]
        //public string stargazers_url { get; set; }

        //[JsonPropertyName("contributors_url")]
        //public string contributors_url { get; set; }

        //[JsonPropertyName("subscribers_url")]
        //public string subscribers_url { get; set; }

        //[JsonPropertyName("subscription_url")]
        //public string subscription_url { get; set; }

        [JsonPropertyName("commits_url")]
        public string CommitsUrl { get; set; }

        [JsonPropertyName("git_commits_url")]
        public string GitCommitsUrl { get; set; }

        [JsonPropertyName("comments_url")]
        public string CommentsUrl { get; set; }

        [JsonPropertyName("issue_comment_url")]
        public string IssueCommentUrl { get; set; }

        [JsonPropertyName("contents_url")]
        public string ContentsUrl { get; set; }

        [JsonPropertyName("compare_url")]
        public string CompareUrl { get; set; }

        [JsonPropertyName("merges_url")]
        public string MergesUrl { get; set; }

        [JsonPropertyName("archive_url")]
        public string ArchiveUrl { get; set; }

        [JsonPropertyName("downloads_url")]
        public string DownloadsUrl { get; set; }

        [JsonPropertyName("issues_url")]
        public string IssuesUrl { get; set; }

        [JsonPropertyName("pulls_url")]
        public string PullsUrl { get; set; }

        [JsonPropertyName("milestones_url")]
        public string MilestonesUrl { get; set; }

        [JsonPropertyName("notifications_url")]
        public string NotificationsUrl { get; set; }

        [JsonPropertyName("labels_url")]
        public string LabelsUrl { get; set; }

        [JsonPropertyName("releases_url")]
        public string ReleasesUrl { get; set; }

        [JsonPropertyName("deployments_url")]
        public string DeploymentsUrl { get; set; }
    }

    public class GithubReferencedWorkflow
    {
        [JsonPropertyName("path")]
        public string Path { get; set; }

        [JsonPropertyName("sha")]
        public string Sha { get; set; }

        //[JsonPropertyName("ref")]
        //public string @ref { get; set; }
    }

    public class GithubRepository
    {
        [JsonPropertyName("id")]
        public string Id { get; set; }

        [JsonPropertyName("node_id")]
        public string NodeId { get; set; }

        [JsonPropertyName("name")]
        public string Name { get; set; }

        [JsonPropertyName("full_name")]
        public string FullName { get; set; }

        [JsonPropertyName("owner")]
        public GithubUser Owner { get; set; }

        [JsonPropertyName("private")]
        public bool PrivateRepo { get; set; }

        [JsonPropertyName("html_url")]
        public string HtmlUrl { get; set; }

        [JsonPropertyName("description")]
        public string Description { get; set; }

        [JsonPropertyName("fork")]
        public bool Fork { get; set; }

        [JsonPropertyName("url")]
        public string Url { get; set; }

        [JsonPropertyName("archive_url")]
        public string ArchiveUrl { get; set; }

        [JsonPropertyName("assignees_url")]
        public string AssigneesUrl { get; set; }

        [JsonPropertyName("blobs_url")]
        public string BlobsUrl { get; set; }

        [JsonPropertyName("branches_url")]
        public string BranchesUrl { get; set; }

        [JsonPropertyName("collaborators_url")]
        public string CollaboratorsUrl { get; set; }

        [JsonPropertyName("comments_url")]
        public string CommentsUrl { get; set; }

        [JsonPropertyName("commits_url")]
        public string CommitsUrl { get; set; }

        [JsonPropertyName("compare_url")]
        public string CompareUrl { get; set; }

        [JsonPropertyName("contents_url")]
        public string ContentsUrl { get; set; }

        [JsonPropertyName("contributors_url")]
        public string ContributorsUrl { get; set; }

        [JsonPropertyName("deployments_url")]
        public string DeploymentsUrl { get; set; }

        [JsonPropertyName("downloads_url")]
        public string DownloadsUrl { get; set; }

        [JsonPropertyName("events_url")]
        public string EventsUrl { get; set; }

        [JsonPropertyName("forks_url")]
        public string ForksUrl { get; set; }

        [JsonPropertyName("git_commits_url")]
        public string GitCommitsUrl { get; set; }

        [JsonPropertyName("git_refs_url")]
        public string GitRefsUrl { get; set; }

        [JsonPropertyName("git_tags_url")]
        public string GitTagsUrl { get; set; }

        [JsonPropertyName("git_url")]
        public string GitUrl { get; set; }

        [JsonPropertyName("issue_comment_url")]
        public string IssueCommentUrl { get; set; }

        [JsonPropertyName("issue_events_url")]
        public string IssueEventsUrl { get; set; }

        [JsonPropertyName("issues_url")]
        public string IssuesUrl { get; set; }

        [JsonPropertyName("keys_url")]
        public string KeysUrl { get; set; }

        [JsonPropertyName("labels_url")]
        public string LabelsUrl { get; set; }

        [JsonPropertyName("languages_url")]
        public string LanguagesUrl { get; set; }

        [JsonPropertyName("merges_url")]
        public string MergesUrl { get; set; }

        [JsonPropertyName("milestones_url")]
        public string MilestonesUrl { get; set; }

        [JsonPropertyName("notifications_url")]
        public string NotificationsUrl { get; set; }

        [JsonPropertyName("pulls_url")]
        public string PullsUrl { get; set; }

        [JsonPropertyName("releases_url")]
        public string ReleasesUrl { get; set; }

        [JsonPropertyName("ssh_url")]
        public string SshUrl { get; set; }

        [JsonPropertyName("stargazers_url")]
        public string StargazersUrl { get; set; }

        [JsonPropertyName("statuses_url")]
        public string StatusesUrl { get; set; }

        [JsonPropertyName("subscribers_url")]
        public string SubscribersUrl { get; set; }

        [JsonPropertyName("subscription_url")]
        public string SubscriptionUrl { get; set; }

        [JsonPropertyName("tags_url")]
        public string TagsUrl { get; set; }

        [JsonPropertyName("teams_url")]
        public string TeamsUrl { get; set; }

        [JsonPropertyName("trees_url")]
        public string TreesUrl { get; set; }

        [JsonPropertyName("hooks_url")]
        public string HooksUrl { get; set; }
    }

    public class GithubActionRun
    {
        [JsonPropertyName("id")]
        public string Id { get; set; }

        [JsonPropertyName("name")]
        public string Name { get; set; }

        [JsonPropertyName("node_id")]
        public string NodeId { get; set; }

        [JsonPropertyName("check_suite_id")]
        public ulong CheckSuiteId { get; set; }

        [JsonPropertyName("check_suite_node_id")]
        public string CheckSuiteNodeId { get; set; }

        [JsonPropertyName("head_branch")]
        public string HeadBranch { get; set; }

        [JsonPropertyName("head_sha")]
        public string HeadSha { get; set; }

        [JsonPropertyName("path")]
        public string Path { get; set; }

        [JsonPropertyName("run_number")]
        public ulong RunNumber { get; set; }

        [JsonPropertyName("event")]
        public string EventName { get; set; }

        [JsonPropertyName("display_title")]
        public string DisplayTitle { get; set; }

        [JsonPropertyName("status")]
        public string Status { get; set; }

        //[JsonPropertyName("conclusion")]
        //public object conclusion { get; set; }

        [JsonPropertyName("workflow_id")]
        public string WorkflowId { get; set; }

        [JsonPropertyName("url")]
        public string Url { get; set; }

        [JsonPropertyName("html_url")]
        public string HtmlUrl { get; set; }

        //[JsonPropertyName("pull_requests")]
        //public List<object> pull_requests { get; set; }

        [JsonPropertyName("created_at")]
        public DateTime CreatedAt { get; set; }

        [JsonPropertyName("updated_at")]
        public DateTime UpdatedAt { get; set; }

        [JsonPropertyName("actor")]
        public GithubUser Actor { get; set; }

        [JsonPropertyName("run_attempt")]
        public string RunAttempt { get; set; }

        [JsonPropertyName("referenced_workflows")]
        public List<GithubReferencedWorkflow> ReferencedWorkflows { get; set; }

        [JsonPropertyName("run_started_at")]
        public DateTime RunStartedAt { get; set; }

        [JsonPropertyName("triggering_actor")]
        public GithubUser TriggeringActor { get; set; }

        [JsonPropertyName("jobs_url")]
        public string JobsUrl { get; set; }

        [JsonPropertyName("logs_url")]
        public string LogsUrl { get; set; }

        [JsonPropertyName("check_suite_url")]
        public string CheckSuiteUrl { get; set; }

        [JsonPropertyName("artifacts_url")]
        public string ArtifactsUrl { get; set; }

        [JsonPropertyName("cancel_url")]
        public string CancelUrl { get; set; }

        [JsonPropertyName("rerun_url")]
        public string RerunUrl { get; set; }

        [JsonPropertyName("previous_attempt_url")]
        public string PreviousAttemptUrl { get; set; }

        [JsonPropertyName("workflow_url")]
        public string WorkflowUrl { get; set; }

        [JsonPropertyName("head_commit")]
        public GithubHeadCommit HeadCommit { get; set; }

        [JsonPropertyName("repository")]
        public GithubRepository Repository { get; set; }

        [JsonPropertyName("head_repository")]
        public GithubHeadRepository HeadRepository { get; set; }
    }

    public class SourceForgeResponseData
    {
        [JsonPropertyName("file_type")]
        public string file_type { get; init; }

        [JsonPropertyName("explicitly_staged")]
        public string explicitly_staged { get; init; }

        [JsonPropertyName("date")]
        public string date { get; init; }

        [JsonPropertyName("mtime")]
        public string Paylomtimead { get; init; }

        [JsonPropertyName("id")]
        public string id { get; init; }

        [JsonPropertyName("size")]
        public string size { get; init; }

        [JsonPropertyName("x_stage")]
        public string x_stage { get; init; }

        [JsonPropertyName("downloadable")]
        public string downloadable { get; init; }

        [JsonPropertyName("stage")]
        public string stage { get; init; }

        [JsonPropertyName("type")]
        public string type { get; init; }

        [JsonPropertyName("mime_type")]
        public string mime_type { get; init; }

        [JsonPropertyName("staged_dir")]
        public string staged_dir { get; init; }

        [JsonPropertyName("vscan")]
        public string vscan { get; init; }

        [JsonPropertyName("path")]
        public string path { get; init; }

        [JsonPropertyName("crtime")]
        public string crtime { get; init; }

        [JsonPropertyName("md5")]
        public string md5 { get; init; }

        [JsonPropertyName("sha1")]
        public string sha1 { get; init; }

        [JsonPropertyName("name")]
        public string name { get; init; }

        //[JsonPropertyName("staged")]
        //public string staged { get; init; }

        [JsonPropertyName("modified")]
        public string modified { get; init; }

        [JsonPropertyName("project")]
        public string project { get; init; }

        [JsonPropertyName("vscan_prog")]
        public string vscan_prog { get; init; }

        [JsonPropertyName("vscan_when")]
        public string vscan_when { get; init; }
    }

    public class SourceForgeUploadResponse
    {
        [JsonPropertyName("result")]
        public SourceForgeResponseData Result { get; init; }
    }

    public class VirusTotalLargeUploadResponse
    {
        [JsonPropertyName("data")]
        public string data { get; init; }
    }

    public class VirusTotalAnalysisLinksResponse
    {
        [JsonPropertyName("self")]
        public string self { get; init; }
    }

    public class VirusTotalAnalysisDataResponse
    {
        [JsonPropertyName("type")]
        public string type { get; init; }

        [JsonPropertyName("id")]
        public string id { get; init; }

        [JsonPropertyName("links")]
        public VirusTotalAnalysisLinksResponse links { get; init; }
    }

    public class VirusTotalAnalysisResponse
    {
        [JsonPropertyName("data")]
        public VirusTotalAnalysisDataResponse data { get; init; }
    }

    [JsonSerializable(typeof(BuildUpdateRequest))]
    [JsonSourceGenerationOptions(DefaultIgnoreCondition = JsonIgnoreCondition.WhenWritingNull, GenerationMode = JsonSourceGenerationMode.Default)]
    public partial class BuildUpdateRequestContext : JsonSerializerContext
    {

    }

    [JsonSerializable(typeof(GithubActionRun))]
    [JsonSerializable(typeof(GithubAssetsResponse))]
    [JsonSerializable(typeof(GithubCommitResponse))]
    [JsonSerializable(typeof(GithubReleasesRequest))]
    [JsonSerializable(typeof(GithubReleasesResponse))]
    [JsonSerializable(typeof(GithubReleaseQueryResponse))]
    [JsonSourceGenerationOptions(DefaultIgnoreCondition = JsonIgnoreCondition.WhenWritingNull, GenerationMode = JsonSourceGenerationMode.Default)]
    public partial class GithubResponseContext : JsonSerializerContext
    {

    }

    [JsonSerializable(typeof(SourceForgeUploadResponse))]
    [JsonSourceGenerationOptions(DefaultIgnoreCondition = JsonIgnoreCondition.WhenWritingNull, GenerationMode = JsonSourceGenerationMode.Default)]
    public partial class SourceForgeUploadResponseContext : JsonSerializerContext
    {

    }

    [JsonSerializable(typeof(VirusTotalLargeUploadResponse))]
    [JsonSerializable(typeof(VirusTotalAnalysisResponse))]
    [JsonSourceGenerationOptions(DefaultIgnoreCondition = JsonIgnoreCondition.WhenWritingNull, GenerationMode = JsonSourceGenerationMode.Default)]
    public partial class VirusTotalResponseContext : JsonSerializerContext
    {

    }

    [JsonSourceGenerationOptions(DefaultIgnoreCondition = JsonIgnoreCondition.WhenWritingNull)]
    [JsonSerializable(typeof(Dictionary<string, string>))]
    public partial class DictionarySerializerContext : JsonSerializerContext
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
            return ((ulong)value).ToPrettySize(decimalPlaces);
        }

        public static string ToPrettySize(this long value, int decimalPlaces = 0)
        {
            return ((ulong)value).ToPrettySize(decimalPlaces);
        }

        public static string ToPrettySize(this ulong value, int decimalPlaces = 0)
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

    public static class VT
    {
        // Reset
        public const string RESET       = "\x1b[0m";

        // Standard foreground colors
        public const string BLACK       = "\x1b[30m";
        public const string RED         = "\x1b[31m";
        public const string GREEN       = "\x1b[32m";
        public const string YELLOW      = "\x1b[33m";
        public const string BLUE        = "\x1b[34m";
        public const string MAGENTA     = "\x1b[35m";
        public const string CYAN        = "\x1b[36m";
        public const string WHITE       = "\x1b[37m";

        // Bright foreground colors
        public const string BRIGHT_BLACK    = "\x1b[90m";
        public const string BRIGHT_RED      = "\x1b[91m";
        public const string BRIGHT_GREEN    = "\x1b[92m";
        public const string BRIGHT_YELLOW   = "\x1b[93m";
        public const string BRIGHT_BLUE     = "\x1b[94m";
        public const string BRIGHT_MAGENTA  = "\x1b[95m";
        public const string BRIGHT_CYAN     = "\x1b[96m";
        public const string BRIGHT_WHITE    = "\x1b[97m";

        // Background colors
        public const string BG_BLACK    = "\x1b[40m";
        public const string BG_RED      = "\x1b[41m";
        public const string BG_GREEN    = "\x1b[42m";
        public const string BG_YELLOW   = "\x1b[43m";
        public const string BG_BLUE     = "\x1b[44m";
        public const string BG_MAGENTA  = "\x1b[45m";
        public const string BG_CYAN     = "\x1b[46m";
        public const string BG_WHITE    = "\x1b[47m";

        // Bright background colors
        public const string BG_BRIGHT_BLACK     = "\x1b[100m";
        public const string BG_BRIGHT_RED       = "\x1b[101m";
        public const string BG_BRIGHT_GREEN     = "\x1b[102m";
        public const string BG_BRIGHT_YELLOW    = "\x1b[103m";
        public const string BG_BRIGHT_BLUE      = "\x1b[104m";
        public const string BG_BRIGHT_MAGENTA   = "\x1b[105m";
        public const string BG_BRIGHT_CYAN      = "\x1b[106m";
        public const string BG_BRIGHT_WHITE     = "\x1b[107m";

        // 256-color foreground
        public static string FROM256(int n) => $"\x1b[38;5;{n}m";

        // 256-color background
        public static string BGFROM256(int n) => $"\x1b[48;5;{n}m";

        // True-color (RGB)
        public static string RGB(int r, int g, int b) => $"\x1b[38;2;{r};{g};{b}m";
        public static string BGRGB(int r, int g, int b) => $"\x1b[48;2;{r};{g};{b}m";
    }
}
