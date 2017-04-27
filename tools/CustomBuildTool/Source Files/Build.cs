using System;
using System.IO;
using System.Net.Http;
using System.Security;
using System.Text;

namespace CustomBuildTool
{
    [SuppressUnmanagedCodeSecurity]
    public static class Build
    {
        private static DateTime TimeStart;
        private static bool BuildNightly = false;
        private static string BuildBranch = "master";
        private static string BuildOutputFolder = "ClientBin";
        private static string BuildVersion;
        private static string BuildRevision;
        private static string BuildMessage;
        private static long BuildSetupFileLength;
        private static long BuildBinFileLength;
        private static string BuildSetupHash;
        private static string BuildBinHash;
        private static string BuildSetupSig;
        private static string GitExePath;
        private static string MSBuildExePath;
        private static string CustomSignToolPath = "tools\\CustomSignTool\\bin\\Release32\\CustomSignTool.exe";

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
            GitExePath = Environment.ExpandEnvironmentVariables("%ProgramFiles%\\Git\\cmd\\git.exe");
            MSBuildExePath = Environment.ExpandEnvironmentVariables(VisualStudio.GetMsbuildFilePath());
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
            Program.PrintColorMessage("Cleaning up build environment...", ConsoleColor.Cyan);

            try
            {

            }
            catch (Exception ex)
            {
                Program.PrintColorMessage("[CleanupBuildEnvironment] " + ex, ConsoleColor.Red);
            }
        }

        public static void ShowBuildEnvironment(bool ShowBuildInfo)
        {
            Program.PrintColorMessage("Setting up build environment...", ConsoleColor.Cyan);

            string currentGitTag = Win32.ExecCommand(GitExePath, "describe --abbrev=0 --tags --always");
            string latestGitRevision = Win32.ExecCommand(GitExePath, "rev-list --count \"" + currentGitTag + ".." + BuildBranch + "\"");

            if (string.IsNullOrEmpty(latestGitRevision))
                BuildRevision = "0";
            else
                BuildRevision = latestGitRevision.Trim();

            BuildVersion = "3.0." + BuildRevision; // TODO: Remove hard-coded major/minor version.
            BuildMessage = Win32.ExecCommand(GitExePath, "log -n 5 --date=format:%Y-%m-%d --pretty=format:\"[%cd] %an %s\" --abbrev-commit");

            if (ShowBuildInfo)
            {
                string buildMessage = string.Empty;

                if (BuildNightly)
                {
                    buildMessage = Win32.ExecCommand(GitExePath, "log -n 5 --date=format:%Y-%m-%d --pretty=format:\"[%cd] %an: %<(65,trunc)%s (%h)\" --abbrev-commit");
                }
                else
                {
                    Win32.GetConsoleMode(Win32.GetStdHandle(Win32.STD_OUTPUT_HANDLE), out ConsoleMode mode);
                    Win32.SetConsoleMode(Win32.GetStdHandle(Win32.STD_OUTPUT_HANDLE), mode | ConsoleMode.ENABLE_VIRTUAL_TERMINAL_PROCESSING);
                    buildMessage = Win32.ExecCommand(GitExePath, "log -n 5 --date=format:%Y-%m-%d --pretty=format:\"%C(green)[%cd]%Creset %C(bold blue)%an%Creset %<(65,trunc)%s%Creset (%C(yellow)%h%Creset)\" --abbrev-commit");
                }

                string currentBranch = Win32.ExecCommand(GitExePath, "rev-parse --abbrev-ref HEAD");
                string currentCommitTag = Win32.ExecCommand(GitExePath, "rev-parse --short HEAD"); // rev-parse HEAD
                //string latestGitCount = Win32.GitExecCommand("rev-list --count " + BuildBranch);         
       
                if (!string.IsNullOrEmpty(currentBranch))
                {
                    Program.PrintColorMessage(Environment.NewLine + "Branch: ", ConsoleColor.Cyan, false);
                    Program.PrintColorMessage(currentBranch, ConsoleColor.White);
                }

                if (!string.IsNullOrEmpty(BuildVersion))
                {
                    Program.PrintColorMessage("Version: ", ConsoleColor.Cyan, false);
                    Program.PrintColorMessage(BuildVersion, ConsoleColor.White);
                }

                Program.PrintColorMessage("Commit: ", ConsoleColor.Cyan, false);
                Program.PrintColorMessage(currentCommitTag + Environment.NewLine, ConsoleColor.White);

                if (!BuildNightly && !string.IsNullOrEmpty(buildMessage))
                {
                    Console.WriteLine(buildMessage + Environment.NewLine);
                }
            }
        }

