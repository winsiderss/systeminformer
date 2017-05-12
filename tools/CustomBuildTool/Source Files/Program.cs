using System;
using System.Collections.Generic;
using System.Security.Principal;

namespace CustomBuildTool
{
    public static class Program
    {
        public static Dictionary<string, string> ProgramArgs;

        public static void Main(string[] args)
        {
            ProgramArgs = ParseArgs(args);

            if (ProgramArgs.ContainsKey("-cleanup"))
            {
                if (Restart("-cleanup"))
                    return;

                if (!Build.InitializeBuildEnvironment(true))
                    return;

                Build.CleanupAppxSignature();
                Build.CleanupBuildEnvironment();

                Build.ShowBuildStats();
            }
            else if (ProgramArgs.ContainsKey("-phapppub_gen"))
            {
                if (!Build.InitializeBuildEnvironment(false))
                    return;

                Build.BuildPublicHeaderFiles();
            }
            else if (ProgramArgs.ContainsKey("-sign"))
            {
                Build.BuildKphSignatureFile(false);
            }
            else if (ProgramArgs.ContainsKey("-sdk"))
            {
                if (!Build.InitializeBuildEnvironment(false))
                    return;

                if (!Build.CopyPluginSdkHeaders())
                    return;
                if (!Build.CopyVersionHeader())
                    return;
                if (!Build.FixupResourceHeader())
                    return;

                if (!Build.CopyLibFiles(BuildFlags.Build32bit | BuildFlags.Build64bit | BuildFlags.BuildVerbose))
                    return;

                Build.CopyRedistFiles(
                    BuildFlags.Build32bit | BuildFlags.Build64bit | 
                    BuildFlags.BuildDebug | BuildFlags.BuildVerbose
                    );

                Build.ShowBuildStats();
            }
            else if (ProgramArgs.ContainsKey("-cleansdk"))
            {
                if (!Build.InitializeBuildEnvironment(false))
                    return;

                if (!Build.BuildSolution("ProcessHacker.sln",
                    BuildFlags.Build32bit | BuildFlags.Build64bit | BuildFlags.BuildVerbose))
                    return;

                if (!Build.CopyPluginSdkHeaders())
                    return;
                if (!Build.CopyVersionHeader())
                    return;
                if (!Build.FixupResourceHeader())
                    return;
                if (!Build.CopyLibFiles(
                    BuildFlags.Build32bit | BuildFlags.Build64bit | BuildFlags.BuildVerbose))
                    return;

                Build.ShowBuildStats();
            }
            else if (ProgramArgs.ContainsKey("-bin"))
            {
                if (!Build.InitializeBuildEnvironment(false))
                    return;

                Build.ShowBuildEnvironment("bin", false);
                Build.BuildSecureFiles();
                Build.UpdateHeaderFileVersion();

                if (!Build.BuildSolution("ProcessHacker.sln",
                    BuildFlags.Build32bit | BuildFlags.Build64bit | BuildFlags.BuildVerbose))
                    return;

                if (!Build.BuildKphSignatureFile(false))
                    return;
                if (!Build.CopyTextFiles())
                    return;
                if (!Build.CopyKProcessHacker(false))
                    return;
                if (!Build.CopyPluginSdkHeaders())
                    return;
                if (!Build.CopyVersionHeader())
                    return;
                if (!Build.FixupResourceHeader())
                    return;

                if (!Build.CopyLibFiles(
                    BuildFlags.Build32bit | BuildFlags.Build64bit | BuildFlags.BuildVerbose))
                    return;

                if (!Build.BuildSolution("plugins\\Plugins.sln",
                    BuildFlags.Build32bit | BuildFlags.Build64bit | BuildFlags.BuildVerbose))
                    return;

                if (!Build.CopyWow64Files(BuildFlags.None))
                    return;

                if (!Build.BuildBinZip())
                    return;

                Build.ShowBuildStats();
            }
            else if (ProgramArgs.ContainsKey("-debug"))
            {
                if (!Build.InitializeBuildEnvironment(true))
                    return;

                Build.ShowBuildEnvironment("debug", true);
                Build.BuildSecureFiles();

                if (!Build.BuildSolution("ProcessHacker.sln",
                    BuildFlags.Build32bit | BuildFlags.Build64bit |
                    BuildFlags.BuildDebug | BuildFlags.BuildVerbose))
                    return;

                if (!Build.BuildKphSignatureFile(true))
                    return;
                if (!Build.CopyTextFiles())
                    return;
                if (!Build.CopyKProcessHacker(true))
                    return;
                if (!Build.CopyPluginSdkHeaders())
                    return;
                if (!Build.CopyVersionHeader())
                    return;
                if (!Build.FixupResourceHeader())
                    return;
                if (!Build.CopyLibFiles(
                    BuildFlags.Build32bit | BuildFlags.Build64bit |
                    BuildFlags.BuildDebug | BuildFlags.BuildVerbose))
                    return;

                if (!Build.BuildSolution("plugins\\Plugins.sln",
                    BuildFlags.Build32bit | BuildFlags.Build64bit |
                    BuildFlags.BuildDebug | BuildFlags.BuildVerbose))
                    return;

                if (!Build.CopyWow64Files(BuildFlags.BuildDebug))
                    return;

                Build.ShowBuildStats();
            }
            else if (ProgramArgs.ContainsKey("-release"))
            {
                if (!Build.InitializeBuildEnvironment(true))
                    return;

                Build.ShowBuildEnvironment("release", true);
                Build.BuildSecureFiles();
                Build.UpdateHeaderFileVersion();

                if (!Build.BuildSolution("ProcessHacker.sln",
                    BuildFlags.Build32bit | BuildFlags.Build64bit | BuildFlags.BuildVerbose))
                    return;

                if (!Build.BuildKphSignatureFile(false))
                    return;
                if (!Build.CopyTextFiles())
                    return;
                if (!Build.CopyKProcessHacker(false))
                    return;
                if (!Build.CopyPluginSdkHeaders())
                    return;
                if (!Build.CopyVersionHeader())
                    return;
                if (!Build.FixupResourceHeader())
                    return;
                if (!Build.CopyLibFiles(
                    BuildFlags.Build32bit | BuildFlags.Build64bit | BuildFlags.BuildVerbose))
                    return;

                if (!Build.BuildSolution("plugins\\Plugins.sln",
                    BuildFlags.Build32bit | BuildFlags.Build64bit | BuildFlags.BuildVerbose))
                    return;

                if (!Build.CopyWow64Files(BuildFlags.None))
                    return;

                if (!Build.BuildBinZip())
                    return;
                if (!Build.BuildSetupExe())
                    return;
                Build.BuildPdbZip();
                Build.BuildSdkZip();
                Build.BuildSrcZip();

                Build.ShowBuildStats();
            }
            else if (ProgramArgs.ContainsKey("-appveyor"))
            {
                if (!Build.InitializeBuildEnvironment(true))
                    return;

                Build.ShowBuildEnvironment("nightly", true);
                Build.BuildSecureFiles();
                Build.UpdateHeaderFileVersion();

                if (!Build.BuildSolution("ProcessHacker.sln",
                    BuildFlags.Build32bit | BuildFlags.Build64bit | BuildFlags.BuildVerbose))
                    return;

                if (!Build.BuildKphSignatureFile(false))
                    return;
                if (!Build.CopyTextFiles())
                    return;
                if (!Build.CopyKProcessHacker(false))
                    return;
                if (!Build.CopyPluginSdkHeaders())
                    return;
                if (!Build.CopyVersionHeader())
                    return;
                if (!Build.FixupResourceHeader())
                    return;
                if (!Build.CopyLibFiles(
                    BuildFlags.Build32bit | BuildFlags.Build64bit | BuildFlags.BuildVerbose))
                    return;

                if (!Build.BuildSolution("plugins\\Plugins.sln",
                    BuildFlags.Build32bit | BuildFlags.Build64bit | BuildFlags.BuildVerbose))
                    return;

                if (!Build.CopyWow64Files(BuildFlags.None))
                    return;

                Build.CopyRedistFiles(
                    BuildFlags.Build32bit | BuildFlags.Build64bit | 
                    BuildFlags.BuildDebug | BuildFlags.BuildVerbose
                    );

                if (!Build.BuildBinZip())
                    return;
                if (!Build.BuildSetupExe())
                    return;
                if (!Build.BuildChecksumsFile())
                    return;
                if (!Build.BuildUpdateSignature())
                    return;

                if (Build.AppveyorUploadBuildFiles())
                    Build.WebServiceUpdateConfig();
            }
            else if (ProgramArgs.ContainsKey("-appxbuild"))
            {
                if (!Build.InitializeBuildEnvironment(true))
                    return;

                Build.ShowBuildEnvironment("appx", true);
                Build.BuildSecureFiles();
                Build.UpdateHeaderFileVersion();

                if (!Build.BuildSolution("ProcessHacker.sln", BuildFlags.Build32bit | BuildFlags.Build64bit | BuildFlags.BuildVerbose))
                    return;

                if (!Build.CopyTextFiles())
                    return;
                if (!Build.CopyPluginSdkHeaders())
                    return;
                if (!Build.CopyVersionHeader())
                    return;
                if (!Build.FixupResourceHeader())
                    return;
                if (!Build.CopyLibFiles(BuildFlags.Build32bit | BuildFlags.Build64bit | BuildFlags.BuildVerbose))
                    return;

                if (!Build.BuildSolution("plugins\\Plugins.sln", BuildFlags.Build32bit | BuildFlags.Build64bit | BuildFlags.BuildVerbose))
                    return;

                if (!Build.CopyWow64Files(BuildFlags.None))
                    return;

                Build.BuildAppxPackage(BuildFlags.Build32bit | BuildFlags.Build64bit | BuildFlags.BuildVerbose);

                Build.ShowBuildStats();
            }
            else if (ProgramArgs.ContainsKey("-appxmakecert"))
            {
                if (Restart("-appxmakecert"))
                    return;

                if (!Build.InitializeBuildEnvironment(false))
                    return;

                Build.ShowBuildEnvironment("appxcert", true);

                Build.BuildAppxSignature();

                Build.ShowBuildStats();
            }
            else
            {
                Console.WriteLine("Invalid arguments.\n");
            }
        }

