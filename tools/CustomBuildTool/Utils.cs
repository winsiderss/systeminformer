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
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Runtime.InteropServices;
using System.Runtime.Serialization;
using System.Runtime.Serialization.Json;
using System.Security.Cryptography;
using System.Text;

namespace CustomBuildTool
{
    [System.Security.SuppressUnmanagedCodeSecurity]
    public static class Win32
    {
        public static int CreateProcess(string FileName, string args)
        {
            using (Process process = Process.Start(new ProcessStartInfo
            {
                UseShellExecute = false,
                FileName = FileName,
                CreateNoWindow = true
            }))
            {
                process.StartInfo.Arguments = args;
                process.Start();

                process.WaitForExit();

                return process.ExitCode;
            }
        }

        public static string ShellExecute(string FileName, string args)
        {
            string output = string.Empty;
            using (Process process = Process.Start(new ProcessStartInfo
            {
                UseShellExecute = false,
                RedirectStandardOutput = true,
                FileName = FileName,
                CreateNoWindow = true
            }))
            {
                process.StartInfo.Arguments = args;
                process.Start();

                output = process.StandardOutput.ReadToEnd();
                output = output.Replace("\n\n", "\r\n", StringComparison.OrdinalIgnoreCase).Trim();

                process.WaitForExit();
            }

            return output;
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

        public static void CopyIfNewer(string CurrentFile, string NewFile)
        {
            if (!File.Exists(CurrentFile))
                return;

            if (CurrentFile.EndsWith(".sys", StringComparison.OrdinalIgnoreCase))
            {
                if (!File.Exists(NewFile))
                {
                    File.Copy(CurrentFile, NewFile, true);
                }
                else
                {
                    FileVersionInfo currentInfo = FileVersionInfo.GetVersionInfo(CurrentFile);
                    FileVersionInfo newInfo = FileVersionInfo.GetVersionInfo(NewFile);
                    var currentInfoVersion = new Version(currentInfo.FileVersion);
                    var newInfoVersion = new Version(newInfo.FileVersion);

                    if (
                        currentInfoVersion > newInfoVersion ||
                        File.GetLastWriteTime(CurrentFile) > File.GetLastWriteTime(NewFile)
                        )
                    {
                        File.Copy(CurrentFile, NewFile, true);
                    }
                }
            }
            else
            {
                if (File.GetLastWriteTime(CurrentFile) > File.GetLastWriteTime(NewFile))
                    File.Copy(CurrentFile, NewFile, true);
            }
        }

        //public static string SearchFile(string FileName)
        //{
        //    string where = Environment.ExpandEnvironmentVariables("%SystemRoot%\\System32\\where.exe");
        //
        //    if (File.Exists(where))
        //    {
        //        string whereResult = ShellExecute(where, FileName);
        //
        //        if (!string.IsNullOrEmpty(whereResult))
        //            return whereResult;
        //    }
        //
        //    return null;
        //}
        //
        //public static void ImageResizeFile(int size, string FileName, string OutName)
        //{
        //    using (var src = System.Drawing.Image.FromFile(FileName))
        //    using (var dst = new System.Drawing.Bitmap(size, size))
        //    using (var g = System.Drawing.Graphics.FromImage(dst))
        //    {
        //        g.SmoothingMode = System.Drawing.Drawing2D.SmoothingMode.AntiAlias;
        //        g.InterpolationMode = System.Drawing.Drawing2D.InterpolationMode.HighQualityBicubic;
        //
        //        g.DrawImage(src, 0, 0, dst.Width, dst.Height);
        //
        //        dst.Save(OutName, System.Drawing.Imaging.ImageFormat.Png);
        //    }
        //}
    }

    public static class Json<T> where T : class
    {
        public static string Serialize(T instance)
        {
            DataContractJsonSerializer serializer = new DataContractJsonSerializer(typeof(T));

            using (MemoryStream stream = new MemoryStream())
            {
                serializer.WriteObject(stream, instance);

                return Encoding.Default.GetString(stream.ToArray());
            }
        }

