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
        public static bool BuildNightly = false;
        public static bool HaveArm64BuildTools = true;
        public static string OutputDirectory = string.Empty;
        public static string CurrentDirectory = string.Empty;
        public static string BuildBranch = string.Empty;
        public static string BuildCommit = string.Empty;
        public static string BuildVersion = "1.0.0";
        public static string BuildLongVersion = "1.0.0.0";
        public static string BuildMajorVersion = "3.0";
        public static string BuildCount = string.Empty;
        public static string BuildRevision = string.Empty;

        public static bool InitializeBuildEnvironment()
        {
            Console.InputEncoding = Utils.UTF8NoBOM;
            Console.OutputEncoding = Utils.UTF8NoBOM;
            Win32.SetErrorMode();

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
            Build.CurrentDirectory = Environment.CurrentDirectory;
            Build.OutputDirectory = $"{Build.CurrentDirectory}\\build\\output";
            Build.BuildNightly = Win32.HasEnvironmentVariable("BUILD_SOURCESDIRECTORY");

            return true;
        }

        public static void SetupBuildEnvironment(bool ShowBuildInfo)
        {
            //Utils.ExecuteGitCommand(Build.CurrentDirectory, "fetch --unshallow");
            Build.BuildBranch = Utils.ExecuteGitCommand(Build.CurrentDirectory, "rev-parse --abbrev-ref HEAD");
            Build.BuildCommit = Utils.ExecuteGitCommand(Build.CurrentDirectory, "rev-parse HEAD");

            if (!string.IsNullOrWhiteSpace(Build.BuildBranch))
            {
                Build.BuildCount = Utils.ExecuteGitCommand(Build.CurrentDirectory, $"rev-list --count {Build.BuildBranch}");
                string currentGitTag = Utils.ExecuteGitCommand(Build.CurrentDirectory, "describe --abbrev=0 --tags --always");

                if (!string.IsNullOrWhiteSpace(Build.BuildCount) && !string.IsNullOrWhiteSpace(currentGitTag))
                {
                    Build.BuildRevision = Utils.ExecuteGitCommand(Build.CurrentDirectory, $"rev-list --count \"{currentGitTag}..{Build.BuildBranch}\"");
                    Build.BuildVersion = $"{Build.BuildMajorVersion}.{Build.BuildRevision}";
                    Build.BuildLongVersion = $"{Build.BuildMajorVersion}.{Build.BuildCount}.{Build.BuildRevision}";
                }
            }

            if (
                string.IsNullOrWhiteSpace(Build.BuildBranch) ||
                string.IsNullOrWhiteSpace(Build.BuildCommit) ||
                string.IsNullOrWhiteSpace(Build.BuildCount) ||
                string.IsNullOrWhiteSpace(Build.BuildRevision)
                )
            {
                Build.BuildBranch = string.Empty;
                Build.BuildCommit = string.Empty;
                Build.BuildCount = string.Empty;
                Build.BuildRevision = string.Empty;
                Build.BuildVersion = "1.0.0";
                Build.BuildLongVersion = "1.0.0.0";
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
                    Program.PrintColorMessage("VisualStudio: ", ConsoleColor.DarkGray, false);
                    Program.PrintColorMessage(instance.Name, ConsoleColor.Green, true);
                    Build.HaveArm64BuildTools = instance.HasARM64BuildToolsComponents;
                }

                Program.PrintColorMessage(Environment.NewLine + "Building: ", ConsoleColor.DarkGray, false);
                Program.PrintColorMessage(Build.BuildLongVersion, ConsoleColor.Green, false);

                if (!string.IsNullOrWhiteSpace(Build.BuildCommit))
                {
                    Program.PrintColorMessage(" (", ConsoleColor.DarkGray, false);
                    Program.PrintColorMessage(Build.BuildCommit.Substring(0, 7), ConsoleColor.DarkYellow, false);
                    Program.PrintColorMessage(")", ConsoleColor.DarkGray, false);
                }

                if (!string.IsNullOrWhiteSpace(Build.BuildBranch))
                {
                    Program.PrintColorMessage(" [", ConsoleColor.DarkGray, false);
                    Program.PrintColorMessage(Build.BuildBranch, ConsoleColor.DarkBlue, false);
                    Program.PrintColorMessage("]", ConsoleColor.DarkGray, false);
                }

                Program.PrintColorMessage(Environment.NewLine, ConsoleColor.DarkGray, true);
            }
        }

        public static void CleanupBuildEnvironment()
        {
            try
            {
                if (!string.IsNullOrWhiteSpace(Build.CurrentDirectory))
                {
                    string output = Utils.ExecuteGitCommand(Build.CurrentDirectory, "clean -x -d -f");

                    Program.PrintColorMessage(output, ConsoleColor.DarkGray);
                }

                {
                    if (Directory.Exists(Build.OutputDirectory)) // output
                    {
                        Program.PrintColorMessage($"Deleting: {Build.OutputDirectory}", ConsoleColor.DarkGray);
                        Directory.Delete(Build.OutputDirectory, true);
                    }

                    if (Directory.Exists(BuildConfig.Build_Sdk_Directories[0])) // sdk
                    {
                        Program.PrintColorMessage($"Deleting: {BuildConfig.Build_Sdk_Directories[0]}", ConsoleColor.DarkGray);
                        Directory.Delete(BuildConfig.Build_Sdk_Directories[0], true);
                    }

                    //foreach (BuildUploadFile file in BuildConfig.Build_Release_Files)
                    //{
                    //    string sourceFile = OutputDirectory + file.FileName;
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

                    var resource_files = Directory.EnumerateFiles(".", "*.aps", new EnumerationOptions
                    {
                        AttributesToSkip = FileAttributes.Offline,
                        RecurseSubdirectories = true,
                        ReturnSpecialDirectories = false
                    });

                    foreach (string file in resource_files)
                    {
                        string path = Path.GetFullPath(file);
                        string name = Path.GetFileName(path);

                        if (name.EndsWith(".aps", StringComparison.OrdinalIgnoreCase))
                        {
                            if (File.Exists(path))
                            {
                                Program.PrintColorMessage($"Deleting: {path}", ConsoleColor.DarkGray);

                                Win32.DeleteFile(path);
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
            Program.PrintColorMessage($" second(s) {Environment.NewLine} {Environment.NewLine}", ConsoleColor.DarkGray, false);
        }

        public static bool CopyTextFiles(bool Update)
        {
            string[] Build_Text_Files =
            {
                "README.txt",
                "COPYRIGHT.txt",
                "LICENSE.txt",
            };

            foreach (string file in Build_Text_Files)
            {
                if (Update)
                    Win32.CopyIfNewer(file, $"bin\\{file}");
                else
                    Win32.DeleteFile($"bin\\{file}");
            }

            return true;
        }

        public static bool CopyWow64Files(BuildFlags Flags)
        {
            string[] Build_Wow64_Files =
            {
                "SystemInformer.exe",
                "SystemInformer.pdb",
                "SystemInformer.sig",
                "plugins\\DotNetTools.dll",
                "plugins\\DotNetTools.pdb",
                "plugins\\DotNetTools.sig",
                "plugins\\ExtendedTools.dll",
                "plugins\\ExtendedTools.pdb",
                "plugins\\ExtendedTools.sig",
            };

            if (Flags.HasFlag(BuildFlags.Build32bit))
                return true;

            foreach (string file in Build_Wow64_Files)
            {
                if (Flags.HasFlag(BuildFlags.BuildDebug))
                {
                    if (Flags.HasFlag(BuildFlags.Build64bit))
                    {
                        if (Directory.Exists("bin\\Debug64"))
                        {
                            Win32.CopyIfNewer($"bin\\Debug32\\{file}", $"bin\\Debug64\\x86\\{file}");
                        }
                    }

                    if (Flags.HasFlag(BuildFlags.BuildARM64))
                    {
                        if (Directory.Exists("bin\\DebugARM64"))
                        {
                            Win32.CopyIfNewer($"bin\\Debug32\\{file}", $"bin\\DebugARM64\\x86\\{file}");
                        }
                    }
                }

                if (Flags.HasFlag(BuildFlags.BuildRelease))
                {
                    if (Flags.HasFlag(BuildFlags.Build64bit))
                    {
                        if (Directory.Exists("bin\\Release64"))
                        {
                            Win32.CopyIfNewer($"bin\\Release32\\{file}", $"bin\\Release64\\x86\\{file}");
                        }
                    }

                    if (Flags.HasFlag(BuildFlags.BuildARM64))
                    {
                        if (Directory.Exists("bin\\ReleaseARM64"))
                        {
                            Win32.CopyIfNewer($"bin\\Release32\\{file}", $"bin\\ReleaseARM64\\x86\\{file}");
                        }
                    }
                }
            }

            return true;
        }

        public static bool CopyResourceFiles(BuildFlags Flags)
        {
            string[] Build_Resource_Files =
            {
                "SystemInformer.png",
                "capslist.txt",
                "etwguids.txt",
                "pooltag.txt",
            };

            foreach (string file in Build_Resource_Files)
            {
                if (Flags.HasFlag(BuildFlags.BuildDebug))
                {
                    if (Flags.HasFlag(BuildFlags.Build32bit))
                    {
                        if (Directory.Exists("bin\\Debug32"))
                        {
                            Win32.CopyIfNewer($"SystemInformer\\resources\\{file}", $"bin\\Debug32\\{file}");
                        }
                    }

                    if (Flags.HasFlag(BuildFlags.Build64bit))
                    {
                        if (Directory.Exists("bin\\Debug64"))
                        {
                            Win32.CopyIfNewer($"SystemInformer\\resources\\{file}", $"bin\\Debug64\\{file}");
                        }
                    }

                    if (Flags.HasFlag(BuildFlags.BuildARM64))
                    {
                        if (Directory.Exists("bin\\DebugARM64"))
                        {
                            Win32.CopyIfNewer($"SystemInformer\\resources\\{file}", $"bin\\DebugARM64\\{file}");
                        }
                    }
                }

                if (Flags.HasFlag(BuildFlags.BuildRelease))
                {
                    if (Flags.HasFlag(BuildFlags.Build32bit))
                    {
                        if (Directory.Exists("bin\\Release32"))
                        {
                            Win32.CopyIfNewer($"SystemInformer\\resources\\{file}", $"bin\\Release32\\{file}");
                        }
                    }

                    if (Flags.HasFlag(BuildFlags.Build64bit))
                    {
                        if (Directory.Exists("bin\\Release64"))
                        {
                            Win32.CopyIfNewer($"SystemInformer\\resources\\{file}", $"bin\\Release64\\{file}");
                        }
                    }

                    if (Flags.HasFlag(BuildFlags.BuildARM64))
                    {
                        if (Directory.Exists("bin\\ReleaseARM64"))
                        {
                            Win32.CopyIfNewer($"SystemInformer\\resources\\{file}", $"bin\\ReleaseARM64\\{file}");
                        }
                    }
                }
            }

            return true;
        }

        public static bool BuildPublicHeaderFiles()
        {
            Program.PrintColorMessage(Build.BuildTimeStamp(), ConsoleColor.DarkGray, false);
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
            //Program.PrintColorMessage(Build.BuildTimeStamp(), ConsoleColor.DarkGray, false);
            //Program.PrintColorMessage("Building dynamic headers...", ConsoleColor.Cyan);

            try
            {
                if (!DynData.Execute())
                {
                    Program.PrintColorMessage($"[ERROR] Dynamic configuration is invalid.", ConsoleColor.Red);
                    return false;
                }
            }
            catch (Exception ex)
            {
                Program.PrintColorMessage($"[ERROR] {ex}", ConsoleColor.Red);
                return false;
            }

            return true;
        }

        public static bool CopyKernelDriver(BuildFlags Flags)
        {
            string[] Build_Driver_Files =
            {
                "SystemInformer.sys",
                "ksi.dll",
            };

            foreach (string file in Build_Driver_Files)
            {
                if (Flags.HasFlag(BuildFlags.BuildDebug))
                {
                    if (Flags.HasFlag(BuildFlags.Build32bit))
                    {
                        if (Directory.Exists("bin\\Debug32"))
                        {
                            Win32.CopyIfNewer($"KSystemInformer\\bin-signed\\i386\\{file}", $"bin\\Debug32\\{file}");
                        }
                    }

                    if (Flags.HasFlag(BuildFlags.Build64bit))
                    {
                        if (Directory.Exists("bin\\Debug64"))
                        {
                            Win32.CopyIfNewer($"KSystemInformer\\bin-signed\\amd64\\{file}", $"bin\\Debug64\\{file}");
                        }
                    }

                    if (Flags.HasFlag(BuildFlags.BuildARM64))
                    {
                        if (Directory.Exists("bin\\DebugARM64"))
                        {
                            Win32.CopyIfNewer($"KSystemInformer\\bin-signed\\arm64\\{file}", $"bin\\DebugARM64\\{file}");
                        }
                    }
                }

                if (Flags.HasFlag(BuildFlags.BuildRelease))
                {
                    if (Flags.HasFlag(BuildFlags.Build32bit))
                    {
                        if (Directory.Exists("bin\\Release32"))
                        {
                            Win32.CopyIfNewer($"KSystemInformer\\bin-signed\\i386\\{file}", $"bin\\Release32\\{file}");
                        }
                    }

                    if (Flags.HasFlag(BuildFlags.Build64bit))
                    {
                        if (Directory.Exists("bin\\Release64"))
                        {
                            Win32.CopyIfNewer($"KSystemInformer\\bin-signed\\amd64\\{file}", $"bin\\Release64\\{file}");
                        }
                    }

                    if (Flags.HasFlag(BuildFlags.BuildARM64))
                    {
                        if (Directory.Exists("bin\\ReleaseARM64"))
                        {
                            Win32.CopyIfNewer($"KSystemInformer\\bin-signed\\arm64\\{file}", $"bin\\ReleaseARM64\\{file}");
                        }
                    }
                }
            }

            return true;
        }

        public static bool CreateSignatureFiles(BuildFlags Flags)
        {
            string[] Build_Sign_Files =
            {
                "SystemInformer.exe",
                "peview.exe",
                "plugins\\DotNetTools.dll",
                "plugins\\ExtendedNotifications.dll",
                "plugins\\ExtendedServices.dll",
                "plugins\\ExtendedTools.dll",
                "plugins\\HardwareDevices.dll",
                "plugins\\NetworkTools.dll",
                "plugins\\OnlineChecks.dll",
                "plugins\\ToolStatus.dll",
                "plugins\\Updater.dll",
                "plugins\\UserNotes.dll",
                "plugins\\WindowExplorer.dll",
            };

            List<string> files = new List<string>();

            foreach (string file in Build_Sign_Files)
            {
                if (Flags.HasFlag(BuildFlags.BuildDebug))
                {
                    if (Flags.HasFlag(BuildFlags.Build32bit))
                    {
                        files.Add($"bin\\Debug32\\{file}");
                    }

                    if (Flags.HasFlag(BuildFlags.Build64bit))
                    {
                        files.Add($"bin\\Debug64\\{file}");
                    }

                    if (Flags.HasFlag(BuildFlags.BuildARM64))
                    {
                        files.Add($"bin\\DebugARM64\\{file}");
                    }
                }

                if (Flags.HasFlag(BuildFlags.BuildRelease))
                {
                    if (Flags.HasFlag(BuildFlags.Build32bit))
                    {
                        files.Add($"bin\\Release32\\{file}");
                    }

                    if (Flags.HasFlag(BuildFlags.Build64bit))
                    {
                        files.Add($"bin\\Release64\\{file}");
                    }

                    if (Flags.HasFlag(BuildFlags.BuildARM64))
                    {
                        files.Add($"bin\\ReleaseARM64\\{file}");
                    }
                }
            }

            if (Build.BuildNightly && !VerifyCache.Contains(VerifyCache.KMODE_CI))
            {
                if (!Verify.DecryptEnvironment(VerifyCache.KMODE_CI))
                {
                    Program.PrintColorMessage("[ERROR] DecryptEnvironment", ConsoleColor.Red);
                    return false;
                }
            }

            if (VerifyCache.Contains(VerifyCache.KMODE_CI))
            {
                foreach (string file in files)
                {
                    if (File.Exists(file))
                    {
                        if (!Verify.CreateSignatureFile(VerifyCache.KMODE_CI, $"{Build.CurrentDirectory}\\{file}", Build.BuildNightly))
                            return false;
                    }
                }
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

            if (Build.BuildNightly && !VerifyCache.Contains(VerifyCache.KMODE_CI))
            {
                if (!Verify.DecryptEnvironment(VerifyCache.KMODE_CI))
                {
                    Program.PrintColorMessage("[ERROR] SignPlugin.DecryptEnvironment", ConsoleColor.Red);
                    return false;
                }
            }

            if (VerifyCache.Contains(VerifyCache.KMODE_CI))
                return Verify.CreateSignatureFile(VerifyCache.KMODE_CI, PluginName);
            return true;
        }

        //public static bool ResignFiles()
        //{
        //    var files = new List<string>();
        //
        //    foreach (string sigFile in Directory.EnumerateFiles("bin", "*.sig", SearchOption.AllDirectories))
        //    {
        //        var file = Path.ChangeExtension(sigFile, ".dll");
        //
        //        if (File.Exists(file))
        //            files.Add(file);
        //
        //        file = Path.ChangeExtension(sigFile, ".exe");
        //
        //        if (File.Exists(file))
        //            files.Add(file);
        //    }
        //
        //    if (Build.BuildNightly && !VerifyCache.Contains(VerifyCache.KMODE_CI))
        //    {
        //        if (!Verify.DecryptEnvironment(VerifyCache.KMODE_CI))
        //        {
        //            Program.PrintColorMessage("[ERROR] DecryptEnvironment", ConsoleColor.Red);
        //            return false;
        //        }
        //    }
        //
        //    if (VerifyCache.Contains(VerifyCache.KMODE_CI))
        //    {
        //        foreach (string file in files)
        //        {
        //            if (File.Exists(file))
        //            {
        //                if (!Verify.CreateSignatureFile(VerifyCache.KMODE_CI, $"{Build.CurrentDirectory}\\{file}", Build.BuildNightly))
        //                    return false;
        //            }
        //        }
        //    }
        //
        //    return true;
        //}

        public static bool BuildAppResourceHeader()
        {
            try
            {
                // Read the default SystemInformer resource header.
                string phappContent = File.ReadAllText("SystemInformer\\resource.h");

                if (string.IsNullOrWhiteSpace(phappContent))
                    return false;

                // Rename strings from "ID_ABC" to "PHAPP_ID_ABC".
                string phsdkContent = phappContent.Replace("#define ID", "#define PHAPP_ID", StringComparison.OrdinalIgnoreCase);

                if (string.IsNullOrWhiteSpace(phsdkContent))
                    return false;

                // Write the updated header to the plugin SDK directory.
                File.WriteAllText("sdk\\include\\phappresource.h", phsdkContent);
            }
            catch (Exception ex)
            {
                Program.PrintColorMessage($"[ERROR] {ex}", ConsoleColor.Red);
                return false;
            }

            return true;
        }

        public static bool BuildSdk(BuildFlags Flags)
        {
            string[] Build_Sdk_Header_Files =
            {
                "phapppub.h",
                "phdk.h",
            };
            string[] Build_Sdk_Static_Files =
            {
                "SystemInformer.lib"
            };

            try
            {
                foreach (string folder in BuildConfig.Build_Sdk_Directories)
                {
                    Win32.CreateDirectory(folder);
                }

                // Build the SDK.
                HeaderGen.Execute();

                // Build the AppResource.h header.
                BuildAppResourceHeader();

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

                // Note: Debug overwrites the Release (local builds only) since they're copied to same directory. (dmex)
                foreach (string file in Build_Sdk_Static_Files)
                {
                    // Copy the release libs
                    if (Flags.HasFlag(BuildFlags.BuildRelease))
                    {
                        if (Flags.HasFlag(BuildFlags.Build32bit))
                            Win32.CopyIfNewer($"bin\\Release32\\{file}", $"sdk\\lib\\i386\\{file}");
                        if (Flags.HasFlag(BuildFlags.Build64bit))
                            Win32.CopyIfNewer($"bin\\Release64\\{file}", $"sdk\\lib\\amd64\\{file}");
                        if (Flags.HasFlag(BuildFlags.BuildARM64))
                            Win32.CopyIfNewer($"bin\\ReleaseARM64\\{file}", $"sdk\\lib\\arm64\\{file}");
                    }

                    // Copy the debug libs
                    if (Flags.HasFlag(BuildFlags.BuildDebug))
                    {
                        if (Flags.HasFlag(BuildFlags.Build32bit))
                            Win32.CopyIfNewer($"bin\\Debug32\\{file}", $"sdk\\lib\\i386\\{file}");
                        if (Flags.HasFlag(BuildFlags.Build64bit))
                            Win32.CopyIfNewer($"bin\\Debug64\\{file}", $"sdk\\lib\\amd64\\{file}");
                        if (Flags.HasFlag(BuildFlags.BuildARM64))
                            Win32.CopyIfNewer($"bin\\DebugARM64\\{file}", $"sdk\\lib\\arm64\\{file}");
                    }
                }

                // Copy SDK headers.
                foreach (string file in BuildConfig.Build_Native_Headers)
                    Win32.CopyIfNewer($"phnt\\include\\{file}", $"sdk\\include\\{file}");
                foreach (string file in BuildConfig.Build_App_Headers)
                    Win32.CopyIfNewer($"phlib\\include\\{file}", $"sdk\\include\\{file}");
                foreach (string file in BuildConfig.Build_Ksilib_Headers)
                    Win32.CopyIfNewer($"kphlib\\include\\{file}", $"sdk\\include\\{file}");

                // Copy remaining headers.
                foreach (string file in Build_Sdk_Header_Files)
                {
                    Win32.CopyIfNewer($"SystemInformer\\sdk\\{file}", $"sdk\\include\\{file}");
                    Win32.CopyIfNewer($"SystemInformer\\sdk\\{file}", $"sdk\\include\\{file}");
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
            Program.PrintColorMessage(Build.BuildTimeStamp(), ConsoleColor.DarkGray, false);
            Program.PrintColorMessage("Building build-setup.exe... ", ConsoleColor.Cyan, false);

            if (!BuildSolution("tools\\CustomSetupTool\\CustomSetupTool.sln", BuildFlags.Build32bit | BuildFlags.BuildApi))
                return false;

            try
            {
                Win32.CreateDirectory(Build.OutputDirectory);

                Win32.DeleteFile(
                    $"{Build.OutputDirectory}\\systeminformer-build-setup.exe"
                    );

                File.Move(
                    "tools\\CustomSetupTool\\bin\\Release32\\CustomSetupTool.exe",
                    $"{Build.OutputDirectory}\\systeminformer-build-setup.exe"
                    );

                Program.PrintColorMessage(
                    Win32.GetFileSize($"{Build.OutputDirectory}\\systeminformer-build-setup.exe").ToPrettySize(),
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

        public static bool BuildSdkZip()
        {
            Program.PrintColorMessage(Build.BuildTimeStamp(), ConsoleColor.DarkGray, false);
            Program.PrintColorMessage("Building build-sdk.zip... ", ConsoleColor.Cyan, false);

            try
            {
                Win32.CreateDirectory(Build.OutputDirectory);

                Win32.DeleteFile(
                    $"{Build.OutputDirectory}\\systeminformer-build-sdk.zip"
                    );

                Zip.CreateCompressedSdkFromFolder(
                    "sdk",
                    $"{Build.OutputDirectory}\\systeminformer-build-sdk.zip"
                    );

                Program.PrintColorMessage(Win32.GetFileSize($"{Build.OutputDirectory}\\systeminformer-build-sdk.zip").ToPrettySize(), ConsoleColor.Green);
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
            Program.PrintColorMessage(Build.BuildTimeStamp(), ConsoleColor.DarkGray, false);
            Program.PrintColorMessage("Building build-bin.zip... ", ConsoleColor.Cyan, false);

            try
            {
                if (Directory.Exists("bin\\Release32"))
                    Win32.CreateEmptyFile("bin\\Release32\\SystemInformer.exe.settings.xml");
                if (Directory.Exists("bin\\Release64"))
                    Win32.CreateEmptyFile("bin\\Release64\\SystemInformer.exe.settings.xml");
                if (Directory.Exists("bin\\ReleaseARM64"))
                    Win32.CreateEmptyFile("bin\\ReleaseARM64\\SystemInformer.exe.settings.xml");
            }
            catch (Exception ex)
            {
                Program.PrintColorMessage($"[WARN] {ex}", ConsoleColor.Yellow);
            }

            try
            {
                Win32.CreateDirectory(Build.OutputDirectory);

                Win32.DeleteFile(
                    $"{Build.OutputDirectory}\\systeminformer-build-bin.zip"
                    );

                Zip.CreateCompressedFolder(
                    "bin",
                    $"{Build.OutputDirectory}\\systeminformer-build-bin.zip"
                    );

                Program.PrintColorMessage(Win32.GetFileSize($"{Build.OutputDirectory}\\systeminformer-build-bin.zip").ToPrettySize(), ConsoleColor.Green);
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
            Program.PrintColorMessage(Build.BuildTimeStamp(), ConsoleColor.DarkGray, false);
            Program.PrintColorMessage(MsixPackageBuild ? "Building setup-package-pdb..." : "Building build-pdb.zip...", ConsoleColor.Cyan, false);

            try
            {
                Win32.CreateDirectory(Build.OutputDirectory);

                if (MsixPackageBuild)
                {
                    Win32.DeleteFile(
                        $"{Build.OutputDirectory}\\systeminformer-setup-package-pdb.zip"
                        );

                    Zip.CreateCompressedPdbFromFolder(
                        ".\\",
                        $"{Build.OutputDirectory}\\systeminformer-setup-package-pdb.zip"
                        );

                    Program.PrintColorMessage(Win32.GetFileSize($"{Build.OutputDirectory}\\systeminformer-setup-package-pdb.zip").ToPrettySize(), ConsoleColor.Green);
                }
                else
                {
                    Win32.DeleteFile(
                        $"{Build.OutputDirectory}\\systeminformer-build-pdb.zip"
                        );

                    Zip.CreateCompressedPdbFromFolder(
                        ".\\",
                        $"{Build.OutputDirectory}\\systeminformer-build-pdb.zip"
                        );

                    Program.PrintColorMessage(Win32.GetFileSize($"{Build.OutputDirectory}\\systeminformer-build-pdb.zip").ToPrettySize(), ConsoleColor.Green);
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
            Program.PrintColorMessage(Build.BuildTimeStamp(), ConsoleColor.DarkGray, false);
            Program.PrintColorMessage("Building release checksums...", ConsoleColor.Cyan);

            try
            {
                StringBuilder sb = new StringBuilder();

                foreach (BuildUploadFile name in BuildConfig.Build_Release_Files)
                {
                    if (!name.UploadNightly)
                        continue;

                    string file = Build.OutputDirectory + name.FileName;

                    if (File.Exists(file))
                    {
                        string info = Path.GetFileName(file);
                        string hash = Verify.HashFile(file);

                        sb.AppendLine(info);
                        sb.AppendLine($"SHA256: {hash}{Environment.NewLine}");
                    }
                }

                Win32.CreateDirectory(Build.OutputDirectory);

                Win32.DeleteFile(
                    $"{Build.OutputDirectory}\\systeminformer-build-checksums.txt"
                    );

                File.WriteAllText(
                    $"{Build.OutputDirectory}\\systeminformer-build-checksums.txt",
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
            if (Flags.HasFlag(BuildFlags.Build32bit))
            {
                StringBuilder compilerOptions = new StringBuilder(0x100);
                StringBuilder commandLine = new StringBuilder(0x100);

                Program.PrintColorMessage(Build.BuildTimeStamp(), ConsoleColor.DarkGray, false, Flags);
                Program.PrintColorMessage($"Building {Path.GetFileNameWithoutExtension(Solution)} (", ConsoleColor.Cyan, false, Flags);
                Program.PrintColorMessage("x32", ConsoleColor.Green, false, Flags);
                Program.PrintColorMessage(")...", ConsoleColor.Cyan, true, Flags);

                if (Flags.HasFlag(BuildFlags.BuildApi))
                    compilerOptions.Append("SI_BUILD_API;");
                if (!string.IsNullOrWhiteSpace(Build.BuildCommit) && Build.BuildNightly)
                    compilerOptions.Append("SI_BUILD_CI;");
                if (Flags.HasFlag(BuildFlags.BuildMsix))
                    compilerOptions.Append("SI_BUILD_MSIX;");
                if (!string.IsNullOrWhiteSpace(Build.BuildCommit))
                    compilerOptions.Append($"PHAPP_VERSION_COMMITHASH=\"{Build.BuildCommit.AsSpan(0, 7)}\";");
                if (!string.IsNullOrWhiteSpace(Build.BuildRevision))
                    compilerOptions.Append($"PHAPP_VERSION_REVISION=\"{Build.BuildRevision}\";");
                if (!string.IsNullOrWhiteSpace(Build.BuildCount))
                    compilerOptions.Append($"PHAPP_VERSION_BUILD=\"{Build.BuildCount}\"");

                commandLine.Append("/m /nologo /nodereuse:false /verbosity:quiet /restore ");
                commandLine.Append($"/p:Platform=x64 /p:Configuration={(Flags.HasFlag(BuildFlags.BuildDebug) ? "Debug " : "Release ")}");
                commandLine.Append($"/p:ExternalCompilerOptions=\"{compilerOptions.ToString()}\" ");
                commandLine.Append(Solution);

                int errorcode = Utils.ExecuteMsbuildCommand(
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

                Program.PrintColorMessage(Build.BuildTimeStamp(), ConsoleColor.DarkGray, false, Flags);
                Program.PrintColorMessage($"Building {Path.GetFileNameWithoutExtension(Solution)} (", ConsoleColor.Cyan, false, Flags);
                Program.PrintColorMessage("x64", ConsoleColor.Green, false, Flags);
                Program.PrintColorMessage(")...", ConsoleColor.Cyan, true, Flags);

                if (Flags.HasFlag(BuildFlags.BuildApi))
                    compilerOptions.Append("SI_BUILD_API;");
                if (!string.IsNullOrWhiteSpace(Build.BuildCommit) && Build.BuildNightly)
                    compilerOptions.Append("SI_BUILD_CI;");
                if (Flags.HasFlag(BuildFlags.BuildMsix))
                    compilerOptions.Append("SI_BUILD_MSIX;");
                if (!string.IsNullOrWhiteSpace(Build.BuildCommit))
                    compilerOptions.Append($"PHAPP_VERSION_COMMITHASH=\"{Build.BuildCommit.AsSpan(0, 7)}\";");
                if (!string.IsNullOrWhiteSpace(Build.BuildRevision))
                    compilerOptions.Append($"PHAPP_VERSION_REVISION=\"{Build.BuildRevision}\";");
                if (!string.IsNullOrWhiteSpace(Build.BuildCount))
                    compilerOptions.Append($"PHAPP_VERSION_BUILD=\"{Build.BuildCount}\"");

                commandLine.Append("/m /nologo /nodereuse:false /verbosity:quiet ");
                commandLine.Append($"/p:Platform=x64 /p:Configuration={(Flags.HasFlag(BuildFlags.BuildDebug) ? "Debug " : "Release ")}");
                commandLine.Append($"/p:ExternalCompilerOptions=\"{compilerOptions.ToString()}\" ");
                commandLine.Append(Solution);

                int errorcode = Utils.ExecuteMsbuildCommand(
                    commandLine.ToString(),
                    out string errorstring
                    );

                if (errorcode != 0)
                {
                    Program.PrintColorMessage($"[ERROR] ({errorcode}) {errorstring}", ConsoleColor.Red, true, Flags | BuildFlags.BuildVerbose);
                    return false;
                }
            }

            if (!Build.HaveArm64BuildTools)
            {
                Program.PrintColorMessage(Build.BuildTimeStamp(), ConsoleColor.DarkGray, false, Flags);
                Program.PrintColorMessage($"Building {Path.GetFileNameWithoutExtension(Solution)} (", ConsoleColor.Cyan, false, Flags);
                Program.PrintColorMessage("arm64", ConsoleColor.Green, false, Flags);
                Program.PrintColorMessage(")... ", ConsoleColor.Cyan, false, Flags);
                Program.PrintColorMessage("[SKIPPED] ARM64 build tools not installed.", ConsoleColor.Yellow, true, Flags);
                return true;
            }

            if (Flags.HasFlag(BuildFlags.BuildARM64))
            {
                StringBuilder compilerOptions = new StringBuilder(0x100);
                StringBuilder commandLine = new StringBuilder(0x100);

                Program.PrintColorMessage(Build.BuildTimeStamp(), ConsoleColor.DarkGray, false, Flags);
                Program.PrintColorMessage($"Building {Path.GetFileNameWithoutExtension(Solution)} (", ConsoleColor.Cyan, false, Flags);
                Program.PrintColorMessage("arm64", ConsoleColor.Green, false, Flags);
                Program.PrintColorMessage(")...", ConsoleColor.Cyan, true, Flags);

                if (Flags.HasFlag(BuildFlags.BuildApi))
                    compilerOptions.Append("SI_BUILD_API;");
                if (!string.IsNullOrWhiteSpace(Build.BuildCommit) && Build.BuildNightly)
                    compilerOptions.Append("SI_BUILD_CI;");
                if (Flags.HasFlag(BuildFlags.BuildMsix))
                    compilerOptions.Append("SI_BUILD_MSIX;");
                if (!string.IsNullOrWhiteSpace(Build.BuildCommit))
                    compilerOptions.Append($"PHAPP_VERSION_COMMITHASH=\"{Build.BuildCommit.AsSpan(0, 7)}\";");
                if (!string.IsNullOrWhiteSpace(Build.BuildRevision))
                    compilerOptions.Append($"PHAPP_VERSION_REVISION=\"{Build.BuildRevision}\";");
                if (!string.IsNullOrWhiteSpace(Build.BuildCount))
                    compilerOptions.Append($"PHAPP_VERSION_BUILD=\"{Build.BuildCount}\"");

                commandLine.Append("/m /nologo /nodereuse:false /verbosity:quiet ");
                commandLine.Append($"/p:Platform=ARM64 /p:Configuration={(Flags.HasFlag(BuildFlags.BuildDebug) ? "Debug " : "Release ")}");
                commandLine.Append($"/p:ExternalCompilerOptions=\"{compilerOptions.ToString()}\" ");
                commandLine.Append(Solution);

                int errorcode = Utils.ExecuteMsbuildCommand(
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

        public static bool BuildDeployUpdateConfig()
        {
            string buildBuildId;
            string buildPostSfUrl;
            string buildPostSfApiKey;
            string buildFilename;
            string buildBinHash = null;
            string buildSetupHash = null;
            long buildBinFileLength = 0;
            long buildSetupFileLength = 0;

            if (!Build.BuildNightly)
                return true;
            if (string.IsNullOrWhiteSpace(Build.BuildVersion))
                return false;

            if (!Win32.GetEnvironmentVariable("BUILD_SF_ID", out buildBuildId))
                return false;
            if (!Win32.GetEnvironmentVariable("BUILD_SF_API", out buildPostSfUrl))
                return false;
            if (!Win32.GetEnvironmentVariable("BUILD_SF_KEY", out buildPostSfApiKey))
                return false;

            Program.PrintColorMessage($"{Environment.NewLine}Uploading build artifacts... {Build.BuildVersion}", ConsoleColor.Cyan);

            if (Build.BuildNightly && !VerifyCache.Contains(VerifyCache.BUILD_CI))
            {
                if (!Verify.DecryptEnvironment(VerifyCache.BUILD_CI))
                {
                    Program.PrintColorMessage("[ERROR] BuildDeploy.DecryptEnvironment", ConsoleColor.Red);
                    return false;
                }
            }

            if (!VerifyCache.Contains(VerifyCache.BUILD_CI))
            {
                Program.PrintColorMessage("[ERROR] BuildDeploy.VerifyCache", ConsoleColor.Red);
                return false;
            }

            if (Verify.CreateSignatureString(
                VerifyCache.BUILD_CI,
                $"{Build.OutputDirectory}\\systeminformer-build-bin.zip",
                out string BuildBinSig
                ))
            {
                Program.PrintColorMessage("build-bin.sig not found.", ConsoleColor.Red);
                return false;
            }

            if (Verify.CreateSignatureString(
                VerifyCache.BUILD_CI,
                $"{Build.OutputDirectory}\\systeminformer-build-setup.exe",
                out string BuildSetupSig
                ))
            {
                Program.PrintColorMessage("build-setup.sig not found.", ConsoleColor.Red);
                return false;
            }

            buildFilename = $"{Build.OutputDirectory}\\systeminformer-build-bin.zip";

            if (File.Exists(buildFilename))
            {
                buildBinFileLength = Win32.GetFileSize(buildFilename);
                buildBinHash = Verify.HashFile(buildFilename);
            }

            if (string.IsNullOrWhiteSpace(buildBinHash))
            {
                Program.PrintColorMessage("build-bin hash not found.", ConsoleColor.Red);
                return false;
            }

            buildFilename = $"{Build.OutputDirectory}\\systeminformer-build-setup.exe";

            if (File.Exists(buildFilename))
            {
                buildSetupFileLength = Win32.GetFileSize(buildFilename);
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

            string binziplink = githubMirrorUpload.GetFileUrl($"systeminformer-{Build.BuildVersion}-bin.zip");
            string setupexelink = githubMirrorUpload.GetFileUrl($"systeminformer-{Build.BuildVersion}-setup.exe");

            if (string.IsNullOrWhiteSpace(binziplink) || string.IsNullOrWhiteSpace(setupexelink))
            {
                Program.PrintColorMessage("build-github downloads not found.", ConsoleColor.Red);
                return false;
            }

            try
            {
                BuildUpdateRequest buildUpdateRequest = new BuildUpdateRequest
                {
                    BuildUpdated = Build.TimeStart.ToString("o"),
                    BuildVersion = Build.BuildVersion,
                    BuildCommit = Build.BuildCommit,
                    BuildId = buildBuildId,
                    BinUrl = binziplink,
                    BinLength = buildBinFileLength.ToString(),
                    BinHash = buildBinHash,
                    BinSig = BuildBinSig,
                    SetupUrl = setupexelink,
                    SetupLength = buildSetupFileLength.ToString(),
                    SetupHash = buildSetupHash,
                    SetupSig = BuildSetupSig,
                };

                byte[] buildPostString = JsonSerializer.SerializeToUtf8Bytes(buildUpdateRequest, BuildUpdateRequestContext.Default.BuildUpdateRequest);

                if (buildPostString.LongLength == 0)
                    return false;

                using (HttpClient httpClient = Http.CreateSourceForgeClient(buildPostSfApiKey))
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
            catch (Exception ex)
            {
                Program.PrintColorMessage($"[UpdateBuildWebService-SF] {ex}", ConsoleColor.Red);
                return false;
            }

            return true;
        }

        public static GithubRelease BuildDeployUploadGithubConfig()
        {
            if (!Github.DeleteRelease(Build.BuildVersion))
                return null;

            var mirror = new GithubRelease();

            try
            {
                // Create new release.

                var response = Github.CreateRelease(Build.BuildVersion);

                if (response == null)
                {
                    Program.PrintColorMessage("[Github.CreateRelease]", ConsoleColor.Red);
                    return null;
                }

                // Upload the files.

                foreach (BuildUploadFile file in BuildConfig.Build_Release_Files)
                {
                    if (!file.UploadNightly)
                        continue;

                    string sourceFile = Build.OutputDirectory + file.FileName;

                    if (File.Exists(sourceFile))
                    {
                        var result = Github.UploadAssets(Build.BuildVersion, sourceFile, response.UploadUrl);

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

                // Query the release download locations.

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

        private static void BuildMsixPackageManifest(BuildFlags Flags)
        {
            if (Flags.HasFlag(BuildFlags.Build32bit))
            {
                // Create the package manifest.

                if (File.Exists("tools\\msix\\PackageTemplate.msix.xml"))
                {
                    string msixManifestString = File.ReadAllText("tools\\msix\\PackageTemplate.msix.xml");

                    msixManifestString = msixManifestString.Replace("SI_MSIX_ARCH", "x86");
                    msixManifestString = msixManifestString.Replace("SI_MSIX_VERSION", Build.BuildLongVersion);
                    msixManifestString = msixManifestString.Replace("SI_MSIX_PUBLISHER",
                        "CN=Winsider Seminars &amp; Solutions Inc., O=Winsider Seminars &amp; Solutions Inc., L=Montréal, S=Quebec, C=CA");

                    File.WriteAllText("tools\\msix\\MsixManifest32.xml", msixManifestString);
                }
            }

            if (Flags.HasFlag(BuildFlags.Build64bit))
            {
                // Create the package manifest.

                if (File.Exists("tools\\msix\\PackageTemplate.msix.xml"))
                {
                    string msixManifestString = File.ReadAllText("tools\\msix\\PackageTemplate.msix.xml");

                    msixManifestString = msixManifestString.Replace("SI_MSIX_ARCH", "x64");
                    msixManifestString = msixManifestString.Replace("SI_MSIX_VERSION", Build.BuildLongVersion);
                    msixManifestString = msixManifestString.Replace("SI_MSIX_PUBLISHER",
                        "CN=Winsider Seminars &amp; Solutions Inc., O=Winsider Seminars &amp; Solutions Inc., L=Montréal, S=Quebec, C=CA");

                    File.WriteAllText("tools\\msix\\MsixManifest64.xml", msixManifestString);
                }
            }
        }

        private static void BuildMsixPackageMapping(BuildFlags Flags)
        {
            // Create the package mapping file.

            if (Flags.HasFlag(BuildFlags.Build32bit))
            {
                StringBuilder packageMap32 = new StringBuilder(0x100);
                packageMap32.AppendLine("[Files]");
                packageMap32.AppendLine($"\"tools\\msix\\MsixManifest32.xml\" \"AppxManifest.xml\"");
                packageMap32.AppendLine($"\"tools\\msix\\Square44x44Logo.png\" \"Assets\\Square44x44Logo.png\"");
                packageMap32.AppendLine($"\"tools\\msix\\Square50x50Logo.png\" \"Assets\\Square50x50Logo.png\"");
                packageMap32.AppendLine($"\"tools\\msix\\Square150x150Logo.png\" \"Assets\\Square150x150Logo.png\"");

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

                    packageMap32.AppendLine($"\"{filePath}\" \"{filePath.Substring("bin\\Release32\\".Length)}\"");
                }

                File.WriteAllText("tools\\msix\\MsixPackage32.map", packageMap32.ToString());
            }

            // Create the package mapping file.

            if (Flags.HasFlag(BuildFlags.Build64bit))
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

                    packageMap64.AppendLine($"\"{filePath}\" \"{filePath.Substring("bin\\Release64\\".Length)}\"");
                }

                File.WriteAllText("tools\\msix\\MsixPackage64.map", packageMap64.ToString());
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
                    Win32.DeleteFile($"{Build.OutputDirectory}\\systeminformer-build-package-x32.appx");
                    Win32.CreateDirectory(Build.OutputDirectory);

                    var result = Utils.ExecuteMsixCommand(
                        $"pack /o /f tools\\msix\\MsixPackage32.map " +
                        $"/p {Build.OutputDirectory}\\systeminformer-build-package-x32.appx"
                        );

                    Program.PrintColorMessage(result, ConsoleColor.Red);
                }
            }

            if (Flags.HasFlag(BuildFlags.Build64bit))
            {
                if (File.Exists("tools\\msix\\MsixPackage64.map"))
                {
                    Win32.DeleteFile($"{Build.OutputDirectory}\\systeminformer-build-package-x64.appx");
                    Win32.CreateDirectory(Build.OutputDirectory);

                    var result = Utils.ExecuteMsixCommand(
                        $"pack /o /f tools\\msix\\MsixPackage64.map " +
                        $"/p {Build.OutputDirectory}\\systeminformer-build-package-x64.msix"
                        );

                    Program.PrintColorMessage(result, ConsoleColor.Red);
                }
            }

            // Create the bundle package.
            if (
                File.Exists($"{Build.OutputDirectory}\\systeminformer-build-package-x32.msix") &&
                File.Exists($"{Build.OutputDirectory}\\systeminformer-build-package-x64.msix")
                )
            {
                StringBuilder bundleMap = new StringBuilder(0x100);
                bundleMap.AppendLine("[Files]");
                bundleMap.AppendLine($"\"{Build.OutputDirectory}\\systeminformer-build-package-x32.msix\" \"systeminformer-build-package-x32.msix\"");
                bundleMap.AppendLine($"\"{Build.OutputDirectory}\\systeminformer-build-package-x64.msix\" \"systeminformer-build-package-x64.msix\"");

                File.WriteAllText("tools\\msix\\bundle.map", bundleMap.ToString());
            }

            if (File.Exists("tools\\msix\\bundle.map"))
            {
                Win32.DeleteFile($"{Build.OutputDirectory}\\systeminformer-build-package.msixbundle");

                var result = Utils.ExecuteMsixCommand(
                    $"bundle /f tools\\msix\\bundle.map " +
                    $"/p {Build.OutputDirectory}\\systeminformer-build-package.msixbundle"
                    );

                Program.PrintColorMessage($"{result}", ConsoleColor.Red);
            }

            Win32.DeleteFile("tools\\msix\\MsixManifest32.xml");
            Win32.DeleteFile("tools\\msix\\MsixManifest64.xml");
            Win32.DeleteFile("tools\\msix\\MsixPackage32.map");
            Win32.DeleteFile("tools\\msix\\MsixPackage64.map");
            Win32.DeleteFile("tools\\msix\\bundle.map");
        }

        public static void BuildSourceLink(bool BuildSourceLink)
        {
            if (BuildSourceLink)
            {
                if (!string.IsNullOrEmpty(Build.BuildCommit))
                {
                    string directory = Build.CurrentDirectory.Replace("\\", "\\\\", StringComparison.OrdinalIgnoreCase);
                    string value = 
                        $"{{ \"documents\": {{ " +
                        $"\"\\\\*\": \"https://raw.githubusercontent.com/winsiderss/systeminformer/{Build.BuildCommit}/*\", " +
                        $"\"{directory}\\\\*\": \"https://raw.githubusercontent.com/winsiderss/systeminformer/{Build.BuildCommit}/*\", " +
                        $"}} }}";

                    File.WriteAllText("sourcelink.json", value);
                }
            }
            else 
            {
                Win32.DeleteFile("sourcelink.json");
            }
        }
    }
}
