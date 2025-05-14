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

            if (ProgramArgs.ContainsKey("-write-tools-id"))
            {
                WriteToolsId();
                return;
            }

            CheckForOutOfDateTools();

            if (ProgramArgs.ContainsKey("-cleanup"))
            {
                Build.CleanupBuildEnvironment();
                Build.ShowBuildStats();
            }
            else if (ProgramArgs.TryGetValue("-azsign", out string Path))
            {
                if (!EntraKeyVault.SignFiles(Path))
                    Environment.Exit(1);
            }
            else if (ProgramArgs.ContainsKey("-cleansdk"))
            {
                BuildFlags flags =
                    BuildFlags.Build32bit | BuildFlags.Build64bit | BuildFlags.BuildArm64bit |
                    BuildFlags.BuildVerbose | BuildFlags.BuildApi;

                if (!Build.BuildSolution("SystemInformer.sln", flags))
                    Environment.Exit(1);

                Build.ShowBuildStats();
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
            else if (ProgramArgs.TryGetValue("-kphsign", out string SignArg))
            {
                if (!Verify.CreateSigFile("kph", SignArg, Build.BuildCanary))
                    Environment.Exit(1);
            }
            else if (ProgramArgs.ContainsKey("-decrypt"))
            {
                if (!Verify.DecryptFile(ProgramArgs["-input"], ProgramArgs["-output"], ProgramArgs["-secret"], ProgramArgs["-salt"]))
                {
                    Environment.Exit(1);
                }
            }
            else if (ProgramArgs.ContainsKey("-encrypt"))
            {
                if (!Verify.EncryptFile(ProgramArgs["-input"], ProgramArgs["-output"], ProgramArgs["-secret"], ProgramArgs["-salt"]))
                {
                    Environment.Exit(1);
                }
            }
            else if (ProgramArgs.ContainsKey("-reflow"))
            {
                Build.ExportDefinitions(true);
            }
            else if (ProgramArgs.ContainsKey("-reflowvalid"))
            {
                if (!Build.BuildValidateExportDefinitions(BuildFlags.Release))
                    Environment.Exit(1);
            }
            else if (ProgramArgs.ContainsKey("-reflowrevert"))
            {
                Build.ExportDefinitionsRevert();
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
                if (!Build.CopyTextFiles(BuildFlags.Release))
                    Environment.Exit(1);
                if (!Build.BuildBinZip())
                    Environment.Exit(1);

                Build.ShowBuildStats();
            }
            else if (ProgramArgs.ContainsKey("-pipeline-build"))
            {
                BuildFlags flags = BuildFlags.Release;

                Build.WriteTimeStampFile();
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
                if (!Build.CopyTextFiles(BuildFlags.Release))
                    Environment.Exit(1);
                if (!Build.BuildBinZip())
                    Environment.Exit(1);

                foreach (var (channel, _) in BuildConfig.Build_Channels)
                {
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
                if (!Build.BuildUpdateServerConfig())
                    Environment.Exit(1);
            }
            else if (ProgramArgs.ContainsKey("-msix-build"))
            {
                BuildFlags flags = BuildFlags.Release | BuildFlags.BuildMsix;

                Build.WriteTimeStampFile();
                Build.SetupBuildEnvironment(true);
                Build.CopySourceLink(true);

                if (!Build.BuildSolution("SystemInformer.sln", flags))
                    Environment.Exit(1);
                if (!Build.BuildSolution("plugins\\Plugins.sln", flags))
                    Environment.Exit(1);

                if (!Build.CopyDebugEngineFiles(flags))
                    Environment.Exit(1);
                if (!Build.CopyWow64Files(flags)) // required after plugin build (dmex)
                    Environment.Exit(1);
                if (!Build.CopyTextFiles(flags))
                    Environment.Exit(1);

                Build.BuildStorePackage(flags);
                Build.BuildPdbZip(true);
            }
            else if (ProgramArgs.ContainsKey("-sdk"))
            {
                BuildFlags flags = BuildFlags.None;

                Build.SetupBuildEnvironment(false);

                if (ProgramArgs.ContainsKey("-verbose"))
                    flags |= BuildFlags.BuildVerbose;

                if (ProgramArgs.ContainsKey("-Debug"))
                {
                    flags |= BuildFlags.BuildDebug;

                    if (ProgramArgs.ContainsKey("-Win32"))
                        flags |= BuildFlags.Build32bit;
                    else if (ProgramArgs.ContainsKey("-x64"))
                        flags |= BuildFlags.Build64bit;
                    else if (ProgramArgs.ContainsKey("-arm64"))
                        flags |= BuildFlags.BuildArm64bit;
                    else
                    {
                        Environment.Exit(1);
                    }
                }
                else if (ProgramArgs.ContainsKey("-release"))
                {
                    flags |= BuildFlags.BuildRelease;

                    if (ProgramArgs.ContainsKey("-Win32"))
                        flags |= BuildFlags.Build32bit;
                    else if (ProgramArgs.ContainsKey("-x64"))
                        flags |= BuildFlags.Build64bit;
                    else if (ProgramArgs.ContainsKey("-arm64"))
                        flags |= BuildFlags.BuildArm64bit;
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
            else if (ProgramArgs.ContainsKey("-debug"))
            {
                Build.SetupBuildEnvironment(true);

                if (!Build.BuildSolution("SystemInformer.sln", BuildFlags.Debug))
                    Environment.Exit(1);
                if (!Build.BuildSolution("plugins\\Plugins.sln", BuildFlags.Debug))
                    Environment.Exit(1);

                if (!Build.CopyDebugEngineFiles(BuildFlags.Debug))
                    Environment.Exit(1);

                Build.ShowBuildStats();
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
                if (!Build.CopyTextFiles(BuildFlags.Release))
                    Environment.Exit(1);
                if (!Build.BuildBinZip())
                    Environment.Exit(1);

                foreach (var (channel, _) in BuildConfig.Build_Channels)
                {
                    if (!Build.BuildSetupExe(channel))
                        Environment.Exit(1);
                }

                Build.BuildPdbZip(false);
                //Build.BuildSdkZip();
                //Build.BuildSrcZip();
                Build.BuildChecksumsFile();
                Build.ShowBuildStats();
            }
        }

        private static bool HandleSingleArgumentCommands()
        {
            if (ProgramArgs.ContainsKey("-write-tools-id"))
            {
                WriteToolsId();
                return true;
            }

            if (ProgramArgs.ContainsKey("-cleanup"))
            {
                Build.CleanupBuildEnvironment();
                Build.ShowBuildStats();
                return true;
            }

            if (ProgramArgs.ContainsKey("-encrypt") && !Verify.EncryptFile(ProgramArgs["-input"], ProgramArgs["-output"], ProgramArgs["-secret"], ProgramArgs["-salt"]))
                Environment.Exit(1);

            if (ProgramArgs.ContainsKey("-decrypt") && !Verify.DecryptFile(ProgramArgs["-input"], ProgramArgs["-output"], ProgramArgs["-secret"], ProgramArgs["-salt"]))
                Environment.Exit(1);

            if (ProgramArgs.TryGetValue("-dyndata", out string ArgDynData) && !Build.BuildDynamicData(ArgDynData))
                Environment.Exit(1);

            if (ProgramArgs.ContainsKey("-phapppub_gen") && !Build.BuildPublicHeaderFiles())
                Environment.Exit(1);

            if (ProgramArgs.TryGetValue("-azsign", out string Path) && !EntraKeyVault.SignFiles(Path))
                Environment.Exit(1);

            if (ProgramArgs.TryGetValue("-kphsign", out string SignArg) && !Verify.CreateSigFile("kph", SignArg, Build.BuildCanary))
                Environment.Exit(1);

            return false;
        }

        private static bool HandleBuildCommands()
        {
            if (ProgramArgs.ContainsKey("-sdk"))
            {
                BuildFlags flags = BuildFlags.None;
                Build.SetupBuildEnvironment(false);

                if (ProgramArgs.ContainsKey("-verbose"))
                    flags |= BuildFlags.BuildVerbose;

                if (!SetBuildFlags(ref flags))
                    Environment.Exit(1);

                ExecuteBuildSteps(flags, Build.CopyResourceFiles, Build.BuildSdk, Build.CopyKernelDriver, Build.CopyWow64Files);
                return true;
            }

            if (ProgramArgs.ContainsKey("-cleansdk"))
            {
                BuildFlags flags = BuildFlags.Build32bit | BuildFlags.Build64bit | BuildFlags.BuildArm64bit | BuildFlags.BuildVerbose | BuildFlags.BuildApi;

                if (!Build.BuildSolution("SystemInformer.sln", flags))
                    return true;

                Build.ShowBuildStats();
                return true;
            }

            if (ProgramArgs.ContainsKey("-bin"))
            {
                Build.SetupBuildEnvironment(false);

                if (!Build.BuildSolution("SystemInformer.sln", BuildFlags.Release) ||
                    !Build.BuildSolution("plugins\\Plugins.sln", BuildFlags.Release) ||
                    !Build.CopyDebugEngineFiles(BuildFlags.Release) ||
                    !Build.CopyTextFiles(BuildFlags.Release) ||
                    !Build.BuildBinZip())
                    Environment.Exit(1);

                Build.ShowBuildStats();
                return true;
            }

            if (ProgramArgs.ContainsKey("-debug"))
            {
                Build.SetupBuildEnvironment(true);

                if (!Build.BuildSolution("SystemInformer.sln", BuildFlags.Debug) ||
                    !Build.BuildSolution("plugins\\Plugins.sln", BuildFlags.Debug) ||
                    !Build.CopyDebugEngineFiles(BuildFlags.Debug))
                    Environment.Exit(1);

                Build.ShowBuildStats();
                return true;
            }

            if (ProgramArgs.ContainsKey("-pipeline-build"))
            {
                BuildPipeline(BuildFlags.Release);
                return true;
            }

            if (ProgramArgs.ContainsKey("-pipeline-package"))
            {
                Build.SetupBuildEnvironment(true);

                if (!Build.ResignFiles() ||
                    !Build.CopyDebugEngineFiles(BuildFlags.Release) ||
                    !Build.CopyTextFiles(BuildFlags.Release) ||
                    !Build.BuildBinZip())
                    Environment.Exit(1);

                foreach (var (channel, _) in BuildConfig.Build_Channels)
                {
                    if (!Build.BuildSetupExe(channel))
                        Environment.Exit(1);
                }

                return true;
            }

            if (ProgramArgs.ContainsKey("-pipeline-deploy"))
            {
                Build.SetupBuildEnvironment(true);

                if (!Build.BuildPdbZip(false) || !Build.BuildUpdateServerConfig())
                    Environment.Exit(1);

                return true;
            }

            if (ProgramArgs.ContainsKey("-msix-build"))
            {
                BuildPipeline(BuildFlags.Release | BuildFlags.BuildMsix);
                return true;
            }

            return false;
        }

        private static void HandleDefaultBuild()
        {
            Build.SetupBuildEnvironment(true);

            if (
                !Build.BuildSolution("SystemInformer.sln", BuildFlags.Release) ||
                !Build.BuildSolution("plugins\\Plugins.sln", BuildFlags.Release)
                )
            {
                return;
            }

            ExecuteBuildSteps(BuildFlags.Release, Build.CopyDebugEngineFiles, Build.CopyWow64Files, Build.CopyTextFiles);

            if (!!Build.BuildBinZip())
                Environment.Exit(1);

            foreach (var (channel, _) in BuildConfig.Build_Channels)
            {
                if (!Build.BuildSetupExe(channel))
                    Environment.Exit(1);
            }

            Build.BuildPdbZip(false);
            Build.BuildChecksumsFile();
            Build.ShowBuildStats();
        }

        private static void BuildPipeline(BuildFlags flags)
        {
            Build.WriteTimeStampFile();
            Build.SetupBuildEnvironment(true);
            Build.CopySourceLink(true);

            if (
                !Build.BuildSolution("SystemInformer.sln", flags) ||
                !Build.BuildSolution("plugins\\Plugins.sln", flags)
                )
            {
                return;
            }

            ExecuteBuildSteps(flags, Build.CopyWow64Files, Build.CopyDebugEngineFiles, Build.CopyTextFiles);

            Build.BuildStorePackage(flags);
            Build.BuildPdbZip(true);
        }

        private static bool SetBuildFlags(ref BuildFlags flags)
        {
            if (ProgramArgs.ContainsKey("-Debug"))
            {
                flags |= BuildFlags.BuildDebug;
                return SetArchitectureFlags(ref flags);
            }

            if (ProgramArgs.ContainsKey("-release"))
            {
                flags |= BuildFlags.BuildRelease;
                return SetArchitectureFlags(ref flags);
            }

            return false;
        }

        private static bool SetArchitectureFlags(ref BuildFlags flags)
        {
            if (ProgramArgs.ContainsKey("-Win32"))
                flags |= BuildFlags.Build32bit;
            else if (ProgramArgs.ContainsKey("-x64"))
                flags |= BuildFlags.Build64bit;
            else if (ProgramArgs.ContainsKey("-arm64"))
                flags |= BuildFlags.BuildArm64bit;
            else
                return false;

            return true;
        }

        private static void ExecuteBuildSteps(BuildFlags flags, params Func<BuildFlags, bool>[] steps)
        {
            foreach (var step in steps)
            {
                if (!step(flags))
                    Environment.Exit(1);
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

        private static void CheckForOutOfDateTools()
        {
#if RELEASE
            string currentId = GetToolsId();
            string previousId = string.Empty;

            if (File.Exists("tools\\CustomBuildTool\\bin\\Release\\ToolsId.txt"))
            {
                previousId = Utils.ReadAllText("tools\\CustomBuildTool\\bin\\Release\\ToolsId.txt");
            }

            if (string.IsNullOrWhiteSpace(previousId) || !previousId.Equals(currentId, StringComparison.OrdinalIgnoreCase))
            {
                PrintColorMessage($"[WARNING] Build tools are out of date!", ConsoleColor.Yellow);
            }
#endif
        }

        private static void WriteToolsId()
        {
            string currentHash = GetToolsId();
            File.WriteAllText("tools\\CustomBuildTool\\bin\\Release\\ToolsId.txt", currentHash);
            Program.PrintColorMessage("Tools Hash: ", ConsoleColor.Gray, false);
            Program.PrintColorMessage($"{currentHash}", ConsoleColor.Green);
        }

        private static string GetToolsId()
        {
            const int bufferSize = 0x1000;
            string[] directories =
            [
                "tools\\CustomBuildTool",
                "tools\\CustomBuildTool\\AzureSignTool",
            ];

            using (var sha256 = SHA256.Create())
            {
                byte[] buffer = ArrayPool<byte>.Shared.Rent(bufferSize);

                try
                {
                    foreach (var directory in directories)
                    {
                        foreach (string source in Directory.EnumerateFiles(directory, "*.cs", SearchOption.TopDirectoryOnly))
                        {
                            int bytesRead;

                            using (var filestream = File.OpenRead(source))
                            using (var bufferedStream = new BufferedStream(filestream, bufferSize))
                            {
                                while ((bytesRead = bufferedStream.Read(buffer, 0, buffer.Length)) > 0)
                                {
                                    sha256.TransformBlock(buffer, 0, bytesRead, null, 0);
                                }
                            }
                        }
                    }
                }
                finally
                {
                    ArrayPool<byte>.Shared.Return(buffer);
                }

                sha256.TransformFinalBlock(Array.Empty<byte>(), 0, 0);
                return sha256.Hash == null ? string.Empty : Convert.ToHexString(sha256.Hash);
            }
        }
    }
}
