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
    public static unsafe class Build
    {
        private static DateTime TimeStart;
        public static bool BuildCanary = false;
        public static bool BuildToolsDebug = false;
        public static bool HaveArm64BuildTools = true;
        public static string BuildOutputFolder = string.Empty;
        public static string BuildWorkingFolder = string.Empty;
        public static string BuildCommitBranch = string.Empty;
        public static string BuildCommitHash = string.Empty;
        public static string BuildShortVersion = string.Empty;
        public static string BuildLongVersion = string.Empty;
        public static string BuildSourceLink = string.Empty;
        public const string BuildVersionMajor = "3";
        public const string BuildVersionMinor = "2";

        public static bool InitializeBuildEnvironment()
        {
            Win32.SetErrorMode();
            Win32.SetBasePriority();

            Console.InputEncoding = Encoding.UTF8;
            Console.OutputEncoding = Encoding.UTF8;

            if (!Utils.SetCurrentDirectoryParent("SystemInformer.sln"))
            {
                Program.PrintColorMessage("Unable to find project solution.", ConsoleColor.Red);
                return false;
            }

            Build.TimeStart = DateTime.UtcNow;
            Build.BuildWorkingFolder = Environment.CurrentDirectory;
            Build.BuildOutputFolder = Utils.GetOutputDirectoryPath("\\build\\output");

            if (Win32.GetEnvironmentVariableSpan("SYSTEM_BUILD", out ReadOnlySpan<char> build_definition))
            {
                if (MemoryExtensions.Equals(build_definition, "canary", StringComparison.OrdinalIgnoreCase))
                {
                    Build.BuildCanary = true;
                    Program.PrintColorMessage("[CANARY BUILD]", ConsoleColor.Cyan);
                }
            }

            if (Win32.GetEnvironmentVariableSpan("SYSTEM_DEBUG", out ReadOnlySpan<char> build_debug))
            {
                if (MemoryExtensions.Equals(build_debug, "true", StringComparison.OrdinalIgnoreCase))
                {
                    Build.BuildToolsDebug = true;
                    Program.PrintColorMessage("[DEBUG BUILD]", ConsoleColor.Cyan);
                }
            }

            if (Win32.GetEnvironmentVariable("BUILD_SOURCEVERSION", out string buildsource))
                Build.BuildCommitHash = buildsource;
            if (Win32.GetEnvironmentVariable("BUILD_SOURCEBRANCHNAME", out string buildbranch))
                Build.BuildCommitBranch = buildbranch;

            //{
            //    VisualStudioInstance instance = Utils.GetVisualStudioInstance();
            //
            //    if (instance == null)
            //    {
            //        Program.PrintColorMessage("Unable to find Visual Studio.", ConsoleColor.Red);
            //        return false;
            //    }
            //
            //    if (!instance.HasRequiredDependency)
            //    {
            //        Program.PrintColorMessage("Visual Studio does not have required the packages: ", ConsoleColor.Red);
            //        Program.PrintColorMessage(instance.MissingDependencyList, ConsoleColor.Cyan);
            //        return false;
            //    }
            //}

            return true;
        }

        public static void SetupBuildEnvironment(bool ShowBuildInfo)
        {
            if (string.IsNullOrWhiteSpace(Build.BuildCommitBranch) && !string.IsNullOrWhiteSpace(Utils.GetGitFilePath()))
            {
                Build.BuildCommitBranch = Utils.ExecuteGitCommand(Build.BuildWorkingFolder, "branch --show-current");
            }

            if (string.IsNullOrWhiteSpace(Build.BuildCommitHash) && !string.IsNullOrWhiteSpace(Utils.GetGitFilePath()))
            {
                Build.BuildCommitHash = Utils.ExecuteGitCommand(Build.BuildWorkingFolder, "rev-parse HEAD");
            }

            if (string.IsNullOrEmpty(Build.BuildCommitHash))
            {
                Build.BuildShortVersion = "1.0.0";
                Build.BuildLongVersion = "1.0.0.0";
            }
            else
            {
                Build.BuildShortVersion = $"{Build.BuildVersionMajor}.{Build.BuildVersionMinor}.{Build.BuildVersionBuild}";
                Build.BuildLongVersion = $"{Build.BuildVersionMajor}.{Build.BuildVersionMinor}.{Build.BuildVersionBuild}.{Build.BuildVersionRevision}";
            }

            if (ShowBuildInfo)
            {
                Program.PrintColorMessage("Windows: ", ConsoleColor.DarkGray, false);
                Program.PrintColorMessage(Win32.GetKernelVersion(), ConsoleColor.Green);

                var instance = VisualStudio.GetVisualStudioInstance();
                if (instance != null)
                {
                    Program.PrintColorMessage("WindowsSDK: ", ConsoleColor.DarkGray, false);
                    //Program.PrintColorMessage(Utils.GetWindowsSdkVersion(), ConsoleColor.Green);
                    Program.PrintColorMessage($"{Utils.GetWindowsSdkVersion()} ({instance.GetWindowsSdkFullVersion()})", ConsoleColor.Green, true);
                    Program.PrintColorMessage("VisualStudio: ", ConsoleColor.DarkGray, false);
                    Program.PrintColorMessage(instance.Name, ConsoleColor.Green);
                    //Program.PrintColorMessage(Utils.GetVisualStudioVersion(), ConsoleColor.Green, true);
                    Build.HaveArm64BuildTools = instance.HasARM64BuildToolsComponents;
                }

                Program.PrintColorMessage("SystemInformer: ", ConsoleColor.DarkGray, false);
                Program.PrintColorMessage(Build.BuildLongVersion, ConsoleColor.Green, false);

                if (!string.IsNullOrWhiteSpace(Build.BuildCommitHash))
                {
                    Program.PrintColorMessage(" (", ConsoleColor.DarkGray, false);
                    Program.PrintColorMessage(Build.BuildCommitHash.Substring(0, 8), ConsoleColor.DarkCyan, false);
                    Program.PrintColorMessage(")", ConsoleColor.DarkGray, false);
                }

                if (!string.IsNullOrWhiteSpace(Build.BuildCommitBranch))
                {
                    Program.PrintColorMessage(" [", ConsoleColor.DarkGray, false);
                    Program.PrintColorMessage(Build.BuildCommitBranch, ConsoleColor.DarkBlue, false);
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
            get { return $"{TimeStart.Year % 100}{TimeStart.DayOfYear:D3}"; }
        }

        public static string BuildVersionRevision
        {
            get { return $"{TimeStart.Hour:D2}{TimeStart.Minute:D2}"; }
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
            TimeSpan buildTime = (DateTime.UtcNow - Build.TimeStart);

            Program.PrintColorMessage($"{Environment.NewLine}Build Time: ", ConsoleColor.DarkGray, false);
            Program.PrintColorMessage(buildTime.Minutes.ToString(), ConsoleColor.Green, false);
            Program.PrintColorMessage(" minute(s), ", ConsoleColor.DarkGray, false);
            Program.PrintColorMessage(buildTime.Seconds.ToString(), ConsoleColor.Green, false);
            Program.PrintColorMessage($" second(s) {Environment.NewLine + Environment.NewLine}", ConsoleColor.DarkGray, false);
        }

        public static bool CopyTextFiles(bool Update)
        {
            if (Update)
            {
                Win32.CopyIfNewer("README.txt", "bin\\README.txt");
                //Win32.CopyIfNewer("CHANGELOG.txt", "bin\\CHANGELOG.txt"); // TODO: Git log
                Win32.CopyIfNewer("COPYRIGHT.txt", "bin\\COPYRIGHT.txt");
                Win32.CopyIfNewer("LICENSE.txt", "bin\\LICENSE.txt");
            }
            else
            {
                Win32.DeleteFile("bin\\README.txt");
                //Win32.DeleteFile("bin\\CHANGELOG.txt");
                Win32.DeleteFile("bin\\COPYRIGHT.txt");
                Win32.DeleteFile("bin\\LICENSE.txt");
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
                { "SystemInformer.png", "icon.png" },
                { "CapsList.txt", "CapsList.txt" },
                { "EtwGuids.txt", "EtwGuids.txt" },
                { "PoolTag.txt", "PoolTag.txt" },
            };

            foreach (var file in Build_Resource_Files)
            {
                if (Flags.HasFlag(BuildFlags.BuildDebug))
                {
                    if (Flags.HasFlag(BuildFlags.Build32bit))
                        Win32.CopyIfNewer($"SystemInformer\\resources\\{file.Key}", $"bin\\Debug32\\Resources\\{file.Value}", Flags);
                    if (Flags.HasFlag(BuildFlags.Build64bit))
                        Win32.CopyIfNewer($"SystemInformer\\resources\\{file.Key}", $"bin\\Debug64\\Resources\\{file.Value}", Flags);
                    if (Flags.HasFlag(BuildFlags.BuildArm64bit))
                        Win32.CopyIfNewer($"SystemInformer\\resources\\{file.Key}", $"bin\\DebugARM64\\Resources\\{file.Value}", Flags);
                }

                if (Flags.HasFlag(BuildFlags.BuildRelease))
                {
                    if (Flags.HasFlag(BuildFlags.Build32bit))
                        Win32.CopyIfNewer($"SystemInformer\\resources\\{file.Key}", $"bin\\Release32\\Resources\\{file.Value}", Flags);
                    if (Flags.HasFlag(BuildFlags.Build64bit))
                        Win32.CopyIfNewer($"SystemInformer\\resources\\{file.Key}", $"bin\\Release64\\Resources\\{file.Value}", Flags);
                    if (Flags.HasFlag(BuildFlags.BuildArm64bit))
                        Win32.CopyIfNewer($"SystemInformer\\resources\\{file.Key}", $"bin\\ReleaseARM64\\Resources\\{file.Value}", Flags);
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
            var Build_DebugCore_Files = new string[]
            {
                "dbgcore.dll",
                "dbghelp.dll",
                "symsrv.dll"
            };

            string windowsSdkPath = Path.GetFullPath(Path.Join([Utils.GetWindowsSdkPath(), "..\\..\\Debuggers\\"]));

            if (string.IsNullOrWhiteSpace(windowsSdkPath))
                return false;

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
            {
                "SystemInformer.sys",
                "ksi.dll",
            };

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
                if (!Verify.CreateSigFile("kph", file, BuildCanary))
                    return false;
            }

            return true;
        }

        public static bool BuildSdk(BuildFlags Flags)
        {
            foreach (string folder in BuildConfig.Build_Sdk_Directories)
            {
                Win32.CreateDirectory(folder);
            }

            // Copy the plugin SDK headers
            foreach (string file in BuildConfig.Build_Phnt_Headers)
                Win32.CopyIfNewer($"phnt\\include\\{file}", $"sdk\\include\\{file}", Flags);
            foreach (string file in BuildConfig.Build_Phlib_Headers)
                Win32.CopyIfNewer($"phlib\\include\\{file}", $"sdk\\include\\{file}", Flags);
            foreach (string file in BuildConfig.Build_Kphlib_Headers)
                Win32.CopyIfNewer($"kphlib\\include\\{file}", $"sdk\\include\\{file}", Flags);

            // Copy readme
            Win32.CopyIfNewer("SystemInformer\\sdk\\readme.txt", "sdk\\readme.txt", Flags);
            // Copy symbols
            //Win32.CopyIfNewer("bin\\Release32\\SystemInformer.pdb", "sdk\\dbg\\i386\\SystemInformer.pdb", Flags);
            //Win32.CopyIfNewer("bin\\Release64\\SystemInformer.pdb", "sdk\\dbg\\amd64\\SystemInformer.pdb", Flags);
            //Win32.CopyIfNewer("bin\\ReleaseARM64\\SystemInformer.pdb", "sdk\\dbg\\arm64\\SystemInformer.pdb", Flags);
            //Win32.CopyIfNewer("KSystemInformer\\bin\\i386\\systeminformer.pdb", "sdk\\dbg\\i386\\ksysteminformer.pdb", Flags);
            //Win32.CopyIfNewer("KSystemInformer\\bin\\amd64\\systeminformer.pdb", "sdk\\dbg\\amd64\\ksysteminformer.pdb", Flags);
            //Win32.CopyIfNewer("KSystemInformer\\bin\\arm64\\systeminformer.pdb", "sdk\\dbg\\arm64\\ksysteminformer.pdb", Flags);
            //Win32.CopyIfNewer("KSystemInformer\\bin\\i386\\ksi.pdb", "sdk\\dbg\\i386\\ksi.pdb", Flags);
            //Win32.CopyIfNewer("KSystemInformer\\bin\\amd64\\ksi.pdb", "sdk\\dbg\\amd64\\ksi.pdb", Flags);
            //Win32.CopyIfNewer("KSystemInformer\\bin\\arm64\\ksi.pdb", "sdk\\dbg\\arm64\\ksi.pdb", Flags);

            // Copy libs
            if (Flags.HasFlag(BuildFlags.BuildDebug))
            {
                if (Flags.HasFlag(BuildFlags.Build32bit))
                    Win32.CopyIfNewer("bin\\Debug32\\SystemInformer.lib", "sdk\\lib\\i386\\SystemInformer.lib", Flags);
                if (Flags.HasFlag(BuildFlags.Build64bit))
                    Win32.CopyIfNewer("bin\\Debug64\\SystemInformer.lib", "sdk\\lib\\amd64\\SystemInformer.lib", Flags);
                if (Flags.HasFlag(BuildFlags.BuildArm64bit))
                    Win32.CopyIfNewer("bin\\DebugARM64\\SystemInformer.lib", "sdk\\lib\\arm64\\SystemInformer.lib", Flags);
            }

            if (Flags.HasFlag(BuildFlags.BuildRelease))
            {
                if (Flags.HasFlag(BuildFlags.Build32bit))
                    Win32.CopyIfNewer("bin\\Release32\\SystemInformer.lib", "sdk\\lib\\i386\\SystemInformer.lib", Flags);
                if (Flags.HasFlag(BuildFlags.Build64bit))
                    Win32.CopyIfNewer("bin\\Release64\\SystemInformer.lib", "sdk\\lib\\amd64\\SystemInformer.lib", Flags);
                if (Flags.HasFlag(BuildFlags.BuildArm64bit))
                    Win32.CopyIfNewer("bin\\ReleaseARM64\\SystemInformer.lib", "sdk\\lib\\arm64\\SystemInformer.lib", Flags);
            }

            // Build the SDK
            HeaderGen.Execute();

            // Copy the SDK headers
            Win32.CopyIfNewer("SystemInformer\\include\\phappres.h", "sdk\\include\\phappres.h", Flags);
            Win32.CopyIfNewer("SystemInformer\\sdk\\phapppub.h", "sdk\\include\\phapppub.h", Flags);
            Win32.CopyIfNewer("SystemInformer\\sdk\\phdk.h", "sdk\\include\\phdk.h", Flags);
            //Win32.CopyIfNewer("SystemInformer\\resource.h", "sdk\\include\\phappresource.h", Flags);

            // Copy the resource header and prefix types with PHAPP
            {
                WIN32_FILE_ATTRIBUTE_DATA sourceFile;
                WIN32_FILE_ATTRIBUTE_DATA destinationFile;

                PInvoke.GetFileAttributesEx(
                    "SystemInformer\\resource.h",
                    GET_FILEEX_INFO_LEVELS.GetFileExInfoStandard,
                    &sourceFile);

                PInvoke.GetFileAttributesEx(
                    "sdk\\include\\phappresource.h",
                    GET_FILEEX_INFO_LEVELS.GetFileExInfoStandard,
                    &destinationFile
                    );

                if (
                    sourceFile.ftCreationTime.FileTimeToDateTime() != destinationFile.ftCreationTime.FileTimeToDateTime() ||
                    sourceFile.ftLastWriteTime.FileTimeToDateTime() != destinationFile.ftLastWriteTime.FileTimeToDateTime()
                    )
                {
                    string resourceContent = Utils.ReadAllText("SystemInformer\\resource.h");

                    if (!string.IsNullOrWhiteSpace(resourceContent))
                    {
                        // Update resource headers with SDK definition
                        string sdkContent = resourceContent.Replace("#define ID", "#define PHAPP_ID", StringComparison.OrdinalIgnoreCase);

                        Utils.WriteAllText(
                            "sdk\\include\\phappresource.h",
                            sdkContent
                            );

                        using (var fs = File.OpenWrite("sdk\\include\\phappresource.h"))
                        {
                            PInvoke.SetFileTime(
                                fs.SafeFileHandle,
                                sourceFile.ftCreationTime,
                                sourceFile.ftLastAccessTime,
                                sourceFile.ftLastWriteTime
                                );
                        }
                    }
                }
            }

            return true;
        }

        public static bool BuildSetupExe(string Channel)
        {
            Program.PrintColorMessage(BuildTimeSpan(), ConsoleColor.DarkGray, false);
            Program.PrintColorMessage($"Building build-{Channel}-setup.exe... ", ConsoleColor.Cyan, false);

            if (!BuildSolution("tools\\CustomSetupTool\\CustomSetupTool.sln", BuildFlags.Build32bit | BuildFlags.BuildApi, Channel))
                return false;

            try
            {
                Utils.CreateOutputDirectory();

                Win32.DeleteFile(
                    $"{BuildOutputFolder}\\systeminformer-build-{Channel}-setup.exe"
                    );

                File.Move(
                    "tools\\CustomSetupTool\\bin\\Release32\\CustomSetupTool.exe",
                    $"{BuildOutputFolder}\\systeminformer-build-{Channel}-setup.exe"
                    );

                Program.PrintColorMessage(Win32.GetFileSize($"{BuildOutputFolder}\\systeminformer-build-{Channel}-setup.exe").ToPrettySize(), ConsoleColor.Green);
            }
            catch (Exception ex)
            {
                Program.PrintColorMessage($"[ERROR] {ex}", ConsoleColor.Red);
                return false;
            }

            return true;
        }

        public static bool BuildSdkZip()
        {
            Program.PrintColorMessage(BuildTimeSpan(), ConsoleColor.DarkGray, false);
            Program.PrintColorMessage("Building build-sdk.zip... ", ConsoleColor.Cyan, false);

            try
            {
                Utils.CreateOutputDirectory();

                Win32.DeleteFile(
                    $"{BuildOutputFolder}\\systeminformer-build-sdk.zip"
                    );

                Zip.CreateCompressedSdkFromFolder(
                    "sdk",
                    $"{BuildOutputFolder}\\systeminformer-build-sdk.zip"
                    );

                Program.PrintColorMessage(Win32.GetFileSize($"{BuildOutputFolder}\\systeminformer-build-sdk.zip").ToPrettySize(), ConsoleColor.Green);
            }
            catch (Exception ex)
            {
                Program.PrintColorMessage($"[ERROR] {ex}", ConsoleColor.Red);
                return false;
            }

            return true;
        }

        public static void CreateSettingsFile(string Channel, string Path)
        {
            StringBuilder stringBuilder = new StringBuilder(0x100);

            stringBuilder.AppendLine("<settings>");

            if (string.Equals(Channel, "release", StringComparison.OrdinalIgnoreCase))
            {
                stringBuilder.AppendLine("<setting name=\"ReleaseChannel\">0</setting>"); // PhReleaseChannel
            }
            else if (string.Equals(Channel, "preview", StringComparison.OrdinalIgnoreCase))
            {
                stringBuilder.AppendLine("<setting name=\"ReleaseChannel\">1</setting>"); // PhPreviewChannel
            }
            else if (string.Equals(Channel, "canary", StringComparison.OrdinalIgnoreCase))
            {
                stringBuilder.AppendLine("<setting name=\"ReleaseChannel\">2</setting>"); // PhCanaryChannel
            }
            else if (string.Equals(Channel, "developer", StringComparison.OrdinalIgnoreCase))
            {
                stringBuilder.AppendLine("<setting name=\"ReleaseChannel\">3</setting>"); // PhDeveloperChannel
            }
            else
            {
                throw new Exception($"Invalid channel: {Channel}");
            }

            stringBuilder.AppendLine("</settings>");

            if (Directory.Exists(Path))
            {
                Win32.CreateEmptyFile($"{Path}\\SystemInformer.exe.settings.xml");
                Utils.WriteAllText($"{Path}\\SystemInformer.exe.settings.xml", stringBuilder.ToString());
            }
        }

        public static bool BuildBinZip(string Channel)
        {
            Program.PrintColorMessage(BuildTimeSpan(), ConsoleColor.DarkGray, false);
            Program.PrintColorMessage($"Building build-{Channel}-bin.zip... ", ConsoleColor.Cyan, false);

            try
            {
                CreateSettingsFile(Channel, "bin\\Release32");
                CreateSettingsFile(Channel, "bin\\Release64");
                CreateSettingsFile(Channel, "bin\\ReleaseARM64");
            }
            catch (Exception ex)
            {
                Program.PrintColorMessage($"[WARN] {ex}", ConsoleColor.Yellow);
                return false;
            }

            //try
            //{
            //    if (Directory.Exists("bin\\Release32"))
            //        Win32.CreateEmptyFile("bin\\Release32\\usernotesdb.xml");
            //    if (Directory.Exists("bin\\Release64"))
            //        Win32.CreateEmptyFile("bin\\Release64\\usernotesdb.xml");
            //    if (Directory.Exists("bin\\ReleaseARM64"))
            //        Win32.CreateEmptyFile("bin\\ReleaseARM64\\usernotesdb.xml");
            //}
            //catch (Exception ex)
            //{
            //    Program.PrintColorMessage($"[WARN] {ex}", ConsoleColor.Yellow);
            //}

            try
            {
                Utils.CreateOutputDirectory();

                Win32.DeleteFile(
                    $"{BuildOutputFolder}\\systeminformer-build-{Channel}-bin.zip"
                    );

                Zip.CreateCompressedFolder(
                    "bin",
                    $"{BuildOutputFolder}\\systeminformer-build-{Channel}-bin.zip"
                    );

                Program.PrintColorMessage(Win32.GetFileSize($"{BuildOutputFolder}\\systeminformer-build-{Channel}-bin.zip").ToPrettySize(), ConsoleColor.Green);
            }
            catch (Exception ex)
            {
                Program.PrintColorMessage($"[ERROR] {ex}", ConsoleColor.Red);
                return false;
            }

            return true;
        }

        public static bool BuildPdbZip(bool MsixPackageBuild)
        {
            Program.PrintColorMessage(BuildTimeSpan(), ConsoleColor.DarkGray, false);
            Program.PrintColorMessage(MsixPackageBuild ? "Building setup-package-pdb..." : "Building build-pdb.zip...", ConsoleColor.Cyan, false);

            try
            {
                Utils.CreateOutputDirectory();

                if (MsixPackageBuild)
                {
                    Win32.DeleteFile(
                        $"{BuildOutputFolder}\\systeminformer-setup-package-pdb.zip"
                        );

                    Zip.CreateCompressedPdbFromFolder(
                        ".\\",
                        $"{BuildOutputFolder}\\systeminformer-setup-package-pdb.zip"
                        );

                    Program.PrintColorMessage(Win32.GetFileSize($"{BuildOutputFolder}\\systeminformer-setup-package-pdb.zip").ToPrettySize(), ConsoleColor.Green);
                }
                else
                {
                    Win32.DeleteFile(
                        $"{BuildOutputFolder}\\systeminformer-build-pdb.zip"
                        );

                    Zip.CreateCompressedPdbFromFolder(
                        ".\\",
                        $"{BuildOutputFolder}\\systeminformer-build-pdb.zip"
                        );

                    Program.PrintColorMessage(Win32.GetFileSize($"{BuildOutputFolder}\\systeminformer-build-pdb.zip").ToPrettySize(), ConsoleColor.Green);
                }
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
            Program.PrintColorMessage(BuildTimeSpan(), ConsoleColor.DarkGray, false);
            Program.PrintColorMessage("Building release checksums...", ConsoleColor.Cyan);

            try
            {
                StringBuilder sb = new StringBuilder();

                foreach (BuildFile name in BuildConfig.Build_Release_Files)
                {
                    if (!name.UploadCanary)
                        continue;

                    string file = Path.Join([BuildOutputFolder, name.FileName]);

                    if (File.Exists(file))
                    {
                        FileInfo info = new FileInfo(file);
                        string hash = Verify.HashFile(file);

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
            StringBuilder commandLine = new StringBuilder(0x100);
            StringBuilder compilerOptions = new StringBuilder(0x100);
            StringBuilder linkerOptions = new StringBuilder(0x100);

            if (Flags.HasFlag(BuildFlags.BuildApi))
                compilerOptions.Append("PH_BUILD_API;");
            if (Flags.HasFlag(BuildFlags.BuildMsix))
                compilerOptions.Append("PH_BUILD_MSIX;");
            if (!string.IsNullOrWhiteSpace(Channel))
                compilerOptions.Append($"PH_RELEASE_CHANNEL_ID=\"{BuildConfig.Build_Channels[Channel]}\";");
            if (!string.IsNullOrWhiteSpace(Build.BuildCommitHash))
                compilerOptions.Append($"PHAPP_VERSION_COMMITHASH=\"{Build.BuildCommitHash.AsSpan(0, 8)}\";");
            if (!string.IsNullOrWhiteSpace(Build.BuildVersionMajor))
                compilerOptions.Append($"PHAPP_VERSION_MAJOR=\"{Build.BuildVersionMajor}\";");
            if (!string.IsNullOrWhiteSpace(Build.BuildVersionMinor))
                compilerOptions.Append($"PHAPP_VERSION_MINOR=\"{Build.BuildVersionMinor}\";");
            if (!string.IsNullOrWhiteSpace(Build.BuildVersionBuild))
                compilerOptions.Append($"PHAPP_VERSION_BUILD=\"{Build.BuildVersionBuild}\";");
            if (!string.IsNullOrWhiteSpace(Build.BuildVersionRevision))
                compilerOptions.Append($"PHAPP_VERSION_REVISION=\"{Build.BuildVersionRevision}\";");
            if (!string.IsNullOrWhiteSpace(Build.BuildSourceLink))
                linkerOptions.Append($"/SOURCELINK:\"{Build.BuildSourceLink}\" ");

            commandLine.Append($"/m /nologo /nodereuse:false /verbosity:{(Build.BuildToolsDebug ? "diagnostic" : "quiet")} ");
            commandLine.Append($"/p:Platform={Platform} /p:Configuration={(Flags.HasFlag(BuildFlags.BuildDebug) ? "Debug" : "Release")} ");
            commandLine.Append($"/p:ExternalCompilerOptions=\"{compilerOptions.ToString()}\" ");
            commandLine.Append($"/p:ExternalLinkerOptions=\"{linkerOptions.ToString()}\" ");
            commandLine.Append($"/bl:build/output/logs/{Utils.GetBuildLogPath(Solution, Platform, Flags)}.binlog ");
            commandLine.Append(Solution);

            return commandLine.ToString();
        }

        private static bool MsbuildCommand(string Solution, string Platform, BuildFlags Flags, string Channel = null)
        {
            string buildCommandLine = MsbuildCommandString(Solution, Platform, Flags, Channel);

            Program.PrintColorMessage(BuildTimeSpan(), ConsoleColor.DarkGray, false, Flags);
            Program.PrintColorMessage($"Building {Path.GetFileNameWithoutExtension(Solution)} (", ConsoleColor.Cyan, false, Flags);
            Program.PrintColorMessage(Platform, ConsoleColor.Green, false, Flags);
            Program.PrintColorMessage(")...", ConsoleColor.Cyan, true, Flags);

            int errorcode = Utils.ExecuteMsbuildCommand(buildCommandLine, Flags, out string errorstring);

            if (errorcode != 0)
            {
                Program.PrintColorMessage($"[ERROR] ({errorcode}) {errorstring}", ConsoleColor.Red, true, Flags | BuildFlags.BuildVerbose);
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

        private struct BuildDeployInfo
        {
            public string BinFilename;
            public string BinHash;
            public string BinSig;
            public ulong BinFileLength;

            public string SetupFilename;
            public string SetupHash;
            public string SetupSig;
            public ulong SetupFileLength;
        }

        private static bool GetBuildDeployInfo(string Channel, out BuildDeployInfo Info)
        {
            Info = new BuildDeployInfo
            {
                BinFilename = $"{BuildOutputFolder}\\systeminformer-build-{Channel}-bin.zip",
                SetupFilename = $"{BuildOutputFolder}\\systeminformer-build-{Channel}-setup.exe"
            };

            if (!Verify.CreateSigString(Channel, Info.BinFilename, out Info.BinSig))
            {
                Program.PrintColorMessage("failed to create bin signature string", ConsoleColor.Red);
                return false;
            }

            if (!Verify.CreateSigString(Channel, Info.SetupFilename, out Info.SetupSig))
            {
                Program.PrintColorMessage("failed to create setup signature string", ConsoleColor.Red);
                return false;
            }

            if (File.Exists(Info.BinFilename))
            {
                Info.BinFileLength = Win32.GetFileSize(Info.BinFilename);
                Info.BinHash = Verify.HashFile(Info.BinFilename);
            }

            if (File.Exists(Info.SetupFilename))
            {
                Info.SetupFileLength = Win32.GetFileSize(Info.SetupFilename);
                Info.SetupHash = Verify.HashFile(Info.SetupFilename);
            }

            if (string.IsNullOrWhiteSpace(Info.BinHash))
            {
                Program.PrintColorMessage("build-bin hash not found.", ConsoleColor.Red);
                return false;
            }

            if (string.IsNullOrWhiteSpace(Info.SetupHash))
            {
                Program.PrintColorMessage("build-setup hash not found.", ConsoleColor.Red);
                return false;
            }

            return true;
        }

        public static bool BuildDeployUpdateConfig()
        {
            if (!Build.BuildCanary)
                return true;
            if (Build.BuildToolsDebug)
                return true;
            if (!Win32.GetEnvironmentVariable("BUILD_BUILDID", out string buildBuildId))
                return false;
            if (!Win32.GetEnvironmentVariable("BUILD_SF_API", out string buildPostSfUrl))
                return false;
            if (!Win32.GetEnvironmentVariable("BUILD_SF_KEY", out string buildPostSfApiKey))
                return false;

            Program.PrintColorMessage($"{Environment.NewLine}Uploading build artifacts... {Build.BuildShortVersion}", ConsoleColor.Cyan);

            if (!GetBuildDeployInfo("release", out BuildDeployInfo release))
                return false;
            //if (!GetBuildDeployInfo("preview", out BuildDeployInfo preview))
            //    return false;
            if (!GetBuildDeployInfo("canary", out BuildDeployInfo canary))
                return false;
            //if (!GetBuildDeployInfo("developer", out BuildDeployInfo developer))
            //    return false;

            GithubRelease githubMirrorUpload = BuildDeployUploadGithubConfig();

            if (githubMirrorUpload == null)
                return false;

            string canaryBinziplink = githubMirrorUpload.GetFileUrl($"systeminformer-{Build.BuildShortVersion}-canary-bin.zip");
            string canarySetupexelink = githubMirrorUpload.GetFileUrl($"systeminformer-{Build.BuildShortVersion}-canary-setup.exe");

            string releaseBinziplink = githubMirrorUpload.GetFileUrl($"systeminformer-{Build.BuildShortVersion}-release-bin.zip");
            string releaseSetupexelink = githubMirrorUpload.GetFileUrl($"systeminformer-{Build.BuildShortVersion}-release-setup.exe");

            if (string.IsNullOrWhiteSpace(canaryBinziplink) ||
                string.IsNullOrWhiteSpace(canarySetupexelink) ||
                string.IsNullOrWhiteSpace(releaseBinziplink) ||
                string.IsNullOrWhiteSpace(releaseSetupexelink))
            {
                Program.PrintColorMessage("build-github downloads not found.", ConsoleColor.Red);
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
                    BinUrl = canaryBinziplink,
                    BinLength = canary.BinFileLength.ToString(),
                    BinHash = canary.BinHash,
                    BinSig = canary.BinSig,
                    SetupUrl = canarySetupexelink,
                    SetupLength = canary.SetupFileLength.ToString(),
                    SetupHash = canary.SetupHash,
                    SetupSig = canary.SetupSig,
                    ReleaseBinUrl = releaseBinziplink,
                    ReleaseBinLength = release.BinFileLength.ToString(),
                    ReleaseBinHash = release.BinHash,
                    ReleaseBinSig = release.BinSig,
                    ReleaseSetupUrl = releaseSetupexelink,
                    ReleaseSetupLength = release.SetupFileLength.ToString(),
                    ReleaseSetupHash = release.SetupHash,
                    ReleaseSetupSig = release.SetupSig,
                };

                byte[] buildPostString = JsonSerializer.SerializeToUtf8Bytes(buildUpdateRequest, BuildUpdateRequestContext.Default.BuildUpdateRequest);

                if (buildPostString.LongLength == 0)
                    return false;

                using (HttpRequestMessage requestMessage = new HttpRequestMessage(HttpMethod.Post, buildPostSfUrl))
                {
                    requestMessage.Headers.Accept.Add(new MediaTypeWithQualityHeaderValue("application/json"));
                    requestMessage.Headers.Add("X-ApiKey", buildPostSfApiKey);
                    requestMessage.VersionPolicy = HttpVersionPolicy.RequestVersionOrHigher;
                    requestMessage.Version = HttpVersion.Version20;

                    requestMessage.Content = new ByteArrayContent(buildPostString);
                    requestMessage.Content.Headers.ContentType = new MediaTypeHeaderValue("application/json");

                    var httpResult = HttpClient.SendMessage(requestMessage);

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

        public static GithubRelease BuildDeployUploadGithubConfig()
        {
            //if (!GithubReleases.DeleteRelease(Build.BuildShortVersion))
            //    return null;

            GithubRelease mirror = null;

            try
            {
                // Create a new github release.

                var response = GithubReleases.CreateRelease(Build.BuildShortVersion);

                if (response == null)
                {
                    Program.PrintColorMessage("[Github.CreateRelease]", ConsoleColor.Red);
                    return null;
                }

                // Upload the files.

                foreach (BuildFile file in BuildConfig.Build_Release_Files)
                {
                    if (!file.UploadCanary)
                        continue;

                    string sourceFile = Path.Join([Build.BuildOutputFolder, file.FileName]);

                    if (File.Exists(sourceFile))
                    {
                        var result = GithubReleases.UploadAssets(Build.BuildShortVersion, sourceFile, response.UploadUrl);

                        if (result == null)
                        {
                            Program.PrintColorMessage("[Github.UploadAssets]", ConsoleColor.Red);
                            return null;
                        }

                        //mirror.Files.Add(new GithubReleaseAsset(file.FileName, result.Download_url));
                    }
                }

                // Update the release and make it public.

                var update = GithubReleases.UpdateRelease(response.ReleaseId);

                if (update == null)
                {
                    Program.PrintColorMessage("[Github.UpdateRelease]", ConsoleColor.Red);
                    return null;
                }

                mirror = new GithubRelease(update.ReleaseId);

                // Grab the download urls.

                foreach (var file in update.Assets)
                {
                    if (!file.Uploaded)
                    {
                        continue;
                    }

                    mirror.Files.Add(new GithubReleaseAsset(file.Name, file.DownloadUrl));
                }
            }
            catch (Exception ex)
            {
                Program.PrintColorMessage($"[Github] {ex}", ConsoleColor.Red);
                return null;
            }

            if (!mirror.Valid)
            {
                Program.PrintColorMessage("[Github-Valid]", ConsoleColor.Red);
                return null;
            }

            return mirror;
        }

        private static void BuildMsixPackageManifest(BuildFlags Flags)
        {
            if (Flags.HasFlag(BuildFlags.Build32bit))
            {
                // Create the package manifest.

                if (File.Exists("tools\\msix\\PackageTemplate.msix.xml"))
                {
                    string msixManifestString = Utils.ReadAllText("tools\\msix\\PackageTemplate.msix.xml");

                    msixManifestString = msixManifestString.Replace("SI_MSIX_ARCH", "x86");
                    msixManifestString = msixManifestString.Replace("SI_MSIX_VERSION", Build.BuildLongVersion);
                    msixManifestString = msixManifestString.Replace("SI_MSIX_PUBLISHER", "CN=Winsider Seminars &amp; Solutions Inc.");

                    Utils.WriteAllText("tools\\msix\\MsixManifest32.xml", msixManifestString);
                }
            }

            if (Flags.HasFlag(BuildFlags.Build64bit))
            {
                // Create the package manifest.

                if (File.Exists("tools\\msix\\PackageTemplate.msix.xml"))
                {
                    string msixManifestString = Utils.ReadAllText("tools\\msix\\PackageTemplate.msix.xml");

                    msixManifestString = msixManifestString.Replace("SI_MSIX_ARCH", "x64");
                    msixManifestString = msixManifestString.Replace("SI_MSIX_VERSION", Build.BuildLongVersion);
                    msixManifestString = msixManifestString.Replace("SI_MSIX_PUBLISHER", "CN=Winsider Seminars &amp; Solutions Inc.");

                    Utils.WriteAllText("tools\\msix\\MsixManifest64.xml", msixManifestString);
                }
            }
        }

        private static void BuildMsixAppInstaller()
        {
            if (File.Exists("tools\\msix\\PackageTemplate.appinstaller"))
            {
                string msixAppInstallerString = Utils.ReadAllText("tools\\msix\\PackageTemplate.appinstaller");

                msixAppInstallerString = msixAppInstallerString.Replace("Version=\"3.0.0.0\"", $"Version=\"{Build.BuildLongVersion}\"");

                Utils.WriteAllText("build\\output\\SystemInformer.appinstaller", msixAppInstallerString);
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

                var filesToAdd = Directory.GetFiles("bin\\Release32", "*", SearchOption.AllDirectories);
                foreach (string filePath in filesToAdd)
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

                var filesToAdd = Directory.GetFiles("bin\\Release64", "*", SearchOption.AllDirectories);
                foreach (string filePath in filesToAdd)
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

        public static void BuildStorePackage(BuildFlags Flags)
        {
            Program.PrintColorMessage("Building package.msixbundle...", ConsoleColor.Cyan);

            BuildMsixPackageManifest(Flags);
            BuildMsixPackageMapping(Flags);

            // Create the MSIX package.

            if (Flags.HasFlag(BuildFlags.Build32bit))
            {
                if (File.Exists("tools\\msix\\MsixPackage32.map"))
                {
                    Win32.DeleteFile($"{BuildOutputFolder}\\systeminformer-build-package-x32.appx");
                    Win32.CreateDirectory(BuildOutputFolder);

                    var result = Utils.ExecuteMsixCommand(
                        $"pack /o /f tools\\msix\\MsixPackage32.map /p {BuildOutputFolder}\\systeminformer-build-package-x32.appx"
                        );

                    Program.PrintColorMessage(result, ConsoleColor.DarkGray);
                }
            }

            if (Flags.HasFlag(BuildFlags.Build64bit))
            {
                if (File.Exists("tools\\msix\\MsixPackage64.map"))
                {
                    Win32.DeleteFile($"{BuildOutputFolder}\\systeminformer-build-package-x64.appx");
                    Win32.CreateDirectory(BuildOutputFolder);

                    var result = Utils.ExecuteMsixCommand(
                        $"pack /o /f tools\\msix\\MsixPackage64.map /p {BuildOutputFolder}\\systeminformer-build-package-x64.msix"
                        );

                    Program.PrintColorMessage(result, ConsoleColor.DarkGray);
                }
            }

            // Create the bundle package.
            if (
                File.Exists($"{BuildOutputFolder}\\systeminformer-build-package-x32.msix") &&
                File.Exists($"{BuildOutputFolder}\\systeminformer-build-package-x64.msix")
                )
            {
                StringBuilder bundleMap = new StringBuilder(0x100);
                bundleMap.AppendLine("[Files]");
                bundleMap.AppendLine($"\"{BuildOutputFolder}\\systeminformer-build-package-x32.msix\" \"systeminformer-build-package-x32.msix\"");
                bundleMap.AppendLine($"\"{BuildOutputFolder}\\systeminformer-build-package-x64.msix\" \"systeminformer-build-package-x64.msix\"");

                Utils.WriteAllText("tools\\msix\\bundle.map", bundleMap.ToString());
            }

            if (File.Exists("tools\\msix\\bundle.map"))
            {
                Win32.DeleteFile($"{BuildOutputFolder}\\systeminformer-build-package.msixbundle");

                var result = Utils.ExecuteMsixCommand(
                    $"bundle /f tools\\msix\\bundle.map /p {BuildOutputFolder}\\systeminformer-build-package.msixbundle"
                    );

                Program.PrintColorMessage($"{result}", ConsoleColor.DarkGray);
            }
            else
            {
                if (File.Exists($"{BuildOutputFolder}\\systeminformer-build-package-x64.msix"))
                {
                    File.Move(
                        $"{BuildOutputFolder}\\systeminformer-build-package-x64.msix",
                        $"{BuildOutputFolder}\\systeminformer-build-package.msix"
                        );
                }
            }

            BuildMsixAppInstaller();

            Win32.DeleteFile("tools\\msix\\MsixManifest32.xml");
            Win32.DeleteFile("tools\\msix\\MsixManifest64.xml");
            Win32.DeleteFile("tools\\msix\\MsixPackage32.map");
            Win32.DeleteFile("tools\\msix\\MsixPackage64.map");
            Win32.DeleteFile("tools\\msix\\bundle.map");
        }

        public static void CopySourceLink(bool CreateSourceLink)
        {
            if (CreateSourceLink)
            {
                if (!string.IsNullOrEmpty(Build.BuildCommitHash))
                {
                    Build.BuildSourceLink = $"{Build.BuildWorkingFolder}\\sourcelink.json";

                    JsonObject jsonObject = new JsonObject
                    {
                        ["documents"] = new JsonObject
                        {
                            ["\\*"] = $"https://raw.githubusercontent.com/winsiderss/systeminformer/{Build.BuildCommitHash}*",
                            [$"{Path.Join([Build.BuildWorkingFolder, "\\"])}*"] = $"https://raw.githubusercontent.com/winsiderss/systeminformer/{Build.BuildCommitHash}*"
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

        public static void CleanupBuildEnvironment()
        {
            try
            {
                if (!string.IsNullOrWhiteSpace(Utils.GetGitFilePath()))
                {
                    string output = Utils.ExecuteGitCommand(BuildWorkingFolder, "clean -x -d -f");

                    Program.PrintColorMessage(output, ConsoleColor.DarkGray);
                }

                {
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
                                    Program.PrintColorMessage($"[ERROR] {ex}", ConsoleColor.Red);
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
                            Program.PrintColorMessage($"Deleting: {path}", ConsoleColor.DarkGray);

                            try
                            {
                                Win32.DeleteFile(path);
                            }
                            catch (Exception ex)
                            {
                                Program.PrintColorMessage($"[ERROR] {ex}", ConsoleColor.Red);
                            }
                        }
                    }
                }
            }
            catch (Exception ex)
            {
                Program.PrintColorMessage($"[Cleanup] {ex}", ConsoleColor.Red);
            }
        }

        public static void ReflowExportDefinitions(bool ReleaseBuild)
        {
            StringBuilder output = new StringBuilder();
            int total = 0; // ushort.MaxValue

            {
                WIN32_FILE_ATTRIBUTE_DATA sourceFile;

                File.Copy(
                    "SystemInformer\\SystemInformer.def",
                    "SystemInformer\\SystemInformer.bak",
                    true
                    );

                if (PInvoke.GetFileAttributesEx(
                    "SystemInformer\\SystemInformer.def",
                    GET_FILEEX_INFO_LEVELS.GetFileExInfoStandard,
                    &sourceFile))
                {
                    using (var fs = File.OpenWrite("SystemInformer\\SystemInformer.bak"))
                    {
                        PInvoke.SetFileTime(
                            fs.SafeFileHandle,
                            sourceFile.ftCreationTime,
                            sourceFile.ftLastAccessTime,
                            sourceFile.ftLastWriteTime
                            );
                    }
                }
            }

            var content = File.ReadAllText("SystemInformer\\SystemInformer.def");
            var lines = content.Split("\r\n");

            foreach (var line in lines)
            {
                if (line.StartsWith("    ", StringComparison.OrdinalIgnoreCase))
                {
                    total++;
                }
            }

            //using (var reader = File.OpenText("SystemInformer\\SystemInformer.def"))
            //{
            //    while (reader.ReadLine() is { } line)
            //    {
            //        if (line.Contains('@', StringComparison.OrdinalIgnoreCase))
            //        {
            //            total++;
            //        }
            //    }
            //}

            List<int> ordinals = new List<int>();

            if (ReleaseBuild)
            {
                while (ordinals.Count < total)
                {
                    var value = Random.Shared.Next(1000, 1000 + total);

                    if (!ordinals.Contains(value))
                    {
                        ordinals.Add(value);
                    }
                }
            }
            else
            {
                while (ordinals.Count < total)
                {
                    ordinals.Add(1000 + ordinals.Count + 1);
                }
            }
            foreach (string line in lines)
            {
                var span = line.AsSpan();

                if (span.IsEmpty || span.IsWhiteSpace())
                {
                    output.Append(span);
                    output.Append(Environment.NewLine);
                }
                else
                {
                    if (span.StartsWith("    ", StringComparison.OrdinalIgnoreCase))
                    {
                        var ordinal = ordinals[0]; ordinals.RemoveAt(0);

                        output.Append(line.Replace("DATA", ""));
                        output.Append(" @");
                        output.Append(ordinal);
                        output.Append(" NONAME");

                        if (span.EndsWith("DATA", StringComparison.OrdinalIgnoreCase))
                        {
                            output.Append(" DATA");
                        }

                        output.Append(Environment.NewLine);
                    }
                    else
                    {
                        output.Append(span);
                        output.Append(Environment.NewLine);
                    }
                }
            }

            string export_content = output.ToString().TrimEnd();

            Utils.WriteAllText("SystemInformer\\SystemInformer.def", export_content);
        }

        public static void RestoreExportDefinitions()
        {
            if (File.Exists("SystemInformer\\SystemInformer.bak"))
            {
                WIN32_FILE_ATTRIBUTE_DATA sourceFile;

                File.Copy(
                    "SystemInformer\\SystemInformer.bak",
                    "SystemInformer\\SystemInformer.def",
                    true
                    );

                if (PInvoke.GetFileAttributesEx(
                    "SystemInformer\\SystemInformer.bak", 
                    GET_FILEEX_INFO_LEVELS.GetFileExInfoStandard, 
                    &sourceFile))
                {
                    using (var fs = File.OpenWrite("SystemInformer\\SystemInformer.def"))
                    {
                        PInvoke.SetFileTime(
                            fs.SafeFileHandle,
                            sourceFile.ftCreationTime,
                            sourceFile.ftLastAccessTime,
                            sourceFile.ftLastWriteTime
                            );
                    }
                }

                File.Delete("SystemInformer\\SystemInformer.bak");
            }
        }

    }
}
