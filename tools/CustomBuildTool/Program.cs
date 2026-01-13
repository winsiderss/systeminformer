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
        private static readonly Dictionary<string, string> ProgramArgsHelp = new(StringComparer.OrdinalIgnoreCase)
        {
            { "-argsfile", "Read arguments from a file instead of the command line." },
            { "-bin", "Builds the binary package." },
            { "-cleanup", "Cleans up the build environment." },
            { "-cleansdk", "Cleans SDK build artifacts (internal)." },
            { "-debug", "Builds the debug configuration." },
            { "-decrypt", "Decrypts a file." },
            { "-devenv-build", "Runs devenv build." },
            { "-dyndata", "Builds dynamic data." },
            { "-encrypt", "Encrypts a file." },
            { "-help", "Shows this help message." },
            { "-msix-build", "Builds MSIX store package." },
            { "-phapppub_gen", "Generates public header files." },
            { "-phnt_headers_gen", "Builds single native header." },
            { "-pipeline-build", "Performs pipeline build operations." },
            { "-pipeline-deploy", "Deploys pipeline artifacts." },
            { "-pipeline-package", "Packages pipeline artifacts." },
            { "-azsign", "Creates signature files for build." },
            { "-kphsign", "Creates signature files for build." },
            { "-reflow", "Exports the current export definitions." },
            { "-reflowrevert", "Revert export definitions to previous state." },
            { "-reflowvalid", "Validates the current export definitions." },
            { "-sdk", "Builds the SDK package." },
            { "-verbose", "Enables verbose output." },
            { "-vtscan", "Uploads a file to VirusTotal for scanning." },
            { "-write-tools-id", "Writes the tools id file (internal)." },
        };

        public static void Main(string[] args)
        {
            if (!Build.InitializeBuildEnvironment())
                return;

            ProgramArgs = Utils.ParseArguments(args);

            // If -argsfile is specified, merge arguments from file
            if (ProgramArgs.TryGetValue("-argsfile", out string argsFilePath))
            {
                Utils.ParseArgumentsFromFile(ProgramArgs, argsFilePath);
            }

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
            else if (ProgramArgs.ContainsKey("-help") || ProgramArgs.ContainsKey("-h"))
            {
                Console.WriteLine("CustomBuildTool usage:\n");

                foreach (var key in ProgramArgsHelp.Keys.OrderBy(k => k, StringComparer.OrdinalIgnoreCase))
                {
                    Console.WriteLine($"  {key,-15} {ProgramArgsHelp[key]}");
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
            else if (ProgramArgs.ContainsKey("-phnt_headers_gen"))
            {
                if (!Build.BuildSingleNativeHeader())
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
            }
            else if (ProgramArgs.ContainsKey("-decrypt"))
            {
                var vargs = Utils.ParseArgumentsFromFile(ProgramArgs["-config"]);

                if (!BuildVerify.DecryptFile(vargs["-input"], vargs["-output"], vargs["-secret"], vargs["-salt"], vargs["-Iterations"]))
                    Environment.Exit(1);
            }
            else if (ProgramArgs.ContainsKey("-encrypt"))
            {
                var vargs = Utils.ParseArgumentsFromFile(ProgramArgs["-config"]);

                if (!BuildVerify.EncryptFile(vargs["-input"], vargs["-output"], vargs["-secret"], vargs["-salt"], vargs["-Iterations"]))
                    Environment.Exit(1);
            }
            else if (ProgramArgs.ContainsKey("-reflow"))
            {
                Build.ExportDefinitions(true);
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
            }
            else if (ProgramArgs.ContainsKey("-reflowrevert"))
            {
                Build.ExportDefinitionsRevert();
            }
            else if (ProgramArgs.ContainsKey("-vtscan"))
            {
                BuildVirusTotal.UploadScanFile(ProgramArgs["-file"]);
            }
            else if (ProgramArgs.ContainsKey("-cmake-build"))
            {
                BuildFlags flags = BuildFlags.Release;

                if (ProgramArgs.ContainsKey("-verbose"))
                {
                    flags |= BuildFlags.BuildVerbose;
                }

                Build.SetupBuildEnvironment(true);

                string generator = ProgramArgs["-generator"];
                string config = ProgramArgs["-config"];
                string toolchain = ProgramArgs["-toolchain"];

                if (string.IsNullOrWhiteSpace(generator))
                {
                    generator = "Ninja";
                }

                if (string.IsNullOrWhiteSpace(config))
                {
                    generator = "CONFIG";
                }

                if (string.IsNullOrWhiteSpace(toolchain))
                {
                    generator = "msvc-amd64";
                }

                // Build main project
                if (!Build.BuildSolutionCMake("SystemInformer", generator, config, toolchain, flags))
                    Environment.Exit(1);

                // Build plugins
                if (!Build.BuildSolutionCMake("Plugins", generator, config, toolchain, flags))
                    Environment.Exit(1);

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
                //if (!Build.BuildSymStoreZip(flags))
                //    Environment.Exit(1);
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
                //if (!Build.BuildSymStoreZip(flags))
                //    Environment.Exit(1);

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
            else if (ProgramArgs.ContainsKey("-release"))
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
                //if (!Build.BuildSymStoreZip(flags))
                //    Environment.Exit(1);
                if (!Build.BuildChecksumsFile())
                    Environment.Exit(1);

                Build.ShowBuildStats();
            }
            else
            {
                Build.SetupBuildEnvironment(true);

                Program.PrintColorMessage("Error: Missing required arguments. Valid commands:\r\n", ConsoleColor.Red,  true);

                foreach (var item in Program.ProgramArgsHelp)
                {
                    Console.WriteLine($"  {item.Key,-25}{item.Value}");
                }
            }
        }

        /// <summary>
        /// Prints a message to the console in the specified color, with optional newline and verbosity control.
        /// </summary>
        /// <param name="Message">The message to print.</param>
        /// <param name="Color">The <see cref="ConsoleColor"/> to use for the message text.</param>
        /// <param name="Newline">
        /// If <c>true</c>, appends a newline after the message; otherwise, prints without a newline. Default is <c>true</c>.
        /// </param>
        /// <param name="Flags">
        /// The <see cref="BuildFlags"/> controlling verbosity. The message is printed only if <see cref="BuildFlags.BuildVerbose"/> is set. Default is <see cref="BuildFlags.BuildVerbose"/>.
        /// </param>
        public static void PrintColorMessage(string Message, ConsoleColor Color, bool Newline = true, BuildFlags Flags = BuildFlags.BuildVerbose)
        {
            if ((Flags & BuildFlags.BuildVerbose) == 0)
                return;

            if (Build.BuildIntegration)
            {
                var colour_ansi = ToAnsiCode(Color);

                if (Message.Contains('\n', StringComparison.OrdinalIgnoreCase))
                {
                    var text = new StringBuilder(Message.Length + 16);
                    int start = 0;

                    for (int i = 0; i < Message.Length; i++)
                    {
                        if (Message[i] == '\n')
                        {
                            text.Append(colour_ansi);
                            text.Append(Message, start, i - start + 1);
                            start = i + 1;
                        }
                    }

                    if (start < Message.Length)
                        text.Append(colour_ansi).Append(Message, start, Message.Length - start);

                    text.Append("\e[0m");

                    if (Newline)
                        Console.WriteLine(text.ToString());
                    else
                        Console.Write(text.ToString());
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

        /// <summary>
        /// Prints a colorized message to the console, supporting both ANSI color codes and native console coloring,
        /// with optional newline and verbosity control.
        /// </summary>
        /// <param name="builder">The <see cref="LogInterpolatedStringHandler"/> used to build the formatted message text.</param>
        /// <param name="Color">The <see cref="ConsoleColor"/> to use for the message output.</param>
        /// <param name="Newline">If <c>true</c>, appends a newline after the message; otherwise, writes without a newline. Default is <c>true</c>.</param>
        /// <param name="Flags">The <see cref="BuildFlags"/> controlling verbosity and output behavior. Default is <see cref="BuildFlags.BuildVerbose"/>.</param>
        public static void PrintColorMessage(LogInterpolatedStringHandler builder, ConsoleColor Color, bool Newline = true, BuildFlags Flags = BuildFlags.BuildVerbose)
        {
            if ((Flags & BuildFlags.BuildVerbose) == 0)
                return;

            var formattedText = builder.GetFormattedText();

            if (Build.BuildIntegration)
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

        public static string CreateConsoleHyperlink(string Uri, string Text)
        {
            return $"\x1B]8;;{Uri}\x1B\\{Text}\x1B]8;;\x1B\\";
        }
    }
}
