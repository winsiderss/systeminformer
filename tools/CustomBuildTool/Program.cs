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
                Build.CleanupBuildEnvironment();
                Build.ShowBuildStats();
            }
            else if (ProgramArgs.ContainsKey("-encrypt"))
            {
                if (!Verify.EncryptFile(ProgramArgs["-input"], ProgramArgs["-output"], ProgramArgs["-secret"]))
                {
                    Environment.Exit(1);
                }
            }
            else if (ProgramArgs.ContainsKey("-decrypt"))
            {
                if (!Verify.DecryptFile(ProgramArgs["-input"], ProgramArgs["-output"], ProgramArgs["-secret"]))
                {
                    Environment.Exit(1);
                }
            }
            else if (ProgramArgs.TryGetValue("-dyndata", out string ArgDynData))
            {
                if (!Build.BuildDynamicData(ArgDynData))
                {
                    Environment.Exit(1);
                }
            }
            else if (ProgramArgs.ContainsKey("-phapppub_gen"))
            {
                if (!Build.BuildPublicHeaderFiles())
                {
                    Environment.Exit(1);
                }
            }
            else if (ProgramArgs.ContainsKey("-sdk"))
            {
                var flags = BuildFlags.BuildVerbose;

                Build.SetupBuildEnvironment(false);

                if (ProgramArgs.ContainsKey("-Debug"))
                {
                    if (ProgramArgs.ContainsKey("-Win32"))
                        flags |= BuildFlags.BuildDebug | BuildFlags.Build32bit;
                    else if (ProgramArgs.ContainsKey("-x64"))
                        flags |= BuildFlags.BuildDebug | BuildFlags.Build64bit;
                    else if (ProgramArgs.ContainsKey("-arm64"))
                        flags |= BuildFlags.BuildDebug | BuildFlags.BuildArm64bit;
                    else
                    {
                        Environment.Exit(1);
                    }
                }
                else if (ProgramArgs.ContainsKey("-release"))
                {
                    if (ProgramArgs.ContainsKey("-Win32"))
                        flags |= BuildFlags.BuildRelease | BuildFlags.Build32bit;
                    else if (ProgramArgs.ContainsKey("-x64"))
                        flags |= BuildFlags.BuildRelease | BuildFlags.Build64bit;
                    else if (ProgramArgs.ContainsKey("-arm64"))
                        flags |= BuildFlags.BuildRelease | BuildFlags.BuildArm64bit;
                    else
                    {
                        Environment.Exit(1);
                    }
                }
                else
                {
                    Environment.Exit(1);
                }

                if (!Build.CopyResourceFiles(flags))
                    Environment.Exit(1);

                if (!Build.BuildSdk(flags))
                    Environment.Exit(1);

                if (!Build.CopyKernelDriver(flags))
                    Environment.Exit(1);

                if (!Build.CopyWow64Files(flags))
                    Environment.Exit(1);
            }
            else if (ProgramArgs.TryGetValue("-kph-sign", out string SignArg))
            {
                if (!Verify.CreateSigFile("kph", SignArg, Build.BuildCanary))
                    Environment.Exit(1);
            }
            else if (ProgramArgs.ContainsKey("-cleansdk"))
            {
                BuildFlags flags =
                    BuildFlags.Build32bit | BuildFlags.Build64bit | BuildFlags.BuildArm64bit |
                    BuildFlags.BuildVerbose | BuildFlags.BuildApi;

                if (!Build.BuildSolution("SystemInformer.sln", flags))
                    return;

                Build.ShowBuildStats();
            }
            else if (ProgramArgs.ContainsKey("-bin"))
            {
                Build.SetupBuildEnvironment(false);

                if (!Build.BuildSolution("SystemInformer.sln", BuildFlags.Release))
                    Environment.Exit(1);
                if (!Build.BuildSolution("plugins\\Plugins.sln", BuildFlags.Release))
                    Environment.Exit(1);

                if (!Build.CopyDebugEngineFiles(BuildFlags.Release))
                    Environment.Exit(1);
                if (!Build.CopyTextFiles(true))
                    Environment.Exit(1);

                foreach (var (channel, _) in BuildConfig.Build_Channels)
                {
                    if (!Build.BuildBinZip(channel))
                        Environment.Exit(1);
                }

                Build.ShowBuildStats();
                Build.CopyTextFiles(false);
            }
            else if (ProgramArgs.ContainsKey("-debug"))
            {
                Build.SetupBuildEnvironment(true);

                if (!Build.BuildSolution("SystemInformer.sln", BuildFlags.Debug))
                    return;
                if (!Build.BuildSolution("plugins\\Plugins.sln", BuildFlags.Debug))
                    return;

                if (!Build.CopyDebugEngineFiles(BuildFlags.Debug))
                    Environment.Exit(1);

                Build.ShowBuildStats();
            }
            else if (ProgramArgs.ContainsKey("-pipeline-build"))
            {
                BuildFlags flags = BuildFlags.Release;

                if (ProgramArgs.ContainsKey("-msix-build"))
                    flags |= BuildFlags.BuildMsix;

                Build.SetupBuildEnvironment(true);

                Build.CopySourceLink(true);

                if (!Build.BuildSolution("SystemInformer.sln", flags))
                    Environment.Exit(1);
                if (!Build.BuildSolution("plugins\\Plugins.sln", flags))
                    Environment.Exit(1);

                Build.CopyWow64Files(flags); // required after plugin build (dmex)
            }
            else if (ProgramArgs.ContainsKey("-pipeline-package"))
            {
                Build.SetupBuildEnvironment(true);

                if (!Build.ResignFiles())
                    Environment.Exit(1);
                if (!Build.CopyDebugEngineFiles(BuildFlags.Release))
                    Environment.Exit(1);
                if (!Build.CopyTextFiles(true))
                    Environment.Exit(1);

                foreach (var (channel, _) in BuildConfig.Build_Channels)
                {
                    if (!Build.BuildBinZip(channel))
                        Environment.Exit(1);
                    if (!Build.BuildSetupExe(channel))
                        Environment.Exit(1);
                }
            }
            else if (ProgramArgs.ContainsKey("-pipeline-deploy"))
            {
                Build.SetupBuildEnvironment(true);

                if (!Build.BuildPdbZip(false))
                    Environment.Exit(1);
                //if (!Build.BuildSdkZip())
                //    Environment.Exit(1);
                //if (!Build.BuildSrcZip())
                //    Environment.Exit(1);
                //if (!Build.BuildChecksumsFile())
                //    Environment.Exit(1);
                //if (!Build.BuildDeployUploadArtifacts())
                //    Environment.Exit(1);
                if (!Build.BuildDeployUpdateConfig())
                    Environment.Exit(1);
            }
            else if (ProgramArgs.ContainsKey("-msix-build"))
            {
                BuildFlags flags = BuildFlags.Release | BuildFlags.BuildMsix;

                Build.SetupBuildEnvironment(true);

                if (!Build.BuildSolution("SystemInformer.sln", flags))
                    Environment.Exit(1);
                if (!Build.BuildSolution("plugins\\Plugins.sln", flags))
                    Environment.Exit(1);

                if (!Build.CopyDebugEngineFiles(flags))
                    Environment.Exit(1);
                if (!Build.CopyWow64Files(flags)) // required after plugin build (dmex)
                    Environment.Exit(1);
                if (!Build.CopyTextFiles(true))
                    Environment.Exit(1);

                Build.BuildStorePackage(flags);

                Build.BuildPdbZip(true);

                Build.CopyTextFiles(false);
            }
            else
            {
                Build.SetupBuildEnvironment(true);

                if (!Build.BuildSolution("SystemInformer.sln", BuildFlags.Release))
                    Environment.Exit(1);
                if (!Build.BuildSolution("plugins\\Plugins.sln", BuildFlags.Release))
                    Environment.Exit(1);

                if (!Build.CopyDebugEngineFiles(BuildFlags.Release))
                    Environment.Exit(1);
                if (!Build.CopyWow64Files(BuildFlags.Release))
                    Environment.Exit(1);
                if (!Build.CopyTextFiles(true))
                    Environment.Exit(1);

                foreach (var (channel, _) in BuildConfig.Build_Channels)
                {
                    if (!Build.BuildBinZip(channel))
                        Environment.Exit(1);
                    if (!Build.BuildSetupExe(channel))
                        Environment.Exit(1);
                }

                Build.BuildPdbZip(false);
                //Build.BuildSdkZip();
                //Build.BuildSrcZip();
                Build.BuildChecksumsFile();
                Build.ShowBuildStats();
                Build.CopyTextFiles(false);
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

        public static void PrintColorMessage(LogInterpolatedStringHandler builder, ConsoleColor Color, bool Newline = true, BuildFlags Flags = BuildFlags.BuildVerbose)
        {
            if ((Flags & BuildFlags.BuildVerbose) != BuildFlags.BuildVerbose)
                return;

            Console.ForegroundColor = Color;
            if (Newline)
                Console.WriteLine(builder.GetFormattedText());
            else
                Console.Write(builder.GetFormattedText());
            Console.ResetColor();
        }
    }
}