        public static T DeSerialize(string json)
        {
            using (MemoryStream stream = new MemoryStream(Encoding.Default.GetBytes(json)))
            {
                DataContractJsonSerializer serializer = new DataContractJsonSerializer(typeof(T));

                return (T)serializer.ReadObject(stream);
            }
        }
    }

    public static class Verify
    {
        private static Rijndael GetRijndael(string secret)
        {
            using (Rfc2898DeriveBytes rfc2898DeriveBytes = new Rfc2898DeriveBytes(secret, Convert.FromBase64String("e0U0RTY2RjU5LUNBRjItNEMzOS1BN0Y4LTQ2MDk3QjFDNDYxQn0="), 10000))
            {
                Rijndael rijndael = Rijndael.Create();

                rijndael.Key = rfc2898DeriveBytes.GetBytes(32);
                rijndael.IV = rfc2898DeriveBytes.GetBytes(16);

                return rijndael;
            }
        }

        public static void Encrypt(string fileName, string outFileName, string secret)
        {
            using (FileStream fileOutStream = File.Create(outFileName))
            using (Rijndael rijndael = GetRijndael(secret))
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
                using (Rijndael rijndael = GetRijndael(secret))
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
            using (SHA256Managed sha = new SHA256Managed())
            {
                byte[] checksum = sha.ComputeHash(bufferedStream);

                return BitConverter.ToString(checksum).Replace("-", string.Empty);
            }
        }
    }

    public static class VisualStudio
    {
        private static readonly string[] CustomSignToolPathArray =
        {
            "\\tools\\CustomSignTool\\bin\\Release32\\CustomSignTool.exe",
            "\\tools\\CustomSignTool\\bin\\Release64\\CustomSignTool.exe",
            //"\\tools\\CustomSignTool\\bin\\Debug32\\CustomSignTool.exe",
            //"\\tools\\CustomSignTool\\bin\\RDebug64\\CustomSignTool.exe",
        };

        private static readonly string[] MsBuildPathArray =
        {
            "\\MSBuild\\Current\\Bin\\MSBuild.exe",
            "\\MSBuild\\15.0\\Bin\\MSBuild.exe"
        };

        private static readonly string[] GitPathArray =
        {
            "%ProgramFiles%\\Git\\bin\\git.exe",
            "%ProgramFiles(x86)%\\Git\\bin\\git.exe",
            "%ProgramW6432%\\Git\\bin\\git.exe"
        };

        private static readonly string[] VswherePathArray =
        {
            "%ProgramFiles%\\Microsoft Visual Studio\\Installer\\vswhere.exe",
            "%ProgramFiles(x86)%\\Microsoft Visual Studio\\Installer\\vswhere.exe",
            "%ProgramW6432%\\Microsoft Visual Studio\\Installer\\vswhere.exe"
        };

        public static string GetOutputDirectoryPath()
        {
            string folder = Environment.CurrentDirectory + "\\build\\output";

            if (File.Exists(folder))
            {
                return folder;
            }

            try
            {
                Directory.CreateDirectory(folder);

                return folder;
            }
            catch (Exception ex)
            {
                Program.PrintColorMessage("Error creating output directory. " + ex, ConsoleColor.Red);
            }

            return null;
        }

        public static string GetGitFilePath()
        {
            string git = Win32.SearchFile("git.exe");

            if (File.Exists(git))
                return git;

            foreach (string path in GitPathArray)
            {
                git = Environment.ExpandEnvironmentVariables(path);

                if (File.Exists(git))
                    return git;
            }

            return null;
        }

        public static string GetGitWorkPath(string Directory)
        {
            return "--git-dir=\"" + Directory + "\\.git\" --work-tree=\"" + Directory + "\" "; ;
        }

