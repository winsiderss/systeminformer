using System;
using System.IO;
using System.Net.Http;
using System.Security;
using System.Security.Cryptography.X509Certificates;
using System.Text;

namespace CustomBuildTool
{
    [SuppressUnmanagedCodeSecurity]
    public static class Build
    {
        private static DateTime TimeStart;
        private static bool BuildNightly = false;
        private static string BuildBranch;
        private static string BuildOutputFolder = "build";
        private static string BuildCommit;
        private static string BuildVersion;
        private static string BuildLongVersion;
        private static string BuildCount;
        private static string BuildRevision;
        private static string BuildMessage;
        private static long BuildSetupFileLength;
        private static long BuildBinFileLength;
        private static string BuildSetupHash;
        private static string BuildBinHash;
        private static string BuildSetupSig;
        private static string GitExePath;
        private static string MSBuildExePath;
        private static string CertUtilExePath;
        private static string MakeAppxExePath;
        private static string CustomSignToolPath;

#region Build Config
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
            "circbuf.h",
            "circbuf_h.h",
            "cpysave.h",
            "dltmgr.h",
            "dspick.h",
            "emenu.h",
            "fastlock.h",
            "filestream.h",
            "graph.h",
            "guisup.h",
            "hexedit.h",
            "hndlinfo.h",
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

        public static bool InitializeBuildEnvironment(bool CheckDependencies)
        {
            TimeStart = DateTime.Now;
            MSBuildExePath = VisualStudio.GetMsbuildFilePath();
            CustomSignToolPath = "tools\\CustomSignTool\\bin\\Release32\\CustomSignTool.exe";
            GitExePath = Environment.ExpandEnvironmentVariables("%ProgramFiles%\\Git\\cmd\\git.exe");
            CertUtilExePath = Environment.ExpandEnvironmentVariables("%SystemRoot%\\system32\\Certutil.exe");
            MakeAppxExePath = Environment.ExpandEnvironmentVariables("%ProgramFiles(x86)%\\Windows Kits\\10\\bin\\x64\\MakeAppx.exe");
            BuildNightly = !string.Equals(Environment.ExpandEnvironmentVariables("%APPVEYOR_BUILD_API%"), "%APPVEYOR_BUILD_API%", StringComparison.OrdinalIgnoreCase);

            try
            {
                DirectoryInfo info = new DirectoryInfo(".");

                while (info.Parent != null && info.Parent.Parent != null)
                {
                    info = info.Parent;

                    if (File.Exists(info.FullName + "\\ProcessHacker.sln"))
                    {
                        // Set the root directory.
                        Directory.SetCurrentDirectory(info.FullName);
                        break;
                    }
                }
            }
            catch (Exception ex)
            {
                Program.PrintColorMessage("Error while setting the root directory: " + ex, ConsoleColor.Red);
                return false;
            }

            if (!File.Exists("ProcessHacker.sln"))
            {
                Program.PrintColorMessage("Unable to find project root directory... Exiting.", ConsoleColor.Red);
                return false;
            }

            if (!File.Exists(MSBuildExePath))
            {
                Program.PrintColorMessage("MsBuild not installed.\r\nExiting...\r\n", ConsoleColor.Red);
                return false;
            }

            if (CheckDependencies)
            {
                if (!File.Exists(GitExePath))
                {
                    Program.PrintColorMessage("Git not installed... Exiting.", ConsoleColor.Red);
                    return false;
                }
            }

            if (!Directory.Exists(BuildOutputFolder))
            {
                try
                {
                    Directory.CreateDirectory(BuildOutputFolder);
                }
                catch (Exception ex)
                {
                    Program.PrintColorMessage("Error creating output directory. " + ex, ConsoleColor.Red);
                    return false;
                }
            }

            return true;
        }

        public static void CleanupBuildEnvironment()
        {
            string[] cleanupFileArray =
            {
                BuildOutputFolder + "\\processhacker-build-setup.exe",
                BuildOutputFolder + "\\processhacker-build-bin.zip",
                BuildOutputFolder + "\\processhacker-build-src.zip",
                BuildOutputFolder + "\\processhacker-build-sdk.zip",
                BuildOutputFolder + "\\processhacker-build-pdb.zip",
                BuildOutputFolder + "\\processhacker-build-checksums.txt",
                BuildOutputFolder + "\\processhacker-build-package.appxbundle",
                BuildOutputFolder + "\\processhacker-" + BuildVersion + "-setup.exe",
                BuildOutputFolder + "\\processhacker-" + BuildVersion + "-bin.zip",
                BuildOutputFolder + "\\processhacker-" + BuildVersion + "-src.zip",
                BuildOutputFolder + "\\processhacker-" + BuildVersion + "-sdk.zip",
                BuildOutputFolder + "\\processhacker-" + BuildVersion + "-pdb.zip",
                BuildOutputFolder + "\\processhacker-" + BuildVersion + "-checksums.txt"
            };

            try
            {
                if (Directory.Exists("build\\output"))
                    Directory.Delete("build\\output", true);

                for (int i = 0; i < cleanupFileArray.Length; i++)
                {
                    if (File.Exists(cleanupFileArray[i]))
                        File.Delete(cleanupFileArray[i]);
                }
            }
            catch (Exception ex)
            {
                Program.PrintColorMessage("[CleanupBuildEnvironment] " + ex, ConsoleColor.Red);
            }
        }

