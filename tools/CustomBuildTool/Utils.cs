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
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Runtime.InteropServices;
using System.Security.Cryptography;
using System.Text;
using System.Text.Json.Serialization;
using System.Threading;

namespace CustomBuildTool
{
    [System.Security.SuppressUnmanagedCodeSecurity]
    public static class Win32
    {
        public static void CreateDirectory(string FileName)
        {
            if (!Directory.Exists(FileName))
            {
                Directory.CreateDirectory(FileName);
            }
        }

        public static int CreateProcess(string FileName, string Arguments, out string outputstring)
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
            catch (Exception)
            {

            }

            outputstring = output.ToString() + error.ToString();
            outputstring = outputstring.Replace("\n\n", "\r\n", StringComparison.OrdinalIgnoreCase).Trim();
            outputstring = outputstring.Replace("\r\n", string.Empty, StringComparison.OrdinalIgnoreCase).Trim();

            return exitcode;
        }

        public static string ShellExecute(string FileName, string Arguments)
        {
            Win32.CreateProcess(FileName, Arguments, out string outputstring);

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
            catch (Exception) { }

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
            catch (Exception) { }

            return string.Empty;
        }

        private static readonly UIntPtr HKEY_LOCAL_MACHINE = new UIntPtr(0x80000002u);
        private static readonly UIntPtr HKEY_CURRENT_USER = new UIntPtr(0x80000001u);
        private static readonly uint KEY_READ = 0x20019u;

        [DllImport("advapi32.dll", CharSet = CharSet.Unicode, ExactSpelling = true)]
        private static extern uint RegOpenKeyExW(UIntPtr RootKeyHandle, string KeyName, uint Options, uint AccessMask, out UIntPtr KeyHandle);
        [DllImport("advapi32.dll", CharSet = CharSet.Unicode, ExactSpelling = true)]
        private static extern uint RegQueryValueExW(UIntPtr KeyHandle, string ValueName, IntPtr Reserved, out int DataType, IntPtr DataBuffer, ref int DataLength);
        [DllImport("advapi32.dll", ExactSpelling = true)]
        private static extern uint RegCloseKey(UIntPtr KeyHandle);

