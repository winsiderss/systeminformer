/*
 * Process Hacker Toolchain - 
 *   Build script
 * 
 * Copyright (C) dmex
 * 
 * This file is part of Process Hacker.
 * 
 * Process Hacker is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Process Hacker is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Process Hacker.  If not, see <http://www.gnu.org/licenses/>.
 */

using System;
using System.IO;
using System.Net;
using System.Net.Http;
using System.Text;
using System.Text.Json;
using System.Security.Cryptography.X509Certificates;
using Microsoft.Win32;

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

                    if (File.Exists(info.FullName + "\\ProcessHacker.sln"))
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

            if (!File.Exists("ProcessHacker.sln"))
            {
                Program.PrintColorMessage("Unable to find project solution.", ConsoleColor.Red);
                return false;
            }

            Build.TimeStart = DateTime.Now;
            Build.BuildNightly = !string.IsNullOrEmpty(Win32.GetEnvironmentVariable("%APPVEYOR_BUILD_API%"));
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
                foreach (BuildFile file in BuildConfig.Build_Release_Files)
                {
                    string sourceFile = BuildOutputFolder + file.FileName;

                    if (File.Exists(sourceFile))
                        File.Delete(sourceFile);
                }

                foreach (string folder in BuildConfig.Build_Sdk_Directories)
                {
                    if (Directory.Exists(folder))
                        Directory.Delete(folder, true);
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

                if (!string.IsNullOrEmpty(currentGitDir) && !string.IsNullOrEmpty(currentGitPath))
                {
                    BuildBranch = Win32.ShellExecute(currentGitPath, currentGitDir + "rev-parse --abbrev-ref HEAD").Trim();
                    BuildCommit = Win32.ShellExecute(currentGitPath, currentGitDir + "rev-parse HEAD").Trim();
                    BuildCount = Win32.ShellExecute(currentGitPath, currentGitDir + "rev-list --count " + BuildBranch).Trim();
                    string currentGitTag = Win32.ShellExecute(currentGitPath, currentGitDir + "describe --abbrev=0 --tags --always").Trim();

                    if (!string.IsNullOrEmpty(currentGitTag))
                    {
                        BuildRevision = Win32.ShellExecute(currentGitPath, currentGitDir + "rev-list --count \"" + currentGitTag + ".." + BuildBranch + "\"").Trim();
                        BuildVersion = "3.0." + BuildRevision;
                        BuildLongVersion = "3.0." + BuildCount + "." + BuildRevision;
                    }
                }
            }
            catch (Exception) {  }

            if (
                string.IsNullOrEmpty(BuildBranch) ||
                string.IsNullOrEmpty(BuildCommit) ||
                string.IsNullOrEmpty(BuildCount) ||
                string.IsNullOrEmpty(BuildRevision)
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
                Program.PrintColorMessage("Windows NT " + Environment.OSVersion.Version.ToString(), ConsoleColor.Green, true);

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

                if (!string.IsNullOrEmpty(BuildCommit))
                {
                    Program.PrintColorMessage(" (", ConsoleColor.DarkGray, false);
                    Program.PrintColorMessage(BuildCommit.Substring(0, 7), ConsoleColor.DarkYellow, false);
                    Program.PrintColorMessage(")", ConsoleColor.DarkGray, false);
                }

                if (!string.IsNullOrEmpty(BuildBranch))
                {
                    Program.PrintColorMessage(" [", ConsoleColor.DarkGray, false);
                    Program.PrintColorMessage(BuildBranch, ConsoleColor.DarkBlue, false);
                    Program.PrintColorMessage("]", ConsoleColor.DarkGray, false);
                }

                Program.PrintColorMessage(Environment.NewLine, ConsoleColor.DarkGray, true);
            }

            //using (var repo = new LibGit2Sharp.Repository(Environment.CurrentDirectory + "\\.git"))
            //{
            //    BuildBranch = repo.Head.FriendlyName;
            //    BuildCommit = repo.Head.Commits.First().Sha;
            //    BuildCount = repo.Commits.Count().ToString();
            //    BuildRevision = repo.Commits.QueryBy(new LibGit2Sharp.CommitFilter
            //    {
            //        SortBy = LibGit2Sharp.CommitSortStrategies.Reverse | LibGit2Sharp.CommitSortStrategies.Time,
            //        ExcludeReachableFrom = repo.Tags[repo.Tags.Last().FriendlyName].Reference,
            //        IncludeReachableFrom = repo.Branches[BuildBranch].Tip
            //    }).Count().ToString();
            //
            //    BuildVersion = "3.0." + BuildRevision;
            //    BuildLongVersion = "3.0." + BuildCount + "." + BuildRevision;
            //}
            //
            //buildChangelog = Win32.ShellExecute(GitExePath, currentGitDir + "log -n 30 --date=format:%Y-%m-%d --pretty=format:\"[%cd] %s (%an)\"");
            //buildSummary = Win32.ShellExecute(GitExePath, currentGitDir + "log -n 5 --date=format:%Y-%m-%d --pretty=format:\"[%cd] %s (%an)\" --abbrev-commit");
            //buildMessage = Win32.ShellExecute(GitExePath, currentGitDir + "log -1 --pretty=%B");
            //
            //log -n 5 --date=format:%Y-%m-%d --pretty=format:\"%C(green)[%cd]%Creset %C(bold blue)%an%Creset %<(65,trunc)%s%Creset %C(#696969)(%Creset%C(yellow)%h%Creset%C(#696969))%Creset\" --abbrev-commit
            //log -n 5 --date=format:%Y-%m-%d --pretty=format:\"[%cd] %an %s\" --abbrev-commit
            //log -n 1 --date=format:%Y-%m-%d --pretty=format:\"[%cd] %an: %<(65,trunc)%s (%h)\" --abbrev-commi
            //log -n 1800 --graph --pretty=format:\"%C(yellow)%h%Creset %C(bold blue)%an%Creset %s %C(dim green)(%cr)\" --abbrev-commit
            //
            // https://api.github.com/repos/processhacker/processhacker/branches
            // https://api.github.com/repos/processhacker/processhacker/tags
            // https://api.github.com/repos/processhacker/processhacker // created_at
            // https://api.github.com/repos/processhacker/processhacker/compare/master@{created_at}...master
            // https://api.github.com/repos/processhacker/processhacker/compare/master@{2016-01-01}...master
            // https://api.github.com/repos/processhacker/processhacker/compare/master...v2.39
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

                    Win32.CopyIfNewer("bin\\Debug32\\ProcessHacker.exe", "bin\\Debug64\\x86\\ProcessHacker.exe");
                    Win32.CopyIfNewer("bin\\Debug32\\ProcessHacker.pdb", "bin\\Debug64\\x86\\ProcessHacker.pdb");
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
                        "bin\\Release32\\ProcessHacker.exe", 
                        "bin\\Release64\\x86\\ProcessHacker.exe"
                        );
                    Win32.CopyIfNewer(
                        "bin\\Release32\\ProcessHacker.pdb", 
                        "bin\\Release64\\x86\\ProcessHacker.pdb"
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
                            "ProcessHacker\\resources\\capslist.txt",
                            "bin\\Debug32\\capslist.txt"
                            );
                    }

                    if (Flags.HasFlag(BuildFlags.Build64bit))
                    {
                        Win32.CopyIfNewer(
                            "ProcessHacker\\resources\\capslist.txt",
                            "bin\\Debug64\\capslist.txt"
                            );
                    }
                }
                else
                {
                    if (Flags.HasFlag(BuildFlags.Build32bit))
                    {
                        Win32.CopyIfNewer(
                            "ProcessHacker\\resources\\capslist.txt",
                            "bin\\Release32\\capslist.txt"
                            );
                    }

                    if (Flags.HasFlag(BuildFlags.Build64bit))
                    {
                        Win32.CopyIfNewer(
                            "ProcessHacker\\resources\\capslist.txt",
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
                        Win32.CopyIfNewer("ProcessHacker\\resources\\etwguids.txt", "bin\\Debug32\\etwguids.txt");
                    if (Flags.HasFlag(BuildFlags.Build64bit))
                        Win32.CopyIfNewer("ProcessHacker\\resources\\etwguids.txt", "bin\\Debug64\\etwguids.txt");
                }
                else
                {
                    if (Flags.HasFlag(BuildFlags.Build32bit))
                        Win32.CopyIfNewer("ProcessHacker\\resources\\etwguids.txt", "bin\\Release32\\etwguids.txt");
                    if (Flags.HasFlag(BuildFlags.Build64bit))
                        Win32.CopyIfNewer("ProcessHacker\\resources\\etwguids.txt", "bin\\Release64\\etwguids.txt");
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

        public static bool CopyKProcessHacker(BuildFlags Flags)
        {
            string CustomSignToolPath = Verify.GetCustomSignToolFilePath();

            try
            {
                if (Flags.HasFlag(BuildFlags.BuildDebug))
                {
                    if (Directory.Exists("bin\\Debug32"))
                        Win32.CopyIfNewer("KProcessHacker\\bin-signed\\i386\\kprocesshacker.sys", "bin\\Debug32\\kprocesshacker.sys");
                    if (Directory.Exists("bin\\Debug64"))
                        Win32.CopyIfNewer("KProcessHacker\\bin-signed\\amd64\\kprocesshacker.sys", "bin\\Debug64\\kprocesshacker.sys");
                }
                else
                {
                    if (Directory.Exists("bin\\Release32"))
                        Win32.CopyIfNewer("KProcessHacker\\bin-signed\\i386\\kprocesshacker.sys", "bin\\Release32\\kprocesshacker.sys");
                    if (Directory.Exists("bin\\Release64"))
                        Win32.CopyIfNewer("KProcessHacker\\bin-signed\\amd64\\kprocesshacker.sys", "bin\\Release64\\kprocesshacker.sys");
                }
            }
            catch (Exception ex)
            {
                Program.PrintColorMessage("[ERROR] (kprocesshacker.sys)" + ex, ConsoleColor.Red);
            }

            if (BuildNightly && !File.Exists(Verify.GetPath("kph.key")))
            {
                string kphKey = Win32.GetEnvironmentVariable("%KPH_BUILD_KEY%");

                if (!string.IsNullOrEmpty(kphKey))
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
                    if (File.Exists("bin\\Debug32\\ProcessHacker.exe"))
                    {
                        File.WriteAllText("bin\\Debug32\\ProcessHacker.sig", string.Empty);
                        Win32.ShellExecute(CustomSignToolPath, "sign -k " + Verify.GetPath("kph.key") + " bin\\Debug32\\ProcessHacker.exe -s bin\\Debug32\\ProcessHacker.sig");
                    }

                    if (File.Exists("bin\\Debug64\\ProcessHacker.exe"))
                    {
                        File.WriteAllText("bin\\Debug64\\ProcessHacker.sig", string.Empty);
                        Win32.ShellExecute(CustomSignToolPath, "sign -k " + Verify.GetPath("kph.key") + " bin\\Debug64\\ProcessHacker.exe -s bin\\Debug64\\ProcessHacker.sig");
                    }
                }

                if ((Flags & BuildFlags.Build64bit) == BuildFlags.Build64bit)
                {
                    if (File.Exists("bin\\Release32\\ProcessHacker.exe"))
                    {
                        File.WriteAllText("bin\\Release32\\ProcessHacker.sig", string.Empty);
                        Win32.ShellExecute(CustomSignToolPath, "sign -k " + Verify.GetPath("kph.key") + " bin\\Release32\\ProcessHacker.exe -s bin\\Release32\\ProcessHacker.sig");
                    }

                    if (File.Exists("bin\\Release64\\ProcessHacker.exe"))
                    {
                        File.WriteAllText("bin\\Release64\\ProcessHacker.sig", string.Empty);
                        Win32.ShellExecute(CustomSignToolPath, "sign -k " + Verify.GetPath("kph.key") + " bin\\Release64\\ProcessHacker.exe -s bin\\Release64\\ProcessHacker.sig");
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

                if (File.Exists(BuildOutputFolder + "\\processhacker-build-websetup.exe"))
                    File.Delete(BuildOutputFolder + "\\processhacker-build-websetup.exe");

                File.Move(
                    "tools\\CustomSetupTool\\bin\\Release32\\CustomSetupTool.exe",
                    BuildOutputFolder + "\\processhacker-build-websetup.exe"
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
                Win32.CopyIfNewer("ProcessHacker\\sdk\\readme.txt", "sdk\\readme.txt");
                // Copy symbols
                Win32.CopyIfNewer("bin\\Release32\\ProcessHacker.pdb", "sdk\\dbg\\i386\\ProcessHacker.pdb");
                Win32.CopyIfNewer("bin\\Release64\\ProcessHacker.pdb", "sdk\\dbg\\amd64\\ProcessHacker.pdb");
                Win32.CopyIfNewer("KProcessHacker\\bin\\i386\\kprocesshacker.pdb", "sdk\\dbg\\i386\\kprocesshacker.pdb");
                Win32.CopyIfNewer("KProcessHacker\\bin\\amd64\\kprocesshacker.pdb", "sdk\\dbg\\amd64\\kprocesshacker.pdb");
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
                        Win32.CopyIfNewer("bin\\Debug32\\ProcessHacker.lib", "sdk\\lib\\i386\\ProcessHacker.lib");
                    if (Flags.HasFlag(BuildFlags.Build64bit))
                        Win32.CopyIfNewer("bin\\Debug64\\ProcessHacker.lib", "sdk\\lib\\amd64\\ProcessHacker.lib");
                }
                else
                {
                    if (Flags.HasFlag(BuildFlags.Build32bit))
                        Win32.CopyIfNewer("bin\\Release32\\ProcessHacker.lib", "sdk\\lib\\i386\\ProcessHacker.lib");
                    if (Flags.HasFlag(BuildFlags.Build64bit))
                        Win32.CopyIfNewer("bin\\Release64\\ProcessHacker.lib", "sdk\\lib\\amd64\\ProcessHacker.lib");
                }

                // Build the SDK
                HeaderGen.Execute();

                // Copy the resource header
                Win32.CopyIfNewer("ProcessHacker\\sdk\\phapppub.h", "sdk\\include\\phapppub.h");
                Win32.CopyIfNewer("ProcessHacker\\sdk\\phdk.h", "sdk\\include\\phdk.h");
                Win32.CopyIfNewer("ProcessHacker\\resource.h", "sdk\\include\\phappresource.h");

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

                if (File.Exists(BuildOutputFolder + "\\processhacker-build-setup.exe"))
                    File.Delete(BuildOutputFolder + "\\processhacker-build-setup.exe");

                File.Move(
                    "tools\\CustomSetupTool\\bin\\Release32\\CustomSetupTool.exe", 
                    BuildOutputFolder + "\\processhacker-build-setup.exe"
                    );

                if (File.Exists(BuildOutputFolder + "\\processhacker-build-bin.64"))
                    File.Delete(BuildOutputFolder + "\\processhacker-build-bin.64");

                Program.PrintColorMessage(new FileInfo(BuildOutputFolder + "\\processhacker-build-setup.exe").Length.ToPrettySize(), ConsoleColor.Green);
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

                Zip.CreateCompressedSdkFromFolder("sdk", BuildOutputFolder + "\\processhacker-build-sdk.zip");

                Program.PrintColorMessage(new FileInfo(BuildOutputFolder + "\\processhacker-build-sdk.zip").Length.ToPrettySize(), ConsoleColor.Green);
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
                if (File.Exists("bin\\Release32\\ProcessHacker.exe.settings.xml"))
                    File.Delete("bin\\Release32\\ProcessHacker.exe.settings.xml");
                if (File.Exists("bin\\Release64\\ProcessHacker.exe.settings.xml"))
                    File.Delete("bin\\Release64\\ProcessHacker.exe.settings.xml");

                if (Directory.Exists("bin\\Release32"))
                    File.Create("bin\\Release32\\ProcessHacker.exe.settings.xml").Dispose();
                if (Directory.Exists("bin\\Release64"))
                    File.Create("bin\\Release64\\ProcessHacker.exe.settings.xml").Dispose();
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

                if (File.Exists(BuildOutputFolder + "\\processhacker-build-bin.zip"))
                    File.Delete(BuildOutputFolder + "\\processhacker-build-bin.zip");

                Zip.CreateCompressedFolder(
                    "bin",
                    BuildOutputFolder + "\\processhacker-build-bin.zip"
                    );

                Program.PrintColorMessage(new FileInfo(BuildOutputFolder + "\\processhacker-build-bin.zip").Length.ToPrettySize(), ConsoleColor.Green);
            }
            catch (Exception ex)
            {
                Program.PrintColorMessage("[ERROR] " + ex, ConsoleColor.Red);
                return false;
            }

            try
            {
                VisualStudio.CreateOutputDirectory();

                if (File.Exists(BuildOutputFolder + "\\processhacker-build-bin.64"))
                    File.Delete(BuildOutputFolder + "\\processhacker-build-bin.64");

                File.WriteAllBytes(
                    BuildOutputFolder + "\\processhacker-build-bin.64",
                    Encoding.UTF8.GetBytes(Convert.ToBase64String(File.ReadAllBytes(BuildOutputFolder + "\\processhacker-build-bin.zip")))
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
                    BuildOutputFolder + "\\processhacker-build-pdb.zip"
                    );

                Program.PrintColorMessage(new FileInfo(BuildOutputFolder + "\\processhacker-build-pdb.zip").Length.ToPrettySize(), ConsoleColor.Green);
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

                if (File.Exists(BuildOutputFolder + "\\processhacker-build-checksums.txt"))
                    File.Delete(BuildOutputFolder + "\\processhacker-build-checksums.txt");
                File.WriteAllText(BuildOutputFolder + "\\processhacker-build-checksums.txt", sb.ToString());
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
                Program.PrintColorMessage(BuildTimeStamp(), ConsoleColor.DarkGray, false, Flags);
                Program.PrintColorMessage("Building " + Path.GetFileNameWithoutExtension(Solution) + " (", ConsoleColor.Cyan, false, Flags);
                Program.PrintColorMessage("x32", ConsoleColor.Green, false, Flags);
                Program.PrintColorMessage(")...", ConsoleColor.Cyan, true, Flags);

                if (Flags.HasFlag(BuildFlags.BuildApi))
                    compilerOptions.Append("PH_BUILD_API;");
                if (!string.IsNullOrEmpty(BuildCommit))
                    compilerOptions.Append($"PHAPP_VERSION_COMMITHASH=\"{BuildCommit.Substring(0, 7)}\";");
                if (!string.IsNullOrEmpty(BuildRevision))
                    compilerOptions.Append($"PHAPP_VERSION_REVISION=\"{BuildRevision}\";");
                if (!string.IsNullOrEmpty(BuildCount))
                    compilerOptions.Append($"PHAPP_VERSION_BUILD=\"{BuildCount}\"");

                int errorcode = Win32.CreateProcess(
                    msbuildExePath,
                    "/m /nologo /nodereuse:false /verbosity:quiet " +
                    "/p:Configuration=" + (Flags.HasFlag(BuildFlags.BuildDebug) ? "Debug " : "Release ") +
                    "/p:Platform=Win32 " +
                    "/p:ExternalCompilerOptions=\"" + compilerOptions.ToString() + "\" " +
                    Solution,
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
                Program.PrintColorMessage(BuildTimeStamp(), ConsoleColor.DarkGray, false, Flags);
                Program.PrintColorMessage("Building " + Path.GetFileNameWithoutExtension(Solution) + " (", ConsoleColor.Cyan, false, Flags);
                Program.PrintColorMessage("x64", ConsoleColor.Green, false, Flags);
                Program.PrintColorMessage(")...", ConsoleColor.Cyan, true, Flags);

                if (Flags.HasFlag(BuildFlags.BuildApi))
                    compilerOptions.Append("PH_BUILD_API;");
                if (!string.IsNullOrEmpty(BuildCommit))
                    compilerOptions.Append($"PHAPP_VERSION_COMMITHASH=\"{BuildCommit.Substring(0, 7)}\";");
                if (!string.IsNullOrEmpty(BuildRevision))
                    compilerOptions.Append($"PHAPP_VERSION_REVISION=\"{BuildRevision}\";");
                if (!string.IsNullOrEmpty(BuildCount))
                    compilerOptions.Append($"PHAPP_VERSION_BUILD=\"{BuildCount}\"");

                int errorcode = Win32.CreateProcess(
                    msbuildExePath,
                    "/m /nologo /nodereuse:false /verbosity:quiet " +
                    "/p:Configuration=" + (Flags.HasFlag(BuildFlags.BuildDebug) ? "Debug " : "Release ") +
                    "/p:Platform=x64 " +
                    "/p:ExternalCompilerOptions=\"" + compilerOptions.ToString() + "\" " +
                    Solution,
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

        public static bool BuildDeployUpdateConfig()
        {
            string buildBuildId;
            string buildJobId;
            string buildPostUrl;
            string buildPostApiKey;
            string buildPostSfUrl;
            string buildPostSfApiKey;
            string buildPostString;
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
            buildJobId = Win32.GetEnvironmentVariable("%APPVEYOR_JOB_ID%");
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

            if (string.IsNullOrEmpty(buildBuildId))
                return false;
            if (string.IsNullOrEmpty(buildJobId))
                return false;
            if (string.IsNullOrEmpty(buildPostUrl))
                return false;
            if (string.IsNullOrEmpty(buildPostApiKey))
                return false;
            if (string.IsNullOrEmpty(buildPostSfUrl))
                return false;
            if (string.IsNullOrEmpty(buildPostSfApiKey))
                return false;
            if (string.IsNullOrEmpty(BuildVersion))
                return false;

            Program.PrintColorMessage(Environment.NewLine + "Uploading build artifacts... " + BuildVersion, ConsoleColor.Cyan);

            if (BuildNightly && !File.Exists(Verify.GetPath("nightly.key")))
            {
                string buildKey = Win32.GetEnvironmentVariable("%NIGHTLY_BUILD_KEY%");

                if (!string.IsNullOrEmpty(buildKey))
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
                    "sign -k " + Verify.GetPath("nightly.key") + " " + BuildOutputFolder + "\\processhacker-build-bin.zip -h"
                    );
                BuildSetupSig = Win32.ShellExecute(
                    customSignToolPath,
                    "sign -k " + Verify.GetPath("nightly.key") + " " + BuildOutputFolder + "\\processhacker-build-setup.exe -h"
                    );
            }
            else
            {
                Program.PrintColorMessage("[SKIPPED] CustomSignTool not found.", ConsoleColor.Yellow);
                return true;
            }

            if (BuildNightly && File.Exists(Verify.GetPath("nightly.key")))
                File.Delete(Verify.GetPath("nightly.key"));

            if (string.IsNullOrEmpty(BuildBinSig))
            {
                Program.PrintColorMessage("build-bin.sig not found.", ConsoleColor.Red);
                return false;
            }
            if (string.IsNullOrEmpty(BuildSetupSig))
            {
                Program.PrintColorMessage("build-setup.sig not found.", ConsoleColor.Red);
                return false;
            }

            buildFilename = BuildOutputFolder + "\\processhacker-build-bin.zip";

            if (File.Exists(buildFilename))
            {
                buildBinFileLength = new FileInfo(buildFilename).Length;
                buildBinHash = Verify.HashFile(buildFilename);
            }

            if (string.IsNullOrEmpty(buildBinHash))
            {
                Program.PrintColorMessage("build-bin hash not found.", ConsoleColor.Red);
                return false;
            }

            buildFilename = BuildOutputFolder + "\\processhacker-build-setup.exe";

            if (File.Exists(buildFilename))
            {
                buildSetupFileLength = new FileInfo(buildFilename).Length;
                buildSetupHash = Verify.HashFile(buildFilename);
            }

            if (string.IsNullOrEmpty(buildSetupHash))
            {
                Program.PrintColorMessage("build-setup hash not found.", ConsoleColor.Red);
                return false;
            }

            try
            {
                buildPostString = JsonSerializer.Serialize(new BuildUpdateRequest
                {
                    BuildUpdated = TimeStart.ToString("o"),
                    BuildVersion = BuildVersion,
                    BuildCommit = BuildCommit,
                    BuildId = buildBuildId,
                    BinUrl = $"https://ci.appveyor.com/api/buildjobs/{buildJobId}/artifacts/processhacker-{BuildVersion}-bin.zip",
                    BinLength = buildBinFileLength.ToString(),
                    BinHash = buildBinHash,
                    BinSig = BuildBinSig,
                    SetupUrl = $"https://ci.appveyor.com/api/buildjobs/{buildJobId}/artifacts/processhacker-{BuildVersion}-setup.exe",
                    SetupLength = buildSetupFileLength.ToString(),
                    SetupHash = buildSetupHash,
                    SetupSig = BuildSetupSig,
                }, new JsonSerializerOptions { IgnoreNullValues = true, PropertyNameCaseInsensitive = true });

                if (string.IsNullOrEmpty(buildPostString))
                    return false;

                using (HttpClientHandler httpClientHandler = new HttpClientHandler())
                {
                    httpClientHandler.AutomaticDecompression = DecompressionMethods.All;

                    using (HttpClient httpClient = new HttpClient(httpClientHandler))
                    using (StringContent httpContent = new StringContent(buildPostString, Encoding.UTF8, "application/json"))
                    {
                        httpClient.DefaultRequestHeaders.Add("X-ApiKey", buildPostApiKey);

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
            catch (Exception ex)
            {
                Program.PrintColorMessage("[UpdateBuildWebService] " + ex, ConsoleColor.Red);
                return false;
            }

            try
            {
                using (HttpClientHandler httpClientHandler = new HttpClientHandler())
                {
                    httpClientHandler.AutomaticDecompression = DecompressionMethods.All;

                    using (HttpClient httpClient = new HttpClient(httpClientHandler))
                    using (StringContent httpContent = new StringContent(buildPostString, Encoding.UTF8, "application/json"))
                    {
                        httpClient.DefaultRequestHeaders.Add("X-ApiKey", buildPostSfApiKey);

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
            catch (Exception ex)
            {
                Program.PrintColorMessage("[UpdateBuildWebService-SF] " + ex, ConsoleColor.Red);
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

            if (string.IsNullOrEmpty(buildPostUrl))
                return false;
            if (string.IsNullOrEmpty(buildPostKey))
                return false;
            if (string.IsNullOrEmpty(buildPostName))
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

                        request = (FtpWebRequest)WebRequest.Create(buildPostUrl + filename);
                        request.Credentials = new NetworkCredential(buildPostKey, buildPostName);
                        request.Method = WebRequestMethods.Ftp.UploadFile;
                        request.Timeout = System.Threading.Timeout.Infinite;
                        request.EnableSsl = true;
                        request.UsePassive = true;
                        request.UseBinary = true;

                        Program.PrintColorMessage($"Uploading {filename}...", ConsoleColor.Cyan, true);

                        using (FileStream stream = File.OpenRead(sourceFile))
                        using (BufferedStream localStream = new BufferedStream(stream))
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

            try
            {
                foreach (BuildFile file in BuildConfig.Build_Release_Files)
                {
                    if (!file.UploadNightly)
                        continue;

                    string sourceFile = BuildOutputFolder + file.FileName;

                    if (File.Exists(sourceFile))
                    {
                        bool uploaded;
                        string fileName;

                        fileName = sourceFile.Replace("-build-", $"-{BuildVersion}-", StringComparison.OrdinalIgnoreCase);

                        File.Move(sourceFile, fileName, true);
                        uploaded = AppVeyor.UploadFile(fileName);
                        File.Move(fileName, sourceFile, true);

                        if (!uploaded)
                        {
                            Program.PrintColorMessage("[WebServiceAppveyorUploadFile]", ConsoleColor.Red);
                            return false;
                        }
                    }
                    else
                    {
                        Program.PrintColorMessage("[SKIPPED] missing file: " + sourceFile, ConsoleColor.Yellow);
                    }
                }
            }
            catch (Exception ex)
            {
                Program.PrintColorMessage("[WebServiceAppveyorPushArtifact] " + ex, ConsoleColor.Red);
                return false;
            }

            return true;
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
        //        BuildOutputFolder + "\\ProcessHacker-44.png",
        //        BuildOutputFolder + "\\ProcessHacker-50.png",
        //        BuildOutputFolder + "\\ProcessHacker-150.png"
        //    };
        //
        //    Program.PrintColorMessage("Building processhacker-build-package.appxbundle...", ConsoleColor.Cyan);
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
        //    if (string.IsNullOrEmpty(sdkRootPath))
        //    {
        //        Program.PrintColorMessage("[Skipped] Windows SDK", ConsoleColor.Red);
        //        return;
        //    }
        //
        //    if (!File.Exists(BuildOutputFolder + "\\processhacker-appx.cer "))
        //    {
        //        BuildAppxSignature();
        //    }
        //
        //    string makeAppxExePath = Environment.ExpandEnvironmentVariables(sdkRootPath + "\\x64\\MakeAppx.exe");
        //    string signToolExePath = Environment.ExpandEnvironmentVariables(sdkRootPath + "\\x64\\SignTool.exe");
        //
        //    try
        //    {
        //        //Win32.ImageResizeFile(44, "ProcessHacker\\resources\\ProcessHacker.png", BuildOutputFolder + "\\ProcessHacker-44.png");
        //        //Win32.ImageResizeFile(50, "ProcessHacker\\resources\\ProcessHacker.png", BuildOutputFolder + "\\ProcessHacker-50.png");
        //        //Win32.ImageResizeFile(150, "ProcessHacker\\resources\\ProcessHacker.png", BuildOutputFolder + "\\ProcessHacker-150.png");
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
        //            packageMap32.AppendLine("\"" + BuildOutputFolder + "\\ProcessHacker-44.png\" \"Assets\\ProcessHacker-44.png\"");
        //            packageMap32.AppendLine("\"" + BuildOutputFolder + "\\ProcessHacker-50.png\" \"Assets\\ProcessHacker-50.png\"");
        //            packageMap32.AppendLine("\"" + BuildOutputFolder + "\\ProcessHacker-150.png\" \"Assets\\ProcessHacker-150.png\"");
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
        //                BuildOutputFolder + "\\processhacker-build-package-x32.appx"
        //                );
        //            Program.PrintColorMessage(Environment.NewLine + error, ConsoleColor.Gray, true, Flags);
        //
        //            // sign the package
        //            error = Win32.ShellExecute(
        //                signToolExePath,
        //                "sign /v /fd SHA256 /a /f " + BuildOutputFolder + "\\processhacker-appx.pfx /td SHA256 " +
        //                BuildOutputFolder + "\\processhacker-build-package-x32.appx"
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
        //            packageMap64.AppendLine("\"" + BuildOutputFolder + "\\ProcessHacker-44.png\" \"Assets\\ProcessHacker-44.png\"");
        //            packageMap64.AppendLine("\"" + BuildOutputFolder + "\\ProcessHacker-50.png\" \"Assets\\ProcessHacker-50.png\"");
        //            packageMap64.AppendLine("\"" + BuildOutputFolder + "\\ProcessHacker-150.png\" \"Assets\\ProcessHacker-150.png\"");
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
        //                BuildOutputFolder + "\\processhacker-build-package-x64.appx"
        //                );
        //            Program.PrintColorMessage(Environment.NewLine + error, ConsoleColor.Gray, true, Flags);
        //
        //            // sign the package
        //            error = Win32.ShellExecute(
        //                signToolExePath,
        //                "sign /v /fd SHA256 /a /f " + BuildOutputFolder + "\\processhacker-appx.pfx /td SHA256 " +
        //                BuildOutputFolder + "\\processhacker-build-package-x64.appx"
        //                );
        //            Program.PrintColorMessage(Environment.NewLine + error, ConsoleColor.Gray, true, Flags);
        //        }
        //
        //        {
        //            // create the appx bundle map
        //            StringBuilder bundleMap = new StringBuilder(0x100);
        //            bundleMap.AppendLine("[Files]");
        //            bundleMap.AppendLine("\"" + BuildOutputFolder + "\\processhacker-build-package-x32.appx\" \"processhacker-build-package-x32.appx\"");
        //            bundleMap.AppendLine("\"" + BuildOutputFolder + "\\processhacker-build-package-x64.appx\" \"processhacker-build-package-x64.appx\"");
        //            File.WriteAllText(BuildOutputFolder + "\\bundle.map", bundleMap.ToString());
        //
        //            if (File.Exists(BuildOutputFolder + "\\processhacker-build-package.appxbundle"))
        //                File.Delete(BuildOutputFolder + "\\processhacker-build-package.appxbundle");
        //
        //            // create the appx bundle package
        //            var error = Win32.ShellExecute(
        //                makeAppxExePath,
        //                "bundle /f " + BuildOutputFolder + "\\bundle.map /p " +
        //                BuildOutputFolder + "\\processhacker-build-package.appxbundle"
        //                );
        //            Program.PrintColorMessage(Environment.NewLine + error, ConsoleColor.Gray, true, Flags);
        //
        //            // sign the appx bundle package
        //            error = Win32.ShellExecute(
        //                signToolExePath,
        //                "sign /v /fd SHA256 /a /f " + BuildOutputFolder + "\\processhacker-appx.pfx /td SHA256 " +
        //                BuildOutputFolder + "\\processhacker-build-package.appxbundle"
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
        //        BuildOutputFolder + "\\processhacker-appx.pvk",
        //        BuildOutputFolder + "\\processhacker-appx.cer",
        //        BuildOutputFolder + "\\processhacker-appx.pfx"
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
        //    if (string.IsNullOrEmpty(sdkRootPath))
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
        //            "\"CN=ProcessHacker, O=ProcessHacker, C=AU\" " +
        //            "/r /h 0 " +
        //            "/eku \"1.3.6.1.5.5.7.3.3,1.3.6.1.4.1.311.10.3.13\" " +
        //            "/sv " +
        //            BuildOutputFolder + "\\processhacker-appx.pvk " +
        //            BuildOutputFolder + "\\processhacker-appx.cer "
        //            );
        //
        //        if (!string.IsNullOrEmpty(output) && !output.Equals("Succeeded", StringComparison.OrdinalIgnoreCase))
        //        {
        //            Program.PrintColorMessage("[MakeCert] " + output, ConsoleColor.Red);
        //            return false;
        //        }
        //
        //        output = Win32.ShellExecute(pvk2PfxExePath,
        //            "/pvk " + BuildOutputFolder + "\\processhacker-appx.pvk " +
        //            "/spc " + BuildOutputFolder + "\\processhacker-appx.cer " +
        //            "/pfx " + BuildOutputFolder + "\\processhacker-appx.pfx "
        //            );
        //
        //        if (!string.IsNullOrEmpty(output))
        //        {
        //            Program.PrintColorMessage("[Pvk2Pfx] " + output, ConsoleColor.Red);
        //            return false;
        //        }
        //
        //        output = Win32.ShellExecute(certUtilExePath,
        //            "-addStore TrustedPeople " + BuildOutputFolder + "\\processhacker-appx.cer"
        //            );
        //
        //        if (!string.IsNullOrEmpty(output) && !output.Contains("command completed successfully", StringComparison.OrdinalIgnoreCase))
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
        //            if (c.Subject.Equals("CN=ProcessHacker, O=ProcessHacker, C=AU", StringComparison.OrdinalIgnoreCase))
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