        public static string GetBuildLogString()
        {
            return Win32.ShellExecute(GitExePath, "log -n 1800 --graph --pretty=format:\"%C(yellow)%h%Creset %C(bold blue)%an%Creset %s %C(dim green)(%cr)\" --abbrev-commit ").Trim();
        }

        public static void ShowBuildEnvironment(string Platform, bool ShowBuildInfo, bool ShowLogInfo)
        {
            BuildBranch = Win32.ShellExecute(GitExePath, "rev-parse --abbrev-ref HEAD");
            BuildCommit = Win32.ShellExecute(GitExePath, "rev-parse --short HEAD"); // rev-parse HEAD       
            Program.PrintColorMessage("Branch: ", ConsoleColor.Cyan, false);
            Program.PrintColorMessage(BuildBranch, ConsoleColor.White);
            Program.PrintColorMessage("Commit: ", ConsoleColor.Cyan, false);
            Program.PrintColorMessage(BuildCommit, ConsoleColor.White);

            string currentGitTag = Win32.ShellExecute(GitExePath, "describe --abbrev=0 --tags --always").Trim();
            BuildRevision = Win32.ShellExecute(GitExePath, "rev-list --count \"" + currentGitTag + ".." + BuildBranch + "\"").Trim();
            BuildCount = Win32.ShellExecute(GitExePath, "rev-list --count " + BuildBranch).Trim();

            if (string.IsNullOrEmpty(BuildRevision))
                BuildRevision = "0";
            if (string.IsNullOrEmpty(BuildCount))
                BuildCount = "0";

            BuildVersion = "3.1." + BuildRevision;
            BuildLongVersion = "3.1." + BuildCount + "." + BuildRevision;
   
            if (ShowBuildInfo)
            {
                Program.PrintColorMessage("Version: ", ConsoleColor.Cyan, false);
                Program.PrintColorMessage(BuildVersion + Environment.NewLine, ConsoleColor.White);

                if (!BuildNightly && ShowLogInfo)
                {
                    Win32.GetConsoleMode(Win32.GetStdHandle(Win32.STD_OUTPUT_HANDLE), out ConsoleMode mode);
                    Win32.SetConsoleMode(Win32.GetStdHandle(Win32.STD_OUTPUT_HANDLE), mode | ConsoleMode.ENABLE_VIRTUAL_TERMINAL_PROCESSING);

                    BuildMessage = Win32.ShellExecute(GitExePath, "log -n 5 --date=format:%Y-%m-%d --pretty=format:\"%C(green)[%cd]%Creset %C(bold blue)%an%Creset %<(65,trunc)%s%Creset (%C(yellow)%h%Creset)\" --abbrev-commit");
                    Console.WriteLine(BuildMessage + Environment.NewLine);

                    //BuildMessage = Win32.ShellExecute(GitExePath, "log -n 5 --date=format:%Y-%m-%d --pretty=format:\"[%cd] %an %s\" --abbrev-commit");
                    //BuildMessage = Win32.ShellExecute(GitExePath, "log -n 1 --date=format:%Y-%m-%d --pretty=format:\"[%cd] %an: %<(65,trunc)%s (%h)\" --abbrev-commit");
                    //Console.WriteLine(BuildMessage + Environment.NewLine);
                }
            }
        }

        public static void ShowBuildStats()
        {
            TimeSpan buildTime = DateTime.Now - TimeStart;

            Console.WriteLine(
                Environment.NewLine + "Build Time: " + 
                buildTime.Minutes + " minute(s), " + 
                buildTime.Seconds + " second(s)");
        }

