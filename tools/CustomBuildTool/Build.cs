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
    public static class Build
    {
        private static DateTime TimeStart;
        public static bool BuildCanary;
        public static bool BuildIntegration;
        public static bool BuildRedirectOutput;
        public static bool BuildToolsDebug;
        public static bool HaveArm64BuildTools = true;
        public static string BuildOutputFolder = string.Empty;
        public static string BuildWorkingFolder = string.Empty;
        public static string BuildCommitBranch = "orphan";
        public static string BuildCommitHash = new('0', 40);
        public static string BuildShortVersion = "0.0.0";
        public static string BuildLongVersion = "0.0.0.0";
        public static string BuildSourceLink = string.Empty;
        public static string BuildSimdExtensions = string.Empty;
        public static string BuildVersionMajor = "0";
        public static string BuildVersionMinor = "0";

        /// <summary>
        /// Initializes the build environment, project solution and build arguments.
        /// </summary>
        /// <returns>True if the build environment is successfully initialized; otherwise, false.</returns>
        public static bool InitializeBuildEnvironment()
        {
            Win32.SetErrorMode();
            Win32.SetBasePriority();

            Console.InputEncoding = Utils.UTF8NoBOM;
            Console.OutputEncoding = Utils.UTF8NoBOM;

            if (!Utils.SetCurrentDirectoryParent("SystemInformer.sln"))
            {
                Console.WriteLine($"{VT.RED}Unable to find project solution.{VT.RESET}");
                return false;
            }

            Build.BuildWorkingFolder = Environment.CurrentDirectory;
            Build.BuildOutputFolder = Utils.GetOutputDirectoryPath("\\build\\output");

            if (!Build.InitializeBuildArguments())
            {
                Console.WriteLine($"{VT.RED}Unable to initialize build arguments.{VT.RESET}");
                return false;
            }

            return true;
        }

        /// <summary>
        /// Initializes build arguments by reading environment variables and setting build configuration flags.
        /// </summary>
        /// <remarks>
        /// This method configures build flags such as canary, debug, integration, and output redirection
        /// based on environment variables. It also sets versioning information, commit hash, branch name,
        /// and SIMD extensions if available. The method updates <see cref="BuildWorkingFolder"/> and
        /// <see cref="BuildOutputFolder"/> as needed, and computes short and long version strings.
        /// </remarks>
        /// <returns>
        /// <c>true</c> if build arguments are successfully initialized; otherwise, <c>false</c>.
        /// </returns>
        public static bool InitializeBuildArguments()
        {
            if (Win32.GetEnvironmentVariableSpan("SYSTEM_BUILD", out ReadOnlySpan<char> build_definition))
            {
                if (build_definition.Equals("canary", StringComparison.OrdinalIgnoreCase))
                {
                    Build.BuildCanary = true;
                    Program.PrintColorMessage("[CANARY BUILD]", ConsoleColor.Cyan);
                }
            }

            if (Win32.GetEnvironmentVariableSpan("SYSTEM_DEBUG", out ReadOnlySpan<char> build_debug))
            {
                if (build_debug.Equals("true", StringComparison.OrdinalIgnoreCase))
                {
                    Build.BuildToolsDebug = true;
                    Program.PrintColorMessage("[DEBUG BUILD]", ConsoleColor.Cyan);
                }
            }

            if (Win32.HasEnvironmentVariable("TF_BUILD")) // Console.IsOutputRedirected
            {
                if (!BuildDevOps.BuildQueryQueueTime(out Build.TimeStart))
                    return false;

                Build.BuildIntegration = true;
            }
            else if (Win32.HasEnvironmentVariable("GITHUB_ACTIONS")) // Console.IsOutputRedirected
            {
                if (!BuildGithub.BuildQueryQueueTime(out Build.TimeStart))
                    return false;

                Build.BuildIntegration = true;
            }
            else
            {
                Build.TimeStart = DateTime.UtcNow;
            }

            if (Win32.GetEnvironmentVariable("BUILD_REDIRECTOUTPUT", out string build_output))
            {
                Build.BuildRedirectOutput = build_output.Equals("true", StringComparison.OrdinalIgnoreCase);
            }

            if (Win32.GetEnvironmentVariable("BUILD_SIMD", out string build_simd))
            {
                Build.BuildSimdExtensions = build_simd;
            }

            if (Win32.GetEnvironmentVariable("BUILD_MAJORVERSION", out string build_major))
            {
                Build.BuildVersionMajor = build_major;
            }

            if (Win32.GetEnvironmentVariable("BUILD_MINORVERSION", out string build_minor))
            {
                Build.BuildVersionMinor = build_minor;
            }

            {
                if (Win32.GetEnvironmentVariable("BUILD_SOURCEVERSION", out var build_source))
                {
                    Build.BuildCommitHash = build_source;
                }

                if (string.IsNullOrWhiteSpace(Build.BuildCommitHash) && !string.IsNullOrWhiteSpace(Utils.GetGitFilePath()))
                {
                    Build.BuildCommitHash = Utils.ExecuteGitCommand(Build.BuildWorkingFolder, "rev-parse HEAD");
                }

                if (Win32.GetEnvironmentVariable("BUILD_SOURCEBRANCHNAME", out var build_branch))
                {
                    Build.BuildCommitBranch = build_branch;
                }

                if (string.IsNullOrWhiteSpace(Build.BuildCommitBranch) && !string.IsNullOrWhiteSpace(Utils.GetGitFilePath()))
                {
                    Build.BuildCommitBranch = Utils.ExecuteGitCommand(Build.BuildWorkingFolder, "branch --show-current");
                }
            }

            if (!string.IsNullOrWhiteSpace(Build.BuildCommitHash) && !string.IsNullOrWhiteSpace(Build.BuildVersionMajor))
            {
                Build.BuildShortVersion = $"{Build.BuildVersionMajor}.{Build.BuildVersionMinor}.{Build.BuildVersionBuild()}";
                Build.BuildLongVersion = $"{Build.BuildVersionMajor}.{Build.BuildVersionMinor}.{Build.BuildVersionBuild()}.{Build.BuildVersionRevision()}";
            }

            return true;
        }

        /// <summary>
        /// Sets up the build environment and prints build information if requested.
        /// </summary>
        /// <param name="ShowBuildInfo">If true, prints build and environment information to the console.</param>
        public static void SetupBuildEnvironment(bool ShowBuildInfo)
        {
            if (ShowBuildInfo)
            {
                Program.PrintColorMessage("> Windows: ", ConsoleColor.DarkGray, false);
                Program.PrintColorMessage(Win32.GetKernelVersion(), ConsoleColor.Green);

                var instance = BuildVisualStudio.GetVisualStudioInstance();
                if (instance != null)
                {
                    Program.PrintColorMessage("> WindowsSDK: ", ConsoleColor.DarkGray, false);
                    Program.PrintColorMessage($"{Utils.GetWindowsSdkVersion()} ({instance.GetWindowsSdkFullVersion()})", ConsoleColor.Green, true);
                    Program.PrintColorMessage("> VisualStudio: ", ConsoleColor.DarkGray, false);
                    Program.PrintColorMessage(instance.Name, ConsoleColor.Green);
                    //Program.PrintColorMessage(Utils.GetVisualStudioVersion(), ConsoleColor.DarkGreen, true);
                    Build.HaveArm64BuildTools = instance.HasARM64BuildToolsComponents;
                }

                Program.PrintColorMessage("> SystemInformer: ", ConsoleColor.DarkGray, false);
                Program.PrintColorMessage(Build.BuildLongVersion, ConsoleColor.Green, false);

                if (!string.IsNullOrWhiteSpace(Build.BuildCommitHash))
                {
                    Program.PrintColorMessage(" (", ConsoleColor.DarkGray, false);

                    if (Build.BuildIntegration)
                    {
                        Program.PrintColorMessage(Build.BuildCommitHash[..8], ConsoleColor.Blue, false);
                    }
                    else
                    {
                        Program.PrintColorMessage(Program.CreateConsoleHyperlink(
                            $"https://github.com/winsiderss/systeminformer/commit/{Build.BuildCommitHash}",
                            Build.BuildCommitHash[..8]), ConsoleColor.Blue, false);
                    }

                    Program.PrintColorMessage(")", ConsoleColor.DarkGray, false);
                }

                if (!string.IsNullOrWhiteSpace(Build.BuildCommitBranch) && !Build.BuildCommitBranch.Equals("master", StringComparison.OrdinalIgnoreCase))
                {
                    Program.PrintColorMessage(" [", ConsoleColor.DarkGray, false);
                    Program.PrintColorMessage(Build.BuildCommitBranch, ConsoleColor.Blue, false);
                    Program.PrintColorMessage("]", ConsoleColor.DarkGray, false);
                }

                Program.PrintColorMessage(Environment.NewLine, ConsoleColor.DarkGray);
            }
        }

        /// <summary>
        /// Gets the elapsed time since the build started, formatted as [mm:ss].
        /// </summary>
        /// <returns>
        /// A string representing the elapsed time since <see cref="TimeStart"/> in the format [mm:ss].
        /// </returns>
        public static string BuildTimeSpan()
        {
            return $"[{DateTime.UtcNow - Build.TimeStart:mm\\:ss}] ";
        }

        /// <summary>
        /// Gets the build version build number, formatted as YYDDD, where YY is the last two digits of the year and DDD is the day of the year.
        /// </summary>
        /// <returns>
        /// A string representing the build number in the format YYDDD.
        /// </returns>
        public static string BuildVersionBuild()
        {
            // Extract the last two digits of the year. For example, if the year is 2023, (Year % 100) will result in 23.
            var first = (TimeStart.Year % 100).ToString();
            // Extract the day of the year (1 to 365 or 366 in a leap year). Padding with leading zeros if necessary.
            // For example, if the day of the year is 5, it will be converted to "005".
            var second = TimeStart.DayOfYear.ToString("D3");
            // Format the strings: 23005
            return string.Concat([first, second]);
        }

        /// <summary>
        /// Gets the build version revision number, formatted as HHMM, where HH is the hour (1-based) and MM is the minute.
        /// </summary>
        /// <returns>
        /// A string representing the revision number in the format HHMM.
        /// </returns>
        public static string BuildVersionRevision()
        {
            // Extract the hour of the day. For example, if the hour is 0|23, (Hour + 1) will result in 1|24.
            var first = (TimeStart.Hour + 1).ToString();
            // Extract the minute of the hour (0 to 59). Padding with leading zeros if necessary.
            // For example, if the minute of the hour is 5, it will be converted to "005".
            var second = TimeStart.Minute.ToString("D2");
            // Format the strings: 1005|24005
            return string.Concat([first, second]);
        }

        /// <summary>
        /// Gets the short hash of the current build commit.
        /// </summary>
        /// <returns>
        /// The first 8 characters of <see cref="BuildCommitHash"/>, or "00000000" if not available.
        /// </returns>
        public static string BuildHash()
        {
            return string.IsNullOrWhiteSpace(Build.BuildCommitHash) ? "00000000" : Build.BuildCommitHash[..8];
        }

        /// <summary>
        /// Gets the base directory for toolchain aware outputs based on the build flags.
        /// </summary>
        /// <param name="Flags">Build flags indicating whether to use CMake paths.</param>
        /// <returns>"bin-cmake" if BuildCMake flag is set; otherwise, "bin".</returns>
        public static string GetBuildBaseDirectory(BuildFlags Flags)
        {
            if (Flags.HasFlag(BuildFlags.BuildCMake))
            {
                return Path.Join([Build.BuildWorkingFolder, "\\bin-cmake"]);
            }
            else
            {
                return Path.Join([Build.BuildWorkingFolder, "\\bin"]);
            }
        }

        /// <summary>
        /// Gets the toolchain suffix for output filenames.
        /// </summary>
        /// <param name="Flags">Build flags indicating which toolchain to use.</param>
        /// <returns>"-clang" if BuildCMake flag is set; otherwise, empty string.</returns>
        public static string GetToolchainSuffix(BuildFlags Flags)
        {
            return Flags.HasFlag(BuildFlags.BuildCMake) ? "-clang" : string.Empty;
        }

        /// <summary>
        /// Gets the platform-specific output directory path.
        /// </summary>
        /// <param name="Flags">Build flags indicating which platform and build type to use.</param>
        /// <param name="platform">Platform identifier ("32", "64", or "ARM64").</param>
        /// <returns>Full path to the platform-specific output directory.</returns>
        public static string GetBuildOutputPath(BuildFlags Flags, string platform)
        {
            string basePath = GetBuildBaseDirectory(Flags);
            string config = Flags.HasFlag(BuildFlags.BuildDebug) ? "Debug" : "Release";

            return Path.Join([basePath, config + platform]);
        }

        /// <summary>
        /// Gets the build updated timestamp in ISO 8601 format, rounded to the hour.
        /// </summary>
        public static string BuildUpdated
        {
            get { return new DateTime(TimeStart.Year, TimeStart.Month, TimeStart.Day, TimeStart.Hour, 0, 0).ToString("o"); }
        }

        /// <summary>
        /// Gets the build date and time, rounded to the hour.
        /// </summary>
        public static DateTime BuildDateTime
        {
            get { return new DateTime(TimeStart.Year, TimeStart.Month, TimeStart.Day, TimeStart.Hour, 0, 0); }
        }

        /// <summary>
        /// Displays the total build time in minutes and seconds to the console.
        /// </summary>
        /// <remarks>
        /// This method calculates the elapsed time since the build started, formats it as minutes and seconds,
        /// and prints the result to the console using color-coded messages.
        /// </remarks>
        public static void ShowBuildStats()
        {
            TimeSpan buildTime = DateTime.UtcNow - Build.TimeStart;

            Program.PrintColorMessage($"{Environment.NewLine}Build Time: ", ConsoleColor.DarkGray, false);
            Program.PrintColorMessage(buildTime.TotalMinutes.ToString("0"), ConsoleColor.Green, false);
            Program.PrintColorMessage(" minute(s), ", ConsoleColor.DarkGray, false);
            Program.PrintColorMessage(buildTime.Seconds.ToString(), ConsoleColor.Green, false);
            Program.PrintColorMessage($" second(s) {Environment.NewLine + Environment.NewLine}", ConsoleColor.DarkGray, false);
        }

        /// <summary>
        /// Copies or deletes text files (README, COPYRIGHT, LICENSE) to or from build output folders based on flags.
        /// </summary>
        /// <param name="Update">If true, copies files; if false, deletes them.</param>
        /// <param name="Flags">Build flags indicating which configurations to process.</param>
        /// <returns>True if operation succeeds.</returns>
        public static bool CopyTextFiles(bool Update, BuildFlags Flags)
        {
            var architectures = new Dictionary<BuildFlags, string>
            {
                { BuildFlags.Build32bit, "Release32" },
                { BuildFlags.Build64bit, "Release64" },
                { BuildFlags.BuildArm64bit, "ReleaseARM64" }
            };

            string[] Build_Text_Files =
            [
                "README.txt",
                "COPYRIGHT.txt",
                "LICENSE.txt",
            ];

            string binPath = GetBuildBaseDirectory(Flags);

            if (!Flags.HasFlag(BuildFlags.BuildRelease))
                return true;

            foreach (string file in Build_Text_Files)
            {
                foreach (var kvp in architectures)
                {
                    if (Flags.HasFlag(kvp.Key))
                    {
                        string targetPath = Path.Join([binPath, $"\\{kvp.Value}\\", file]);
                        
                        if (Update)
                            Win32.CopyIfNewer(file, targetPath, Flags);
                        else
                            Win32.DeleteFile(targetPath, Flags);
                    }
                }
            }

            return true;
        }

        /// <summary>
        /// Copies Wow64-related files to appropriate output folders based on build flags.
        /// </summary>
        /// <param name="Flags">Build flags indicating which configurations to process.</param>
        /// <returns>True if operation succeeds.</returns>
        public static bool CopyWow64Files(BuildFlags Flags)
        {
            var configurations = new Dictionary<(BuildFlags config, BuildFlags arch), (string source, string target)>
            {
                { (BuildFlags.BuildDebug, BuildFlags.Build64bit), ("Debug32", "Debug64\\x86") },
                { (BuildFlags.BuildDebug, BuildFlags.BuildArm64bit), ("Debug32", "DebugARM64\\x86") },
                { (BuildFlags.BuildRelease, BuildFlags.Build64bit), ("Release32", "Release64\\x86") },
                { (BuildFlags.BuildRelease, BuildFlags.BuildArm64bit), ("Release32", "ReleaseARM64\\x86") }
            };

            string[] Build_Wow64_Files =
            [
                "SystemInformer.exe",
                "SystemInformer.pdb",
                "SystemInformer.sig",
                "plugins\\DotNetTools.dll",
                "plugins\\DotNetTools.pdb",
                "plugins\\DotNetTools.sig",
                "plugins\\ExtendedTools.dll",
                "plugins\\ExtendedTools.pdb",
                "plugins\\ExtendedTools.sig",
            ];

            string binPath = GetBuildBaseDirectory(Flags);

            foreach (string file in Build_Wow64_Files)
            {
                foreach (var kvp in configurations)
                {
                    if (Flags.HasFlag(kvp.Key.config) && Flags.HasFlag(kvp.Key.arch))
                    {
                        string sourcePath = Path.Join([binPath, $"\\{kvp.Value.source}\\", file]);
                        string targetPath = Path.Join([binPath, $"\\{kvp.Value.target}\\", file]);
                        Win32.CopyIfNewer(sourcePath, targetPath, Flags);
                    }
                }
            }

            return true;
        }

        /// <summary>
        /// Copies resource files to output folders for each build configuration.
        /// </summary>
        /// <param name="Flags">Build flags indicating which configurations to process.</param>
        /// <returns>True if operation succeeds.</returns>
        public static bool CopyResourceFiles(BuildFlags Flags)
        {
            var configurations = new Dictionary<(BuildFlags config, BuildFlags arch), string>
            {
                { (BuildFlags.BuildDebug, BuildFlags.Build32bit), "Debug32\\Resources" },
                { (BuildFlags.BuildDebug, BuildFlags.Build64bit), "Debug64\\Resources" },
                { (BuildFlags.BuildDebug, BuildFlags.BuildArm64bit), "DebugARM64\\Resources" },
                { (BuildFlags.BuildRelease, BuildFlags.Build32bit), "Release32\\Resources" },
                { (BuildFlags.BuildRelease, BuildFlags.Build64bit), "Release64\\Resources" },
                { (BuildFlags.BuildRelease, BuildFlags.BuildArm64bit), "ReleaseARM64\\Resources" }
            };

            var Build_Resource_Files = new Dictionary<string, string>(4, StringComparer.OrdinalIgnoreCase)
            {
                ["SystemInformer.png"] = "icon.png",
                ["CapsList.txt"] = "CapsList.txt",
                ["EtwGuids.txt"] = "EtwGuids.txt",
                ["PoolTag.txt"] = "PoolTag.txt",
            };

            string binPath = GetBuildBaseDirectory(Flags);

            foreach (var file in Build_Resource_Files)
            {
                foreach (var kvp in configurations)
                {
                    if (Flags.HasFlag(kvp.Key.config) && Flags.HasFlag(kvp.Key.arch))
                    {
                        Win32.CopyIfNewer(
                            Path.Join([BuildWorkingFolder, "\\SystemInformer\\resources\\", file.Key]),
                            Path.Join([binPath, $"\\{kvp.Value}\\", file.Value]),
                            Flags);
                    }
                }
            }

            return true;
        }

        /// <summary>
        /// Copies Windows Debug Engine files from the SDK to output folders for each build configuration.
        /// </summary>
        /// <param name="Flags">Build flags indicating which configurations to process.</param>
        /// <returns>True if operation succeeds.</returns>
        public static bool CopyDebugEngineFiles(BuildFlags Flags)
        {
            //
            // https://learn.microsoft.com/en-us/legal/windows-sdk/redist#debugging-tools-for-windows
            //
            // The following is quoted from the link above on 2024-07-13:
            //
            // You may distribute these files as part of your program.
            // - Dbgeng.dll
            // - Dbgcore.dll
            // - Dbgmodel.dll
            // - Dbghelp.dll
            // - Srcsrv.dll
            // - Symsrv.dll
            // - Symchk.exe
            // - Symbolcheck.dll
            // - Symstore.exe
            // - Program Files\Windows Kits\10\Debuggers\Redist\X86 Debuggers and Tools-x86_en-us.msi
            // - Program Files\Windows Kits\10\Debuggers\Redist\X64 Debuggers and Tools-x64_en-us.msi
            // - App Verifier
            //

            var debugArchitectures = new Dictionary<BuildFlags, (string sdkArch, string debugDir)>
            {
                { BuildFlags.Build32bit, ("x86", "Debug32") },
                { BuildFlags.Build64bit, ("x64", "Debug64") },
                { BuildFlags.BuildArm64bit, ("arm64", "DebugARM64") }
            };

            var releaseArchitectures = new Dictionary<BuildFlags, (string sdkArch, string releaseDir)>
            {
                { BuildFlags.Build32bit, ("x86", "Release32") },
                { BuildFlags.Build64bit, ("x64", "Release64") },
                { BuildFlags.BuildArm64bit, ("arm64", "ReleaseARM64") }
            };

            string[] Build_DebugCore_Files =
            [
                "dbgcore.dll",
                "dbghelp.dll",
                "symsrv.dll"
            ];

            string binPath = GetBuildBaseDirectory(Flags);

            // Required for the ESDK
            string windowsSdkPath = Path.GetFullPath(Path.Join([Utils.GetWindowsSdkPath(), "..\\..\\Debuggers\\"]));

            if (string.IsNullOrWhiteSpace(windowsSdkPath))
                return false;

            if (!Directory.Exists(windowsSdkPath))
            {
                Program.PrintColorMessage($"[SDK] Skipped CopyDebugEngineFiles: Directory doesn't exist.", ConsoleColor.DarkGray, true, Flags);
                return true;
            }

            foreach (var file in Build_DebugCore_Files)
            {
                if (Flags.HasFlag(BuildFlags.BuildDebug))
                {
                    foreach (var kvp in debugArchitectures)
                    {
                        if (Flags.HasFlag(kvp.Key))
                        {
                            Win32.CopyIfNewer(
                                Path.Join([windowsSdkPath, $"\\{kvp.Value.sdkArch}\\", file]),
                                Path.Join([binPath, $"\\{kvp.Value.debugDir}\\", file]),
                                Flags);
                        }
                    }
                }

                if (Flags.HasFlag(BuildFlags.BuildRelease))
                {
                    foreach (var kvp in releaseArchitectures)
                    {
                        if (Flags.HasFlag(kvp.Key))
                        {
                            Win32.CopyIfNewer(
                                Path.Join([windowsSdkPath, $"\\{kvp.Value.sdkArch}\\", file]),
                                Path.Join([binPath, $"\\{kvp.Value.releaseDir}\\", file]),
                                Flags);
                        }
                    }
                }
            }

            return true;
        }

        /// <summary>
        /// Validates export definitions for SystemInformer executables in output folders.
        /// </summary>
        /// <param name="Flags">Build flags indicating which configurations to validate.</param>
        /// <returns>True if all validations succeed.</returns>
        public static bool BuildValidateExportDefinitions(BuildFlags Flags)
        {
            Program.PrintColorMessage(BuildTimeSpan(), ConsoleColor.DarkGray, false);
            Program.PrintColorMessage("Validating exports...", ConsoleColor.Cyan);

            var validationPaths = new Dictionary<(BuildFlags config, BuildFlags arch), string>
            {
                { (BuildFlags.BuildDebug, BuildFlags.Build64bit), "Debug32\\SystemInformer.exe" },
                { (BuildFlags.BuildDebug, BuildFlags.BuildArm64bit), "Debug32\\SystemInformer.exe" },
                { (BuildFlags.BuildRelease, BuildFlags.Build64bit), "Release32\\SystemInformer.exe" },
                { (BuildFlags.BuildRelease, BuildFlags.BuildArm64bit), "Release32\\SystemInformer.exe" }
            };

            string binPath = GetBuildBaseDirectory(Flags);

            foreach (var kvp in validationPaths)
            {
                if (Flags.HasFlag(kvp.Key.config) && Flags.HasFlag(kvp.Key.arch))
                {
                    if (!Utils.ValidateImageExports(Path.Join([binPath, $"\\{kvp.Value}"])) )
                        return false;
                }
            }

            return true;
        }

        /// <summary>
        /// Builds public SDK header files by invoking the header generator.
        /// </summary>
        /// <returns>True if headers are built successfully; otherwise, false.</returns>
        public static bool BuildPublicHeaderFiles()
        {
            Program.PrintColorMessage(BuildTimeSpan(), ConsoleColor.DarkGray, false);
            Program.PrintColorMessage("Building public SDK headers...", ConsoleColor.Cyan);

            try
            {
                HeaderGen.Execute();
            }
            catch (Exception ex)
            {
                Program.PrintColorMessage($"[ERROR] {ex}", ConsoleColor.Red);
                return false;
            }

            return true;
        }

        /// <summary>
        /// Builds a single native header file by invoking the single header generator.
        /// </summary>
        /// <returns>True if the header is built successfully; otherwise, false.</returns>
        public static bool BuildSingleNativeHeader()
        {
            Program.PrintColorMessage(BuildTimeSpan(), ConsoleColor.DarkGray, false);
            Program.PrintColorMessage("Building single header file...", ConsoleColor.Cyan);

            try
            {
                if (!SingleHeaderGen.Execute())
                    return false;
            }
            catch (Exception ex)
            {
                Program.PrintColorMessage($"[ERROR] {ex}", ConsoleColor.Red);
                return false;
            }

            return true;
        }

        /// <summary>
        /// Builds dynamic data files for the specified output directory.
        /// </summary>
        /// <param name="OutDir">Output directory for dynamic data.</param>
        /// <returns>True if dynamic data is built successfully; otherwise, false.</returns>
        public static bool BuildDynamicData(string OutDir)
        {
            Program.PrintColorMessage("Building dynamic data...", ConsoleColor.Cyan);

            try
            {
                return DynData.Execute(OutDir, BuildCanary);
            }
            catch (Exception ex)
            {
                Program.PrintColorMessage($"[ERROR] {ex}", ConsoleColor.Red);
                return false;
            }
        }

        /// <summary>
        /// Copies kernel driver files to output folders for each build configuration.
        /// </summary>
        /// <param name="Flags">Build flags indicating which configurations to process.</param>
        /// <returns>True if operation succeeds.</returns>
        public static bool CopyKernelDriver(BuildFlags Flags)
        {
            var configurations = new Dictionary<(BuildFlags config, BuildFlags arch), (string sourceArch, string targetDir)>
            {
                { (BuildFlags.BuildDebug, BuildFlags.Build64bit), ("amd64", "Debug64") },
                { (BuildFlags.BuildDebug, BuildFlags.BuildArm64bit), ("arm64", "DebugARM64") },
                { (BuildFlags.BuildRelease, BuildFlags.Build64bit), ("amd64", "Release64") },
                { (BuildFlags.BuildRelease, BuildFlags.BuildArm64bit), ("arm64", "ReleaseARM64") }
            };
            string[] Build_Driver_Files =
            [
                "SystemInformer.sys",
                "ksi.dll"
            ];

            string binPath = GetBuildBaseDirectory(Flags);

            try
            {
                foreach (string file in Build_Driver_Files)
                {
                    foreach (var kvp in configurations)
                    {
                        if (Flags.HasFlag(kvp.Key.config) && Flags.HasFlag(kvp.Key.arch))
                        {
                            Win32.CopyVersionIfNewer(
                                Path.Join([Build.BuildWorkingFolder, $"\\KSystemInformer\\bin-signed\\{kvp.Value.sourceArch}\\", file]),
                                Path.Join([binPath, $"\\{kvp.Value.targetDir}\\", file]),
                                Flags);
                        }
                    }
                }
            }
            catch (Exception ex)
            {
                Program.PrintColorMessage($"[ERROR] (CopyKernelDriver) {ex}", ConsoleColor.Red);
                return false;
            }

            return true;
        }

        /// <summary>
        /// Resigns files in the output directory by deleting old signature files and creating new ones.
        /// </summary>
        /// <returns>True if all files are resigned successfully; otherwise, false.</returns>
        public static bool ResignFiles()
        {
            var files = new List<string>();

            if (!Directory.Exists("bin"))
                return false;

            foreach (string sigFile in Directory.EnumerateFiles("bin", "*.sig", SearchOption.AllDirectories))
            {
                var file = Path.ChangeExtension(sigFile, ".dll");

                if (File.Exists(file))
                    files.Add(file);

                file = Path.ChangeExtension(sigFile, ".exe");

                if (File.Exists(file))
                    files.Add(file);

                file = Path.ChangeExtension(sigFile, ".bin");

                if (File.Exists(file))
                    files.Add(file);

                File.Delete(sigFile);
            }

            foreach (string file in files)
            {
                if (!BuildVerify.CreateSigFile("kph", file, BuildCanary))
                    return false;
            }

            return true;
        }

        /// <summary>
        /// Builds the SDK by copying headers, libraries, and generating additional files.
        /// </summary>
        /// <param name="Flags">Build flags indicating which configurations to process.</param>
        /// <returns>True if the SDK is built successfully; otherwise, false.</returns>
        public static bool BuildSdk(BuildFlags Flags)
        {
            var configurations = new Dictionary<(BuildFlags config, BuildFlags arch), (string sourceDir, string targetArch)>
            {
                { (BuildFlags.BuildDebug, BuildFlags.Build32bit), ("Debug32", "i386") },
                { (BuildFlags.BuildDebug, BuildFlags.Build64bit), ("Debug64", "amd64") },
                { (BuildFlags.BuildDebug, BuildFlags.BuildArm64bit), ("DebugARM64", "arm64") },
                { (BuildFlags.BuildRelease, BuildFlags.Build32bit), ("Release32", "i386") },
                { (BuildFlags.BuildRelease, BuildFlags.Build64bit), ("Release64", "amd64") },
                { (BuildFlags.BuildRelease, BuildFlags.BuildArm64bit), ("ReleaseARM64", "arm64") }
            };

            string[] Build_Sdk_Files =
            [
                "SystemInformer.lib",
                "SystemInformer.pdb"
            ];

            string binPath = GetBuildBaseDirectory(Flags);

            foreach (string folder in BuildConfig.Build_Sdk_Directories)
            {
                Win32.CreateDirectory(folder);
            }

            //
            // Copy headers
            //

            foreach (string file in BuildConfig.Build_Phnt_Headers)
            {
                Win32.CopyIfNewer(
                    Path.Join([Build.BuildWorkingFolder, "\\phnt\\include\\", file]),
                    Path.Join([Build.BuildWorkingFolder, "\\sdk\\include\\", file]), 
                    Flags, true);
            }

            foreach (string file in BuildConfig.Build_Phlib_Headers)
            {
                Win32.CopyIfNewer(
                    Path.Join([Build.BuildWorkingFolder, "\\phlib\\include\\", file]),
                    Path.Join([Build.BuildWorkingFolder, "\\sdk\\include\\", file]), 
                    Flags, true);
            }
            foreach (string file in BuildConfig.Build_Kphlib_Headers)
            {
                Win32.CopyIfNewer(
                    Path.Join([Build.BuildWorkingFolder, "\\kphlib\\include\\", file]), 
                    Path.Join([Build.BuildWorkingFolder, "\\sdk\\include\\", file]), 
                    Flags, true);
            }

            //
            // Copy readme
            //

            Win32.CopyIfNewer(
                Path.Join([Build.BuildWorkingFolder, "\\SystemInformer\\sdk\\readme.txt"]), 
                Path.Join([Build.BuildWorkingFolder, "\\sdk\\readme.txt"]), 
                Flags, true);

            //
            // Copy files
            //

            foreach (string file in Build_Sdk_Files)
            {
                foreach (var kvp in configurations)
                {
                    if (Flags.HasFlag(kvp.Key.config) && Flags.HasFlag(kvp.Key.arch))
                    {
                        Win32.CopyIfNewer(
                            Path.Join([binPath, $"\\{kvp.Value.sourceDir}\\", file]),
                            Path.Join([Build.BuildWorkingFolder, $"\\sdk\\lib\\{kvp.Value.targetArch}\\", file]),
                            Flags,
                            true
                            );
                    }
                }
            }

            //
            // Build the SDK
            //

            HeaderGen.Execute();

            //
            // Copy the SDK headers
            //

            Win32.CopyIfNewer(
                Path.Join([Build.BuildWorkingFolder, "\\SystemInformer\\include\\phappres.h"]), 
                Path.Join([Build.BuildWorkingFolder, "\\sdk\\include\\phappres.h"]), 
                Flags, true);

            Win32.CopyIfNewer(
                Path.Join([Build.BuildWorkingFolder, "\\SystemInformer\\sdk\\phapppub.h"]), 
                Path.Join([Build.BuildWorkingFolder, "\\sdk\\include\\phapppub.h"]), 
                Flags, true);

            Win32.CopyIfNewer(
                Path.Join([Build.BuildWorkingFolder, "\\SystemInformer\\sdk\\phdk.h"]), 
                Path.Join([Build.BuildWorkingFolder, "\\sdk\\include\\phdk.h"]),
                Flags, true);

            //
            // Copy the resource header and prefix types with PHAPP for the SDK
            //

            Win32.GetFileBasicInfo(
                Path.Join([Build.BuildWorkingFolder, "\\SystemInformer\\resource.h"]), 
                out var sourceCreationTime, 
                out var sourceWriteTime, 
                out _
                );

            Win32.GetFileBasicInfo(
                Path.Join([Build.BuildWorkingFolder, "\\sdk\\include\\phappresource.h"]),
                out var targetCreationTime, 
                out var targetWriteTime, 
                out var targetAttributes
                );

            if (sourceCreationTime != targetCreationTime || sourceWriteTime != targetWriteTime)
            {
                string resourceContent = Utils.ReadAllText(Path.Join([Build.BuildWorkingFolder, "\\SystemInformer\\resource.h"]));
                string targetContent = resourceContent.Replace("#define ID", "#define PHAPP_ID", StringComparison.OrdinalIgnoreCase);

                if (!resourceContent.Equals(targetContent, StringComparison.OrdinalIgnoreCase))
                {
                    if ((targetAttributes & FileAttributes.ReadOnly) != 0)
                        File.SetAttributes(Path.Join([Build.BuildWorkingFolder, "\\sdk\\include\\phappresource.h"]), FileAttributes.Normal);
                    Utils.WriteAllText(Path.Join([Build.BuildWorkingFolder, "\\sdk\\include\\phappresource.h"]), targetContent);
                }

                Win32.SetFileBasicInfo(Path.Join([Build.BuildWorkingFolder, "\\sdk\\include\\phappresource.h"]), sourceCreationTime, sourceWriteTime, true);
                //Win32.CopyIfNewer("SystemInformer\\resource.h", "sdk\\include\\phappresource.h", Flags);
            }

            return true;
        }

        /// <summary>
        /// Builds the setup executable for the specified channel.
        /// </summary>
        /// <param name="Channel">The release channel (e.g., "release", "canary").</param>
        /// <param name="Flags">Build flags indicating which configurations to process.</param>
        /// <returns>True if the setup executable is built successfully; otherwise, false.</returns>
        public static bool BuildSetupExe(string Channel, BuildFlags Flags)
        {
            Program.PrintColorMessage(BuildTimeSpan(), ConsoleColor.DarkGray, false);
            Program.PrintColorMessage($"Building build-{Channel}-setup.exe... ", ConsoleColor.Cyan, false);

            if (!BuildSolution(
                Path.Join([Build.BuildWorkingFolder, "\\tools\\CustomSetupTool\\CustomSetupTool.sln"]),
                BuildFlags.Build32bit | BuildFlags.BuildApi,
                Channel
                ))
            {
                return false;
            }

            try
            {
                string binPath = GetBuildBaseDirectory(Flags);
                string suffix = GetToolchainSuffix(Flags);
                string config = Flags.HasFlag(BuildFlags.BuildDebug) ? "Debug" : "Release";
                string out_file = Path.Join([Build.BuildWorkingFolder, "\\tools\\CustomSetupTool\\", binPath, $"\\{config}32\\CustomSetupTool.exe"]);
                string exe_file = Path.Join([Build.BuildOutputFolder, $"\\systeminformer-build{suffix}-{Channel}-setup.exe"]);

                Utils.CreateOutputDirectory();

                Win32.DeleteFile(
                    exe_file,
                    Flags
                    );

                File.Move(
                    out_file,
                    exe_file
                    );

                Program.PrintColorMessage(Win32.GetFileSize(exe_file).ToPrettySize(), ConsoleColor.Green);
            }
            catch (Exception ex)
            {
                Program.PrintColorMessage($"[ERROR] {ex}", ConsoleColor.Red);
                return false;
            }

            return true;
        }

        /// <summary>
        /// Builds a zip archive containing the SDK files.
        /// </summary>
        /// <param name="Flags">Build flags indicating which configurations to process.</param>
        /// <returns>True if the SDK zip is built successfully; otherwise, false.</returns>
        public static bool BuildSdkZip(BuildFlags Flags)
        {
            Program.PrintColorMessage(BuildTimeSpan(), ConsoleColor.DarkGray, false);
            Program.PrintColorMessage("Building build-sdk.zip... ", ConsoleColor.Cyan, false);

            try
            {
                string suffix = GetToolchainSuffix(Flags);
                string zip_file = Path.Join([Build.BuildOutputFolder, $"\\systeminformer-build{suffix}-sdk.zip"]);

                Utils.CreateOutputDirectory();

                Win32.DeleteFile(
                    zip_file,
                    Flags
                    );

                Zip.CreateCompressedSdkFromFolder(
                    "sdk",
                    zip_file,
                    Flags
                    );

                Program.PrintColorMessage(Win32.GetFileSize(zip_file).ToPrettySize(), ConsoleColor.Green);
            }
            catch (Exception ex)
            {
                Program.PrintColorMessage($"[ERROR] {ex}", ConsoleColor.Red);
                return false;
            }

            return true;
        }

        /// <summary>
        /// Builds zip archives containing binary files for each platform and a combined archive.
        /// </summary>
        /// <param name="Flags">Build flags indicating which configurations to process.</param>
        /// <returns>True if all zip archives are built successfully; otherwise, false.</returns>
        public static bool BuildBinZip(BuildFlags Flags)
        {
            string binPath = GetBuildBaseDirectory(Flags);
            string config = Flags.HasFlag(BuildFlags.BuildDebug) ? "Debug" : "Release";
            string suffix = GetToolchainSuffix(Flags);

            var Build_Zip_Files = new Dictionary<string, string>(4, StringComparer.OrdinalIgnoreCase)
            {
                [Path.Join([binPath, $"\\{config}32"])] = $"systeminformer-build{suffix}-win32-bin.zip",
                [Path.Join([binPath, $"\\{config}64"])] = $"systeminformer-build{suffix}-win64-bin.zip",
                [Path.Join([binPath, $"\\{config}ARM64"])] = $"systeminformer-build{suffix}-arm64-bin.zip",
                [binPath] = $"systeminformer-build{suffix}-bin.zip",
            };

            Program.PrintColorMessage(BuildTimeSpan(), ConsoleColor.DarkGray, false);

            try
            {
                foreach (var file in Build_Zip_Files)
                {
                    string zip_file = Path.Join([Build.BuildOutputFolder, file.Value]);

                    Program.PrintColorMessage($"Building {file.Value}... ", ConsoleColor.Cyan, false);
                    Win32.DeleteFile(zip_file, Flags);
                    Zip.CreateCompressedFolder(file.Key, zip_file, Flags);
                    Program.PrintColorMessage(Win32.GetFileSize(zip_file).ToPrettySize(), ConsoleColor.Green);
                }
            }
            catch (Exception ex)
            {
                Program.PrintColorMessage($"[ERROR] {ex}", ConsoleColor.Red);
                return false;
            }

            return true;
        }

        /// <summary>
        /// Builds a zip archive containing PDB files, optionally for MSIX package builds.
        /// </summary>
        /// <param name="MsixPackageBuild">If true, builds for MSIX package; otherwise, for standard build.</param>
        /// <param name="Flags">Build flags indicating which configurations to process.</param>
        /// <returns>True if the PDB zip is built successfully; otherwise, false.</returns>
        public static bool BuildPdbZip(bool MsixPackageBuild, BuildFlags Flags)
        {
            Program.PrintColorMessage(BuildTimeSpan(), ConsoleColor.DarkGray, false);
            Program.PrintColorMessage(MsixPackageBuild ? "Building setup-package-pdb..." : "Building build-pdb.zip...", ConsoleColor.Cyan, false);

            try
            {
                Utils.CreateOutputDirectory();
                string suffix = GetToolchainSuffix(Flags);

                if (MsixPackageBuild)
                {
                    string zip_file = Path.Join([Build.BuildOutputFolder, $"\\systeminformer-setup{suffix}-package-pdb.zip"]);

                    Win32.DeleteFile(
                        zip_file,
                        Flags
                        );

                    Zip.CreateCompressedPdbFromFolder(
                        ".\\",
                        zip_file,
                        Flags
                        );

                    Program.PrintColorMessage(
                        Win32.GetFileSize(zip_file).ToPrettySize(),
                        ConsoleColor.Green
                        );
                }
                else
                {
                    string zip_file = Path.Join([Build.BuildOutputFolder, $"\\systeminformer-build{suffix}-pdb.zip"]);

                    Win32.DeleteFile(
                        zip_file,
                        Flags
                        );

                    Zip.CreateCompressedPdbFromFolder(
                        ".\\",
                        zip_file,
                        Flags
                        );

                    Program.PrintColorMessage(
                        Win32.GetFileSize(zip_file).ToPrettySize(),
                        ConsoleColor.Green
                        );
                }
            }
            catch (Exception ex)
            {
                Program.PrintColorMessage($"[ERROR] {ex}", ConsoleColor.Red);
                return false;
            }

            return true;
        }

        /// <summary>
        /// Builds a zip archive containing symbol files using SymStore.
        /// </summary>
        /// <param name="Flags">Build flags indicating which configurations to process.</param>
        /// <returns>True if the symbol package is built successfully; otherwise, false.</returns>
        public static bool BuildSymStoreZip(BuildFlags Flags)
        {
            Program.PrintColorMessage(BuildTimeSpan(), ConsoleColor.DarkGray, false);
            Program.PrintColorMessage("Building symsrv-package...", ConsoleColor.Cyan, false);

            if (!Utils.SymStoreExists())
            {
                Program.PrintColorMessage("[ERROR] SymStore.exe not found.", ConsoleColor.Red);
                if (Win32.HasEnvironmentVariable("GITHUB_ACTIONS"))
                    return true;
                return false;
            }

            try
            {
                string suffix = GetToolchainSuffix(Flags);
                string zip_file = Path.Join([Build.BuildOutputFolder, $"\\systeminformer{suffix}-symbols-package.zip"]);

                Win32.DeleteFile(
                    zip_file
                    );
                Utils.CreateOutputDirectory();

                Utils.ExecuteSymStoreCommand(
                    @$"add -:REL /r /f ""bin\\Release32\\*.*"" /s ""build\\output\\symbols"" /t SystemInformer /v ""{Build.BuildLongVersion}"" /c ""32bit-{Build.BuildCommitHash}"" /o"
                    );
                Utils.ExecuteSymStoreCommand(
                    @$"add -:REL /r /f ""bin\\Release64\\*.*"" /s ""build\\output\\symbols"" /t SystemInformer /v ""{Build.BuildLongVersion}"" /c ""64bit-{Build.BuildCommitHash}"" /o"
                    );
                Utils.ExecuteSymStoreCommand(
                    @$"add -:REL /r /f ""bin\\ReleaseARM64\\*.*"" /s ""build\\output\\symbols"" /t SystemInformer /v ""{Build.BuildLongVersion}"" /c ""arm64-{Build.BuildCommitHash}"" /o"
                    );

                Program.PrintColorMessage("Building symbols-package.zip...", ConsoleColor.Cyan, false);

                Zip.CreateCompressedSdkFromFolder(
                    "build\\output\\symbols",
                    zip_file,
                    Flags
                    );
                Program.PrintColorMessage(
                    Win32.GetFileSize(zip_file).ToPrettySize(),
                    ConsoleColor.Green
                    );
            }
            catch (Exception ex)
            {
                Program.PrintColorMessage($"[ERROR] {ex}", ConsoleColor.Red);
                return false;
            }

            return true;
        }

        /// <summary>
        /// Builds a checksums file for release artifacts.
        /// </summary>
        /// <returns>True if the checksums file is built successfully; otherwise, false.</returns>
        public static bool BuildChecksumsFile()
        {
            string[] Build_Upload_Files =
            [
                "systeminformer-build-win32-bin.zip",
                "systeminformer-build-win64-bin.zip",
                "systeminformer-build-arm64-bin.zip",
                "systeminformer-build-bin.zip",
                "systeminformer-build-pdb.zip",
                "systeminformer-build-release-setup.exe",
                "systeminformer-build-canary-setup.exe"
            ];

            Program.PrintColorMessage(BuildTimeSpan(), ConsoleColor.DarkGray, false);
            Program.PrintColorMessage("Building release checksums...", ConsoleColor.Cyan);

            try
            {
                StringBuilder sb = new StringBuilder();

                foreach (var name in Build_Upload_Files)
                {
                    string file = Path.Join([Build.BuildOutputFolder, name]);

                    if (File.Exists(file))
                    {
                        FileInfo info = new FileInfo(file);
                        string hash = BuildVerify.HashFile(file);

                        sb.AppendLine(info.Name);
                        sb.AppendLine($"SHA256: {hash}{Environment.NewLine}");
                    }
                }

                Utils.CreateOutputDirectory();

                Win32.DeleteFile(
                    $"{BuildOutputFolder}\\systeminformer-build-checksums.txt"
                    );

                Utils.WriteAllText(
                    $"{BuildOutputFolder}\\systeminformer-build-checksums.txt",
                    sb.ToString()
                    );
            }
            catch (Exception ex)
            {
                Program.PrintColorMessage($"[ERROR] {ex}", ConsoleColor.Red);
                return false;
            }

            return true;
        }

        /// <summary>
        /// Constructs the MSBuild command line string for building a solution with the specified platform, flags, and channel.
        /// </summary>
        /// <param name="Solution">The solution file to build.</param>
        /// <param name="Platform">The target platform (e.g., Win32, x64, ARM64).</param>
        /// <param name="Flags">Build flags indicating which configurations to process.</param>
        /// <param name="Channel">Optional release channel.</param>
        /// <returns>The constructed MSBuild command line string.</returns>
        private static string MsbuildCommandString(string Solution, string Platform, BuildFlags Flags, string Channel = null)
        {
            List<string> preprocessorOptionsList = new List<string>();
            StringBuilder commandLine = new StringBuilder(0x100);
            StringBuilder preprocessorOptions = new StringBuilder(0x100);
            StringBuilder linkerOptions = new StringBuilder(0x100);

            if (Flags.HasFlag(BuildFlags.BuildApi))
                preprocessorOptionsList.Add("PH_BUILD_API");
            if (Flags.HasFlag(BuildFlags.BuildMsix))
                preprocessorOptionsList.Add("PH_BUILD_MSIX");
            if (!string.IsNullOrWhiteSpace(Channel))
                preprocessorOptionsList.Add($"PH_RELEASE_CHANNEL_ID=\"{BuildConfig.Build_Channels[Channel]}\"");
            if (!string.IsNullOrWhiteSpace(Build.BuildCommitHash))
                preprocessorOptionsList.Add($"PHAPP_VERSION_COMMITHASH=\"{Build.BuildHash()}\"");
            if (!string.IsNullOrWhiteSpace(Build.BuildVersionMajor))
                preprocessorOptionsList.Add($"PHAPP_VERSION_MAJOR=\"{Build.BuildVersionMajor}\"");
            if (!string.IsNullOrWhiteSpace(Build.BuildVersionMinor))
                preprocessorOptionsList.Add($"PHAPP_VERSION_MINOR=\"{Build.BuildVersionMinor}\"");
            if (!string.IsNullOrWhiteSpace(Build.BuildVersionBuild()))
                preprocessorOptionsList.Add($"PHAPP_VERSION_BUILD=\"{Build.BuildVersionBuild()}\"");
            if (!string.IsNullOrWhiteSpace(Build.BuildVersionRevision()))
                preprocessorOptionsList.Add($"PHAPP_VERSION_REVISION=\"{Build.BuildVersionRevision()}\"");

            if (!string.IsNullOrWhiteSpace(Build.BuildSourceLink))
                linkerOptions.Append($"/SOURCELINK:\"{Build.BuildSourceLink}\" ");

            preprocessorOptions.AppendJoin(";", preprocessorOptionsList);

            commandLine.Append($"/m /nologo /nodereuse:false /verbosity:{(Build.BuildToolsDebug ? "diagnostic" : "minimal")} ");
            commandLine.Append($"/p:Platform={Platform} /p:Configuration={(Flags.HasFlag(BuildFlags.BuildDebug) ? "Debug" : "Release")} ");

            if (preprocessorOptions.Length > 0)
                commandLine.Append($"/p:ExternalPreprocessorOptions=\"{preprocessorOptions.ToString()}\" ");
            if (linkerOptions.Length > 0)
                commandLine.Append($"/p:ExternalLinkerOptions=\"{linkerOptions.ToString()}\" ");
            if (!string.IsNullOrWhiteSpace(Build.BuildSimdExtensions))
                commandLine.Append($"/p:ExternalSimdOptions=\"{Build.BuildSimdExtensions}\" ");

            commandLine.Append($"/bl:build/output/logs/{Utils.GetBuildLogPath(Solution, Platform, Flags)}.binlog ");

            if (!Build.BuildRedirectOutput && !Build.BuildIntegration)
            {
                commandLine.Append("-terminalLogger:on ");
            }

            commandLine.Append(Solution);

            return commandLine.ToString();
        }

        private static bool MsbuildCommand(string Solution, string Platform, BuildFlags Flags, string Channel = null)
        {
            Program.PrintColorMessage(BuildTimeSpan(), ConsoleColor.DarkGray, false, Flags);
            Program.PrintColorMessage($"Building {Path.GetFileNameWithoutExtension(Solution)} (", ConsoleColor.Cyan, false, Flags);
            Program.PrintColorMessage(Platform, ConsoleColor.Green, false, Flags);
            Program.PrintColorMessage(")...", ConsoleColor.Cyan, true, Flags);

            string buildCommandLine = MsbuildCommandString(Solution, Platform, Flags, Channel);

            if (Build.BuildRedirectOutput && !Build.BuildIntegration)
            {
                int errorcode = Utils.ExecuteMsbuildCommand(buildCommandLine, Flags, out string errorstring);

                if (errorcode != 0)
                {
                    Program.PrintColorMessage($"[ERROR] ({errorcode}) {errorstring}", ConsoleColor.Red, true, Flags | BuildFlags.BuildVerbose);
                    return false;
                }
            }
            else
            {
                int errorcode = Utils.ExecuteMsbuildCommand(buildCommandLine, Flags, out _, false);

                if (!Build.BuildRedirectOutput && !Build.BuildIntegration)
                {
                    Console.Write(Environment.NewLine);
                }

                if (errorcode != 0)
                    return false;
            }

            return true;
        }

        /// <summary>
        /// Builds the specified solution using MSBuild for the given build flags and channel.
        /// </summary>
        /// <param name="Solution">The solution file to build.</param>
        /// <param name="Flags">Build flags indicating which configurations to process.</param>
        /// <param name="Channel">Optional release channel.</param>
        /// <returns>True if the solution is built successfully; otherwise, false.</returns>
        public static bool BuildSolution(string Solution, BuildFlags Flags, string Channel = null)
        {
            // Default/MSBuild path (existing behavior).
            if (Flags.HasFlag(BuildFlags.Build32bit))
            {
                if (!MsbuildCommand(Solution, "Win32", Flags, Channel))
                    return false;
            }

            if (Flags.HasFlag(BuildFlags.Build64bit))
            {
                if (!MsbuildCommand(Solution, "x64", Flags, Channel))
                    return false;
            }

            if (Flags.HasFlag(BuildFlags.BuildArm64bit))
            {
                if (HaveArm64BuildTools)
                {
                    if (!MsbuildCommand(Solution, "ARM64", Flags, Channel))
                        return false;
                }
                else
                {
                    Program.PrintColorMessage("[SKIPPED] ARM64 build tools not installed.", ConsoleColor.Yellow, true, Flags);
                }
            }

            return true;
        }

        /// <summary>
        /// Builds the specified solution using CMake for the given generator, configuration, and toolchain.
        /// </summary>
        /// <param name="Solution">The solution file to build.</param>
        /// <param name="Generator">The CMake generator to use.</param>
        /// <param name="Config">The build configuration (e.g., Debug, Release).</param>
        /// <param name="Toolchain">The toolchain identifier.</param>
        /// <param name="Flags">Build flags indicating which configurations to process.</param>
        /// <returns>True if the solution is built successfully; otherwise, false.</returns>
        public static bool BuildSolutionCMake(string Solution, BuildGenerator Generator, BuildToolchain Toolchain, BuildFlags Flags)
        {
            // If CMake build is requested, route to BuildSolutionCMake for each requested arch.
            if (Flags.HasFlag(BuildFlags.BuildCMake))
            {
                // 32-bit
                if (Flags.HasFlag(BuildFlags.Build32bit))
                {
                    if (!ExecuteBuildSolutionCMake(Solution, Generator, Toolchain, Flags))
                        return false;
                }

                // 64-bit
                if (Flags.HasFlag(BuildFlags.Build64bit))
                {
                    if (!ExecuteBuildSolutionCMake(Solution, Generator, Toolchain, Flags))
                        return false;
                }

                // ARM64
                if (Flags.HasFlag(BuildFlags.BuildArm64bit))
                {
                    if (HaveArm64BuildTools)
                    {
                        if (!ExecuteBuildSolutionCMake(Solution, Generator, Toolchain, Flags))
                            return false;
                    }
                    else
                    {
                        Program.PrintColorMessage("[SKIPPED] ARM64 build tools not installed.", ConsoleColor.Yellow, true, Flags);
                    }
                }

                return true;
            }

            return false;
        }

        public static bool ExecuteBuildSolutionCMake(string Solution, BuildGenerator Generator, BuildToolchain Toolchain, BuildFlags Flags)
        {
            string buildConfig = Flags.HasFlag(BuildFlags.Debug) ? "Debug" : "Release";
            string buildFolder = Flags.HasFlag(BuildFlags.Debug) ? "build\\debug" : "build\\release";

            string platformSuffix = Toolchain switch
            {
                BuildToolchain.MsvcX86 => "-msvc-32",
                BuildToolchain.MsvcAmd64 => "-msvc-64",
                BuildToolchain.MsvcArm64 => "-msvc-arm64",
                BuildToolchain.ClangMsvcX86 => "-clang-msvc-32",
                BuildToolchain.ClangMsvcAmd64 => "-clang-msvc-64",
                BuildToolchain.ClangMsvcArm64 => "-clang-msvc-arm64",
                _ => throw new ArgumentException("Unsupported toolchain")
            };

            string toolchainFile = Toolchain switch
            {
                BuildToolchain.MsvcX86 => "cmake/toolchain/msvc-x86.cmake",
                BuildToolchain.MsvcAmd64 => "cmake/toolchain/msvc-amd64.cmake",
                BuildToolchain.MsvcArm64 => "cmake/toolchain/msvc-arm64.cmake",
                BuildToolchain.ClangMsvcX86 => "cmake/toolchain/clang-msvc-x86.cmake",
                BuildToolchain.ClangMsvcAmd64 => "cmake/toolchain/clang-msvc-amd64.cmake",
                BuildToolchain.ClangMsvcArm64 => "cmake/toolchain/clang-msvc-arm64.cmake",
                _ => throw new ArgumentException("Unsupported toolchain")
            };

            buildFolder = Path.Join([Build.BuildWorkingFolder, buildFolder + platformSuffix]);
            toolchainFile = Path.Join([Build.BuildWorkingFolder, toolchainFile]);

            //
            // Clean
            //

            if (Directory.Exists(buildFolder))
            {
                try
                { Directory.Delete(buildFolder, true); }
                catch { }
            }

            Program.PrintColorMessage(BuildTimeSpan(), ConsoleColor.DarkGray, false, Flags);
            Program.PrintColorMessage($"Generating {Path.GetFileNameWithoutExtension(Solution)} [", ConsoleColor.Cyan, false, Flags);
            Program.PrintColorMessage(Utils.GetToolchainString(Toolchain), ConsoleColor.Green, false, Flags);
            Program.PrintColorMessage("] (", ConsoleColor.Cyan, false, Flags);
            Program.PrintColorMessage(buildConfig, ConsoleColor.Green, false, Flags);
            Program.PrintColorMessage(")", ConsoleColor.Cyan, true, Flags);

            //
            // Generate
            //

            string cmakeGenOpts = string.Empty;
            if (Generator == BuildGenerator.Ninja)
            {
                cmakeGenOpts = $"-DCMAKE_BUILD_TYPE={buildConfig}";
            }

            string generateArgs = $"-G \"{Utils.GetGeneratorString(Generator)}\" -S \"{Build.BuildWorkingFolder}\" -B \"{buildFolder}\" " +
                                  $"--toolchain \"{toolchainFile}\" " +
                                  $"-DCMAKE_EXPORT_COMPILE_COMMANDS=ON " +
                                  $"-DSI_WITH_CORE=ON " +
                                  $"-DSI_WITH_PLUGINS=ON " +
                                  $"{cmakeGenOpts}";

            int errorcode = Utils.ExecuteCMakeCommand(generateArgs);
            if (errorcode != 0)
            {
                Program.PrintColorMessage($"[ERROR] CMake generate failed ({errorcode})", ConsoleColor.Red, true, Flags | BuildFlags.BuildVerbose);
                return false;
            }

            Console.WriteLine();

            //
            // Build
            //

            Program.PrintColorMessage(BuildTimeSpan(), ConsoleColor.DarkGray, false, Flags);
            Program.PrintColorMessage($"Building {Path.GetFileNameWithoutExtension(Solution)} [", ConsoleColor.Cyan, false, Flags);
            Program.PrintColorMessage(Utils.GetToolchainString(Toolchain), ConsoleColor.Green, false, Flags);
            Program.PrintColorMessage("] (", ConsoleColor.Cyan, false, Flags);
            Program.PrintColorMessage(buildConfig, ConsoleColor.Green, false, Flags);
            Program.PrintColorMessage(")", ConsoleColor.Cyan, true, Flags);

            string cmakeBuildOpts = string.Empty;
            if (Generator == BuildGenerator.VisualStudio)
            {
                cmakeBuildOpts = $"-- /m /p:Platform={Utils.CMakeGetPlatform(Toolchain)} -terminalLogger:auto";
            }

            errorcode = Utils.ExecuteCMakeCommand($"--build \"{buildFolder}\" --config {buildConfig} {cmakeBuildOpts}");
            if (errorcode != 0)
            {
                Program.PrintColorMessage($"[ERROR] CMake build failed ({errorcode})", ConsoleColor.Red, true, Flags | BuildFlags.BuildVerbose);
                return false;
            }

            Console.WriteLine();

            //
            // Clean
            //

            if (Directory.Exists(buildFolder))
            {
                try
                { Directory.Delete(buildFolder, true); }
                catch { }
            }

            return true;
        }

#nullable enable
        /// <summary>
        /// Creates a DeployFile object for a build artifact, optionally generating a signature and hash.
        /// </summary>
        /// <param name="Channel">The release channel (e.g., "release", "canary").</param>
        /// <param name="Name">The display name of the file.</param>
        /// <param name="FileName">The path to the file.</param>
        /// <param name="CreateSignature">If true, generates a signature and hash for the file.</param>
        /// <returns>A DeployFile object if successful; otherwise, null.</returns>
        private static DeployFile? CreateBuildDeployFile(string Channel, string Name, string FileName, bool CreateSignature = true)
        {
            if (CreateSignature)
            {
                string fileSign = BuildVerify.CreateSigString(Channel, FileName);

                if (string.IsNullOrWhiteSpace(fileSign))
                {
                    Program.PrintColorMessage("[ERROR] File signature failed.", ConsoleColor.Red);
                    return null;
                }

                string fileHash = BuildVerify.HashFile(FileName);
                string fileSize = Win32.GetFileSize(FileName).ToString();

                if (string.IsNullOrWhiteSpace(fileHash))
                {
                    Program.PrintColorMessage("[ERROR] File hash failed.", ConsoleColor.Red);
                    return null;
                }

                return new DeployFile(Name, FileName, fileHash, fileSign, fileSize);
            }
            else
            {
                string fileSize = Win32.GetFileSize(FileName).ToString();

                return new DeployFile(Name, FileName, null, null, fileSize);
            }
        }
#nullable disable

        /// <summary>
        /// Updates the server configuration with build information and asset links after a canary build.
        /// Uploads a list of build artifacts to GitHub as release assets and returns the release information.
        /// </summary>
        /// <returns>True if the server configuration is updated successfully; otherwise, false.</returns>
        public static bool BuildUpdateServerConfig()
        {
            if (!Build.BuildCanary)
                return true;
            if (Build.BuildToolsDebug)
                return true;

            // Define the files to upload (bool true create hash and signatute)
            var Build_Upload_Files = new Dictionary<string, bool>(4, StringComparer.OrdinalIgnoreCase)
            {
                ["systeminformer-build-win32-bin.zip"] = true,
                ["systeminformer-build-win64-bin.zip"] = true,
                ["systeminformer-build-arm64-bin.zip"] = true,
                //["systeminformer-build-bin.zip"] = true,
                ["systeminformer-build-pdb.zip"] = false,
                //["systeminformer-build-release-setup.exe"] = true,
            };

            List<DeployFile> deployFiles = new List<DeployFile>();

            // N.B. HACK we only produce a "release" (default setting) build for the binary (portable).
            // Sign it using the "release" key. (jxy-s)

            var portable_zip = CreateBuildDeployFile("release", "systeminformer-build-bin.zip", Path.Join([Build.BuildOutputFolder, "\\systeminformer-build-bin.zip"]));
            var release_exe = CreateBuildDeployFile("release", "systeminformer-build-release-setup.exe", Path.Join([Build.BuildOutputFolder, "\\systeminformer-build-release-setup.exe"]));
            var canary_exe = CreateBuildDeployFile("canary", "systeminformer-build-canary-setup.exe", Path.Join([Build.BuildOutputFolder, "\\systeminformer-build-canary-setup.exe"]));

            if (portable_zip == null || release_exe == null || canary_exe == null)
            {
                Program.PrintColorMessage("[ERROR] CreateBuildDeployFile.", ConsoleColor.Red);
                return false;
            }

            deployFiles.Add(portable_zip);
            deployFiles.Add(release_exe);
            deployFiles.Add(canary_exe);

            foreach (var file in Build_Upload_Files)
            {
                deployFiles.Add(CreateBuildDeployFile(
                    "release",
                    file.Key,
                    Path.Join([Build.BuildOutputFolder, file.Key]),
                    file.Value
                    ));
            }

            // Upload the assets to github

            var githubMirrorUpload = BuildUploadFilesToGithub(deployFiles);
            if (githubMirrorUpload == null)
                return false;

            // Check assets uploaded to github
            var github_release_id = githubMirrorUpload.ReleaseId.ToString();
            var binzipdownloadlink = githubMirrorUpload.GetFileUrl("systeminformer-build-bin.zip"); // $"systeminformer-{Build.BuildLongVersion}-bin.zip"
            var relzipdownloadlink = githubMirrorUpload.GetFileUrl("systeminformer-build-release-setup.exe");
            var canzipdownloadlink = githubMirrorUpload.GetFileUrl("systeminformer-build-canary-setup.exe");

            if (string.IsNullOrWhiteSpace(github_release_id) || string.IsNullOrWhiteSpace(binzipdownloadlink) || string.IsNullOrWhiteSpace(relzipdownloadlink) || string.IsNullOrWhiteSpace(canzipdownloadlink))
            {
                Program.PrintColorMessage("[ERROR] GetDeployInfo failed.", ConsoleColor.Red);
                return false;
            }

            // Update the build information.

            if (!BuildUploadServerConfig(portable_zip, release_exe, canary_exe, github_release_id))
            {
                Program.PrintColorMessage("[ERROR] BuildUploadServerConfig failed.", ConsoleColor.Red);
                return false;
            }

            return true;
        }

        /// <summary>
        /// Uploads a list of build artifacts to GitHub as release assets and returns the release information.
        /// </summary>
        /// <param name="Files">The list of DeployFile objects to upload.</param>
        /// <returns>A GithubRelease object with asset information if successful; otherwise, null.</returns>
        private static GithubRelease BuildUploadFilesToGithub(List<DeployFile> Files)
        {
            //if (!GithubReleases.DeleteRelease(Build.BuildLongVersion))
            //    return null;

            try
            {
                // Create a new github release.

                var newGithubRelease = BuildGithub.CreateRelease(
                    Build.BuildLongVersion,
                    true, // draft
                    false, // prerelease
                    true
                    );

                if (newGithubRelease == null)
                {
                    Program.PrintColorMessage("[Github.CreateRelease]", ConsoleColor.Red);
                    return null;
                }

                // Upload the files.

                foreach (DeployFile deployfile in Files)
                {
                    if (!File.Exists(deployfile.FileName))
                    {
                        Program.PrintColorMessage($"[Github.UploadAssets-NotFound] {deployfile.FileName}", ConsoleColor.Yellow);
                        continue;
                    }

                    string sourceName = Path.GetFileName(deployfile.FileName);

                    if (string.IsNullOrWhiteSpace(sourceName))
                    {
                        Program.PrintColorMessage("[Github.UploadAssets] Path.GetFileName", ConsoleColor.Red);
                        return null;
                    }

                    var label = sourceName.Replace(
                        "-build-",
                        $"-{Build.BuildLongVersion}-",
                        StringComparison.OrdinalIgnoreCase
                        );

                    var result = BuildGithub.UploadAsset(
                        newGithubRelease.UploadUrl,
                        deployfile.FileName,
                        sourceName,
                        label
                        );

                    if (result == null)
                    {
                        Program.PrintColorMessage("[Github.UploadAssets]", ConsoleColor.Red);
                        return null;
                    }

                    //mirror.Files.Add(new GithubReleaseAsset(file.FileName, result.Download_url));

                    if (string.IsNullOrWhiteSpace(result.HashValue))
                    {
                        Program.PrintColorMessage("[UpdateAssetsResponse] HashValue null.", ConsoleColor.Red);
                        return null;
                    }
                    if (string.IsNullOrWhiteSpace(result.DownloadUrl))
                    {
                        Program.PrintColorMessage("[UpdateAssetsResponse] DownloadUrl null.", ConsoleColor.Red);
                        return null;
                    }

                    //deployfile.UpdateAssetsResponse(result);
                }

                // Update the release and make it public.

                var update = BuildGithub.UpdateRelease(newGithubRelease.ReleaseId, false, false);

                if (update == null)
                {
                    Program.PrintColorMessage("[Github.UpdateRelease]", ConsoleColor.Red);
                    return null;
                }

                // Get the release information.

                var release = BuildGithub.GetRelease(update.ReleaseId);

                if (release == null)
                {
                    Program.PrintColorMessage("[Github.GetRelease]", ConsoleColor.Red);
                    return null;
                }

                var mirror = new GithubRelease(release.ReleaseId);

                // Grab the download urls.

                foreach (var file in release.Assets)
                {
                    if (!file.Uploaded)
                    {
                        Program.PrintColorMessage($"[GithubReleaseAsset-NotUploaded] {file.Name}", ConsoleColor.Red);
                        return null;
                    }

                    var deployFile = Files.Find(f => f.Name.Equals(file.Name, StringComparison.OrdinalIgnoreCase));
                    if (deployFile == null)
                    {
                        Program.PrintColorMessage($"[GithubReleaseAsset-MissingDeployFile] {file.Name}", ConsoleColor.Red);
                        return null;
                    }

                    if (!deployFile.UpdateAssetsResponse(file))
                    {
                        Program.PrintColorMessage($"[GithubReleaseAsset-UpdateAssetsResponse] {file.Name}", ConsoleColor.Red);
                        return null;
                    }

                    mirror.Files.Add(new GithubReleaseAsset(file.Name, file.DownloadUrl, deployFile));
                }

                return mirror;
            }
            catch (Exception ex)
            {
                Program.PrintColorMessage($"[Github] {ex}", ConsoleColor.Red);
                return null;
            }
        }

        /// <summary>
        /// Updates the server configuration with build and asset information after uploading to GitHub.
        /// </summary>
        /// <param name="BinFile">The DeployFile for the binary zip.</param>
        /// <param name="ReleaseFile">The DeployFile for the release setup executable.</param>
        /// <param name="CanaryFile">The DeployFile for the canary setup executable.</param>
        /// <param name="GithubReleaseId">The GitHub release ID.</param>
        /// <returns>True if the server configuration is updated successfully; otherwise, false.</returns>
        private static bool BuildUploadServerConfig(
            DeployFile BinFile,
            DeployFile ReleaseFile,
            DeployFile CanaryFile,
            string GithubReleaseId
            )
        {
            if (!Build.BuildCanary)
                return true;
            if (Build.BuildToolsDebug)
                return true;

            if (!Win32.GetEnvironmentVariable("BUILD_BUILDID", out string buildBuildId))
                return false;
            if (!Win32.GetEnvironmentVariable("BUILD_CF_API", out string buildPostUrl))
                return false;
            if (!Win32.GetEnvironmentVariable("BUILD_CF_KEY", out string buildPostKey))
                return false;

            if (string.IsNullOrWhiteSpace(buildBuildId) ||
                string.IsNullOrWhiteSpace(buildPostUrl) ||
                string.IsNullOrWhiteSpace(buildPostKey) ||
                string.IsNullOrWhiteSpace(GithubReleaseId))
            {
                Program.PrintColorMessage("BuildUploadServerConfig invalid config.", ConsoleColor.Red);
                return false;
            }

            if (string.IsNullOrWhiteSpace(BinFile.DownloadLink) ||
                string.IsNullOrWhiteSpace(ReleaseFile.DownloadLink) ||
                string.IsNullOrWhiteSpace(CanaryFile.DownloadLink))
            {
                Program.PrintColorMessage("BuildUploadServerConfig invalid DownloadLinks.", ConsoleColor.Red);
                return false;
            }
            if (string.IsNullOrWhiteSpace(BinFile.FileSignature) ||
                string.IsNullOrWhiteSpace(ReleaseFile.FileSignature) ||
                string.IsNullOrWhiteSpace(CanaryFile.FileSignature))
            {
                Program.PrintColorMessage("BuildUploadServerConfig invalid FileSignatures.", ConsoleColor.Red);
                return false;
            }
            if (string.IsNullOrWhiteSpace(BinFile.FileHash) ||
                string.IsNullOrWhiteSpace(ReleaseFile.FileHash) ||
                string.IsNullOrWhiteSpace(CanaryFile.FileHash))
            {
                Program.PrintColorMessage("BuildUploadServerConfig invalid FileHashs.", ConsoleColor.Red);
                return false;
            }

            try
            {
                BuildUpdateRequest buildUpdateRequest = new BuildUpdateRequest
                {
                    BuildUpdated = Build.BuildUpdated,
                    BuildDisplay = Build.BuildShortVersion,
                    BuildVersion = Build.BuildLongVersion,
                    BuildCommit = Build.BuildCommitHash,
                    BuildGithubId = GithubReleaseId,
                    BuildId = buildBuildId,
                    BinUrl = BinFile.DownloadLink,
                    BinLength = BinFile.FileLength,
                    BinHash = BinFile.FileHash,
                    BinSig = BinFile.FileSignature,
                    ReleaseSetupUrl = ReleaseFile.DownloadLink,
                    ReleaseSetupLength = ReleaseFile.FileLength,
                    ReleaseSetupHash = ReleaseFile.FileHash,
                    ReleaseSetupSig = ReleaseFile.FileSignature,
                    CanarySetupUrl = CanaryFile.DownloadLink,
                    CanarySetupLength = CanaryFile.FileLength,
                    CanarySetupHash = CanaryFile.FileHash,
                    CanarySetupSig = CanaryFile.FileSignature,
                };

                using (HttpRequestMessage requestMessage = new HttpRequestMessage(HttpMethod.Post, buildPostUrl))
                {
                    requestMessage.Headers.Accept.Add(new MediaTypeWithQualityHeaderValue("application/json"));
                    requestMessage.Headers.Add("X-ApiKey", buildPostKey);
                    requestMessage.VersionPolicy = HttpVersionPolicy.RequestVersionOrHigher;
                    requestMessage.Version = HttpVersion.Version20;

                    requestMessage.Content = new ByteArrayContent(buildUpdateRequest.SerializeToBytes());
                    requestMessage.Content.Headers.ContentType = new MediaTypeHeaderValue("application/json");

                    var httpResult = BuildHttpClient.SendMessage(requestMessage);

                    if (string.IsNullOrWhiteSpace(httpResult))
                    {
                        Program.PrintColorMessage("[UpdateBuildWebService-SF-NullOrWhiteSpace]", ConsoleColor.Red);
                        return false;
                    }

                    if (!httpResult.Equals("OK", StringComparison.OrdinalIgnoreCase))
                    {
                        Program.PrintColorMessage($"[UpdateBuildWebService-SF] {httpResult}", ConsoleColor.Red);
                        return false;
                    }
                }
            }
            catch (Exception ex)
            {
                Program.PrintColorMessage($"[UpdateBuildWebService-SF] {ex}", ConsoleColor.Red);
                return false;
            }

            return true;
        }

        /// <summary>
        /// Generates MSIX package manifest files for the specified build architectures based on the provided build
        /// flags.
        /// </summary>
        /// <remarks>This method creates separate manifest files for each architecture indicated in the
        /// <paramref name="Flags"/> parameter. The generated files are written to the tools\msix directory and are
        /// based on a template manifest file. Existing manifest files with the same names will be
        /// overwritten.</remarks>
        /// <param name="Flags">A set of flags that specify which architectures (such as 32-bit or 64-bit) to generate MSIX package
        /// manifests for.</param>
        private static void BuildMsixPackageManifest(BuildFlags Flags)
        {
            // Create the package manifest.

            if (Flags.HasFlag(BuildFlags.Build32bit))
            {
                string msixManifestString = Utils.ReadAllText("tools\\msix\\PackageTemplate.msix.xml");

                msixManifestString = msixManifestString.Replace("SI_MSIX_ARCH", "x86");
                msixManifestString = msixManifestString.Replace("SI_MSIX_VERSION", Build.BuildLongVersion);
                msixManifestString = msixManifestString.Replace("SI_MSIX_PUBLISHER", "CN=Winsider Seminars &amp; Solutions Inc., O=Winsider Seminars &amp; Solutions Inc., L=Montr&#233;al, S=Quebec, C=CA");

                Utils.WriteAllText("tools\\msix\\MsixManifest32.xml", msixManifestString);
            }

            if (Flags.HasFlag(BuildFlags.Build64bit))
            {
                string msixManifestString = Utils.ReadAllText("tools\\msix\\PackageTemplate.msix.xml");

                msixManifestString = msixManifestString.Replace("SI_MSIX_ARCH", "x64");
                msixManifestString = msixManifestString.Replace("SI_MSIX_VERSION", Build.BuildLongVersion);
                msixManifestString = msixManifestString.Replace("SI_MSIX_PUBLISHER", "CN=Winsider Seminars &amp; Solutions Inc., O=Winsider Seminars &amp; Solutions Inc., L=Montr&#233;al, S=Quebec, C=CA");

                Utils.WriteAllText("tools\\msix\\MsixManifest64.xml", msixManifestString);
            }
        }

        /// <summary>
        /// Creates the MSIX package mapping files for 32-bit and 64-bit builds, listing files to include in the package.
        /// </summary>
        /// <param name="Flags">Build flags indicating which configurations to process.</param>
        private static void BuildMsixPackageMapping(BuildFlags Flags)
        {
            // Create the package mapping file.
            string Build_Base_Path = GetBuildBaseDirectory(Flags);
            string release32Path = Path.Join([Build_Base_Path, "\\Release32"]);
            string release64Path = Path.Join([Build_Base_Path, "\\Release64"]);

            if (Flags.HasFlag(BuildFlags.Build32bit) && Directory.Exists(release32Path))
            {
                StringBuilder packageMap32 = new StringBuilder(0x100);
                packageMap32.AppendLine("[Files]");
                packageMap32.AppendLine("\"tools\\msix\\MsixManifest32.xml\" \"AppxManifest.xml\"");
                packageMap32.AppendLine("\"tools\\msix\\Square44x44Logo.png\" \"Assets\\Square44x44Logo.png\"");
                packageMap32.AppendLine("\"tools\\msix\\Square50x50Logo.png\" \"Assets\\Square50x50Logo.png\"");
                packageMap32.AppendLine("\"tools\\msix\\Square150x150Logo.png\" \"Assets\\Square150x150Logo.png\"");

                foreach (string filePath in Directory.EnumerateFiles(release32Path, "*", SearchOption.AllDirectories))
                {
                    if (filePath.EndsWith(".pdb", StringComparison.OrdinalIgnoreCase) ||
                        filePath.EndsWith(".iobj", StringComparison.OrdinalIgnoreCase) ||
                        filePath.EndsWith(".ipdb", StringComparison.OrdinalIgnoreCase) ||
                        filePath.EndsWith(".exp", StringComparison.OrdinalIgnoreCase) ||
                        filePath.EndsWith(".lib", StringComparison.OrdinalIgnoreCase) ||
                        filePath.EndsWith(".xml", StringComparison.OrdinalIgnoreCase) ||
                        filePath.EndsWith(".manifest", StringComparison.OrdinalIgnoreCase)
                        )
                    {
                        continue;
                    }

                    packageMap32.AppendLine($"\"{filePath}\" \"{filePath.AsSpan((release32Path + "\\").Length)}\"");
                }

                Utils.WriteAllText("tools\\msix\\MsixPackage32.map", packageMap32.ToString());
            }

            // Create the package mapping file.

            if (Flags.HasFlag(BuildFlags.Build64bit) && Directory.Exists(release64Path))
            {
                StringBuilder packageMap64 = new StringBuilder(0x100);
                packageMap64.AppendLine("[Files]");
                packageMap64.AppendLine("\"tools\\msix\\MsixManifest64.xml\" \"AppxManifest.xml\"");
                packageMap64.AppendLine("\"tools\\msix\\Square44x44Logo.png\" \"Assets\\Square44x44Logo.png\"");
                packageMap64.AppendLine("\"tools\\msix\\Square50x50Logo.png\" \"Assets\\Square50x50Logo.png\"");
                packageMap64.AppendLine("\"tools\\msix\\Square150x150Logo.png\" \"Assets\\Square150x150Logo.png\"");

                foreach (string filePath in Directory.EnumerateFiles(release64Path, "*", SearchOption.AllDirectories))
                {
                    // Ignore junk files
                    if (filePath.EndsWith(".pdb", StringComparison.OrdinalIgnoreCase) ||
                        filePath.EndsWith(".iobj", StringComparison.OrdinalIgnoreCase) ||
                        filePath.EndsWith(".ipdb", StringComparison.OrdinalIgnoreCase) ||
                        filePath.EndsWith(".exp", StringComparison.OrdinalIgnoreCase) ||
                        filePath.EndsWith(".lib", StringComparison.OrdinalIgnoreCase) ||
                        filePath.EndsWith(".xml", StringComparison.OrdinalIgnoreCase) ||
                        filePath.EndsWith(".manifest", StringComparison.OrdinalIgnoreCase)
                        )
                    {
                        continue;
                    }

                    packageMap64.AppendLine($"\"{filePath}\" \"{filePath.AsSpan((release64Path + "\\").Length)}\"");
                }

                Utils.WriteAllText("tools\\msix\\MsixPackage64.map", packageMap64.ToString());
            }
        }

        /// <summary>
        /// Builds the MSIX package.
        /// </summary>
        /// <param name="Flags">Build flags indicating which configurations to process.</param>
        /// <returns>True if the MSIX package is built successfully; otherwise, false.</returns>
        public static bool BuildStorePackage(BuildFlags Flags)
        {
            // Cleanup package manifests.

            Win32.DeleteFile($"{BuildOutputFolder}\\systeminformer-build-package-x32.appx", Flags);
            Win32.DeleteFile($"{BuildOutputFolder}\\systeminformer-build-package-x64.appx", Flags);
            Win32.DeleteFile($"{BuildOutputFolder}\\systeminformer-build-package.msixbundle", Flags);
            Win32.DeleteFile($"{BuildWorkingFolder}\\tools\\msix\\MsixManifest32.xml", Flags);
            Win32.DeleteFile($"{BuildWorkingFolder}\\tools\\msix\\MsixManifest64.xml", Flags);
            Win32.DeleteFile($"{BuildWorkingFolder}\\tools\\msix\\MsixPackage32.map", Flags);
            Win32.DeleteFile($"{BuildWorkingFolder}\\tools\\msix\\MsixPackage64.map", Flags);
            Win32.DeleteFile($"{BuildWorkingFolder}\\tools\\msix\\bundle.map", Flags);
            Utils.CreateOutputDirectory();

            // Create the package manifests.

            BuildMsixPackageManifest(Flags);
            BuildMsixPackageMapping(Flags);

            // Create the MSIX package.

            if (Flags.HasFlag(BuildFlags.Build32bit) && File.Exists("tools\\msix\\MsixPackage32.map"))
            {
                Program.PrintColorMessage(BuildTimeSpan(), ConsoleColor.DarkGray, false, Flags);
                Program.PrintColorMessage("Building systeminformer-build-package-x64.msix...", ConsoleColor.Cyan, false);

                string result = Utils.ExecuteMsixCommand(
                    $"pack /o /f {BuildWorkingFolder}\\tools\\msix\\MsixPackage32.map /p {BuildOutputFolder}\\systeminformer-build-package-x32.msix"
                    );

                if (!result.EndsWith("Package creation succeeded.", StringComparison.OrdinalIgnoreCase))
                {
                    Program.PrintColorMessage(result, ConsoleColor.Gray);
                    return false;
                }

                Program.PrintColorMessage(Win32.GetFileSize($"{BuildOutputFolder}\\systeminformer-build-package-x32.msix").ToPrettySize(), ConsoleColor.Green);
            }

            if (Flags.HasFlag(BuildFlags.Build64bit) && File.Exists("tools\\msix\\MsixPackage64.map"))
            {
                Program.PrintColorMessage(BuildTimeSpan(), ConsoleColor.DarkGray, false, Flags);
                Program.PrintColorMessage("Building systeminformer-build-package-x64.msix...", ConsoleColor.Cyan, false);

                string result = Utils.ExecuteMsixCommand(
                    $"pack /o /f {BuildWorkingFolder}\\tools\\msix\\MsixPackage64.map /p {BuildOutputFolder}\\systeminformer-build-package-x64.msix"
                    );

                if (!result.EndsWith("Package creation succeeded.", StringComparison.OrdinalIgnoreCase))
                {
                    Program.PrintColorMessage(result, ConsoleColor.Gray);
                    return false;
                }

                Program.PrintColorMessage(Win32.GetFileSize($"{BuildOutputFolder}\\systeminformer-build-package-x64.msix").ToPrettySize(), ConsoleColor.Green);
            }

            // Create the bundle package.

            if (
                File.Exists($"{BuildOutputFolder}\\systeminformer-build-package-x32.msix") &&
                File.Exists($"{BuildOutputFolder}\\systeminformer-build-package-x64.msix")
                )
            {
                Program.PrintColorMessage(BuildTimeSpan(), ConsoleColor.DarkGray, false, Flags);
                Program.PrintColorMessage("Building systeminformer-build-package.msixbundle...", ConsoleColor.Cyan, false);

                {
                    StringBuilder bundleMap = new StringBuilder(0x100);
                    bundleMap.AppendLine("[Files]");
                    bundleMap.AppendLine($"\"{BuildOutputFolder}\\systeminformer-build-package-x32.msix\" \"systeminformer-build-package-x32.msix\"");
                    bundleMap.AppendLine($"\"{BuildOutputFolder}\\systeminformer-build-package-x64.msix\" \"systeminformer-build-package-x64.msix\"");
                    Utils.WriteAllText($"{BuildWorkingFolder}\\tools\\msix\\bundle.map", bundleMap.ToString());
                }

                string result = Utils.ExecuteMsixCommand(
                    $"bundle /f {BuildWorkingFolder}\\tools\\msix\\bundle.map /p {BuildOutputFolder}\\systeminformer-build-package.msixbundle"
                    );

                if (!result.EndsWith("Bundle creation succeeded.", StringComparison.OrdinalIgnoreCase))
                {
                    Program.PrintColorMessage($"{result}", ConsoleColor.Gray);
                    return false;
                }

                Program.PrintColorMessage(Win32.GetFileSize($"{BuildOutputFolder}\\systeminformer-build-package-x64.msix").ToPrettySize(), ConsoleColor.Green);
            }

            if (File.Exists("tools\\msix\\PackageTemplate.appinstaller"))
            {
                string msixAppInstallerString = Utils.ReadAllText("tools\\msix\\PackageTemplate.appinstaller");

                msixAppInstallerString = msixAppInstallerString.Replace("Version=\"3.0.0.0\"", $"Version=\"{Build.BuildLongVersion}\"");

                Utils.WriteAllText("build\\output\\SystemInformer.appinstaller", msixAppInstallerString);
            }

            return true;
        }

        /// <summary>
        /// Copies or deletes the source link file for the build, depending on the parameter.
        /// </summary>
        /// <param name="CreateSourceLink">If true, creates the source link file; if false, deletes it.</param>
        public static void CopySourceLink(bool CreateSourceLink)
        {
            if (CreateSourceLink)
            {
                if (!string.IsNullOrWhiteSpace(Build.BuildCommitHash))
                {
                    Build.BuildSourceLink = $"{Build.BuildWorkingFolder}\\sourcelink.json";

                    JsonObject jsonObject = new JsonObject
                    {
                        ["documents"] = new JsonObject
                        {
                            ["*"] = $"https://raw.githubusercontent.com/winsiderss/systeminformer/{Build.BuildCommitHash}/*",
                            [$"{Path.Join([Build.BuildWorkingFolder, "\\"])}*"] = $"https://raw.githubusercontent.com/winsiderss/systeminformer/{Build.BuildCommitHash}/*"
                        }
                    };

                    Utils.WriteAllText(Build.BuildSourceLink, jsonObject.ToJsonString(new JsonSerializerOptions { WriteIndented = true }));
                }
            }
            else
            {
                Win32.DeleteFile(Build.BuildSourceLink);
            }
        }

        /// <summary>
        /// Cleans up the build environment by removing generated files, build output directories, and temporary folders
        /// related to the build process.
        /// </summary>
        /// <remarks>This method attempts to delete common build artifacts such as output folders, SDK
        /// directories, '.vs' and 'obj' folders, and files with the '.aps' extension. Any errors encountered during
        /// deletion are logged, but the method continues processing remaining items. This method is intended to be used
        /// before or after a build to ensure a clean working state.</remarks>
        /// <returns>true if the cleanup process completes; otherwise, false.</returns>
        public static bool CleanupBuildEnvironment()
        {
            try
            {
                if (!string.IsNullOrWhiteSpace(Utils.GetGitFilePath()))
                {
                    string output = Utils.ExecuteGitCommand(BuildWorkingFolder, "clean -x -d -f");

                    Program.PrintColorMessage(output, ConsoleColor.DarkGray);
                }

                if (Directory.Exists(BuildOutputFolder)) // output
                {
                    Program.PrintColorMessage($"Deleting: {BuildOutputFolder}", ConsoleColor.DarkGray);
                    Directory.Delete(BuildOutputFolder, true);
                }

                if (Directory.Exists(BuildConfig.Build_Sdk_Directories[0])) // sdk
                {
                    Program.PrintColorMessage($"Deleting: {BuildConfig.Build_Sdk_Directories[0]}", ConsoleColor.DarkGray);
                    Directory.Delete(BuildConfig.Build_Sdk_Directories[0], true);
                }

                //foreach (BuildFile file in BuildConfig.Build_Release_Files)
                //{
                //    string sourceFile = BuildOutputFolder + file.FileName;
                //
                //    Win32.DeleteFile(sourceFile);
                //}
                //
                //foreach (string folder in BuildConfig.Build_Sdk_Directories)
                //{
                //    if (Directory.Exists(folder))
                //        Directory.Delete(folder, true);
                //}

                var project_folders = Directory.EnumerateDirectories(".", "*", new EnumerationOptions
                {
                    AttributesToSkip = FileAttributes.Offline,
                    RecurseSubdirectories = true,
                    ReturnSpecialDirectories = false
                });

                foreach (string folder in project_folders)
                {
                    string path = Path.GetFullPath(folder);
                    var name = Path.GetFileName(path.AsSpan());

                    if (
                        name.Equals(".vs", StringComparison.OrdinalIgnoreCase) ||
                        name.Equals("obj", StringComparison.OrdinalIgnoreCase)
                        )
                    {
                        if (Directory.Exists(path))
                        {
                            Program.PrintColorMessage($"Deleting: {path}", ConsoleColor.DarkGray);

                            try
                            {
                                Directory.Delete(path, true);
                            }
                            catch (Exception ex)
                            {
                                Program.PrintColorMessage($"[WARN] {ex}", ConsoleColor.Yellow);
                            }
                        }
                    }
                }

                // Delete files with abs

                var res_files = Directory.EnumerateFiles(".", "*.aps", new EnumerationOptions
                {
                    AttributesToSkip = FileAttributes.Offline,
                    RecurseSubdirectories = true,
                    ReturnSpecialDirectories = false
                });

                foreach (string file in res_files)
                {
                    string path = Path.GetFullPath(file);
                    var name = Path.GetFileName(path.AsSpan());

                    if (name.EndsWith(".aps", StringComparison.OrdinalIgnoreCase))
                    {
                        try
                        {
                            Win32.DeleteFile(path, BuildFlags.BuildVerbose);
                        }
                        catch (Exception ex)
                        {
                            Program.PrintColorMessage($"[WARN] {ex}", ConsoleColor.Yellow);
                        }
                    }
                }
            }
            catch (Exception ex)
            {
                Program.PrintColorMessage($"[Cleanup] {ex}", ConsoleColor.Red);
            }

            return true;
        }

        private const string ExportHeader = @"/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2008-2016
 *     dmex    2017-2024
 *
 *
 * This file was automatically generated.
 *
 * Do not link at runtime. Use the SystemInformer.def.h header file instead.
 *
 */

#pragma once

#ifndef _PH_EXPORT_DEF_H
#define _PH_EXPORT_DEF_H
";

        private const string ExportFooter = @"
#endif _PH_EXPORT_DEF_H
";

        /// <summary>
        /// Exports the current function and data definitions to the SystemInformer module definition (.def) and header
        /// (.def.h) files, updating them if changes are detected.
        /// </summary>
        /// <remarks>This method reads the existing SystemInformer.def file, processes its contents, and
        /// writes updated .def and .def.h files only if changes are detected. The original .def file is backed up as
        /// SystemInformer.def.bak before being overwritten. Use the ReleaseBuild parameter to control whether export
        /// ordinals are randomized (for release builds) or assigned sequentially (for development or
        /// debugging).</remarks>
        /// <param name="ReleaseBuild">true to generate randomized export ordinals for a release build; false to assign sequential ordinals
        /// starting from 1001.</param>
        public static void ExportDefinitions(bool ReleaseBuild)
        {
            List<int> ordinals = [];
            StringBuilder output = new StringBuilder();
            StringBuilder output_header = new StringBuilder();

            var content = Utils.ReadAllText("SystemInformer\\SystemInformer.def");
            var lines = content.Split("\r\n");
            int total = lines.Length;

            //if (ReleaseBuild)
            //{
            //    while (ordinals.Count < total)
            //    {
            //        var value = Random.Shared.Next(1000, 1000 + total);
            //
            //        if (!ordinals.Contains(value))
            //        {
            //            ordinals.Add(value);
            //        }
            //    }
            //}
            //else
            {
                while (ordinals.Count < total)
                {
                    ordinals.Add(1000 + ordinals.Count + 1);
                }
            }

            output_header.AppendLine(ExportHeader);

            foreach (string line in lines)
            {
                var span = line.AsSpan();

                if (span.IsWhiteSpace())
                {
                    output.Append(span);
                    output.AppendLine();
                }
                else
                {
                    if (span.StartsWith("    ", StringComparison.OrdinalIgnoreCase))
                    {
                        var ordinal = ordinals[0]; ordinals.RemoveAt(0);
                        var name_end = span.Slice(4).IndexOf(' ');
                        if (name_end == -1)
                            name_end = span.Slice(4).Length;
                        var name = span.Slice(4, name_end).ToString();

                        if (span.IndexOf(" DATA", StringComparison.OrdinalIgnoreCase) != -1)
                            output.AppendLine($"    {name,-55} @{ordinal,-5} NONAME DATA");
                        else
                            output.AppendLine($"    {name,-55} @{ordinal,-5} NONAME");

                        output_header.AppendLine($"#define EXPORT_{name.ToUpper(),-55} {ordinal}");
                    }
                    else
                    {
                        output.Append(span);
                        output.AppendLine();
                    }
                }
            }

            output_header.AppendLine(ExportFooter);

            string export_content = output.ToString().TrimEnd();
            string export_header = output_header.ToString().TrimEnd();

            // Only write to the file if it has changed.
            if (!string.Equals(content, export_content, StringComparison.OrdinalIgnoreCase))
            {
                Utils.WriteAllText("SystemInformer\\SystemInformer.def", export_content);
                Utils.WriteAllText("SystemInformer\\SystemInformer.def.h", export_header);
                Utils.WriteAllText("SystemInformer\\SystemInformer.def.bak", content);
            }
        }

        /// <summary>
        /// Reverts the export definitions to the previous backup if available.
        /// </summary>
        public static void ExportDefinitionsRevert()
        {
            try
            {
                if (File.Exists("SystemInformer\\SystemInformer.def.bak"))
                {
                    File.Move("SystemInformer\\SystemInformer.def.bak", "SystemInformer\\SystemInformer.def", true);
                }
            }
            catch (Exception ex)
            {
                Program.PrintColorMessage($"[WARN] {ex}", ConsoleColor.Yellow);
            }
        }
    }
}