        public static string GetKeyValue(bool LocalMachine, string KeyName, string ValueName, string DefaultValue)
        {
            string value = string.Empty;
            UIntPtr keyHandle;
            IntPtr valueBuffer;

            if (RegOpenKeyExW(
                LocalMachine ? HKEY_LOCAL_MACHINE : HKEY_CURRENT_USER,
                KeyName,
                0,
                KEY_READ,
                out keyHandle
                ) == 0)
            {
                int valueType;
                int valueLength = 0;

                RegQueryValueExW(
                    keyHandle,
                    ValueName,
                    IntPtr.Zero,
                    out valueType,
                    IntPtr.Zero,
                    ref valueLength
                    );

                if (valueType == 1 || valueLength > 4)
                {
                    valueBuffer = Marshal.AllocHGlobal(valueLength);

                    if (RegQueryValueExW(
                        keyHandle,
                        ValueName,
                        IntPtr.Zero,
                        out valueType,
                        valueBuffer,
                        ref valueLength
                        ) == 0)
                    {
                        value = Marshal.PtrToStringUni(valueBuffer, valueLength / 2 - 1);
                    }

                    Marshal.FreeHGlobal(valueBuffer);
                }

                RegCloseKey(keyHandle);
            }

            return string.IsNullOrWhiteSpace(value) ? DefaultValue : value;
        }
    }

    public static class Verify
    {
        private static Aes GetRijndael(string secret)
        {
            using (Rfc2898DeriveBytes rfc2898DeriveBytes = new Rfc2898DeriveBytes(
                secret,
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

        public static void Encrypt(string fileName, string outFileName, string secret)
        {
            using (FileStream fileOutStream = File.Create(outFileName))
            using (Aes rijndael = GetRijndael(secret))
            using (FileStream fileStream = File.OpenRead(fileName))
            using (CryptoStream cryptoStream = new CryptoStream(fileOutStream, rijndael.CreateEncryptor(), CryptoStreamMode.Write, true))
            {
                fileStream.CopyTo(cryptoStream);
            }
        }

        public static bool Decrypt(string FileName, string outFileName, string secret)
        {
            try
            {
                using (FileStream fileOutStream = File.Create(outFileName))
                using (Aes rijndael = GetRijndael(secret))
                using (FileStream fileStream = File.OpenRead(FileName))
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

        private static Aes GetRijndaelLegacy(string secret)
        {
            using (Rfc2898DeriveBytes rfc2898DeriveBytes = new Rfc2898DeriveBytes(
                secret,
                Convert.FromBase64String("e0U0RTY2RjU5LUNBRjItNEMzOS1BN0Y4LTQ2MDk3QjFDNDYxQn0="),
                10000))
            {
                Aes rijndael = Aes.Create();

                rijndael.Key = rfc2898DeriveBytes.GetBytes(32);
                rijndael.IV = rfc2898DeriveBytes.GetBytes(16);

                return rijndael;
            }
        }

        public static bool DecryptLegacy(string FileName, string outFileName, string secret)
        {
            try
            {
                using (FileStream fileOutStream = File.Create(outFileName))
                using (Aes rijndael = GetRijndaelLegacy(secret))
                using (FileStream fileStream = File.OpenRead(FileName))
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

        private static readonly string[] CustomSignToolPathArray =
        {
            "\\tools\\CustomSignTool\\bin\\Release64\\CustomSignTool.exe",
            "\\tools\\CustomSignTool\\bin\\Release32\\CustomSignTool.exe",
            //"\\tools\\CustomSignTool\\bin\\Debug64\\CustomSignTool.exe",
            //"\\tools\\CustomSignTool\\bin\\Debug32\\CustomSignTool.exe",
        };

        public static string GetPath(string FileName)
        {
            return "tools\\CustomSignTool\\Resources\\" + FileName;
        }

        public static string GetCustomSignToolFilePath()
        {
            foreach (string path in CustomSignToolPathArray)
            {
                string file = Environment.CurrentDirectory + path;

                if (File.Exists(file))
                    return file;
            }

            return null;
        }
    }

    public static class VisualStudio
    {
        private static readonly string[] MsBuildPathArray =
        {
            "\\MSBuild\\Current\\Bin\\amd64\\MSBuild.exe",
            "\\MSBuild\\Current\\Bin\\MSBuild.exe",
            "\\MSBuild\\15.0\\Bin\\MSBuild.exe"
        };

        private static string MsBuildFilePath;
        private static VisualStudioInstance VisualStudioInstance;
        private static List<VisualStudioInstance> VisualStudioInstanceList;

        public static string GetOutputDirectoryPath()
        {
            return Environment.CurrentDirectory + "\\build\\output";
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
                Program.PrintColorMessage("Error creating output directory: " + ex, ConsoleColor.Red);
            }
        }

        public static VisualStudioInstance GetVisualStudioInstance()
        {
            if (VisualStudioInstanceList == null)
            {
                VisualStudioInstanceList = new List<VisualStudioInstance>();

                try
                {
                    if (new SetupConfigurationClass() is ISetupConfiguration2 setupConfiguration)
                    {
                        IEnumSetupInstances instanceEnumerator = setupConfiguration.EnumAllInstances();
                        ISetupInstance2[] instances = new ISetupInstance2[3];

                        instanceEnumerator.Next(instances.Length, instances, out var instancesFetched);

                        if (instancesFetched == 0)
                            return null;

                        do
                        {
                            for (int i = 0; i < instancesFetched; i++)
                                VisualStudioInstanceList.Add(new VisualStudioInstance(instances[i]));

                            instanceEnumerator.Next(instances.Length, instances, out instancesFetched);
                        }
                        while (instancesFetched != 0);
                    }
                }
                catch { }

                VisualStudioInstanceList.Sort((p1, p2) =>
                {
                    if (Version.TryParse(p1.InstallationVersion, out Version version1) && Version.TryParse(p2.InstallationVersion, out Version version2))
                    {
                        if (version1 < version2)
                            return 1;
                        else if (version1 > version2)
                            return -1;

                        return version1.CompareTo(version2);
                    }

                    return 1;
                });
            }

            if (VisualStudioInstance == null)
            {
                foreach (VisualStudioInstance instance in VisualStudioInstanceList)
                {
                    if (instance.HasRequiredDependency)
                    {
                        VisualStudioInstance = instance;
                        break;
                    }
                }
            }

            return VisualStudioInstance;
        }

        public static string GetVswhereFilePath()
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
                    return file;
            }

            {
                string file = Win32.SearchFile("vswhere.exe");

                if (File.Exists(file))
                    return file;
            }

            return null;
        }

        public static string GetMsbuildFilePath()
        {
            if (string.IsNullOrWhiteSpace(MsBuildFilePath))
            {
                VisualStudioInstance instance = GetVisualStudioInstance();

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
            if (Directory.Exists(Environment.CurrentDirectory + "\\.git"))
            {
                return "--git-dir=\"" + Environment.CurrentDirectory + "\\.git\" --work-tree=\"" + Environment.CurrentDirectory + "\" ";
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
                Program.PrintColorMessage("[UpdateBuildVersion] " + ex, ConsoleColor.Red, true);
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
                Program.PrintColorMessage("[UploadFile] " + ex, ConsoleColor.Red, true);
            }

            return false;
        }
    }

    public class VisualStudioInstance : IComparable, IComparable<VisualStudioInstance>
    {
        public string Name { get; }
        public string Path { get; }
        public string InstallationVersion { get; }
        public string DisplayName { get; }
        public string ResolvePath { get; }
        public string ProductPath { get; }
        public string InstanceId { get; }
        public InstanceState State { get; }
        public List<VisualStudioPackage> Packages { get; }

        public VisualStudioInstance()
        {
            this.Packages = new List<VisualStudioPackage>();
        }

        public VisualStudioInstance(ISetupInstance2 FromInstance)
        {
            this.Packages = new List<VisualStudioPackage>();
            this.Name = FromInstance.GetInstallationName();
            this.Path = FromInstance.GetInstallationPath();
            this.InstallationVersion = FromInstance.GetInstallationVersion();
            this.DisplayName = FromInstance.GetDisplayName();
            this.ResolvePath = FromInstance.ResolvePath();
            this.InstanceId = FromInstance.GetInstanceId();
            this.ProductPath = FromInstance.GetProductPath();
            this.State = FromInstance.GetState();

            var packages = FromInstance.GetPackages();
            foreach (var item in packages)
            {
                this.Packages.Add(new VisualStudioPackage(item));
            }
        }

        private VisualStudioPackage GetLatestSdkPackage()
        {
            var found = this.Packages.FindAll(p =>
            {
                return p.Id.StartsWith("Microsoft.VisualStudio.Component.Windows10SDK", StringComparison.OrdinalIgnoreCase) ||
                    p.Id.StartsWith("Microsoft.VisualStudio.Component.Windows11SDK", StringComparison.OrdinalIgnoreCase);
            });

            if (found == null)
                return null;
            if (found.Count == 0)
                return null;

            found.Sort((p1, p2) =>
            {
                if (Version.TryParse(p1.Version, out Version v1) && Version.TryParse(p2.Version, out Version v2))
                    return v1.CompareTo(v2);
                else
                    return 1;
            });

            return found[^1];
        }

        public string GetWindowsSdkVersion()
        {
            List<string> versions = new List<string>();
            string kitpath32 = Environment.ExpandEnvironmentVariables("%ProgramFiles(x86)%\\Windows Kits\\10\\bin");

            if (Directory.Exists(kitpath32))
            {
                var windowsKitsDirectory = Directory.EnumerateDirectories(kitpath32);

                foreach (string path in windowsKitsDirectory)
                {
                    string name = System.IO.Path.GetFileName(path);

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

            {
                VisualStudioPackage package = GetLatestSdkPackage();

                if (package == null)
                    return string.Empty;

                // SDK
                if (package.Id.Length > "Microsoft.VisualStudio.Component.Windows10SDK.".Length)
                {
                    return package.Id.Substring("Microsoft.VisualStudio.Component.Windows10SDK.".Length);
                }
                else
                {
                    var found = this.Packages.FindAll(p =>
                    {
                        return p.Id.StartsWith("Microsoft.VisualStudio.Component.Windows10SDK", StringComparison.OrdinalIgnoreCase) ||
                            p.Id.StartsWith("Microsoft.VisualStudio.Component.Windows11SDK", StringComparison.OrdinalIgnoreCase);
                    });

                    foreach (var sdk in found)
                    {
                        string[] tokens = sdk.Id.Split(".", StringSplitOptions.RemoveEmptyEntries | StringSplitOptions.TrimEntries);

                        foreach (string name in tokens)
                        {
                            if (uint.TryParse(name, out uint version))
                            {
                                versions.Add(name);
                                break;
                            }
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
                        return versions[^1];
                    }
                    else
                    {
                        return package.UniqueId;
                    }
                }
            }
        }

        public string GetWindowsSdkFullVersion()
        {
            VisualStudioPackage package = GetLatestSdkPackage();

            if (package == null)
                return string.Empty;

            return package.Version;
        }

        public bool HasRequiredDependency
        {
            get
            {
                bool hasbuild = false;
                bool hasruntimes = false;
                bool haswindowssdk = false;
                //string[] vctoolsPackageArray =
                //{
                //    "Microsoft.Component.MSBuild",
                //    "Microsoft.VisualStudio.Component.Windows10SDK.19041",
                //    "Microsoft.VisualStudio.Component.VC.Tools.x86.x64",
                //    "Microsoft.VisualStudio.Component.VC.14.20.x86.x64",
                //    "Microsoft.VisualStudio.Component.VC.14.21.x86.x64",
                //    "Microsoft.VisualStudio.Component.VC.14.22.x86.x64",
                //};

                // Microsoft.VisualStudio.Component.VC.Redist.14.Latest
                // Microsoft.VisualStudio.Component.VC.Tools.x86.x64

                hasbuild = this.Packages.Exists(p => p.Id.StartsWith("Microsoft.Component.MSBuild", StringComparison.OrdinalIgnoreCase));
                hasruntimes = this.Packages.Exists(p => p.Id.StartsWith("Microsoft.VisualStudio.Component.VC", StringComparison.OrdinalIgnoreCase));
                haswindowssdk = this.Packages.Exists(p =>
                {
                    return p.Id.StartsWith("Microsoft.VisualStudio.Component.Windows10SDK", StringComparison.OrdinalIgnoreCase) ||
                        p.Id.StartsWith("Microsoft.VisualStudio.Component.Windows11SDK", StringComparison.OrdinalIgnoreCase);
                });

                return hasbuild && hasruntimes && haswindowssdk;
            }
        }

        public string MissingDependencyList
        {
            get
            {
                string list = string.Empty;
                //string[] vctoolsPackageArray =
                //{
                //    "Microsoft.Component.MSBuild",
                //    "Microsoft.VisualStudio.Component.Windows10SDK.19041",
                //    "Microsoft.VisualStudio.Component.VC.Tools.x86.x64",
                //    "Microsoft.VisualStudio.Component.VC.14.20.x86.x64",
                //    "Microsoft.VisualStudio.Component.VC.14.21.x86.x64",
                //    "Microsoft.VisualStudio.Component.VC.14.22.x86.x64",
                //};

                if (!this.Packages.Exists(p => p.Id.StartsWith("Microsoft.Component.MSBuild", StringComparison.OrdinalIgnoreCase)))
                {
                    list += "MSBuild" + Environment.NewLine;
                }

                if (!this.Packages.Exists(p => p.Id.StartsWith("Microsoft.VisualStudio.Component.VC", StringComparison.OrdinalIgnoreCase)))
                {
                    list += "VC.14.Redist" + Environment.NewLine;
                }

                if (!this.Packages.Exists(p =>
                {
                    return p.Id.StartsWith("Microsoft.VisualStudio.Component.Windows10SDK", StringComparison.OrdinalIgnoreCase) ||
                        p.Id.StartsWith("Microsoft.VisualStudio.Component.Windows11SDK", StringComparison.OrdinalIgnoreCase);
                }))
                {
                    list += "Windows SDK" + Environment.NewLine;
                }

                return list;
            }
        }

        public override string ToString()
        {
            return this.DisplayName;
        }

        public int CompareTo(object obj)
        {
            if (obj == null)
                return 1;

            if (obj is VisualStudioInstance instance)
                return this.Name.CompareTo(instance.Name);
            else
                return 1;
        }

        public int CompareTo(VisualStudioInstance obj)
        {
            if (obj == null)
                return 1;

            return this.Name.CompareTo(obj.Name);
        }
    }

    public class VisualStudioPackage : IComparable, IComparable<VisualStudioPackage>
    {
        public string Id { get; }
        public string Version { get; }
        public string Type { get; }
        public string UniqueId { get; }

        public VisualStudioPackage(ISetupPackageReference FromPackage)
        {
            this.Id = FromPackage.GetId();
            this.Version = FromPackage.GetVersion();
            this.Type = FromPackage.GetType();
            this.UniqueId = FromPackage.GetUniqueId();
        }

        public override string ToString()
        {
            return this.Id;
        }

        public int CompareTo(object obj)
        {
            if (obj == null)
                return 1;

            if (obj is VisualStudioPackage package)
                return this.Id.CompareTo(package.Id);
            else
                return 1;
        }

        public int CompareTo(VisualStudioPackage obj)
        {
            if (obj == null)
                return 1;

            return this.Id.CompareTo(obj.Id);
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
        [JsonPropertyName("generate_release_notes")] public bool generate_release_notes { get; set; }

        public override string ToString()
        {
            return this.ReleaseTag;
        }
    }

    public class GithubReleasesResponse
    {
        [JsonPropertyName("id")] public long ID { get; set; }
        [JsonPropertyName("upload_url")] public string upload_url { get; set; }
        [JsonPropertyName("html_url")] public string html_url { get; set; }
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
        [JsonPropertyName("browser_download_url")] public string download_url { get; set; }

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

    [Flags]
    public enum ConsoleMode : uint
    {
        DEFAULT,
        ENABLE_PROCESSED_INPUT = 0x0001,
        ENABLE_LINE_INPUT = 0x0002,
        ENABLE_ECHO_INPUT = 0x0004,
        ENABLE_WINDOW_INPUT = 0x0008,
        ENABLE_MOUSE_INPUT = 0x0010,
        ENABLE_INSERT_MODE = 0x0020,
        ENABLE_QUICK_EDIT_MODE = 0x0040,
        ENABLE_EXTENDED_FLAGS = 0x0080,
        ENABLE_AUTO_POSITION = 0x0100,
        ENABLE_PROCESSED_OUTPUT = 0x0001,
        ENABLE_WRAP_AT_EOL_OUTPUT = 0x0002,
        ENABLE_VIRTUAL_TERMINAL_PROCESSING = 0x0004,
        DISABLE_NEWLINE_AUTO_RETURN = 0x0008,
        ENABLE_LVB_GRID_WORLDWIDE = 0x0010,
        ENABLE_VIRTUAL_TERMINAL_INPUT = 0x0200
    }

    [Flags]
    public enum InstanceState : uint
    {
        None = 0u,
        Local = 1u,
        Registered = 2u,
        NoRebootRequired = 4u,
        NoErrors = 8u,
        Complete = 4294967295u
    }

    [ComImport, ClassInterface(ClassInterfaceType.None), Guid("177F0C4A-1CD3-4DE7-A32C-71DBBB9FA36D")]
    public class SetupConfigurationClass
    {

    }

    [ComImport, CoClass(typeof(SetupConfigurationClass)), Guid("42843719-DB4C-46C2-8E7C-64F1816EFD5B")]
    public interface ISetupConfigurationClass : ISetupConfiguration2, ISetupConfiguration
    {

    }

    [ComImport, Guid("6380BCFF-41D3-4B2E-8B2E-BF8A6810C848"), InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
    public interface IEnumSetupInstances
    {
        void Next(
            [MarshalAs(UnmanagedType.U4)] [In] int celt,
            [MarshalAs(UnmanagedType.LPArray, ArraySubType = UnmanagedType.Interface)] [Out] ISetupInstance[] rgelt,
            [MarshalAs(UnmanagedType.U4)] out int pceltFetched
            );

        void Skip([MarshalAs(UnmanagedType.U4)] [In] int celt);
        void Reset();
        IEnumSetupInstances Clone();
    }

    [ComImport, Guid("42843719-DB4C-46C2-8E7C-64F1816EFD5B"), InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
    public interface ISetupConfiguration
    {
        IEnumSetupInstances EnumInstances();
        ISetupInstance GetInstanceForCurrentProcess();
        ISetupInstance GetInstanceForPath([In] string path);
    }

    [ComImport, Guid("26AAB78C-4A60-49D6-AF3B-3C35BC93365D"), InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
    public interface ISetupConfiguration2 : ISetupConfiguration
    {
        IEnumSetupInstances EnumAllInstances();
    }

    [ComImport, Guid("46DCCD94-A287-476A-851E-DFBC2FFDBC20"), InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
    public interface ISetupErrorState
    {
        ISetupFailedPackageReference[] GetFailedPackages();
        ISetupPackageReference[] GetSkippedPackages();
    }

    [ComImport, Guid("E73559CD-7003-4022-B134-27DC650B280F"), InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
    public interface ISetupFailedPackageReference : ISetupPackageReference
    {
        new string GetId();
        new string GetVersion();
        new string GetChip();
        new string GetLanguage();
        new string GetBranch();
        new string GetType();
        new string GetUniqueId();
        new bool GetIsExtension();
    }

    [ComImport, Guid("42B21B78-6192-463E-87BF-D577838F1D5C"), InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
    public interface ISetupHelper
    {
        /// <summary>
        /// Parses a dotted quad version string into a 64-bit unsigned integer.
        /// </summary>
        /// <param name="version">The dotted quad version string to parse, e.g. 1.2.3.4.</param>
        /// <returns>A 64-bit unsigned integer representing the version. You can compare this to other versions.</returns>
        ulong ParseVersion([In] string version);

        /// <summary>
        /// Parses a dotted quad version string into a 64-bit unsigned integer.
        /// </summary>
        /// <param name="versionRange">The string containing 1 or 2 dotted quad version strings to parse, e.g. [1.0,) that means 1.0.0.0 or newer.</param>
        /// <param name="minVersion">A 64-bit unsigned integer representing the minimum version, which may be 0. You can compare this to other versions.</param>
        /// <param name="maxVersion">A 64-bit unsigned integer representing the maximum version, which may be MAXULONGLONG. You can compare this to other versions.</param>
        void ParseVersionRange([In] string versionRange, out ulong minVersion, out ulong maxVersion);
    }

    [ComImport, Guid("B41463C3-8866-43B5-BC33-2B0676F7F42E"), InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
    public interface ISetupInstance
    {
        string GetInstanceId();
        System.Runtime.InteropServices.ComTypes.FILETIME GetInstallDate();
        string GetInstallationName();
        string GetInstallationPath();
        string GetInstallationVersion();
        string GetDisplayName([In] int lcid = 0);
        string GetDescription([In] int lcid = 0);
        string ResolvePath([In] string pwszRelativePath = null);
    }

    [ComImport, Guid("89143C9A-05AF-49B0-B717-72E218A2185C"), InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
    public interface ISetupInstance2 : ISetupInstance
    {
        new string GetInstanceId();
        new System.Runtime.InteropServices.ComTypes.FILETIME GetInstallDate();
        new string GetInstallationName();
        new string GetInstallationPath();
        new string GetInstallationVersion();
        new string GetDisplayName([MarshalAs(UnmanagedType.U4)] [In] int lcid = 0);
        new string GetDescription([MarshalAs(UnmanagedType.U4)] [In] int lcid = 0);
        new string ResolvePath([MarshalAs(21)] [In] string pwszRelativePath = null);
        InstanceState GetState();
        ISetupPackageReference[] GetPackages();
        ISetupPackageReference GetProduct();
        string GetProductPath();
        ISetupErrorState GetErrors();
        bool IsLaunchable();
        bool IsComplete();
        ISetupPropertyStore GetProperties();
        string GetEnginePath();
    }

    [ComImport, Guid("DA8D8A16-B2B6-4487-A2F1-594CCCCD6BF5"), InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
    public interface ISetupPackageReference
    {
        string GetId();
        string GetVersion();
        string GetChip();
        string GetLanguage();
        string GetBranch();
        string GetType();
        string GetUniqueId();
        bool GetIsExtension();
    }

    [ComImport, Guid("c601c175-a3be-44bc-91f6-4568d230fc83"), InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
    public interface ISetupPropertyStore
    {
        [return: MarshalAs(UnmanagedType.SafeArray, SafeArraySubType = VarEnum.VT_BSTR)]
        string[] GetNames();

        object GetValue([MarshalAs(UnmanagedType.LPWStr)] [In] string pwszName);
    }
}
