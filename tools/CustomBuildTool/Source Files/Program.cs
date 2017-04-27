using System;
using System.Collections.Generic;

namespace CustomBuildTool
{
    public static class Program
    {
        public static Dictionary<string, string> ProgramArgs;

        public static void PrintColorMessage(string Message, ConsoleColor Color, bool Newline = true)
        {
            Console.ForegroundColor = Color;

            if (Newline)
                Console.WriteLine(Message);
            else
                Console.Write(Message);

            Console.ResetColor();
        }

        public static void Main(string[] args)
        {
            ProgramArgs = ParseArgs(args);

            if (ProgramArgs.ContainsKey("-cleanup"))
            {
                if (!Build.InitializeBuildEnvironment(false))
                    return;

                PrintColorMessage("Cleanup build...", ConsoleColor.Cyan);
                Build.CleanupBuildEnvironment();
                return;
            }
            else if (ProgramArgs.ContainsKey("-updaterev"))
            {
                if (!Build.InitializeBuildEnvironment(false))
                    return;

                PrintColorMessage("Updating header version...", ConsoleColor.Cyan);
                Build.ShowBuildEnvironment(false);
                Build.UpdateHeaderFileVersion();
                return;
            }
            else if (ProgramArgs.ContainsKey("-phapppub_gen"))
            {
                if (!Build.InitializeBuildEnvironment(false))
                    return;

                PrintColorMessage("Building public headers...", ConsoleColor.Cyan);
                Build.BuildPublicHeaderFiles();
                return;
            }
            else if (ProgramArgs.ContainsKey("-sign"))
            {
                Build.BuildKphSignatureFile(false);
                return;
            }
            else if (ProgramArgs.ContainsKey("-sdk"))
            {
                PrintColorMessage("Setting up build environment...", ConsoleColor.Cyan);
                if (!Build.InitializeBuildEnvironment(false))
                    return;

                PrintColorMessage("Copying Plugin SDK headers...", ConsoleColor.Cyan);
                if (!Build.CopyPluginSdkHeaders())
                    return;

                PrintColorMessage("Copying Plugin SDK version header...", ConsoleColor.Cyan);
                if (!Build.CopyVersionHeader())
                    return;

                PrintColorMessage("Building Plugin SDK resource header...", ConsoleColor.Cyan);
                if (!Build.FixupResourceHeader())
                    return;

                PrintColorMessage("Copying Plugin SDK linker files...", ConsoleColor.Cyan);
                if (!Build.CopyLibFiles(false))
                    return;

                Build.ShowBuildStats(false);
                return;
            }
            else if (ProgramArgs.ContainsKey("-cleansdk"))
            {
                PrintColorMessage("Setting up build environment...", ConsoleColor.Cyan);
                if (!Build.InitializeBuildEnvironment(false))
                    return;

                if (!Build.BuildSolution("ProcessHacker.sln", true, false, true))
                    return;

                PrintColorMessage("Copying Plugin SDK headers...", ConsoleColor.Cyan);
                if (!Build.CopyPluginSdkHeaders())
                    return;

                PrintColorMessage("Copying Plugin SDK version header...", ConsoleColor.Cyan);
                if (!Build.CopyVersionHeader())
                    return;

                PrintColorMessage("Building Plugin SDK resource header...", ConsoleColor.Cyan);
                if (!Build.FixupResourceHeader())
                    return;

                PrintColorMessage("Copying Plugin SDK linker files...", ConsoleColor.Cyan);
                if (!Build.CopyLibFiles(false))
                    return;

                Build.ShowBuildStats(false);
                return;
            }
            else if (ProgramArgs.ContainsKey("-bin"))
            {
                PrintColorMessage("Setting up build environment...", ConsoleColor.Cyan);
                if (!Build.InitializeBuildEnvironment(false))
                    return;

                Build.ShowBuildEnvironment(false);
                Build.BuildSecureFiles();

                if (!Build.BuildSolution("ProcessHacker.sln", true, false, true))
                    return;

                PrintColorMessage("Building KPH signature...", ConsoleColor.Cyan);
                if (!Build.BuildKphSignatureFile(false))
                    return;

                PrintColorMessage("Copying text files...", ConsoleColor.Cyan);
                if (!Build.CopyTextFiles())
                    return;

                PrintColorMessage("Copying KPH driver...", ConsoleColor.Cyan);
                if (!Build.CopyKProcessHacker(false))
                    return;

                PrintColorMessage("Copying Plugin SDK headers...", ConsoleColor.Cyan);
                if (!Build.CopyPluginSdkHeaders())
                    return;

                PrintColorMessage("Copying Plugin SDK version header...", ConsoleColor.Cyan);
                if (!Build.CopyVersionHeader())
                    return;

                PrintColorMessage("Building Plugin SDK resource header...", ConsoleColor.Cyan);
                if (!Build.FixupResourceHeader())
                    return;
                if (!Build.CopyLibFiles(false))
                    return;
                if (!Build.BuildSolution("plugins\\Plugins.sln", true, false, true))
                    return;
                if (!Build.CopyWow64Files(false))
                    return;

                PrintColorMessage("Building build-bin.zip...", ConsoleColor.Cyan);
                if (!Build.BuildBinZip())
                    return;

                Build.ShowBuildStats(false);
            }
            else if (ProgramArgs.ContainsKey("-exe"))
            {
                PrintColorMessage("Setting up build environment...", ConsoleColor.Cyan);
                if (!Build.InitializeBuildEnvironment(false))
                    return;

                PrintColorMessage("Building build-bin.zip...", ConsoleColor.Cyan);
                if (!Build.BuildBinZip())
                    return;

                PrintColorMessage("Building build-setup.exe...", ConsoleColor.Cyan);
                if (!Build.BuildSetupExe())
                    return;

                PrintColorMessage("Building build-sdk.zip...", ConsoleColor.Cyan);
                Build.BuildSdkZip();

                PrintColorMessage("Building build-src.zip...", ConsoleColor.Cyan);
                Build.BuildSrcZip();
            }
            else if (ProgramArgs.ContainsKey("-debug"))
            {
                PrintColorMessage("Setting up build environment...", ConsoleColor.Cyan);
                if (!Build.InitializeBuildEnvironment(true))
                    return;

                Build.ShowBuildEnvironment(true);
                Build.BuildSecureFiles();

                if (!Build.BuildSolution("ProcessHacker.sln", true, true, true))
                    return;

                PrintColorMessage("Building KPH signature...", ConsoleColor.Cyan);
                if (!Build.BuildKphSignatureFile(true))
                    return;

                PrintColorMessage("Copying text files...", ConsoleColor.Cyan);
                if (!Build.CopyTextFiles())
                    return;

                PrintColorMessage("Copying KPH driver...", ConsoleColor.Cyan);
                if (!Build.CopyKProcessHacker(true))
                    return;

                PrintColorMessage("Copying Plugin SDK headers...", ConsoleColor.Cyan);
                if (!Build.CopyPluginSdkHeaders())
                    return;

                PrintColorMessage("Copying Plugin SDK version header...", ConsoleColor.Cyan);
                if (!Build.CopyVersionHeader())
                    return;

                PrintColorMessage("Building Plugin SDK resource header...", ConsoleColor.Cyan);
                if (!Build.FixupResourceHeader())
                    return;

                PrintColorMessage("Copying Plugin SDK linker files...", ConsoleColor.Cyan);
                if (!Build.CopyLibFiles(true))
                    return;

                if (!Build.BuildSolution("plugins\\Plugins.sln", true, true, true))
                    return;

                PrintColorMessage("Copying Wow64 support files...", ConsoleColor.Cyan);
                if (!Build.CopyWow64Files(true))
                    return;

                Build.ShowBuildStats(true);
            }
            else // release
            {
                if (!Build.InitializeBuildEnvironment(true))
                    return;

                PrintColorMessage("Setting up build environment...", ConsoleColor.Cyan);
                Build.ShowBuildEnvironment(true);
                Build.BuildSecureFiles();

                if (!Build.BuildSolution("ProcessHacker.sln", true, false, true))
                    return;

                PrintColorMessage("Building KPH signature...", ConsoleColor.Cyan);
                if (!Build.BuildKphSignatureFile(false))
                    return;

                PrintColorMessage("Copying text files...", ConsoleColor.Cyan);
                if (!Build.CopyTextFiles())
                    return;

                PrintColorMessage("Copying KPH driver...", ConsoleColor.Cyan);
                if (!Build.CopyKProcessHacker(false))
                    return;

                PrintColorMessage("Copying Plugin SDK headers...", ConsoleColor.Cyan);
                if (!Build.CopyPluginSdkHeaders())
                    return;

                PrintColorMessage("Copying Plugin SDK version header...", ConsoleColor.Cyan);
                if (!Build.CopyVersionHeader())
                    return;

                PrintColorMessage("Building Plugin SDK resource header...", ConsoleColor.Cyan);
                if (!Build.FixupResourceHeader())
                    return;

                PrintColorMessage("Copying Plugin SDK linker files...", ConsoleColor.Cyan);
                if (!Build.CopyLibFiles(false))
                    return;

                if (!Build.BuildSolution("plugins\\Plugins.sln", true, false, true))
                    return;

                PrintColorMessage("Copying Wow64 support files...", ConsoleColor.Cyan);
                if (!Build.CopyWow64Files(false))
                    return;

                PrintColorMessage("Building build-bin.zip...", ConsoleColor.Cyan);
                if (!Build.BuildBinZip())
                    return;

                PrintColorMessage("Building build-setup.exe...", ConsoleColor.Cyan);
                if (!Build.BuildSetupExe())
                    return;

                PrintColorMessage("Building build-checksums.txt...", ConsoleColor.Cyan);
                if (!Build.BuildChecksumsFile())
                    return;

                PrintColorMessage("Building release signature...", ConsoleColor.Cyan);
                if (!Build.BuildUpdateSignature())
                    return;

                if (Build.AppveyorUploadBuildFiles())
                    Build.WebServiceUpdateConfig();

                Build.ShowBuildStats(true);
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
    }
}