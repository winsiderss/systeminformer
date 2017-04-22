using System;
using System.Collections.Generic;

namespace CustomBuildTool
{
    public static class Program
    {
        private static Dictionary<string, string> ProgramArgs;

        public static void PrintColorMessage(string Message, ConsoleColor Color)
        {
            Console.ForegroundColor = Color;
            Console.WriteLine(Message);
            Console.ForegroundColor = ConsoleColor.White;
        }

        public static void Main(string[] args)
        {
            if (Win32.GetConsoleMode(Win32.GetStdHandle(Win32.STD_OUTPUT_HANDLE), out ConsoleMode mode))
                Win32.SetConsoleMode(Win32.GetStdHandle(Win32.STD_OUTPUT_HANDLE), mode | ConsoleMode.ENABLE_VIRTUAL_TERMINAL_PROCESSING);

            ProgramArgs = ParseArgs(args);

            if (ProgramArgs.ContainsKey("-updaterev"))
            {
                if (!Build.InitializeBuildEnvironment(false))
                {
                    Build.CleanupBuildEnvironment();
                    return;
                }

                PrintColorMessage("Updating header version...", ConsoleColor.Cyan);
                Build.ShowBuildEnvironment();
                Build.UpdateHeaderFileVersion();
                return;
            }

            if (ProgramArgs.ContainsKey("-phapppub_gen"))
            {
                if (!Build.InitializeBuildEnvironment(false))
                {
                    Build.CleanupBuildEnvironment();
                    return;
                }

                PrintColorMessage("Building public headers...", ConsoleColor.Cyan);
                Build.BuildPublicHeaderFiles();
                return;
            }

            if (ProgramArgs.ContainsKey("-sign"))
            {
                Build.BuildKphSignatureFile();
                return;
            }

            if (ProgramArgs.ContainsKey("-cleansdk"))
            {
                if (!Build.InitializeBuildEnvironment(false))
                {
                    Build.CleanupBuildEnvironment();
                    return;
                }

                if (!Build.BuildSolution("ProcessHacker.sln", true, true))
                {
                    Build.CleanupBuildEnvironment();
                    return;
                }

                PrintColorMessage("Copying SDK headers...", ConsoleColor.Cyan);
                if (!Build.CopyPluginSdkHeaders())
                {
                    Build.CleanupBuildEnvironment();
                    return;
                }

                PrintColorMessage("Copying version headers...", ConsoleColor.Cyan);
                if (!Build.CopyVersionHeader())
                {
                    Build.CleanupBuildEnvironment();
                    return;
                }

                PrintColorMessage("Building sdk resource header...", ConsoleColor.Cyan);
                if (!Build.FixupResourceHeader())
                {
                    Build.CleanupBuildEnvironment();
                    return;
                }

                PrintColorMessage("Copying plugin linker files...", ConsoleColor.Cyan);
                if (!Build.CopyLibFiles())
                {
                    Build.CleanupBuildEnvironment();
                    return;
                }

                Build.CleanupBuildEnvironment();
                Build.ShowBuildStats();
                return;
            }

            if (ProgramArgs.ContainsKey("-sdk"))
            {
                if (!Build.InitializeBuildEnvironment(false))
                {
                    Build.CleanupBuildEnvironment();
                    return;
                }

                PrintColorMessage("Copying SDK headers...", ConsoleColor.Cyan);
                if (!Build.CopyPluginSdkHeaders())
                {
                    Build.CleanupBuildEnvironment();
                    return;
                }

                PrintColorMessage("Copying version headers...", ConsoleColor.Cyan);
                if (!Build.CopyVersionHeader())
                {
                    Build.CleanupBuildEnvironment();
                    return;
                }

                PrintColorMessage("Building sdk resource header...", ConsoleColor.Cyan);
                if (!Build.FixupResourceHeader())
                {
                    Build.CleanupBuildEnvironment();
                    return;
                }

                PrintColorMessage("Copying plugin linker files...", ConsoleColor.Cyan);
                if (!Build.CopyLibFiles())
                {
                    Build.CleanupBuildEnvironment();
                    return;
                }

                Build.CleanupBuildEnvironment();
                Build.ShowBuildStats();
                return;
            }

            if (ProgramArgs.ContainsKey("-exe"))
            {
                if (!Build.InitializeBuildEnvironment(false))
                {
                    Build.CleanupBuildEnvironment();
                    return;
                }

                if (!Build.BuildBinZip())
                {
                    Build.CleanupBuildEnvironment();
                    return;
                }

                if (!Build.BuildSetupExe())
                {
                    Build.CleanupBuildEnvironment();
                    return;
                }

                Build.BuildSrcZip();
                Build.BuildSdkZip();
                return;
            }

            if (!Build.InitializeBuildEnvironment(true))
            {
                Build.CleanupBuildEnvironment();
                return;
            }

            PrintColorMessage("Setting up build environment...", ConsoleColor.Cyan);
            Build.ShowBuildEnvironment();
            Build.BuildSecureFiles();

            if (!Build.BuildSolution("ProcessHacker.sln", true, true))
            {
                Build.CleanupBuildEnvironment();
                return;
            }

            PrintColorMessage("Building KPH signature...", ConsoleColor.Cyan);
            if (!Build.BuildKphSignatureFile())
            {
                Build.CleanupBuildEnvironment();
                return;
            }

            PrintColorMessage("Copying text files...", ConsoleColor.Cyan);
            if (!Build.CopyTextFiles())
            {
                Build.CleanupBuildEnvironment();
                return;
            }

            PrintColorMessage("Copying KPH driver...", ConsoleColor.Cyan);
            if (!Build.CopyKProcessHacker())
            {
                Build.CleanupBuildEnvironment();
                return;
            }

            PrintColorMessage("Copying SDK headers...", ConsoleColor.Cyan);
            if (!Build.CopyPluginSdkHeaders())
            {
                Build.CleanupBuildEnvironment();
                return;
            }

            PrintColorMessage("Copying version headers...", ConsoleColor.Cyan);
            if (!Build.CopyVersionHeader())
            {
                Build.CleanupBuildEnvironment();
                return;
            }

            PrintColorMessage("Building sdk resource header...", ConsoleColor.Cyan);
            if (!Build.FixupResourceHeader())
            {
                Build.CleanupBuildEnvironment();
                return;
            }

            PrintColorMessage("Copying plugin linker files...", ConsoleColor.Cyan);
            if (!Build.CopyLibFiles())
            {
                Build.CleanupBuildEnvironment();
                return;
            }

            if (!Build.BuildSolution("plugins\\Plugins.sln", true, true))
            {
                Build.CleanupBuildEnvironment();
                return;
            }

            PrintColorMessage("Copying Wow64 support files...", ConsoleColor.Cyan);
            if (!Build.CopyWow64Files())
            {
                Build.CleanupBuildEnvironment();
                return;
            }

            PrintColorMessage("Building build-setup.exe...", ConsoleColor.Cyan);
            if (!Build.BuildSetupExe())
            {
                Build.CleanupBuildEnvironment();
                return;
            }

            PrintColorMessage("Building build-bin.zip...", ConsoleColor.Cyan);
            if (!Build.BuildBinZip())
            {
                Build.CleanupBuildEnvironment();
                return;
            }

            //Build.BuildSrcZip();
            //Build.BuildSdkZip();
            //Build.BuildPdbZip();

            PrintColorMessage("Building build-checksums.txt...", ConsoleColor.Cyan);
            if (!Build.BuildChecksumsFile())
            {
                Build.CleanupBuildEnvironment();
                return;
            }

            PrintColorMessage("Building release signature...", ConsoleColor.Cyan);
            Build.BuildUpdateSignature();

            PrintColorMessage("Moving build-setup.exe...", ConsoleColor.Cyan);
            if (!Build.MoveBuildFiles())
            {
                Build.CleanupBuildEnvironment();
                return;
            }

            Build.WebServiceUploadBuild();
            Build.WebServiceUpdateConfig();

            Build.CleanupBuildEnvironment();
            Build.ShowBuildStats();

            Console.ReadKey();
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