        private static Dictionary<string, string> ParseArgs(string[] args)
        {
            var dict = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase);
            string argPending = null;

            foreach (string s in args)
            {
                if (s.StartsWith("-", StringComparison.OrdinalIgnoreCase))
                {
                    if (!dict.ContainsKey(s))
                        dict.Add(s, string.Empty);

                    argPending = s;
                }
                else
                {
                    if (argPending != null)
                    {
                        dict[argPending] = s;
                        argPending = null;
                    }
                    else
                    {
                        if (!dict.ContainsKey(string.Empty))
                            dict.Add(string.Empty, s);
                    }
                }
            }

            return dict;
        }

        public static void PrintColorMessage(string Message, ConsoleColor Color, bool Newline = true, BuildFlags Flags = BuildFlags.BuildVerbose)
        {
            if ((Flags & BuildFlags.BuildVerbose) != BuildFlags.BuildVerbose)
                return;

            Console.ForegroundColor = Color;
            if (Newline)
                Console.WriteLine(Message);
            else
                Console.Write(Message);
            Console.ResetColor();
        }

        private static bool Restart(string Arguments)
        {
            WindowsIdentity identity = WindowsIdentity.GetCurrent();
            WindowsPrincipal principal = new WindowsPrincipal(identity);

            if (principal.IsInRole(WindowsBuiltInRole.Administrator))
                return false;

            Win32.ShowWindow(Win32.GetConsoleWindow(), Win32.SW_HIDE);

            try
            {
                System.Diagnostics.Process.Start(new System.Diagnostics.ProcessStartInfo
                {
                    FileName = System.Diagnostics.Process.GetCurrentProcess().MainModule.FileName,
                    Arguments = Arguments,
                    Verb = "runas"
                });
            }
            catch (Exception) { }

            return true;
        }
    }

    [Flags]
    public enum BuildFlags
    {
        None,
        Build32bit = 0x1,
        Build64bit = 0x2,
        BuildDebug = 0x4,
        BuildVerbose = 0x8
    }
}