        public static void ShowBuildStats(bool block)
        {
            TimeSpan buildTime = DateTime.Now - TimeStart;

            Console.WriteLine();
            Console.WriteLine("Build Time: " + buildTime.Minutes + " minute(s), " + buildTime.Seconds + " second(s)");
            Console.WriteLine("Build complete.");
            if (!BuildNightly && block) Console.ReadKey();
        }

        public static bool CopyTextFiles()
        {
            Program.PrintColorMessage("Copying text files...", ConsoleColor.Cyan);

            try
            {
                File.Copy("README.md", "bin\\README.txt", true);
                File.Copy("CHANGELOG.txt", "bin\\CHANGELOG.txt", true);
                File.Copy("COPYRIGHT.txt", "bin\\COPYRIGHT.txt", true);
                File.Copy("LICENSE.txt", "bin\\LICENSE.txt", true);
            }
            catch (Exception ex)
            {
                Program.PrintColorMessage("[ERROR] " + ex, ConsoleColor.Red);
                return false;
            }

            return true;
        }

        public static bool CopyKProcessHacker(bool DebugBuild)
        {
            Program.PrintColorMessage("Copying KPH driver...", ConsoleColor.Cyan);

            try
            {
                if (DebugBuild)
                {
                    File.Copy("KProcessHacker\\bin-signed\\i386\\kprocesshacker.sys", "bin\\Debug32\\kprocesshacker.sys", true);
                    File.Copy("KProcessHacker\\bin-signed\\amd64\\kprocesshacker.sys", "bin\\Debug64\\kprocesshacker.sys", true);
                }
                else
                {
                    File.Copy("KProcessHacker\\bin-signed\\i386\\kprocesshacker.sys", "bin\\Release32\\kprocesshacker.sys", true);
                    File.Copy("KProcessHacker\\bin-signed\\amd64\\kprocesshacker.sys", "bin\\Release64\\kprocesshacker.sys", true);
                }
            }
            catch (Exception ex)
            {
                Program.PrintColorMessage("[ERROR] " + ex, ConsoleColor.Red);
                return false;
            }

            return true;
        }

        public static bool CopyLibFiles(bool DebugBuild)
        {
            Program.PrintColorMessage("Copying Plugin SDK linker files...", ConsoleColor.Cyan);

            try
            {
                if (DebugBuild)
                {
                    File.Copy("bin\\Debug32\\ProcessHacker.lib", "sdk\\lib\\i386\\ProcessHacker.lib", true);
                    File.Copy("bin\\Debug64\\ProcessHacker.lib", "sdk\\lib\\amd64\\ProcessHacker.lib", true);
                }
                else
                {
                    File.Copy("bin\\Release32\\ProcessHacker.lib", "sdk\\lib\\i386\\ProcessHacker.lib", true);
                    File.Copy("bin\\Release64\\ProcessHacker.lib", "sdk\\lib\\amd64\\ProcessHacker.lib", true);
                }
            }
            catch (Exception ex)
            {
                Program.PrintColorMessage("[ERROR] " + ex, ConsoleColor.Red);
                return false;
            }
            
            return true;
        }

