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

            ProgramArgs = Utils.ParseArguments(args);

            if (ProgramArgs.ContainsKey("-write-tools-id"))
            {
                BuildToolsId.WriteToolsId();
                return;
            }

            BuildToolsId.CheckForOutOfDateTools();

            if (ProgramArgs.ContainsKey("-cleanup"))
            {
                Build.CleanupBuildEnvironment();
                Build.ShowBuildStats();
            }
            else if (ProgramArgs.TryGetValue("-azsign", out string Path))
            {
                if (!BuildAzure.SignFiles(Path))
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

                Build.ShowBuildStats();
            }
            else if (ProgramArgs.TryGetValue("-kphsign", out string keyName))
            {
                if (!BuildVerify.CreateSigFile("kph", keyName, Build.BuildCanary))
                {
                    Environment.Exit(1);
                }

                Build.ShowBuildStats();
            }
            else if (ProgramArgs.ContainsKey("-decrypt"))
            {
                if (!BuildVerify.DecryptFile(ProgramArgs["-input"], ProgramArgs["-output"], ProgramArgs["-secret"], ProgramArgs["-salt"]))
                {
                    Environment.Exit(1);
                }

                Build.ShowBuildStats();
            }
            else if (ProgramArgs.ContainsKey("-encrypt"))
            {
                if (!BuildVerify.EncryptFile(ProgramArgs["-input"], ProgramArgs["-output"], ProgramArgs["-secret"], ProgramArgs["-salt"]))
                {
                    Environment.Exit(1);
                }

                Build.ShowBuildStats();
            }
            else if (ProgramArgs.ContainsKey("-reflow"))
            {
                Build.ExportDefinitions(true);

                Build.ShowBuildStats();
            }
            else if (ProgramArgs.ContainsKey("-reflowvalid"))
            {
                BuildFlags flags = BuildFlags.Release;

                if (ProgramArgs.ContainsKey("-verbose"))
                {
                    flags |= BuildFlags.BuildVerbose;
                }

                if (!Build.BuildValidateExportDefinitions(flags))
                    Environment.Exit(1);

                Build.ShowBuildStats();
            }
            else if (ProgramArgs.ContainsKey("-reflowrevert"))
            {
                Build.ExportDefinitionsRevert();

                Build.ShowBuildStats();
            }
            else if (ProgramArgs.TryGetValue("-devenv-build", out string Command))
            {
                Utils.ExecuteDevEnvCommand(Command);

                Build.ShowBuildStats();
            }
            else if (ProgramArgs.ContainsKey("-bin"))
            {
                BuildFlags flags = BuildFlags.Release;

                if (ProgramArgs.ContainsKey("-verbose"))
                {
                    flags |= BuildFlags.BuildVerbose;
                }

                Build.SetupBuildEnvironment(false);

                if (!Build.BuildSolution("SystemInformer.sln", flags))
                    Environment.Exit(1);
                if (!Build.BuildSolution("plugins\\Plugins.sln", flags))
                    Environment.Exit(1);

                //if (!Build.CopyDebugEngineFiles(flags))
                //    Environment.Exit(1);
                if (!Build.CopyTextFiles(true, flags))
                    Environment.Exit(1);
                if (!Build.BuildBinZip(flags))
                    Environment.Exit(1);
                if (!Build.CopyTextFiles(false, flags))
                    Environment.Exit(1);

                Build.ShowBuildStats();
            }
            else if (ProgramArgs.ContainsKey("-pipeline-build"))
            {
                BuildFlags flags = BuildFlags.Release;

                if (ProgramArgs.ContainsKey("-verbose"))
                {
                    flags |= BuildFlags.BuildVerbose;
                }

                Build.SetupBuildEnvironment(true);
                Build.CopySourceLink(true);

                if (!Build.BuildSolution("SystemInformer.sln", flags))
                    Environment.Exit(1);
                if (!Build.BuildSolution("plugins\\Plugins.sln", flags))
                    Environment.Exit(1);

                Build.CopyWow64Files(flags); // required after plugin build (dmex)

                Build.ShowBuildStats();
            }
            else if (ProgramArgs.ContainsKey("-pipeline-package"))
            {
                BuildFlags flags = BuildFlags.Release;

                if (ProgramArgs.ContainsKey("-verbose"))
                {
                    flags |= BuildFlags.BuildVerbose;
                }

                Build.SetupBuildEnvironment(true);

                if (!Build.ResignFiles())
                    Environment.Exit(1);
                //if (!Build.CopyDebugEngineFiles(flags))
                //    Environment.Exit(1);
                if (!Build.CopyTextFiles(true, flags))
                    Environment.Exit(1);
                if (!Build.BuildBinZip(flags))
                    Environment.Exit(1);

                foreach (var (channel, _) in BuildConfig.Build_Channels)
                {
                    if (!Build.BuildSetupExe(channel, flags))
                        Environment.Exit(1);
                }

                if (!Build.CopyTextFiles(false, flags))
                    Environment.Exit(1);

                Build.ShowBuildStats();
            }
            else if (ProgramArgs.ContainsKey("-pipeline-deploy"))
            {
                BuildFlags flags = BuildFlags.Release;

                if (ProgramArgs.ContainsKey("-verbose"))
                {
                    flags |= BuildFlags.BuildVerbose;
                }

                Build.SetupBuildEnvironment(true);

                if (!Build.BuildPdbZip(false, flags))
                    Environment.Exit(1);
                if (!Build.BuildSymStoreZip(flags))
                    Environment.Exit(1);
                //if (!Build.BuildSdkZip())
                //    Environment.Exit(1);
                //if (!Build.BuildSrcZip())
                //    Environment.Exit(1);
                //if (!Build.BuildChecksumsFile())
                //    Environment.Exit(1);
                if (!Build.BuildUpdateServerConfig())
                    Environment.Exit(1);

                Build.ShowBuildStats();
            }
            else if (ProgramArgs.ContainsKey("-msix-build"))
            {
                BuildFlags flags = BuildFlags.Release | BuildFlags.BuildMsix;

                if (ProgramArgs.ContainsKey("-verbose"))
                {
                    flags |= BuildFlags.BuildVerbose;
                }

                Build.SetupBuildEnvironment(true);
                Build.CopySourceLink(true);

                if (!Build.BuildSolution("SystemInformer.sln", flags))
                    Environment.Exit(1);
                if (!Build.BuildSolution("plugins\\Plugins.sln", flags))
                    Environment.Exit(1);

                //if (!Build.CopyDebugEngineFiles(flags))
                //    Environment.Exit(1);
                if (!Build.CopyWow64Files(flags)) // required after plugin build (dmex)
                    Environment.Exit(1);
                if (!Build.CopyTextFiles(true, flags))
                    Environment.Exit(1);
                if (!Build.BuildStorePackage(flags))
                    Environment.Exit(1);
                if (!Build.CopyTextFiles(false, flags))
                    Environment.Exit(1);

                if (!Build.BuildPdbZip(true, flags))
                    Environment.Exit(1);
                if (!Build.BuildSymStoreZip(flags))
                    Environment.Exit(1);

                Build.ShowBuildStats();
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

                Build.ShowBuildStats();
            }
            else if (ProgramArgs.ContainsKey("-debug"))
            {
                BuildFlags flags = BuildFlags.Debug;

                if (ProgramArgs.ContainsKey("-verbose"))
                {
                    flags |= BuildFlags.BuildVerbose;
                }

                Build.SetupBuildEnvironment(true);

                if (!Build.BuildSolution("SystemInformer.sln", flags))
                    Environment.Exit(1);
                if (!Build.BuildSolution("plugins\\Plugins.sln", flags))
                    Environment.Exit(1);

                //if (!Build.CopyDebugEngineFiles(flags))
                //    Environment.Exit(1);

                Build.ShowBuildStats();
            }
            else
            {
                BuildFlags flags = BuildFlags.Release;

                if (ProgramArgs.ContainsKey("-verbose"))
                {
                    flags |= BuildFlags.BuildVerbose;
                }

                Build.SetupBuildEnvironment(true);

                if (!Build.BuildSolution("SystemInformer.sln", flags))
                    Environment.Exit(1);
                if (!Build.BuildSolution("plugins\\Plugins.sln", flags))
                    Environment.Exit(1);

                //if (!Build.CopyDebugEngineFiles(flags))
                //    Environment.Exit(1);
                if (!Build.CopyWow64Files(flags))
                    Environment.Exit(1);
                if (!Build.CopyTextFiles(true, flags))
                    Environment.Exit(1);
                if (!Build.BuildBinZip(flags))
                    Environment.Exit(1);

                foreach (var (channel, _) in BuildConfig.Build_Channels)
                {
                    if (!Build.BuildSetupExe(channel, flags))
                        Environment.Exit(1);
                }

                if (!Build.CopyTextFiles(false, flags))
                    Environment.Exit(1);

                if (!Build.BuildPdbZip(false, flags))
                    Environment.Exit(1);
                if (!Build.BuildSymStoreZip(flags))
                    Environment.Exit(1);
                if (!Build.BuildChecksumsFile())
                    Environment.Exit(1);

                Build.ShowBuildStats();
            }
        }

        public static void PrintColorMessage(string Message, ConsoleColor Color, bool Newline = true, BuildFlags Flags = BuildFlags.BuildVerbose)
        {
            if ((Flags & BuildFlags.BuildVerbose) == 0)
                return;

            if (Build.BuildIntegrationTF)
            {
                var colour_ansi = ToAnsiCode(Color);

                // Avoid repeated string allocations by using StringBuilder if multiple newlines are expected

                if (Message.Contains('\n', StringComparison.OrdinalIgnoreCase))
                {
                    var sb = new StringBuilder(Message.Length + 16);
                    int start = 0;

                    for (int i = 0; i < Message.Length; i++)
                    {
                        if (Message[i] == '\n')
                        {
                            sb.Append(colour_ansi);
                            sb.Append(Message, start, i - start + 1);
                            start = i + 1;
                        }
                    }

                    if (start < Message.Length)
                        sb.Append(colour_ansi).Append(Message, start, Message.Length - start);

                    sb.Append("\e[0m");

                    if (Newline)
                        Console.WriteLine(sb.ToString());
                    else
                        Console.Write(sb.ToString());
                }
                else
                {
                    var color_reset = $"{colour_ansi}{Message}\e[0m";
                    if (Newline)
                        Console.WriteLine(color_reset);
                    else
                        Console.Write(color_reset);
                }
            }
            else
            {
                Console.ForegroundColor = Color;
                if (Newline)
                    Console.WriteLine(Message);
                else
                    Console.Write(Message);
                Console.ResetColor();
            }
        }

        public static void PrintColorMessage(LogInterpolatedStringHandler builder, ConsoleColor Color, bool Newline = true, BuildFlags Flags = BuildFlags.BuildVerbose)
        {
            if ((Flags & BuildFlags.BuildVerbose) == 0)
                return;

            var formattedText = builder.GetFormattedText();

            if (Build.BuildIntegrationTF)
            {
                var colour_ansi = ToAnsiCode(Color);

                if (formattedText.Contains('\n'))
                {
                    var sb = new System.Text.StringBuilder(formattedText.Length + 16);
                    int start = 0;

                    for (int i = 0; i < formattedText.Length; i++)
                    {
                        if (formattedText[i] == '\n')
                        {
                            sb.Append(colour_ansi);
                            sb.Append(formattedText, start, i - start + 1);
                            start = i + 1;
                        }
                    }

                    if (start < formattedText.Length)
                        sb.Append(colour_ansi).Append(formattedText, start, formattedText.Length - start);

                    sb.Append("\e[0m");

                    if (Newline)
                        Console.WriteLine(sb.ToString());
                    else
                        Console.Write(sb.ToString());
                }
                else
                {
                    var color_reset = $"{colour_ansi}{formattedText}\e[0m";
                    if (Newline)
                        Console.WriteLine(color_reset);
                    else
                        Console.Write(color_reset);
                }
            }
            else
            {
                Console.ForegroundColor = Color;
                if (Newline)
                    Console.WriteLine(formattedText);
                else
                    Console.Write(formattedText);
                Console.ResetColor();
            }
        }

        public static string ToAnsiCode(ConsoleColor color)
        {
            return color switch
            {
                ConsoleColor.Black           => "\e[30m",
                ConsoleColor.DarkBlue        => "\e[34m",
                ConsoleColor.DarkGreen       => "\e[32m",
                ConsoleColor.DarkCyan        => "\e[36m",
                ConsoleColor.DarkRed         => "\e[31m",
                ConsoleColor.DarkMagenta     => "\e[35m",
                ConsoleColor.DarkYellow      => "\e[33m",
                ConsoleColor.Gray            => "\e[37m",
                ConsoleColor.DarkGray        => "\e[90m",
                ConsoleColor.Blue            => "\e[94m",
                ConsoleColor.Green           => "\e[92m",
                ConsoleColor.Cyan            => "\e[96m",
                ConsoleColor.Red             => "\e[91m",
                ConsoleColor.Magenta         => "\e[95m",
                ConsoleColor.Yellow          => "\e[93m",
                ConsoleColor.White           => "\e[97m",
                _                            => "\e[0m"
            };
        }
    }
}
