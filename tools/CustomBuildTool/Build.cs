/*
 * Process Hacker Toolchain - 
 *   Build script
 * 
 * Copyright (C) 2017-2018 dmex
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

namespace CustomBuildTool
{
    public static class Build
    {
        private static DateTime TimeStart;
        private static bool BuildNightly;
        private static string GitExePath;
        private static string MSBuildExePath;
        private static string CustomSignToolPath;

        private static string BuildBranch;
        private static string BuildOutputFolder;
        private static string BuildCommit;
        private static string BuildVersion;
        private static string BuildLongVersion;
        private static string BuildCount;
        private static string BuildRevision;
        //private static string BuildMessage;

        private static long BuildBinFileLength;
        private static string BuildBinHash;
        private static string BuildBinSig;
        private static long BuildSetupFileLength;
        private static string BuildSetupHash;
        private static string BuildSetupSig;

        #region Build Config
        private static readonly string[] Build_Release_Files =
        {
            "\\processhacker-build-checksums.txt",
            "\\processhacker-build-setup.exe",
            "\\processhacker-build-bin.zip",
            "\\processhacker-build-src.zip",
            "\\processhacker-build-sdk.zip",
            "\\processhacker-build-pdb.zip"
        };
        private static readonly string[] Build_Nightly_Files =  
        {
            //"\\processhacker-build-checksums.txt",
            "\\processhacker-build-setup.exe",
            "\\processhacker-build-bin.zip",
            //"\\processhacker-build-src.zip",
            //"\\processhacker-build-sdk.zip",
            //"\\processhacker-build-pdb.zip"
        };

        private static readonly string[] sdk_directories =
        {
            "sdk",
            "sdk\\include",
            "sdk\\dbg\\amd64",
            "sdk\\dbg\\i386",
            "sdk\\lib\\amd64",
            "sdk\\lib\\i386",
            "sdk\\samples\\SamplePlugin",
            "sdk\\samples\\SamplePlugin\\bin\\Release32"
        };

        private static readonly string[] phnt_headers =
        {
            "ntdbg.h",
            "ntexapi.h",
            "ntgdi.h",
            "ntioapi.h",
            "ntkeapi.h",
            "ntldr.h",
            "ntlpcapi.h",
            "ntmisc.h",
            "ntmmapi.h",
            "ntnls.h",
            "ntobapi.h",
            "ntpebteb.h",
            "ntpfapi.h",
            "ntpnpapi.h",
            "ntpoapi.h",
            "ntpsapi.h",
            "ntregapi.h",
            "ntrtl.h",
            "ntsam.h",
            "ntseapi.h",
            "nttmapi.h",
            "nttp.h",
            "ntwow64.h",
            "ntxcapi.h",
            "ntzwapi.h",
            "phnt.h",
            "phnt_ntdef.h",
            "phnt_windows.h",
            "subprocesstag.h",
            "winsta.h"
        };

        private static readonly string[] phlib_headers =
        {
            "appresolver.h",
            "circbuf.h",
            "circbuf_h.h",
            "cpysave.h",
            "dltmgr.h",
            "dspick.h",
            "emenu.h",
            "exlf.h",
            "fastlock.h",
            "filestream.h",
            "graph.h",
            "guisup.h",
            "hexedit.h",
            "hndlinfo.h",
            "json.h",
            "kphapi.h",
            "kphuser.h",
            "lsasup.h",
            "mapimg.h",
            "ph.h",
            "phbase.h",
            "phbasesup.h",
            "phconfig.h",
            "phdata.h",
            "phnative.h",
            "phnativeinl.h",
            "phnet.h",
            "phsup.h",
            "phutil.h",
            "provider.h",
            "queuedlock.h",
            "ref.h",
            "secedit.h",
            "settings.h",
            "svcsup.h",
            "symprv.h",
            "templ.h",
            "treenew.h",
            "verify.h",
            "workqueue.h"
        };
        #endregion

        public static bool InitializeBuildEnvironment()
        {
            TimeStart = DateTime.Now;

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

            BuildOutputFolder = VisualStudio.GetOutputDirectoryPath();
            MSBuildExePath = VisualStudio.GetMsbuildFilePath();
            CustomSignToolPath = VisualStudio.GetCustomSignToolFilePath();
            GitExePath = VisualStudio.GetGitFilePath();
            BuildNightly = !string.Equals(Environment.ExpandEnvironmentVariables("%APPVEYOR_BUILD_API%"), "%APPVEYOR_BUILD_API%", StringComparison.OrdinalIgnoreCase);
            
            if (!File.Exists(MSBuildExePath))
            {
                Program.PrintColorMessage("Unable to find MsBuild.", ConsoleColor.Red);
                return false;
            }

            var instance = VisualStudio.GetVisualStudioInstance();

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

            return true;
        }

        public static void CleanupBuildEnvironment()
        {
            try
            {
                foreach (string file in Build_Release_Files)
                {
                    string sourceFile = BuildOutputFolder + file;

                    if (File.Exists(sourceFile))
                        File.Delete(sourceFile);
                }
            }
            catch (Exception ex)
            {
                Program.PrintColorMessage("[Cleanup] " + ex, ConsoleColor.Red);
            }

            try
            {
                foreach (string folder in sdk_directories)
                {
                    if (Directory.Exists(folder))
                    {
                        Directory.Delete(folder, true);
                    }
                }
            }
            catch (Exception ex)
            {
                Program.PrintColorMessage("[Cleanup SDK] " + ex, ConsoleColor.Red);
            }
        }

        public static void ShowBuildEnvironment(bool ShowBuildInfo)
        {
            if (File.Exists(GitExePath))
            {
                string currentGitDir;
                string currentGitTag;

                currentGitDir = VisualStudio.GetGitWorkPath(Environment.CurrentDirectory);
                BuildBranch = Win32.ShellExecute(GitExePath, currentGitDir + "rev-parse --abbrev-ref HEAD").Trim();
                BuildCommit = Win32.ShellExecute(GitExePath, currentGitDir + "rev-parse HEAD").Trim();

                currentGitTag = Win32.ShellExecute(GitExePath, currentGitDir + "describe --abbrev=0 --tags --always").Trim();
                BuildRevision = Win32.ShellExecute(GitExePath, currentGitDir + "rev-list --count \"" + currentGitTag + ".." + BuildBranch + "\"").Trim();
                BuildCount = Win32.ShellExecute(GitExePath, currentGitDir + "rev-list --count " + BuildBranch).Trim();
            }

            BuildVersion = "3.0." + (string.IsNullOrWhiteSpace(BuildRevision) ? "0" : BuildRevision);
            BuildLongVersion = "3.0." + (string.IsNullOrWhiteSpace(BuildCount) ? "0" : BuildCount) + "." + (string.IsNullOrWhiteSpace(BuildRevision) ? "0" : BuildRevision);

            if (ShowBuildInfo)
            {
                //Program.PrintColorMessage("BuildTools: ", ConsoleColor.DarkGray, false);
                //Program.PrintColorMessage("1.0 ", ConsoleColor.Green, true);
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

                Program.PrintColorMessage("Git: ", ConsoleColor.DarkGray, false);
                if (File.Exists(GitExePath))
                {
                    var info = Win32.ShellExecute(GitExePath, "version").Trim();
                    Program.PrintColorMessage(info.Substring("git version ".Length), ConsoleColor.Green, true);
                }
                else
                {
                    Program.PrintColorMessage("Not installed.", ConsoleColor.Yellow);
                }

                Program.PrintColorMessage(Environment.NewLine + "Building... ", ConsoleColor.DarkGray, false);
                Program.PrintColorMessage(BuildLongVersion, ConsoleColor.Green, false);

                if (!string.IsNullOrEmpty(BuildCommit))
                {
                    Program.PrintColorMessage(" (", ConsoleColor.DarkGray, false);
                    Program.PrintColorMessage(BuildCommit.Substring(0, 8), ConsoleColor.DarkYellow, false);
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
            Program.PrintColorMessage(" second(s) " + Environment.NewLine, ConsoleColor.DarkGray, true);
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

        public static bool CopyLibFiles(BuildFlags Flags)
        {
            try
            {
                if (Flags.HasFlag(BuildFlags.BuildDebug))
                {
                    if (Flags.HasFlag(BuildFlags.Build32bit))
                    {
                        Win32.CopyIfNewer("bin\\Debug32\\ProcessHacker.lib", "sdk\\lib\\i386\\ProcessHacker.lib");
                    }

                    if (Flags.HasFlag(BuildFlags.Build64bit))
                    {
                        Win32.CopyIfNewer("bin\\Debug64\\ProcessHacker.lib", "sdk\\lib\\amd64\\ProcessHacker.lib");
                    }
                }
                else
                {
                    if (Flags.HasFlag(BuildFlags.Build32bit))
                    {
                        Win32.CopyIfNewer("bin\\Release32\\ProcessHacker.lib", "sdk\\lib\\i386\\ProcessHacker.lib");
                    }

                    if (Flags.HasFlag(BuildFlags.Build64bit))
                    {
                        Win32.CopyIfNewer("bin\\Release64\\ProcessHacker.lib", "sdk\\lib\\amd64\\ProcessHacker.lib");
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

        public static bool CopyPluginSdkHeaders()
        {
            try
            {
                foreach (string folder in sdk_directories)
                {
                    if (!Directory.Exists(folder))
                    {
                        Directory.CreateDirectory(folder);
                    }
                }

                Win32.CopyIfNewer("phlib\\mxml\\mxml.h", "sdk\\include\\mxml.h");

                // Copy the plugin SDK headers
                foreach (string file in phnt_headers)
                    Win32.CopyIfNewer("phnt\\include\\" + file, "sdk\\include\\" + file);
                foreach (string file in phlib_headers)
                    Win32.CopyIfNewer("phlib\\include\\" + file, "sdk\\include\\" + file);

                // Copy readme
                Win32.CopyIfNewer("ProcessHacker\\sdk\\readme.txt", "sdk\\readme.txt");
                // Copy symbols
                Win32.CopyIfNewer("bin\\Release32\\ProcessHacker.pdb", "sdk\\dbg\\i386\\ProcessHacker.pdb");
                Win32.CopyIfNewer("bin\\Release64\\ProcessHacker.pdb", "sdk\\dbg\\amd64\\ProcessHacker.pdb");
                Win32.CopyIfNewer("KProcessHacker\\bin\\i386\\kprocesshacker.pdb", "sdk\\dbg\\i386\\kprocesshacker.pdb");
                Win32.CopyIfNewer("KProcessHacker\\bin\\amd64\\kprocesshacker.pdb", "sdk\\dbg\\amd64\\kprocesshacker.pdb");
                // Copy libs
                Win32.CopyIfNewer("bin\\Release32\\ProcessHacker.lib", "sdk\\lib\\i386\\ProcessHacker.lib");
                Win32.CopyIfNewer("bin\\Release64\\ProcessHacker.lib", "sdk\\lib\\amd64\\ProcessHacker.lib");
                // Copy sample plugin
                //Win32.CopyIfNewer("plugins\\SamplePlugin\\main.c", "sdk\\samples\\SamplePlugin\\main.c");
                //Win32.CopyIfNewer("plugins\\SamplePlugin\\SamplePlugin.sln", "sdk\\samples\\SamplePlugin\\SamplePlugin.sln");
                //Win32.CopyIfNewer("plugins\\SamplePlugin\\SamplePlugin.vcxproj", "sdk\\samples\\SamplePlugin\\SamplePlugin.vcxproj");
                //Win32.CopyIfNewer("plugins\\SamplePlugin\\SamplePlugin.vcxproj.filters", "sdk\\samples\\SamplePlugin\\SamplePlugin.vcxproj.filters");
                //Win32.CopyIfNewer("plugins\\SamplePlugin\\bin\\Release32\\SamplePlugin.dll", "sdk\\samples\\SamplePlugin\\bin\\Release32\\SamplePlugin.dll");
            }
            catch (Exception ex)
            {
                Program.PrintColorMessage("[ERROR] " + ex, ConsoleColor.Red);
                return false;
            }

            return true;
        }

        public static bool CopyVersionHeader()
        {
            try
            {
                HeaderGen gen = new HeaderGen();
                gen.Execute();

                Win32.CopyIfNewer("ProcessHacker\\sdk\\phapppub.h", "sdk\\include\\phapppub.h");
                Win32.CopyIfNewer("ProcessHacker\\sdk\\phdk.h", "sdk\\include\\phdk.h");
                Win32.CopyIfNewer("ProcessHacker\\resource.h", "sdk\\include\\phappresource.h");
                
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
                    {
                        Win32.CopyIfNewer(
                            "ProcessHacker\\resources\\etwguids.txt",
                            "bin\\Debug32\\etwguids.txt"
                            );
                    }

                    if (Flags.HasFlag(BuildFlags.Build64bit))
                    {
                        Win32.CopyIfNewer(
                            "ProcessHacker\\resources\\etwguids.txt",
                            "bin\\Debug64\\etwguids.txt"
                            );
                    }
                }
                else
                {
                    if (Flags.HasFlag(BuildFlags.Build32bit))
                    {
                        Win32.CopyIfNewer(
                            "ProcessHacker\\resources\\etwguids.txt",
                            "bin\\Release32\\etwguids.txt"
                            );
                    }

                    if (Flags.HasFlag(BuildFlags.Build64bit))
                    {
                        Win32.CopyIfNewer(
                            "ProcessHacker\\resources\\etwguids.txt",
                            "bin\\Release64\\etwguids.txt"
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

        public static bool FixupResourceHeader()
        {
            try
            {
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

        public static bool BuildPublicHeaderFiles()
        {
            Program.PrintColorMessage(BuildTimeStamp(), ConsoleColor.DarkGray, false);
            Program.PrintColorMessage("Building public SDK headers...", ConsoleColor.Cyan);

            try
            {
                HeaderGen gen = new HeaderGen();
                gen.Execute();
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
            try
            {
                if (Flags.HasFlag(BuildFlags.BuildDebug))
                {
                    if (Directory.Exists("bin\\Debug32"))
                    {
                        Win32.CopyIfNewer("KProcessHacker\\bin-signed\\i386\\kprocesshacker.sys", "bin\\Debug32\\kprocesshacker.sys");
                    }

                    if (Directory.Exists("bin\\Debug64"))
                    {
                        Win32.CopyIfNewer("KProcessHacker\\bin-signed\\amd64\\kprocesshacker.sys", "bin\\Debug64\\kprocesshacker.sys");
                    }
                }
                else
                {
                    if (Directory.Exists("bin\\Release32"))
                    {
                        Win32.CopyIfNewer("KProcessHacker\\bin-signed\\i386\\kprocesshacker.sys", "bin\\Release32\\kprocesshacker.sys");
                    }

                    if (Directory.Exists("bin\\Release64"))
                    {
                        Win32.CopyIfNewer("KProcessHacker\\bin-signed\\amd64\\kprocesshacker.sys", "bin\\Release64\\kprocesshacker.sys");
                    }
                }
            }
            catch (Exception ex)
            {
                Program.PrintColorMessage("[ERROR] (kprocesshacker.sys)" + ex, ConsoleColor.Red);
            }

            if (!File.Exists(CustomSignToolPath))
                return true;

            if (BuildNightly && !File.Exists("build\\kph.key"))
            {
                string kphKey = Environment.ExpandEnvironmentVariables("%KPH_BUILD_KEY%").Replace("%KPH_BUILD_KEY%", string.Empty, StringComparison.OrdinalIgnoreCase);

                if (!string.IsNullOrEmpty(kphKey))
                {
                    Verify.Decrypt("build\\kph.s", "build\\kph.key", kphKey);
                }

                if (!File.Exists("build\\kph.key"))
                {
                    Program.PrintColorMessage("[SKIPPED] kph.key not found.", ConsoleColor.Yellow);
                    return true;
                }
            }

            if ((Flags & BuildFlags.Build32bit) == BuildFlags.Build32bit)
            {
                if (File.Exists("bin\\Debug32\\ProcessHacker.exe"))
                {
                    File.WriteAllText("bin\\Debug32\\ProcessHacker.sig", string.Empty);
                    Win32.ShellExecute(CustomSignToolPath, "sign -k build\\kph.key bin\\Debug32\\ProcessHacker.exe -s bin\\Debug32\\ProcessHacker.sig");
                }

                if (File.Exists("bin\\Debug64\\ProcessHacker.exe"))
                {
                    File.WriteAllText("bin\\Debug64\\ProcessHacker.sig", string.Empty);
                    Win32.ShellExecute(CustomSignToolPath, "sign -k build\\kph.key bin\\Debug64\\ProcessHacker.exe -s bin\\Debug64\\ProcessHacker.sig");
                }
            }

            if ((Flags & BuildFlags.Build64bit) == BuildFlags.Build64bit)
            {
                if (File.Exists("bin\\Release32\\ProcessHacker.exe"))
                {
                    File.WriteAllText("bin\\Release32\\ProcessHacker.sig", string.Empty);
                    Win32.ShellExecute(CustomSignToolPath, "sign -k build\\kph.key bin\\Release32\\ProcessHacker.exe -s bin\\Release32\\ProcessHacker.sig");
                }

                if (File.Exists("bin\\Release64\\ProcessHacker.exe"))
                {
                    File.WriteAllText("bin\\Release64\\ProcessHacker.sig", string.Empty);
                    Win32.ShellExecute(CustomSignToolPath, "sign -k build\\kph.key bin\\Release64\\ProcessHacker.exe -s bin\\Release64\\ProcessHacker.sig");
                }
            }

            if (BuildNightly && File.Exists("build\\kph.key"))
                File.Delete("build\\kph.key");

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

            try
            {
                //var webSetupVersion = System.Diagnostics.FileVersionInfo.GetVersionInfo(BuildOutputFolder + "\\processhacker-build-websetup.exe");
                //BuildWebSetupVersion = webSetupVersion.FileVersion.Replace(",", ".");
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
            //PrintColorMessage("Copying Plugin SDK...", ConsoleColor.Cyan);

            if (!CopyTextFiles())
                return false;
            if (!CopyPluginSdkHeaders())
                return false;
            if (!CopyVersionHeader())
                return false;
            if (!FixupResourceHeader())
                return false;
            if (!CopyLibFiles(Flags))
                return false;

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
            catch { }

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
            catch { }

            try
            {
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

        public static bool BuildSrcZip()
        {
            Program.PrintColorMessage(BuildTimeStamp(), ConsoleColor.DarkGray, false);
            Program.PrintColorMessage("Building build-src.zip... ", ConsoleColor.Cyan, false);

            if (!File.Exists(GitExePath))
            {
                Program.PrintColorMessage("[SKIPPED] Git not installed.", ConsoleColor.Yellow);
                return false;
            }

            try
            {
                if (File.Exists(BuildOutputFolder + "\\processhacker-build-src.zip"))
                    File.Delete(BuildOutputFolder + "\\processhacker-build-src.zip");

                string output = Win32.ShellExecute(
                    GitExePath,
                    "--git-dir=.git " +
                    "--work-tree=.\\ archive " +
                    "--format zip " +
                    "--output " + BuildOutputFolder + "\\processhacker-build-src.zip " +
                    BuildBranch
                    );

                if (!string.IsNullOrEmpty(output))
                {
                    Program.PrintColorMessage("[ERROR] " + output, ConsoleColor.Red);
                    return false;
                }

                Program.PrintColorMessage(new FileInfo(BuildOutputFolder + "\\processhacker-build-src.zip").Length.ToPrettySize(), ConsoleColor.Green);
            }
            catch (Exception ex)
            {
                Program.PrintColorMessage("[ERROR] " + ex, ConsoleColor.Red);
                return false;
            }

            return true;
        }

        public static bool BuildPdbZip()
        {
            Program.PrintColorMessage(BuildTimeStamp(), ConsoleColor.DarkGray, false);
            Program.PrintColorMessage("Building build-pdb.zip... ", ConsoleColor.Cyan, false);

            try
            {
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
            Program.PrintColorMessage("Building build-checksums.txt...", ConsoleColor.Cyan);

            try
            {
                StringBuilder sb = new StringBuilder();

                foreach (string name in Build_Release_Files)
                {
                    string file = BuildOutputFolder + name;

                    if (File.Exists(file))
                    {
                        var info = new FileInfo(file);
                        var hash = Verify.HashFile(file);

                        if (name.Equals("\\processhacker-build-setup.exe", StringComparison.OrdinalIgnoreCase))
                        {
                            BuildSetupFileLength = info.Length;
                            BuildSetupHash = hash;
                        }

                        if (name.Equals("\\processhacker-build-bin.zip", StringComparison.OrdinalIgnoreCase))
                        {
                            BuildBinFileLength = info.Length;
                            BuildBinHash = hash;
                        }

                        sb.AppendLine(info.Name);
                        sb.AppendLine("SHA256: " + hash + Environment.NewLine);
                    }
                }

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

        public static bool BuildUpdateSignature()
        {
            Program.PrintColorMessage(BuildTimeStamp(), ConsoleColor.DarkGray, false);
            Program.PrintColorMessage("Building release signatures...", ConsoleColor.Cyan);

            if (!File.Exists(CustomSignToolPath))
            {
                Program.PrintColorMessage("[SKIPPED] CustomSignTool not found.", ConsoleColor.Yellow);
                return true;
            }

            if (BuildNightly && !File.Exists("build\\nightly.key"))
            {
                string buildKey = Environment.ExpandEnvironmentVariables("%NIGHTLY_BUILD_KEY%").Replace("%NIGHTLY_BUILD_KEY%", string.Empty, StringComparison.OrdinalIgnoreCase);

                if (!string.IsNullOrEmpty(buildKey))
                {
                    Verify.Decrypt("build\\nightly.s", "build\\nightly.key", buildKey);
                }

                if (!File.Exists("build\\nightly.key"))
                {
                    Program.PrintColorMessage("[SKIPPED] nightly.key not found.", ConsoleColor.Yellow);
                    return true;
                }
            }

            foreach (string file in Build_Nightly_Files)
            {
                string name = BuildOutputFolder + file;

                if (!File.Exists(name))
                {
                    Program.PrintColorMessage("[SKIPPED] " + name +" not found.", ConsoleColor.Yellow);
                    return false;
                }
            }

            BuildBinSig = Win32.ShellExecute(
                CustomSignToolPath,
                "sign -k build\\nightly.key " + BuildOutputFolder + "\\processhacker-build-bin.zip -h"
                );
            BuildSetupSig = Win32.ShellExecute(
                CustomSignToolPath,
                "sign -k build\\nightly.key " + BuildOutputFolder + "\\processhacker-build-setup.exe -h"
                );

            if (BuildNightly && File.Exists("build\\nightly.key"))
                File.Delete("build\\nightly.key");

            return true;
        }

        //public static bool BuildRenameReleaseFiles()
        //{
        //    try
        //    {
        //        foreach (string file in Build_Release_Files)
        //        {
        //            string sourceFile = BuildOutputFolder + file;
        //            string destinationFile = BuildOutputFolder + file.Replace("-build-", $"-{BuildVersion}-");
        //
        //            if (File.Exists(destinationFile))
        //                File.Delete(destinationFile);
        //
        //            if (File.Exists(sourceFile))
        //                File.Move(sourceFile, destinationFile);
        //        }
        //    }
        //    catch (Exception ex)
        //    {
        //        Program.PrintColorMessage("[WebServiceUploadBuild] " + ex, ConsoleColor.Red);
        //        return false;
        //    }
        //
        //    return true;
        //}

        public static bool BuildSolution(string Solution, BuildFlags Flags)
        {
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
                    compilerOptions.Append($"PHAPP_VERSION_COMMITHASH=\"{BuildCommit.Substring(0, 8)}\";");
                if (!string.IsNullOrEmpty(BuildRevision))
                    compilerOptions.Append($"PHAPP_VERSION_REVISION=\"{BuildRevision}\";");
                if (!string.IsNullOrEmpty(BuildCount))
                    compilerOptions.Append($"PHAPP_VERSION_BUILD=\"{BuildCount}\"");

                string error32 = Win32.ShellExecute(
                    MSBuildExePath,
                    "/m /nologo /verbosity:quiet " +
                    "/p:Configuration=" + (Flags.HasFlag(BuildFlags.BuildDebug) ? "Debug " : "Release ") +
                    "/p:Platform=Win32 " +
                    "/p:ExternalCompilerOptions=\"" + compilerOptions.ToString() + "\" " +
                    Solution
                    );
                
                if (!string.IsNullOrEmpty(error32))
                {
                    Program.PrintColorMessage("[ERROR] " + error32, ConsoleColor.Red, true, Flags | BuildFlags.BuildVerbose);
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
                    compilerOptions.Append($"PHAPP_VERSION_COMMITHASH=\"{BuildCommit.Substring(0, 8)}\";");
                if (!string.IsNullOrEmpty(BuildRevision))
                    compilerOptions.Append($"PHAPP_VERSION_REVISION=\"{BuildRevision}\";");
                if (!string.IsNullOrEmpty(BuildCount))
                    compilerOptions.Append($"PHAPP_VERSION_BUILD=\"{BuildCount}\"");

                string error64 = Win32.ShellExecute(
                    MSBuildExePath,
                    "/m /nologo /verbosity:quiet " +
                    "/p:Configuration=" + (Flags.HasFlag(BuildFlags.BuildDebug) ? "Debug " : "Release ") +
                    "/p:Platform=x64 " +
                    "/p:ExternalCompilerOptions=\"" + compilerOptions.ToString() + "\" " +
                    Solution
                    );

                if (!string.IsNullOrEmpty(error64))
                {
                    Program.PrintColorMessage("[ERROR] " + error64, ConsoleColor.Red, true, Flags | BuildFlags.BuildVerbose);
                    return false;
                }
            }

            return true;
        }

        public static bool BuildDeployUpdateConfig()
        {
            string buildJobId;
            string buildPostUrl;
            string buildPostApiKey;
            string buildChangelog;
            string buildSummary;
            string buildMessage;
            string buildPostString;

            if (!BuildNightly)
                return true;

            buildJobId = Environment.ExpandEnvironmentVariables("%APPVEYOR_JOB_ID%").Replace("%APPVEYOR_JOB_ID%", string.Empty, StringComparison.OrdinalIgnoreCase);
            buildPostUrl = Environment.ExpandEnvironmentVariables("%APPVEYOR_BUILD_API%").Replace("%APPVEYOR_BUILD_API%", string.Empty, StringComparison.OrdinalIgnoreCase);
            buildPostApiKey = Environment.ExpandEnvironmentVariables("%APPVEYOR_BUILD_KEY%").Replace("%APPVEYOR_BUILD_KEY%", string.Empty, StringComparison.OrdinalIgnoreCase);

            if (string.IsNullOrEmpty(buildJobId))
                return false;
            if (string.IsNullOrEmpty(buildPostUrl))
                return false;
            if (string.IsNullOrEmpty(buildPostApiKey))
                return false;
            if (string.IsNullOrEmpty(BuildVersion))
                return false;
            if (string.IsNullOrEmpty(BuildSetupHash))
                return false;
            if (string.IsNullOrEmpty(BuildSetupSig))
                return false;
            if (string.IsNullOrEmpty(BuildBinHash))
                return false;
            if (string.IsNullOrEmpty(BuildBinSig))
                return false;

            string currentGitDir = VisualStudio.GetGitWorkPath(Environment.CurrentDirectory);
            buildChangelog = Win32.ShellExecute(GitExePath, currentGitDir + "log -n 30 --date=format:%Y-%m-%d --pretty=format:\"[%cd] %s (%an)\"");
            buildSummary = Win32.ShellExecute(GitExePath, currentGitDir + "log -n 5 --date=format:%Y-%m-%d --pretty=format:\"[%cd] %s (%an)\" --abbrev-commit");
            buildMessage = Win32.ShellExecute(GitExePath, currentGitDir + "log -1 --pretty=%B");
            // log -n 5 --date=format:%Y-%m-%d --pretty=format:\"%C(green)[%cd]%Creset %C(bold blue)%an%Creset %<(65,trunc)%s%Creset %C(#696969)(%Creset%C(yellow)%h%Creset%C(#696969))%Creset\" --abbrev-commit
            // log -n 5 --date=format:%Y-%m-%d --pretty=format:\"[%cd] %an %s\" --abbrev-commit
            // log -n 1 --date=format:%Y-%m-%d --pretty=format:\"[%cd] %an: %<(65,trunc)%s (%h)\" --abbrev-commi
            // log -n 1800 --graph --pretty=format:\"%C(yellow)%h%Creset %C(bold blue)%an%Creset %s %C(dim green)(%cr)\" --abbrev-commit

            buildPostString = Json<BuildUpdateRequest>.Serialize(new BuildUpdateRequest
            {
                BuildUpdated = TimeStart.ToString("o"),
                BuildVersion = BuildVersion,
                BuildCommit = BuildCommit,
                BuildMessage = buildMessage,

                BinUrl = $"https://ci.appveyor.com/api/buildjobs/{buildJobId}/artifacts/processhacker-build-bin.zip",
                BinLength = BuildBinFileLength.ToString(),
                BinHash = BuildBinHash,
                BinSig = BuildBinSig,

                SetupUrl = $"https://ci.appveyor.com/api/buildjobs/{buildJobId}/artifacts/processhacker-build-setup.exe",
                SetupLength = BuildSetupFileLength.ToString(),
                SetupHash = BuildSetupHash,
                SetupSig = BuildSetupSig,

                Message = buildSummary,
                Changelog = buildChangelog
            });

            if (string.IsNullOrEmpty(buildPostString))
                return false;

            Program.PrintColorMessage(Environment.NewLine + "Updating Build WebService... " + BuildVersion, ConsoleColor.Cyan);

            try
            {
                using (HttpClientHandler httpClientHandler = new HttpClientHandler())
                {
                    httpClientHandler.AutomaticDecompression = DecompressionMethods.All;
                    httpClientHandler.ServerCertificateCustomValidationCallback = (message, cert, chain, sslPolicyErrors) =>
                    {
                        // Allow this client to communicate with authenticated servers.
                        if (sslPolicyErrors == System.Net.Security.SslPolicyErrors.None)
                            return true;

                        // Temporarily ignore wj32.org expired certificate.
                        if (string.Equals(cert.GetCertHashString(System.Security.Cryptography.HashAlgorithmName.SHA1), "b60cb3b6aac5f59075689fc3c7dfd561750ce100", StringComparison.OrdinalIgnoreCase))
                            return true;

                        // Do not allow this client to communicate with unauthenticated servers.
                        return false;
                    };

                    using (HttpClient client = new HttpClient(httpClientHandler))
                    {
                        client.DefaultRequestHeaders.Add("X-ApiKey", buildPostApiKey);

                        var httpTask = client.PostAsync(buildPostUrl, new StringContent(buildPostString, Encoding.UTF8, "application/json"));
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
            string buildBuildUrl;
            string buildBuildUrlKey;

            if (!BuildNightly)
                return true;

            buildPostUrl = Environment.ExpandEnvironmentVariables("%APPVEYOR_NIGHTLY_URL%").Replace("%APPVEYOR_NIGHTLY_URL%", string.Empty, StringComparison.OrdinalIgnoreCase);
            buildPostKey = Environment.ExpandEnvironmentVariables("%APPVEYOR_NIGHTLY_KEY%").Replace("%APPVEYOR_NIGHTLY_KEY%", string.Empty, StringComparison.OrdinalIgnoreCase);
            buildPostName = Environment.ExpandEnvironmentVariables("%APPVEYOR_NIGHTLY_NAME%").Replace("%APPVEYOR_NIGHTLY_NAME%", string.Empty, StringComparison.OrdinalIgnoreCase);
            buildBuildUrl = Environment.ExpandEnvironmentVariables("%APPVEYOR_BUILD_URL%").Replace("%APPVEYOR_BUILD_URL%", string.Empty, StringComparison.OrdinalIgnoreCase);
            buildBuildUrlKey = Environment.ExpandEnvironmentVariables("%APPVEYOR_BUILD_URL_KEY%").Replace("%APPVEYOR_BUILD_URL_KEY%", string.Empty, StringComparison.OrdinalIgnoreCase);

            if (string.IsNullOrEmpty(buildPostUrl))
                return false;
            if (string.IsNullOrEmpty(buildPostKey))
                return false;
            if (string.IsNullOrEmpty(buildPostName))
                return false;
            if (string.IsNullOrEmpty(buildBuildUrl))
                return false;
            if (string.IsNullOrEmpty(buildBuildUrlKey))
                return false;

            try
            {
                foreach (string file in Build_Release_Files)
                {
                    string sourceFile = BuildOutputFolder + file;

                    if (File.Exists(sourceFile))
                    {
                        string filename = Path.GetFileName(sourceFile);
                        FtpWebRequest request = (FtpWebRequest)WebRequest.Create(buildPostUrl + filename);
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
                foreach (string file in Build_Nightly_Files)
                {
                    string sourceFile = BuildOutputFolder + file;

                    if (File.Exists(sourceFile))
                    {
                        string fileName = Path.GetFileName(sourceFile);

                        using (HttpClientHandler httpClientHandler = new HttpClientHandler())
                        {
                            httpClientHandler.AutomaticDecompression = DecompressionMethods.All;
                            httpClientHandler.ServerCertificateCustomValidationCallback = (message, cert, chain, sslPolicyErrors) =>
                            {
                                // Allow this client to communicate with authenticated servers.
                                if (sslPolicyErrors == System.Net.Security.SslPolicyErrors.None)
                                    return true;

                                // Temporarily ignore wj32.org expired certificate.
                                if (string.Equals(cert.GetCertHashString(System.Security.Cryptography.HashAlgorithmName.SHA1), "b60cb3b6aac5f59075689fc3c7dfd561750ce100", StringComparison.OrdinalIgnoreCase))
                                    return true;

                                // Do not allow this client to communicate with unauthenticated servers.
                                return false;
                            };

                            using (HttpClient client = new HttpClient(httpClientHandler))
                            using (FileStream fileStream = File.OpenRead(sourceFile))
                            using (StreamContent streamContent = new StreamContent(fileStream))
                            using (MultipartFormDataContent content = new MultipartFormDataContent())
                            {
                                client.DefaultRequestHeaders.Add("X-ApiKey", buildBuildUrlKey);
                                streamContent.Headers.Add("Content-Type", "application/octet-stream");
                                streamContent.Headers.Add("Content-Disposition", "form-data; name=\"file\"; filename=\"" + fileName + "\"");
                                content.Add(streamContent, "file", fileName);

                                var httpTask = client.PostAsync(buildBuildUrl, content);
                                httpTask.Wait();

                                if (!httpTask.Result.IsSuccessStatusCode)
                                {
                                    Program.PrintColorMessage("[HttpClient PostAsync] " + httpTask.Result, ConsoleColor.Red);
                                    return false;
                                }
                            }
                        }
                    }
                }
            }
            catch (Exception ex)
            {
                Program.PrintColorMessage("[HttpClient PostAsync] " + ex.Message, ConsoleColor.Red);
                return false;
            }

            try
            {
                foreach (string file in Build_Nightly_Files)
                {
                    string sourceFile = BuildOutputFolder + file;

                    if (File.Exists(sourceFile))
                    {
                        if (!AppVeyor.UploadFile(sourceFile))
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
    }
}