        public static bool CopyWow64Files(bool DebugBuild)
        {
            Program.PrintColorMessage("Copying Wow64 support files...", ConsoleColor.Cyan);

            try
            {
                if (DebugBuild)
                {
                    if (!Directory.Exists("bin\\Debug64\\x86"))
                        Directory.CreateDirectory("bin\\Debug64\\x86");
                    if (!Directory.Exists("bin\\Debug64\\x86\\plugins"))
                        Directory.CreateDirectory("bin\\Debug64\\x86\\plugins");

                    File.Copy("bin\\Debug32\\ProcessHacker.exe", "bin\\Debug64\\x86\\ProcessHacker.exe", true);
                    File.Copy("bin\\Debug32\\ProcessHacker.pdb", "bin\\Debug64\\x86\\ProcessHacker.pdb", true);
                    File.Copy("bin\\Debug32\\plugins\\DotNetTools.dll", "bin\\Debug64\\x86\\plugins\\DotNetTools.dll", true);
                    File.Copy("bin\\Debug32\\plugins\\DotNetTools.pdb", "bin\\Debug64\\x86\\plugins\\DotNetTools.pdb", true);
                }
                else
                {
                    if (!Directory.Exists("bin\\Release64\\x86"))
                        Directory.CreateDirectory("bin\\Release64\\x86");
                    if (!Directory.Exists("bin\\Release64\\x86\\plugins"))
                        Directory.CreateDirectory("bin\\Release64\\x86\\plugins");

                    File.Copy("bin\\Release32\\ProcessHacker.exe", "bin\\Release64\\x86\\ProcessHacker.exe", true);
                    File.Copy("bin\\Release32\\ProcessHacker.pdb", "bin\\Release64\\x86\\ProcessHacker.pdb", true);
                    File.Copy("bin\\Release32\\plugins\\DotNetTools.dll", "bin\\Release64\\x86\\plugins\\DotNetTools.dll", true);
                    File.Copy("bin\\Release32\\plugins\\DotNetTools.pdb", "bin\\Release64\\x86\\plugins\\DotNetTools.pdb", true);
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
            Program.PrintColorMessage("Copying Plugin SDK headers...", ConsoleColor.Cyan);

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
                {
                    File.Copy("phnt\\include\\" + file, "sdk\\include\\" + file, true);
                }

                foreach (string file in phlib_headers)
                {
                    File.Copy("phlib\\include\\" + file, "sdk\\include\\" + file, true);
                }
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
            Program.PrintColorMessage("Copying Plugin SDK version header...", ConsoleColor.Cyan);

            try
            {
                HeaderGen gen = new HeaderGen();
                gen.LoadConfig("build\\phapppub_options.txt");
                gen.Execute();

                File.Copy("ProcessHacker\\sdk\\phapppub.h", "sdk\\include\\phapppub.h", true);
                File.Copy("ProcessHacker\\sdk\\phdk.h", "sdk\\include\\phdk.h", true);
                File.Copy("ProcessHacker\\mxml\\mxml.h", "sdk\\include\\mxml.h", true);
                File.Copy("ProcessHacker\\resource.h", "sdk\\include\\phappresource.h", true);
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
            Program.PrintColorMessage("Building Plugin SDK resource header...", ConsoleColor.Cyan);

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

        public static bool BuildKphSignatureFile(bool DebugBuild)
        {
            string output;

            Program.PrintColorMessage("Building KPH signature...", ConsoleColor.Cyan);

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

                if (!string.IsNullOrEmpty(output = Win32.ExecCommand(CustomSignToolPath, "sign -k build\\kph.key bin\\Debug32\\ProcessHacker.exe -s bin\\Debug32\\ProcessHacker.sig")))
                {
                    Program.PrintColorMessage("[WARN] (Debug32) " + output, ConsoleColor.Yellow);
                    return false;
                }

                if (!string.IsNullOrEmpty(output = Win32.ExecCommand(CustomSignToolPath, "sign -k build\\kph.key bin\\Debug64\\ProcessHacker.exe -s bin\\Debug64\\ProcessHacker.sig")))
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

                if (!string.IsNullOrEmpty(output = Win32.ExecCommand(CustomSignToolPath, "sign -k build\\kph.key bin\\Release32\\ProcessHacker.exe -s bin\\Release32\\ProcessHacker.sig")))
                {
                    Program.PrintColorMessage("[ERROR] (Release32) " + output, ConsoleColor.Red);
                    return false;
                }

                if (!string.IsNullOrEmpty(output = Win32.ExecCommand(CustomSignToolPath, "sign -k build\\kph.key bin\\Release64\\ProcessHacker.exe -s bin\\Release64\\ProcessHacker.sig")))
                {
                    Program.PrintColorMessage("[ERROR] (Release64) " + output, ConsoleColor.Red);
                    return false;
                }
            }

            return true;
        }

        public static bool BuildSecureFiles()
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
                Verify.Decrypt("plugins\\OnlineChecks\\virustotal.s", "plugins\\OnlineChecks\\virustotal.h", vtBuildKey);
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
            Program.PrintColorMessage("Building build-setup.exe...", ConsoleColor.Cyan);

            if (!BuildSolution("tools\\CustomSetupTool\\CustomSetupTool.sln", false, false, false))
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
                if (File.Exists(BuildOutputFolder + "\\processhacker-build-sdk.zip"))
                    File.Delete(BuildOutputFolder + "\\processhacker-build-sdk.zip");

                Zip.CreateCompressedFolder("sdk", BuildOutputFolder + "\\processhacker-build-sdk.zip");
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
            }
            catch (Exception ex)
            {
                Program.PrintColorMessage("[ERROR] " + ex, ConsoleColor.Red);
                return false;
            }

            string output = Win32.ExecCommand(
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
            
            return true;
        }

        public static bool BuildPdbZip()
        {
            try
            {
                if (File.Exists(BuildOutputFolder + "\\processhacker-build-pdb.zip"))
                    File.Delete(BuildOutputFolder + "\\processhacker-build-pdb.zip");

                // *.pdb    # Include only PDB files
                // Debug32  # Ignore junk directories
                // Debug64 
                // Obj
                //Zip.CreateCompressedFolder("sdk", BuildOutputFolder + "\\processhacker-build-pdb.zip");
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

            BuildSetupSig = Win32.ExecCommand(
                CustomSignToolPath, 
                "sign -k build\\nightly.key " + BuildOutputFolder + "\\processhacker-build-setup.exe -h"
                );

            return true;
        }


        public static void WebServiceUpdateConfig()
        {
            string buildPostString;
            string buildPosturl;
            string buildPostApiKey;

            if (string.IsNullOrEmpty(BuildSetupHash))
                return;
            if (string.IsNullOrEmpty(BuildBinHash))
                return;
            if (string.IsNullOrEmpty(BuildSetupSig))
                return;
            if (string.IsNullOrEmpty(BuildMessage))
                return;

            buildPostString = Json<BuildUpdateRequest>.Serialize(new BuildUpdateRequest
            {
                Updated = TimeStart.ToString("o"),
                Version = BuildVersion,
                FileLength = BuildSetupFileLength.ToString(),
                ForumUrl = "https://wj32.org/processhacker/forums/viewtopic.php?t=2315",
                Setupurl = "https://ci.appveyor.com/api/projects/processhacker/processhacker2/artifacts/processhacker-" + BuildVersion + "-setup.exe",
                Binurl = "https://ci.appveyor.com/api/projects/processhacker/processhacker2/artifacts/processhacker-" + BuildVersion + "-bin.zip",
                HashSetup = BuildSetupHash,
                HashBin = BuildBinHash,
                sig = BuildSetupSig,
                Message = BuildMessage
            });

            if (string.IsNullOrEmpty(buildPostString))
                return;

            buildPosturl = Environment.ExpandEnvironmentVariables("%APPVEYOR_BUILD_API%").Replace("%APPVEYOR_BUILD_API%", string.Empty);
            buildPostApiKey = Environment.ExpandEnvironmentVariables("%APPVEYOR_BUILD_KEY%").Replace("%APPVEYOR_BUILD_KEY%", string.Empty);

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
                        Win32.ExecCommand("appveyor", "PushArtifact " + releaseFileArray[i]);
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

        public static bool UpdateHeaderFileVersion()
        {
            Program.PrintColorMessage("Updating Process Hacker build revision...", ConsoleColor.Cyan);

            try
            {
                if (File.Exists("ProcessHacker\\include\\phapprev.h"))
                    File.Delete("ProcessHacker\\include\\phapprev.h");

                File.WriteAllText("ProcessHacker\\include\\phapprev.h",
@"#ifndef PHAPPREV_H 
#define PHAPPREV_H 

#define PHAPP_VERSION_REVISION " + BuildRevision + @"

#endif // PHAPPREV_H
");
            }
            catch (Exception ex)
            {
                Program.PrintColorMessage("[ERROR] " + ex.ToString(), ConsoleColor.Yellow);
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
                gen.LoadConfig("build\\phapppub_options.txt");
                gen.Execute();
            }
            catch (Exception ex)
            {
                Program.PrintColorMessage("[ERROR] " + ex.ToString(), ConsoleColor.Red);
                return false;
            }

            return true;
        }

        public static bool BuildSolution(string Solution, bool Build64bit, bool BuildDebug, bool Verbose)
        {
            if (Verbose)
            {
                Program.PrintColorMessage("Building " + Path.GetFileNameWithoutExtension(Solution) + " (", ConsoleColor.Cyan, false);
                Program.PrintColorMessage("x32", ConsoleColor.Green, false);
                Program.PrintColorMessage(")...", ConsoleColor.Cyan);
            }

            string error32 = Win32.ExecCommand(
                MSBuildExePath, 
                "/m /nologo /verbosity:quiet /p:Configuration=" + (BuildDebug ? "Debug" : "Release") + " /p:Platform=Win32 " + Solution
                );

            if (!string.IsNullOrEmpty(error32))
            {
                Program.PrintColorMessage("[ERROR] " + error32, ConsoleColor.Red);
                return false;
            }

            if (Build64bit)
            {
                Program.PrintColorMessage("Building " + Path.GetFileNameWithoutExtension(Solution) + " (", ConsoleColor.Cyan, false);
                Program.PrintColorMessage("x64", ConsoleColor.Green, false);
                Program.PrintColorMessage(")...", ConsoleColor.Cyan);

                string error64 = Win32.ExecCommand(
                    MSBuildExePath,
                    "/m /nologo /verbosity:quiet /p:Configuration=" + (BuildDebug ? "Debug" : "Release") + " /p:Platform=x64 " + Solution
                    );

                if (!string.IsNullOrEmpty(error64))
                {
                    Program.PrintColorMessage("[ERROR] " + error64, ConsoleColor.Red);
                    return false;
                }
            }

            return true;
        }
    }
}