        public static bool CopyTextFiles()
        {
            try
            {
                Win32.CopyIfNewer("README.md", "bin\\README.txt");
                Win32.CopyIfNewer("CHANGELOG.txt", "bin\\CHANGELOG.txt");
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
                        Win32.CopyIfNewer(
                            "bin\\Debug32\\ProcessHacker.lib", 
                            "sdk\\lib\\i386\\ProcessHacker.lib"
                            );
                    }

                    if (Flags.HasFlag(BuildFlags.Build64bit))
                    {
                        Win32.CopyIfNewer(
                            "bin\\Debug64\\ProcessHacker.lib", 
                            "sdk\\lib\\amd64\\ProcessHacker.lib"
                            );
                    }
                }
                else
                {
                    if (Flags.HasFlag(BuildFlags.Build32bit))
                    {
                        Win32.CopyIfNewer(
                            "bin\\Release32\\ProcessHacker.lib", 
                            "sdk\\lib\\i386\\ProcessHacker.lib"
                            );
                    }

                    if (Flags.HasFlag(BuildFlags.Build64bit))
                    {
                        Win32.CopyIfNewer(
                            "bin\\Release64\\ProcessHacker.lib", 
                            "sdk\\lib\\amd64\\ProcessHacker.lib"
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

        public static bool CopyWow64Files(BuildFlags Flags)
        {
            Program.PrintColorMessage("Copying Wow64 support files...", ConsoleColor.Cyan);

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
                    // Remove the existing SDK directory
                    //if (Directory.Exists(folder))
                    //{
                    //    Directory.Delete(folder, true);
                    //}

                    // Create the SDK directories
                    if (!Directory.Exists(folder))
                    {
                        Directory.CreateDirectory(folder);
                    }
                }

                // Copy the plugin SDK headers
                foreach (string file in phnt_headers)
                    Win32.CopyIfNewer("phnt\\include\\" + file, "sdk\\include\\" + file);
                foreach (string file in phlib_headers)
                    Win32.CopyIfNewer("phlib\\include\\" + file, "sdk\\include\\" + file);

                Win32.CopyIfNewer("phlib\\mxml\\mxml.h", "sdk\\include\\mxml.h");

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
                Win32.CopyIfNewer("plugins\\SamplePlugin\\main.c", "sdk\\samples\\SamplePlugin\\main.c");
                Win32.CopyIfNewer("plugins\\SamplePlugin\\SamplePlugin.sln", "sdk\\samples\\SamplePlugin\\SamplePlugin.sln");
                Win32.CopyIfNewer("plugins\\SamplePlugin\\SamplePlugin.vcxproj", "sdk\\samples\\SamplePlugin\\SamplePlugin.vcxproj");
                Win32.CopyIfNewer("plugins\\SamplePlugin\\SamplePlugin.vcxproj.filters", "sdk\\samples\\SamplePlugin\\SamplePlugin.vcxproj.filters");
                Win32.CopyIfNewer("plugins\\SamplePlugin\\bin\\Release32\\SamplePlugin.dll", "sdk\\samples\\SamplePlugin\\bin\\Release32\\SamplePlugin.dll");
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

        public static bool FixupResourceHeader()
        {
            try
            {
                string phappContent = File.ReadAllText("sdk\\include\\phappresource.h");

                if (!string.IsNullOrWhiteSpace(phappContent))
                {
                    phappContent = phappContent.Replace("#define ID", "#define PHAPP_ID");

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
            Program.PrintColorMessage("Building public SDK headers...", ConsoleColor.Cyan);

            try
            {
                HeaderGen gen = new HeaderGen();
                gen.Execute();
            }
            catch (Exception ex)
            {
                Program.PrintColorMessage("[ERROR] " + ex.ToString(), ConsoleColor.Red);
                return false;
            }

            return true;
        }

        public static bool CopyKProcessHacker(bool DebugBuild)
        {
            Program.PrintColorMessage("Copying KPH driver...", ConsoleColor.Cyan);

            if (!File.Exists(CustomSignToolPath))
            {
                Program.PrintColorMessage("[SKIPPED] CustomSignTool not found.", ConsoleColor.Yellow);
                return true;
            }

            if (!File.Exists("build\\kph.key"))
            {
                Program.PrintColorMessage("[SKIPPED] kph.key not found.", ConsoleColor.Yellow);
                return true;
            }

            if (DebugBuild)
            {
                if (!File.Exists("bin\\Debug32\\ProcessHacker.exe"))
                {
                    Program.PrintColorMessage("[SKIPPED] Debug32\\ProcessHacker.exe not found.", ConsoleColor.Yellow);
                    return true;
                }

                if (!File.Exists("bin\\Debug64\\ProcessHacker.exe"))
                {
                    Program.PrintColorMessage("[SKIPPED] Debug64\\ProcessHacker.exe not found.", ConsoleColor.Yellow);
                    return true;
                }

                if (File.Exists("bin\\Debug32\\ProcessHacker.sig"))
                    File.Delete("bin\\Debug32\\ProcessHacker.sig");
                if (File.Exists("bin\\Debug64\\ProcessHacker.sig"))
                    File.Delete("bin\\Debug64\\ProcessHacker.sig");

                File.Create("bin\\Debug32\\ProcessHacker.sig").Dispose();
                File.Create("bin\\Debug64\\ProcessHacker.sig").Dispose();

                string output = Win32.ShellExecute(CustomSignToolPath, "sign -k build\\kph.key bin\\Debug32\\ProcessHacker.exe -s bin\\Debug32\\ProcessHacker.sig");
                if (!string.IsNullOrEmpty(output))
                {
                    Program.PrintColorMessage("[WARN] (Debug32) " + output, ConsoleColor.Yellow);
                    return false;
                }

                output = Win32.ShellExecute(CustomSignToolPath, "sign -k build\\kph.key bin\\Debug64\\ProcessHacker.exe -s bin\\Debug64\\ProcessHacker.sig");
                if (!string.IsNullOrEmpty(output))
                {
                    Program.PrintColorMessage("[WARN] (Debug64) " + output, ConsoleColor.Yellow);
                    return false;
                }
            }
            else
            {
                if (!File.Exists("bin\\Release32\\ProcessHacker.exe"))
                {
                    Program.PrintColorMessage("[SKIPPED] Release32\\ProcessHacker.exe not found.", ConsoleColor.Yellow);
                    return true;
                }

                if (!File.Exists("bin\\Release64\\ProcessHacker.exe"))
                {
                    Program.PrintColorMessage("[SKIPPED] Release64\\ProcessHacker.exe not found.", ConsoleColor.Yellow);
                    return true;
                }

                if (File.Exists("bin\\Release32\\ProcessHacker.sig"))
                    File.Delete("bin\\Release32\\ProcessHacker.sig");
                if (File.Exists("bin\\Release64\\ProcessHacker.sig"))
                    File.Delete("bin\\Release64\\ProcessHacker.sig");

                File.Create("bin\\Release32\\ProcessHacker.sig").Dispose();
                File.Create("bin\\Release64\\ProcessHacker.sig").Dispose();

                string output = Win32.ShellExecute(CustomSignToolPath, "sign -k build\\kph.key bin\\Release32\\ProcessHacker.exe -s bin\\Release32\\ProcessHacker.sig");
                if (!string.IsNullOrEmpty(output))
                {
                    Program.PrintColorMessage("[ERROR] (Release32) " + output, ConsoleColor.Red);
                    return false;
                }

                output = Win32.ShellExecute(CustomSignToolPath, "sign -k build\\kph.key bin\\Release64\\ProcessHacker.exe -s bin\\Release64\\ProcessHacker.sig");
                if (!string.IsNullOrEmpty(output))
                {
                    Program.PrintColorMessage("[ERROR] (Release64) " + output, ConsoleColor.Red);
                    return false;
                }
            }

            try
            {
                if (DebugBuild)
                {
                    Win32.CopyIfNewer(
                        "KProcessHacker\\bin-signed\\i386\\kprocesshacker.sys",
                        "bin\\Debug32\\kprocesshacker.sys"
                        );
                    Win32.CopyIfNewer(
                        "KProcessHacker\\bin-signed\\amd64\\kprocesshacker.sys",
                        "bin\\Debug64\\kprocesshacker.sys"
                        );
                }
                else
                {
                    Win32.CopyIfNewer(
                        "KProcessHacker\\bin-signed\\i386\\kprocesshacker.sys",
                        "bin\\Release32\\kprocesshacker.sys"
                        );
                    Win32.CopyIfNewer(
                        "KProcessHacker\\bin-signed\\amd64\\kprocesshacker.sys",
                        "bin\\Release64\\kprocesshacker.sys"
                        );
                }
            }
            catch (Exception ex)
            {
                Program.PrintColorMessage("[ERROR] (kprocesshacker.sys)" + ex, ConsoleColor.Red);
                return false;
            }

            return true;
        }

        public static bool CopyKeyFiles()
        {
            string buildKey = Environment.ExpandEnvironmentVariables("%NIGHTLY_BUILD_KEY%").Replace("%NIGHTLY_BUILD_KEY%", string.Empty);
            string kphKey = Environment.ExpandEnvironmentVariables("%KPH_BUILD_KEY%").Replace("%KPH_BUILD_KEY%", string.Empty);
            string vtBuildKey = Environment.ExpandEnvironmentVariables("%VIRUSTOTAL_BUILD_KEY%").Replace("%VIRUSTOTAL_BUILD_KEY%", string.Empty);

            if (!BuildNightly)
                return true;

            if (string.IsNullOrEmpty(buildKey))
            {
                Program.PrintColorMessage("[Build] (missing build key).", ConsoleColor.Yellow);
                return false;
            }
            if (string.IsNullOrEmpty(kphKey))
            {
                Program.PrintColorMessage("[Build] (missing kph key).", ConsoleColor.Yellow);
                return false;
            }
            if (string.IsNullOrEmpty(vtBuildKey))
            {
                Program.PrintColorMessage("[Build] (missing VT key).", ConsoleColor.Yellow);
                return false;
            }

            try
            {
                Verify.Decrypt("build\\kph.s", "build\\kph.key", kphKey);
                Verify.Decrypt("build\\nightly.s", "build\\nightly.key", buildKey);
                Verify.Decrypt("build\\virustotal.s", "build\\virustotal.h", vtBuildKey);
            }
            catch (Exception ex)
            {
                Program.PrintColorMessage("[ERROR] (Verify) " + ex, ConsoleColor.Red);
                return false;
            }

            return true;
        }

        public static bool BuildSetupExe()
        {
            Program.PrintColorMessage("Building build-setup.exe...", ConsoleColor.Cyan, true, BuildFlags.BuildVerbose);

            if (!BuildSolution("tools\\CustomSetupTool\\CustomSetupTool.sln", BuildFlags.Build32bit))
                return false;

            try
            {
                if (File.Exists(BuildOutputFolder + "\\processhacker-build-setup.exe"))
                    File.Delete(BuildOutputFolder + "\\processhacker-build-setup.exe");

                File.Move(
                    "tools\\CustomSetupTool\\CustomSetupTool\\bin\\Release32\\CustomSetupTool.exe", 
                    BuildOutputFolder + "\\processhacker-build-setup.exe"
                    );
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
            Program.PrintColorMessage("Building build-sdk.zip...", ConsoleColor.Cyan);

            try
            {
                Zip.CreateCompressedSdkFromFolder("sdk", BuildOutputFolder + "\\processhacker-build-sdk.zip");
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
            Program.PrintColorMessage("Building build-bin.zip...", ConsoleColor.Cyan);

            try
            {
                if (File.Exists(BuildOutputFolder + "\\processhacker-build-bin.zip"))
                    File.Delete(BuildOutputFolder + "\\processhacker-build-bin.zip");

                if (Directory.Exists("bin\\x32"))
                    Directory.Delete("bin\\x32", true);
                if (Directory.Exists("bin\\x64"))
                    Directory.Delete("bin\\x64", true);

                Directory.Move("bin\\Release32", "bin\\x32");
                Directory.Move("bin\\Release64", "bin\\x64");

                Zip.CreateCompressedFolder("bin", BuildOutputFolder + "\\processhacker-build-bin.zip");

                Directory.Move("bin\\x32", "bin\\Release32");
                Directory.Move("bin\\x64", "bin\\Release64");
            }
            catch (Exception ex)
            {
                Program.PrintColorMessage("[ERROR] " + ex, ConsoleColor.Red);
                return false;
            }

            return true;
        }

        public static bool BuildSrcZip()
        {
            Program.PrintColorMessage("Building build-src.zip...", ConsoleColor.Cyan);

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
            Program.PrintColorMessage("Building build-pdb.zip...", ConsoleColor.Cyan);

            try
            {
                Zip.CreateCompressedPdbFromFolder(".\\", BuildOutputFolder + "\\processhacker-build-pdb.zip");
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
            Program.PrintColorMessage("Building build-checksums.txt...", ConsoleColor.Cyan);

            try
            {
                if (File.Exists(BuildOutputFolder + "\\processhacker-build-checksums.txt"))
                    File.Delete(BuildOutputFolder + "\\processhacker-build-checksums.txt");

                if (File.Exists(BuildOutputFolder + "\\processhacker-build-setup.exe"))
                    BuildSetupFileLength = new FileInfo(BuildOutputFolder + "\\processhacker-build-setup.exe").Length;
                if (File.Exists(BuildOutputFolder + "\\processhacker-build-bin.zip"))
                    BuildBinFileLength = new FileInfo(BuildOutputFolder + "\\processhacker-build-bin.zip").Length;
                if (File.Exists(BuildOutputFolder + "\\processhacker-build-setup.exe"))
                    BuildSetupHash = Verify.HashFile(BuildOutputFolder + "\\processhacker-build-setup.exe");
                if (File.Exists(BuildOutputFolder + "\\processhacker-build-bin.zip"))
                    BuildBinHash = Verify.HashFile(BuildOutputFolder + "\\processhacker-build-bin.zip");

                StringBuilder sb = new StringBuilder();
                sb.AppendLine("processhacker-build-setup.exe");
                sb.AppendLine("SHA256: " + BuildSetupHash + Environment.NewLine);
                sb.AppendLine("processhacker-build-bin.zip");
                sb.AppendLine("SHA256: " + BuildBinHash + Environment.NewLine);

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
            Program.PrintColorMessage("Building release signature...", ConsoleColor.Cyan);

            if (!File.Exists(CustomSignToolPath))
            {
                Program.PrintColorMessage("[SKIPPED] CustomSignTool not found.", ConsoleColor.Yellow);
                return true;
            }

            if (!File.Exists("build\\nightly.key"))
            {
                Program.PrintColorMessage("[SKIPPED] nightly.key not found.", ConsoleColor.Yellow);
                return true;
            }

            if (!File.Exists(BuildOutputFolder + "\\processhacker-build-setup.exe"))
            {
                Program.PrintColorMessage("[SKIPPED] build-setup.exe not found.", ConsoleColor.Yellow);
                return false;
            }

            BuildSetupSig = Win32.ShellExecute(
                CustomSignToolPath, 
                "sign -k build\\nightly.key " + BuildOutputFolder + "\\processhacker-build-setup.exe -h"
                );

            return true;
        }

        public static void WebServiceUpdateConfig()
        {
            if (string.IsNullOrEmpty(BuildSetupHash))
                return;
            if (string.IsNullOrEmpty(BuildBinHash))
                return;
            if (string.IsNullOrEmpty(BuildSetupSig))
                return;
            if (string.IsNullOrEmpty(BuildMessage))
                return;

            string buildPostString = Json<BuildUpdateRequest>.Serialize(new BuildUpdateRequest
            {
                Updated = TimeStart.ToString("o"),
                Version = BuildVersion,
                FileLength = BuildSetupFileLength.ToString(),
                ForumUrl = "https://wj32.org/processhacker/nightly.php",
                Setupurl = "https://ci.appveyor.com/api/projects/processhacker/processhacker2/artifacts/processhacker-" + BuildVersion + "-setup.exe",
                Binurl = "https://ci.appveyor.com/api/projects/processhacker/processhacker2/artifacts/processhacker-" + BuildVersion + "-bin.zip",
                HashSetup = BuildSetupHash,
                HashBin = BuildBinHash,
                sig = BuildSetupSig,
                Message = BuildMessage
            });

            if (string.IsNullOrEmpty(buildPostString))
                return;

            string buildPosturl = Environment.ExpandEnvironmentVariables("%APPVEYOR_BUILD_API%").Replace("%APPVEYOR_BUILD_API%", string.Empty);
            string buildPostApiKey = Environment.ExpandEnvironmentVariables("%APPVEYOR_BUILD_KEY%").Replace("%APPVEYOR_BUILD_KEY%", string.Empty);

            if (string.IsNullOrEmpty(buildPosturl))
                return;
            if (string.IsNullOrEmpty(buildPostApiKey))
                return;

            Program.PrintColorMessage("Updating Build WebService... " + BuildVersion, ConsoleColor.Cyan);

            try
            {
                System.Net.ServicePointManager.Expect100Continue = false;

                using (HttpClient client = new HttpClient())
                {
                    client.DefaultRequestHeaders.Add("X-ApiKey", buildPostApiKey);

                    var httpTask = client.PostAsync(buildPosturl, new StringContent(buildPostString, Encoding.UTF8, "application/json"));

                    httpTask.Wait();

                    if (!httpTask.Result.IsSuccessStatusCode)
                    {
                        Program.PrintColorMessage("[UpdateBuildWebService] " + httpTask.Result.ToString(), ConsoleColor.Red);
                    }
                }
            }
            catch (Exception ex)
            {
                Program.PrintColorMessage("[UpdateBuildWebService] " + ex, ConsoleColor.Red);
            }
        }

        public static bool AppveyorUploadBuildFiles()
        {
            string[] buildFileArray =
            {
                BuildOutputFolder + "\\processhacker-build-setup.exe",
                BuildOutputFolder + "\\processhacker-build-bin.zip",
                BuildOutputFolder + "\\processhacker-build-checksums.txt"
            };
            string[] releaseFileArray =
            {
                BuildOutputFolder + "\\processhacker-" + BuildVersion + "-setup.exe",
                BuildOutputFolder + "\\processhacker-" + BuildVersion + "-bin.zip",
                BuildOutputFolder + "\\processhacker-" + BuildVersion + "-checksums.txt"
            };

            if (!BuildNightly)
                return false;

            for (int i = 0; i < releaseFileArray.Length; i++)
            {
                if (File.Exists(releaseFileArray[i]))
                {
                    try
                    {
                        File.Delete(releaseFileArray[i]);
                    }
                    catch (Exception ex)
                    {
                        Program.PrintColorMessage("[WebServiceUploadBuild] " + ex, ConsoleColor.Red);
                        return false;
                    }
                }
            }

            for (int i = 0; i < buildFileArray.Length; i++)
            {
                if (File.Exists(buildFileArray[i]))
                {
                    try
                    {
                        File.Move(buildFileArray[i], releaseFileArray[i]);
                    }
                    catch (Exception ex)
                    {
                        Program.PrintColorMessage("[WebServiceUploadBuild] " + ex, ConsoleColor.Red);
                        return false;
                    }
                }
            }

            for (int i = 0; i < releaseFileArray.Length; i++)
            {
                if (File.Exists(releaseFileArray[i]))
                {
                    Console.WriteLine("Uploading " + releaseFileArray[i] + "...");

                    try
                    {
                        Win32.ShellExecute("appveyor", "PushArtifact " + releaseFileArray[i]);
                    }
                    catch (Exception ex)
                    {
                        Program.PrintColorMessage("[WebServicePushArtifact] " + ex, ConsoleColor.Red);
                        return false;
                    }
                }
            }

            return true;
        }

        public static bool BuildSolution(string Solution, BuildFlags Flags)
        {
            if ((Flags & BuildFlags.Build32bit) == BuildFlags.Build32bit)
            {
                Program.PrintColorMessage("Building " + Path.GetFileNameWithoutExtension(Solution) + " (", ConsoleColor.Cyan, false, Flags);
                Program.PrintColorMessage("x32", ConsoleColor.Green, false, Flags);
                Program.PrintColorMessage(")...", ConsoleColor.Cyan, true, Flags);

                string error32 = Win32.ShellExecute(
                    MSBuildExePath,
                    "/m /nologo /verbosity:quiet " +
                    "/p:Configuration=" + (Flags.HasFlag(BuildFlags.BuildDebug) ? "Debug" : "Release") + " " +
                    "/p:Platform=Win32 " +
                    "/p:ExternalCompilerOptions=\"PH_BUILD_API;PHAPP_VERSION_REVISION=\"" + BuildRevision + "\";PHAPP_VERSION_BUILD=\"" + BuildCount + "\"\" " + 
                    Solution
                    );
                
                if (!string.IsNullOrEmpty(error32))
                {
                    Program.PrintColorMessage("[ERROR] " + error32, ConsoleColor.Red, true, Flags);
                    return false;
                }
            }

            if ((Flags & BuildFlags.Build64bit) == BuildFlags.Build64bit)
            {
                Program.PrintColorMessage("Building " + Path.GetFileNameWithoutExtension(Solution) + " (", ConsoleColor.Cyan, false, Flags);
                Program.PrintColorMessage("x64", ConsoleColor.Green, false, Flags);
                Program.PrintColorMessage(")...", ConsoleColor.Cyan, true, Flags);

                string error64 = Win32.ShellExecute(
                    MSBuildExePath,
                    "/m /nologo /verbosity:quiet " +
                    "/p:Configuration=" + (Flags.HasFlag(BuildFlags.BuildDebug) ? "Debug" : "Release") + " " +
                    "/p:Platform=x64 " +
                    "/p:ExternalCompilerOptions=\"PH_BUILD_API;PHAPP_VERSION_REVISION=\"" + BuildRevision + "\";PHAPP_VERSION_BUILD=\"" + BuildCount + "\"\" " +
                    Solution
                    );

                if (!string.IsNullOrEmpty(error64))
                {
                    Program.PrintColorMessage("[ERROR] " + error64, ConsoleColor.Red, true, Flags);
                    return false;
                }
            }

            return true;
        }

#region Appx Package
        public static void BuildAppxPackage(BuildFlags Flags)
        {
            string error;
            string signToolExePath = Environment.ExpandEnvironmentVariables("%ProgramFiles(x86)%\\Windows Kits\\10\\bin\\x64\\SignTool.exe");

            Program.PrintColorMessage("Building processhacker-build-package.appxbundle...", ConsoleColor.Cyan);

            try
            {
                if (!Directory.Exists("build\\output"))
                    Directory.CreateDirectory("build\\output");

                if (File.Exists(BuildOutputFolder + "\\processhacker-build-package.appxbundle"))
                    File.Delete(BuildOutputFolder + "\\processhacker-build-package.appxbundle");

                Win32.ImageResizeFile(44, "ProcessHacker\\resources\\ProcessHacker.png", "build\\output\\ProcessHacker-44.png");
                Win32.ImageResizeFile(50, "ProcessHacker\\resources\\ProcessHacker.png", "build\\output\\ProcessHacker-50.png");
                Win32.ImageResizeFile(150, "ProcessHacker\\resources\\ProcessHacker.png", "build\\output\\ProcessHacker-150.png");

                if ((Flags & BuildFlags.Build32bit) == BuildFlags.Build32bit)
                {
                    // create the package manifest
                    string appxManifestString = Properties.Resources.AppxManifest;
                    appxManifestString = appxManifestString.Replace("PH_APPX_ARCH", "x86");
                    appxManifestString = appxManifestString.Replace("PH_APPX_VERSION", BuildLongVersion);
                    File.WriteAllText("build\\output\\AppxManifest32.xml", appxManifestString);

                    // create the package mapping file
                    StringBuilder packageMap32 = new StringBuilder(0x100);
                    packageMap32.AppendLine("[Files]");
                    packageMap32.AppendLine("\"build\\output\\AppxManifest32.xml\" \"AppxManifest.xml\"");
                    packageMap32.AppendLine("\"build\\output\\ProcessHacker-44.png\" \"Assets\\ProcessHacker-44.png\"");
                    packageMap32.AppendLine("\"build\\output\\ProcessHacker-50.png\" \"Assets\\ProcessHacker-50.png\"");
                    packageMap32.AppendLine("\"build\\output\\ProcessHacker-150.png\" \"Assets\\ProcessHacker-150.png\"");

                    var filesToAdd = Directory.GetFiles("bin\\Release32", "*", SearchOption.AllDirectories);
                    for (int i = 0; i < filesToAdd.Length; i++)
                    {
                        string filePath = filesToAdd[i];

                        // Ignore junk files
                        if (filePath.EndsWith(".pdb", StringComparison.OrdinalIgnoreCase) ||
                            filePath.EndsWith(".iobj", StringComparison.OrdinalIgnoreCase) ||
                            filePath.EndsWith(".ipdb", StringComparison.OrdinalIgnoreCase) ||
                            filePath.EndsWith(".exp", StringComparison.OrdinalIgnoreCase) ||
                            filePath.EndsWith(".lib", StringComparison.OrdinalIgnoreCase))
                        {
                            continue;
                        }

                        packageMap32.AppendLine("\"" + filePath + "\" \"" + filePath.Substring("bin\\Release32\\".Length) + "\"");
                    } 
                    File.WriteAllText("build\\output\\package32.map", packageMap32.ToString());

                    // create the package
                    error = Win32.ShellExecute(
                        MakeAppxExePath,
                        "pack /o /f build\\output\\package32.map /p " + BuildOutputFolder + "\\output\\processhacker-build-package-x32.appx"
                        );
                    Program.PrintColorMessage(Environment.NewLine + error, ConsoleColor.Gray);

                    // sign the package
                    error = Win32.ShellExecute(
                        signToolExePath,
                        "sign /v /fd SHA256 /a /f build\\processhacker-appx.pfx /td SHA256 " + BuildOutputFolder + "\\output\\processhacker-build-package-x32.appx"
                        );
                    Program.PrintColorMessage(Environment.NewLine + error, ConsoleColor.Gray);
                }

                if ((Flags & BuildFlags.Build64bit) == BuildFlags.Build64bit)
                {
                    // create the package manifest
                    string appxManifestString = Properties.Resources.AppxManifest;
                    appxManifestString = appxManifestString.Replace("PH_APPX_ARCH", "x64");
                    appxManifestString = appxManifestString.Replace("PH_APPX_VERSION", BuildLongVersion);
                    File.WriteAllText("build\\output\\AppxManifest64.xml", appxManifestString);

                    StringBuilder packageMap64 = new StringBuilder(0x100);
                    packageMap64.AppendLine("[Files]");
                    packageMap64.AppendLine("\"build\\output\\AppxManifest64.xml\" \"AppxManifest.xml\"");
                    packageMap64.AppendLine("\"build\\output\\ProcessHacker-44.png\" \"Assets\\ProcessHacker-44.png\"");
                    packageMap64.AppendLine("\"build\\output\\ProcessHacker-50.png\" \"Assets\\ProcessHacker-50.png\"");
                    packageMap64.AppendLine("\"build\\output\\ProcessHacker-150.png\" \"Assets\\ProcessHacker-150.png\"");

                    var filesToAdd = Directory.GetFiles("bin\\Release64", "*", SearchOption.AllDirectories);
                    for (int i = 0; i < filesToAdd.Length; i++)
                    {
                        string filePath = filesToAdd[i];

                        // Ignore junk files
                        if (filePath.EndsWith(".pdb", StringComparison.OrdinalIgnoreCase) ||
                            filePath.EndsWith(".iobj", StringComparison.OrdinalIgnoreCase) ||
                            filePath.EndsWith(".ipdb", StringComparison.OrdinalIgnoreCase) ||
                            filePath.EndsWith(".exp", StringComparison.OrdinalIgnoreCase) ||
                            filePath.EndsWith(".lib", StringComparison.OrdinalIgnoreCase))
                        {
                            continue;
                        }

                        packageMap64.AppendLine("\"" + filePath + "\" \"" + filePath.Substring("bin\\Release64\\".Length) + "\"");
                    }
                    File.WriteAllText("build\\output\\package64.map", packageMap64.ToString());

                    // create the package
                    error = Win32.ShellExecute(
                        MakeAppxExePath,
                        "pack /o /f build\\output\\package64.map /p " + BuildOutputFolder + "\\output\\processhacker-build-package-x64.appx"
                        );
                    Program.PrintColorMessage(Environment.NewLine + error, ConsoleColor.Gray);

                    // sign the package
                    error = Win32.ShellExecute(
                        signToolExePath,
                        "sign /v /fd SHA256 /a /f build\\processhacker-appx.pfx /td SHA256 " + BuildOutputFolder + "\\output\\processhacker-build-package-x64.appx"
                        );
                    Program.PrintColorMessage(Environment.NewLine + error, ConsoleColor.Gray);
                }

                {
                    // create the appx bundle map
                    StringBuilder bundleMap = new StringBuilder(0x100);
                    bundleMap.AppendLine("[Files]");
                    bundleMap.AppendLine("\"" + BuildOutputFolder + "\\output\\processhacker-build-package-x32.appx\" \"processhacker-build-package-x32.appx\"");
                    bundleMap.AppendLine("\"" + BuildOutputFolder + "\\output\\processhacker-build-package-x64.appx\" \"processhacker-build-package-x64.appx\"");
                    File.WriteAllText("build\\output\\bundle.map", bundleMap.ToString());

                    // create the appx bundle package
                    error = Win32.ShellExecute(
                        MakeAppxExePath,
                        "bundle /f build\\output\\bundle.map /p " + BuildOutputFolder + "\\processhacker-build-package.appxbundle"
                        );
                    Program.PrintColorMessage(Environment.NewLine + error, ConsoleColor.Gray);

                    // sign the appx bundle package
                    error = Win32.ShellExecute(
                        signToolExePath,
                        "sign /v /fd SHA256 /a /f build\\processhacker-appx.pfx /td SHA256 " + BuildOutputFolder + "\\processhacker-build-package.appxbundle"
                        );
                    Program.PrintColorMessage(Environment.NewLine + error, ConsoleColor.Gray);
                }

                Directory.Delete("build\\output", true);
            }
            catch (Exception ex)
            {
                Program.PrintColorMessage("[ERROR] " + ex, ConsoleColor.Red);
            }
        }

        public static bool BuildAppxSignature()
        {
            Program.PrintColorMessage("Building Appx Signature...", ConsoleColor.Cyan);

            var makeCertExePath = Environment.ExpandEnvironmentVariables("%ProgramFiles(x86)%\\Windows Kits\\10\\bin\\x64\\MakeCert.exe");
            var pvk2PfxExePath = Environment.ExpandEnvironmentVariables("%ProgramFiles(x86)%\\Windows Kits\\10\\bin\\x64\\Pvk2Pfx.exe");

            try
            {
                if (File.Exists("build\\processhacker-appx.pvk"))
                    File.Delete("build\\processhacker-appx.pvk");
                if (File.Exists("build\\processhacker-appx.cer"))
                    File.Delete("build\\processhacker-appx.cer");
                if (File.Exists("build\\processhacker-appx.pfx"))
                    File.Delete("build\\processhacker-appx.pfx");

                string output = Win32.ShellExecute(makeCertExePath,
                    "/n " +
                    "\"CN=ProcessHacker, O=ProcessHacker, C=AU\" " +
                    "/r /h 0 " +
                    "/eku \"1.3.6.1.5.5.7.3.3,1.3.6.1.4.1.311.10.3.13\" " +
                    "/sv " +
                    "build\\processhacker-appx.pvk " +
                    "build\\processhacker-appx.cer "
                    );

                if (!string.IsNullOrEmpty(output) && !output.Equals("Succeeded", StringComparison.OrdinalIgnoreCase))
                {
                    Program.PrintColorMessage("[MakeCert] " + output, ConsoleColor.Red);
                    return false;
                }

                output = Win32.ShellExecute(pvk2PfxExePath,
                    "/pvk build\\processhacker-appx.pvk " +
                    "/spc build\\processhacker-appx.cer " +
                    "/pfx build\\processhacker-appx.pfx "
                    );

                if (!string.IsNullOrEmpty(output))
                {
                    Program.PrintColorMessage("[Pvk2Pfx] " + output, ConsoleColor.Red);
                    return false;
                }

                output = Win32.ShellExecute(CertUtilExePath,
                    "-addStore TrustedPeople build\\processhacker-appx.cer"
                    );

                if (!string.IsNullOrEmpty(output) && !output.Contains("command completed successfully"))
                {
                    Program.PrintColorMessage("[Certutil] " + output, ConsoleColor.Red);
                    return false;
                }
            }
            catch (Exception ex)
            {
                Program.PrintColorMessage("[ERROR] " + ex, ConsoleColor.Red);
                return false;
            }

            return true;
        }

        public static bool CleanupAppxSignature()
        {
            try
            {
                X509Store store = new X509Store(StoreName.TrustedPeople, StoreLocation.LocalMachine);
                store.Open(OpenFlags.ReadWrite);

                foreach (X509Certificate2 c in store.Certificates)
                {
                    if (c.Subject.Equals("CN=ProcessHacker, O=ProcessHacker, C=AU", StringComparison.OrdinalIgnoreCase))
                    {
                        Console.WriteLine("Removing: {0}", c.Subject);
                        store.Remove(c);
                    }
                }

                if (File.Exists("build\\processhacker-appx.pvk"))
                    File.Delete("build\\processhacker-appx.pvk");
                if (File.Exists("build\\processhacker-appx.cer"))
                    File.Delete("build\\processhacker-appx.cer");
                if (File.Exists("build\\processhacker-appx.pfx"))
                    File.Delete("build\\processhacker-appx.pfx");
            }
            catch (Exception ex)
            {
                Program.PrintColorMessage("[ERROR] " + ex, ConsoleColor.Red);
                return false;
            }

            return true;
        }
#endregion
    }
}