        public static string GetVswhereFilePath()
        {
            foreach (string path in VswherePathArray)
            {
                string file = Environment.ExpandEnvironmentVariables(path);

                if (File.Exists(file))
                    return file;
            }

            return null;
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

        public static string GetMsbuildFilePath()
        {
            VisualStudioInstance instance;
            string vswhere;
            string vswhereResult;

            instance = GetVisualStudioInstance();

            if (instance != null)
            {
                foreach (string path in MsBuildPathArray)
                {
                    string file = instance.Path + path;

                    if (File.Exists(file))
                    {
                        return file;
                    }
                }
            }

            vswhere = GetVswhereFilePath();

            if (string.IsNullOrEmpty(vswhere))
                return null;

            vswhereResult = Win32.ShellExecute(
                vswhere,
                "-latest " +
                "-prerelease " +
                "-products * " +
                "-requiresAny " +
                "-requires Microsoft.Component.MSBuild " +
                "-property installationPath "
                );

            if (string.IsNullOrEmpty(vswhereResult))
                return null;

            foreach (string path in MsBuildPathArray)
            {
                string file = vswhereResult + path;

                if (File.Exists(file))
                {
                    return file;
                }
            }

            return null;
        }


        private static List<VisualStudioInstance> VisualStudioInstanceList = null;
        public static VisualStudioInstance GetVisualStudioInstance()
        {
            if (VisualStudioInstanceList == null)
            {
                VisualStudioInstanceList = new List<VisualStudioInstance>();

                try
                {
                    var setupConfiguration = new SetupConfigurationClass() as ISetupConfiguration2;
                    var instanceEnumerator = setupConfiguration.EnumAllInstances();
                    var instances = new ISetupInstance2[3];

                    instanceEnumerator.Next(instances.Length, instances, out var instancesFetched);

                    if (instancesFetched == 0)
                        return null;

                    do
                    {
                        for (int i = 0; i < instancesFetched; i++)
                        {
                            VisualStudioInstanceList.Add(new VisualStudioInstance(instances[i]));
                        }

                        instanceEnumerator.Next(instances.Length, instances, out instancesFetched);
                    }
                    while (instancesFetched != 0);
                }
                catch { }
            }

            foreach (VisualStudioInstance instance in VisualStudioInstanceList)
            {
                if (
                    instance.HasRequiredDependency &&
                    instance.DisplayName.EndsWith("2019", StringComparison.OrdinalIgnoreCase) // HACK
                    )
                {
                    return instance;
                }
            }

            return null;
        }
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
        public bool IsLaunchable { get; }
        public bool IsComplete { get; }
        public string Name { get; }
        public string Path { get; }
        public string Version { get; }
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
            this.IsLaunchable = FromInstance.IsLaunchable();
            this.IsComplete = FromInstance.IsComplete();
            this.Name = FromInstance.GetInstallationName();
            this.Path = FromInstance.GetInstallationPath();
            this.Version = FromInstance.GetInstallationVersion();
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
            var found = this.Packages.FindAll(p => p.Id.StartsWith("Microsoft.VisualStudio.Component.Windows10SDK.", StringComparison.OrdinalIgnoreCase));

            if (found == null)
                return null;
            if (found.Count == 0)
                return null;

            found.Sort((p1, p2) => string.Compare(p1.Id, p2.Id, StringComparison.OrdinalIgnoreCase));

            return found[found.Count - 1];
        }

