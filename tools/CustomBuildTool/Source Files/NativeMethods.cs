using System;
using System.Diagnostics;
using System.IO;
using System.Runtime.InteropServices;

namespace CustomBuildTool
{
    [System.Security.SuppressUnmanagedCodeSecurity]
    public static class Win32
    {
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

            if (File.GetLastWriteTime(CurrentFile) > File.GetLastWriteTime(NewFile))
            {
                File.Copy(CurrentFile, NewFile, true);
            }
        }

        public const int SW_HIDE = 0;
        public const int SW_SHOW = 5;
        public static readonly IntPtr INVALID_HANDLE_VALUE = new IntPtr(-1);
        public static readonly IntPtr STD_OUTPUT_HANDLE = new IntPtr(-11);
        public static readonly IntPtr STD_INPUT_HANDLE = new IntPtr(-10);
        public static readonly IntPtr STD_ERROR_HANDLE = new IntPtr(-12);

        [DllImport("kernel32.dll", ExactSpelling = true)]
        public static extern IntPtr GetStdHandle(IntPtr StdHandle);
        [DllImport("kernel32.dll", ExactSpelling = true)]
        public static extern bool GetConsoleMode(IntPtr ConsoleHandle, out ConsoleMode Mode);
        [DllImport("kernel32.dll", ExactSpelling = true)]
        public static extern bool SetConsoleMode(IntPtr ConsoleHandle, ConsoleMode Mode);
        [DllImport("kernel32.dll", ExactSpelling = true)]
        public static extern IntPtr GetConsoleWindow();
        [DllImport("user32.dll", ExactSpelling = true)]
        public static extern bool ShowWindow(IntPtr hWnd, int nCmdShow);    
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

    [ComImport, ClassInterface(ClassInterfaceType.AutoDispatch), Guid("177F0C4A-1CD3-4DE7-A32C-71DBBB9FA36D")]
    public class SetupConfigurationClass
    {

    }

    [ComImport, CoClass(typeof(SetupConfigurationClass)), Guid("42843719-DB4C-46C2-8E7C-64F1816EFD5B")]
    public interface SetupConfiguration : ISetupConfiguration2, ISetupConfiguration
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