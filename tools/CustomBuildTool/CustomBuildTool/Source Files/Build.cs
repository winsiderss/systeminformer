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

            if (!File.Exists(MSBuildExePath))
            {
                Console.ForegroundColor = ConsoleColor.Red;
                Console.WriteLine("MsBuild not installed.\r\nExiting...\r\n");
                Console.ForegroundColor = ConsoleColor.White;
                return false;
            }

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
                Console.ForegroundColor = ConsoleColor.Red;
                Console.WriteLine("Error while setting the root directory: " + ex);
                Console.ForegroundColor = ConsoleColor.White;
                return false;
            }

            if (!File.Exists("ProcessHacker.sln"))
            {
                Console.ForegroundColor = ConsoleColor.Red;
                Console.WriteLine("Unable to find project root directory... Exiting.");
                Console.ForegroundColor = ConsoleColor.White;
                return false;
            }

            if (CheckDependencies)
            {
                if (!File.Exists(GitExePath))
                {
                    Console.ForegroundColor = ConsoleColor.Red;
                    Console.WriteLine("Git not installed... Exiting.");
                    Console.ForegroundColor = ConsoleColor.White;
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
                    Console.ForegroundColor = ConsoleColor.Red;
                    Console.WriteLine("Error creating output directory. " + ex);
                    Console.ForegroundColor = ConsoleColor.White;
                    return false;
                }
            }

            return true;
        }

        public static void CleanupBuildEnvironment()
        {
            //Console.WriteLine(ANSI.HIGH_INTENSITY_CYAN + "Cleaning up..." + ANSI.SANE);
        }

        public static void ShowBuildEnvironment(bool ShowVersion = false)
        {
            string currentGitTag = Win32.ExecCommand(GitExePath, "describe --abbrev=0 --tags --always");
            string latestGitRevision = Win32.ExecCommand(GitExePath, "rev-list --count \"" + currentGitTag + ".." + BuildBranch + "\"");

            if (string.IsNullOrEmpty(latestGitRevision))
                BuildRevision = "0";
            else
                BuildRevision = latestGitRevision.Trim();

            BuildVersion = "3.0." + BuildRevision; // TODO: Remove hard-coded major/minor version.
            BuildMessage = Win32.ExecCommand(GitExePath, "log -n 5 --date=format:%Y-%m-%d --pretty=format:\"[%cd] %an %s\" --abbrev-commit");
            
            if (ShowVersion)
            {
                string currentBranch = Win32.ExecCommand(GitExePath, "rev-parse --abbrev-ref HEAD");
                string currentCommitTag = Win32.ExecCommand(GitExePath, "rev-parse --short HEAD"); // rev-parse HEAD
                //string latestGitCount = Win32.GitExecCommand("rev-list --count " + BuildBranch);         
                string buildMessageColor = Win32.ExecCommand(GitExePath, "log -n 1 --date=format:%Y-%m-%d --pretty=format:\"%C(green)[%cd]%Creset %C(bold blue)%an%Creset %<(65,trunc)%s%Creset (%C(yellow)%h%Creset)\" --abbrev-commit");

                Console.WriteLine("Branch: " + currentBranch);
                Console.WriteLine("Version: " + BuildVersion);
                Console.WriteLine("Commit: " + currentCommitTag);
                Console.WriteLine("Message: " + buildMessageColor + Environment.NewLine);
            }
        }

        public static void ShowBuildStats()
        {
            TimeSpan buildTime = DateTime.Now - TimeStart;
            Console.WriteLine();
            Console.WriteLine("Build Time: " + buildTime.Minutes + " minute(s), " + buildTime.Seconds + " second(s)");
            Console.WriteLine("Build complete.");
        }

        public static bool CopyTextFiles()
        {
            try
            {
                File.Copy("README.md", "bin\\README.txt", true);
                File.Copy("CHANGELOG.txt", "bin\\CHANGELOG.txt", true);
                File.Copy("COPYRIGHT.txt", "bin\\COPYRIGHT.txt", true);
                File.Copy("LICENSE.txt", "bin\\LICENSE.txt", true);
            }
            catch (Exception ex)
            {
                Console.ForegroundColor = ConsoleColor.Red;
                Console.WriteLine("[ERROR] " + ex);
                Console.ForegroundColor = ConsoleColor.White;
                return false;
            }

            return true;
        }

        public static bool CopyKProcessHacker()
        {
            try
            {
                File.Copy("KProcessHacker\\bin-signed\\i386\\kprocesshacker.sys", "bin\\Release32\\kprocesshacker.sys", true);
                File.Copy("KProcessHacker\\bin-signed\\amd64\\kprocesshacker.sys", "bin\\Release64\\kprocesshacker.sys", true);
            }
            catch (Exception ex)
            {
                Console.ForegroundColor = ConsoleColor.Red;
                Console.WriteLine("[ERROR] " + ex);
                Console.ForegroundColor = ConsoleColor.White;
                return false;
            }

            return true;
        }

        public static bool CopyLibFiles()
        {
            try
            {
                File.Copy("bin\\Release32\\ProcessHacker.lib", "sdk\\lib\\i386\\ProcessHacker.lib", true);
                File.Copy("bin\\Release64\\ProcessHacker.lib", "sdk\\lib\\amd64\\ProcessHacker.lib", true);
            }
            catch (Exception ex)
            {
                Console.ForegroundColor = ConsoleColor.Red;
                Console.WriteLine("[ERROR] " + ex);
                Console.ForegroundColor = ConsoleColor.White;
                return false;
            }
            
            return true;
        }

        public static bool CopyWow64Files()
        {
            try
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
            catch (Exception ex)
            {
                Console.ForegroundColor = ConsoleColor.Red;
                Console.WriteLine("[ERROR] " + ex);
                Console.ForegroundColor = ConsoleColor.White;
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
                Console.ForegroundColor = ConsoleColor.Red;
                Console.WriteLine("[ERROR] " + ex);
                Console.ForegroundColor = ConsoleColor.White;
                return false;
            }

            return true;
        }

        public static bool CopyVersionHeader()
        {
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
                Console.ForegroundColor = ConsoleColor.Red;
                Console.WriteLine("[ERROR] " + ex);
                Console.ForegroundColor = ConsoleColor.White;
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
                Console.ForegroundColor = ConsoleColor.Red;
                Console.WriteLine("[ERROR] " + ex);
                Console.ForegroundColor = ConsoleColor.White;
                return false;
            }

            return true;
        }

        public static bool BuildKphSignatureFile()
        {
            if (!File.Exists(CustomSignToolPath))
            {
                Console.ForegroundColor = ConsoleColor.Yellow;
                Console.WriteLine("[SKIPPED] CustomSignTool not found.");
                Console.ForegroundColor = ConsoleColor.White;
                return true;
            }

            if (!File.Exists("build\\kph.key"))
            {
                Console.ForegroundColor = ConsoleColor.Yellow;
                Console.WriteLine("[SKIPPED] kph.key not found.");
                Console.ForegroundColor = ConsoleColor.White;
                return true;
            }

            if (!File.Exists("bin\\Release32\\ProcessHacker.exe"))
            {
                Console.ForegroundColor = ConsoleColor.Yellow;
                Console.WriteLine("[SKIPPED] Release32\\ProcessHacker.exe not found.");
                Console.ForegroundColor = ConsoleColor.White;
                return true;
            }

            if (!File.Exists("bin\\Release64\\ProcessHacker.exe"))
            {
                Console.ForegroundColor = ConsoleColor.Yellow;
                Console.WriteLine("[SKIPPED] Release64\\ProcessHacker.exe not found.");
                Console.ForegroundColor = ConsoleColor.White;
                return true;
            }

            if (File.Exists("bin\\Debug32\\ProcessHacker.sig"))
                File.Delete("bin\\Debug32\\ProcessHacker.sig");
            if (File.Exists("bin\\Debug64\\ProcessHacker.sig"))
                File.Delete("bin\\Debug64\\ProcessHacker.sig");
            if (File.Exists("bin\\Release32\\ProcessHacker.sig"))
                File.Delete("bin\\Release32\\ProcessHacker.sig");
            if (File.Exists("bin\\Release64\\ProcessHacker.sig"))
                File.Delete("bin\\Release64\\ProcessHacker.sig");

            File.Create("bin\\Release32\\ProcessHacker.sig").Dispose();
            File.Create("bin\\Release64\\ProcessHacker.sig").Dispose();

            string output = Win32.ExecCommand(CustomSignToolPath, "sign -k build\\kph.key bin\\Debug32\\ProcessHacker.exe -s bin\\Debug32\\ProcessHacker.sig");
            if (!string.IsNullOrEmpty(output))
            {
                Console.ForegroundColor = ConsoleColor.Red;
                Console.WriteLine("[ERROR] (Debug32) " + output);
                Console.ForegroundColor = ConsoleColor.White;
                return false;
            }
            output = Win32.ExecCommand(CustomSignToolPath, "sign -k build\\kph.key bin\\Debug64\\ProcessHacker.exe -s bin\\Debug64\\ProcessHacker.sig");
            if (!string.IsNullOrEmpty(output))
            {
                Console.ForegroundColor = ConsoleColor.Red;
                Console.WriteLine("[ERROR] (Debug64) " + output);
                Console.ForegroundColor = ConsoleColor.White;
                return false;
            }

            output = Win32.ExecCommand(CustomSignToolPath, "sign -k build\\kph.key bin\\Release32\\ProcessHacker.exe -s bin\\Release32\\ProcessHacker.sig");
            if (!string.IsNullOrEmpty(output))
            {
                Console.ForegroundColor = ConsoleColor.Red;
                Console.WriteLine("[ERROR] (Release32) " + output);
                Console.ForegroundColor = ConsoleColor.White;
                return false;
            }
            output = Win32.ExecCommand(CustomSignToolPath, "sign -k build\\kph.key bin\\Release64\\ProcessHacker.exe -s bin\\Release64\\ProcessHacker.sig");
            if (!string.IsNullOrEmpty(output))
            {
                Console.ForegroundColor = ConsoleColor.Red;
                Console.WriteLine("[ERROR] (Release64) " + output);
                Console.ForegroundColor = ConsoleColor.White;
                return false;
            }

            return true;
        }

        public static bool BuildSecureFiles()
        {
            string buildKey = Environment.ExpandEnvironmentVariables("%NIGHTLY_BUILD_KEY%");
            string vtBuildKey = Environment.ExpandEnvironmentVariables("%VIRUSTOTAL_BUILD_KEY%");

            if (string.IsNullOrEmpty(buildKey))
            {
                Console.ForegroundColor = ConsoleColor.Yellow;
                Console.WriteLine("[SKIPPED] (missing build keys).");
                Console.ForegroundColor = ConsoleColor.White;
                return false;
            }

            if (string.IsNullOrEmpty(vtBuildKey))
            {
                Console.ForegroundColor = ConsoleColor.Yellow;
                Console.WriteLine("[SKIPPED] (missing build keys).");
                Console.ForegroundColor = ConsoleColor.White;
                return false;
            }

            try
            {
                Verify.Decrypt(
                    "build\\nightly.s",
                    "build\\nightly.key",
                    buildKey
                    );

                Verify.Decrypt(
                    "plugins\\OnlineChecks\\virustotal.s",
                    "plugins\\OnlineChecks\\virustotal.h",
                    vtBuildKey
                    );
            }
            catch (Exception)
            {

            }

            return true;
        }

        public static bool BuildSetupExe()
        {
            try
            {
                //if (File.Exists(BuildOutputFolder + "\\processhacker-build-setup.exe"))
                //    File.Delete(BuildOutputFolder + "\\processhacker-build-setup.exe");

                //string output = Win32.InnoSetupExecCommand(
                //    InnoSetupExePath,
                //    "build\\installer\\Process_Hacker_installer.iss"
                //    );

                //if (!string.IsNullOrEmpty(output))
                //{
                //    Console.ForegroundColor = ConsoleColor.Red;
                //    Console.Write("\t[ERROR] " + output);
                //    return false;
                //}

                //File.Move(
                //    "build\\installer\\processhacker-3.0-setup.exe", 
                //    BuildOutputFolder + "\\processhacker-build-setup.exe"
                //    );
            }
            catch (Exception ex)
            {
                Console.ForegroundColor = ConsoleColor.Red;
                Console.WriteLine("[ERROR] " + ex);
                Console.ForegroundColor = ConsoleColor.White;
                return false;
            }

            return true;
        }

        public static bool BuildSdkZip()
        {
            try
            {
                if (File.Exists(BuildOutputFolder + "\\processhacker-build-sdk.zip"))
                    File.Delete(BuildOutputFolder + "\\processhacker-build-sdk.zip");

                Zip.CreateCompressedFolder(BuildOutputFolder + "\\processhacker-build-sdk.zip", ".\\sdk");
            }
            catch (Exception ex)
            {
                Console.ForegroundColor = ConsoleColor.Red;
                Console.WriteLine("[ERROR] " + ex);
                Console.ForegroundColor = ConsoleColor.White;
                return false;
            }

            return true;
        }

        public static bool BuildBinZip()
        {
            try
            {
                if (File.Exists(BuildOutputFolder + "\\processhacker-build-bin.zip"))
                    File.Delete(BuildOutputFolder + "\\processhacker-build-bin.zip");

                Directory.Move("bin\\Release32", "bin\\x32");
                Directory.Move("bin\\Release64", "bin\\x64");

                Zip.CreateCompressedFolder(BuildOutputFolder + "\\processhacker-build-bin.zip", "bin");

                Directory.Move("bin\\x32", "bin\\Release32");
                Directory.Move("bin\\x64", "bin\\Release64");
            }
            catch (Exception ex)
            {
                Console.ForegroundColor = ConsoleColor.Red;
                Console.WriteLine("[ERROR] " + ex);
                Console.ForegroundColor = ConsoleColor.White;
                return false;
            }

            return true;
        }

        public static bool BuildSrcZip()
        {
            if (!File.Exists(GitExePath))
            {
                Console.ForegroundColor = ConsoleColor.Yellow;
                Console.WriteLine("[SKIPPED] Git not installed.");
                return false;
            }

            try
            {
                if (File.Exists(BuildOutputFolder + "\\processhacker-build-src.zip"))
                    File.Delete(BuildOutputFolder + "\\processhacker-build-src.zip");
            }
            catch (Exception ex)
            {
                Console.ForegroundColor = ConsoleColor.Red;
                Console.WriteLine("[ERROR] " + ex);
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
                Console.ForegroundColor = ConsoleColor.Red;
                Console.WriteLine("[ERROR] " + output);
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
            }
            catch (Exception ex)
            {
                Console.ForegroundColor = ConsoleColor.Red;
                Console.WriteLine("[ERROR] " + ex);
                return false;
            }

            //    "-ir!*.pdb" //# Include only PDB files
            //    "-xr!Debug32 " + //# Ignore junk directories
            //    "-xr!Debug64 " +
            //    "-xr!Obj "

            //Zip.CreateCompressedFolder(BuildOutputFolder + "\\processhacker-build-pdb.zip", "sdk");

            return true;
        }

        public static bool BuildChecksumsFile()
        {
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
                Console.ForegroundColor = ConsoleColor.Red;
                Console.WriteLine("[ERROR] " + ex);
                Console.ForegroundColor = ConsoleColor.White;
                return false;
            }
            
            return true;
        }

        public static bool BuildUpdateSignature()
        {
            if (!File.Exists(CustomSignToolPath))
            {
                Console.ForegroundColor = ConsoleColor.Yellow;
                Console.WriteLine("[SKIPPED] CustomSignTool not found.");
                Console.ForegroundColor = ConsoleColor.White;
                return true;
            }

            if (!File.Exists("build\\nightly.key"))
            {
                Console.ForegroundColor = ConsoleColor.Yellow;
                Console.WriteLine("[SKIPPED] nightly.key not found.");
                Console.ForegroundColor = ConsoleColor.White;
                return false;
            }

            if (!File.Exists(BuildOutputFolder + "\\processhacker-build-setup.exe"))
            {
                Console.ForegroundColor = ConsoleColor.Yellow;
                Console.WriteLine("[SKIPPED] build-setup.exe not found.");
                Console.ForegroundColor = ConsoleColor.White;
                return false;
            }

            BuildSetupSig = Win32.ExecCommand(
                CustomSignToolPath, 
                "sign -k build\\nightly.key " + BuildOutputFolder + "\\processhacker-build-setup.exe -h"
                );

            return true;
        }

        public static bool MoveBuildFiles()
        {
            string[] releaseFiles =
            {
                BuildOutputFolder + "\\processhacker-" + BuildVersion + "-setup.exe",
                BuildOutputFolder + "\\processhacker-" + BuildVersion + "-bin.zip",
                BuildOutputFolder + "\\processhacker-" + BuildVersion + "-checksums.txt"
            };

            foreach (string file in releaseFiles)
            {
                if (File.Exists(file))
                {
                    try
                    {
                        File.Delete(file);
                    }
                    catch (Exception ex)
                    {
                        Console.ForegroundColor = ConsoleColor.Red;
                        Console.WriteLine("[ERROR] " + ex);
                        return false;
                    }
                }
            }

            //if (File.Exists(BuildOutputFolder + "\\processhacker-build-setup.exe"))
            //File.Move(BuildOutputFolder + "\\processhacker-build-setup.exe",
            //          BuildOutputFolder + "\\processhacker-" + BuildVersion + "-setup.exe");
            //
            //if (File.Exists(BuildOutputFolder + "\\processhacker-build-bin.zip"))
            //File.Move(BuildOutputFolder + "\\processhacker-build-bin.zip",
            //          BuildOutputFolder + "\\processhacker-" + BuildVersion + "-bin.zip");
            //
            //if (File.Exists(BuildOutputFolder + "\\processhacker-build-checksums.txt"))
            //File.Move(BuildOutputFolder + "\\processhacker-build-checksums.txt",
            //          BuildOutputFolder + "\\processhacker-" + BuildVersion + "-checksums.txt");

            return true;
        }




        public static async void UpdateBuildWebService()
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

            string buildPosturl = Environment.ExpandEnvironmentVariables("%APPVEYOR_BUILD_API%");
            string buildPostApiKey = Environment.ExpandEnvironmentVariables("%APPVEYOR_BUILD_KEY%");

            if (string.IsNullOrEmpty(buildPosturl))
                return;
            if (string.IsNullOrEmpty(buildPostApiKey))
                return;

            try
            {
                System.Net.ServicePointManager.Expect100Continue = false;

                using (HttpClient client = new HttpClient())
                {
                    //client.DefaultRequestHeaders.Add("X-ApiKey", buildPostApiKey);
                    var response = await client.PostAsync(buildPosturl, new StringContent(buildPostString, Encoding.UTF8, "application/json"));

                    //if (!response.IsSuccessStatusCode)
                    {
                        Console.ForegroundColor = ConsoleColor.Red;
                        Console.WriteLine("[ERROR] " + response.ToString());
                        Console.ForegroundColor = ConsoleColor.White;
                    }
                }
            }
            catch (Exception ex)
            {
                Console.ForegroundColor = ConsoleColor.Red;
                Console.WriteLine("[ERROR] " + ex);
                Console.ForegroundColor = ConsoleColor.White;
            }
        }


        public static bool UpdateHeaderFileVersion()
        {
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
                Console.ForegroundColor = ConsoleColor.Yellow;
                Console.WriteLine("[ERROR] " + ex.ToString());
                Console.ForegroundColor = ConsoleColor.White;
            }

            return true;
        }


        public static bool BuildPublicHeaderFiles()
        {
            try
            {
                HeaderGen gen = new HeaderGen();
                gen.LoadConfig("build\\phapppub_options.txt");
                gen.Execute();
            }
            catch (Exception ex)
            {
                Console.ForegroundColor = ConsoleColor.Red;
                Console.WriteLine("[ERROR] " + ex.ToString());
                Console.ForegroundColor = ConsoleColor.White;
                return false;
            }

            return true;
        }

        public static bool BuildSolution(string Solution)
        {
            Console.ForegroundColor = ConsoleColor.Cyan;
            Console.Write("Building " + Path.GetFileNameWithoutExtension(Solution) + " (");
            Console.ForegroundColor = ConsoleColor.Green;
            Console.Write("x32");
            Console.ForegroundColor = ConsoleColor.Cyan;
            Console.WriteLine(")...");
            Console.ForegroundColor = ConsoleColor.White;

            string error32 = Win32.ExecCommand(
                MSBuildExePath, 
                "/m /nologo /verbosity:quiet /p:Configuration=Release /p:Platform=Win32 " + Solution
                );

            if (!string.IsNullOrEmpty(error32))
            {
                Console.ForegroundColor = ConsoleColor.Red;
                Console.WriteLine("[ERROR] " + error32);
                Console.ForegroundColor = ConsoleColor.White;
                return false;
            }

            Console.ForegroundColor = ConsoleColor.Cyan;
            Console.Write("Building " + Path.GetFileNameWithoutExtension(Solution) + " (");
            Console.ForegroundColor = ConsoleColor.Green;
            Console.Write("x64");
            Console.ForegroundColor = ConsoleColor.Cyan;
            Console.WriteLine(")...");
            Console.ForegroundColor = ConsoleColor.White;

            string error64 = Win32.ExecCommand(
                MSBuildExePath, 
                "/m /nologo /verbosity:quiet /p:Configuration=Release /p:Platform=x64 " + Solution
                );

            if (!string.IsNullOrEmpty(error64))
            {
                Console.ForegroundColor = ConsoleColor.Red;
                Console.WriteLine("[ERROR] " + error64);
                Console.ForegroundColor = ConsoleColor.White;
                return false;
            }

            return true;
        }
    }
}