        public string GetWindowsSdkVersion()
        {
            VisualStudioPackage package = GetLatestSdkPackage();

            if (package == null)
                return string.Empty;

            return package.Id.Substring("Microsoft.VisualStudio.Component.Windows10SDK.".Length);
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
                //    "Microsoft.VisualStudio.Component.Windows10SDK.17134",
                //    "Microsoft.VisualStudio.Component.Windows10SDK.17763",
                //    "Microsoft.VisualStudio.Component.Windows10SDK.18362",
                //    "Microsoft.VisualStudio.Component.VC.Tools.x86.x64",
                //    "Microsoft.VisualStudio.Component.VC.14.20.x86.x64",
                //    "Microsoft.VisualStudio.Component.VC.14.21.x86.x64",
                //    "Microsoft.VisualStudio.Component.VC.14.22.x86.x64",
                //};

                // Microsoft.VisualStudio.Component.VC.Redist.14.Latest
                // Microsoft.VisualStudio.Component.VC.Tools.x86.x64

                hasbuild = this.Packages.Find(p => p.Id.StartsWith("Microsoft.Component.MSBuild", StringComparison.OrdinalIgnoreCase)) != null;
                hasruntimes = this.Packages.Find(p => p.Id.StartsWith("Microsoft.VisualStudio.Component.VC", StringComparison.OrdinalIgnoreCase)) != null;
                haswindowssdk = this.Packages.Find(p => p.Id.StartsWith("Microsoft.VisualStudio.Component.Windows10SDK", StringComparison.OrdinalIgnoreCase)) != null;

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
                //    "Microsoft.VisualStudio.Component.Windows10SDK.17134",
                //    "Microsoft.VisualStudio.Component.Windows10SDK.17763",
                //    "Microsoft.VisualStudio.Component.Windows10SDK.18362",
                //    "Microsoft.VisualStudio.Component.VC.Tools.x86.x64",
                //    "Microsoft.VisualStudio.Component.VC.14.20.x86.x64",
                //    "Microsoft.VisualStudio.Component.VC.14.21.x86.x64",
                //    "Microsoft.VisualStudio.Component.VC.14.22.x86.x64",
                //};

                if (this.Packages.Find(p => p.Id.StartsWith("Microsoft.Component.MSBuild", StringComparison.OrdinalIgnoreCase)) == null)
                {
                    list += "MSBuild" + Environment.NewLine;
                }

                if (this.Packages.Find(p => p.Id.StartsWith("Microsoft.VisualStudio.Component.VC", StringComparison.OrdinalIgnoreCase)) == null)
                {
                    list += "VC.14.Redist" + Environment.NewLine;
                }

                if (this.Packages.Find(p => p.Id.StartsWith("Microsoft.VisualStudio.Component.Windows10SDK", StringComparison.OrdinalIgnoreCase)) == null)
                {
                    list += "Windows10SDK" + Environment.NewLine;
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

    [DataContract]
    public class BuildUpdateRequest
    {
        [DataMember(Name = "build_version")] public string BuildVersion { get; set; }
        [DataMember(Name = "build_commit")] public string BuildCommit { get; set; }
        [DataMember(Name = "build_updated")] public string BuildUpdated { get; set; }
        [DataMember(Name = "build_message")] public string BuildMessage { get; set; }

        [DataMember(Name = "bin_url")] public string BinUrl { get; set; }
        [DataMember(Name = "bin_length")] public string BinLength { get; set; }
        [DataMember(Name = "bin_hash")] public string BinHash { get; set; }
        [DataMember(Name = "bin_sig")] public string BinSig { get; set; }

        [DataMember(Name = "setup_url")] public string SetupUrl { get; set; }
        [DataMember(Name = "setup_length")] public string SetupLength { get; set; }
        [DataMember(Name = "setup_hash")] public string SetupHash { get; set; }
        [DataMember(Name = "setup_sig")] public string SetupSig { get; set; }

        [DataMember(Name = "websetup_url")] public string WebSetupUrl { get; set; }
        [DataMember(Name = "websetup_version")] public string WebSetupVersion { get; set; }
        [DataMember(Name = "websetup_length")] public string WebSetupLength { get; set; }
        [DataMember(Name = "websetup_hash")] public string WebSetupHash { get; set; }
        [DataMember(Name = "websetup_sig")] public string WebSetupSig { get; set; }

        [DataMember(Name = "message")] public string Message { get; set; }
        [DataMember(Name = "changelog")] public string Changelog { get; set; }
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
