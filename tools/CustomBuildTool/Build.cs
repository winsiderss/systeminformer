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
        public static bool BuildCanary = false;
        public static bool BuildToolsDebug = false;
        public static bool HaveArm64BuildTools = true;
        public static bool BuildRedirectOutput = false;
        public static bool BuildIntegration = false;
        public static bool BuildIntegrationTF = false;
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

        public static bool InitializeBuildEnvironment()
        {
            Win32.SetErrorMode();
            Win32.SetBasePriority();

            Console.InputEncoding = Utils.UTF8NoBOM;
            Console.OutputEncoding = Utils.UTF8NoBOM;

            if (!Utils.SetCurrentDirectoryParent("SystemInformer.sln"))
            {
                Program.PrintColorMessage("Unable to find project solution.", ConsoleColor.Red);
                return false;
            }

            Build.BuildWorkingFolder = Environment.CurrentDirectory;
            Build.BuildOutputFolder = Utils.GetOutputDirectoryPath("\\build\\output");

            if (!InitializeBuildTimestamp())
            {
                Program.PrintColorMessage("Unable to initialize build timestamp.", ConsoleColor.Red);
                return false;
            }

            if (!InitializeBuildArguments())
            {
                Program.PrintColorMessage("Unable to initialize build arguments.", ConsoleColor.Red);
                return false;
            }

            return true;
        }

        public static bool InitializeBuildTimestamp()
        {
            // Ensures consistent time stamp across build invocations.
            if (Build.BuildIntegration)
                Build.TimeStart = BuildDevOps.BuildQueryQueueTime();
            else
                Build.TimeStart = DateTime.UtcNow;

            return true;
        }

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

            if (!Win32.GetEnvironmentVariable("BUILD_SOURCEVERSION", out Build.BuildCommitHash))
            {
                Build.BuildCommitHash = Utils.ExecuteGitCommand(Build.BuildWorkingFolder, "rev-parse HEAD");
            }

            if (!Win32.GetEnvironmentVariable("BUILD_SOURCEBRANCHNAME", out Build.BuildCommitBranch))
            {
                Build.BuildCommitBranch = Utils.ExecuteGitCommand(Build.BuildWorkingFolder, "branch --show-current");
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

            if (Win32.HasEnvironmentVariable("GITHUB_ACTIONS"))
            {
                Build.BuildIntegration = true;
            }
            else if (Win32.HasEnvironmentVariable("TF_BUILD"))
            {
                Build.BuildIntegration = true;
                Build.BuildIntegrationTF = true;
            }

            if (!string.IsNullOrWhiteSpace(Build.BuildCommitHash) && !string.IsNullOrWhiteSpace(Build.BuildVersionMajor))
            {
                Build.BuildShortVersion = $"{Build.BuildVersionMajor}.{Build.BuildVersionMinor}.{Build.BuildVersionBuild}";
                Build.BuildLongVersion = $"{Build.BuildVersionMajor}.{Build.BuildVersionMinor}.{Build.BuildVersionBuild}.{Build.BuildVersionRevision}";
            }

            return true;
        }

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
                    //Program.PrintColorMessage(Build.BuildCommitHash[..8], ConsoleColor.DarkGray, false);
                    Program.PrintColorMessage(Program.CreateConsoleHyperlink(
                        $"https://github.com/winsiderss/systeminformer/commit/{Build.BuildCommitHash}", 
                        Build.BuildCommitHash[..8]), ConsoleColor.Blue, false);
                    Program.PrintColorMessage(")", ConsoleColor.DarkGray, false);
                }

                if (!string.IsNullOrWhiteSpace(Build.BuildCommitBranch) && !Build.BuildCommitBranch.Equals("master"))
                {
                    Program.PrintColorMessage(" [", ConsoleColor.DarkGray, false);
                    Program.PrintColorMessage(Build.BuildCommitBranch, ConsoleColor.Blue, false);
                    Program.PrintColorMessage("]", ConsoleColor.DarkGray, false);
                }

                Program.PrintColorMessage(Environment.NewLine, ConsoleColor.DarkGray);
            }
        }

        public static string BuildTimeSpan()
        {
            return $"[{DateTime.UtcNow - Build.TimeStart:mm\\:ss}] ";
        }

        public static string BuildVersionBuild
        {
            get
            {
                // Extract the last two digits of the year. For example, if the year is 2023, (Year % 100) will result in 23.
                var first = (TimeStart.Year % 100).ToString();
                // Extract the day of the year (1 to 365 or 366 in a leap year). Padding with leading zeros if necessary.
                // For example, if the day of the year is 5, it will be converted to "005".
                var second = TimeStart.DayOfYear.ToString("D3");
                // Format the strings: 23005
                return string.Concat([first, second]);
            }
        }

        public static string BuildVersionRevision
        {
            get
            {
                // Extract the hour of the day. For example, if the hour is 0|23, (Hour + 1) will result in 1|24.
                var first = (TimeStart.Hour + 1).ToString();
                // Extract the minute of the hour (0 to 59). Padding with leading zeros if necessary.
                // For example, if the minute of the hour is 5, it will be converted to "005".
                var second = TimeStart.Minute.ToString("D2");
                // Format the strings: 1005|24005
                return string.Concat([first, second]);
            }
        }

        public static string BuildHash()
        {
            return string.IsNullOrWhiteSpace(Build.BuildCommitHash) ? "00000000" : Build.BuildCommitHash[..8];
        }

        public static string BuildUpdated
        {
            get { return new DateTime(TimeStart.Year, TimeStart.Month, TimeStart.Day, TimeStart.Hour, 0, 0).ToString("o"); }
        }

        public static DateTime BuildDateTime
        {
            get { return new DateTime(TimeStart.Year, TimeStart.Month, TimeStart.Day, TimeStart.Hour, 0, 0); }
        }

        public static void ShowBuildStats()
        {
            TimeSpan buildTime = DateTime.UtcNow - Build.TimeStart;

            Program.PrintColorMessage($"{Environment.NewLine}Build Time: ", ConsoleColor.DarkGray, false);
            Program.PrintColorMessage(buildTime.TotalMinutes.ToString("0"), ConsoleColor.Green, false);
            Program.PrintColorMessage(" minute(s), ", ConsoleColor.DarkGray, false);
            Program.PrintColorMessage(buildTime.Seconds.ToString(), ConsoleColor.Green, false);
            Program.PrintColorMessage($" second(s) {Environment.NewLine + Environment.NewLine}", ConsoleColor.DarkGray, false);
        }

        public static bool CopyTextFiles(bool Update, BuildFlags Flags)
        {
            string[] Build_Text_Files =
            [
                "README.txt",
                "COPYRIGHT.txt",
                "LICENSE.txt",
            ];

            if (Update)
            {
                foreach (string file in Build_Text_Files)
                {
                    if (Flags.HasFlag(BuildFlags.BuildRelease))
                    {
                        if (Flags.HasFlag(BuildFlags.Build32bit))
                            Win32.CopyIfNewer(file, $"bin\\Release32\\{file}", Flags);
                        if (Flags.HasFlag(BuildFlags.Build64bit))
                            Win32.CopyIfNewer(file, $"bin\\Release64\\{file}", Flags);
                        if (Flags.HasFlag(BuildFlags.BuildArm64bit))
                            Win32.CopyIfNewer(file, $"bin\\ReleaseARM64\\{file}", Flags);
                    }
                }
            }
            else
            {
                foreach (string file in Build_Text_Files)
                {
                    if (Flags.HasFlag(BuildFlags.BuildRelease))
                    {
                        if (Flags.HasFlag(BuildFlags.Build32bit))
                            Win32.DeleteFile($"bin\\Release32\\{file}", Flags);
                        if (Flags.HasFlag(BuildFlags.Build64bit))
                            Win32.DeleteFile($"bin\\Release64\\{file}", Flags);
                        if (Flags.HasFlag(BuildFlags.BuildArm64bit))
                            Win32.DeleteFile($"bin\\ReleaseARM64\\{file}", Flags);
                    }
                }
            }

            return true;
        }

        public static bool CopyWow64Files(BuildFlags Flags)
        {
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

            foreach (string file in Build_Wow64_Files)
            {
                if (Flags.HasFlag(BuildFlags.BuildDebug))
                {
                    if (Flags.HasFlag(BuildFlags.Build64bit))
                        Win32.CopyIfNewer($"bin\\Debug32\\{file}", $"bin\\Debug64\\x86\\{file}", Flags);
                    if (Flags.HasFlag(BuildFlags.BuildArm64bit))
                        Win32.CopyIfNewer($"bin\\Debug32\\{file}", $"bin\\DebugARM64\\x86\\{file}", Flags);
                }

                if (Flags.HasFlag(BuildFlags.BuildRelease))
                {
                    if (Flags.HasFlag(BuildFlags.Build64bit))
                        Win32.CopyIfNewer($"bin\\Release32\\{file}", $"bin\\Release64\\x86\\{file}", Flags);
                    if (Flags.HasFlag(BuildFlags.BuildArm64bit))
                        Win32.CopyIfNewer($"bin\\Release32\\{file}", $"bin\\ReleaseARM64\\x86\\{file}", Flags);
                }
            }

            return true;
        }

        public static bool CopyResourceFiles(BuildFlags Flags)
        {
            var Build_Resource_Files = new Dictionary<string, string>(4, StringComparer.OrdinalIgnoreCase)
            {
                ["SystemInformer.png"] = "icon.png",
                ["CapsList.txt"] = "CapsList.txt",
                ["EtwGuids.txt"] = "EtwGuids.txt",
                ["PoolTag.txt"] = "PoolTag.txt",
            };

            foreach (var file in Build_Resource_Files)
            {
                if (Flags.HasFlag(BuildFlags.BuildDebug))
                {
                    if (Flags.HasFlag(BuildFlags.Build32bit))
                    {
                        Win32.CopyIfNewer(
                            $"{BuildWorkingFolder}\\SystemInformer\\resources\\{file.Key}",
                            $"{BuildWorkingFolder}\\bin\\Debug32\\Resources\\{file.Value}",
                            Flags);
                    }

                    if (Flags.HasFlag(BuildFlags.Build64bit))
                    {
                        Win32.CopyIfNewer(
                            $"{BuildWorkingFolder}\\SystemInformer\\resources\\{file.Key}",
                            $"{BuildWorkingFolder}\\bin\\Debug64\\Resources\\{file.Value}",
                            Flags);
                    }

                    if (Flags.HasFlag(BuildFlags.BuildArm64bit))
                    {
                        Win32.CopyIfNewer(
                            $"{BuildWorkingFolder}\\SystemInformer\\resources\\{file.Key}",
                            $"{BuildWorkingFolder}\\bin\\DebugARM64\\Resources\\{file.Value}",
                            Flags);
                    }
                }

                if (Flags.HasFlag(BuildFlags.BuildRelease))
                {
                    if (Flags.HasFlag(BuildFlags.Build32bit))
                    {
                        Win32.CopyIfNewer(
                            $"{BuildWorkingFolder}\\SystemInformer\\resources\\{file.Key}",
                            $"{BuildWorkingFolder}\\bin\\Release32\\Resources\\{file.Value}",
                            Flags);
                    }

                    if (Flags.HasFlag(BuildFlags.Build64bit))
                    {
                        Win32.CopyIfNewer(
                            $"{BuildWorkingFolder}\\SystemInformer\\resources\\{file.Key}",
                            $"{BuildWorkingFolder}\\bin\\Release64\\Resources\\{file.Value}",
                            Flags);
                    }

                    if (Flags.HasFlag(BuildFlags.BuildArm64bit))
                    {
                        Win32.CopyIfNewer(
                            $"{BuildWorkingFolder}\\SystemInformer\\resources\\{file.Key}",
                            $"{BuildWorkingFolder}\\bin\\ReleaseARM64\\Resources\\{file.Value}",
                            Flags);
                    }
                }
            }

            return true;
        }

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
            string[] Build_DebugCore_Files =
            [
                "dbgcore.dll",
                "dbghelp.dll",
                "symsrv.dll"
            ];

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
                    if (Flags.HasFlag(BuildFlags.Build32bit))
                        Win32.CopyIfNewer($"{windowsSdkPath}\\x86\\{file}", $"bin\\Debug32\\{file}", Flags);
                    if (Flags.HasFlag(BuildFlags.Build64bit))
                        Win32.CopyIfNewer($"{windowsSdkPath}\\x64\\{file}", $"bin\\Debug64\\{file}", Flags);
                    if (Flags.HasFlag(BuildFlags.BuildArm64bit))
                        Win32.CopyIfNewer($"{windowsSdkPath}\\arm64\\{file}", $"bin\\DebugARM64\\{file}", Flags);
                }

                if (Flags.HasFlag(BuildFlags.BuildRelease))
                {
                    if (Flags.HasFlag(BuildFlags.Build32bit))
                        Win32.CopyIfNewer($"{windowsSdkPath}\\x86\\{file}", $"bin\\Release32\\{file}", Flags);
                    if (Flags.HasFlag(BuildFlags.Build64bit))
                        Win32.CopyIfNewer($"{windowsSdkPath}\\x64\\{file}", $"bin\\Release64\\{file}", Flags);
                    if (Flags.HasFlag(BuildFlags.BuildArm64bit))
                        Win32.CopyIfNewer($"{windowsSdkPath}\\arm64\\{file}", $"bin\\ReleaseARM64\\{file}", Flags);
                }
            }

            return true;
        }

        public static bool BuildValidateExportDefinitions(BuildFlags Flags)
        {
            Program.PrintColorMessage(BuildTimeSpan(), ConsoleColor.DarkGray, false);
            Program.PrintColorMessage("Validating exports...", ConsoleColor.Cyan);

            if (Flags.HasFlag(BuildFlags.BuildDebug))
            {
                if (Flags.HasFlag(BuildFlags.Build64bit))
                {
                    if (!Utils.ValidateImageExports($"bin\\Debug32\\SystemInformer.exe"))
                        return false;
                }

                if (Flags.HasFlag(BuildFlags.BuildArm64bit))
                {
                    if (!Utils.ValidateImageExports($"bin\\Debug32\\SystemInformer.exe"))
                        return false;
                }
            }

            if (Flags.HasFlag(BuildFlags.BuildRelease))
            {
                if (Flags.HasFlag(BuildFlags.Build64bit))
                {
                    if (!Utils.ValidateImageExports($"bin\\Release32\\SystemInformer.exe"))
                        return false;
                }

                if (Flags.HasFlag(BuildFlags.BuildArm64bit))
                {
                    if (!Utils.ValidateImageExports($"bin\\Release32\\SystemInformer.exe"))
                        return false;
                }
            }

            return true;
        }

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

        public static bool CopyKernelDriver(BuildFlags Flags)
        {
            string[] Build_Driver_Files =
            [
                "SystemInformer.sys",
                "ksi.dll"
            ];

            try
            {
                foreach (string file in Build_Driver_Files)
                {
                    if (Flags.HasFlag(BuildFlags.BuildDebug))
                    {
                        if (Flags.HasFlag(BuildFlags.Build64bit))
                            Win32.CopyVersionIfNewer($"KSystemInformer\\bin-signed\\amd64\\{file}", $"bin\\Debug64\\{file}", Flags);
                        if (Flags.HasFlag(BuildFlags.BuildArm64bit))
                            Win32.CopyVersionIfNewer($"KSystemInformer\\bin-signed\\arm64\\{file}", $"bin\\DebugARM64\\{file}", Flags);
                    }

                    if (Flags.HasFlag(BuildFlags.BuildRelease))
                    {
                        if (Flags.HasFlag(BuildFlags.Build64bit))
                            Win32.CopyVersionIfNewer($"KSystemInformer\\bin-signed\\amd64\\{file}", $"bin\\Release64\\{file}", Flags);
                        if (Flags.HasFlag(BuildFlags.BuildArm64bit))
                            Win32.CopyVersionIfNewer($"KSystemInformer\\bin-signed\\arm64\\{file}", $"bin\\ReleaseARM64\\{file}", Flags);
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

        public static bool BuildSdk(BuildFlags Flags)
        {
            string[] Build_Sdk_Files =
            [
                "SystemInformer.lib",
                "SystemInformer.pdb"
            ];

            foreach (string folder in BuildConfig.Build_Sdk_Directories)
            {
                Win32.CreateDirectory(folder);
            }

            // Copy headers
            foreach (string file in BuildConfig.Build_Phnt_Headers)
                Win32.CopyIfNewer($"phnt\\include\\{file}", $"sdk\\include\\{file}", Flags, true);
            foreach (string file in BuildConfig.Build_Phlib_Headers)
                Win32.CopyIfNewer($"phlib\\include\\{file}", $"sdk\\include\\{file}", Flags, true);
            foreach (string file in BuildConfig.Build_Kphlib_Headers)
                Win32.CopyIfNewer($"kphlib\\include\\{file}", $"sdk\\include\\{file}", Flags, true);

            // Copy readme
            Win32.CopyIfNewer("SystemInformer\\sdk\\readme.txt", "sdk\\readme.txt", Flags, true);

            // Copy files
            foreach (string file in Build_Sdk_Files)
            {
                if (Flags.HasFlag(BuildFlags.BuildDebug))
                {
                    if (Flags.HasFlag(BuildFlags.Build32bit))
                        Win32.CopyIfNewer($"bin\\Debug32\\{file}", $"sdk\\lib\\i386\\{file}", Flags, true);
                    if (Flags.HasFlag(BuildFlags.Build64bit))
                        Win32.CopyIfNewer($"bin\\Debug64\\{file}", $"sdk\\lib\\amd64\\{file}", Flags, true);
                    if (Flags.HasFlag(BuildFlags.BuildArm64bit))
                        Win32.CopyIfNewer($"bin\\DebugARM64\\{file}", $"sdk\\lib\\arm64\\{file}", Flags, true);
                }

                if (Flags.HasFlag(BuildFlags.BuildRelease))
                {
                    if (Flags.HasFlag(BuildFlags.Build32bit))
                        Win32.CopyIfNewer($"bin\\Release32\\{file}", $"sdk\\lib\\i386\\{file}", Flags, true);
                    if (Flags.HasFlag(BuildFlags.Build64bit))
                        Win32.CopyIfNewer($"bin\\Release64\\{file}", $"sdk\\lib\\amd64\\{file}", Flags, true);
                    if (Flags.HasFlag(BuildFlags.BuildArm64bit))
                        Win32.CopyIfNewer($"bin\\ReleaseARM64\\{file}", $"sdk\\lib\\arm64\\{file}", Flags, true);
                }
            }

            // Build the SDK
            HeaderGen.Execute();

            // Copy the SDK headers
            Win32.CopyIfNewer("SystemInformer\\include\\phappres.h", "sdk\\include\\phappres.h", Flags, true);
            Win32.CopyIfNewer("SystemInformer\\sdk\\phapppub.h", "sdk\\include\\phapppub.h", Flags, true);
            Win32.CopyIfNewer("SystemInformer\\sdk\\phdk.h", "sdk\\include\\phdk.h", Flags, true);

            //
            // Copy the resource header and prefix types with PHAPP for the SDK
            //

            Win32.GetFileBasicInfo("SystemInformer\\resource.h", out var sourceCreationTime, out var sourceWriteTime, out _);
            Win32.GetFileBasicInfo("sdk\\include\\phappresource.h", out var targetCreationTime, out var targetWriteTime, out var targetAttributes);

            if (sourceCreationTime != targetCreationTime || sourceWriteTime != targetWriteTime)
            {
                string resourceContent = Utils.ReadAllText("SystemInformer\\resource.h");
                string targetContent = resourceContent.Replace("#define ID", "#define PHAPP_ID", StringComparison.OrdinalIgnoreCase);

                if (!resourceContent.Equals(targetContent, StringComparison.OrdinalIgnoreCase))
                {
                    if ((targetAttributes & FileAttributes.ReadOnly) != 0)
                        File.SetAttributes("sdk\\include\\phappresource.h", FileAttributes.Normal);
                    Utils.WriteAllText("sdk\\include\\phappresource.h", targetContent);
                }

                Win32.SetFileBasicInfo("sdk\\include\\phappresource.h", sourceCreationTime, sourceWriteTime, true);           
                //Win32.CopyIfNewer("SystemInformer\\resource.h", "sdk\\include\\phappresource.h", Flags);
            }

            return true;
        }

        public static bool BuildSetupExe(string Channel, BuildFlags Flags)
        {
            Program.PrintColorMessage(BuildTimeSpan(), ConsoleColor.DarkGray, false);
            Program.PrintColorMessage($"Building build-{Channel}-setup.exe... ", ConsoleColor.Cyan, false);

            if (!BuildSolution("tools\\CustomSetupTool\\CustomSetupTool.sln", BuildFlags.Build32bit | BuildFlags.BuildApi, Channel))
                return false;

            try
            {
                string out_file = Path.Join([Build.BuildWorkingFolder, "\\tools\\CustomSetupTool\\bin\\Release32\\CustomSetupTool.exe"]);
                string exe_file = Path.Join([Build.BuildOutputFolder, $"\\systeminformer-build-{Channel}-setup.exe"]);

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

        public static bool BuildSdkZip(BuildFlags Flags)
        {
            Program.PrintColorMessage(BuildTimeSpan(), ConsoleColor.DarkGray, false);
            Program.PrintColorMessage("Building build-sdk.zip... ", ConsoleColor.Cyan, false);

            try
            {
                string zip_file = Path.Join([Build.BuildOutputFolder, "\\systeminformer-build-sdk.zip"]);

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

        public static bool BuildBinZip(BuildFlags Flags)
        {
            var Build_Zip_Files = new Dictionary<string, string>(4, StringComparer.OrdinalIgnoreCase)
            {
                ["bin\\Release32"] = "systeminformer-build-win32-bin.zip",
                ["bin\\Release64"] = "systeminformer-build-win64-bin.zip",
                ["bin\\ReleaseARM64"] = "systeminformer-build-arm64-bin.zip",
                ["bin"] = "systeminformer-build-bin.zip",
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

        public static bool BuildPdbZip(bool MsixPackageBuild, BuildFlags Flags)
        {
            Program.PrintColorMessage(BuildTimeSpan(), ConsoleColor.DarkGray, false);
            Program.PrintColorMessage(MsixPackageBuild ? "Building setup-package-pdb..." : "Building build-pdb.zip...", ConsoleColor.Cyan, false);

            try
            {
                Utils.CreateOutputDirectory();

                if (MsixPackageBuild)
                {
                    string zip_file = Path.Join([Build.BuildOutputFolder, "\\systeminformer-setup-package-pdb.zip"]);

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
                    string zip_file = Path.Join([Build.BuildOutputFolder, "\\systeminformer-build-pdb.zip"]);

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
        
        public static bool BuildSymStoreZip(BuildFlags Flags)
        {
            Program.PrintColorMessage(BuildTimeSpan(), ConsoleColor.DarkGray, false);
            Program.PrintColorMessage("Building symsrv-package...", ConsoleColor.Cyan, false);

            try
            {
                string zip_file = Path.Join([Build.BuildOutputFolder, "\\systeminformer-symbols-package.zip"]);

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

        private static string MsbuildCommandString(string Solution, string Platform, BuildFlags Flags, string Channel = null)
        {
            List<string> compilerOptionsList = new List<string>();
            StringBuilder commandLine = new StringBuilder(0x100);
            StringBuilder compilerOptions = new StringBuilder(0x100);
            StringBuilder linkerOptions = new StringBuilder(0x100);

            if (Flags.HasFlag(BuildFlags.BuildApi))
                compilerOptionsList.Add("PH_BUILD_API");
            if (Flags.HasFlag(BuildFlags.BuildMsix))
                compilerOptionsList.Add("PH_BUILD_MSIX");
            if (!string.IsNullOrWhiteSpace(Channel))
                compilerOptionsList.Add($"PH_RELEASE_CHANNEL_ID=\"{BuildConfig.Build_Channels[Channel]}\"");
            if (!string.IsNullOrWhiteSpace(Build.BuildCommitHash))
                compilerOptionsList.Add($"PHAPP_VERSION_COMMITHASH=\"{Build.BuildHash()}\"");
            if (!string.IsNullOrWhiteSpace(Build.BuildVersionMajor))
                compilerOptionsList.Add($"PHAPP_VERSION_MAJOR=\"{Build.BuildVersionMajor}\"");
            if (!string.IsNullOrWhiteSpace(Build.BuildVersionMinor))
                compilerOptionsList.Add($"PHAPP_VERSION_MINOR=\"{Build.BuildVersionMinor}\"");
            if (!string.IsNullOrWhiteSpace(Build.BuildVersionBuild))
                compilerOptionsList.Add($"PHAPP_VERSION_BUILD=\"{Build.BuildVersionBuild}\"");
            if (!string.IsNullOrWhiteSpace(Build.BuildVersionRevision))
                compilerOptionsList.Add($"PHAPP_VERSION_REVISION=\"{Build.BuildVersionRevision}\"");
            if (!string.IsNullOrWhiteSpace(Build.BuildSourceLink))
                linkerOptions.Append($"/SOURCELINK:\"{Build.BuildSourceLink}\" ");

            compilerOptions.AppendJoin(";", compilerOptionsList);

            commandLine.Append($"/m /nologo /nodereuse:false /verbosity:{(Build.BuildToolsDebug ? "diagnostic" : "minimal")} ");
            commandLine.Append($"/p:Platform={Platform} /p:Configuration={(Flags.HasFlag(BuildFlags.BuildDebug) ? "Debug" : "Release")} ");

            if (compilerOptions.Length > 0)
                commandLine.Append($"/p:ExternalCompilerOptions=\"{compilerOptions.ToString()}\" ");
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

        public static bool BuildSolution(string Solution, BuildFlags Flags, string Channel = null)
        {
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

#nullable enable
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

            var binzipdownloadlink = githubMirrorUpload.GetFileUrl("systeminformer-build-bin.zip"); // $"systeminformer-{Build.BuildLongVersion}-bin.zip"
            var relzipdownloadlink = githubMirrorUpload.GetFileUrl("systeminformer-build-release-setup.exe");
            var canzipdownloadlink = githubMirrorUpload.GetFileUrl("systeminformer-build-canary-setup.exe");

            if (string.IsNullOrWhiteSpace(binzipdownloadlink) || string.IsNullOrWhiteSpace(relzipdownloadlink) || string.IsNullOrWhiteSpace(canzipdownloadlink))
            {
                Program.PrintColorMessage("[ERROR] GetDeployInfo failed.", ConsoleColor.Red);
                return false;
            }

            // Update the build information.

            if (!BuildUploadServerConfig(portable_zip, release_exe, canary_exe))
            {
                Program.PrintColorMessage("[ERROR] BuildUploadServerConfig failed.", ConsoleColor.Red);
                return false;
            }

            return true;
        }

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

                    var name = sourceName.Replace(
                        "-build-",
                        $"-{Build.BuildLongVersion}-",
                        StringComparison.OrdinalIgnoreCase
                        );

                    var result = BuildGithub.UploadAsset(
                        newGithubRelease.UploadUrl,
                        deployfile.FileName,
                        sourceName,
                        name
                        );

                    if (result == null)
                    {
                        Program.PrintColorMessage("[Github.UploadAssets]", ConsoleColor.Red);
                        return null;
                    }

                    //mirror.Files.Add(new GithubReleaseAsset(file.FileName, result.Download_url));

                    deployfile.UpdateAssetsResponse(result);
                }

                // Update the release and make it public.

                var update = BuildGithub.UpdateRelease(newGithubRelease.ReleaseId, false);

                if (update == null)
                {
                    Program.PrintColorMessage("[Github.UpdateRelease]", ConsoleColor.Red);
                    return null;
                }

                var mirror = new GithubRelease(update.ReleaseId);

                // Grab the download urls.

                foreach (var file in update.Assets)
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

        private static bool BuildUploadServerConfig(
            DeployFile BinFile,
            DeployFile ReleaseFile,
            DeployFile CanaryFile
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
                string.IsNullOrWhiteSpace(buildPostKey))
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

        private static void BuildMsixPackageManifest(BuildFlags Flags)
        {
            // Create the package manifest.

            if (Flags.HasFlag(BuildFlags.Build32bit))
            {
                string msixManifestString = Utils.ReadAllText("tools\\msix\\PackageTemplate.msix.xml");

                msixManifestString = msixManifestString.Replace("SI_MSIX_ARCH", "x86");
                msixManifestString = msixManifestString.Replace("SI_MSIX_VERSION", Build.BuildLongVersion);
                msixManifestString = msixManifestString.Replace("SI_MSIX_PUBLISHER", "CN=Winsider Seminars &amp; Solutions Inc., O=Winsider Seminars &amp; Solutions Inc., L=Montréal, S=Quebec, C=CA");

                Utils.WriteAllText("tools\\msix\\MsixManifest32.xml", msixManifestString);
            }

            if (Flags.HasFlag(BuildFlags.Build64bit))
            {
                string msixManifestString = Utils.ReadAllText("tools\\msix\\PackageTemplate.msix.xml");

                msixManifestString = msixManifestString.Replace("SI_MSIX_ARCH", "x64");
                msixManifestString = msixManifestString.Replace("SI_MSIX_VERSION", Build.BuildLongVersion);
                msixManifestString = msixManifestString.Replace("SI_MSIX_PUBLISHER", "CN=Winsider Seminars &amp; Solutions Inc., O=Winsider Seminars &amp; Solutions Inc., L=Montréal, S=Quebec, C=CA");

                Utils.WriteAllText("tools\\msix\\MsixManifest64.xml", msixManifestString);
            }
        }

        private static void BuildMsixPackageMapping(BuildFlags Flags)
        {
            // Create the package mapping file.

            if (Flags.HasFlag(BuildFlags.Build32bit) && Directory.Exists("bin\\Release32"))
            {
                StringBuilder packageMap32 = new StringBuilder(0x100);
                packageMap32.AppendLine("[Files]");
                packageMap32.AppendLine("\"tools\\msix\\MsixManifest32.xml\" \"AppxManifest.xml\"");
                packageMap32.AppendLine("\"tools\\msix\\Square44x44Logo.png\" \"Assets\\Square44x44Logo.png\"");
                packageMap32.AppendLine("\"tools\\msix\\Square50x50Logo.png\" \"Assets\\Square50x50Logo.png\"");
                packageMap32.AppendLine("\"tools\\msix\\Square150x150Logo.png\" \"Assets\\Square150x150Logo.png\"");

                foreach (string filePath in Directory.EnumerateFiles("bin\\Release32", "*", SearchOption.AllDirectories))
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

                    packageMap32.AppendLine($"\"{filePath}\" \"{filePath.AsSpan("bin\\Release32\\".Length)}\"");
                }

                Utils.WriteAllText("tools\\msix\\MsixPackage32.map", packageMap32.ToString());
            }

            // Create the package mapping file.

            if (Flags.HasFlag(BuildFlags.Build64bit) && Directory.Exists("bin\\Release64"))
            {
                StringBuilder packageMap64 = new StringBuilder(0x100);
                packageMap64.AppendLine("[Files]");
                packageMap64.AppendLine("\"tools\\msix\\MsixManifest64.xml\" \"AppxManifest.xml\"");
                packageMap64.AppendLine("\"tools\\msix\\Square44x44Logo.png\" \"Assets\\Square44x44Logo.png\"");
                packageMap64.AppendLine("\"tools\\msix\\Square50x50Logo.png\" \"Assets\\Square50x50Logo.png\"");
                packageMap64.AppendLine("\"tools\\msix\\Square150x150Logo.png\" \"Assets\\Square150x150Logo.png\"");

                foreach (string filePath in Directory.EnumerateFiles("bin\\Release64", "*", SearchOption.AllDirectories))
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

                    packageMap64.AppendLine($"\"{filePath}\" \"{filePath.AsSpan("bin\\Release64\\".Length)}\"");
                }

                Utils.WriteAllText("tools\\msix\\MsixPackage64.map", packageMap64.ToString());
            }
        }

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
