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
    /// Provides utility methods for file operations, environment management, argument parsing, and build tool
    /// integration. This class includes helpers for interacting with build systems, locating toolchain executables,
    /// reading and writing files, and manipulating environment variables.
    /// </summary>
    /// <remarks>The methods in this class are designed to support build automation and scripting scenarios,
    /// including locating Visual Studio, MSBuild, Git, and CMake executables, parsing command-line arguments, and
    /// handling Windows SDK paths. Many methods assume Windows environments and may rely on environment variables or
    /// registry keys. Thread safety is not guaranteed for all static members; use caution when accessing shared
    /// resources concurrently.</remarks>
    public static unsafe partial class Utils
    {
        private static readonly Dictionary<string, string> EnvironmentBlock = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase);
        public static readonly Encoding UTF8NoBOM = new UTF8Encoding(encoderShouldEmitUTF8Identifier: false, throwOnInvalidBytes: true);
        private static string GitFilePath;
        private static string VsWhereFilePath;

        /// <summary>
        /// Generates a random integer from 1 through 7, inclusive.
        /// </summary>
        public static int GetRandomNumberOneToSeven()
        {
            return RandomNumberGenerator.GetInt32(1, 8);
        }

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
        public static Dictionary<string, string> ParseArguments(string[] Args)
        {
            // 2. Track the current key (string) if it starts with "-".
            // 3. For each string in args:
            //    a. If it starts with "-", set as current key, add to dict if not present.
            //    b. Else, if the current key is set, append a value to its list.
            //    c. If no current key, ignore or add to a special key (optional).
            // 4. Return the dictionary.

            var kvp = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase);
            var key = string.Empty;

            foreach (string s in Args)
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

            var args = File.ReadLines(FileName);
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

        /// <summary>
        /// Parses command-line arguments from a JSON file and returns them as a dictionary with normalized keys.
        /// </summary>
        /// <remarks>Keys in the resulting dictionary are prefixed with a hyphen if not already present.
        /// Parsing errors are handled gracefully, and no exception is thrown; instead, an empty dictionary is
        /// returned.</remarks>
        /// <param name="filePath">The path to the JSON file containing argument key-value pairs. The file must exist and be readable.</param>
        /// <returns>A dictionary containing the parsed arguments, with keys normalized to start with a hyphen ('-'). The
        /// dictionary is case-insensitive. If the file cannot be parsed, an empty dictionary is returned.</returns>
        public static Dictionary<string, string> ParseArgumentsFromFile(string filePath)
        {
            var kvp = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase);

            try
            {
                using var jsonContent = File.OpenRead(filePath);
                var args = JsonSerializer.Deserialize(jsonContent, DictionarySerializerContext.Default.DictionaryStringString);

                foreach (var s in args)
                {
                    string key = s.Key;

                    if (!key.StartsWith('-'))
                    {
                        key = "-" + key;
                    }

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

        /// <summary>
        /// Executes an MSBuild command using the specified build flags and returns the process exit code.
        /// </summary>
        /// <remarks>If the MSBuild executable cannot be located based on the provided flags and system
        /// architecture, the method returns 3 and sets <paramref name="OutputString"/> to an error message. Otherwise,
        /// the output from the MSBuild process is captured in <paramref name="OutputString"/> if <paramref
        /// name="RedirectOutput"/> is <see langword="true"/>.</remarks>
        /// <param name="Arguments">The MSBuild command-line arguments to execute.</param>
        /// <param name="Flags">The build flags that determine which MSBuild executable to use and how the command is executed.</param>
        /// <param name="OutputString">When the method returns, contains the output produced by the MSBuild process. If the MSBuild executable
        /// cannot be found, it contains an error message.</param>
        /// <param name="RedirectOutput">Indicates whether the output from the MSBuild process should be redirected and captured. The default is <see
        /// langword="true"/>.</param>
        /// <returns>The exit code returned by the MSBuild process. Returns 3 if the MSBuild executable cannot be found.</returns>
        public static int ExecuteMsbuildCommand(string Arguments, BuildFlags Flags, out string OutputString, bool RedirectOutput = true)
        {
            string file = GetMsbuildFilePath(Flags, RuntimeInformation.ProcessArchitecture);

            if (string.IsNullOrWhiteSpace(file))
            {
                OutputString = "[ExecuteMsbuildCommand] MsbuildFilePath is invalid.";
                return 3; // file not found.
            }

            return Win32.CreateProcess(file, Arguments, out OutputString, false, RedirectOutput);
        }

        /// <summary>
        /// Executes a command using the vswhere utility and returns the trimmed output as a string.
        /// </summary>
        /// <remarks>The method locates the vswhere executable and runs the specified command. If the
        /// vswhere file path is invalid, the method returns null and displays an error message. The output is trimmed
        /// to remove leading and trailing whitespace.</remarks>
        /// <param name="Arguments">The command-line arguments to pass to the vswhere executable. Cannot be null or empty.</param>
        /// <returns>A string containing the trimmed output from the vswhere command. Returns null if the vswhere executable
        /// cannot be found.</returns>
        public static string ExecuteVsWhereCommand(IEnumerable<string> Arguments)
        {
            string file = GetVswhereFilePath();

            if (string.IsNullOrWhiteSpace(file))
            {
                Program.PrintColorMessage("[ExecuteVsWhereCommand] VswhereFilePath is invalid.", ConsoleColor.Red);
                return null;
            }

            Win32.CreateProcess(file, Arguments, out var outputString, false);

            return outputString.Trim();
        }

        /// <summary>
        /// Attempts to set the current working directory to the nearest parent directory containing the specified file.
        /// </summary>
        /// <remarks>If the specified file is found in a parent directory, the current working directory
        /// is changed to that directory. If the file is not found or an error occurs, the current directory remains
        /// unchanged and the method returns false.</remarks>
        /// <param name="FileName">The name of the file to search for in parent directories. Cannot be null or empty.</param>
        /// <returns>true if the current directory contains the specified file after the operation; otherwise, false.</returns>
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

        /// <summary>
        /// Combines the working folder path with the specified file name to generate the full output directory path.
        /// </summary>
        /// <param name="FileName">The name of the file to append to the working folder path. Cannot be null or empty.</param>
        /// <returns>A string containing the full path to the output directory for the specified file.</returns>
        public static string GetOutputDirectoryPath(string FileName)
        {
            return Path.Join([Build.BuildWorkingFolder, FileName]);
        }

        /// <summary>
        /// Creates the output directory specified by the build configuration if it does not already exist and is not a
        /// file.
        /// </summary>
        /// <remarks>If the output folder path is null, empty, or whitespace, or if it points to an
        /// existing file, the method does nothing. Any errors encountered during directory creation are reported to the
        /// console.</remarks>
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

        /// <summary>
        /// Gets the full path to the vswhere.exe utility if it exists on the system.
        /// </summary>
        /// <remarks>Searches common installation directories and the system PATH for
        /// vswhere.exe.</remarks>
        /// <returns>The full file path to vswhere.exe, or an empty string if not found.</returns>
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

        /// <summary>
        /// Locates the file path to the MSBuild executable for the specified build flags and target architecture.
        /// </summary>
        /// <remarks>The method searches for MSBuild in environment variables, system paths, and Visual
        /// Studio installation directories. If multiple locations are available, the search order prioritizes
        /// environment variables, then system paths, followed by Visual Studio instances. The returned path may vary
        /// depending on the installed Visual Studio versions and system configuration.</remarks>
        /// <param name="Flags">The build flags that influence how MSBuild is located and which version is selected.</param>
        /// <param name="Architecture">The target processor architecture for which the MSBuild executable should be found. Must be a valid
        /// architecture supported by MSBuild.</param>
        /// <returns>A string containing the full file path to the MSBuild executable matching the specified architecture and
        /// build flags, or null if no suitable executable is found.</returns>
        private static string GetMsbuildFilePath(BuildFlags Flags, Architecture Architecture)
        {
            // ESDK Begin
            if (Win32.GetEnvironmentVariable("MSBuild", out string msbuildPath))
            {
                if (File.Exists(msbuildPath))
                    return msbuildPath;
            }

            {
                string file = Win32.SearchPath("MSBuild.exe");

                if (File.Exists(file))
                {
                    return file;
                }
            }
            // ESDK End

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
                [
                    "-latest",
                    "-prerelease",
                    "-products",
                    "*",
                    "-requiresAny",
                    "-requires",
                    "Microsoft.Component.MSBuild",
                    "-property",
                    "installationPath"
                ]);

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
                [
                    "-latest",
                    "-prerelease",
                    "-products",
                    "*",
                    "-requiresAny",
                    "-requires",
                    "Microsoft.Component.MSBuild",
                    "-find",
                    "MSBuild\\**\\Bin\\MSBuild.exe"
                ]);

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

        /// <summary>
        /// Retrieves the full file path to the Git executable on the local machine.
        /// </summary>
        /// <remarks>This method searches common installation directories for the Git executable and
        /// caches the result for later calls. If Git is not installed in the standard locations, it attempts to
        /// locate the executable using the system search path.</remarks>
        /// <returns>A string containing the absolute path to the Git executable if found; otherwise, an empty string.</returns>
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

        /// <summary>
        /// Constructs a command-line argument string for specifying the Git directory and working tree based on the
        /// provided directory path.
        /// </summary>
        /// <remarks>The returned string can be used with Git commands to explicitly set the repository
        /// location and working tree. If the directory does not contain a .git folder, the method returns
        /// null.</remarks>
        /// <param name="DirectoryPath">The file system path to the directory containing the Git repository. Must not be null, empty, or whitespace.</param>
        /// <returns>A string containing the Git command-line arguments for the specified directory, or null if the directory
        /// path is invalid or does not contain a .git folder.</returns>
        private static string GetGitWorkPath(string DirectoryPath)
        {
            if (string.IsNullOrWhiteSpace(DirectoryPath))
                return null;

            if (!Directory.Exists($"{DirectoryPath}\\.git"))
                return null;

            return $"--git-dir=\"{DirectoryPath}\\.git\" --work-tree=\"{DirectoryPath}\" ";
        }

        /// <summary>
        /// Executes a Git command in the specified working folder and returns the command output as a string.
        /// </summary>
        /// <remarks>If the working folder is not a valid Git directory or the Git executable cannot be
        /// found, the method returns null and prints an error message to the console. The output string is trimmed
        /// before being returned.</remarks>
        /// <param name="WorkingFolder">The path to the folder where the Git command will be executed. Must be a valid Git working directory.</param>
        /// <param name="Arguments">The Git command to execute. This should be a valid command supported by the Git executable.</param>
        /// <returns>A string containing the trimmed output of the executed Git command. Returns null if the working folder or
        /// Git executable path is invalid.</returns>
        public static string ExecuteGitCommand(string WorkingFolder, IEnumerable<string> Arguments)
        {
            if (string.IsNullOrWhiteSpace(WorkingFolder) || !Directory.Exists(WorkingFolder))
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

            List<string> arguments =
            [
                "-C",
                WorkingFolder
            ];

            arguments.AddRange(Arguments);

            Win32.CreateProcess(currentGitPath, arguments, out var outputString, false);

            return outputString.Trim();
        }

        /// <summary>
        /// Lists all files in the specified directory and its subdirectories that match the given file extensions,
        /// optionally excluding specified file names.
        /// </summary>
        /// <remarks>The search includes all subdirectories and ignores special directories such as "."
        /// and "..". File extension and exclusion comparisons are performed in a case-insensitive manner.</remarks>
        /// <param name="FilePath">The path to the directory to search. Must be a valid directory path.</param>
        /// <param name="Extensions">An array of file extensions to include in the results. Extensions should include the leading dot (e.g.,
        /// ".txt").</param>
        /// <param name="Exclude">An optional array of file names to exclude from the results. Comparison is case-insensitive.</param>
        /// <returns>A list of file paths that match the specified extensions and are not excluded. The list will be empty if no
        /// files are found.</returns>
        public static IEnumerable<string> EnumerateDirectory(string FilePath, string[] Extensions, string[] Exclude = null)
        {
            var files = Directory.EnumerateFiles(FilePath, "*", new EnumerationOptions
            {
                RecurseSubdirectories = true,
                AttributesToSkip = FileAttributes.ReparsePoint,
                ReturnSpecialDirectories = false
            });

            foreach (var s in files)
            {
                if (Extensions.Any(ext => string.Equals(ext, Path.GetExtension(s), StringComparison.OrdinalIgnoreCase)))
                {
                    if (Exclude == null || !Exclude.Any(f => f.Equals(Path.GetFileName(s), StringComparison.OrdinalIgnoreCase)))
                    {
                        yield return s;
                    }
                }
            }
        }

        /// <summary>
        /// Retrieves the file system path to the Windows SDK binaries directory, if available.
        /// </summary>
        /// <remarks>This method searches for the Windows SDK path using environment variables and
        /// registry keys. The returned path can be used to locate SDK tools and binaries. If multiple SDK versions are
        /// installed, the method returns the path to the highest available version.</remarks>
        /// <returns>A string containing the path to the Windows SDK binaries directory. Returns null if the SDK path cannot be
        /// determined.</returns>
        public static string GetWindowsSdkPath()
        {
            // ESDK Begin
            if (
                Win32.GetEnvironmentVariable("WindowsSdkDir", out string windowsSdkDir) &&
                Win32.GetEnvironmentVariable("WindowsSDKVersion", out string windowsSdkVersion)
                )
            {
                string path = Path.Join([windowsSdkDir, "bin", windowsSdkVersion.TrimEnd('\\')]);
                if (Directory.Exists(path))
                    return path;
            }
            // ESDK End

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

        /// <summary>
        /// Retrieves the version number of the installed Windows SDK, if available.
        /// </summary>
        /// <remarks>The method first checks the 'WindowsSDKVersion' environment variable. If not set, it
        /// searches the Windows Kits installation directory for available SDK versions and returns the highest version
        /// found. The returned value does not include a trailing backslash.</remarks>
        /// <returns>A string containing the Windows SDK version number, or null if no SDK version is found.</returns>
        public static string GetWindowsSdkVersion()
        {
            // ESDK Begin
            if (Win32.GetEnvironmentVariable("WindowsSDKVersion", out string windowsSdkVersion))
            {
                return windowsSdkVersion.TrimEnd('\\');
            }
            // ESDK End

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

        /// <summary>
        /// Retrieves the path to the Windows SDK include directory for the current system.
        /// </summary>
        /// <remarks>The method searches for the Windows SDK include path using environment variables and
        /// registry keys. If multiple SDK versions are installed, it returns the path for the highest available
        /// version. The returned path can be used to locate header files required for Windows development.</remarks>
        /// <returns>A string containing the full path to the Windows SDK include directory if found; otherwise, null.</returns>
        public static string GetWindowsSdkIncludePath()
        {
            // ESDK Begin
            if (
                Win32.GetEnvironmentVariable("WindowsSdkDir", out string windowsSdkDir) &&
                Win32.GetEnvironmentVariable("WindowsSDKVersion", out string windowsSdkVersion)
                )
            {
                string path = Path.Join([windowsSdkDir, "Include", windowsSdkVersion.TrimEnd('\\')]);
                if (Directory.Exists(path))
                    return path;
            }
            // ESDK End

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

        /// <summary>
        /// Retrieves the Visual Studio product version associated with the specified build flags and architecture.
        /// </summary>
        /// <remarks>Returns an empty string if the MSBuild path cannot be resolved or if the Visual
        /// Studio installation is not found. This method does not throw exceptions; errors are logged and an empty
        /// string is returned.</remarks>
        /// <param name="Flags">The build flags that determine the MSBuild configuration and location.</param>
        /// <param name="Architecture">The target architecture for which to locate the Visual Studio version.</param>
        /// <returns>A string containing the Visual Studio product version, or an empty string if the version cannot be
        /// determined.</returns>
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

        /// <summary>
        /// Retrieves the full path to the MakeAppx.exe tool from the Windows SDK installation.
        /// </summary>
        /// <remarks>This method searches for MakeAppx.exe in standard Windows SDK locations. If the tool
        /// is not installed or cannot be found, the method returns null. Use this method to locate MakeAppx.exe for
        /// packaging Windows applications.</remarks>
        /// <returns>The full path to MakeAppx.exe if found; otherwise, null.</returns>
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

        /// <summary>
        /// Retrieves the full path to the Message Compiler (mc.exe) tool from the Windows SDK installation.
        /// </summary>
        /// <remarks>mc.exe ships with the Windows SDK (not the WDK), so it can be used to compile message
        /// files for the usermode build without requiring the WDK build customizations.</remarks>
        /// <returns>The full path to mc.exe if found; otherwise, null.</returns>
        public static string GetMessageCompilerPath()
        {
            string windowsSdkPath = Utils.GetWindowsSdkPath();

            if (string.IsNullOrWhiteSpace(windowsSdkPath))
                return null;

            string messageCompilerPath = Path.Join([windowsSdkPath, "\\x64\\mc.exe"]);

            if (!File.Exists(messageCompilerPath))
            {
                string sdkRootPath = Win32.GetKeyValue(true, "Software\\Microsoft\\Windows Kits\\Installed Roots", "WdkBinRootVersioned", null);

                if (string.IsNullOrWhiteSpace(sdkRootPath))
                    return null;

                messageCompilerPath = Utils.ExpandFullPath(Path.Join([sdkRootPath, "\\x64\\mc.exe"]));

                if (!File.Exists(messageCompilerPath))
                    return null;
            }

            return messageCompilerPath;
        }

        /// <summary>
        /// Locates the full file path to the CMake executable (cmake.exe) available on the system.
        /// </summary>
        /// <remarks>The method first searches for cmake.exe in the system PATH. If not found, it attempts
        /// to locate a bundled version with Visual Studio. Returns null if CMake is not installed or cannot be located
        /// by either method.</remarks>
        /// <returns>The full path to the CMake executable if found; otherwise, null.</returns>
        public static string GetCMakeFilePath()
        {
            //
            // Try searching in the PATH first
            //

            string file = Win32.SearchPath("cmake.exe");

            if (File.Exists(file))
            {
                return file;
            }

            //
            // Try bundled with Visual Studio
            //

            VisualStudioInstance instance = BuildVisualStudio.GetVisualStudioInstance();

            if (instance != null)
            {
                string[] cmakePathArray =
                [
                    "\\Common7\\IDE\\CommonExtensions\\Microsoft\\CMake\\CMake\\bin\\cmake.exe",
                ];

                foreach (string path in cmakePathArray)
                {
                    file = Path.Join([instance.Path, path]);

                    if (File.Exists(file))
                    {
                        return file;
                    }
                }
            }

            return null;
        }

        /// <summary>
        /// Executes a CMake command in a Visual Studio environment, initializing the required build tools before
        /// running the command.
        /// </summary>
        /// <remarks>This method sets up the Visual Studio build environment by invoking VsDevCmd.bat for the
        /// appropriate architecture before executing the CMake command. The exit code can be used to determine
        /// success or failure of the operation.</remarks>
        /// <param name="Arguments">The CMake command line arguments to execute. Must specify the desired build configuration and architecture.</param>
        /// <returns>The exit code returned by the CMake process. Returns <see cref="int.MaxValue"/> if required tools are not
        /// found or initialization fails.</returns>
        public static int ExecuteCMakeCommand(IEnumerable<string> Arguments)
        {
            string[] arguments = Arguments?.ToArray() ?? [];
            string cmakeFile = GetCMakeFilePath();

            if (string.IsNullOrWhiteSpace(cmakeFile))
            {
                Program.PrintColorMessage("[ExecuteCMakeCommand] cmake.exe is invalid.", ConsoleColor.Red);
                return int.MaxValue;
            }

            var instance = BuildVisualStudio.GetVisualStudioInstance();
            if (instance == null)
            {
                Program.PrintColorMessage("[ExecuteCMakeCommand] instance not found.", ConsoleColor.Red);
                return int.MaxValue;
            }

            string vsDevCmd = Path.Join([instance.Path, "Common7\\Tools\\VsDevCmd.bat"]);
            string arch = null;

            if (!File.Exists(vsDevCmd))
            {
                Program.PrintColorMessage("[ExecuteCMakeCommand] VsDevCmd.bat not found.", ConsoleColor.Red);
                return int.MaxValue;
            }

            foreach (string argument in arguments)
            {
                if (argument.Contains("msvc-x86", StringComparison.OrdinalIgnoreCase) || argument.Contains("Win32", StringComparison.OrdinalIgnoreCase))
                {
                    arch = "x86";
                    break;
                }

                if (argument.Contains("msvc-arm64", StringComparison.OrdinalIgnoreCase) || argument.Contains("ARM64", StringComparison.OrdinalIgnoreCase))
                {
                    arch = "arm64";
                    break;
                }
            }

            arch ??= "amd64";

            string command = string.Concat(
                "\"\"",
                vsDevCmd,
                "\"",
                " -arch=",
                arch,
                " -host_arch=amd64 && ",
                QuoteCmdArgument(cmakeFile),
                " ",
                string.Join(" ", arguments.Select(QuoteCmdArgument)),
                "\""
                );

            return Win32.CreateProcess(
                "cmd.exe",
                $"/d /s /c {command}",
                out _,
                false,
                false
                );
        }

        /// <summary>
        /// Encloses the specified argument in double quotes for use on a cmd.exe command line.
        /// </summary>
        /// <param name="Argument">The argument to quote.</param>
        /// <returns>The quoted argument.</returns>
        private static string QuoteCmdArgument(string Argument)
        {
            return $"\"{Argument}\"";
        }

        /// <summary>
        /// Executes an MSIX packaging command using the MakeAppx tool and returns the output as a string.
        /// </summary>
        /// <remarks>The returned output is trimmed and consecutive blank lines are reduced to a single
        /// blank line. This method does not throw exceptions for invalid tool paths; instead, it returns null and
        /// prints an error message.</remarks>
        /// <param name="Arguments">The command-line arguments to pass to the MakeAppx tool. Cannot be null or empty.</param>
        /// <returns>A string containing the output from the MakeAppx tool. Returns null if the tool path is invalid.</returns>
        public static string ExecuteMsixCommand(IEnumerable<string> Arguments)
        {
            string file = GetMakeAppxPath();

            if (string.IsNullOrWhiteSpace(file))
            {
                Program.PrintColorMessage("[ExecuteMsixCommand] File is invalid.", ConsoleColor.Red);
                return null;
            }

            Win32.CreateProcess(file, Arguments, out var outputString, false);

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

        /// <summary>
        /// Retrieves the full file path to the SymStore executable (symstore.exe) from the Windows SDK installation, if
        /// available.
        /// </summary>
        /// <remarks>This method searches for symstore.exe in the Windows SDK directory, using environment
        /// variables and registry keys. The returned path can be used to invoke SymStore for symbol storage operations.
        /// If the SDK is not installed or symstore.exe is not present, the method returns null.</remarks>
        /// <returns>The full path to symstore.exe if found; otherwise, null.</returns>
        public static string GetSymStorePath()
        {
            // Required for the ESDK
            if (Win32.GetEnvironmentVariable("WindowsSdkDir", out string windowsSdkDir))
            {
                string path = Path.Join([windowsSdkDir, "Debuggers\\x64\\symstore.exe"]);
                if (File.Exists(path))
                    return path;
            }

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

        /// <summary>
        /// Retrieves the full file path to the COSE signing tool (CoseSignTool.exe) from the Windows SDK/WDK
        /// installation, if available.
        /// </summary>
        /// <remarks>CoseSignTool.exe ships under the versioned Tools directory of the Windows Kit
        /// (<c>{KitsRoot10}\Tools\{version}\x64\CoseSignTool.exe</c>). The method honors the ESDK environment
        /// variables when set, otherwise it selects the highest installed Tools version that contains the tool.
        /// Returns null if the tool is not found.</remarks>
        /// <returns>The full path to CoseSignTool.exe if found; otherwise, null.</returns>
        public static string GetCoseSignToolPath()
        {
            // Required for the ESDK
            if (
                Win32.GetEnvironmentVariable("WindowsSdkDir", out string windowsSdkDir) &&
                Win32.GetEnvironmentVariable("WindowsSDKVersion", out string windowsSdkVersion)
                )
            {
                string path = Path.Join([windowsSdkDir, "Tools", windowsSdkVersion.TrimEnd('\\'), "x64", "CoseSignTool.exe"]);
                if (File.Exists(path))
                    return path;
            }

            string kitsRoot = Win32.GetKeyValue(true, "Software\\Microsoft\\Windows Kits\\Installed Roots", "KitsRoot10", "%ProgramFiles(x86)%\\Windows Kits\\10\\");
            if (string.IsNullOrWhiteSpace(kitsRoot))
                return null;

            string toolsPath = Utils.ExpandFullPath(Path.Join([kitsRoot, "Tools"]));
            if (string.IsNullOrWhiteSpace(toolsPath) || !Directory.Exists(toolsPath))
                return null;

            List<KeyValuePair<Version, string>> versionList = new List<KeyValuePair<Version, string>>();

            foreach (string directory in Directory.EnumerateDirectories(toolsPath))
            {
                if (Version.TryParse(Path.GetFileName(directory), out var version))
                {
                    string candidate = Path.Join([directory, "x64", "CoseSignTool.exe"]);

                    if (File.Exists(candidate))
                        versionList.Add(new KeyValuePair<Version, string>(version, candidate));
                }
            }

            if (versionList.Count == 0)
                return null;

            versionList.Sort((first, second) => first.Key.CompareTo(second.Key));

            return versionList[^1].Value;
        }

        /// <summary>
        /// Determines whether the symstore.exe file exists and is valid.
        /// </summary>
        /// <returns>true if the symstore.exe file exists and is valid; otherwise, false.</returns>
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

        /// <summary>
        /// Executes a command using symstore.exe and returns the process exit code.
        /// </summary>
        /// <param name="Arguments">The command-line arguments to pass to symstore.exe.</param>
        /// <returns>The exit code of the process, or int.MaxValue if symstore.exe is invalid or not found.</returns>
        public static int ExecuteSymStoreCommand(IEnumerable<string> Arguments)
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

            return Win32.CreateProcess(file, Arguments, out _, false, false);
        }

        /// <summary>
        /// Retrieves the full path to the Visual Studio 'devenv.com' executable for the latest installed instance.
        /// </summary>
        /// <remarks>This method searches for the latest Visual Studio installation and returns the path
        /// to its 'devenv.com' executable. If no suitable installation is found, the method returns null. The result
        /// can be used to launch Visual Studio from the command line.</remarks>
        /// <returns>A string containing the path to 'devenv.com' if found; otherwise, null.</returns>
        public static string GetDevEnvPath()
        {
            var instance = BuildVisualStudio.GetVisualStudioInstance();
            if (instance != null)
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
                [
                    "-latest",
                    "-prerelease",
                    "-products",
                    "*",
                    "-requiresAny",
                    "-requires",
                    "Microsoft.Component.MSBuild",
                    "-property",
                    "installationPath"
                ],
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

        /// <summary>
        /// Executes a command using the Visual Studio Developer Environment (devenv.exe) and returns the process exit
        /// code.
        /// </summary>
        /// <param name="Arguments">The command-line arguments to pass to devenv.exe.</param>
        /// <returns>The exit code of the process, or Int32.MaxValue if the devenv.exe path is invalid.</returns>
        public static int ExecuteDevEnvCommand(string Arguments)
        {
            string currentDevEnvPath = GetDevEnvPath();

            if (string.IsNullOrWhiteSpace(currentDevEnvPath))
            {
                Program.PrintColorMessage("[ExecuteDevEnvCommand] File is invalid.", ConsoleColor.Red);
                return int.MaxValue;
            }

            return Win32.CreateProcess(currentDevEnvPath, Arguments, out _, false, false);
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

        /// <summary>
        /// Reads all text from the specified file using UTF-8 encoding without a byte order mark (BOM).
        /// </summary>
        /// <param name="FileName">The path to the file to read.</param>
        /// <returns>A string containing all text from the file.</returns>
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

        /// <summary>
        /// Reads the first non-empty line of a file, trimmed, without reading the rest of the file.
        /// </summary>
        /// <param name="FileName">The path to the file to read.</param>
        /// <returns>The trimmed first non-empty line, or an empty string if the file has none.</returns>
        public static string ReadFirstLine(string FileName)
        {
            foreach (string line in File.ReadLines(FileName))
            {
                string value = line.Trim();

                if (value.Length != 0)
                    return value;
            }

            return string.Empty;
        }

        /// <summary>
        /// Creates a new file, writes the specified string to the file using UTF-8 encoding without a BOM, and then
        /// closes the file. Overwrites the file if it already exists.
        /// </summary>
        /// <param name="FileName">The path to the file to write.</param>
        /// <param name="Content">The string to write to the file.</param>
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

        /// <summary>
        /// Writes generated text to a temporary file and replaces the destination file only when the content differs.
        /// </summary>
        /// <remarks>The destination file is left untouched when the generated content matches the existing content.
        /// The temporary file is always deleted before the method returns.</remarks>
        /// <param name="FileName">The path of the destination file.</param>
        /// <param name="Write">A callback that writes the generated content to the supplied <see cref="TextWriter"/>.</param>
        /// <param name="BeforeReplace">An optional callback invoked immediately before the destination file is replaced.</param>
        /// <param name="ShouldReplace">An optional predicate evaluated after generation; returning false skips the replacement.</param>
        /// <param name="Comparison">The comparison used to determine whether the content has changed.</param>
        /// <returns>True if the destination file was created or replaced; otherwise, false.</returns>
        public static bool WriteTextIfChanged(string FileName, Action<TextWriter> Write, Action BeforeReplace = null, Func<bool> ShouldReplace = null, StringComparison Comparison = StringComparison.OrdinalIgnoreCase)
        {
            string temporaryFile = FileName + "." + Guid.NewGuid().ToString("N") + ".tmp";

            try
            {
                using (var writer = new StreamWriter(temporaryFile, false, UTF8NoBOM))
                    Write(writer);

                if (ShouldReplace != null && !ShouldReplace())
                    return false;

                if (File.Exists(FileName))
                {
                    using var previous = new StreamReader(FileName, UTF8NoBOM, true);
                    using var generated = new StreamReader(temporaryFile, UTF8NoBOM, true);
                    char[] left = new char[4096];
                    char[] right = new char[4096];

                    while (true)
                    {
                        int leftCount = previous.ReadBlock(left);
                        int rightCount = generated.ReadBlock(right);
                        if (leftCount != rightCount)
                            break;
                        if (leftCount == 0)
                            return false;
                        if (!left.AsSpan(0, leftCount).Equals(right.AsSpan(0, rightCount), Comparison))
                            break;
                    }
                }

                BeforeReplace?.Invoke();
                File.Move(temporaryFile, FileName, true);
                return true;
            }
            finally
            {
                if (File.Exists(temporaryFile))
                    File.Delete(temporaryFile);
            }
        }

        /// <summary>
        /// Copies a text file to a destination, applying case-insensitive find and replace operations to each line.
        /// </summary>
        /// <remarks>Line endings are preserved and the destination is only rewritten when its content changes.</remarks>
        /// <param name="SourceFile">The path of the source file.</param>
        /// <param name="DestinationFile">The path of the destination file.</param>
        /// <param name="Replacements">The find and replace pairs to apply to each line.</param>
        public static void ReplaceTextFile(string SourceFile, string DestinationFile, params (string Find, string Replace)[] Replacements)
        {
            WriteTextIfChanged(DestinationFile, writer =>
            {
                foreach (string sourceLine in ReadLinesWithEndings(SourceFile))
                {
                    string line = sourceLine;
                    foreach (var replacement in Replacements)
                        line = line.Replace(replacement.Find, replacement.Replace, StringComparison.OrdinalIgnoreCase);
                    writer.Write(line);
                }
            }, Comparison: StringComparison.Ordinal);
        }

        /// <summary>
        /// Enumerates the lines of a text file, preserving the original line ending characters.
        /// </summary>
        /// <param name="FileName">The path of the file to read.</param>
        /// <returns>An enumerable sequence of lines, each including its trailing line ending if present.</returns>
        public static IEnumerable<string> ReadLinesWithEndings(string FileName)
        {
            using var reader = new StreamReader(FileName, UTF8NoBOM, true);
            var line = new StringBuilder();
            int value;
            while ((value = reader.Read()) != -1)
            {
                line.Append((char)value);
                if (value == '\n')
                {
                    yield return line.ToString();
                    line.Clear();
                }
            }
            if (line.Length != 0)
                yield return line.ToString();
        }

        /// <summary>
        /// A <see cref="TextWriter"/> wrapper that suppresses trailing whitespace by buffering it until non-whitespace content is written.
        /// </summary>
        /// <param name="output">The underlying writer that receives the trimmed output.</param>
        public sealed class TrimmingTextWriter(TextWriter output) : TextWriter
        {
            private readonly StringBuilder pending = new();

            public override Encoding Encoding => output.Encoding;
            public bool HasContent { get; private set; }

            /// <summary>
            /// Writes a character, deferring whitespace until non-whitespace content follows.
            /// </summary>
            /// <param name="value">The character to write.</param>
            public override void Write(char value)
            {
                if (char.IsWhiteSpace(value))
                    this.pending.Append(value);
                else
                {
                    if (this.pending.Length != 0)
                    {
                        output.Write(this.pending.ToString());
                        this.pending.Clear();
                    }
                    output.Write(value);
                    HasContent = true;
                }
            }

            /// <summary>
            /// Writes a string, deferring trailing whitespace until non-whitespace content follows.
            /// </summary>
            /// <param name="value">The string to write.</param>
            public override void Write(string value)
            {
                Write(value.AsSpan());
            }

            /// <summary>
            /// Writes a span of characters, deferring trailing whitespace until non-whitespace content follows.
            /// </summary>
            /// <param name="value">The characters to write.</param>
            public override void Write(ReadOnlySpan<char> value)
            {
                int lastContent = value.Length - 1;
                while (lastContent >= 0 && char.IsWhiteSpace(value[lastContent]))
                    lastContent--;

                if (lastContent >= 0)
                {
                    if (this.pending.Length != 0)
                    {
                        output.Write(this.pending.ToString());
                        this.pending.Clear();
                    }
                    output.Write(value[..(lastContent + 1)]);
                    HasContent = true;
                }
                if (lastContent + 1 < value.Length)
                    this.pending.Append(value[(lastContent + 1)..]);
            }
        }

        /// <summary>
        /// Reads the contents of the specified file into a byte array.
        /// </summary>
        /// <param name="FileName">The path to the file to read.</param>
        /// <returns>A byte array containing the contents of the file.</returns>
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

            using (FileStream filestream = new FileStream(FileName, options))
            {
                filestream.ReadExactly(buffer);
            }

            return buffer;
        }

        /// <summary>
        /// Writes a byte array to a file, creating or overwriting the file as needed.
        /// </summary>
        /// <remarks>If the file already exists, it is overwritten. The file is created if it does not
        /// exist.</remarks>
        /// <param name="FileName">The path to the file to write.</param>
        /// <param name="Buffer">The byte array to write to the file.</param>
        public static void WriteAllBytes(string FileName, ReadOnlySpan<byte> Buffer)
        {
            FileStreamOptions options = new FileStreamOptions
            {
                Mode = FileMode.Create,
                Access = FileAccess.Write,
                Share = FileShare.Write | FileShare.Delete,
                Options = FileOptions.SequentialScan,
                PreallocationSize = Buffer.Length,
                BufferSize = 0x1000,
            };

            using (FileStream filestream = new FileStream(FileName, options))
            {
                filestream.Write(Buffer);
            }
        }

        /// <summary>
        /// Indicates whether a read-only character span is empty or consists only of white-space characters.
        /// </summary>
        /// <param name="value">The read-only character span to evaluate.</param>
        /// <returns>true if the span is empty or contains only white-space characters; otherwise, false.</returns>
        public static bool IsSpanNullOrWhiteSpace(ReadOnlySpan<char> value)
        {
            if (value.IsEmpty)
                return true;
            if (value.IsWhiteSpace())
                return true;

            return false;
        }

        /// <summary>
        /// Returns the absolute path after expanding any environment variables in the specified path string.
        /// </summary>
        /// <param name="Name">A path string that may contain environment variables.</param>
        /// <returns>The fully qualified path with environment variables expanded.</returns>
        public static string ExpandFullPath(string Name)
        {
            string value = Environment.ExpandEnvironmentVariables(Name);

            value = Path.GetFullPath(value);

            return value;
        }

        /// <summary>
        /// Retrieves the system environment variables as a dictionary of key-value pairs.
        /// </summary>
        /// <remarks>The environment block is cached after the first retrieval. Subsequent calls return
        /// the cached dictionary.</remarks>
        /// <returns>A dictionary containing the system environment variables.</returns>
        public static Dictionary<string, string> GetSystemEnvironmentBlock()
        {
            if (EnvironmentBlock.Count == 0)
            {
                if (PInvoke.CreateEnvironmentBlock(out var block, null, false))
                {
                    char* offset = (char*)block;

                    while (*offset != '\0')
                    {
                        string variable = new string(offset);

                        if (string.IsNullOrWhiteSpace(variable))
                            break;

                        ReadOnlySpan<char> varSpan = variable.AsSpan();
                        int eqIndex = varSpan.IndexOf('=');

                        if (eqIndex != -1)
                        {
                            string key = varSpan[..eqIndex].Trim().ToString();
                            string val = varSpan[(eqIndex + 1)..].Trim().ToString();
                            EnvironmentBlock.Add(key, val);
                        }
                        else
                        {
                            EnvironmentBlock.Add(variable.Trim(), string.Empty);
                        }

                        offset += variable.Length + 1;
                    }

                    PInvoke.DestroyEnvironmentBlock(block);
                }
            }

            return EnvironmentBlock;
        }

        /// <summary>
        /// Generates a build log file name based on the solution name, build configuration, and platform.
        /// </summary>
        /// <param name="Solution">The path to the solution file.</param>
        /// <param name="Platform">The target platform for the build.</param>
        /// <param name="Flags">The build flags indicating the configuration.</param>
        /// <returns>A string representing the build log file name.</returns>
        public static string GetBuildLogPath(string Solution, string Platform, BuildFlags Flags)
        {
            return $"{Path.GetFileNameWithoutExtension(Solution)}{(Flags.HasFlag(BuildFlags.BuildDebug) ? "Debug" : "Release")}{Platform}";
        }

        /// <summary>
        /// Converts a FILETIME structure to an equivalent DateTime value.
        /// </summary>
        /// <param name="FileTime">The FILETIME structure to convert.</param>
        /// <returns>A DateTime value that represents the same point in time as the specified FILETIME.</returns>
        public static DateTime FileTimeToDateTime(this System.Runtime.InteropServices.ComTypes.FILETIME FileTime)
        {
            long fileTime = ((long)FileTime.dwHighDateTime << 32) + FileTime.dwLowDateTime;

            return DateTime.FromFileTime(fileTime);
        }

        /// <summary>
        /// Converts a FILETIME structure to a 64-bit file time value representing the number of 100-nanosecond
        /// intervals since January 1, 1601 (UTC).
        /// </summary>
        /// <param name="FileTime">The FILETIME structure to convert.</param>
        /// <returns>A 64-bit signed integer representing the file time.</returns>
        public static long FileTimeFromFileTime(this System.Runtime.InteropServices.ComTypes.FILETIME FileTime)
        {
            long fileTime = ((long)FileTime.dwHighDateTime << 32) + FileTime.dwLowDateTime;

            return fileTime;
        }

        /// <summary>
        /// Returns the CMake platform string for the specified toolchain.
        /// </summary>
        /// <param name="toolchain">The toolchain identifier.</param>
        /// <returns>The platform string (e.g., Win32, x64, ARM64).</returns>
        /// <exception cref="ArgumentException">Thrown if the toolchain is unsupported.</exception>
        public static string CMakeGetPlatform(string toolchain) => toolchain switch
        {
            "msvc-x86" or "clang-msvc-x86" => "Win32",
            "msvc-amd64" or "clang-msvc-amd64" => "x64",
            "msvc-arm64" or "clang-msvc-arm64" => "ARM64",
            _ => throw new ArgumentException("Unsupported toolchain")
        };

        /// <summary>
        /// Gets the CMake platform name corresponding to the specified build toolchain.
        /// </summary>
        /// <param name="Toolchain">The build toolchain for which to retrieve the CMake platform name.</param>
        /// <returns>The CMake platform name associated with the specified toolchain.</returns>
        /// <exception cref="ArgumentException">Thrown when the specified toolchain is not supported.</exception>
        public static string CMakeGetPlatform(BuildToolchain Toolchain) => Toolchain switch
        {
            BuildToolchain.MsvcX86 or BuildToolchain.ClangMsvcX86 => "Win32",
            BuildToolchain.MsvcAmd64 or BuildToolchain.ClangMsvcAmd64 => "x64",
            BuildToolchain.MsvcArm64 or BuildToolchain.ClangMsvcArm64 => "ARM64",
            _ => throw new ArgumentException("Unsupported toolchain")
        };

        /// <summary>
        /// Determines whether the specified toolchain targets the x86 architecture.
        /// </summary>
        /// <param name="Toolchain">The build toolchain to evaluate.</param>
        /// <returns>true if the toolchain targets x86; otherwise, false.</returns>
        public static bool IsX86Toolchain(BuildToolchain Toolchain)
        {
            return Toolchain == BuildToolchain.MsvcX86 || Toolchain == BuildToolchain.ClangMsvcX86;
        }

        /// <summary>
        /// Determines whether the specified toolchain targets the AMD64 architecture.
        /// </summary>
        /// <param name="Toolchain">The build toolchain to evaluate.</param>
        /// <returns>true if the toolchain targets AMD64; otherwise, false.</returns>
        public static bool IsAmd64Toolchain(BuildToolchain Toolchain)
        {
            return Toolchain == BuildToolchain.MsvcAmd64 || Toolchain == BuildToolchain.ClangMsvcAmd64;
        }

        /// <summary>
        /// Determines whether the specified toolchain targets the ARM64 architecture.
        /// </summary>
        /// <param name="Toolchain">The build toolchain to evaluate.</param>
        /// <returns>true if the toolchain targets ARM64; otherwise, false.</returns>
        public static bool IsArm64Toolchain(BuildToolchain Toolchain)
        {
            return Toolchain == BuildToolchain.MsvcArm64 || Toolchain == BuildToolchain.ClangMsvcArm64;
        }

        /// <summary>
        /// Returns the string representation of the specified build toolchain.
        /// </summary>
        /// <param name="Toolchain">The build toolchain to convert.</param>
        /// <returns>A string representing the specified build toolchain.</returns>
        /// <exception cref="ArgumentException">Thrown when the specified toolchain is not supported.</exception>
        public static string GetToolchainString(BuildToolchain Toolchain)
        {
            switch (Toolchain)
            {
            case BuildToolchain.MsvcX86:
                return "msvc-x86";
            case BuildToolchain.MsvcAmd64:
                return "msvc-amd64";
            case BuildToolchain.MsvcArm64:
                return "msvc-arm64";
            case BuildToolchain.ClangMsvcX86:
                return "clang-msvc-x86";
            case BuildToolchain.ClangMsvcAmd64:
                return "clang-msvc-amd64";
            case BuildToolchain.ClangMsvcArm64:
                return "clang-msvc-arm64";
            }

            throw new ArgumentException("Unsupported toolchain");
        }

        /// <summary>
        /// Parses a string to determine the corresponding build toolchain.
        /// </summary>
        /// <param name="s">The string representation of the build toolchain.</param>
        /// <returns>A value of the BuildToolchain enumeration that matches the specified string, or ClangMsvcAmd64 if the string
        /// is null, empty, or unrecognized.</returns>
        public static BuildToolchain GetToolchainFromString(string s)
        {
            if (string.IsNullOrWhiteSpace(s))
                return BuildToolchain.ClangMsvcAmd64;

            if (s.Equals("msvc-x86", StringComparison.OrdinalIgnoreCase))
                return BuildToolchain.MsvcX86;

            if (s.Equals("msvc-amd64", StringComparison.OrdinalIgnoreCase))
                return BuildToolchain.MsvcAmd64;

            if (s.Equals("msvc-arm64", StringComparison.OrdinalIgnoreCase))
                return BuildToolchain.MsvcArm64;

            if (s.Equals("clang-msvc-x86", StringComparison.OrdinalIgnoreCase))
                return BuildToolchain.ClangMsvcX86;

            if (s.Equals("clang-msvc-amd64", StringComparison.OrdinalIgnoreCase))
                return BuildToolchain.ClangMsvcAmd64;

            if (s.Equals("clang-msvc-arm64", StringComparison.OrdinalIgnoreCase))
                return BuildToolchain.ClangMsvcArm64;

            return BuildToolchain.ClangMsvcAmd64;
        }

        /// <summary>
        /// Parses a string to determine the appropriate build generator.
        /// </summary>
        /// <param name="s">The input string representing the build generator.</param>
        /// <returns>A value of the BuildGenerator enumeration corresponding to the input string.</returns>
        public static BuildGenerator GetGeneratorFromString(string s)
        {
            if (string.IsNullOrWhiteSpace(s))
                return BuildGenerator.Ninja;

            if (s.Contains("ninja", StringComparison.OrdinalIgnoreCase))
                return BuildGenerator.Ninja;

            if (
                s.Contains("visual studio", StringComparison.OrdinalIgnoreCase) ||
                s.Contains("visualstudio", StringComparison.OrdinalIgnoreCase)
                )
            {
                return BuildGenerator.VisualStudio;
            }

            return BuildGenerator.Ninja;
        }

        /// <summary>
        /// Returns the string representation of the specified build generator.
        /// </summary>
        /// <param name="gen">The build generator to convert to a string.</param>
        /// <param name="original">The original string to use for unknown generators. Defaults to null.</param>
        /// <returns>A string representing the build generator, or the original string if the generator is unknown.</returns>
        public static string GetGeneratorString(BuildGenerator gen, string original = null)
        {
            return gen switch
            {
                BuildGenerator.Ninja => "Ninja",
                BuildGenerator.VisualStudio => "Visual Studio",
                _ => original ?? string.Empty
            };
        }
    }

    /// <summary>
    /// Represents a request to update build information, including identifiers, version details, and associated binary
    /// and setup metadata.
    /// </summary>
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

        /// <summary>
        /// Serializes the update request to a UTF-8 encoded JSON byte array.
        /// </summary>
        /// <returns>A byte array containing the JSON representation of the request.</returns>
        public byte[] SerializeToBytes()
        {
            return JsonSerializer.SerializeToUtf8Bytes(this, BuildUpdateRequestContext.Default.BuildUpdateRequest);
        }
    }

    /// <summary>
    /// Represents a request to create a new release on GitHub, including release tag, target branch, name, description,
    /// and release options.
    /// </summary>
    /// <remarks>Provides properties for configuring release details such as draft status, prerelease status,
    /// and automatic release note generation. Supports serialization to JSON and UTF-8 byte arrays.</remarks>
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
    }

    /// <summary>
    /// Represents a GitHub release as returned by the GitHub API.
    /// </summary>
    public class GithubReleasesResponse
    {
        [JsonPropertyName("id")]
        public ulong ReleaseId { get; init; }

        [JsonPropertyName("tag_name")]
        public string TagName { get; init; }

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
    }

    /// <summary>
    /// Represents the response from a GitHub release query.
    /// </summary>
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
    }

    /// <summary>
    /// Represents a GitHub release asset as returned by the GitHub API.
    /// </summary>
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

                if (this.Digest.AsSpan().IndexOf(':') is var idx && idx >= 0)
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

                if (this.Digest.AsSpan().IndexOf(':') is var idx && idx >= 0)
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

    /// <summary>
    /// Represents the author or committer information of a GitHub commit.
    /// </summary>
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

    /// <summary>
    /// Represents the commit details of a GitHub commit.
    /// </summary>
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

    /// <summary>
    /// Represents a GitHub commit as returned by the GitHub API.
    /// </summary>
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

    /// <summary>
    /// Represents the line change statistics of a GitHub commit.
    /// </summary>
    public class GithubCommitStats
    {
        [JsonPropertyName("total")]
        public ulong Total { get; init; }

        [JsonPropertyName("additions")]
        public ulong Additions { get; init; }

        [JsonPropertyName("deletions")]
        public ulong Deletions { get; init; }
    }

    /// <summary>
    /// Represents the tree referenced by a GitHub commit.
    /// </summary>
    public class GithubCommitTree
    {
        [JsonPropertyName("sha")]
        public string Sha { get; init; }

        [JsonPropertyName("url")]
        public string Url { get; init; }
    }

    /// <summary>
    /// Represents the signature verification details of a GitHub commit.
    /// </summary>
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

    /// <summary>
    /// Represents the head commit of a GitHub Actions workflow run.
    /// </summary>
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

    /// <summary>
    /// Represents the head repository of a GitHub Actions workflow run.
    /// </summary>
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

    /// <summary>
    /// Represents a workflow referenced by a GitHub Actions workflow run.
    /// </summary>
    public class GithubReferencedWorkflow
    {
        [JsonPropertyName("path")]
        public string Path { get; set; }

        [JsonPropertyName("sha")]
        public string Sha { get; set; }

        //[JsonPropertyName("ref")]
        //public string @ref { get; set; }
    }

    /// <summary>
    /// Represents a GitHub repository as returned by the GitHub API.
    /// </summary>
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

    /// <summary>
    /// Represents a GitHub Actions workflow run as returned by the GitHub API.
    /// </summary>
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

    /// <summary>
    /// Represents the file details returned by a SourceForge upload.
    /// </summary>
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

    /// <summary>
    /// Represents the response from a SourceForge upload request.
    /// </summary>
    public class SourceForgeUploadResponse
    {
        [JsonPropertyName("result")]
        public SourceForgeResponseData Result { get; init; }
    }

    /// <summary>
    /// Represents the response from a VirusTotal large file upload URL request.
    /// </summary>
    public class VirusTotalLargeUploadResponse
    {
        [JsonPropertyName("data")]
        public string data { get; init; }
    }

    /// <summary>
    /// Represents the links associated with a VirusTotal analysis.
    /// </summary>
    public class VirusTotalAnalysisLinksResponse
    {
        [JsonPropertyName("self")]
        public string self { get; init; }
    }

    /// <summary>
    /// Represents the data object of a VirusTotal analysis response.
    /// </summary>
    public class VirusTotalAnalysisDataResponse
    {
        [JsonPropertyName("type")]
        public string type { get; init; }

        [JsonPropertyName("id")]
        public string id { get; init; }

        [JsonPropertyName("links")]
        public VirusTotalAnalysisLinksResponse links { get; init; }
    }

    /// <summary>
    /// Represents the response from a VirusTotal file analysis request.
    /// </summary>
    public class VirusTotalAnalysisResponse
    {
        [JsonPropertyName("data")]
        public VirusTotalAnalysisDataResponse data { get; init; }
    }

    /// <summary>
    /// Represents a SourceLink document mapping local source paths to remote URLs.
    /// </summary>
    public class SourceLink
    {
        [JsonPropertyName("documents")]
        public Dictionary<string, string> Documents { get; init; }
    }

    /// <summary>
    /// Represents the response from a NuGet flat container package versions query.
    /// </summary>
    public class NugetFlatContainerResponse
    {
        [JsonPropertyName("versions")]
        public List<string> Versions { get; init; }
    }


    [JsonSerializable(typeof(BuildUpdateRequest))]
    [JsonSourceGenerationOptions(DefaultIgnoreCondition = JsonIgnoreCondition.WhenWritingNull, GenerationMode = JsonSourceGenerationMode.Default)]
    public partial class BuildUpdateRequestContext : JsonSerializerContext;

    [JsonSerializable(typeof(GithubActionRun))]
    [JsonSerializable(typeof(GithubAssetsResponse))]
    [JsonSerializable(typeof(GithubCommitResponse))]
    [JsonSerializable(typeof(GithubReleasesRequest))]
    [JsonSerializable(typeof(GithubReleasesResponse))]
    [JsonSerializable(typeof(GithubReleaseQueryResponse))]
    [JsonSerializable(typeof(SourceLink))]
    [JsonSerializable(typeof(Dictionary<string, JsonElement>))]
    [JsonSourceGenerationOptions(DefaultIgnoreCondition = JsonIgnoreCondition.WhenWritingNull, GenerationMode = JsonSourceGenerationMode.Default)]
    public partial class GithubResponseContext : JsonSerializerContext;

    [JsonSerializable(typeof(SourceForgeUploadResponse))]
    [JsonSourceGenerationOptions(DefaultIgnoreCondition = JsonIgnoreCondition.WhenWritingNull, GenerationMode = JsonSourceGenerationMode.Default)]
    public partial class SourceForgeUploadResponseContext : JsonSerializerContext;

    [JsonSerializable(typeof(VirusTotalLargeUploadResponse))]
    [JsonSerializable(typeof(VirusTotalAnalysisResponse))]
    [JsonSourceGenerationOptions(DefaultIgnoreCondition = JsonIgnoreCondition.WhenWritingNull, GenerationMode = JsonSourceGenerationMode.Default)]
    public partial class VirusTotalResponseContext : JsonSerializerContext;

    [JsonSourceGenerationOptions(DefaultIgnoreCondition = JsonIgnoreCondition.WhenWritingNull, GenerationMode = JsonSourceGenerationMode.Default)]
    [JsonSerializable(typeof(Dictionary<string, string>))]
    public partial class DictionarySerializerContext : JsonSerializerContext;

    /// <summary>
    /// Provides extension methods for formatting numeric values as human-readable file sizes.
    /// </summary>
    /// <remarks>The methods in this class convert numeric values representing byte counts into formatted
    /// strings using common size units such as kilobytes (Kb), megabytes (Mb), gigabytes (Gb), and terabytes (Tb).
    /// These extensions are intended to simplify displaying file sizes in user interfaces or logs. All methods are
    /// thread-safe and can be used with integer and long values, including unsigned long. The class is static and
    /// cannot be instantiated.</remarks>
    public static class Extensions
    {
        private const long OneKb = 1024;
        private const long OneMb = OneKb * 1024;
        private const long OneGb = OneMb * 1024;
        private const long OneTb = OneGb * 1024;

        /// <summary>
        /// Formats a byte count as a human-readable size string.
        /// </summary>
        /// <param name="value">The byte count.</param>
        /// <param name="decimalPlaces">The number of decimal places to round to.</param>
        /// <returns>The formatted size string.</returns>
        public static string ToPrettySize(this int value, int decimalPlaces = 0)
        {
            return ((ulong)value).ToPrettySize(decimalPlaces);
        }

        /// <summary>
        /// Formats a byte count as a human-readable size string.
        /// </summary>
        /// <param name="value">The byte count.</param>
        /// <param name="decimalPlaces">The number of decimal places to round to.</param>
        /// <returns>The formatted size string.</returns>
        public static string ToPrettySize(this long value, int decimalPlaces = 0)
        {
            return ((ulong)value).ToPrettySize(decimalPlaces);
        }

        /// <summary>
        /// Formats a byte count as a human-readable size string.
        /// </summary>
        /// <param name="value">The byte count.</param>
        /// <param name="decimalPlaces">The number of decimal places to round to.</param>
        /// <returns>The formatted size string.</returns>
        public static string ToPrettySize(this ulong value, int decimalPlaces = 0)
        {
            double asTb = Math.Round((double)value / OneTb, decimalPlaces);
            double asGb = Math.Round((double)value / OneGb, decimalPlaces);
            double asMb = Math.Round((double)value / OneMb, decimalPlaces);
            double asKb = Math.Round((double)value / OneKb, decimalPlaces);
            string chosenValue = asTb > 1 ? $"{asTb} Tb"
                : asGb > 1 ? $"{asGb} Gb"
                : asMb > 1 ? $"{asMb} Mb"
                : asKb > 1 ? $"{asKb} Kb"
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
        BuildCMake = 256,
        BuildAnalyze = 512,

        Debug = Build32bit | Build64bit | BuildArm64bit | BuildDebug | BuildApi | BuildVerbose,
        Release = Build32bit | Build64bit | BuildArm64bit | BuildRelease | BuildApi | BuildVerbose,
        All = Build32bit | Build64bit | BuildArm64bit | BuildDebug | BuildRelease | BuildApi | BuildVerbose,
    }

    /// <summary>
    /// Specifies the compiler toolchain and target architecture used for a build.
    /// </summary>
    public enum BuildToolchain
    {
        None,
        MsvcX86 = 1,
        MsvcAmd64 = 2,
        MsvcArm64 = 3,
        ClangMsvcX86 = 4,
        ClangMsvcAmd64 = 5,
        ClangMsvcArm64 = 6
    }

    /// <summary>
    /// Specifies the CMake generator used for a build.
    /// </summary>
    public enum BuildGenerator
    {
        Ninja,
        VisualStudio,
        Other
    }

    /// <summary>
    /// https://learn.microsoft.com/en-us/dotnet/csharp/whats-new/tutorials/interpolated-string-handler
    /// </summary>
    [InterpolatedStringHandler]
    public readonly ref struct LogInterpolatedStringHandler
    {
        private readonly StringBuilder _builder;
        public readonly bool Enabled;

        /// <summary>
        /// Initializes a new instance of the handler that always formats the message.
        /// </summary>
        /// <param name="literalLength">The number of literal characters in the interpolated string.</param>
        /// <param name="formattedCount">The number of formatted holes in the interpolated string.</param>
        /// <param name="enabled">Receives true, indicating the message is formatted.</param>
        public LogInterpolatedStringHandler(int literalLength, int formattedCount, out bool enabled)
        {
            this.Enabled = true;
            enabled = true;
            this._builder = new StringBuilder(literalLength);
        }

        /// <summary>
        /// Initializes a new instance of the handler that formats the message only when verbose output is enabled.
        /// </summary>
        /// <param name="literalLength">The number of literal characters in the interpolated string.</param>
        /// <param name="formattedCount">The number of formatted holes in the interpolated string.</param>
        /// <param name="Flags">The build flags used to determine whether verbose output is enabled.</param>
        /// <param name="enabled">Receives true if the message is formatted; otherwise, false.</param>
        public LogInterpolatedStringHandler(int literalLength, int formattedCount, BuildFlags Flags, out bool enabled)
        {
            this.Enabled = (Flags & BuildFlags.BuildVerbose) != 0;
            enabled = this.Enabled;
            this._builder = enabled ? new StringBuilder(literalLength) : null;
        }

        /// <summary>
        /// Appends a literal string to the message.
        /// </summary>
        /// <param name="s">The literal string.</param>
        public void AppendLiteral(string s)
        {
            _builder?.Append(s.AsSpan());
        }

        /// <summary>
        /// Appends a formatted value to the message.
        /// </summary>
        /// <typeparam name="T">The type of the value.</typeparam>
        /// <param name="t">The value to append.</param>
        public void AppendFormatted<T>(T t)
        {
            _builder?.Append(t?.ToString());
        }

        /// <summary>
        /// Appends a formatted value to the message using the specified format string.
        /// </summary>
        /// <typeparam name="T">The type of the value.</typeparam>
        /// <param name="t">The value to append.</param>
        /// <param name="format">The format string.</param>
        public void AppendFormatted<T>(T t, string format) where T : IFormattable
        {
            _builder?.Append(t?.ToString(format, null));
        }

        /// <summary>
        /// Gets the formatted message text.
        /// </summary>
        /// <returns>The formatted message, or an empty string if formatting was disabled.</returns>
        internal string GetFormattedText()
        {
            return _builder?.ToString() ?? string.Empty;
        }
    }

    /// <summary>
    /// Provides ANSI virtual terminal escape sequences for console text colors.
    /// </summary>
    public static class VT
    {
        /// <summary>
        /// Returns the escape sequence, or an empty string when output is redirected
        /// outside of CI (e.g. MSBuild Exec) so captured logs stay free of escape codes.
        /// Evaluated per access because <see cref="Build.BuildIntegration"/> is set after startup.
        /// </summary>
        private static string C(string Sequence)
        {
            return Console.IsOutputRedirected && !Build.BuildIntegration ? string.Empty : Sequence;
        }

        // Reset
        public static string RESET       => C("\x1b[0m");

        // Standard foreground colors
        public static string BLACK       => C("\x1b[30m");
        public static string RED         => C("\x1b[31m");
        public static string GREEN       => C("\x1b[32m");
        public static string YELLOW      => C("\x1b[33m");
        public static string BLUE        => C("\x1b[34m");
        public static string MAGENTA     => C("\x1b[35m");
        public static string CYAN        => C("\x1b[36m");
        public static string WHITE       => C("\x1b[37m");
        public static string GRAY        => C("\x1b[90m");

        // Bright foreground colors
        public static string BRIGHT_RED      => C("\x1b[91m");
        public static string BRIGHT_GREEN    => C("\x1b[92m");
        public static string BRIGHT_YELLOW   => C("\x1b[93m");
        public static string BRIGHT_BLUE     => C("\x1b[94m");
        public static string BRIGHT_MAGENTA  => C("\x1b[95m");
        public static string BRIGHT_CYAN     => C("\x1b[96m");
        public static string BRIGHT_WHITE    => C("\x1b[97m");

        // Background colors
        public static string BG_BLACK    => C("\x1b[40m");
        public static string BG_RED      => C("\x1b[41m");
        public static string BG_GREEN    => C("\x1b[42m");
        public static string BG_YELLOW   => C("\x1b[43m");
        public static string BG_BLUE     => C("\x1b[44m");
        public static string BG_MAGENTA  => C("\x1b[45m");
        public static string BG_CYAN     => C("\x1b[46m");
        public static string BG_WHITE    => C("\x1b[47m");

        // Bright background colors
        public static string BG_BRIGHT_BLACK     => C("\x1b[100m");
        public static string BG_BRIGHT_RED       => C("\x1b[101m");
        public static string BG_BRIGHT_GREEN     => C("\x1b[102m");
        public static string BG_BRIGHT_YELLOW    => C("\x1b[103m");
        public static string BG_BRIGHT_BLUE      => C("\x1b[104m");
        public static string BG_BRIGHT_MAGENTA   => C("\x1b[105m");
        public static string BG_BRIGHT_CYAN      => C("\x1b[106m");
        public static string BG_BRIGHT_WHITE     => C("\x1b[107m");

        // 256-color foreground
        public static string FROM256(int n) => C($"\x1b[38;5;{n}m");

        // 256-color background
        public static string BGFROM256(int n) => C($"\x1b[48;5;{n}m");

        // True-color (RGB)
        public static string RGB(int r, int g, int b) => C($"\x1b[38;2;{r};{g};{b}m");
        public static string BGRGB(int r, int g, int b) => C($"\x1b[48;2;{r};{g};{b}m");

        // Extended palette colors (256-color ANSI)
        public static string ORANGE => C("\u001b[38;5;208m");
        public static string PURPLE => C("\u001b[38;5;141m");
        public static string TEAL => C("\u001b[38;5;44m");
        public static string PINK => C("\u001b[38;5;213m");
        public static string LIME => C("\u001b[38;5;118m");
    }

    /// <summary>
    /// Represents an unmanaged, non-paged buffer of characters that is securely zeroed out after use.
    /// </summary>
    public sealed unsafe class SecureBuffer : IDisposable
    {
        private IntPtr _buffer;
        private nuint _regionSize;
        private readonly int _length;

        // NTSTATUS Constants
        private const int STATUS_SUCCESS = 0;

        // Memory Constants
        private const uint MEM_COMMIT = 0x00001000;
        private const uint MEM_RESERVE = 0x00002000;
        private const uint MEM_RELEASE = 0x00008000;
        private const uint PAGE_READWRITE = 0x04;
        private const uint LOCK_VM_IN_WSL = 1;

        public int Length => this._length;

        public ReadOnlySpan<char> Span
        {
            get
            {
                ObjectDisposedException.ThrowIf(this._buffer == IntPtr.Zero, this);
                return new ReadOnlySpan<char>((void*)this._buffer, this._length);
            }
        }

        internal Span<char> WritableSpan
        {
            get
            {
                ObjectDisposedException.ThrowIf(this._buffer == IntPtr.Zero, this);
                return new Span<char>((void*)this._buffer, this._length);
            }
        }

        /// <summary>
        /// Allocates a new secure buffer and attempts to lock it into the working set.
        /// </summary>
        /// <param name="length">The number of characters the buffer holds.</param>
        /// <exception cref="ArgumentOutOfRangeException">Thrown when <paramref name="length"/> is negative.</exception>
        /// <exception cref="Win32Exception">Thrown when the memory allocation fails.</exception>
        public SecureBuffer(int length)
        {
            if (length < 0)
                throw new ArgumentOutOfRangeException(nameof(length));

            this._length = length;
            if (length == 0)
                return;

            nuint size = (nuint)(length * sizeof(char));
            void* baseAddress = PInvoke.VirtualAlloc(
                null,
                size,
                VIRTUAL_ALLOCATION_TYPE.MEM_COMMIT | VIRTUAL_ALLOCATION_TYPE.MEM_RESERVE,
                PAGE_PROTECTION_FLAGS.PAGE_READWRITE
                );

            if (baseAddress == null)
            {
                throw new Win32Exception(Marshal.GetLastWin32Error());
            }

            this._buffer = (IntPtr)baseAddress;
            this._regionSize = size;

            // Attempt to lock memory in the working set to prevent it from being written to the pagefile.
            // We do not throw on failure, as the process might lack the necessary quotas or privileges, 
            // making this a best-effort security enhancement.
            PInvoke.VirtualLock(baseAddress, size); // LOCK_VM_IN_WSL
        }

        /// <summary>
        /// Zeroes, unlocks, and releases the buffer memory.
        /// </summary>
        public void Dispose()
        {
            IntPtr buffer = Interlocked.Exchange(ref this._buffer, IntPtr.Zero);

            if (buffer != IntPtr.Zero)
            {
                // CryptographicOperations.ZeroMemory is preserved here instead of P/Invoking RtlSecureZeroMemory
                // because it uses the exact same JIT/AOT intrinsic protections without the P/Invoke boundary overhead.
                CryptographicOperations.ZeroMemory(new Span<byte>((void*)buffer, (int)this._regionSize));

                PInvoke.VirtualUnlock((void*)buffer, this._regionSize); // LOCK_VM_IN_WSL
                PInvoke.VirtualFree((void*)buffer, this._regionSize, VIRTUAL_FREE_TYPE.MEM_RELEASE);
            }
        }
    }

    public static partial class Utils
    {
        /// <summary>
        /// Reads the contents of a text file into a <see cref="SecureBuffer"/>, detecting the encoding from the byte order mark.
        /// </summary>
        /// <remarks>The intermediate byte array is zeroed before the method returns. Files without a byte order mark are decoded as UTF-8.</remarks>
        /// <param name="FileName">The path of the file to read.</param>
        /// <returns>A secure buffer containing the file text, or null if the file does not exist.</returns>
        public static SecureBuffer ReadAllTextSecure(string FileName)
        {
            if (!File.Exists(FileName))
                return null;

            byte[] fileBytes = File.ReadAllBytes(FileName);

            try
            {
                Encoding encoding = Utils.UTF8NoBOM;
                int preambleLength = 0;
                ReadOnlySpan<byte> bytes = fileBytes;
                ReadOnlySpan<byte> start1 = [0x00, 0x00, 0xFE, 0xFF];
                ReadOnlySpan<byte> start2 = [0xFF, 0xFE, 0x00, 0x00];
                ReadOnlySpan<byte> start3 = [0xEF, 0xBB, 0xBF];
                ReadOnlySpan<byte> start4 = [0xFE, 0xFF];
                ReadOnlySpan<byte> start5 = [0xFF, 0xFE];

                if (bytes.StartsWith(start1))
                {
                    encoding = new UTF32Encoding(bigEndian: true, byteOrderMark: true, throwOnInvalidCharacters: true);
                    preambleLength = 4;
                }
                else if (bytes.StartsWith(start2))
                {
                    encoding = new UTF32Encoding(bigEndian: false, byteOrderMark: true, throwOnInvalidCharacters: true);
                    preambleLength = 4;
                }
                else if (bytes.StartsWith(start3))
                {
                    preambleLength = 3;
                }
                else if (bytes.StartsWith(start4))
                {
                    encoding = new UnicodeEncoding(bigEndian: true, byteOrderMark: true, throwOnInvalidBytes: true);
                    preambleLength = 2;
                }
                else if (bytes.StartsWith(start5))
                {
                    encoding = new UnicodeEncoding(bigEndian: false, byteOrderMark: true, throwOnInvalidBytes: true);
                    preambleLength = 2;
                }

                bytes = bytes[preambleLength..];
                var secureBuffer = new SecureBuffer(encoding.GetCharCount(bytes));

                try
                {
                    encoding.GetChars(bytes, secureBuffer.WritableSpan);
                    return secureBuffer;
                }
                catch
                {
                    secureBuffer.Dispose();
                    throw;
                }
            }
            finally
            {
                CryptographicOperations.ZeroMemory(fileBytes);
            }
        }

        /// <summary>
        /// Calculates the maximum number of bytes required to form URL encode the specified number of characters.
        /// </summary>
        /// <param name="CharacterCount">The number of characters to encode.</param>
        /// <returns>The maximum number of encoded bytes.</returns>
        public static int GetFormUrlEncodedMaxByteCount(int CharacterCount)
        {
            return checked(Utils.UTF8NoBOM.GetMaxByteCount(CharacterCount) * 3);
        }

        /// <summary>
        /// Form URL encodes the specified characters as UTF-8 into the destination buffer.
        /// </summary>
        /// <param name="Value">The characters to encode.</param>
        /// <param name="Destination">The buffer that receives the encoded bytes.</param>
        /// <returns>The number of bytes written to <paramref name="Destination"/>.</returns>
        /// <exception cref="ArgumentException">Thrown when <paramref name="Destination"/> is too small.</exception>
        public static int WriteFormUrlEncoded(ReadOnlySpan<char> Value, Span<byte> Destination)
        {
            int byteCount = Utils.UTF8NoBOM.GetByteCount(Value);
            byte[] utf8Buffer = ArrayPool<byte>.Shared.Rent(byteCount);

            try
            {
                int bytesWritten = Utils.UTF8NoBOM.GetBytes(Value, utf8Buffer);
                ReadOnlySpan<byte> utf8Value = utf8Buffer.AsSpan(0, bytesWritten);
                int encodedLength = 0;

                foreach (byte value in utf8Value)
                {
                    encodedLength = checked(encodedLength + (IsFormUrlUnreserved(value) ? 1 : 3));
                }

                if (Destination.Length < encodedLength)
                    throw new ArgumentException("The destination is too small.", nameof(Destination));

                ReadOnlySpan<byte> hexDigits = "0123456789ABCDEF"u8;
                int destinationIndex = 0;

                foreach (byte value in utf8Value)
                {
                    if (IsFormUrlUnreserved(value))
                    {
                        Destination[destinationIndex++] = value;
                    }
                    else
                    {
                        Destination[destinationIndex++] = (byte)'%';
                        Destination[destinationIndex++] = hexDigits[value >> 4];
                        Destination[destinationIndex++] = hexDigits[value & 0xf];
                    }
                }

                return destinationIndex;
            }
            finally
            {
                ArrayPool<byte>.Shared.Return(utf8Buffer, clearArray: true);
            }
        }

        /// <summary>
        /// Determines whether the specified byte is an unreserved character that does not require percent encoding.
        /// </summary>
        /// <param name="Value">The byte to test.</param>
        /// <returns>True if the byte is unreserved; otherwise, false.</returns>
        private static bool IsFormUrlUnreserved(byte Value)
        {
            return
                Value >= (byte)'A' && Value <= (byte)'Z' ||
                Value >= (byte)'a' && Value <= (byte)'z' ||
                Value >= (byte)'0' && Value <= (byte)'9' ||
                Value == (byte)'-' ||
                Value == (byte)'_' ||
                Value == (byte)'.' ||
                Value == (byte)'~';
        }
    }
}
