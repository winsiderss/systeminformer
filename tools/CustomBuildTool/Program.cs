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
    public static class Program
    {
        private static Dictionary<string, string> ProgramArgs;

        public static void Main(string[] args)
        {
            if (!Build.InitializeBuildEnvironment())
                return;

            ProgramArgs = Utils.ParseArgs(args);

            if (ProgramArgs.ContainsKey("-cleanup"))
            {
                Program.BuildCleanup();
            }
            else if (ProgramArgs.ContainsKey("-dyndata"))
            {
                Program.BuildDriverHeaders();
            }
            else if (ProgramArgs.ContainsKey("-phapppub_gen"))
            {
                Program.BuildPublicHeaders();
            }
            else if (ProgramArgs.ContainsKey("-sdk"))
            {
                Program.BuildSdk();
            }
            else if (ProgramArgs.ContainsKey("-sign_plugin"))
            {
                Program.SignPlugin();
            }
            else if (ProgramArgs.ContainsKey("-encrypt"))
            {
                Program.EncryptDecrypt(true);
            }
            else if (ProgramArgs.ContainsKey("-decrypt"))
            {
                Program.EncryptDecrypt(false);
            }
            else if (ProgramArgs.ContainsKey("-cleansdk"))
            {
                Program.BuildCleanSdk();
            }
            else if (ProgramArgs.ContainsKey("-bin"))
            {
                Program.BuildBin();
            }
            else if (ProgramArgs.ContainsKey("-debug"))
            {
                Program.BuildDebug();
            }
            else if (ProgramArgs.ContainsKey("-nightly-build"))
            {
                Program.BuildNightly();
            }
            else if (ProgramArgs.ContainsKey("-nightly-package"))
            {
                Program.BuildNightlyPackage();
            }
            else if (ProgramArgs.ContainsKey("-nightly-deploy"))
            {
                Program.BuildNightlyDeploy();
            }
            else if (ProgramArgs.ContainsKey("-msix-package"))
            {
                Program.BuildStorePackage();
            }
            else
            {
                Program.BuildBin();
                //Program.BuildDebug();
            }
        }

        private static void BuildCleanup()
        {
            Build.CleanupBuildEnvironment();
            Build.ShowBuildStats();
        }

        private static void BuildCleanSdk()
        {
            Build.BuildSourceLink(true);

            try
            {
                if (!Build.BuildSolution("SystemInformer.sln",
                    BuildFlags.Build32bit | BuildFlags.Build64bit | BuildFlags.BuildARM64 |
                    BuildFlags.BuildRelease | BuildFlags.BuildVerbose | BuildFlags.BuildApi
                    ))
                {
                    Environment.Exit(1);
                    return;
                }
            }
            finally
            {
                Build.BuildSourceLink(false);
            }

            Build.CreateSignatureFiles(
                BuildFlags.Build32bit | BuildFlags.Build64bit | BuildFlags.BuildARM64 |
                BuildFlags.BuildRelease | BuildFlags.BuildVerbose | BuildFlags.BuildApi
                );

            Build.ShowBuildStats();
        }

        private static void BuildDriverHeaders()
        {
            if (!Build.BuildDynamicHeaderFiles())
                Environment.Exit(1);
        }

        private static void BuildPublicHeaders()
        {
            if (!Build.BuildPublicHeaderFiles())
                Environment.Exit(1);
        }

        private static void BuildSdk()
        {
            BuildFlags flags = BuildFlags.BuildVerbose;

            if (ProgramArgs.ContainsKey("-release"))
            {
                flags |= BuildFlags.BuildRelease;
            }
            else
            {
                flags |= BuildFlags.BuildDebug;
            }

            if (ProgramArgs.ContainsKey("-x64"))
            {
                flags |= BuildFlags.Build64bit;
            }
            else if (ProgramArgs.ContainsKey("-win32"))
            {
                flags |= BuildFlags.Build32bit;
            }
            else if (ProgramArgs.ContainsKey("-arm64"))
            {
                flags |= BuildFlags.BuildARM64;
            }
            else
            {
                flags |= BuildFlags.BuildDebug |
                    BuildFlags.BuildRelease |
                    BuildFlags.Build32bit |
                    BuildFlags.Build64bit |
                    BuildFlags.BuildARM64;
            }

            if (!Build.BuildSdk(flags))
                Environment.Exit(1);

            if (!Build.CopyKernelDriver(flags))
                Environment.Exit(1);

            if (!Build.CopyWow64Files(flags))
                Environment.Exit(1);

            if (!Build.BuildSdk(flags))
                Environment.Exit(1);

            if (!Build.CopyKernelDriver(flags))
                Environment.Exit(1);

            if (!Build.CopyResourceFiles(flags))
                Environment.Exit(1);

            if (!Build.CopyWow64Files(flags))
                Environment.Exit(1);
        }

        private static void BuildBin()
        {
            Build.SetupBuildEnvironment(true);
            Build.BuildSourceLink(true);

            try
            {
                if (!Build.BuildSolution("SystemInformer.sln",
                    BuildFlags.Build32bit | BuildFlags.Build64bit | BuildFlags.BuildARM64 |
                    BuildFlags.BuildRelease | BuildFlags.BuildVerbose | BuildFlags.BuildApi
                    ))
                {
                    Environment.Exit(1);
                    return;
                }

                if (!Build.BuildSolution("plugins\\Plugins.sln",
                    BuildFlags.Build32bit | BuildFlags.Build64bit | BuildFlags.BuildARM64 |
                    BuildFlags.BuildRelease | BuildFlags.BuildVerbose | BuildFlags.BuildApi
                    ))
                {
                    Environment.Exit(1);
                    return;
                }
            }
            finally
            {
                Build.BuildSourceLink(false);
            }

            Build.CreateSignatureFiles(
                BuildFlags.Build32bit | BuildFlags.Build64bit | BuildFlags.BuildARM64 |
                BuildFlags.BuildRelease | BuildFlags.BuildVerbose | BuildFlags.BuildApi
                );

            if (!Build.CopyTextFiles(true))
            {
                Environment.Exit(1);
                return;
            }

            if (!Build.BuildBinZip())
            {
                Environment.Exit(1);
                return;
            }

            Build.ShowBuildStats();
            Build.CopyTextFiles(false);
        }

        private static void BuildDebug()
        {
            Build.SetupBuildEnvironment(true);
            Build.BuildSourceLink(true);

            try
            {
                if (!Build.BuildSolution("SystemInformer.sln",
                    BuildFlags.Build32bit | BuildFlags.Build64bit | BuildFlags.BuildARM64 |
                    BuildFlags.BuildDebug | BuildFlags.BuildVerbose | BuildFlags.BuildApi
                    ))
                {
                    Environment.Exit(1);
                    return;
                }

                if (!Build.BuildSolution("plugins\\Plugins.sln",
                    BuildFlags.Build32bit | BuildFlags.Build64bit | BuildFlags.BuildARM64 |
                    BuildFlags.BuildDebug | BuildFlags.BuildVerbose | BuildFlags.BuildApi
                    ))
                {
                    Environment.Exit(1);
                    return;
                }
            }
            finally
            {
                Build.BuildSourceLink(false);
            }

            Build.CreateSignatureFiles(
                BuildFlags.Build32bit | BuildFlags.Build64bit | BuildFlags.BuildARM64 |
                BuildFlags.BuildDebug | BuildFlags.BuildVerbose | BuildFlags.BuildApi
                );

            Build.ShowBuildStats();
        }

        private static void BuildNightly()
        {
            Build.SetupBuildEnvironment(true);
            Build.BuildSourceLink(true);

            try
            {
                if (!Build.BuildSolution("SystemInformer.sln",
                    BuildFlags.Build32bit | BuildFlags.Build64bit | BuildFlags.BuildARM64 |
                    BuildFlags.BuildRelease | BuildFlags.BuildVerbose | BuildFlags.BuildApi
                    ))
                    Environment.Exit(1);

                if (!Build.BuildSolution("plugins\\Plugins.sln",
                    BuildFlags.Build32bit | BuildFlags.Build64bit | BuildFlags.BuildARM64 |
                    BuildFlags.BuildRelease | BuildFlags.BuildVerbose | BuildFlags.BuildApi
                    ))
                    Environment.Exit(1);
            }
            finally
            {
                Build.BuildSourceLink(false);
            }

            Build.CopyWow64Files( // required after plugin build (dmex)
                BuildFlags.Build32bit | BuildFlags.Build64bit | BuildFlags.BuildARM64 |
                BuildFlags.BuildDebug | BuildFlags.BuildRelease
                );
        }

        private static void BuildNightlyPackage()
        {
            Build.SetupBuildEnvironment(true);

            if (!Build.CreateSignatureFiles(
                BuildFlags.Build32bit | BuildFlags.Build64bit | BuildFlags.BuildARM64 |
                BuildFlags.BuildDebug | BuildFlags.BuildRelease |
                BuildFlags.BuildVerbose | BuildFlags.BuildApi
                ))
            {
                Environment.Exit(1);
            }

            if (!Build.CopyTextFiles(true))
                Environment.Exit(1);

            if (!Build.BuildBinZip())
                Environment.Exit(1);

            if (!Build.BuildSetupExe())
                Environment.Exit(1);
        }

        private static void BuildNightlyDeploy()
        {
            Build.SetupBuildEnvironment(true);

            if (!Build.BuildPdbZip(false))
                Environment.Exit(1);

            if (!Build.BuildDeployUpdateConfig())
                Environment.Exit(1);
        }

        private static void BuildStorePackage()
        {
            BuildFlags flags = BuildFlags.Build32bit |
                BuildFlags.Build64bit | BuildFlags.BuildARM64 |
                BuildFlags.BuildDebug | BuildFlags.BuildVerbose |
                BuildFlags.BuildApi | BuildFlags.BuildMsix;

            if (!Build.CopyTextFiles(true))
                Environment.Exit(1);

            if (!Build.CopyResourceFiles(flags))
                Environment.Exit(1);

            Build.BuildStorePackage(flags);

            if (!Build.CopyTextFiles(false))
                Environment.Exit(1);

            Build.ShowBuildStats();
        }

        private static void SignPlugin()
        {
            if (!Build.SignPlugin(ProgramArgs["-sign_plugin"]))
            {
                Environment.Exit(1);
            }
        }

        private static void EncryptDecrypt(bool Encrypt)
        {
            if (Encrypt)
            {
                if (!Verify.EncryptFile(ProgramArgs["-i"], ProgramArgs["-o"], ProgramArgs["-s"], ProgramArgs["-t"]))
                {
                    Environment.Exit(1);
                }
            }
            else
            {
                if (!Verify.DecryptFile(ProgramArgs["-i"], ProgramArgs["-o"], ProgramArgs["-s"], ProgramArgs["-t"]))
                {
                    Environment.Exit(1);
                }
            }
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

    [Flags]
    public enum BuildFlags
    {
        None,
        Build32bit = 1,
        Build64bit = 2,
        BuildARM64 = 4,
        BuildDebug = 8,
        BuildRelease = 16,
        BuildVerbose = 32,
        BuildApi = 64,
        BuildMsix = 128,
    }
}
