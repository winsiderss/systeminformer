using System;
using System.Collections.Generic;

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
                if (!Build.InitializeBuildEnvironment(true))
                    return;

                Build.CleanupAppxSignature();

                Build.CleanupBuildEnvironment();

                Build.ShowBuildStats(true);
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

                Build.ShowBuildStats(false);
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

                Build.ShowBuildStats(false);
            }
            else if (ProgramArgs.ContainsKey("-bin"))
            {
                if (!Build.InitializeBuildEnvironment(false))
                    return;

                Build.ShowBuildEnvironment(false);
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

                Build.ShowBuildStats(false);
            }
            else if (ProgramArgs.ContainsKey("-debug"))
            {
                if (!Build.InitializeBuildEnvironment(true))
                    return;

                Build.ShowBuildEnvironment(true);
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

                Build.ShowBuildStats(true);
            }
            else if (ProgramArgs.ContainsKey("-release"))
            {
                if (!Build.InitializeBuildEnvironment(true))
                    return;

                Build.ShowBuildEnvironment(true);
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

                Build.ShowBuildStats(true);
            }
            else if (ProgramArgs.ContainsKey("-appveyor"))
            {
                if (!Build.InitializeBuildEnvironment(true))
                    return;

                Build.ShowBuildEnvironment(false);
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

                Build.ShowBuildEnvironment(true);
                Build.BuildSecureFiles();
                Build.UpdateHeaderFileVersion();

                if (!Build.BuildSolution("ProcessHacker.sln", BuildFlags.Build64bit | BuildFlags.BuildVerbose))
                    return;

                if (!Build.CopyTextFiles())
                    return;
                if (!Build.CopyPluginSdkHeaders())
                    return;
                if (!Build.CopyVersionHeader())
                    return;
                if (!Build.FixupResourceHeader())
                    return;
                if (!Build.CopyLibFiles(BuildFlags.Build64bit | BuildFlags.BuildVerbose))
                    return;

                if (!Build.BuildSolution("plugins\\Plugins.sln", BuildFlags.Build64bit | BuildFlags.BuildVerbose))
                    return;

                if (!Build.CopyWow64Files(BuildFlags.None))
                    return;

                Build.BuildAppxPackage();

                Build.ShowBuildStats(true);
            }
            else if (ProgramArgs.ContainsKey("-appxmakecert"))
            {
                if (!Build.InitializeBuildEnvironment(false))
                    return;

                Build.ShowBuildEnvironment(true);

                Build.BuildAppxSignature();

                Build.ShowBuildStats(true);
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
    }
}