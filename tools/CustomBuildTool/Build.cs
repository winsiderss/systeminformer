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
        private static bool BuildNightly = false;
        private static bool HaveArm64BuildTools = true;
        public static string BuildOutputFolder = string.Empty;
        public static string BuildBranch = string.Empty;
        public static string BuildCommit = string.Empty;
        public static string BuildVersion = "1.0.0";
        public static string BuildLongVersion = "1.0.0.0";
        public static string BuildCount = string.Empty;
        public static string BuildRevision = string.Empty;

        public static bool InitializeBuildEnvironment()
        {
            Console.InputEncoding = Encoding.UTF8;
            Console.OutputEncoding = Encoding.UTF8;

            try
            {
                DirectoryInfo info = new DirectoryInfo(".");

                while (info.Parent != null && info.Parent.Parent != null)
                {
                    info = info.Parent;

                    if (File.Exists($"{info.FullName}\\SystemInformer.sln"))
                    {
                        Directory.SetCurrentDirectory(info.FullName);
                        break;
                    }
                }
            }
            catch (Exception ex)
            {
                Program.PrintColorMessage($"Unable to find project root directory: {ex}", ConsoleColor.Red);
                return false;
            }

            if (!File.Exists("SystemInformer.sln"))
            {
                Program.PrintColorMessage("Unable to find project solution.", ConsoleColor.Red);
                return false;
            }

            Build.TimeStart = DateTime.Now;
            Build.BuildNightly = !string.IsNullOrWhiteSpace(Win32.GetEnvironmentVariable("%APPVEYOR_BUILD_API%"));
            Build.BuildOutputFolder = Utils.GetOutputDirectoryPath();

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

        public static void CleanupBuildEnvironment()
        {
            try
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
                    string name = Path.GetFileName(path);

                    if (
                        string.Equals(name, ".vs", StringComparison.OrdinalIgnoreCase) ||
                        string.Equals(name, "obj", StringComparison.OrdinalIgnoreCase)
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
                    string name = Path.GetFileName(path);

                    if (name.EndsWith(".aps", StringComparison.OrdinalIgnoreCase))
                    {
                        if (File.Exists(path))
                        {
                            Program.PrintColorMessage($"Deleting: {path}", ConsoleColor.DarkGray);

                            try
                            {
                                File.Delete(path);
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

        public static void SetupBuildEnvironment(bool ShowBuildInfo)
        {
            string currentGitDir = Utils.GetGitWorkPath();
            string currentGitPath = Utils.GetGitFilePath();

            if (!string.IsNullOrWhiteSpace(currentGitDir) && !string.IsNullOrWhiteSpace(currentGitPath))
            {
                BuildBranch = Win32.ShellExecute(currentGitPath, $"{currentGitDir} rev-parse --abbrev-ref HEAD").Trim();
                BuildCommit = Win32.ShellExecute(currentGitPath, $"{currentGitDir} rev-parse HEAD").Trim();
                BuildCount = Win32.ShellExecute(currentGitPath, $"{currentGitDir} rev-list --count {BuildBranch}").Trim();
                string currentGitTag = Win32.ShellExecute(currentGitPath, $"{currentGitDir} describe --abbrev=0 --tags --always").Trim();

                if (!string.IsNullOrWhiteSpace(currentGitTag))
                {
                    BuildRevision = Win32.ShellExecute(currentGitPath, $"{currentGitDir} rev-list --count \"{currentGitTag}..{BuildBranch}\"").Trim();
                    BuildVersion = $"3.0.{BuildRevision}";
                    BuildLongVersion = $"3.0.{BuildCount}.{BuildRevision}";
                }
            }

            if (
                string.IsNullOrWhiteSpace(BuildBranch) ||
                string.IsNullOrWhiteSpace(BuildCommit) ||
                string.IsNullOrWhiteSpace(BuildCount) ||
                string.IsNullOrWhiteSpace(BuildRevision)
                )
            {
                BuildBranch = string.Empty;
                BuildCommit = string.Empty;
                BuildCount = string.Empty;
                BuildRevision = string.Empty;
                BuildVersion = "1.0.0";
                BuildLongVersion = "1.0.0.0";
            }

            if (ShowBuildInfo)
            {
                Program.PrintColorMessage("Windows: ", ConsoleColor.DarkGray, false);
                Program.PrintColorMessage(Win32.GetKernelVersion(), ConsoleColor.Green, true);

                var instance = VisualStudio.GetVisualStudioInstance();
                if (instance != null)
                {
                    Program.PrintColorMessage("WindowsSDK: ", ConsoleColor.DarkGray, false);
                    Program.PrintColorMessage(Utils.GetWindowsSdkVersion(), ConsoleColor.Green, true);
                    //Program.PrintColorMessage(Utils.GetWindowsSdkVersion() + " (" + instance.GetWindowsSdkFullVersion() + ")", ConsoleColor.Green, true);
                    Program.PrintColorMessage("VisualStudio: ", ConsoleColor.DarkGray, false);
                    Program.PrintColorMessage(instance.Name, ConsoleColor.Green, true);
                    //Program.PrintColorMessage(Utils.GetVisualStudioVersion(), ConsoleColor.Green, true);
                    HaveArm64BuildTools = instance.HasARM64BuildToolsComponents;
                }

                Program.PrintColorMessage(Environment.NewLine + "Building... ", ConsoleColor.DarkGray, false);
                Program.PrintColorMessage(BuildLongVersion, ConsoleColor.Green, false);

                if (!string.IsNullOrWhiteSpace(BuildCommit))
                {
                    Program.PrintColorMessage(" (", ConsoleColor.DarkGray, false);
                    Program.PrintColorMessage(BuildCommit.Substring(0, 7), ConsoleColor.DarkYellow, false);
                    Program.PrintColorMessage(")", ConsoleColor.DarkGray, false);
                }

                if (!string.IsNullOrWhiteSpace(BuildBranch))
                {
                    Program.PrintColorMessage(" [", ConsoleColor.DarkGray, false);
                    Program.PrintColorMessage(BuildBranch, ConsoleColor.DarkBlue, false);
                    Program.PrintColorMessage("]", ConsoleColor.DarkGray, false);
                }

                Program.PrintColorMessage(Environment.NewLine, ConsoleColor.DarkGray, true);
            }
        }

        public static string BuildTimeStamp()
        {
            return $"[{DateTime.Now - TimeStart:mm\\:ss}] ";
        }

        public static void ShowBuildStats()
        {
            TimeSpan buildTime = DateTime.Now - TimeStart;

            Program.PrintColorMessage($"{Environment.NewLine}Build Time: ", ConsoleColor.DarkGray, false);
            Program.PrintColorMessage(buildTime.Minutes.ToString(), ConsoleColor.Green, false);
            Program.PrintColorMessage(" minute(s), ", ConsoleColor.DarkGray, false);
            Program.PrintColorMessage(buildTime.Seconds.ToString(), ConsoleColor.Green, false);
            Program.PrintColorMessage($" second(s) {Environment.NewLine + Environment.NewLine}", ConsoleColor.DarkGray, false);
        }

        public static bool CopyTextFiles(bool Update)
        {
            try
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
            }
            catch (Exception ex)
            {
                Program.PrintColorMessage($"[CopyTextFiles] {ex}", ConsoleColor.Red);
                return false;
            }

            return true;
        }

        public static bool CopyWow64Files(BuildFlags Flags)
        {
            try
            {
                if (Flags.HasFlag(BuildFlags.BuildDebug))
                {
                    if (Flags.HasFlag(BuildFlags.Build64bit))
                    {
                        if (Directory.Exists("bin\\Debug64"))
                        {
                            Win32.CreateDirectory("bin\\Debug64\\x86");
                            Win32.CreateDirectory("bin\\Debug64\\x86\\plugins");

                            Win32.CopyIfNewer("bin\\Debug32\\SystemInformer.exe", "bin\\Debug64\\x86\\SystemInformer.exe");
                            Win32.CopyIfNewer("bin\\Debug32\\SystemInformer.pdb", "bin\\Debug64\\x86\\SystemInformer.pdb");
                            Win32.CopyIfNewer("bin\\Debug32\\SystemInformer.sig", "bin\\Debug64\\x86\\SystemInformer.sig");

                            Win32.CopyIfNewer("bin\\Debug32\\plugins\\DotNetTools.dll", "bin\\Debug64\\x86\\plugins\\DotNetTools.dll");
                            Win32.CopyIfNewer("bin\\Debug32\\plugins\\DotNetTools.pdb", "bin\\Debug64\\x86\\plugins\\DotNetTools.pdb");
                            Win32.CopyIfNewer("bin\\Debug32\\plugins\\DotNetTools.sig", "bin\\Debug64\\x86\\plugins\\DotNetTools.sig");

                            Win32.CopyIfNewer("bin\\Debug32\\plugins\\ExtendedTools.dll", "bin\\Debug64\\x86\\plugins\\ExtendedTools.dll");
                            Win32.CopyIfNewer("bin\\Debug32\\plugins\\ExtendedTools.pdb", "bin\\Debug64\\x86\\plugins\\ExtendedTools.pdb");
                            Win32.CopyIfNewer("bin\\Debug32\\plugins\\ExtendedTools.sig", "bin\\Debug64\\x86\\plugins\\ExtendedTools.sig");
                        }
                    }
                }

                if (Flags.HasFlag(BuildFlags.BuildRelease))
                {
                    if (Flags.HasFlag(BuildFlags.Build64bit))
                    {
                        if (Directory.Exists("bin\\Release64"))
                        {
                            Win32.CreateDirectory("bin\\Release64\\x86");
                            Win32.CreateDirectory("bin\\Release64\\x86\\plugins");

                            Win32.CopyIfNewer("bin\\Release32\\SystemInformer.exe", "bin\\Release64\\x86\\SystemInformer.exe");
                            Win32.CopyIfNewer("bin\\Release32\\SystemInformer.pdb", "bin\\Release64\\x86\\SystemInformer.pdb");
                            Win32.CopyIfNewer("bin\\Release32\\SystemInformer.sig", "bin\\Release64\\x86\\SystemInformer.sig");

                            Win32.CopyIfNewer("bin\\Release32\\plugins\\DotNetTools.dll", "bin\\Release64\\x86\\plugins\\DotNetTools.dll");
                            Win32.CopyIfNewer("bin\\Release32\\plugins\\DotNetTools.pdb", "bin\\Release64\\x86\\plugins\\DotNetTools.pdb");
                            Win32.CopyIfNewer("bin\\Release32\\plugins\\DotNetTools.sig", "bin\\Release64\\x86\\plugins\\DotNetTools.sig");

                            Win32.CopyIfNewer("bin\\Release32\\plugins\\ExtendedTools.dll", "bin\\Release64\\x86\\plugins\\ExtendedTools.dll");
                            Win32.CopyIfNewer("bin\\Release32\\plugins\\ExtendedTools.pdb", "bin\\Release64\\x86\\plugins\\ExtendedTools.pdb");
                            Win32.CopyIfNewer("bin\\Release32\\plugins\\ExtendedTools.sig", "bin\\Release64\\x86\\plugins\\ExtendedTools.sig");
                        }
                    }
                }
            }
            catch (Exception ex)
            {
                Program.PrintColorMessage($"[ERROR] {ex}", ConsoleColor.Red);
                return false;
            }

            return true;
        }

        public static bool CopySidCapsFile(BuildFlags Flags)
        {
            try
            {
                if (Flags.HasFlag(BuildFlags.BuildDebug))
                {
                    if (Flags.HasFlag(BuildFlags.Build32bit) && Directory.Exists("bin\\Debug32"))
                        Win32.CopyIfNewer("SystemInformer\\resources\\capslist.txt", "bin\\Debug32\\capslist.txt");
                    if (Flags.HasFlag(BuildFlags.Build64bit) && Directory.Exists("bin\\Debug64"))
                        Win32.CopyIfNewer("SystemInformer\\resources\\capslist.txt", "bin\\Debug64\\capslist.txt");
                    if (Flags.HasFlag(BuildFlags.BuildArm64bit) && Directory.Exists("bin\\DebugARM64"))
                        Win32.CopyIfNewer("SystemInformer\\resources\\capslist.txt", "bin\\DebugARM64\\capslist.txt");
                }

                if (Flags.HasFlag(BuildFlags.BuildRelease))
                {
                    if (Flags.HasFlag(BuildFlags.Build32bit) && Directory.Exists("bin\\Release32"))
                        Win32.CopyIfNewer("SystemInformer\\resources\\capslist.txt", "bin\\Release32\\capslist.txt");
                    if (Flags.HasFlag(BuildFlags.Build64bit) && Directory.Exists("bin\\Release64"))
                        Win32.CopyIfNewer("SystemInformer\\resources\\capslist.txt", "bin\\Release64\\capslist.txt");
                    if (Flags.HasFlag(BuildFlags.BuildArm64bit) && Directory.Exists("bin\\ReleaseARM64"))
                        Win32.CopyIfNewer("SystemInformer\\resources\\capslist.txt", "bin\\ReleaseARM64\\capslist.txt");
                }
            }
            catch (Exception ex)
            {
                Program.PrintColorMessage($"[ERROR] {ex}", ConsoleColor.Red);
                return false;
            }

            return true;
        }

        public static bool CopyEtwTraceGuidsFile(BuildFlags Flags)
        {
            try
            {
                if (Flags.HasFlag(BuildFlags.BuildDebug))
                {
                    if (Flags.HasFlag(BuildFlags.Build32bit) && Directory.Exists("bin\\Debug32"))
                        Win32.CopyIfNewer("SystemInformer\\resources\\etwguids.txt", "bin\\Debug32\\etwguids.txt");
                    if (Flags.HasFlag(BuildFlags.Build64bit) && Directory.Exists("bin\\Debug64"))
                        Win32.CopyIfNewer("SystemInformer\\resources\\etwguids.txt", "bin\\Debug64\\etwguids.txt");
                    if (Flags.HasFlag(BuildFlags.BuildArm64bit) && Directory.Exists("bin\\DebugARM64"))
                        Win32.CopyIfNewer("SystemInformer\\resources\\etwguids.txt", "bin\\DebugARM64\\etwguids.txt");
                }

                if (Flags.HasFlag(BuildFlags.BuildRelease))
                {
                    if (Flags.HasFlag(BuildFlags.Build32bit) && Directory.Exists("bin\\Release32"))
                        Win32.CopyIfNewer("SystemInformer\\resources\\etwguids.txt", "bin\\Release32\\etwguids.txt");
                    if (Flags.HasFlag(BuildFlags.Build64bit) && Directory.Exists("bin\\Release64"))
                        Win32.CopyIfNewer("SystemInformer\\resources\\etwguids.txt", "bin\\Release64\\etwguids.txt");
                    if (Flags.HasFlag(BuildFlags.BuildArm64bit) && Directory.Exists("bin\\ReleaseARM64"))
                        Win32.CopyIfNewer("SystemInformer\\resources\\etwguids.txt", "bin\\ReleaseARM64\\etwguids.txt");
                }
            }
            catch (Exception ex)
            {
                Program.PrintColorMessage($"[ERROR] {ex}", ConsoleColor.Red);
                return false;
            }

            return true;
        }

        public static bool BuildPublicHeaderFiles()
        {
            Program.PrintColorMessage(BuildTimeStamp(), ConsoleColor.DarkGray, false);
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

        public static bool BuildDynamicHeaderFiles()
        {
            //Program.PrintColorMessage(BuildTimeStamp(), ConsoleColor.DarkGray, false);
            //Program.PrintColorMessage("Building dynamic headers...", ConsoleColor.Cyan);

            try
            {
                DynData.Execute();
            }
            catch (Exception ex)
            {
                Program.PrintColorMessage($"[ERROR] {ex}", ConsoleColor.Red);
                return false;
            }

            Program.PrintColorMessage("Done!", ConsoleColor.Green);

            return true;
        }

        public static bool CopyKernelDriver(BuildFlags Flags)
        {
            string CustomSignToolPath = Verify.GetCustomSignToolFilePath();

            try
            {
                if (Flags.HasFlag(BuildFlags.BuildDebug))
                {
                    if (Flags.HasFlag(BuildFlags.Build32bit))
                    {
                        if (Directory.Exists("KSystemInformer\\bin-signed\\i386") && Directory.Exists("bin\\Debug32"))
                        {
                            Win32.CopyIfNewer("KSystemInformer\\bin-signed\\i386\\systeminformer.sys", "bin\\Debug32\\SystemInformer.sys");
                            Win32.CopyIfNewer("KSystemInformer\\bin-signed\\i386\\ksi.dll", "bin\\Debug32\\ksi.dll");
                        }
                    }

                    if (Flags.HasFlag(BuildFlags.Build64bit))
                    {
                        if (Directory.Exists("KSystemInformer\\bin-signed\\amd64") && Directory.Exists("bin\\Debug64"))
                        {
                            Win32.CopyIfNewer("KSystemInformer\\bin-signed\\amd64\\systeminformer.sys", "bin\\Debug64\\SystemInformer.sys");
                            Win32.CopyIfNewer("KSystemInformer\\bin-signed\\amd64\\ksi.dll", "bin\\Debug64\\ksi.dll");
                        }
                    }

                    if (Flags.HasFlag(BuildFlags.BuildArm64bit))
                    {
                        if (Directory.Exists("KSystemInformer\\bin-signed\\arm64") && Directory.Exists("bin\\DebugARM64"))
                        {
                            Win32.CopyIfNewer("KSystemInformer\\bin-signed\\arm64\\systeminformer.sys", "bin\\DebugARM64\\SystemInformer.sys");
                            Win32.CopyIfNewer("KSystemInformer\\bin-signed\\arm64\\ksi.dll", "bin\\DebugARM64\\ksi.dll");
                        }
                    }
                }

                if (Flags.HasFlag(BuildFlags.BuildRelease))
                {
                    if (Flags.HasFlag(BuildFlags.Build32bit))
                    {
                        if (Directory.Exists("KSystemInformer\\bin-signed\\i386") && Directory.Exists("bin\\Release32"))
                        {
                            Win32.CopyIfNewer("KSystemInformer\\bin-signed\\i386\\systeminformer.sys", "bin\\Release32\\SystemInformer.sys");
                            Win32.CopyIfNewer("KSystemInformer\\bin-signed\\i386\\ksi.dll", "bin\\Release32\\ksi.dll");
                        }
                    }

                    if (Flags.HasFlag(BuildFlags.Build64bit))
                    {
                        if (Directory.Exists("KSystemInformer\\bin-signed\\amd64") && Directory.Exists("bin\\Release64"))
                        {
                            Win32.CopyIfNewer("KSystemInformer\\bin-signed\\amd64\\systeminformer.sys", "bin\\Release64\\SystemInformer.sys");
                            Win32.CopyIfNewer("KSystemInformer\\bin-signed\\amd64\\ksi.dll", "bin\\Release64\\ksi.dll");
                        }
                    }

                    if (Flags.HasFlag(BuildFlags.BuildArm64bit))
                    {
                        if (Directory.Exists("KSystemInformer\\bin-signed\\arm64") && Directory.Exists("bin\\ReleaseARM64"))
                        {
                            Win32.CopyIfNewer("KSystemInformer\\bin-signed\\arm64\\systeminformer.sys", "bin\\ReleaseARM64\\SystemInformer.sys");
                            Win32.CopyIfNewer("KSystemInformer\\bin-signed\\arm64\\ksi.dll", "bin\\ReleaseARM64\\ksi.dll");
                        }
                    }
                }
            }
            catch (Exception ex)
            {
                Program.PrintColorMessage($"[ERROR] (CopyKernelDriver) {ex}", ConsoleColor.Red);
            }

            if (BuildNightly && !File.Exists(Verify.GetPath("kph.key")))
            {
                string kphKey = Win32.GetEnvironmentVariable("%KPH_BUILD_KEY%");

                if (!string.IsNullOrWhiteSpace(kphKey))
                {
                    Verify.Decrypt(Verify.GetPath("kph.s"), Verify.GetPath("kph.key"), kphKey);
                }

                if (!File.Exists(Verify.GetPath("kph.key")))
                {
                    Program.PrintColorMessage("[SKIPPED] kph.key not found.", ConsoleColor.Yellow);
                    return true;
                }
            }

            if (File.Exists(CustomSignToolPath))
            {
                var files = new List<string>();

                if (Flags.HasFlag(BuildFlags.BuildDebug))
                {
                    if (Flags.HasFlag(BuildFlags.Build32bit))
                    {
                        files.Add("bin\\Debug32\\SystemInformer.exe");
                    }

                    if (Flags.HasFlag(BuildFlags.Build64bit))
                    {
                        files.Add("bin\\Debug64\\SystemInformer.exe");
                    }

                    if (Flags.HasFlag(BuildFlags.BuildArm64bit))
                    {
                        files.Add("bin\\DebugARM64\\SystemInformer.exe");
                    }
                }

                if (Flags.HasFlag(BuildFlags.BuildRelease))
                {
                    if (Flags.HasFlag(BuildFlags.Build32bit))
                    {
                        files.Add("bin\\Release32\\SystemInformer.exe");
                    }

                    if (Flags.HasFlag(BuildFlags.Build64bit))
                    {
                        files.Add("bin\\Release64\\SystemInformer.exe");
                    }

                    if (Flags.HasFlag(BuildFlags.BuildArm64bit))
                    {
                        files.Add("bin\\ReleaseARM64\\SystemInformer.exe");
                    }
                }

                foreach (string file in files)
                {
                    if (File.Exists(file))
                    {
                        string sigfile = Path.ChangeExtension(file, ".sig");
                        File.WriteAllText(sigfile, string.Empty);

                        if (File.Exists(Verify.GetPath("kph.key")))
                        {
                            int errorcode = Win32.CreateProcess(
                                CustomSignToolPath,
                                $"sign -k {Verify.GetPath("kph.key")} {file} -s {sigfile}",
                                out string errorstring
                                );

                            if (errorcode != 0)
                            {
                                Program.PrintColorMessage($"[CopyKernelDriver] ({errorcode}) {errorstring}", ConsoleColor.Red, true, BuildFlags.BuildVerbose);

                                if (BuildNightly)
                                {
                                    Win32.DeleteFile(Verify.GetPath("kph.key"));
                                }

                                return false;
                            }
                        }
                    }
                }
            }

            if (BuildNightly)
            {
                Win32.DeleteFile(Verify.GetPath("kph.key"));
            }

            return true;
        }
        
        public static bool SignPlugin(string PluginName)
        {
            if (!File.Exists(PluginName))
            {
                Program.PrintColorMessage($"[SKIPPED] Plugin not found: {PluginName}", ConsoleColor.Yellow);
                return true;
            }

            string CustomSignToolPath = Verify.GetCustomSignToolFilePath();

            if (!File.Exists(CustomSignToolPath))
            {
                Program.PrintColorMessage($"[SKIPPED] CustomSignTool not found: {PluginName}", ConsoleColor.Yellow);
                return true;
            }

            if (BuildNightly && !File.Exists(Verify.GetPath("kph.key")))
            {
                string kphKey = Win32.GetEnvironmentVariable("%KPH_BUILD_KEY%");

                if (!string.IsNullOrWhiteSpace(kphKey))
                {
                    Verify.Decrypt(Verify.GetPath("kph.s"), Verify.GetPath("kph.key"), kphKey);
                }

                if (!File.Exists(Verify.GetPath("kph.key")))
                {
                    Program.PrintColorMessage("[SKIPPED] kph.key not found.", ConsoleColor.Yellow);
                    return true;
                }
            }

            {
                string sigfile = Path.ChangeExtension(PluginName, ".sig");
                File.WriteAllText(sigfile, string.Empty);

                if (File.Exists(Verify.GetPath("kph.key")))
                {
                    int errorcode = Win32.CreateProcess(
                        CustomSignToolPath,
                        $"sign -k {Verify.GetPath("kph.key")} {PluginName} -s {sigfile}",
                        out string errorstring
                        );

                    if (BuildNightly)
                    {
                        Win32.DeleteFile(Verify.GetPath("kph.key"));
                    }

                    if (errorcode != 0)
                    {
                        Program.PrintColorMessage($"[SignPlugin] ({errorcode}) {errorstring}", ConsoleColor.Red, true, BuildFlags.BuildVerbose);
                        return false;
                    }
                }
            }

            if (BuildNightly)
            {
                Win32.DeleteFile(Verify.GetPath("kph.key"));
            }

            return true;
        }

        //public static bool BuildWebSetupExe()
        //{
        //    Program.PrintColorMessage(BuildTimeStamp(), ConsoleColor.DarkGray, false);
        //    Program.PrintColorMessage("Building build-websetup.exe...", ConsoleColor.Cyan);
        //
        //    if (!BuildSolution("tools\\CustomSetupTool\\CustomSetupTool.sln", BuildFlags.Build32bit))
        //        return false;
        //
        //    try
        //    {
        //        Win32.DeleteFile(
        //            $"{BuildOutputFolder}\\systeminformer-build-websetup.exe"
        //            );
        //
        //        File.Move(
        //            "tools\\CustomSetupTool\\bin\\Release32\\CustomSetupTool.exe",
        //            $"{BuildOutputFolder}\\systeminformer-build-websetup.exe"
        //            );
        //    }
        //    catch (Exception ex)
        //    {
        //        Program.PrintColorMessage($"[ERROR] {ex}", ConsoleColor.Red);
        //        return false;
        //    }
        //
        //    return true;
        //}

        public static bool BuildSdk(BuildFlags Flags)
        {
            try
            {
                foreach (string folder in BuildConfig.Build_Sdk_Directories)
                {
                    Win32.CreateDirectory(folder);
                }

                // Copy the plugin SDK headers
                foreach (string file in BuildConfig.Build_Phnt_Headers)
                    Win32.CopyIfNewer($"phnt\\include\\{file}", $"sdk\\include\\{file}");
                foreach (string file in BuildConfig.Build_Phlib_Headers)
                    Win32.CopyIfNewer($"phlib\\include\\{file}", $"sdk\\include\\{file}");

                // Copy readme
                Win32.CopyIfNewer("SystemInformer\\sdk\\readme.txt", "sdk\\readme.txt");
                // Copy symbols
                //Win32.CopyIfNewer("bin\\Release32\\SystemInformer.pdb", "sdk\\dbg\\i386\\SystemInformer.pdb");
                //Win32.CopyIfNewer("bin\\Release64\\SystemInformer.pdb", "sdk\\dbg\\amd64\\SystemInformer.pdb");
                //Win32.CopyIfNewer("bin\\ReleaseARM64\\SystemInformer.pdb", "sdk\\dbg\\arm64\\SystemInformer.pdb");
                //Win32.CopyIfNewer("KSystemInformer\\bin\\i386\\systeminformer.pdb", "sdk\\dbg\\i386\\ksysteminformer.pdb");
                //Win32.CopyIfNewer("KSystemInformer\\bin\\amd64\\systeminformer.pdb", "sdk\\dbg\\amd64\\ksysteminformer.pdb");
                //Win32.CopyIfNewer("KSystemInformer\\bin\\arm64\\systeminformer.pdb", "sdk\\dbg\\arm64\\ksysteminformer.pdb");
                //Win32.CopyIfNewer("KSystemInformer\\bin\\i386\\ksi.pdb", "sdk\\dbg\\i386\\ksi.pdb");
                //Win32.CopyIfNewer("KSystemInformer\\bin\\amd64\\ksi.pdb", "sdk\\dbg\\amd64\\ksi.pdb");
                //Win32.CopyIfNewer("KSystemInformer\\bin\\arm64\\ksi.pdb", "sdk\\dbg\\arm64\\ksi.pdb");

                // Copy libs
                if (Flags.HasFlag(BuildFlags.BuildDebug))
                {
                    if (Flags.HasFlag(BuildFlags.Build32bit))
                        Win32.CopyIfNewer("bin\\Debug32\\SystemInformer.lib", "sdk\\lib\\i386\\SystemInformer.lib");
                    if (Flags.HasFlag(BuildFlags.Build64bit))
                        Win32.CopyIfNewer("bin\\Debug64\\SystemInformer.lib", "sdk\\lib\\amd64\\SystemInformer.lib");
                    if (Flags.HasFlag(BuildFlags.BuildArm64bit))
                        Win32.CopyIfNewer("bin\\DebugARM64\\SystemInformer.lib", "sdk\\lib\\arm64\\SystemInformer.lib");
                }

                if (Flags.HasFlag(BuildFlags.BuildRelease))
                {
                    if (Flags.HasFlag(BuildFlags.Build32bit))
                        Win32.CopyIfNewer("bin\\Release32\\SystemInformer.lib", "sdk\\lib\\i386\\SystemInformer.lib");
                    if (Flags.HasFlag(BuildFlags.Build64bit))
                        Win32.CopyIfNewer("bin\\Release64\\SystemInformer.lib", "sdk\\lib\\amd64\\SystemInformer.lib");
                    if (Flags.HasFlag(BuildFlags.BuildArm64bit))
                        Win32.CopyIfNewer("bin\\ReleaseARM64\\SystemInformer.lib", "sdk\\lib\\arm64\\SystemInformer.lib");
                }

                // Build the SDK
                HeaderGen.Execute();

                // Copy the resource header
                Win32.CopyIfNewer("SystemInformer\\sdk\\phapppub.h", "sdk\\include\\phapppub.h");
                Win32.CopyIfNewer("SystemInformer\\sdk\\phdk.h", "sdk\\include\\phdk.h");
                Win32.CopyIfNewer("SystemInformer\\resource.h", "sdk\\include\\phappresource.h");

                // Append resource headers with SDK exports
                string phappContent = File.ReadAllText("sdk\\include\\phappresource.h");

                if (!string.IsNullOrWhiteSpace(phappContent))
                {
                    phappContent = phappContent.Replace("#define ID", "#define PHAPP_ID", StringComparison.OrdinalIgnoreCase);
                    File.WriteAllText("sdk\\include\\phappresource.h", phappContent);
                }
            }
            catch (Exception ex)
            {
                Program.PrintColorMessage($"[ERROR] {ex}", ConsoleColor.Red);
                return false;
            }

            return true;
        }

        public static bool BuildSetupExe()
        {
            Program.PrintColorMessage(BuildTimeStamp(), ConsoleColor.DarkGray, false);
            Program.PrintColorMessage("Building build-setup.exe... ", ConsoleColor.Cyan, false);

            if (!BuildSolution("tools\\CustomSetupTool\\CustomSetupTool.sln", BuildFlags.Build32bit | BuildFlags.BuildApi))
                return false;

            try
            {
                Utils.CreateOutputDirectory();

                Win32.DeleteFile(
                    $"{BuildOutputFolder}\\systeminformer-build-setup.exe"
                    );

                File.Move(
                    "tools\\CustomSetupTool\\bin\\Release32\\CustomSetupTool.exe",
                    $"{BuildOutputFolder}\\systeminformer-build-setup.exe"
                    );

                Program.PrintColorMessage(new FileInfo($"{BuildOutputFolder}\\systeminformer-build-setup.exe").Length.ToPrettySize(), ConsoleColor.Green);
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
            Program.PrintColorMessage(BuildTimeStamp(), ConsoleColor.DarkGray, false);
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

                Program.PrintColorMessage(new FileInfo($"{BuildOutputFolder}\\systeminformer-build-sdk.zip").Length.ToPrettySize(), ConsoleColor.Green);
            }
            catch (Exception ex)
            {
                Program.PrintColorMessage($"[ERROR] {ex}", ConsoleColor.Red);
                return false;
            }

            return true;
        }

        public static bool BuildBinZip()
        {
            Program.PrintColorMessage(BuildTimeStamp(), ConsoleColor.DarkGray, false);
            Program.PrintColorMessage("Building build-bin.zip... ", ConsoleColor.Cyan, false);

            try
            {
                Win32.DeleteFile("bin\\Release32\\SystemInformer.exe.settings.xml");
                Win32.DeleteFile("bin\\Release64\\SystemInformer.exe.settings.xml");
                Win32.DeleteFile("bin\\ReleaseARM64\\SystemInformer.exe.settings.xml");

                if (Directory.Exists("bin\\Release32"))
                    File.Create("bin\\Release32\\SystemInformer.exe.settings.xml").Dispose();
                if (Directory.Exists("bin\\Release64"))
                    File.Create("bin\\Release64\\SystemInformer.exe.settings.xml").Dispose();
                if (Directory.Exists("bin\\ReleaseARM64"))
                    File.Create("bin\\ReleaseARM64\\SystemInformer.exe.settings.xml").Dispose();
            }
            catch (Exception ex)
            {
                Program.PrintColorMessage($"[WARN] {ex}", ConsoleColor.Yellow);
            }

            try
            {
                Win32.DeleteFile("bin\\Release32\\usernotesdb.xml");
                Win32.DeleteFile("bin\\Release64\\usernotesdb.xml");
                Win32.DeleteFile("bin\\ReleaseARM64\\usernotesdb.xml");

                if (Directory.Exists("bin\\Release32"))
                    File.Create("bin\\Release32\\usernotesdb.xml").Dispose();
                if (Directory.Exists("bin\\Release64"))
                    File.Create("bin\\Release64\\usernotesdb.xml").Dispose();
                if (Directory.Exists("bin\\ReleaseARM64"))
                    File.Create("bin\\ReleaseARM64\\usernotesdb.xml").Dispose();
            }
            catch (Exception ex)
            {
                Program.PrintColorMessage($"[WARN] {ex}", ConsoleColor.Yellow);
            }

            try
            {
                Utils.CreateOutputDirectory();

                Win32.DeleteFile(
                    $"{BuildOutputFolder}\\systeminformer-build-bin.zip"
                    );

                Zip.CreateCompressedFolder(
                    "bin",
                    $"{BuildOutputFolder}\\systeminformer-build-bin.zip"
                    );

                Program.PrintColorMessage(new FileInfo($"{BuildOutputFolder}\\systeminformer-build-bin.zip").Length.ToPrettySize(), ConsoleColor.Green);
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
            Program.PrintColorMessage(BuildTimeStamp(), ConsoleColor.DarkGray, false);
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

                    Program.PrintColorMessage(new FileInfo($"{BuildOutputFolder}\\systeminformer-setup-package-pdb.zip").Length.ToPrettySize(), ConsoleColor.Green);
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

                    Program.PrintColorMessage(new FileInfo($"{BuildOutputFolder}\\systeminformer-build-pdb.zip").Length.ToPrettySize(), ConsoleColor.Green);
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
            Program.PrintColorMessage(BuildTimeStamp(), ConsoleColor.DarkGray, false);
            Program.PrintColorMessage("Building release checksums...", ConsoleColor.Cyan);

            try
            {
                StringBuilder sb = new StringBuilder();

                foreach (BuildFile name in BuildConfig.Build_Release_Files)
                {
                    if (!name.UploadNightly)
                        continue;

                    string file = BuildOutputFolder + name.FileName;

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

                File.WriteAllText(
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

        public static bool BuildSolution(string Solution, BuildFlags Flags)
        {
            string msbuildExePath = Utils.GetMsbuildFilePath();

            if (!File.Exists(msbuildExePath))
            {
                Program.PrintColorMessage("Unable to find MsBuild.", ConsoleColor.Red);
                return false;
            }

            if (Flags.HasFlag(BuildFlags.Build32bit))
            {
                StringBuilder compilerOptions = new StringBuilder(0x100);
                StringBuilder commandLine = new StringBuilder(0x100);

                Program.PrintColorMessage(BuildTimeStamp(), ConsoleColor.DarkGray, false, Flags);
                Program.PrintColorMessage($"Building {Path.GetFileNameWithoutExtension(Solution)} (", ConsoleColor.Cyan, false, Flags);
                Program.PrintColorMessage("x32", ConsoleColor.Green, false, Flags);
                Program.PrintColorMessage(")...", ConsoleColor.Cyan, true, Flags);

                if (Flags.HasFlag(BuildFlags.BuildApi))
                    compilerOptions.Append("PH_BUILD_API;");
                if (Flags.HasFlag(BuildFlags.BuildMsix))
                    compilerOptions.Append("PH_BUILD_MSIX;");
                if (!string.IsNullOrWhiteSpace(BuildCommit))
                    compilerOptions.Append($"PHAPP_VERSION_COMMITHASH=\"{BuildCommit.AsSpan(0, 7)}\";");
                if (!string.IsNullOrWhiteSpace(BuildRevision))
                    compilerOptions.Append($"PHAPP_VERSION_REVISION=\"{BuildRevision}\";");
                if (!string.IsNullOrWhiteSpace(BuildCount))
                    compilerOptions.Append($"PHAPP_VERSION_BUILD=\"{BuildCount}\"");

                commandLine.Append("/m /nologo /nodereuse:false /verbosity:quiet ");
                commandLine.Append("/p:Platform=Win32 /p:Configuration=" + (Flags.HasFlag(BuildFlags.BuildDebug) ? "Debug " : "Release "));
                commandLine.Append($"/p:ExternalCompilerOptions=\"{compilerOptions.ToString()}\" ");
                commandLine.Append(Solution);

                int errorcode = Win32.CreateProcess(
                    msbuildExePath,
                    commandLine.ToString(),
                    out string errorstring
                    );

                if (errorcode != 0)
                {
                    Program.PrintColorMessage($"[ERROR] ({errorcode}) {errorstring}", ConsoleColor.Red, true, Flags | BuildFlags.BuildVerbose);
                    return false;
                }
            }

            if (Flags.HasFlag(BuildFlags.Build64bit))
            {
                StringBuilder compilerOptions = new StringBuilder(0x100);
                StringBuilder commandLine = new StringBuilder(0x100);

                Program.PrintColorMessage(BuildTimeStamp(), ConsoleColor.DarkGray, false, Flags);
                Program.PrintColorMessage($"Building {Path.GetFileNameWithoutExtension(Solution)} (", ConsoleColor.Cyan, false, Flags);
                Program.PrintColorMessage("x64", ConsoleColor.Green, false, Flags);
                Program.PrintColorMessage(")...", ConsoleColor.Cyan, true, Flags);

                if (Flags.HasFlag(BuildFlags.BuildApi))
                    compilerOptions.Append("PH_BUILD_API;");
                if (Flags.HasFlag(BuildFlags.BuildMsix))
                    compilerOptions.Append("PH_BUILD_MSIX;");
                if (!string.IsNullOrWhiteSpace(BuildCommit))
                    compilerOptions.Append($"PHAPP_VERSION_COMMITHASH=\"{BuildCommit.AsSpan(0, 7)}\";");
                if (!string.IsNullOrWhiteSpace(BuildRevision))
                    compilerOptions.Append($"PHAPP_VERSION_REVISION=\"{BuildRevision}\";");
                if (!string.IsNullOrWhiteSpace(BuildCount))
                    compilerOptions.Append($"PHAPP_VERSION_BUILD=\"{BuildCount}\"");

                commandLine.Append("/m /nologo /nodereuse:false /verbosity:quiet ");
                commandLine.Append("/p:Platform=x64 /p:Configuration=" + (Flags.HasFlag(BuildFlags.BuildDebug) ? "Debug " : "Release "));
                commandLine.Append($"/p:ExternalCompilerOptions=\"{compilerOptions.ToString()}\" ");
                commandLine.Append(Solution);

                int errorcode = Win32.CreateProcess(
                    msbuildExePath,
                    commandLine.ToString(),
                    out string errorstring
                    );

                if (errorcode != 0)
                {
                    Program.PrintColorMessage($"[ERROR] ({errorcode}) {errorstring}", ConsoleColor.Red, true, Flags | BuildFlags.BuildVerbose);
                    return false;
                }
            }

            if (!HaveArm64BuildTools)
            {
                Program.PrintColorMessage(BuildTimeStamp(), ConsoleColor.DarkGray, false, Flags);
                Program.PrintColorMessage($"Building {Path.GetFileNameWithoutExtension(Solution)} (", ConsoleColor.Cyan, false, Flags);
                Program.PrintColorMessage("arm64", ConsoleColor.Green, false, Flags);
                Program.PrintColorMessage(")... ", ConsoleColor.Cyan, false, Flags);
                Program.PrintColorMessage("[SKIPPED] ARM64 build tools not installed.", ConsoleColor.Yellow, true, Flags);
                return true;
            }

            if (Flags.HasFlag(BuildFlags.BuildArm64bit))
            {
                StringBuilder compilerOptions = new StringBuilder(0x100);
                StringBuilder commandLine = new StringBuilder(0x100);

                Program.PrintColorMessage(BuildTimeStamp(), ConsoleColor.DarkGray, false, Flags);
                Program.PrintColorMessage($"Building {Path.GetFileNameWithoutExtension(Solution)} (", ConsoleColor.Cyan, false, Flags);
                Program.PrintColorMessage("arm64", ConsoleColor.Green, false, Flags);
                Program.PrintColorMessage(")...", ConsoleColor.Cyan, true, Flags);

                if (Flags.HasFlag(BuildFlags.BuildApi))
                    compilerOptions.Append("PH_BUILD_API;");
                if (Flags.HasFlag(BuildFlags.BuildMsix))
                    compilerOptions.Append("PH_BUILD_MSIX;");
                if (!string.IsNullOrWhiteSpace(BuildCommit))
                    compilerOptions.Append($"PHAPP_VERSION_COMMITHASH=\"{BuildCommit.AsSpan(0, 7)}\";");
                if (!string.IsNullOrWhiteSpace(BuildRevision))
                    compilerOptions.Append($"PHAPP_VERSION_REVISION=\"{BuildRevision}\";");
                if (!string.IsNullOrWhiteSpace(BuildCount))
                    compilerOptions.Append($"PHAPP_VERSION_BUILD=\"{BuildCount}\"");

                commandLine.Append("/m /nologo /nodereuse:false /verbosity:quiet ");
                commandLine.Append("/p:Platform=ARM64 /p:Configuration=" + (Flags.HasFlag(BuildFlags.BuildDebug) ? "Debug " : "Release "));
                commandLine.Append($"/p:ExternalCompilerOptions=\"{compilerOptions.ToString()}\" ");
                commandLine.Append(Solution);

                int errorcode = Win32.CreateProcess(
                    msbuildExePath,
                    commandLine.ToString(),
                    out string errorstring
                    );

                if (errorcode != 0)
                {
                    Program.PrintColorMessage($"[ERROR] ({errorcode}) {errorstring}", ConsoleColor.Red, true, Flags | BuildFlags.BuildVerbose);
                    return false;
                }
            }

            return true;
        }

        //public static bool BuildVersionInfo(BuildFlags Flags)
        //{
        //    if (!File.Exists("tools\\versioning\\version.rc"))
        //        return true;
        //
        //    StringBuilder commandLine = new StringBuilder();
        //    string windowsSdkPath = Utils.GetWindowsSdkPath();
        //    string windowsSdkIncludePath = Utils.GetWindowsSdkIncludePath();
        //    string resIncludePath = Path.GetFullPath("systeminformer");
        //    string rcExePath = windowsSdkPath + "\\x64\\rc.exe";
        //
        //    if (!File.Exists(rcExePath))
        //    {
        //        Program.PrintColorMessage("Unable to find the resource compiler.", ConsoleColor.Red);
        //        return true;
        //    }
        //
        //    commandLine.Append($"/nologo /v /i \"{windowsSdkIncludePath}\\um\" /i \"{windowsSdkIncludePath}\\shared\" /i \"{resIncludePath}\" ");
        //
        //    if (Flags.HasFlag(BuildFlags.BuildApi))
        //        commandLine.Append(" /d \"PH_BUILD_API\" ");
        //    if (!string.IsNullOrWhiteSpace(BuildCommit))
        //        commandLine.Append($"/d \"PHAPP_VERSION_COMMITHASH=\"{BuildCommit.AsSpan(0, 7)}\"\" ");
        //    if (!string.IsNullOrWhiteSpace(BuildRevision))
        //        commandLine.Append($"/d \"PHAPP_VERSION_REVISION=\"{BuildRevision}\"\" ");
        //    if (!string.IsNullOrWhiteSpace(BuildCount))
        //        commandLine.Append($"/d \"PHAPP_VERSION_BUILD=\"{BuildCount}\"\" ");
        //
        //    commandLine.Append("/fo tools\\versioning\\version.res tools\\versioning\\version.rc");
        //
        //    int errorcode = Win32.CreateProcess(
        //        rcExePath,
        //        commandLine.ToString(),
        //        out string errorstring
        //        );
        //
        //    if (errorcode != 0)
        //    {
        //        Program.PrintColorMessage($"[ERROR] ({errorcode}) {errorstring}", ConsoleColor.Red, true, Flags | BuildFlags.BuildVerbose);
        //        return false;
        //    }
        //
        //    return true;
        //}

        public static bool BuildDeployUpdateConfig()
        {
            string buildBuildId;
            string buildPostSfUrl;
            string buildPostSfApiKey;
            string buildFilename;
            string buildBinHash;
            string buildSetupHash;
            string BuildBinSig;
            string BuildSetupSig;
            long buildBinFileLength;
            long buildSetupFileLength;

            if (!BuildNightly)
                return true;

            buildBuildId = Win32.GetEnvironmentVariable("%APPVEYOR_BUILD_ID%");
            buildPostSfUrl = Win32.GetEnvironmentVariable("%APPVEYOR_BUILD_SF_API%");
            buildPostSfApiKey = Win32.GetEnvironmentVariable("%APPVEYOR_BUILD_SF_KEY%");
            buildBinFileLength = 0;
            buildSetupFileLength = 0;
            buildBinHash = null;
            buildSetupHash = null;
            BuildBinSig = null;
            BuildSetupSig = null;

            if (string.IsNullOrWhiteSpace(buildBuildId))
                return false;
            if (string.IsNullOrWhiteSpace(buildPostSfUrl))
                return false;
            if (string.IsNullOrWhiteSpace(buildPostSfApiKey))
                return false;
            if (string.IsNullOrWhiteSpace(BuildVersion))
                return false;

            Program.PrintColorMessage($"{Environment.NewLine}Uploading build artifacts... {BuildVersion}", ConsoleColor.Cyan);

            if (BuildNightly)
            {
                if (!File.Exists(Verify.GetPath("nightly.key")))
                {
                    string buildKey = Win32.GetEnvironmentVariable("%NIGHTLY_BUILD_KEY%");

                    if (!string.IsNullOrWhiteSpace(buildKey))
                    {
                        //Verify.Decrypt(Verify.GetPath("nightly.s"), Verify.GetPath("nightly.key"), buildKey);
                        Verify.DecryptLegacy(Verify.GetPath("nightly-legacy.s"), Verify.GetPath("nightly.key"), buildKey);
                    }

                    if (!File.Exists(Verify.GetPath("nightly.key")))
                    {
                        Program.PrintColorMessage("[SKIPPED] nightly.key not found.", ConsoleColor.Yellow);
                        return true;
                    }
                }
            }

            string customSignToolPath = Verify.GetCustomSignToolFilePath();

            if (File.Exists(customSignToolPath))
            {
                BuildBinSig = Win32.ShellExecute(
                    customSignToolPath,
                    $"sign -k {Verify.GetPath("nightly.key")} {BuildOutputFolder}\\systeminformer-build-bin.zip -h"
                    );
                BuildSetupSig = Win32.ShellExecute(
                    customSignToolPath,
                    $"sign -k {Verify.GetPath("nightly.key")} {BuildOutputFolder}\\systeminformer-build-setup.exe -h"
                    );
            }
            else
            {
                Program.PrintColorMessage("[SKIPPED] CustomSignTool not found.", ConsoleColor.Yellow);
                return true;
            }

            if (BuildNightly)
            {
                Win32.DeleteFile(Verify.GetPath("nightly.key"));
            }

            if (string.IsNullOrWhiteSpace(BuildBinSig))
            {
                Program.PrintColorMessage("build-bin.sig not found.", ConsoleColor.Red);
                return false;
            }
            if (string.IsNullOrWhiteSpace(BuildSetupSig))
            {
                Program.PrintColorMessage("build-setup.sig not found.", ConsoleColor.Red);
                return false;
            }

            buildFilename = $"{BuildOutputFolder}\\systeminformer-build-bin.zip";

            if (File.Exists(buildFilename))
            {
                buildBinFileLength = new FileInfo(buildFilename).Length;
                buildBinHash = Verify.HashFile(buildFilename);
            }

            if (string.IsNullOrWhiteSpace(buildBinHash))
            {
                Program.PrintColorMessage("build-bin hash not found.", ConsoleColor.Red);
                return false;
            }

            buildFilename = $"{BuildOutputFolder}\\systeminformer-build-setup.exe";

            if (File.Exists(buildFilename))
            {
                buildSetupFileLength = new FileInfo(buildFilename).Length;
                buildSetupHash = Verify.HashFile(buildFilename);
            }

            if (string.IsNullOrWhiteSpace(buildSetupHash))
            {
                Program.PrintColorMessage("build-setup hash not found.", ConsoleColor.Red);
                return false;
            }

            GithubRelease githubMirrorUpload = BuildDeployUploadGithubConfig();

            if (githubMirrorUpload == null)
                return false;

            string binziplink = githubMirrorUpload.GetFileUrl($"systeminformer-{BuildVersion}-bin.zip");
            string setupexelink = githubMirrorUpload.GetFileUrl($"systeminformer-{BuildVersion}-setup.exe");

            if (string.IsNullOrWhiteSpace(binziplink) || string.IsNullOrWhiteSpace(setupexelink))
            {
                Program.PrintColorMessage("build-github downloads not found.", ConsoleColor.Red);
                return false;
            }

            try
            {
                BuildUpdateRequest buildUpdateRequest = new BuildUpdateRequest
                {
                    BuildUpdated = TimeStart.ToString("o"),
                    BuildVersion = BuildVersion,
                    BuildCommit = BuildCommit,
                    BuildId = buildBuildId,
                    BinUrl = binziplink, // $"https://ci.appveyor.com/api/buildjobs/{buildJobId}/artifacts/systeminformer-{BuildVersion}-bin.zip",
                    BinLength = buildBinFileLength.ToString(),
                    BinHash = buildBinHash,
                    BinSig = BuildBinSig,
                    SetupUrl = setupexelink, // $"https://ci.appveyor.com/api/buildjobs/{buildJobId}/artifacts/systeminformer-{BuildVersion}-setup.exe",
                    SetupLength = buildSetupFileLength.ToString(),
                    SetupHash = buildSetupHash,
                    SetupSig = BuildSetupSig,
                };

                byte[] buildPostString = JsonSerializer.SerializeToUtf8Bytes(buildUpdateRequest, BuildUpdateRequestContext.Default.BuildUpdateRequest);

                if (buildPostString == null || buildPostString.LongLength == 0)
                    return false;

                using (HttpClientHandler httpClientHandler = new HttpClientHandler())
                {
                    httpClientHandler.AutomaticDecompression = DecompressionMethods.All;

                    using (HttpClient httpClient = new HttpClient(httpClientHandler))
                    {
                        httpClient.DefaultRequestHeaders.Add("X-ApiKey", buildPostSfApiKey);
                        httpClient.DefaultVersionPolicy = HttpVersionPolicy.RequestVersionOrHigher;
                        httpClient.DefaultRequestVersion = HttpVersion.Version20;

                        using (ByteArrayContent httpContent = new ByteArrayContent(buildPostString))
                        {
                            httpContent.Headers.ContentType = new MediaTypeHeaderValue("application/json");

                            var httpTask = httpClient.PostAsync(buildPostSfUrl, httpContent);
                            httpTask.Wait();

                            if (!httpTask.Result.IsSuccessStatusCode)
                            {
                                Program.PrintColorMessage($"[UpdateBuildWebService-SF] {httpTask.Result}", ConsoleColor.Red);
                                return false;
                            }
                        }
                    }
                }
            }
            catch (Exception ex)
            {
                Program.PrintColorMessage($"[UpdateBuildWebService-SF] {ex}", ConsoleColor.Red);
                return false;
            }

            if (!AppVeyor.UpdateBuildVersion(BuildVersion)) // Update Appveyor build version string.
            {
                return false;
            }

            return true;
        }

        //public static bool BuildDeployUploadArtifacts()
        //{
        //    string buildPostUrl;
        //    string buildPostKey;
        //    string buildPostName;
        //
        //    if (!BuildNightly)
        //        return true;
        //
        //    buildPostUrl = Win32.GetEnvironmentVariable("%APPVEYOR_NIGHTLY_URL%");
        //    buildPostKey = Win32.GetEnvironmentVariable("%APPVEYOR_NIGHTLY_KEY%");
        //    buildPostName = Win32.GetEnvironmentVariable("%APPVEYOR_NIGHTLY_NAME%");
        //
        //    if (string.IsNullOrWhiteSpace(buildPostUrl))
        //        return false;
        //    if (string.IsNullOrWhiteSpace(buildPostKey))
        //        return false;
        //    if (string.IsNullOrWhiteSpace(buildPostName))
        //        return false;
        //
        //    Program.PrintColorMessage(string.Empty, ConsoleColor.Black);
        //
        //    try
        //    {
        //        foreach (BuildFile file in BuildConfig.Build_Release_Files)
        //        {
        //            string sourceFile = BuildOutputFolder + file.FileName;
        //
        //            if (File.Exists(sourceFile))
        //            {
        //                string filename;
        //                FtpWebRequest request;
        //
        //                filename = Path.GetFileName(sourceFile);
        //                //filename = filename.Replace("-build-", $"-{BuildVersion}-", StringComparison.OrdinalIgnoreCase);
        //
        //                #pragma warning disable SYSLIB0014 // Type or member is obsolete
        //                request = (FtpWebRequest)WebRequest.Create(buildPostUrl + filename);
        //                #pragma warning restore SYSLIB0014 // Type or member is obsolete
        //                request.Credentials = new NetworkCredential(buildPostKey, buildPostName);
        //                request.Method = WebRequestMethods.Ftp.UploadFile;
        //                request.Timeout = System.Threading.Timeout.Infinite;
        //                request.EnableSsl = true;
        //                request.UsePassive = true;
        //                request.UseBinary = true;
        //
        //                Program.PrintColorMessage($"Uploading {filename}...", ConsoleColor.Cyan, true);
        //
        //                using (FileStream fileStream = File.OpenRead(sourceFile))
        //                using (BufferedStream localStream = new BufferedStream(fileStream))
        //                using (BufferedStream remoteStream = new BufferedStream(request.GetRequestStream()))
        //                {
        //                    localStream.CopyTo(remoteStream);
        //                }
        //
        //                using (FtpWebResponse response = (FtpWebResponse)request.GetResponse())
        //                {
        //                    if (response.StatusCode != FtpStatusCode.CommandOK && response.StatusCode != FtpStatusCode.ClosingData)
        //                    {
        //                        Program.PrintColorMessage($"[HttpWebResponse] {response.StatusDescription}", ConsoleColor.Red);
        //                        return false;
        //                    }
        //                }
        //            }
        //        }
        //    }
        //    catch (Exception ex)
        //    {
        //        Program.PrintColorMessage($"[UploadBuildWebServiceAsync-Exception] {ex.Message}", ConsoleColor.Red);
        //        return false;
        //    }
        //
        //    //try
        //    //{
        //    //    foreach (BuildFile file in BuildConfig.Build_Release_Files)
        //    //    {
        //    //        if (!file.UploadNightly)
        //    //            continue;
        //    //
        //    //        string sourceFile = BuildOutputFolder + file.FileName;
        //    //
        //    //        if (File.Exists(sourceFile))
        //    //        {
        //    //            bool uploaded;
        //    //            string fileName;
        //    //
        //    //            fileName = sourceFile.Replace("-build-", $"-{BuildVersion}-", StringComparison.OrdinalIgnoreCase);
        //    //
        //    //            File.Move(sourceFile, fileName, true);
        //    //            uploaded = AppVeyor.UploadFile(fileName);
        //    //            File.Move(fileName, sourceFile, true);
        //    //
        //    //            if (!uploaded)
        //    //            {
        //    //                Program.PrintColorMessage("[WebServiceAppveyorUploadFile]", ConsoleColor.Red);
        //    //                return false;
        //    //            }
        //    //        }
        //    //        else
        //    //        {
        //    //            Program.PrintColorMessage("[SKIPPED] missing file: " + sourceFile, ConsoleColor.Yellow);
        //    //        }
        //    //    }
        //    //}
        //    //catch (Exception ex)
        //    //{
        //    //    Program.PrintColorMessage("[WebServiceAppveyorPushArtifact] " + ex, ConsoleColor.Red);
        //    //    return false;
        //    //}
        //
        //    return true;
        //}

        public static GithubRelease BuildDeployUploadGithubConfig()
        {
            if (!Github.DeleteRelease(BuildVersion))
                return null;

            var mirror = new GithubRelease();

            try
            {
                // Create a new github release.

                var response = Github.CreateRelease(BuildVersion);

                if (response == null)
                {
                    Program.PrintColorMessage("[Github.CreateRelease]", ConsoleColor.Red);
                    return null;
                }

                // Upload the files.

                foreach (BuildFile file in BuildConfig.Build_Release_Files)
                {
                    if (!file.UploadNightly)
                        continue;

                    string sourceFile = BuildOutputFolder + file.FileName;

                    if (File.Exists(sourceFile))
                    {
                        var result = Github.UploadAssets(BuildVersion, sourceFile, response.UploadUrl);

                        if (result == null)
                        {
                            Program.PrintColorMessage("[Github.UploadAssets]", ConsoleColor.Red);
                            return null;
                        }

                        //mirror.Files.Add(new GithubReleaseAsset(file.FileName, result.Download_url));
                    }
                }

                // Update the release and make it public.

                var update = Github.UpdateRelease(response.ReleaseId);

                if (update == null)
                {
                    Program.PrintColorMessage("[Github.UpdateRelease]", ConsoleColor.Red);
                    return null;
                }

                // Grab the download urls.

                foreach (var file in update.Assets)
                {
                    if (!file.Uploaded)
                    {
                        continue;
                    }

                    mirror.Files.Add(new GithubReleaseAsset(file.Name, file.DownloadUrl));
                }

                mirror.ReleaseId = update.ReleaseId;
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

    }
}
