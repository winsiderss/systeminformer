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
                Verify.Encrypt(ProgramArgs["-input"], ProgramArgs["-output"], ProgramArgs["-secret"]);
            }
            else if (ProgramArgs.ContainsKey("-decrypt"))
            {
                if (!Verify.Decrypt(ProgramArgs["-input"], ProgramArgs["-output"], ProgramArgs["-secret"]))
                {
                    Environment.Exit(1);
                }
            }
            else if (ProgramArgs.ContainsKey("-dyndata"))
            {
                if (!Build.BuildDynamicData(ProgramArgs["-dyndata"]))
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
                if (ProgramArgs.ContainsKey("-x64"))
                {
                    if (ProgramArgs.ContainsKey("-release"))
                    {
                        BuildFlags flags = BuildFlags.Build64bit | BuildFlags.BuildRelease | BuildFlags.BuildVerbose;

                        Build.CopySidCapsFile(flags);
                        Build.CopyEtwTraceGuidsFile(flags);
                        Build.CopyIconFile(flags);
                        Build.BuildSdk(flags);
                        Build.CopyKernelDriver(flags);
                        Build.CopyWow64Files(flags);
                    }
                    else
                    {
                        BuildFlags flags = BuildFlags.Build64bit | BuildFlags.BuildDebug | BuildFlags.BuildVerbose;

                        Build.CopySidCapsFile(flags);
                        Build.CopyEtwTraceGuidsFile(flags);
                        Build.CopyIconFile(flags);
                        Build.BuildSdk(flags);
                        Build.CopyKernelDriver(flags);
                        Build.CopyWow64Files(flags);
                    }
                }
                else if (ProgramArgs.ContainsKey("-win32"))
                {
                    if (ProgramArgs.ContainsKey("-release"))
                    {
                        BuildFlags flags = BuildFlags.Build32bit | BuildFlags.BuildRelease | BuildFlags.BuildVerbose;

                        Build.CopySidCapsFile(flags);
                        Build.CopyEtwTraceGuidsFile(flags);
                        Build.CopyIconFile(flags);
                        Build.BuildSdk(flags);
                        Build.CopyKernelDriver(flags);
                    }
                    else
                    {
                        BuildFlags flags = BuildFlags.Build32bit | BuildFlags.BuildDebug | BuildFlags.BuildVerbose;

                        Build.CopySidCapsFile(flags);
                        Build.CopyEtwTraceGuidsFile(flags);
                        Build.CopyIconFile(flags);
                        Build.BuildSdk(flags);
                        Build.CopyKernelDriver(flags);
                    }
                }
                else if (ProgramArgs.ContainsKey("-arm64"))
                {
                    if (ProgramArgs.ContainsKey("-release"))
                    {
                        BuildFlags flags = BuildFlags.BuildArm64bit | BuildFlags.BuildRelease | BuildFlags.BuildVerbose;

                        Build.CopySidCapsFile(flags);
                        Build.CopyEtwTraceGuidsFile(flags);
                        Build.CopyIconFile(flags);
                        Build.BuildSdk(flags);
                        Build.CopyKernelDriver(flags);
                        Build.CopyWow64Files(flags);
                    }
                    else
                    {
                        BuildFlags flags = BuildFlags.BuildArm64bit | BuildFlags.BuildDebug | BuildFlags.BuildVerbose;

                        Build.CopySidCapsFile(flags);
                        Build.CopyEtwTraceGuidsFile(flags);
                        Build.CopyIconFile(flags);
                        Build.BuildSdk(flags);
                        Build.CopyKernelDriver(flags);
                        Build.CopyWow64Files(flags);
                    }
                }
                else
                {
                    BuildFlags flags = 
                        BuildFlags.Build32bit | BuildFlags.Build64bit | BuildFlags.BuildArm64bit | 
                        BuildFlags.BuildDebug | BuildFlags.BuildRelease;

                    Build.CopySidCapsFile(flags);
                    Build.CopyEtwTraceGuidsFile(flags);
                    Build.CopyIconFile(flags);
                    Build.BuildSdk(flags);
                    Build.CopyKernelDriver(flags);
                    Build.CopyWow64Files(flags);
                }
            }
            else if (ProgramArgs.ContainsKey("-sign_plugin"))
            {
                if (!Build.SignPlugin(ProgramArgs["-sign_plugin"]))
                {
                    Environment.Exit(1);
                }
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
                BuildFlags flags =
                    BuildFlags.Build32bit | BuildFlags.Build64bit | BuildFlags.BuildArm64bit |
                    BuildFlags.BuildVerbose | BuildFlags.BuildApi;

                Build.SetupBuildEnvironment(false);

                if (!Build.BuildSolution("SystemInformer.sln", flags))
                    return;
                if (!Build.BuildSolution("plugins\\Plugins.sln", flags))
                    return;

                if (!Build.CopyTextFiles(true))
                    return;

                foreach (var (channel, _) in BuildConfig.Build_Channels)
                {
                    if (!Build.BuildBinZip(channel))
                        return;
                }

                Build.ShowBuildStats();
                Build.CopyTextFiles(false);
            }
            else if (ProgramArgs.ContainsKey("-debug"))
            {
                BuildFlags flags =
                    BuildFlags.Build32bit | BuildFlags.Build64bit | BuildFlags.BuildArm64bit |
                    BuildFlags.BuildVerbose | BuildFlags.BuildApi;

                Build.SetupBuildEnvironment(true);

                if (!Build.BuildSolution("SystemInformer.sln", flags))
                    return;
                if (!Build.BuildSolution("plugins\\Plugins.sln", flags))
                    return;

                Build.ShowBuildStats();
            }
            else if (ProgramArgs.ContainsKey("-pipeline-build"))
            {
                BuildFlags flags =
                    BuildFlags.Build32bit | BuildFlags.Build64bit | BuildFlags.BuildArm64bit |
                    BuildFlags.BuildRelease | BuildFlags.BuildVerbose |
                    BuildFlags.BuildApi | BuildFlags.BuildSourceLink;

                if (ProgramArgs.ContainsKey("-msix-build"))
                    flags |= BuildFlags.BuildMsix;

                Build.SetupBuildEnvironment(true);

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
                BuildFlags flags =
                    BuildFlags.Build32bit | BuildFlags.Build64bit | BuildFlags.BuildArm64bit |
                    BuildFlags.BuildRelease | BuildFlags.BuildVerbose | BuildFlags.BuildApi | 
                    BuildFlags.BuildMsix | BuildFlags.BuildSourceLink;

                Build.SetupBuildEnvironment(true);

                if (!Build.BuildSolution("SystemInformer.sln", flags))
                    Environment.Exit(1);

                if (!Build.BuildSolution("plugins\\Plugins.sln", flags))
                    Environment.Exit(1);

                if (!Build.CopyWow64Files(flags)) // required after plugin build (dmex)
                    return;
                if (!Build.CopyTextFiles(true))
                    return;

                Build.BuildStorePackage(flags);

                Build.BuildPdbZip(true);

                Build.CopyTextFiles(false);
            }
            else
            {
                BuildFlags flags =
                    BuildFlags.Build32bit | BuildFlags.Build64bit | BuildFlags.BuildArm64bit |
                    BuildFlags.BuildVerbose | BuildFlags.BuildApi;

                Build.SetupBuildEnvironment(true);

                if (!Build.BuildSolution("SystemInformer.sln", flags))
                    return;
                if (!Build.BuildSolution("plugins\\Plugins.sln", flags))
                    return;

                Build.CopyWow64Files(flags);

                if (!Build.CopyTextFiles(true))
                    return;

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
    }

    [Flags]
    public enum BuildFlags
    {
        None,
        Build32bit = 1,
        Build64bit = 2,
        BuildArm64bit = 4,
        BuildDebug = 8,
        BuildRelease = 16,
        BuildVerbose = 32,
        BuildApi = 64,
        BuildMsix = 128,
        BuildSourceLink = 8192
    }
}
