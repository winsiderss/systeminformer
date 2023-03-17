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
    public static class Win32
    {
        public static void CreateDirectory(string FileName)
        {
            if (!Directory.Exists(FileName))
            {
                Directory.CreateDirectory(FileName);
            }
        }

        public static int CreateProcess(string FileName, string Arguments, out string outputstring, bool FixNewLines = true)
        {
            int exitcode = int.MaxValue;
            StringBuilder output = new StringBuilder();
            StringBuilder error = new StringBuilder();

            try
            {
                using (Process process = new Process())
                {
                    process.StartInfo.FileName = FileName;
                    process.StartInfo.Arguments = Arguments;
                    process.StartInfo.UseShellExecute = false;
                    process.StartInfo.RedirectStandardOutput = true;
                    process.StartInfo.RedirectStandardError = true;
                    process.StartInfo.StandardErrorEncoding = Encoding.UTF8;
                    process.StartInfo.StandardOutputEncoding = Encoding.UTF8;

                    using (AutoResetEvent outputWaitHandle = new AutoResetEvent(false))
                    using (AutoResetEvent errorWaitHandle = new AutoResetEvent(false))
                    {
                        process.OutputDataReceived += (sender, e) =>
                        {
                            if (e.Data == null)
                            {
                                outputWaitHandle.Set();
                            }
                            else
                            {
                                output.AppendLine(e.Data);
                            }
                        };
                        process.ErrorDataReceived += (sender, e) =>
                        {
                            if (e.Data == null)
                            {
                                errorWaitHandle.Set();
                            }
                            else
                            {
                                error.AppendLine(e.Data);
                            }
                        };

                        process.Start();
                        process.BeginOutputReadLine();
                        process.BeginErrorReadLine();
                        process.WaitForExit();

                        if (outputWaitHandle.WaitOne() && errorWaitHandle.WaitOne())
                        {
                            exitcode = process.ExitCode;
                        }
                    }
                }
            }
            catch (Exception ex)
            {
                Program.PrintColorMessage($"[CreateProcess] {ex}", ConsoleColor.Red);
            }

            outputstring = output.ToString() + error.ToString();

            if (FixNewLines)
            {
                //outputstring = outputstring.Replace("\n\n", "\r\n", StringComparison.OrdinalIgnoreCase).Trim();
                outputstring = outputstring.Replace("\r\n", string.Empty, StringComparison.OrdinalIgnoreCase).Trim();
            }

            return exitcode;
        }

        public static bool DeleteFile(string FileName)
        {
            try
            {
                if (File.Exists(FileName))
                    File.Delete(FileName);
                return true;
            }
            catch (Exception ex)
            {
                Program.PrintColorMessage($"[DeleteFile] {ex}", ConsoleColor.Red);
            }

            return false;
        }

        public static string ShellExecute(string FileName, string Arguments, bool FixNewLines = true)
        {
            CreateProcess(FileName, Arguments, out string outputstring, FixNewLines);

            return outputstring;
        }

        public static string SearchFile(string FileName)
        {
            try
            {
                if (File.Exists(FileName))
                    return Path.GetFullPath(FileName);

                string values = Environment.GetEnvironmentVariable("PATH");

                foreach (string path in values.Split(new[] { ';' }, StringSplitOptions.RemoveEmptyEntries))
                {
                    string whereResult = Path.Combine(path, FileName);

                    if (File.Exists(whereResult))
                        return whereResult;
                }
            }
            catch (Exception ex)
            {
                Program.PrintColorMessage($"[SearchFile] {ex}", ConsoleColor.Yellow);
            }

            return null;
        }

        public static void CopyIfNewer(string SourceFile, string DestinationFile)
        {
            if (!File.Exists(SourceFile))
                return;

            if (SourceFile.EndsWith(".sys", StringComparison.OrdinalIgnoreCase))
            {
                if (!File.Exists(DestinationFile))
                {
                    File.Copy(SourceFile, DestinationFile, true);
                }
                else
                {
                    FileVersionInfo currentInfo = FileVersionInfo.GetVersionInfo(SourceFile);
                    FileVersionInfo newInfo = FileVersionInfo.GetVersionInfo(DestinationFile);
                    var currentInfoVersion = new Version(currentInfo.FileVersion);
                    var newInfoVersion = new Version(newInfo.FileVersion);

                    if (
                        currentInfoVersion > newInfoVersion ||
                        File.GetLastWriteTime(SourceFile) > File.GetLastWriteTime(DestinationFile)
                        )
                    {
                        File.Copy(SourceFile, DestinationFile, true);
                    }
                }
            }
            else
            {
                if (File.GetLastWriteTime(SourceFile) > File.GetLastWriteTime(DestinationFile))
                    File.Copy(SourceFile, DestinationFile, true);
            }
        }

        public static string GetEnvironmentVariable(string Name)
        {
            return Environment.ExpandEnvironmentVariables(Name).Replace(Name, string.Empty, StringComparison.OrdinalIgnoreCase);
        }

        public static string GetKernelVersion()
        {
            try
            {
                string filePath = GetEnvironmentVariable("%SystemRoot%\\System32\\ntoskrnl.exe");

                if (!File.Exists(filePath))
                    filePath = GetEnvironmentVariable("%SystemRoot%\\Sysnative\\ntoskrnl.exe");

                if (File.Exists(filePath))
                {
                    FileVersionInfo versionInfo = FileVersionInfo.GetVersionInfo(filePath);
                    return versionInfo.FileVersion ?? string.Empty;
                }
            }
            catch (Exception ex)
            {
                Program.PrintColorMessage($"[GetKernelVersion] {ex}", ConsoleColor.Red);
            }

            return string.Empty;
        }

        public static unsafe string GetKeyValue(bool LocalMachine, string KeyName, string ValueName, string DefaultValue)
        {
            string value = string.Empty;
            IntPtr valueBuffer;
            IntPtr keyHandle;

            if (NativeMethods.RegOpenKeyExW(
                LocalMachine ? NativeMethods.HKEY_LOCAL_MACHINE : NativeMethods.HKEY_CURRENT_USER,
                KeyName,
                0,
                NativeMethods.KEY_READ,
                &keyHandle
                ) == 0)
            {
                int valueLength = 0;
                int valueType = 0;

                NativeMethods.RegQueryValueExW(
                    keyHandle,
                    ValueName,
                    IntPtr.Zero,
                    &valueType,
                    IntPtr.Zero,
                    &valueLength
                    );

                if (valueType == 1 || valueLength > 4)
                {
                    valueBuffer = Marshal.AllocHGlobal(valueLength);

                    if (NativeMethods.RegQueryValueExW(
                        keyHandle,
                        ValueName,
                        IntPtr.Zero,
                        null,
                        valueBuffer,
                        &valueLength
                        ) == 0)
                    {
                        value = Marshal.PtrToStringUni(valueBuffer, valueLength / 2 - 1);
                    }

                    Marshal.FreeHGlobal(valueBuffer);
                }

                _ = NativeMethods.RegCloseKey(keyHandle);
            }

            return string.IsNullOrWhiteSpace(value) ? DefaultValue : value;
        }
    }

    public static class Verify
    {
        private static Aes GetRijndael(string Secret)
        {
            using (Rfc2898DeriveBytes rfc2898DeriveBytes = new Rfc2898DeriveBytes(
                Secret,
                Convert.FromBase64String("e0U0RTY2RjU5LUNBRjItNEMzOS1BN0Y4LTQ2MDk3QjFDNDYxQn0="),
                10000,
                HashAlgorithmName.SHA512))
            {
                Aes rijndael = Aes.Create();

                rijndael.Key = rfc2898DeriveBytes.GetBytes(32);
                rijndael.IV = rfc2898DeriveBytes.GetBytes(16);

                return rijndael;
            }
        }

        public static void Encrypt(string FileName, string outFileName, string Secret)
        {
            if (!File.Exists(FileName))
            {
                Program.PrintColorMessage($"Unable to encrypt file {Path.GetFileName(FileName)}: Does not exist.", ConsoleColor.Yellow);
                return;
            }

            using (Aes rijndael = GetRijndael(Secret))
            using (FileStream fileStream = File.OpenRead(FileName))
            using (FileStream fileOutStream = File.Create(outFileName))
            using (CryptoStream cryptoStream = new CryptoStream(fileOutStream, rijndael.CreateEncryptor(), CryptoStreamMode.Write, true))
            {
                fileStream.CopyTo(cryptoStream);
            }
        }

        public static bool Decrypt(string FileName, string outFileName, string Secret)
        {
            if (!File.Exists(FileName))
            {
                Program.PrintColorMessage($"Unable to decrypt file {Path.GetFileName(FileName)}: Does not exist.", ConsoleColor.Yellow);
                return false;
            }

            try
            {
                using (Aes rijndael = GetRijndael(Secret))
                using (FileStream fileStream = File.OpenRead(FileName))
                using (FileStream fileOutStream = File.Create(outFileName))
                using (CryptoStream cryptoStream = new CryptoStream(fileOutStream, rijndael.CreateDecryptor(), CryptoStreamMode.Write, true))
                {
                    fileStream.CopyTo(cryptoStream);
                }
            }
            catch
            {
                return false;
            }

            return true;
        }

        private static Aes GetRijndaelLegacy(string Secret)
        {
#pragma warning disable SYSLIB0041 // Type or member is obsolete
            using (Rfc2898DeriveBytes rfc2898DeriveBytes = new Rfc2898DeriveBytes(
                Secret,
                Convert.FromBase64String("e0U0RTY2RjU5LUNBRjItNEMzOS1BN0Y4LTQ2MDk3QjFDNDYxQn0="),
                10000))
            {
                Aes rijndael = Aes.Create();

                rijndael.Key = rfc2898DeriveBytes.GetBytes(32);
                rijndael.IV = rfc2898DeriveBytes.GetBytes(16);

                return rijndael;
            }
#pragma warning restore SYSLIB0041 // Type or member is obsolete
        }

        public static bool DecryptLegacy(string FileName, string outFileName, string Secret)
        {
            if (!File.Exists(FileName))
            {
                Program.PrintColorMessage($"Unable to decrypt legacy file {Path.GetFileName(FileName)}: Does not exist.", ConsoleColor.Yellow);
                return false;
            }

            try
            {
                using (Aes rijndael = GetRijndaelLegacy(Secret))
                using (FileStream fileStream = File.OpenRead(FileName))
                using (FileStream fileOutStream = File.Create(outFileName))
                using (CryptoStream cryptoStream = new CryptoStream(fileOutStream, rijndael.CreateDecryptor(), CryptoStreamMode.Write, true))
                {
                    fileStream.CopyTo(cryptoStream);
                }
            }
            catch
            {
                return false;
            }

            return true;
        }

        public static string HashFile(string FileName)
        {
            if (!File.Exists(FileName))
            {
                Program.PrintColorMessage($"Unable to hash file {Path.GetFileName(FileName)}: Does not exist.", ConsoleColor.Yellow);
                return string.Empty;
            }

            //using (HashAlgorithm algorithm = new SHA256CryptoServiceProvider())
            //{
            //    byte[] inputBytes = Encoding.UTF8.GetBytes(input);
            //    byte[] hashBytes = algorithm.ComputeHash(inputBytes);
            //    return BitConverter.ToString(hashBytes).Replace("-", String.Empty);
            //}

            using (FileStream fileInStream = File.OpenRead(FileName))
            using (BufferedStream bufferedStream = new BufferedStream(fileInStream, 0x1000))
            using (SHA256 sha = SHA256.Create())
            {
                byte[] checksum = sha.ComputeHash(bufferedStream);

                return BitConverter.ToString(checksum).Replace("-", string.Empty, StringComparison.OrdinalIgnoreCase);
            }
        }

        public static string GetPath(string FileName)
        {
            return $"{Build.BuildWorkingFolder}\\tools\\CustomSignTool\\Resources\\{FileName}";
        }

        public static string GetKeyTempPath(string FileName)
        {
            var filename = Path.ChangeExtension(FileName, ".key");
            var basename = Path.GetFileName(filename);

            return $"{Build.BuildWorkingFolder}\\tools\\CustomSignTool\\Resources\\{basename}";
        }

        public static string GetSignToolPath()
        {
            if (Environment.Is64BitOperatingSystem)
                return $"{Build.BuildWorkingFolder}\\tools\\CustomSignTool\\bin\\Release64\\CustomSignTool.exe";
            return $"{Build.BuildWorkingFolder}\\tools\\CustomSignTool\\bin\\Release32\\CustomSignTool.exe";
        }

        public static string CreateSignatureString(string KeyFile, string SignFile)
        {
            if (!File.Exists(GetSignToolPath()))
            {
                Program.PrintColorMessage($"[CreateSignature - SignTool not found]", ConsoleColor.Red, true, BuildFlags.BuildVerbose);
                return null;
            }

            if (!File.Exists(KeyFile))
            {
                Program.PrintColorMessage($"[CreateSignature - Key not found]", ConsoleColor.Red, true, BuildFlags.BuildVerbose);
                return null;
            }

            int errorcode = Win32.CreateProcess(
                GetSignToolPath(),
                $"sign -k {KeyFile} {SignFile} -h",
                out string outstring
                );

            if (errorcode != 0)
            {
                Program.PrintColorMessage($"[CreateSignature] ({errorcode}) {outstring}", ConsoleColor.Red, true, BuildFlags.BuildVerbose);
                return null;
            }

            if (string.IsNullOrWhiteSpace(outstring))
            {
                Program.PrintColorMessage($"[CreateSignature - Empty]", ConsoleColor.Red, true, BuildFlags.BuildVerbose);
                return null;
            }

            return outstring;
        }

        public static bool CreateSignatureFile(string KeyFile, string SignFile, bool FailIfExists = false)
        {
            string sigfile = Path.ChangeExtension(SignFile, ".sig");

            if (FailIfExists)
            {
                if (File.Exists(sigfile) && new FileInfo(sigfile).Length != 0)
                {
                    Program.PrintColorMessage($"[CreateSignature - FailIfExists] ({sigfile})", ConsoleColor.Red, true, BuildFlags.BuildVerbose);
                    return false;
                }
            }

            File.WriteAllText(sigfile, string.Empty);

            if (!File.Exists(GetSignToolPath()))
            {
                Program.PrintColorMessage($"[CreateSignature - SignTool not found]", ConsoleColor.Red, true, BuildFlags.BuildVerbose);
                return false;
            }

            if (!File.Exists(KeyFile))
            {
                Program.PrintColorMessage($"[CreateSignature - Key not found]", ConsoleColor.Red, true, BuildFlags.BuildVerbose);
                return false;
            }
            
            int errorcode = Win32.CreateProcess(
                GetSignToolPath(),
                $"sign -k {KeyFile} {SignFile} -s {sigfile}",
                out string outstring
                );

            if (errorcode != 0)
            {
                Program.PrintColorMessage($"[CreateSignature] ({errorcode}) {outstring}", ConsoleColor.Red, true, BuildFlags.BuildVerbose);
                return false;
            }

            if (!string.IsNullOrWhiteSpace(outstring))
            {
                Program.PrintColorMessage($"[CreateSignature - Empty]", ConsoleColor.Red, true, BuildFlags.BuildVerbose);
                return false;
            }

            return true;
        }
    }

    public static class Utils
    {
        private static readonly string[] MsBuildPathArray =
        {
            "\\MSBuild\\Current\\Bin\\amd64\\MSBuild.exe",
            "\\MSBuild\\Current\\Bin\\MSBuild.exe",
            "\\MSBuild\\15.0\\Bin\\MSBuild.exe"
        };

        private static string MsBuildFilePath;
        private static string VsWhereFilePath;

        public static string GetOutputDirectoryPath()
        {
            return $"{Build.BuildWorkingFolder}\\build\\output";
        }

        public static void CreateOutputDirectory()
        {
            string folder = GetOutputDirectoryPath();

            if (File.Exists(folder))
                return;

            try
            {
                Directory.CreateDirectory(folder);
            }
            catch (Exception ex)
            {
                Program.PrintColorMessage($"Error creating output directory: {ex}", ConsoleColor.Red);
            }
        }

        public static string GetVswhereFilePath()
        {
            if (string.IsNullOrWhiteSpace(VsWhereFilePath))
            {
                string[] vswherePathArray =
                {
                    "%ProgramFiles%\\Microsoft Visual Studio\\Installer\\vswhere.exe",
                    "%ProgramFiles(x86)%\\Microsoft Visual Studio\\Installer\\vswhere.exe",
                    "%ProgramW6432%\\Microsoft Visual Studio\\Installer\\vswhere.exe"
                };

                foreach (string path in vswherePathArray)
                {
                    string file = Environment.ExpandEnvironmentVariables(path);

                    if (File.Exists(file))
                    {
                        VsWhereFilePath = file;
                        break;
                    }
                }

                {
                    string file = Win32.SearchFile("vswhere.exe");

                    if (File.Exists(file))
                    {
                        VsWhereFilePath = file;
                    }
                }
            }

            return VsWhereFilePath;
        }

        public static string GetMsbuildFilePath()
        {
            if (string.IsNullOrWhiteSpace(MsBuildFilePath))
            {
                VisualStudioInstance instance = VisualStudio.GetVisualStudioInstance();

                if (instance != null)
                {
                    foreach (string path in MsBuildPathArray)
                    {
                        string file = instance.Path + path;

                        if (File.Exists(file))
                        {
                            MsBuildFilePath = file;
                            break;
                        }
                    }
                }
            }

            if (string.IsNullOrWhiteSpace(MsBuildFilePath))
            {
                string vswhere = GetVswhereFilePath();

                if (string.IsNullOrWhiteSpace(vswhere))
                    return null;

                // -latest -requires Microsoft.Component.MSBuild -find "MSBuild\**\Bin\MSBuild.exe"
                string vswhereResult = Win32.ShellExecute(
                    vswhere,
                    "-latest " +
                    "-prerelease " +
                    "-products * " +
                    "-requiresAny " +
                    "-requires Microsoft.Component.MSBuild " +
                    "-property installationPath "
                    );

                if (!string.IsNullOrWhiteSpace(vswhereResult))
                {
                    foreach (string path in MsBuildPathArray)
                    {
                        string file = vswhereResult + path;

                        if (File.Exists(file))
                        {
                            MsBuildFilePath = file;
                            break;
                        }
                    }
                }
            }

            return MsBuildFilePath;
        }

        public static string GetGitFilePath()
        {
            string[] gitPathArray =
            {
                "%ProgramFiles%\\Git\\bin\\git.exe",
                "%ProgramFiles(x86)%\\Git\\bin\\git.exe",
                "%ProgramW6432%\\Git\\bin\\git.exe"
            };

            foreach (string path in gitPathArray)
            {
                string file = Environment.ExpandEnvironmentVariables(path);

                if (File.Exists(file))
                    return file;
            }

            {
                string file = Win32.SearchFile("git.exe");

                if (File.Exists(file))
                    return file;
            }

            return null;
        }

        public static string GetGitWorkPath()
        {
            if (Directory.Exists($"{Build.BuildWorkingFolder}\\.git"))
            {
                return $"--git-dir=\"{Build.BuildWorkingFolder}\\.git\" --work-tree=\"{Build.BuildWorkingFolder}\" ";
            }

            return string.Empty;
        }

        public static string GetWindowsSdkIncludePath()
        {
            List<KeyValuePair<Version, string>> versionList = new List<KeyValuePair<Version, string>>();
            string kitsRoot = Win32.GetKeyValue(true, "Software\\Microsoft\\Windows Kits\\Installed Roots", "KitsRoot10", "%ProgramFiles(x86)%\\Windows Kits\\10\\");
            string kitsPath = Environment.ExpandEnvironmentVariables($"{kitsRoot}\\Include");

            if (Directory.Exists(kitsPath))
            {
                var windowsKitsDirectory = Directory.EnumerateDirectories(kitsPath);

                foreach (string path in windowsKitsDirectory)
                {
                    string name = Path.GetFileName(path);

                    if (Version.TryParse(name, out var version))
                    {
                        versionList.Add(new KeyValuePair<Version, string>(version, path));
                    }
                }

                versionList.Sort((first, second) => first.Key.CompareTo(second.Key));

                if (versionList.Count > 0)
                {
                    var result = versionList[versionList.Count - 1];

                    if (!string.IsNullOrWhiteSpace(result.Value))
                    {
                        return result.Value;
                    }
                }
            }

            return string.Empty;
        }

        public static string GetWindowsSdkPath()
        {
            List<KeyValuePair<Version, string>> versionList = new List<KeyValuePair<Version, string>>();
            string kitsRoot = Win32.GetKeyValue(true, "Software\\Microsoft\\Windows Kits\\Installed Roots", "KitsRoot10", "%ProgramFiles(x86)%\\Windows Kits\\10\\");
            string kitsPath = Environment.ExpandEnvironmentVariables($"{kitsRoot}\\bin");

            if (Directory.Exists(kitsPath))
            {
                var windowsKitsDirectory = Directory.EnumerateDirectories(kitsPath);

                foreach (string path in windowsKitsDirectory)
                {
                    string name = Path.GetFileName(path);

                    if (Version.TryParse(name, out var version))
                    {
                        versionList.Add(new KeyValuePair<Version, string>(version, path));
                    }
                }

                versionList.Sort((first, second) => first.Key.CompareTo(second.Key));

                if (versionList.Count > 0)
                {
                    var result = versionList[versionList.Count - 1];

                    if (!string.IsNullOrWhiteSpace(result.Value))
                    {
                        return result.Value;
                    }
                }
            }

            return string.Empty;
        }

        public static string GetWindowsSdkVersion()
        {
            List<string> versions = new List<string>();
            string kitsRoot = Win32.GetKeyValue(true, "Software\\Microsoft\\Windows Kits\\Installed Roots", "KitsRoot10", "%ProgramFiles(x86)%\\Windows Kits\\10\\");
            string kitsPath = Environment.ExpandEnvironmentVariables($"{kitsRoot}\\bin");

            if (Directory.Exists(kitsPath))
            {
                var windowsKitsDirectory = Directory.EnumerateDirectories(kitsPath);

                foreach (string path in windowsKitsDirectory)
                {
                    string name = Path.GetFileName(path);

                    if (Version.TryParse(name, out var version))
                    {
                        versions.Add(name);
                    }
                }

                versions.Sort((p1, p2) =>
                {
                    if (Version.TryParse(p1, out Version v1) && Version.TryParse(p2, out Version v2))
                        return v1.CompareTo(v2);
                    else
                        return 1;
                });

                if (versions.Count > 0)
                {
                    string result = versions[^1];

                    if (!string.IsNullOrWhiteSpace(result))
                    {
                        return result;
                    }
                }
            }

            return string.Empty;
        }

        public static string GetVisualStudioVersion()
        {
            string msbuild = GetMsbuildFilePath();

            if (string.IsNullOrWhiteSpace(msbuild))
                return string.Empty;

            try
            {
                DirectoryInfo info = new DirectoryInfo(Path.GetDirectoryName(msbuild));

                while (info.Parent != null && info.Parent.Parent != null)
                {
                    info = info.Parent;

                    if (File.Exists($"{info.FullName}\\Common7\\IDE\\devenv.exe"))
                    {
                        FileVersionInfo currentInfo = FileVersionInfo.GetVersionInfo($"{info.FullName}\\Common7\\IDE\\devenv.exe");
                        return currentInfo.ProductVersion;
                    }
                }
            }
            catch (Exception ex)
            {
                Program.PrintColorMessage($"Unable to find devenv directory: {ex}", ConsoleColor.Red);
            }

            return string.Empty;
        }

        public static string GetMakeAppxPath()
        {
            string windowsSdkPath = Utils.GetWindowsSdkPath();

            if (string.IsNullOrWhiteSpace(windowsSdkPath))
                return string.Empty;

            string makeAppxPath = $"{windowsSdkPath}\\x64\\MakeAppx.exe";

            if (string.IsNullOrWhiteSpace(makeAppxPath))
                return string.Empty;

            if (!File.Exists(makeAppxPath))
                return string.Empty;

            return makeAppxPath;
        }

        public static string GetSignToolPath()
        {
            string windowsSdkPath = Utils.GetWindowsSdkPath();

            if (string.IsNullOrWhiteSpace(windowsSdkPath))
                return string.Empty;

            string signToolPath = $"{windowsSdkPath}\\x64\\SignTool.exe";

            if (string.IsNullOrWhiteSpace(signToolPath))
                return string.Empty;

            if (!File.Exists(signToolPath))
                return string.Empty;

            return signToolPath;
        }

        //public static string GetMsbuildFilePath()
        //{
        //    VisualStudioInstance instance;
        //    string vswhere;
        //    string vswhereResult;
        //
        //    instance = GetVisualStudioInstance();
        //
        //    if (instance != null)
        //    {
        //        foreach (string path in MsBuildPathArray)
        //        {
        //            string file = instance.Path + path;
        //
        //            if (File.Exists(file))
        //            {
        //                return file;
        //            }
        //        }
        //    }
        //
        //    vswhere = GetVswhereFilePath();
        //
        //    if (string.IsNullOrWhiteSpace(vswhere))
        //        return null;
        //
        //    vswhereResult = Win32.ShellExecute(
        //        vswhere,
        //        "-latest " +
        //        "-prerelease " +
        //        "-products * " +
        //        "-requiresAny " +
        //        "-requires Microsoft.Component.MSBuild " +
        //        "-property installationPath "
        //        );
        //
        //    if (string.IsNullOrWhiteSpace(vswhereResult))
        //        return null;
        //
        //    foreach (string path in MsBuildPathArray)
        //    {
        //        string file = vswhereResult + path;
        //
        //        if (File.Exists(file))
        //        {
        //            return file;
        //        }
        //    }
        //
        //    return null;
        //}
    }

    public static class AppVeyor
    {
        public static bool UpdateBuildVersion(string BuildVersion)
        {
            try
            {
                string AppVeyorFilePath = Win32.SearchFile("appveyor.exe");

                if (!string.IsNullOrWhiteSpace(AppVeyorFilePath) && !string.IsNullOrWhiteSpace(BuildVersion))
                {
                    Win32.ShellExecute("appveyor", $"UpdateBuild -Version \"{BuildVersion}\" ");
                    return true;
                }
            }
            catch (Exception ex)
            {
                Program.PrintColorMessage($"[UpdateBuildVersion] {ex}", ConsoleColor.Red);
            }

            return false;
        }

        public static bool UploadFile(string FileName)
        {
            try
            {
                string AppVeyorFilePath = Win32.SearchFile("appveyor.exe");

                if (!string.IsNullOrWhiteSpace(AppVeyorFilePath) && !string.IsNullOrWhiteSpace(FileName))
                {
                    Win32.ShellExecute("appveyor", $"PushArtifact \"{FileName}\" ");
                    return true;
                }
            }
            catch (Exception ex)
            {
                Program.PrintColorMessage($"[UploadFile] {ex}", ConsoleColor.Red);
            }

            return false;
        }
    }

    public class BuildUpdateRequest
    {
        [JsonPropertyName("build_id")] public string BuildId { get; set; }
        [JsonPropertyName("build_version")] public string BuildVersion { get; set; }
        [JsonPropertyName("build_commit")] public string BuildCommit { get; set; }
        [JsonPropertyName("build_updated")] public string BuildUpdated { get; set; }

        [JsonPropertyName("bin_url")] public string BinUrl { get; set; }
        [JsonPropertyName("bin_length")] public string BinLength { get; set; }
        [JsonPropertyName("bin_hash")] public string BinHash { get; set; }
        [JsonPropertyName("bin_sig")] public string BinSig { get; set; }

        [JsonPropertyName("setup_url")] public string SetupUrl { get; set; }
        [JsonPropertyName("setup_length")] public string SetupLength { get; set; }
        [JsonPropertyName("setup_hash")] public string SetupHash { get; set; }
        [JsonPropertyName("setup_sig")] public string SetupSig { get; set; }
    }

    public class GithubReleasesRequest
    {
        [JsonPropertyName("tag_name")] public string ReleaseTag { get; set; }
        [JsonPropertyName("target_commitish")] public string Branch { get; set; }
        [JsonPropertyName("name")] public string Name { get; set; }
        [JsonPropertyName("body")] public string Description { get; set; }

        [JsonPropertyName("draft")] public bool Draft { get; set; }
        [JsonPropertyName("prerelease")] public bool Prerelease { get; set; }
        [JsonPropertyName("generate_release_notes")] public bool GenerateReleaseNotes { get; set; }

        public override string ToString()
        {
            return this.ReleaseTag;
        }
    }

    public class GithubReleasesResponse
    {
        [JsonPropertyName("id")] public long ID { get; set; }
        [JsonPropertyName("upload_url")] public string UploadUrl { get; set; }
        [JsonPropertyName("html_url")] public string HtmlUrl { get; set; }
        [JsonPropertyName("assets")] public List<GithubAssetsResponse> Assets { get; set; }
        [JsonIgnore] public string ReleaseId { get { return this.ID.ToString(); } }

        public override string ToString()
        {
            return this.ReleaseId;
        }
    }

    public class GithubAssetsResponse
    {
        [JsonPropertyName("name")] public string Name { get; set; }
        [JsonPropertyName("label")] public string Label { get; set; }
        [JsonPropertyName("size")] public long Size { get; set; }
        [JsonPropertyName("state")] public string State { get; set; }
        [JsonPropertyName("browser_download_url")] public string DownloadUrl { get; set; }

        [JsonIgnore]
        public bool Uploaded
        {
            get
            {
                return !string.IsNullOrWhiteSpace(this.State) && string.Equals(this.State, "uploaded", StringComparison.OrdinalIgnoreCase);
            }
        }

        public override string ToString()
        {
            return this.Name;
        }
    }

    [JsonSourceGenerationOptions(DefaultIgnoreCondition = JsonIgnoreCondition.WhenWritingNull, GenerationMode = JsonSourceGenerationMode.Serialization)]
    [JsonSerializable(typeof(BuildUpdateRequest))]
    public partial class BuildUpdateRequestContext : JsonSerializerContext
    {

    }

    [JsonSourceGenerationOptions(DefaultIgnoreCondition = JsonIgnoreCondition.WhenWritingNull, GenerationMode = JsonSourceGenerationMode.Serialization)]
    [JsonSerializable(typeof(GithubReleasesRequest))]
    public partial class GithubReleasesRequestContext : JsonSerializerContext
    {

    }

    [JsonSourceGenerationOptions(DefaultIgnoreCondition = JsonIgnoreCondition.WhenWritingNull, GenerationMode = JsonSourceGenerationMode.Metadata)]
    [JsonSerializable(typeof(GithubReleasesResponse))]
    public partial class GithubReleasesResponseContext : JsonSerializerContext
    {

    }

    [JsonSourceGenerationOptions(DefaultIgnoreCondition = JsonIgnoreCondition.WhenWritingNull, GenerationMode = JsonSourceGenerationMode.Metadata)]
    [JsonSerializable(typeof(GithubAssetsResponse))]
    public partial class GithubAssetsResponseContext : JsonSerializerContext
    {

    }

    public static class Extextensions
    {
        private const long OneKb = 1024;
        private const long OneMb = OneKb * 1024;
        private const long OneGb = OneMb * 1024;
        private const long OneTb = OneGb * 1024;

        public static string ToPrettySize(this int value, int decimalPlaces = 0)
        {
            return ((long)value).ToPrettySize(decimalPlaces);
        }

        public static string ToPrettySize(this long value, int decimalPlaces = 0)
        {
            double asTb = Math.Round((double)value / OneTb, decimalPlaces);
            double asGb = Math.Round((double)value / OneGb, decimalPlaces);
            double asMb = Math.Round((double)value / OneMb, decimalPlaces);
            double asKb = Math.Round((double)value / OneKb, decimalPlaces);
            string chosenValue = asTb > 1 ? string.Format("{0}Tb", asTb)
                : asGb > 1 ? string.Format("{0}Gb", asGb)
                : asMb > 1 ? string.Format("{0}Mb", asMb)
                : asKb > 1 ? string.Format("{0}Kb", asKb)
                : string.Format("{0}B", Math.Round((double)value, decimalPlaces));
            return chosenValue;
        }
    }
}
