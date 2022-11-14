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
            ProgramArgs = ParseArgs(args);

            if (!Build.InitializeBuildEnvironment())
                return;

            if (ProgramArgs.ContainsKey("-cleanup"))
            {
                Build.CleanupBuildEnvironment();
                Build.ShowBuildStats();
            }
            //else if (ProgramArgs.ContainsKey("-appxbuild"))
            //{
            //    Build.SetupBuildEnvironment(true);
            //
            //    if (!Build.BuildSolution("SystemInformer.sln",
            //        BuildFlags.Build32bit | BuildFlags.Build64bit |
            //        BuildFlags.BuildVerbose | BuildFlags.BuildApi
            //        ))
            //        return;
            //
            //    if (!Build.BuildSdk(BuildFlags.Build32bit | BuildFlags.Build64bit | BuildFlags.BuildVerbose))
            //        return;
            //
            //    if (!Build.BuildSolution("plugins\\Plugins.sln",
            //        BuildFlags.Build32bit | BuildFlags.Build64bit |
            //        BuildFlags.BuildVerbose | BuildFlags.BuildApi
            //        ))
            //        return;
            //
            //    if (!Build.CopyKernelDriver(BuildFlags.Build32bit | BuildFlags.Build64bit | BuildFlags.BuildVerbose))
            //        return;
            //
            //    if (!Build.CopyTextFiles())
            //        return;
            //    if (!Build.CopyWow64Files(BuildFlags.None))
            //        return;
            //    if (!Build.CopySidCapsFile(BuildFlags.Build32bit | BuildFlags.Build64bit | BuildFlags.BuildVerbose))
            //        return;
            //    if (!Build.CopyEtwTraceGuidsFile(BuildFlags.Build32bit | BuildFlags.Build64bit | BuildFlags.BuildVerbose))
            //        return;
            //
            //    if (!Build.BuildBinZip())
            //        return;
            //    if (!Build.BuildSetupExe())
            //        return;
            //    Build.BuildPdbZip();
            //    Build.BuildSdkZip();
            //    //Build.BuildSrcZip();
            //    Build.BuildChecksumsFile();
            //
            //    Build.BuildAppxPackage(
            //        BuildFlags.Build32bit | BuildFlags.Build64bit | BuildFlags.BuildVerbose
            //        );
            //
            //    Build.ShowBuildStats();
            //}
            else if (ProgramArgs.ContainsKey("-encrypt"))
            {
                Verify.Encrypt(ProgramArgs["-input"], ProgramArgs["-output"], ProgramArgs["-secret"]);
            }
            else if (ProgramArgs.ContainsKey("-decrypt"))
            {
                Verify.Decrypt(ProgramArgs["-input"], ProgramArgs["-output"], ProgramArgs["-secret"]);
            }
            else if (ProgramArgs.ContainsKey("-dyndata"))
            {
                var kphdyn = System.IO.Path.Combine(ProgramArgs["-dyndata"], "kphdyn.xml");
                var source = System.IO.Path.Combine(ProgramArgs["-dyndata"], "kphdyn.c");
                var header = System.IO.Path.Combine(ProgramArgs["-dyndata"], "include", "kphdyn.h");

                Program.PrintColorMessage(kphdyn, ConsoleColor.White);
                Program.PrintColorMessage(source, ConsoleColor.White);
                Program.PrintColorMessage(header, ConsoleColor.White);

                var dyn = new DynData(Verify.GetCustomSignToolFilePath(), kphdyn, Verify.GetPath("kph.key"));

                System.IO.File.WriteAllText(source, dyn.Source);
                System.IO.File.WriteAllText(header, dyn.Header);
                Program.PrintColorMessage("Done!", ConsoleColor.Green);
            }
            else if (ProgramArgs.ContainsKey("-phapppub_gen"))
            {
                Build.BuildPublicHeaderFiles();
            }
            else if (ProgramArgs.ContainsKey("-sdk"))
            {
                if (ProgramArgs.ContainsKey("-x64"))
                {
                    if (ProgramArgs.ContainsKey("-release"))
                    {
                        Build.CopySidCapsFile(BuildFlags.Build64bit | BuildFlags.BuildVerbose);
                        Build.CopyEtwTraceGuidsFile(BuildFlags.Build64bit | BuildFlags.BuildVerbose);
                        Build.BuildSdk(BuildFlags.Build64bit | BuildFlags.BuildVerbose);
                        Build.CopyKernelDriver(BuildFlags.Build64bit | BuildFlags.BuildVerbose);
                        Build.CopyWow64Files(BuildFlags.Build64bit | BuildFlags.BuildVerbose);
                    }
                    else
                    {
                        Build.CopySidCapsFile(BuildFlags.Build64bit | BuildFlags.BuildDebug | BuildFlags.BuildVerbose);
                        Build.CopyEtwTraceGuidsFile(BuildFlags.Build64bit | BuildFlags.BuildDebug | BuildFlags.BuildVerbose);
                        Build.BuildSdk(BuildFlags.Build64bit | BuildFlags.BuildDebug | BuildFlags.BuildVerbose);
                        Build.CopyKernelDriver(BuildFlags.Build64bit | BuildFlags.BuildDebug | BuildFlags.BuildVerbose);
                        Build.CopyWow64Files(BuildFlags.Build64bit | BuildFlags.BuildDebug | BuildFlags.BuildVerbose);
                    }
                }
                else if (ProgramArgs.ContainsKey("-win32"))
                {
                    if (ProgramArgs.ContainsKey("-release"))
                    {
                        Build.CopySidCapsFile(BuildFlags.Build32bit | BuildFlags.BuildRelease);
                        Build.CopyEtwTraceGuidsFile(BuildFlags.Build32bit | BuildFlags.BuildRelease);
                        Build.BuildSdk(BuildFlags.Build32bit | BuildFlags.BuildRelease);
                        Build.CopyKernelDriver(BuildFlags.Build32bit | BuildFlags.BuildRelease);
                    }
                    else
                    {
                        Build.CopySidCapsFile(BuildFlags.Build32bit | BuildFlags.BuildDebug);
                        Build.CopyEtwTraceGuidsFile(BuildFlags.Build32bit | BuildFlags.BuildDebug);
                        Build.BuildSdk(BuildFlags.Build32bit | BuildFlags.BuildDebug);
                        Build.CopyKernelDriver(BuildFlags.Build32bit | BuildFlags.BuildDebug);
                    }
                }
                else
                {
                    Build.CopySidCapsFile(BuildFlags.Build32bit | BuildFlags.Build64bit | BuildFlags.BuildDebug | BuildFlags.BuildRelease);
                    Build.CopyEtwTraceGuidsFile(BuildFlags.Build32bit | BuildFlags.Build64bit | BuildFlags.BuildDebug | BuildFlags.BuildRelease);
                    Build.BuildSdk(BuildFlags.Build32bit | BuildFlags.Build64bit | BuildFlags.BuildDebug | BuildFlags.BuildRelease);
                    Build.CopyKernelDriver(BuildFlags.Build32bit | BuildFlags.Build64bit | BuildFlags.BuildDebug | BuildFlags.BuildRelease);
                    Build.CopyWow64Files(BuildFlags.Build32bit | BuildFlags.Build64bit | BuildFlags.BuildDebug | BuildFlags.BuildRelease);
                }
            }
            else if (ProgramArgs.ContainsKey("-sign_plugin"))
            {
                Build.SignPlugin(ProgramArgs["-sign_plugin"]);
            }
            else if (ProgramArgs.ContainsKey("-cleansdk"))
            {
                if (!Build.BuildSolution("SystemInformer.sln",
                    BuildFlags.Build32bit | BuildFlags.Build64bit |
                    BuildFlags.BuildVerbose | BuildFlags.BuildApi
                    ))
                {
                    return;
                }

                Build.BuildSdk(BuildFlags.Build32bit | BuildFlags.Build64bit | BuildFlags.BuildDebug | BuildFlags.BuildRelease);

                Build.ShowBuildStats();
            }
            else if (ProgramArgs.ContainsKey("-bin"))
            {
                Build.SetupBuildEnvironment(false);

                if (!Build.BuildSolution("SystemInformer.sln",
                    BuildFlags.Build32bit | BuildFlags.Build64bit |
                    BuildFlags.BuildVerbose | BuildFlags.BuildApi
                    ))
                    return;

                if (!Build.BuildSdk(BuildFlags.Build32bit | BuildFlags.Build64bit | BuildFlags.BuildDebug | BuildFlags.BuildRelease))
                    return;

                if (!Build.BuildSolution("plugins\\Plugins.sln",
                    BuildFlags.Build32bit | BuildFlags.Build64bit |
                    BuildFlags.BuildVerbose | BuildFlags.BuildApi
                    ))
                    return;

                if (!Build.CopyKernelDriver(BuildFlags.Build32bit | BuildFlags.Build64bit | BuildFlags.BuildDebug | BuildFlags.BuildRelease))
                    return;

                if (!Build.CopyTextFiles())
                    return;
                if (!Build.CopyWow64Files(BuildFlags.Build32bit | BuildFlags.Build64bit | BuildFlags.BuildDebug | BuildFlags.BuildRelease))
                    return;
                //if (!Build.CopySidCapsFile(BuildFlags.Build32bit | BuildFlags.Build64bit | BuildFlags.BuildVerbose))
                //    return;
                //if (!Build.CopyEtwTraceGuidsFile(BuildFlags.Build32bit | BuildFlags.Build64bit | BuildFlags.BuildVerbose))
                //    return;

                if (!Build.BuildBinZip())
                    return;

                Build.ShowBuildStats();
            }
            else if (ProgramArgs.ContainsKey("-debug"))
            {
                Build.SetupBuildEnvironment(true);

                Build.BuildVersionInfo(
                    BuildFlags.Build32bit | BuildFlags.Build64bit | BuildFlags.BuildVerbose
                    );

                if (!Build.BuildSolution("SystemInformer.sln",
                    BuildFlags.Build32bit | BuildFlags.Build64bit |
                    BuildFlags.BuildDebug | BuildFlags.BuildVerbose |
                    BuildFlags.BuildApi
                    ))
                    return;

                if (!Build.BuildSdk(BuildFlags.Build32bit | BuildFlags.Build64bit | BuildFlags.BuildDebug | BuildFlags.BuildRelease))
                    return;

                if (!Build.BuildSolution("plugins\\Plugins.sln",
                    BuildFlags.Build32bit | BuildFlags.Build64bit |
                    BuildFlags.BuildDebug | BuildFlags.BuildVerbose |
                    BuildFlags.BuildApi
                    ))
                {
                    return;
                }

                if (!Build.CopyKernelDriver(BuildFlags.Build32bit | BuildFlags.Build64bit | BuildFlags.BuildDebug | BuildFlags.BuildRelease))
                    return;

                if (!Build.CopyWow64Files(BuildFlags.Build32bit | BuildFlags.Build64bit | BuildFlags.BuildDebug | BuildFlags.BuildRelease))
                    return;

                //if (!Build.CopySidCapsFile(
                //    BuildFlags.Build32bit | BuildFlags.Build64bit |
                //    BuildFlags.BuildDebug | BuildFlags.BuildVerbose))
                //    return;
                //if (!Build.CopyEtwTraceGuidsFile(
                //    BuildFlags.Build32bit | BuildFlags.Build64bit |
                //    BuildFlags.BuildDebug | BuildFlags.BuildVerbose))
                //    return;

                Build.ShowBuildStats();
            }
            else if (ProgramArgs.ContainsKey("-appveyor"))
            {
                Build.SetupBuildEnvironment(true);

                Build.BuildVersionInfo(
                    BuildFlags.Build32bit | BuildFlags.Build64bit | BuildFlags.BuildVerbose
                    );

                if (!Build.BuildSolution("SystemInformer.sln",
                    BuildFlags.Build32bit | BuildFlags.Build64bit |
                    BuildFlags.BuildVerbose | BuildFlags.BuildApi
                    ))
                    Environment.Exit(1);

                if (!Build.BuildSdk(BuildFlags.Build32bit | BuildFlags.Build64bit | BuildFlags.BuildDebug | BuildFlags.BuildRelease))
                    Environment.Exit(1);

                if (!Build.BuildSolution("plugins\\Plugins.sln",
                    BuildFlags.Build32bit | BuildFlags.Build64bit |
                    BuildFlags.BuildVerbose | BuildFlags.BuildApi
                    ))
                    Environment.Exit(1);

                if (!Build.CopyKernelDriver(BuildFlags.Build32bit | BuildFlags.Build64bit | BuildFlags.BuildDebug | BuildFlags.BuildRelease))
                    Environment.Exit(1);

                if (!Build.CopyTextFiles())
                    Environment.Exit(1);
                if (!Build.CopyWow64Files(BuildFlags.Build32bit | BuildFlags.Build64bit | BuildFlags.BuildDebug | BuildFlags.BuildRelease))
                    Environment.Exit(1);
                //if (!Build.CopySidCapsFile(BuildFlags.Build32bit | BuildFlags.Build64bit | BuildFlags.BuildVerbose))
                //    Environment.Exit(1);
                //if (!Build.CopyEtwTraceGuidsFile(BuildFlags.Build32bit | BuildFlags.Build64bit | BuildFlags.BuildVerbose))
                //    Environment.Exit(1);

                if (!Build.BuildBinZip())
                    Environment.Exit(1);
                if (!Build.BuildSetupExe())
                    Environment.Exit(1);
                if (!Build.BuildPdbZip())
                    Environment.Exit(1);
                //if (!Build.BuildSdkZip())
                //    Environment.Exit(1);
                //if (!Build.BuildSrcZip())
                //    Environment.Exit(1);
                //if (!Build.BuildChecksumsFile())
                //    Environment.Exit(1);
                if (!Build.BuildDeployUploadArtifacts())
                    Environment.Exit(1);
                if (!Build.BuildDeployUpdateConfig())
                    Environment.Exit(1);
            }
            else
            {
                Build.SetupBuildEnvironment(true);

                Build.BuildVersionInfo(
                    BuildFlags.Build32bit | BuildFlags.Build64bit | BuildFlags.BuildVerbose
                    );

                if (!Build.BuildSolution("SystemInformer.sln",
                    BuildFlags.Build32bit | BuildFlags.Build64bit |
                    BuildFlags.BuildVerbose | BuildFlags.BuildApi
                    ))
                    return;

                if (!Build.BuildSdk(BuildFlags.Build32bit | BuildFlags.Build64bit | BuildFlags.BuildDebug | BuildFlags.BuildRelease))
                    return;

                if (!Build.BuildSolution("plugins\\Plugins.sln",
                    BuildFlags.Build32bit | BuildFlags.Build64bit |
                    BuildFlags.BuildVerbose | BuildFlags.BuildApi
                    ))
                    return;

                if (!Build.CopyKernelDriver(BuildFlags.Build32bit | BuildFlags.Build64bit | BuildFlags.BuildDebug | BuildFlags.BuildRelease))
                    return;

                if (!Build.CopyTextFiles())
                    return;
                if (!Build.CopyWow64Files(BuildFlags.Build32bit | BuildFlags.Build64bit | BuildFlags.BuildDebug | BuildFlags.BuildRelease))
                    return;
                //if (!Build.CopySidCapsFile(BuildFlags.Build32bit | BuildFlags.Build64bit | BuildFlags.BuildVerbose))
                //    return;
                //if (!Build.CopyEtwTraceGuidsFile(BuildFlags.Build32bit | BuildFlags.Build64bit | BuildFlags.BuildVerbose))
                //    return;

                if (!Build.BuildBinZip())
                    return;
                if (!Build.BuildSetupExe())
                    return;
                Build.BuildPdbZip();
                //Build.BuildSdkZip();
                //Build.BuildSrcZip();
                Build.BuildChecksumsFile();
                Build.ShowBuildStats();
            }

            Environment.Exit(Environment.ExitCode);
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
    }
}
