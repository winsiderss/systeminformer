/*
 * Process Hacker Toolchain - 
 *   Build script
 * 
 * Copyright (C) dmex
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
using System.Collections.Generic;

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
            else if (ProgramArgs.ContainsKey("-phapppub_gen"))
            {
                Build.BuildPublicHeaderFiles();
            }
            else if (ProgramArgs.ContainsKey("-sdk"))
            {
                Build.CopyKProcessHacker(
                    BuildFlags.Build32bit | BuildFlags.Build64bit |
                    BuildFlags.BuildDebug
                    );

                Build.BuildSdk(
                    BuildFlags.Build32bit | BuildFlags.Build64bit |
                    BuildFlags.BuildDebug | BuildFlags.BuildVerbose
                    );
            }
            else if (ProgramArgs.ContainsKey("-cleansdk"))
            {
                if (!Build.BuildSolution("ProcessHacker.sln",
                    BuildFlags.Build32bit | BuildFlags.Build64bit |
                    BuildFlags.BuildVerbose | BuildFlags.BuildApi
                    ))
                {
                    return;
                }

                Build.BuildSdk(
                    BuildFlags.Build32bit |
                    BuildFlags.Build64bit |
                    BuildFlags.BuildVerbose
                    );

                Build.ShowBuildStats();
            }
            else if (ProgramArgs.ContainsKey("-bin"))
            {
                Build.SetupBuildEnvironment(false);

                if (!Build.BuildSolution("ProcessHacker.sln",
                    BuildFlags.Build32bit | BuildFlags.Build64bit |
                    BuildFlags.BuildVerbose | BuildFlags.BuildApi
                    ))
                    return;

                if (!Build.CopyKProcessHacker(BuildFlags.Build32bit | BuildFlags.Build64bit | BuildFlags.BuildVerbose))
                    return;
                if (!Build.BuildSdk(BuildFlags.Build32bit | BuildFlags.Build64bit | BuildFlags.BuildVerbose))
                    return;

                if (!Build.BuildSolution("plugins\\Plugins.sln",
                    BuildFlags.Build32bit | BuildFlags.Build64bit |
                    BuildFlags.BuildVerbose | BuildFlags.BuildApi
                    ))
                    return;

                if (!Build.CopyTextFiles())
                    return;
                if (!Build.CopyWow64Files(BuildFlags.None))
                    return;
                if (!Build.CopySidCapsFile(BuildFlags.Build32bit | BuildFlags.Build64bit | BuildFlags.BuildVerbose))
                    return;
                if (!Build.CopyEtwTraceGuidsFile(BuildFlags.Build32bit | BuildFlags.Build64bit | BuildFlags.BuildVerbose))
                    return;

                if (!Build.BuildBinZip())
                    return;

                Build.ShowBuildStats();
            }
            else if (ProgramArgs.ContainsKey("-debug"))
            {
                Build.SetupBuildEnvironment(true);

                if (!Build.BuildSolution("ProcessHacker.sln",
                    BuildFlags.Build32bit | BuildFlags.Build64bit |
                    BuildFlags.BuildDebug | BuildFlags.BuildVerbose |
                    BuildFlags.BuildApi
                    ))
                    return;

                if (!Build.CopyKProcessHacker(
                    BuildFlags.Build32bit | BuildFlags.Build64bit |
                    BuildFlags.BuildDebug | BuildFlags.BuildVerbose))
                    return;

                if (!Build.BuildSdk(
                    BuildFlags.Build32bit | BuildFlags.Build64bit |
                    BuildFlags.BuildDebug | BuildFlags.BuildVerbose
                    ))
                {
                    return;
                }

                if (!Build.BuildSolution("plugins\\Plugins.sln",
                    BuildFlags.Build32bit | BuildFlags.Build64bit |
                    BuildFlags.BuildDebug | BuildFlags.BuildVerbose |
                    BuildFlags.BuildApi
                    ))
                {
                    return;
                }

                if (!Build.CopyWow64Files(BuildFlags.BuildDebug))
                    return;
                if (!Build.CopySidCapsFile(
                    BuildFlags.Build32bit | BuildFlags.Build64bit |
                    BuildFlags.BuildDebug | BuildFlags.BuildVerbose))
                    return;
                if (!Build.CopyEtwTraceGuidsFile(
                    BuildFlags.Build32bit | BuildFlags.Build64bit |
                    BuildFlags.BuildDebug | BuildFlags.BuildVerbose))
                    return;

                Build.ShowBuildStats();
            }
            else if (ProgramArgs.ContainsKey("-appveyor"))
            {
                Build.SetupBuildEnvironment(true);

                if (!Build.BuildSolution("ProcessHacker.sln",
                    BuildFlags.Build32bit | BuildFlags.Build64bit |
                    BuildFlags.BuildVerbose | BuildFlags.BuildApi
                    ))
                    Environment.Exit(1);

                if (!Build.CopyKProcessHacker(BuildFlags.Build32bit | BuildFlags.Build64bit | BuildFlags.BuildVerbose))
                    Environment.Exit(1);

                if (!Build.BuildSdk(BuildFlags.Build32bit | BuildFlags.Build64bit | BuildFlags.BuildVerbose))
                    Environment.Exit(1);

                if (!Build.BuildSolution("plugins\\Plugins.sln",
                    BuildFlags.Build32bit | BuildFlags.Build64bit |
                    BuildFlags.BuildVerbose | BuildFlags.BuildApi
                    ))
                    Environment.Exit(1);

                if (!Build.CopyTextFiles())
                    Environment.Exit(1);
                if (!Build.CopyWow64Files(BuildFlags.None))
                    Environment.Exit(1);
                if (!Build.CopySidCapsFile(BuildFlags.Build32bit | BuildFlags.Build64bit | BuildFlags.BuildVerbose))
                    Environment.Exit(1);
                if (!Build.CopyEtwTraceGuidsFile(BuildFlags.Build32bit | BuildFlags.Build64bit | BuildFlags.BuildVerbose))
                    Environment.Exit(1);

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

                if (!Build.BuildSolution("ProcessHacker.sln",
                    BuildFlags.Build32bit | BuildFlags.Build64bit |
                    BuildFlags.BuildVerbose | BuildFlags.BuildApi
                    ))
                    return;

                if (!Build.CopyKProcessHacker(BuildFlags.Build32bit | BuildFlags.Build64bit | BuildFlags.BuildVerbose))
                    return;
                if (!Build.BuildSdk(BuildFlags.Build32bit | BuildFlags.Build64bit | BuildFlags.BuildVerbose))
                    return;

                if (!Build.BuildSolution("plugins\\Plugins.sln",
                    BuildFlags.Build32bit | BuildFlags.Build64bit |
                    BuildFlags.BuildVerbose | BuildFlags.BuildApi
                    ))
                    return;

                if (!Build.CopyTextFiles())
                    return;
                if (!Build.CopyWow64Files(BuildFlags.None))
                    return;
                if (!Build.CopySidCapsFile(BuildFlags.Build32bit | BuildFlags.Build64bit | BuildFlags.BuildVerbose))
                    return;
                if (!Build.CopyEtwTraceGuidsFile(BuildFlags.Build32bit | BuildFlags.Build64bit | BuildFlags.BuildVerbose))
                    return;

                if (!Build.BuildBinZip())
                    return;
                if (!Build.BuildSetupExe())
                    return;
                Build.BuildPdbZip();
                Build.BuildSdkZip();
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
        BuildVerbose = 16,
        BuildApi = 32,
    }
}
