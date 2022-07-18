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

using System;
using System.IO;
using System.Net;
using System.Net.Http;
using System.Text;
using System.Text.Json;

namespace CustomBuildTool
{
    public static class Build
    {
        private static DateTime TimeStart;
        private static bool BuildNightly;
        private static string BuildOutputFolder;
        private static string BuildBranch;
        private static string BuildCommit;
        private static string BuildVersion;
        private static string BuildLongVersion;
        private static string BuildCount;
        private static string BuildRevision;

        public static bool InitializeBuildEnvironment()
        {
            try
            {
                DirectoryInfo info = new DirectoryInfo(".");

                while (info.Parent != null && info.Parent.Parent != null)
                {
                    info = info.Parent;

                    if (File.Exists(info.FullName + "\\SystemInformer.sln"))
                    {
                        Directory.SetCurrentDirectory(info.FullName);
                        break;
                    }
                }
            }
            catch (Exception ex)
            {
                Program.PrintColorMessage("Unable to find project root directory: " + ex, ConsoleColor.Red);
                return false;
            }

            if (!File.Exists("SystemInformer.sln"))
            {
                Program.PrintColorMessage("Unable to find project solution.", ConsoleColor.Red);
                return false;
            }

            Build.TimeStart = DateTime.Now;
            Build.BuildNightly = !string.IsNullOrWhiteSpace(Win32.GetEnvironmentVariable("%APPVEYOR_BUILD_API%"));
            Build.BuildOutputFolder = VisualStudio.GetOutputDirectoryPath();

            {
                VisualStudioInstance instance = VisualStudio.GetVisualStudioInstance();

                if (instance == null)
                {
                    Program.PrintColorMessage("Unable to find Visual Studio.", ConsoleColor.Red);
                    return false;
                }

                if (!instance.HasRequiredDependency)
                {
                    Program.PrintColorMessage("Visual Studio does not have required the packages: ", ConsoleColor.Red);
                    Program.PrintColorMessage(instance.MissingDependencyList, ConsoleColor.Cyan);
                    return false;
                }
            }

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
                //    if (File.Exists(sourceFile))
                //        File.Delete(sourceFile);
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
                    try
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
                                Directory.Delete(path, true);
                            }
                        }
                    }
                    catch (Exception)
                    { }
                }
            }
            catch (Exception ex)
            {
                Program.PrintColorMessage("[Cleanup] " + ex, ConsoleColor.Red);
            }
        }

        public static void SetupBuildEnvironment(bool ShowBuildInfo)
        {
            BuildBranch = string.Empty;
            BuildCommit = string.Empty;
            BuildCount = string.Empty;
            BuildRevision = string.Empty;
            BuildVersion = "1.0.0";
            BuildLongVersion = "1.0.0.0";

            try
            {
                string currentGitDir = VisualStudio.GetGitWorkPath();
                string currentGitPath = VisualStudio.GetGitFilePath();

                if (!string.IsNullOrWhiteSpace(currentGitDir) && !string.IsNullOrWhiteSpace(currentGitPath))
                {
                    BuildBranch = Win32.ShellExecute(currentGitPath, currentGitDir + "rev-parse --abbrev-ref HEAD").Trim();
                    BuildCommit = Win32.ShellExecute(currentGitPath, currentGitDir + "rev-parse HEAD").Trim();
                    BuildCount = Win32.ShellExecute(currentGitPath, currentGitDir + "rev-list --count " + BuildBranch).Trim();
                    string currentGitTag = Win32.ShellExecute(currentGitPath, currentGitDir + "describe --abbrev=0 --tags --always").Trim();

                    if (!string.IsNullOrWhiteSpace(currentGitTag))
                    {
                        BuildRevision = Win32.ShellExecute(currentGitPath, currentGitDir + "rev-list --count \"" + currentGitTag + ".." + BuildBranch + "\"").Trim();
                        BuildVersion = "3.0." + BuildRevision;
                        BuildLongVersion = "3.0." + BuildCount + "." + BuildRevision;
                    }
                }
            }
            catch (Exception) {  }

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
                    Program.PrintColorMessage(instance.GetWindowsSdkVersion() + " (" + instance.GetWindowsSdkFullVersion() + ")", ConsoleColor.Green, true);//  
                    Program.PrintColorMessage("VisualStudio: ", ConsoleColor.DarkGray, false);
                    Program.PrintColorMessage(instance.Name, ConsoleColor.Green, true);
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
            return "[" + (DateTime.Now - TimeStart).ToString("mm\\:ss") + "] ";
        }

        public static void ShowBuildStats()
        {
            TimeSpan buildTime = DateTime.Now - TimeStart;

            Program.PrintColorMessage(Environment.NewLine + "Build Time: ", ConsoleColor.DarkGray, false);
            Program.PrintColorMessage(buildTime.Minutes.ToString(), ConsoleColor.Green, false);
            Program.PrintColorMessage(" minute(s), ", ConsoleColor.DarkGray, false);
            Program.PrintColorMessage(buildTime.Seconds.ToString(), ConsoleColor.Green, false);
            Program.PrintColorMessage(" second(s) " + Environment.NewLine + Environment.NewLine, ConsoleColor.DarkGray, false);
        }

        public static bool CopyTextFiles()
        {
            try
            {
                Win32.CopyIfNewer("README.txt", "bin\\README.txt");
                //Win32.CopyIfNewer("CHANGELOG.txt", "bin\\CHANGELOG.txt"); // TODO: Git log
                Win32.CopyIfNewer("COPYRIGHT.txt", "bin\\COPYRIGHT.txt");
                Win32.CopyIfNewer("LICENSE.txt", "bin\\LICENSE.txt");
            }
            catch (Exception ex)
            {
                Program.PrintColorMessage("[CopyTextFiles] " + ex, ConsoleColor.Red);
                return false;
            }

            return true;
        }

        public static bool CopyWow64Files(BuildFlags Flags)
        {
            //Program.PrintColorMessage("Copying Wow64 support files...", ConsoleColor.Cyan);
            try
            {
                if (Flags.HasFlag(BuildFlags.BuildDebug))
                {
                    if (!Directory.Exists("bin\\Debug64\\x86"))
                        Directory.CreateDirectory("bin\\Debug64\\x86");
                    if (!Directory.Exists("bin\\Debug64\\x86\\plugins"))
                        Directory.CreateDirectory("bin\\Debug64\\x86\\plugins");

                    Win32.CopyIfNewer("bin\\Debug32\\SystemInformer.exe", "bin\\Debug64\\x86\\SystemInformer.exe");
                    Win32.CopyIfNewer("bin\\Debug32\\SystemInformer.pdb", "bin\\Debug64\\x86\\SystemInformer.pdb");
                    Win32.CopyIfNewer("bin\\Debug32\\plugins\\DotNetTools.dll", "bin\\Debug64\\x86\\plugins\\DotNetTools.dll");
                    Win32.CopyIfNewer("bin\\Debug32\\plugins\\DotNetTools.pdb", "bin\\Debug64\\x86\\plugins\\DotNetTools.pdb");
                    Win32.CopyIfNewer("bin\\Debug32\\plugins\\ExtendedTools.dll", "bin\\Debug64\\x86\\plugins\\ExtendedTools.dll");
                    Win32.CopyIfNewer("bin\\Debug32\\plugins\\ExtendedTools.pdb", "bin\\Debug64\\x86\\plugins\\ExtendedTools.pdb");      
                }
                else
                {
                    if (!Directory.Exists("bin\\Release64\\x86"))
                        Directory.CreateDirectory("bin\\Release64\\x86");
                    if (!Directory.Exists("bin\\Release64\\x86\\plugins"))
                        Directory.CreateDirectory("bin\\Release64\\x86\\plugins");

                    Win32.CopyIfNewer(
                        "bin\\Release32\\SystemInformer.exe",
                        "bin\\Release64\\x86\\SystemInformer.exe"
                        );
                    Win32.CopyIfNewer(
                        "bin\\Release32\\SystemInformer.pdb",
                        "bin\\Release64\\x86\\SystemInformer.pdb"
                        );
                    Win32.CopyIfNewer(
                        "bin\\Release32\\plugins\\DotNetTools.dll", 
                        "bin\\Release64\\x86\\plugins\\DotNetTools.dll"
                        );
                    Win32.CopyIfNewer(
                        "bin\\Release32\\plugins\\DotNetTools.pdb", 
                        "bin\\Release64\\x86\\plugins\\DotNetTools.pdb"
                        );
                    Win32.CopyIfNewer(
                        "bin\\Release32\\plugins\\ExtendedTools.dll",
                        "bin\\Release64\\x86\\plugins\\ExtendedTools.dll"
                        );
                    Win32.CopyIfNewer(
                        "bin\\Release32\\plugins\\ExtendedTools.pdb",
                        "bin\\Release64\\x86\\plugins\\ExtendedTools.pdb"
                        );
                }
            }
            catch (Exception ex)
            {
                Program.PrintColorMessage("[ERROR] " + ex, ConsoleColor.Red);
                return false;
            }

            return true;
        }

        public static bool CopySidCapsFile(BuildFlags Flags)
        {
            //Program.PrintColorMessage("Copying capability sid support file...", ConsoleColor.Cyan);
            try
            {
                if (Flags.HasFlag(BuildFlags.BuildDebug))
                {
                    if (Flags.HasFlag(BuildFlags.Build32bit))
                    {
                        Win32.CopyIfNewer(
                            "SystemInformer\\resources\\capslist.txt",
                            "bin\\Debug32\\capslist.txt"
                            );
                    }

                    if (Flags.HasFlag(BuildFlags.Build64bit))
                    {
                        Win32.CopyIfNewer(
                            "SystemInformer\\resources\\capslist.txt",
                            "bin\\Debug64\\capslist.txt"
                            );
                    }
                }
                else
                {
                    if (Flags.HasFlag(BuildFlags.Build32bit))
                    {
                        Win32.CopyIfNewer(
                            "SystemInformer\\resources\\capslist.txt",
                            "bin\\Release32\\capslist.txt"
                            );
                    }

                    if (Flags.HasFlag(BuildFlags.Build64bit))
                    {
                        Win32.CopyIfNewer(
                            "SystemInformer\\resources\\capslist.txt",
                            "bin\\Release64\\capslist.txt"
                            );
                    }
                }
            }
            catch (Exception ex)
            {
                Program.PrintColorMessage("[ERROR] " + ex, ConsoleColor.Red);
                return false;
            }

            return true;
        }

        public static bool CopyEtwTraceGuidsFile(BuildFlags Flags)
        {
            //Program.PrintColorMessage("Copying ETW tracelog guids file...", ConsoleColor.Cyan);
            try
            {
                if (Flags.HasFlag(BuildFlags.BuildDebug))
                {
                    if (Flags.HasFlag(BuildFlags.Build32bit))
                        Win32.CopyIfNewer("SystemInformer\\resources\\etwguids.txt", "bin\\Debug32\\etwguids.txt");
                    if (Flags.HasFlag(BuildFlags.Build64bit))
                        Win32.CopyIfNewer("SystemInformer\\resources\\etwguids.txt", "bin\\Debug64\\etwguids.txt");
                }
                else
                {
                    if (Flags.HasFlag(BuildFlags.Build32bit))
                        Win32.CopyIfNewer("SystemInformer\\resources\\etwguids.txt", "bin\\Release32\\etwguids.txt");
                    if (Flags.HasFlag(BuildFlags.Build64bit))
                        Win32.CopyIfNewer("SystemInformer\\resources\\etwguids.txt", "bin\\Release64\\etwguids.txt");
                }
            }
            catch (Exception ex)
            {
                Program.PrintColorMessage("[ERROR] " + ex, ConsoleColor.Red);
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
                Program.PrintColorMessage("[ERROR] " + ex, ConsoleColor.Red);
                return false;
            }

            return true;
        }

        public static bool CopyKernelDriver(BuildFlags Flags)
        {
            string CustomSignToolPath = Verify.GetCustomSignToolFilePath();

            try
            {
                if (Flags.HasFlag(BuildFlags.BuildDebug))
                {
                    if (Directory.Exists("bin\\Debug32"))
                        Win32.CopyIfNewer("KSystemInformer\\bin-signed\\i386\\kprocesshacker.sys", "bin\\Debug32\\SystemInformer.sys");
                    if (Directory.Exists("bin\\Debug64"))
                        Win32.CopyIfNewer("KSystemInformer\\bin-signed\\amd64\\kprocesshacker.sys", "bin\\Debug64\\SystemInformer.sys");
                }
                else
                {
                    if (Directory.Exists("bin\\Release32"))
                        Win32.CopyIfNewer("KSystemInformer\\bin-signed\\i386\\kprocesshacker.sys", "bin\\Release32\\SystemInformer.sys");
                    if (Directory.Exists("bin\\Release64"))
                        Win32.CopyIfNewer("KSystemInformer\\bin-signed\\amd64\\kprocesshacker.sys", "bin\\Release64\\SystemInformer.sys");
                }
            }
            catch (Exception ex)
            {
                Program.PrintColorMessage("[ERROR] (CopyKernelDriver)" + ex, ConsoleColor.Red);
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
                if ((Flags & BuildFlags.Build32bit) == BuildFlags.Build32bit)
                {
                    if (File.Exists("bin\\Debug32\\SystemInformer.exe"))
                    {
                        File.WriteAllText("bin\\Debug32\\SystemInformer.sig", string.Empty);
                        Win32.ShellExecute(CustomSignToolPath, "sign -k " + Verify.GetPath("kph.key") + " bin\\Debug32\\SystemInformer.exe -s bin\\Debug32\\SystemInformer.sig");
                    }

                    if (File.Exists("bin\\Debug64\\SystemInformer.exe"))
                    {
                        File.WriteAllText("bin\\Debug64\\SystemInformer.sig", string.Empty);
                        Win32.ShellExecute(CustomSignToolPath, "sign -k " + Verify.GetPath("kph.key") + " bin\\Debug64\\SystemInformer.exe -s bin\\Debug64\\SystemInformer.sig");
                    }
                }

                if ((Flags & BuildFlags.Build64bit) == BuildFlags.Build64bit)
                {
                    if (File.Exists("bin\\Release32\\SystemInformer.exe"))
                    {
                        File.WriteAllText("bin\\Release32\\SystemInformer.sig", string.Empty);
                        Win32.ShellExecute(CustomSignToolPath, "sign -k " + Verify.GetPath("kph.key") + " bin\\Release32\\SystemInformer.exe -s bin\\Release32\\SystemInformer.sig");
                    }

                    if (File.Exists("bin\\Release64\\SystemInformer.exe"))
                    {
                        File.WriteAllText("bin\\Release64\\SystemInformer.sig", string.Empty);
                        Win32.ShellExecute(CustomSignToolPath, "sign -k " + Verify.GetPath("kph.key") + " bin\\Release64\\SystemInformer.exe -s bin\\Release64\\SystemInformer.sig");
                    }
                }
            }

            if (BuildNightly && File.Exists(Verify.GetPath("kph.key")))
                File.Delete(Verify.GetPath("kph.key"));

            return true;
        }

        public static bool BuildWebSetupExe()
        {
            Program.PrintColorMessage(BuildTimeStamp(), ConsoleColor.DarkGray, false);
            Program.PrintColorMessage("Building build-websetup.exe...", ConsoleColor.Cyan);

            if (!BuildSolution("tools\\CustomSetupTool\\CustomSetupTool.sln", BuildFlags.Build32bit))
                return false;

            try
            {
                VisualStudio.CreateOutputDirectory();

                if (File.Exists(BuildOutputFolder + "\\systeminformer-build-websetup.exe"))
                    File.Delete(BuildOutputFolder + "\\systeminformer-build-websetup.exe");

                File.Move(
                    "tools\\CustomSetupTool\\bin\\Release32\\CustomSetupTool.exe",
                    BuildOutputFolder + "\\systeminformer-build-websetup.exe"
                    );
            }
            catch (Exception ex)
            {
                Program.PrintColorMessage("[ERROR] " + ex, ConsoleColor.Red);
                return false;
            }

            return true;
        }

        public static bool BuildSdk(BuildFlags Flags)
        {
            try
            {
                foreach (string folder in BuildConfig.Build_Sdk_Directories)
                {
                    if (!Directory.Exists(folder))
                    {
                        Directory.CreateDirectory(folder);
                    }
                }

                // Copy the plugin SDK headers
                foreach (string file in BuildConfig.Build_Phnt_Headers)
                    Win32.CopyIfNewer("phnt\\include\\" + file, "sdk\\include\\" + file);
                foreach (string file in BuildConfig.Build_Phlib_Headers)
                    Win32.CopyIfNewer("phlib\\include\\" + file, "sdk\\include\\" + file);

                // Copy readme
                Win32.CopyIfNewer("SystemInformer\\sdk\\readme.txt", "sdk\\readme.txt");
                // Copy symbols
                Win32.CopyIfNewer("bin\\Release32\\SystemInformer.pdb", "sdk\\dbg\\i386\\SystemInformer.pdb");
                Win32.CopyIfNewer("bin\\Release64\\SystemInformer.pdb", "sdk\\dbg\\amd64\\SystemInformer.pdb");
                Win32.CopyIfNewer("KSystemInformer\\bin\\i386\\kprocesshacker.pdb", "sdk\\dbg\\i386\\ksysteminformer.pdb");
                Win32.CopyIfNewer("KSystemInformer\\bin\\amd64\\kprocesshacker.pdb", "sdk\\dbg\\amd64\\ksysteminformer.pdb");
                // Copy sample plugin
                //Win32.CopyIfNewer("plugins\\SamplePlugin\\main.c", "sdk\\samples\\SamplePlugin\\main.c");
                //Win32.CopyIfNewer("plugins\\SamplePlugin\\SamplePlugin.sln", "sdk\\samples\\SamplePlugin\\SamplePlugin.sln");
                //Win32.CopyIfNewer("plugins\\SamplePlugin\\SamplePlugin.vcxproj", "sdk\\samples\\SamplePlugin\\SamplePlugin.vcxproj");
                //Win32.CopyIfNewer("plugins\\SamplePlugin\\SamplePlugin.vcxproj.filters", "sdk\\samples\\SamplePlugin\\SamplePlugin.vcxproj.filters");
                //Win32.CopyIfNewer("plugins\\SamplePlugin\\bin\\Release32\\SamplePlugin.dll", "sdk\\samples\\SamplePlugin\\bin\\Release32\\SamplePlugin.dll");

                // Copy libs
                if (Flags.HasFlag(BuildFlags.BuildDebug))
                {
                    if (Flags.HasFlag(BuildFlags.Build32bit))
                        Win32.CopyIfNewer("bin\\Debug32\\SystemInformer.lib", "sdk\\lib\\i386\\SystemInformer.lib");
                    if (Flags.HasFlag(BuildFlags.Build64bit))
                        Win32.CopyIfNewer("bin\\Debug64\\SystemInformer.lib", "sdk\\lib\\amd64\\SystemInformer.lib");
                }
                else
                {
                    if (Flags.HasFlag(BuildFlags.Build32bit))
                        Win32.CopyIfNewer("bin\\Release32\\SystemInformer.lib", "sdk\\lib\\i386\\SystemInformer.lib");
                    if (Flags.HasFlag(BuildFlags.Build64bit))
                        Win32.CopyIfNewer("bin\\Release64\\SystemInformer.lib", "sdk\\lib\\amd64\\SystemInformer.lib");
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
                Program.PrintColorMessage("[ERROR] " + ex, ConsoleColor.Red);
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
                VisualStudio.CreateOutputDirectory();

                if (File.Exists(BuildOutputFolder + "\\systeminformer-build-setup.exe"))
                    File.Delete(BuildOutputFolder + "\\systeminformer-build-setup.exe");

                File.Move(
                    "tools\\CustomSetupTool\\bin\\Release32\\CustomSetupTool.exe", 
                    BuildOutputFolder + "\\systeminformer-build-setup.exe"
                    );

                if (File.Exists(BuildOutputFolder + "\\systeminformer-build-bin.64"))
                    File.Delete(BuildOutputFolder + "\\systeminformer-build-bin.64");

                Program.PrintColorMessage(new FileInfo(BuildOutputFolder + "\\systeminformer-build-setup.exe").Length.ToPrettySize(), ConsoleColor.Green);
            }
            catch (Exception ex)
            {
                Program.PrintColorMessage("[ERROR] " + ex, ConsoleColor.Red);
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
                VisualStudio.CreateOutputDirectory();

                Zip.CreateCompressedSdkFromFolder("sdk", BuildOutputFolder + "\\systeminformer-build-sdk.zip");

                Program.PrintColorMessage(new FileInfo(BuildOutputFolder + "\\systeminformer-build-sdk.zip").Length.ToPrettySize(), ConsoleColor.Green);
            }
            catch (Exception ex)
            {
                Program.PrintColorMessage("[ERROR] " + ex, ConsoleColor.Red);
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
                if (File.Exists("bin\\Release32\\SystemInformer.exe.settings.xml"))
                    File.Delete("bin\\Release32\\SystemInformer.exe.settings.xml");
                if (File.Exists("bin\\Release64\\SystemInformer.exe.settings.xml"))
                    File.Delete("bin\\Release64\\SystemInformer.exe.settings.xml");

                if (Directory.Exists("bin\\Release32"))
                    File.Create("bin\\Release32\\SystemInformer.exe.settings.xml").Dispose();
                if (Directory.Exists("bin\\Release64"))
                    File.Create("bin\\Release64\\SystemInformer.exe.settings.xml").Dispose();
            }
            catch (Exception ex)
            {
                Program.PrintColorMessage("[WARN] " + ex, ConsoleColor.Yellow);
            }

            try
            {
                if (File.Exists("bin\\Release32\\usernotesdb.xml"))
                    File.Delete("bin\\Release32\\usernotesdb.xml");
                if (File.Exists("bin\\Release64\\usernotesdb.xml"))
                    File.Delete("bin\\Release64\\usernotesdb.xml");

                if (Directory.Exists("bin\\Release32"))
                    File.Create("bin\\Release32\\usernotesdb.xml").Dispose();
                if (Directory.Exists("bin\\Release64"))
                    File.Create("bin\\Release64\\usernotesdb.xml").Dispose();
            }
            catch (Exception ex)
            {
                Program.PrintColorMessage("[WARN] " + ex, ConsoleColor.Yellow);
            }

            try
            {
                VisualStudio.CreateOutputDirectory();

                if (File.Exists(BuildOutputFolder + "\\systeminformer-build-bin.zip"))
                    File.Delete(BuildOutputFolder + "\\systeminformer-build-bin.zip");

                Zip.CreateCompressedFolder(
                    "bin",
                    BuildOutputFolder + "\\systeminformer-build-bin.zip"
                    );

                Program.PrintColorMessage(new FileInfo(BuildOutputFolder + "\\systeminformer-build-bin.zip").Length.ToPrettySize(), ConsoleColor.Green);
            }
            catch (Exception ex)
            {
                Program.PrintColorMessage("[ERROR] " + ex, ConsoleColor.Red);
                return false;
            }

            try
            {
                VisualStudio.CreateOutputDirectory();

                if (File.Exists(BuildOutputFolder + "\\systeminformer-build-bin.64"))
                    File.Delete(BuildOutputFolder + "\\systeminformer-build-bin.64");

                File.WriteAllBytes(
                    BuildOutputFolder + "\\systeminformer-build-bin.64",
                    Encoding.UTF8.GetBytes(Convert.ToBase64String(File.ReadAllBytes(BuildOutputFolder + "\\systeminformer-build-bin.zip")))
                    );
            }
            catch (Exception ex)
            {
                Program.PrintColorMessage("[ERROR] " + ex, ConsoleColor.Red);
            }

            return true;
        }

        public static bool BuildPdbZip()
        {
            Program.PrintColorMessage(BuildTimeStamp(), ConsoleColor.DarkGray, false);
            Program.PrintColorMessage("Building build-pdb.zip... ", ConsoleColor.Cyan, false);

            try
            {
                VisualStudio.CreateOutputDirectory();

                Zip.CreateCompressedPdbFromFolder(
                    ".\\",
                    BuildOutputFolder + "\\systeminformer-build-pdb.zip"
                    );

                Program.PrintColorMessage(new FileInfo(BuildOutputFolder + "\\systeminformer-build-pdb.zip").Length.ToPrettySize(), ConsoleColor.Green);
            }
            catch (Exception ex)
            {
                Program.PrintColorMessage("[ERROR] " + ex, ConsoleColor.Red);
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
                        sb.AppendLine("SHA256: " + hash + Environment.NewLine);
                    }
                }

                VisualStudio.CreateOutputDirectory();

                if (File.Exists(BuildOutputFolder + "\\systeminformer-build-checksums.txt"))
                    File.Delete(BuildOutputFolder + "\\systeminformer-build-checksums.txt");
                File.WriteAllText(BuildOutputFolder + "\\systeminformer-build-checksums.txt", sb.ToString());
            }
            catch (Exception ex)
            {
                Program.PrintColorMessage("[ERROR] " + ex, ConsoleColor.Red);
                return false;
            }

            return true;
        }

        public static bool BuildSolution(string Solution, BuildFlags Flags)
        {
            string msbuildExePath = VisualStudio.GetMsbuildFilePath();

            if (!File.Exists(msbuildExePath))
            {
                Program.PrintColorMessage("Unable to find MsBuild.", ConsoleColor.Red);
                return false;
            }

            if ((Flags & BuildFlags.Build32bit) == BuildFlags.Build32bit)
            {
                StringBuilder compilerOptions = new StringBuilder();
                StringBuilder commandLine = new StringBuilder();

                Program.PrintColorMessage(BuildTimeStamp(), ConsoleColor.DarkGray, false, Flags);
                Program.PrintColorMessage("Building " + Path.GetFileNameWithoutExtension(Solution) + " (", ConsoleColor.Cyan, false, Flags);
                Program.PrintColorMessage("x32", ConsoleColor.Green, false, Flags);
                Program.PrintColorMessage(")...", ConsoleColor.Cyan, true, Flags);

                if (Flags.HasFlag(BuildFlags.BuildApi))
                    compilerOptions.Append("PH_BUILD_API;");
                if (!string.IsNullOrWhiteSpace(BuildCommit))
                    compilerOptions.Append($"PHAPP_VERSION_COMMITHASH=\"{BuildCommit.Substring(0, 7)}\";");
                if (!string.IsNullOrWhiteSpace(BuildRevision))
                    compilerOptions.Append($"PHAPP_VERSION_REVISION=\"{BuildRevision}\";");
                if (!string.IsNullOrWhiteSpace(BuildCount))
                    compilerOptions.Append($"PHAPP_VERSION_BUILD=\"{BuildCount}\"");

                commandLine.Append("/m /nologo /nodereuse:false /verbosity:quiet ");
                commandLine.Append("/p:Configuration=" + (Flags.HasFlag(BuildFlags.BuildDebug) ? "Debug " : "Release "));
                commandLine.Append("/p:Platform=Win32 ");
                commandLine.Append("/p:ExternalCompilerOptions=\"" + compilerOptions.ToString() + "\" ");
                if (File.Exists("tools\\versioning\\version.res"))
                    commandLine.Append($"/p:ExternalAdditionalDependencies=\"{Path.GetFullPath("tools\\versioning\\version.res")}\" ");
                commandLine.Append(Solution);

                int errorcode = Win32.CreateProcess(
                    msbuildExePath,
                    commandLine.ToString(),
                    out string errorstring
                    );

                if (errorcode != 0)
                {
                    Program.PrintColorMessage("[ERROR] (" + errorcode + ") " + errorstring, ConsoleColor.Red, true, Flags | BuildFlags.BuildVerbose);
                    return false;
                }
            }

            if ((Flags & BuildFlags.Build64bit) == BuildFlags.Build64bit)
            {
                StringBuilder compilerOptions = new StringBuilder();
                StringBuilder commandLine = new StringBuilder();

                Program.PrintColorMessage(BuildTimeStamp(), ConsoleColor.DarkGray, false, Flags);
                Program.PrintColorMessage("Building " + Path.GetFileNameWithoutExtension(Solution) + " (", ConsoleColor.Cyan, false, Flags);
                Program.PrintColorMessage("x64", ConsoleColor.Green, false, Flags);
                Program.PrintColorMessage(")...", ConsoleColor.Cyan, true, Flags);

                if (Flags.HasFlag(BuildFlags.BuildApi))
                    compilerOptions.Append("PH_BUILD_API;");
                if (!string.IsNullOrWhiteSpace(BuildCommit))
                    compilerOptions.Append($"PHAPP_VERSION_COMMITHASH=\"{BuildCommit.Substring(0, 7)}\";");
                if (!string.IsNullOrWhiteSpace(BuildRevision))
                    compilerOptions.Append($"PHAPP_VERSION_REVISION=\"{BuildRevision}\";");
                if (!string.IsNullOrWhiteSpace(BuildCount))
                    compilerOptions.Append($"PHAPP_VERSION_BUILD=\"{BuildCount}\"");

                commandLine.Append("/m /nologo /nodereuse:false /verbosity:quiet ");
                commandLine.Append("/p:Configuration=" + (Flags.HasFlag(BuildFlags.BuildDebug) ? "Debug " : "Release "));
                commandLine.Append("/p:Platform=x64 ");
                commandLine.Append("/p:ExternalCompilerOptions=\"" + compilerOptions.ToString() + "\" ");
                if (File.Exists("tools\\versioning\\version.res"))
                    commandLine.Append($"/p:ExternalAdditionalDependencies=\"{Path.GetFullPath("tools\\versioning\\version.res")}\" ");
                commandLine.Append(Solution);

                int errorcode = Win32.CreateProcess(
                    msbuildExePath,
                    commandLine.ToString(),
                    out string errorstring
                    );

                if (errorcode != 0)
                {
                    Program.PrintColorMessage("[ERROR] (" + errorcode + ") " + errorstring, ConsoleColor.Red, true, Flags | BuildFlags.BuildVerbose);
                    return false;
                }
            }

            return true;
        }

        public static bool BuildVersionInfo(BuildFlags Flags)
        {
            if (!File.Exists("tools\\versioning\\version.rc"))
                return true;

            StringBuilder commandLine = new StringBuilder();
            string windowsSdkPath = VisualStudio.GetWindowsSdkPath();
            string windowsSdkIncludePath = VisualStudio.GetWindowsSdkIncludePath();
            string resIncludePath = Path.GetFullPath("systeminformer");
            string rcExePath = windowsSdkPath + "\\x64\\rc.exe";

            if (!File.Exists(rcExePath))
            {
                Program.PrintColorMessage("Unable to find the resource compiler.", ConsoleColor.Red);
                return true;
            }

            commandLine.Append($"/nologo /v /i \"{windowsSdkIncludePath}\\um\" /i \"{windowsSdkIncludePath}\\shared\" /i \"{resIncludePath}\" ");

            if (Flags.HasFlag(BuildFlags.BuildApi))
                commandLine.Append(" /d \"PH_BUILD_API\" ");
            if (!string.IsNullOrWhiteSpace(BuildCommit))
                commandLine.Append($"/d \"PHAPP_VERSION_COMMITHASH=\"{BuildCommit.Substring(0, 7)}\"\" ");
            if (!string.IsNullOrWhiteSpace(BuildRevision))
                commandLine.Append($"/d \"PHAPP_VERSION_REVISION=\"{BuildRevision}\"\" ");
            if (!string.IsNullOrWhiteSpace(BuildCount))
                commandLine.Append($"/d \"PHAPP_VERSION_BUILD=\"{BuildCount}\"\" ");

            commandLine.Append("/fo tools\\versioning\\version.res tools\\versioning\\version.rc");

            int errorcode = Win32.CreateProcess(
                rcExePath,
                commandLine.ToString(),
                out string errorstring
                );

            if (errorcode != 0)
            {
                Program.PrintColorMessage("[ERROR] (" + errorcode + ") " + errorstring, ConsoleColor.Red, true, Flags | BuildFlags.BuildVerbose);
                return false;
            }

            return true;
        }

        public static bool BuildDeployUpdateConfig()
        {
            string buildBuildId;
            //string buildJobId;
            string buildPostUrl;
            string buildPostApiKey;
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
            //buildJobId = Win32.GetEnvironmentVariable("%APPVEYOR_JOB_ID%");
            buildPostUrl = Win32.GetEnvironmentVariable("%APPVEYOR_BUILD_API%");
            buildPostApiKey = Win32.GetEnvironmentVariable("%APPVEYOR_BUILD_KEY%");
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
            //if (string.IsNullOrWhiteSpace(buildJobId))
            //    return false;
            if (string.IsNullOrWhiteSpace(buildPostUrl))
                return false;
            if (string.IsNullOrWhiteSpace(buildPostApiKey))
                return false;
            if (string.IsNullOrWhiteSpace(buildPostSfUrl))
                return false;
            if (string.IsNullOrWhiteSpace(buildPostSfApiKey))
                return false;
            if (string.IsNullOrWhiteSpace(BuildVersion))
                return false;

            Program.PrintColorMessage(Environment.NewLine + "Uploading build artifacts... " + BuildVersion, ConsoleColor.Cyan);

            if (BuildNightly && !File.Exists(Verify.GetPath("nightly.key")))
            {
                string buildKey = Win32.GetEnvironmentVariable("%NIGHTLY_BUILD_KEY%");

                if (!string.IsNullOrWhiteSpace(buildKey))
                {
                    Verify.Decrypt(Verify.GetPath("nightly.s"), Verify.GetPath("nightly.key"), buildKey);
                }

                if (!File.Exists(Verify.GetPath("nightly.key")))
                {
                    Program.PrintColorMessage("[SKIPPED] nightly.key not found.", ConsoleColor.Yellow);
                    return true;
                }
            }

            string customSignToolPath = Verify.GetCustomSignToolFilePath();

            if (File.Exists(customSignToolPath))
            {
                BuildBinSig = Win32.ShellExecute(
                    customSignToolPath,
                    "sign -k " + Verify.GetPath("nightly.key") + " " + BuildOutputFolder + "\\systeminformer-build-bin.zip -h"
                    );
                BuildSetupSig = Win32.ShellExecute(
                    customSignToolPath,
                    "sign -k " + Verify.GetPath("nightly.key") + " " + BuildOutputFolder + "\\systeminformer-build-setup.exe -h"
                    );
            }
            else
            {
                Program.PrintColorMessage("[SKIPPED] CustomSignTool not found.", ConsoleColor.Yellow);
                return true;
            }

            if (BuildNightly && File.Exists(Verify.GetPath("nightly.key")))
                File.Delete(Verify.GetPath("nightly.key"));

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

            buildFilename = BuildOutputFolder + "\\systeminformer-build-bin.zip";

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

            buildFilename = BuildOutputFolder + "\\systeminformer-build-setup.exe";

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
                        httpClient.DefaultRequestHeaders.Add("X-ApiKey", buildPostApiKey);
                        //httpClient.DefaultVersionPolicy = HttpVersionPolicy.RequestVersionOrHigher;
                        //httpClient.DefaultRequestVersion = HttpVersion.Version20;

                        using (ByteArrayContent httpContent = new ByteArrayContent(buildPostString))
                        {
                            httpContent.Headers.ContentType = new System.Net.Http.Headers.MediaTypeHeaderValue("application/json");

                            var httpTask = httpClient.PostAsync(buildPostUrl, httpContent);
                            httpTask.Wait();

                            if (!httpTask.Result.IsSuccessStatusCode)
                            {
                                Program.PrintColorMessage("[UpdateBuildWebService] " + httpTask.Result, ConsoleColor.Red);
                                return false;
                            }
                        }
                    }
                }

                try
                {
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
                                httpContent.Headers.ContentType = new System.Net.Http.Headers.MediaTypeHeaderValue("application/json");

                                var httpTask = httpClient.PostAsync(buildPostSfUrl, httpContent);
                                httpTask.Wait();

                                if (!httpTask.Result.IsSuccessStatusCode)
                                {
                                    Program.PrintColorMessage("[UpdateBuildWebService-SF] " + httpTask.Result, ConsoleColor.Red);
                                    return false;
                                }
                            }
                        }
                    }
                }
                catch (Exception ex)
                {
                    Program.PrintColorMessage("[UpdateBuildWebService-SF] " + ex, ConsoleColor.Red);
                    return false;
                }
            }
            catch (Exception ex)
            {
                Program.PrintColorMessage("[UpdateBuildWebService] " + ex, ConsoleColor.Red);
                return false;
            }

            if (!AppVeyor.UpdateBuildVersion(BuildVersion)) // Update Appveyor build version string.
            {
                return false;
            }

            return true;
        }

        public static bool BuildDeployUploadArtifacts()
        {
            string buildPostUrl;
            string buildPostKey;
            string buildPostName;

            if (!BuildNightly)
                return true;

            buildPostUrl = Win32.GetEnvironmentVariable("%APPVEYOR_NIGHTLY_URL%");
            buildPostKey = Win32.GetEnvironmentVariable("%APPVEYOR_NIGHTLY_KEY%");
            buildPostName = Win32.GetEnvironmentVariable("%APPVEYOR_NIGHTLY_NAME%");

            if (string.IsNullOrWhiteSpace(buildPostUrl))
                return false;
            if (string.IsNullOrWhiteSpace(buildPostKey))
                return false;
            if (string.IsNullOrWhiteSpace(buildPostName))
                return false;

            Program.PrintColorMessage(string.Empty, ConsoleColor.Black);

            try
            {
                foreach (BuildFile file in BuildConfig.Build_Release_Files)
                {
                    string sourceFile = BuildOutputFolder + file.FileName;

                    if (File.Exists(sourceFile))
                    {
                        string filename;
                        FtpWebRequest request;

                        filename = Path.GetFileName(sourceFile);
                        //filename = filename.Replace("-build-", $"-{BuildVersion}-", StringComparison.OrdinalIgnoreCase);

                        #pragma warning disable SYSLIB0014 // Type or member is obsolete
                        request = (FtpWebRequest)WebRequest.Create(buildPostUrl + filename);
                        #pragma warning restore SYSLIB0014 // Type or member is obsolete
                        request.Credentials = new NetworkCredential(buildPostKey, buildPostName);
                        request.Method = WebRequestMethods.Ftp.UploadFile;
                        request.Timeout = System.Threading.Timeout.Infinite;
                        request.EnableSsl = true;
                        request.UsePassive = true;
                        request.UseBinary = true;

                        Program.PrintColorMessage($"Uploading {filename}...", ConsoleColor.Cyan, true);

                        using (FileStream fileStream = File.OpenRead(sourceFile))
                        using (BufferedStream localStream = new BufferedStream(fileStream))
                        using (BufferedStream remoteStream = new BufferedStream(request.GetRequestStream()))
                        {
                            localStream.CopyTo(remoteStream);
                        }

                        using (FtpWebResponse response = (FtpWebResponse)request.GetResponse())
                        {
                            if (response.StatusCode != FtpStatusCode.CommandOK && response.StatusCode != FtpStatusCode.ClosingData)
                            {
                                Program.PrintColorMessage($"[HttpWebResponse] {response.StatusDescription}", ConsoleColor.Red);
                                return false;
                            }
                        }
                    }
                }
            }
            catch (Exception ex)
            {
                Program.PrintColorMessage("[UploadBuildWebServiceAsync-Exception]" + ex.Message, ConsoleColor.Red);
                return false;
            }

            //try
            //{
            //    foreach (BuildFile file in BuildConfig.Build_Release_Files)
            //    {
            //        if (!file.UploadNightly)
            //            continue;
            //
            //        string sourceFile = BuildOutputFolder + file.FileName;
            //
            //        if (File.Exists(sourceFile))
            //        {
            //            bool uploaded;
            //            string fileName;
            //
            //            fileName = sourceFile.Replace("-build-", $"-{BuildVersion}-", StringComparison.OrdinalIgnoreCase);
            //
            //            File.Move(sourceFile, fileName, true);
            //            uploaded = AppVeyor.UploadFile(fileName);
            //            File.Move(fileName, sourceFile, true);
            //
            //            if (!uploaded)
            //            {
            //                Program.PrintColorMessage("[WebServiceAppveyorUploadFile]", ConsoleColor.Red);
            //                return false;
            //            }
            //        }
            //        else
            //        {
            //            Program.PrintColorMessage("[SKIPPED] missing file: " + sourceFile, ConsoleColor.Yellow);
            //        }
            //    }
            //}
            //catch (Exception ex)
            //{
            //    Program.PrintColorMessage("[WebServiceAppveyorPushArtifact] " + ex, ConsoleColor.Red);
            //    return false;
            //}

            return true;
        }

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
                        var result = Github.UploadAssets(BuildVersion, sourceFile, response.upload_url);

                        if (result == null)
                        {
                            Program.PrintColorMessage("[Github.UploadAssets]", ConsoleColor.Red);
                            return null;
                        }

                        //mirror.Files.Add(new GithubReleaseAsset(file.FileName, result.download_url));
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

                    mirror.Files.Add(new GithubReleaseAsset(file.Name, file.download_url));
                }

                mirror.ReleaseId = update.ReleaseId;
            }
            catch (Exception ex)
            {
                Program.PrintColorMessage("[Github] " + ex, ConsoleColor.Red);
                return null;
            }

            if (!mirror.Valid)
            {
                Program.PrintColorMessage("[Github-Valid]" , ConsoleColor.Red);
                return null;
            }

            return mirror;
        }

        //public static void BuildAppxPackage(BuildFlags Flags)
        //{
        //    string sdkRootPath = string.Empty;
        //    string[] cleanupAppxArray =
        //    {
        //        BuildOutputFolder + "\\AppxManifest32.xml",
        //        BuildOutputFolder + "\\AppxManifest64.xml",
        //        BuildOutputFolder + "\\package32.map",
        //        BuildOutputFolder + "\\package64.map",
        //        BuildOutputFolder + "\\bundle.map",
        //        BuildOutputFolder + "\\SystemInformer-44.png",
        //        BuildOutputFolder + "\\SystemInformer-50.png",
        //        BuildOutputFolder + "\\SystemInformer-150.png"
        //    };
        //
        //    Program.PrintColorMessage("Building systeminformer-build-package.appxbundle...", ConsoleColor.Cyan);
        //
        //    // "shellexecute(reg query)"
        //    using (var view32 = RegistryKey.OpenBaseKey(RegistryHive.LocalMachine, RegistryView.Registry32))
        //    {
        //        using (var kitsroot = view32.OpenSubKey("Software\\Microsoft\\Windows Kits\\Installed Roots", false))
        //        {
        //            sdkRootPath = (string)kitsroot.GetValue("WdkBinRootVersioned", string.Empty);
        //        }
        //    }
        //
        //    if (string.IsNullOrWhiteSpace(sdkRootPath))
        //    {
        //        Program.PrintColorMessage("[Skipped] Windows SDK", ConsoleColor.Red);
        //        return;
        //    }
        //
        //    if (!File.Exists(BuildOutputFolder + "\\systeminformer-appx.cer "))
        //    {
        //        BuildAppxSignature();
        //    }
        //
        //    string makeAppxExePath = Environment.ExpandEnvironmentVariables(sdkRootPath + "\\x64\\MakeAppx.exe");
        //    string signToolExePath = Environment.ExpandEnvironmentVariables(sdkRootPath + "\\x64\\SignTool.exe");
        //
        //    try
        //    {
        //        //Win32.ImageResizeFile(44, "SystemInformer\\resources\\SystemInformer.png", BuildOutputFolder + "\\SystemInformer-44.png");
        //        //Win32.ImageResizeFile(50, "SystemInformer\\resources\\SystemInformer.png", BuildOutputFolder + "\\SystemInformer-50.png");
        //        //Win32.ImageResizeFile(150, "SystemInformer\\resources\\SystemInformer.png", BuildOutputFolder + "\\SystemInformer-150.png");
        //
        //        if ((Flags & BuildFlags.Build32bit) == BuildFlags.Build32bit)
        //        {
        //            // create the package manifest
        //            string appxManifestString = Properties.Resources.AppxManifest;
        //            appxManifestString = appxManifestString.Replace("PH_APPX_ARCH", "x86");
        //            appxManifestString = appxManifestString.Replace("PH_APPX_VERSION", BuildVersion + ".0");
        //            File.WriteAllText(BuildOutputFolder + "\\AppxManifest32.xml", appxManifestString);
        //
        //            // create the package mapping file
        //            StringBuilder packageMap32 = new StringBuilder(0x100);
        //            packageMap32.AppendLine("[Files]");
        //            packageMap32.AppendLine("\"" + BuildOutputFolder + "\\AppxManifest32.xml\" \"AppxManifest.xml\"");
        //            packageMap32.AppendLine("\"" + BuildOutputFolder + "\\SystemInformer-44.png\" \"Assets\\SystemInformer-44.png\"");
        //            packageMap32.AppendLine("\"" + BuildOutputFolder + "\\SystemInformer-50.png\" \"Assets\\SystemInformer-50.png\"");
        //            packageMap32.AppendLine("\"" + BuildOutputFolder + "\\SystemInformer-150.png\" \"Assets\\SystemInformer-150.png\"");
        //
        //            var filesToAdd = Directory.GetFiles("bin\\Release32", "*", SearchOption.AllDirectories);
        //            foreach (string filePath in filesToAdd)
        //            {
        //                // Ignore junk files
        //                if (filePath.EndsWith(".pdb", StringComparison.OrdinalIgnoreCase) ||
        //                    filePath.EndsWith(".iobj", StringComparison.OrdinalIgnoreCase) ||
        //                    filePath.EndsWith(".ipdb", StringComparison.OrdinalIgnoreCase) ||
        //                    filePath.EndsWith(".exp", StringComparison.OrdinalIgnoreCase) ||
        //                    filePath.EndsWith(".lib", StringComparison.OrdinalIgnoreCase) ||
        //                    filePath.EndsWith(".xml", StringComparison.OrdinalIgnoreCase) ||
        //                    filePath.EndsWith(".manifest", StringComparison.OrdinalIgnoreCase)
        //                    )
        //                {
        //                    continue;
        //                }
        //
        //                packageMap32.AppendLine("\"" + filePath + "\" \"" + filePath.Substring("bin\\Release32\\".Length) + "\"");
        //            }
        //            File.WriteAllText(BuildOutputFolder + "\\package32.map", packageMap32.ToString());
        //
        //            // create the package
        //            var error = Win32.ShellExecute(
        //                makeAppxExePath,
        //                "pack /o /f " + BuildOutputFolder + "\\package32.map /p " +
        //                BuildOutputFolder + "\\systeminformer-build-package-x32.appx"
        //                );
        //            Program.PrintColorMessage(Environment.NewLine + error, ConsoleColor.Gray, true, Flags);
        //
        //            // sign the package
        //            error = Win32.ShellExecute(
        //                signToolExePath,
        //                "sign /v /fd SHA256 /a /f " + BuildOutputFolder + "\\systeminformer-appx.pfx /td SHA256 " +
        //                BuildOutputFolder + "\\systeminformer-build-package-x32.appx"
        //                );
        //            Program.PrintColorMessage(Environment.NewLine + error, ConsoleColor.Gray, true, Flags);
        //        }
        //
        //        if ((Flags & BuildFlags.Build64bit) == BuildFlags.Build64bit)
        //        {
        //            // create the package manifest
        //            string appxManifestString = Properties.Resources.AppxManifest;
        //            appxManifestString = appxManifestString.Replace("PH_APPX_ARCH", "x64");
        //            appxManifestString = appxManifestString.Replace("PH_APPX_VERSION", BuildVersion + ".0");
        //            File.WriteAllText(BuildOutputFolder + "\\AppxManifest64.xml", appxManifestString);
        //
        //            StringBuilder packageMap64 = new StringBuilder(0x100);
        //            packageMap64.AppendLine("[Files]");
        //            packageMap64.AppendLine("\"" + BuildOutputFolder + "\\AppxManifest64.xml\" \"AppxManifest.xml\"");
        //            packageMap64.AppendLine("\"" + BuildOutputFolder + "\\SystemInformer-44.png\" \"Assets\\SystemInformer-44.png\"");
        //            packageMap64.AppendLine("\"" + BuildOutputFolder + "\\SystemInformer-50.png\" \"Assets\\SystemInformer-50.png\"");
        //            packageMap64.AppendLine("\"" + BuildOutputFolder + "\\SystemInformer-150.png\" \"Assets\\SystemInformer-150.png\"");
        //
        //            var filesToAdd = Directory.GetFiles("bin\\Release64", "*", SearchOption.AllDirectories);
        //            foreach (string filePath in filesToAdd)
        //            {
        //                // Ignore junk files
        //                if (filePath.EndsWith(".pdb", StringComparison.OrdinalIgnoreCase) ||
        //                    filePath.EndsWith(".iobj", StringComparison.OrdinalIgnoreCase) ||
        //                    filePath.EndsWith(".ipdb", StringComparison.OrdinalIgnoreCase) ||
        //                    filePath.EndsWith(".exp", StringComparison.OrdinalIgnoreCase) ||
        //                    filePath.EndsWith(".lib", StringComparison.OrdinalIgnoreCase) ||
        //                    filePath.EndsWith(".xml", StringComparison.OrdinalIgnoreCase) ||
        //                    filePath.EndsWith(".manifest", StringComparison.OrdinalIgnoreCase)
        //                    )
        //                {
        //                    continue;
        //                }
        //
        //                packageMap64.AppendLine("\"" + filePath + "\" \"" + filePath.Substring("bin\\Release64\\".Length) + "\"");
        //            }
        //            File.WriteAllText(BuildOutputFolder + "\\package64.map", packageMap64.ToString());
        //
        //            // create the package
        //            var error = Win32.ShellExecute(
        //                makeAppxExePath,
        //                "pack /o /f " + BuildOutputFolder + "\\package64.map /p " +
        //                BuildOutputFolder + "\\systeminformer-build-package-x64.appx"
        //                );
        //            Program.PrintColorMessage(Environment.NewLine + error, ConsoleColor.Gray, true, Flags);
        //
        //            // sign the package
        //            error = Win32.ShellExecute(
        //                signToolExePath,
        //                "sign /v /fd SHA256 /a /f " + BuildOutputFolder + "\\systeminformer-appx.pfx /td SHA256 " +
        //                BuildOutputFolder + "\\systeminformer-build-package-x64.appx"
        //                );
        //            Program.PrintColorMessage(Environment.NewLine + error, ConsoleColor.Gray, true, Flags);
        //        }
        //
        //        {
        //            // create the appx bundle map
        //            StringBuilder bundleMap = new StringBuilder(0x100);
        //            bundleMap.AppendLine("[Files]");
        //            bundleMap.AppendLine("\"" + BuildOutputFolder + "\\systeminformer-build-package-x32.appx\" \"systeminformer-build-package-x32.appx\"");
        //            bundleMap.AppendLine("\"" + BuildOutputFolder + "\\systeminformer-build-package-x64.appx\" \"systeminformer-build-package-x64.appx\"");
        //            File.WriteAllText(BuildOutputFolder + "\\bundle.map", bundleMap.ToString());
        //
        //            if (File.Exists(BuildOutputFolder + "\\systeminformer-build-package.appxbundle"))
        //                File.Delete(BuildOutputFolder + "\\systeminformer-build-package.appxbundle");
        //
        //            // create the appx bundle package
        //            var error = Win32.ShellExecute(
        //                makeAppxExePath,
        //                "bundle /f " + BuildOutputFolder + "\\bundle.map /p " +
        //                BuildOutputFolder + "\\systeminformer-build-package.appxbundle"
        //                );
        //            Program.PrintColorMessage(Environment.NewLine + error, ConsoleColor.Gray, true, Flags);
        //
        //            // sign the appx bundle package
        //            error = Win32.ShellExecute(
        //                signToolExePath,
        //                "sign /v /fd SHA256 /a /f " + BuildOutputFolder + "\\systeminformer-appx.pfx /td SHA256 " +
        //                BuildOutputFolder + "\\systeminformer-build-package.appxbundle"
        //                );
        //            Program.PrintColorMessage(Environment.NewLine + error, ConsoleColor.Gray, true, Flags);
        //        }
        //
        //        foreach (string file in cleanupAppxArray)
        //        {
        //            if (File.Exists(file))
        //                File.Delete(file);
        //        }
        //    }
        //    catch (Exception ex)
        //    {
        //        Program.PrintColorMessage("[ERROR] " + ex, ConsoleColor.Red);
        //    }
        //}
        //
        //public static bool BuildAppxSignature()
        //{
        //    string sdkRootPath = string.Empty;
        //    string[] cleanupAppxArray =
        //    {
        //        BuildOutputFolder + "\\systeminformer-appx.pvk",
        //        BuildOutputFolder + "\\systeminformer-appx.cer",
        //        BuildOutputFolder + "\\systeminformer-appx.pfx"
        //    };
        //
        //    Program.PrintColorMessage("Building Appx Signature...", ConsoleColor.Cyan);
        //
        //    // "shellexecute(reg query)"
        //    using (var view32 = RegistryKey.OpenBaseKey(RegistryHive.LocalMachine, RegistryView.Registry32))
        //    {
        //        using (var kitsroot = view32.OpenSubKey("Software\\Microsoft\\Windows Kits\\Installed Roots", false))
        //        {
        //            sdkRootPath = (string)kitsroot.GetValue("WdkBinRootVersioned", string.Empty);
        //        }
        //    }
        //
        //    if (string.IsNullOrWhiteSpace(sdkRootPath))
        //    {
        //        Program.PrintColorMessage("[Skipped] Windows SDK", ConsoleColor.Red);
        //        return false;
        //    }
        //
        //    string makeCertExePath = Environment.ExpandEnvironmentVariables(sdkRootPath + "\\x64\\MakeCert.exe");
        //    string pvk2PfxExePath = Environment.ExpandEnvironmentVariables(sdkRootPath + "\\x64\\Pvk2Pfx.exe");
        //    string certUtilExePath = Win32.SearchFile("certutil.exe");
        //
        //    try
        //    {
        //        foreach (string file in cleanupAppxArray)
        //        {
        //            if (File.Exists(file))
        //                File.Delete(file);
        //        }
        //
        //        string output = Win32.ShellExecute(makeCertExePath,
        //            "/n " +
        //            "\"CN=SystemInformer, O=SystemInformer, C=AU\" " +
        //            "/r /h 0 " +
        //            "/eku \"1.3.6.1.5.5.7.3.3,1.3.6.1.4.1.311.10.3.13\" " +
        //            "/sv " +
        //            BuildOutputFolder + "\\systeminformer-appx.pvk " +
        //            BuildOutputFolder + "\\systeminformer-appx.cer "
        //            );
        //
        //        if (!string.IsNullOrWhiteSpace(output) && !output.Equals("Succeeded", StringComparison.OrdinalIgnoreCase))
        //        {
        //            Program.PrintColorMessage("[MakeCert] " + output, ConsoleColor.Red);
        //            return false;
        //        }
        //
        //        output = Win32.ShellExecute(pvk2PfxExePath,
        //            "/pvk " + BuildOutputFolder + "\\systeminformer-appx.pvk " +
        //            "/spc " + BuildOutputFolder + "\\systeminformer-appx.cer " +
        //            "/pfx " + BuildOutputFolder + "\\systeminformer-appx.pfx "
        //            );
        //
        //        if (!string.IsNullOrWhiteSpace(output))
        //        {
        //            Program.PrintColorMessage("[Pvk2Pfx] " + output, ConsoleColor.Red);
        //            return false;
        //        }
        //
        //        output = Win32.ShellExecute(certUtilExePath,
        //            "-addStore TrustedPeople " + BuildOutputFolder + "\\systeminformer-appx.cer"
        //            );
        //
        //        if (!string.IsNullOrWhiteSpace(output) && !output.Contains("command completed successfully", StringComparison.OrdinalIgnoreCase))
        //        {
        //            Program.PrintColorMessage("[Certutil] " + output, ConsoleColor.Red);
        //            return false;
        //        }
        //
        //        Program.PrintColorMessage("[Pvk2Pfx] " + output, ConsoleColor.Green);
        //    }
        //    catch (Exception ex)
        //    {
        //        Program.PrintColorMessage("[ERROR] " + ex, ConsoleColor.Red);
        //        return false;
        //    }
        //
        //    return true;
        //}
        //
        //public static bool CleanupAppxSignature()
        //{
        //    try
        //    {
        //        X509Store store = new X509Store(StoreName.TrustedPeople, StoreLocation.LocalMachine);
        //
        //        store.Open(OpenFlags.ReadWrite);
        //
        //        foreach (X509Certificate2 c in store.Certificates)
        //        {
        //            if (c.Subject.Equals("CN=SystemInformer, O=SystemInformer, C=AU", StringComparison.OrdinalIgnoreCase))
        //            {
        //                Console.WriteLine("Removing: {0}", c.Subject);
        //                store.Remove(c);
        //            }
        //        }
        //    }
        //    catch (Exception ex)
        //    {
        //        Program.PrintColorMessage("[ERROR] " + ex, ConsoleColor.Red);
        //        return false;
        //    }
        //
        //    return true;
        //}

    }
}
