/*
 * Process Hacker Toolchain - 
 *   Build script
 * 
 * Copyright (C) 2017 dmex
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
using System.Diagnostics;
using System.IO;
using System.Linq;
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
                output = output.Replace("\n\n", "\r\n").Trim();

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

        public static void ImageResizeFile(int size, string FileName, string OutName)
        {
            using (var src = System.Drawing.Image.FromFile(FileName))
            using (var dst = new System.Drawing.Bitmap(size, size))
            using (var g = System.Drawing.Graphics.FromImage(dst))
            {
                g.SmoothingMode = System.Drawing.Drawing2D.SmoothingMode.AntiAlias;
                g.InterpolationMode = System.Drawing.Drawing2D.InterpolationMode.HighQualityBicubic;

                g.DrawImage(src, 0, 0, dst.Width, dst.Height);

                dst.Save(OutName, System.Drawing.Imaging.ImageFormat.Png);
            }
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

        public static readonly IntPtr INVALID_HANDLE_VALUE = new IntPtr(-1);
        public const int STD_OUTPUT_HANDLE = -11;
        public const int STD_INPUT_HANDLE = -10;
        public const int STD_ERROR_HANDLE = -12;

        [DllImport("kernel32.dll", ExactSpelling = true)]
        public static extern IntPtr GetStdHandle(int StdHandle);
        [DllImport("kernel32.dll", ExactSpelling = true)]
        public static extern bool GetConsoleMode(IntPtr ConsoleHandle, out ConsoleMode Mode);
        [DllImport("kernel32.dll", ExactSpelling = true)]
        public static extern bool SetConsoleMode(IntPtr ConsoleHandle, ConsoleMode Mode);
        [DllImport("kernel32.dll", ExactSpelling = true)]
        public static extern IntPtr GetConsoleWindow();
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
            Rfc2898DeriveBytes rfc2898DeriveBytes = new Rfc2898DeriveBytes(secret, Convert.FromBase64String("e0U0RTY2RjU5LUNBRjItNEMzOS1BN0Y4LTQ2MDk3QjFDNDYxQn0="), 10000);
            Rijndael rijndael = Rijndael.Create();
            rijndael.Key = rfc2898DeriveBytes.GetBytes(32);
            rijndael.IV = rfc2898DeriveBytes.GetBytes(16);
            return rijndael;
        }

        public static void Encrypt(string fileName, string outFileName, string secret)
        {
            FileStream fileOutStream = File.Create(outFileName);

            using (Rijndael rijndael = GetRijndael(secret))
            using (FileStream fileStream = File.OpenRead(fileName))
            using (CryptoStream cryptoStream = new CryptoStream(fileOutStream, rijndael.CreateEncryptor(), CryptoStreamMode.Write))
            {
                fileStream.CopyTo(cryptoStream);
            }
        }

        public static void Decrypt(string FileName, string outFileName, string secret)
        {
            FileStream fileOutStream = File.Create(outFileName);

            using (Rijndael rijndael = GetRijndael(secret))
            using (FileStream fileStream = File.OpenRead(FileName))
            using (CryptoStream cryptoStream = new CryptoStream(fileOutStream, rijndael.CreateDecryptor(), CryptoStreamMode.Write))
            {
                fileStream.CopyTo(cryptoStream);
            }
        }

        public static string HashFile(string FileName)
        {
            //using (HashAlgorithm algorithm = new SHA256CryptoServiceProvider())
            //{
            //    byte[] inputBytes = Encoding.UTF8.GetBytes(input);
            //    byte[] hashBytes = algorithm.ComputeHash(inputBytes);
            //    return BitConverter.ToString(hashBytes).Replace("-", String.Empty);
            //}

            FileStream fileInStream = File.OpenRead(FileName);

            using (BufferedStream bufferedStream = new BufferedStream(fileInStream, 0x1000))
            {
                SHA256Managed sha = new SHA256Managed();
                byte[] checksum = sha.ComputeHash(bufferedStream);

                return BitConverter.ToString(checksum).Replace("-", string.Empty);
            }
        }
    }

    public static class VisualStudio
    {
        public static string GetMsbuildFilePath()
        {
            string[] MsBuildPathArray =
            {
                "\\MSBuild\\Current\\Bin\\MSBuild.exe",
                "\\MSBuild\\15.0\\Bin\\MSBuild.exe"
            };
            string vswhere = string.Empty;

            if (Environment.Is64BitOperatingSystem)
                vswhere = Environment.ExpandEnvironmentVariables("%ProgramFiles(x86)%\\Microsoft Visual Studio\\Installer\\vswhere.exe");
            else
                vswhere = Environment.ExpandEnvironmentVariables("%ProgramFiles%\\Microsoft Visual Studio\\Installer\\vswhere.exe");

            // Note: vswere.exe was only released with build 15.0.26418.1
            if (File.Exists(vswhere))
            {
                string vswhereResult = Win32.ShellExecute(vswhere,
                    "-latest " +
                    "-prerelease " +
                    "-products * " +
                    "-requires Microsoft.Component.MSBuild " +
                    "-property installationPath "
                    );

                if (string.IsNullOrEmpty(vswhereResult))
                    return null;

                foreach (string path in MsBuildPathArray)
                {
                    if (File.Exists(vswhereResult + path))
                        return vswhereResult + path;
                }

                return null;
            }
            else
            {
                try
                {
                    VisualStudioInstance instance = FindVisualStudioInstance();

                    if (instance != null)
                    {
                        foreach (string path in MsBuildPathArray)
                        {
                            if (File.Exists(instance.Path + path))
                                return instance.Path + path;
                        }
                    }
                }
                catch (Exception ex)
                {
                    Program.PrintColorMessage("[VisualStudioInstance] " + ex, ConsoleColor.Red, true);
                }

                return null;
            }
        }

        private static VisualStudioInstance FindVisualStudioInstance()
        {
            var setupConfiguration = new SetupConfiguration() as ISetupConfiguration2;
            var instanceEnumerator = setupConfiguration.EnumAllInstances();
            var instances = new ISetupInstance2[3];

            instanceEnumerator.Next(instances.Length, instances, out var instancesFetched);

            if (instancesFetched == 0)
                return null;

            do
            {
                for (int i = 0; i < instancesFetched; i++)
                {
                    var instance = new VisualStudioInstance(instances[i]);
                    var state = instances[i].GetState();
                    var packages = instances[i].GetPackages().Where(package => package.GetId().Contains("Microsoft.Component.MSBuild"));

                    if (
                        state.HasFlag(InstanceState.Local | InstanceState.Registered | InstanceState.Complete) &&
                        packages.Count() > 0 &&
                        instance.Version.StartsWith("15.0", StringComparison.OrdinalIgnoreCase)
                        )
                    {
                        return instance;
                    }
                }

                instanceEnumerator.Next(instances.Length, instances, out instancesFetched);
            }
            while (instancesFetched != 0);

            return null;
        }
    }

    public static class AppVeyor
    {
        public static readonly string AppVeyorPath = string.Empty;

        static AppVeyor()
        {
            AppVeyorPath = Win32.SearchFile("appveyor.exe");
        }

        public static bool AppVeyorNightlyBuild()
        {
            return !string.IsNullOrWhiteSpace(AppVeyorPath);
        }

        public static bool UpdateBuildVersion(string BuildVersion)
        {
            try
            {
                if (!string.IsNullOrWhiteSpace(AppVeyorPath))
                {
                    Win32.ShellExecute("appveyor", $"UpdateBuild -Version \"{BuildVersion}\" ");
                }
            }
            catch (Exception ex)
            {
                Program.PrintColorMessage("[VisualStudioInstance] " + ex, ConsoleColor.Red, true);
                return false;
            }

            return true;
        }

        public static bool UploadFile(string FileName)
        {
            try
            {
                if (!string.IsNullOrWhiteSpace(AppVeyorPath))
                {
                    Win32.ShellExecute("appveyor", $"PushArtifact \"{FileName}\" ");
                }
            }
            catch (Exception ex)
            {
                Program.PrintColorMessage("[UploadFile] " + ex, ConsoleColor.Red, true);
                return false;
            }

            return true;
        }

        private static VisualStudioInstance FindVisualStudioInstance()
        {
            var setupConfiguration = new SetupConfiguration() as ISetupConfiguration2;
            var instanceEnumerator = setupConfiguration.EnumAllInstances();
            var instances = new ISetupInstance2[3];

            instanceEnumerator.Next(instances.Length, instances, out var instancesFetched);

            if (instancesFetched == 0)
                return null;

            do
            {
                for (int i = 0; i < instancesFetched; i++)
                {
                    var instance = new VisualStudioInstance(instances[i]);
                    var state = instances[i].GetState();
                    var packages = instances[i].GetPackages().Where(package => package.GetId().Contains("Microsoft.Component.MSBuild"));

                    if (
                        state.HasFlag(InstanceState.Local | InstanceState.Registered | InstanceState.Complete) &&
                        packages.Count() > 0 &&
                        instance.Version.StartsWith("15.0", StringComparison.OrdinalIgnoreCase)
                        )
                    {
                        return instance;
                    }
                }

                instanceEnumerator.Next(instances.Length, instances, out instancesFetched);
            }
            while (instancesFetched != 0);

            return null;
        }
    }

    public class VisualStudioInstance
    {
        public bool IsLaunchable { get; }
        public bool IsComplete { get; }
        public string Name { get; }
        public string Path { get; }
        public string Version { get; }
        public string DisplayName { get; }
        public string Description { get; }
        public string ResolvePath { get; }
        public string EnginePath { get; }
        public string ProductPath { get; }
        public string InstanceId { get; }
        public DateTime InstallDate { get; }

        public VisualStudioInstance(ISetupInstance2 FromInstance)
        {
            this.IsLaunchable = FromInstance.IsLaunchable();
            this.IsComplete = FromInstance.IsComplete();
            this.Name = FromInstance.GetInstallationName();
            this.Path = FromInstance.GetInstallationPath();
            this.Version = FromInstance.GetInstallationVersion();
            this.DisplayName = FromInstance.GetDisplayName();
            this.Description = FromInstance.GetDescription();
            this.ResolvePath = FromInstance.ResolvePath();
            this.EnginePath = FromInstance.GetEnginePath();
            this.InstanceId = FromInstance.GetInstanceId();
            this.ProductPath = FromInstance.GetProductPath();

            try
            {
                var time = FromInstance.GetInstallDate();
                ulong high = (ulong)time.dwHighDateTime;
                uint low = (uint)time.dwLowDateTime;
                long fileTime = (long)((high << 32) + low);

                this.InstallDate = DateTime.FromFileTimeUtc(fileTime);
            }
            catch
            {
                this.InstallDate = DateTime.UtcNow;
            }

            // FromInstance.GetState();
            // FromInstance.GetPackages();
            // FromInstance.GetProduct();            
            // FromInstance.GetProperties();
            // FromInstance.GetErrors();
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
}
