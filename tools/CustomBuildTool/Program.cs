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
    /// <summary>
    /// Provides the entry point and core command-line processing logic for the custom build tool application.
    /// </summary>
    public static class Program
    {
        /// <summary>
        /// Initializes the build environment and command-line arguments.
        /// </summary>
        /// <param name="args">An array of command-line arguments.</param>
        /// <returns>A task that represents the asynchronous operation. The task result contains the process exit code.</returns>
        public static async Task<int> Main(string[] args)
        {
            if (!await Build.InitializeBuildEnvironment())
                return 1;

            return await Program.InitializeCommandLine(args);
        }

        /// <summary>
        /// Initializes the command-line interface, configuring global options and subcommands.
        /// </summary>
        /// <remarks>Sets up global options such as verbose output and argument file support, registers
        /// all available subcommands, and defines the default handler for missing arguments.</remarks>
        /// <returns>A task that represents the asynchronous operation and contains the process exit code.</returns>
        private static Task<int> InitializeCommandLine(string[] args)
        {
            var rootCommand = new RootCommand("CustomBuildTool for System Informer.");

            // Global Options
            var verboseOption = new Option<bool>("-verbose", ["--verbose"])
            {
                Description = "Enables verbose output.",
                Recursive = true
            };
            rootCommand.Add(verboseOption);

            var argsFileOption = new Option<string>("-argsfile")
            {
                Description = "Read arguments from a file instead of the command line.",
                Recursive = true
            };
            rootCommand.Add(argsFileOption);

            // Subcommands
            rootCommand.Add(CreateWriteToolsIdCommand());
            rootCommand.Add(CreateCleanupCommand());
            rootCommand.Add(CreateAzSignCommand());
            rootCommand.Add(CreateCleanSdkCommand());
            rootCommand.Add(CreateCheckMsvcCommand());
            rootCommand.Add(CreateInstallMsvcCommand());
            rootCommand.Add(CreateDynDataCommand());
            rootCommand.Add(CreatePhAppPubGenCommand());
            rootCommand.Add(CreatePhntHeadersGenCommand());
            rootCommand.Add(CreateKphSignCommand());
            rootCommand.Add(CreateDecryptCommand());
            rootCommand.Add(CreateEncryptCommand());
            rootCommand.Add(CreateReflowCommand());
            rootCommand.Add(CreateReflowValidCommand(verboseOption));
            rootCommand.Add(CreateReflowRevertCommand());
            rootCommand.Add(CreateVtScanCommand());
            rootCommand.Add(CreateDevEnvBuildCommand());
            rootCommand.Add(CreateBinCommand(verboseOption));
            rootCommand.Add(CreatePipelineBuildCommand(verboseOption));
            rootCommand.Add(CreatePipelinePackageCommand(verboseOption));
            rootCommand.Add(CreatePipelineDeployCommand(verboseOption));
            rootCommand.Add(CreateMsixBuildCommand(verboseOption));
            rootCommand.Add(CreateSdkCommand(verboseOption));
            rootCommand.Add(CreateDebugCommand(verboseOption));
            rootCommand.Add(CreateReleaseCommand(verboseOption));
            rootCommand.Add(CreateCMakeBuildCommand(verboseOption));
            rootCommand.Add(CreateCMakeBinCommand(verboseOption));
            rootCommand.Add(CreateCMakeReleaseCommand(verboseOption));
            rootCommand.Add(CreateCMakePipelineBuildCommand(verboseOption));
            rootCommand.Add(CreateCMakePipelinePackageCommand(verboseOption));
            rootCommand.Add(CreateCMakePipelineDeployCommand(verboseOption));
            rootCommand.Add(CreateCheckThirdPartyCommand());

            // Default handler when no command is provided
            rootCommand.SetAction(parseResult =>
            {
                bool verbose = parseResult.GetValue(verboseOption);
                Dictionary<string, string> ProgramArgsHelp = new(StringComparer.OrdinalIgnoreCase)
                {
                    { "-argsfile", "Read arguments from a file instead of the command line." },
                    { "-bin", "Builds the binary package." },
                    { "-check_msvc", "Check required build dependencies are installed." },
                    { "-install_msvc", "Installs any missing build dependencies." },
                    { "-cleanup", "Cleans up the build environment." },
                    { "-cleansdk", "Cleans SDK build artifacts (internal)." },
                    { "-cmake-bin", "Builds the binary package using CMake (clang)." },
                    { "-cmake-pipeline-build", "Performs pipeline build operations using CMake (clang)." },
                    { "-cmake-pipeline-deploy", "Deploys pipeline artifacts using CMake (clang)." },
                    { "-cmake-pipeline-package", "Packages pipeline artifacts using CMake (clang)." },
                    { "-cmake-release", "Builds release configuration using CMake (clang)." },
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
                    { "-check-thirdparty", "Checks thirdparty library versions against latest GitHub releases." },
                };

                PrintColorMessage("Error: Missing required arguments. Use -h or --help for valid commands.\r\n", ConsoleColor.Red, true);

            });

            rootCommand.Add(new VersionOption());
            rootCommand.Add(new DiagramDirective());
            rootCommand.Add(new System.CommandLine.Completions.SuggestDirective());

            return rootCommand.Parse(args).InvokeAsync(new InvocationConfiguration
            {
                EnableDefaultExceptionHandler = true,
                ProcessTerminationTimeout = TimeSpan.FromSeconds(2)
            });
        }

        /// <summary>
        /// Creates a command that writes the tools ID file.
        /// </summary>
        /// <returns>A command configured to execute the tools ID file writing operation.</returns>
        private static Command CreateWriteToolsIdCommand()
        {
            var cmd = new Command("-write-tools-id", "Writes the tools id file (internal).");
            cmd.SetAction(_ => BuildToolsId.WriteToolsId());
            return cmd;
        }

        /// <summary>
        /// Creates a command that cleans the build environment and displays build statistics.
        /// </summary>
        /// <returns>A command configured to perform cleanup operations and show build statistics.</returns>
        private static Command CreateCleanupCommand()
        {
            var cmd = new Command("-cleanup", "Cleans up the build environment.");
            cmd.SetAction(_ =>
            {
                BuildToolsId.CheckForOutOfDateTools();
                Build.CleanupBuildEnvironment();
                Build.ShowBuildStats();
            });
            return cmd;
        }

        /// <summary>
        /// Creates a command that generates signature files for a specified build path.
        /// </summary>
        /// <returns>A Command object configured to sign files using Azure.</returns>
        private static Command CreateAzSignCommand()
        {
            var cmd = new Command("-azsign", "Creates signature files for build.");
            var pathArg = new Argument<string>("path")
            {
                Description = "Path to sign"
            };
            cmd.Add(pathArg);
            cmd.SetAction(async parseResult =>
            {
                string path = parseResult.GetValue(pathArg);
                BuildToolsId.CheckForOutOfDateTools();
                if (!await BuildAzure.SignFiles(path)) Environment.Exit(1);
            });
            return cmd;
        }

        /// <summary>
        /// Creates a command that cleans SDK build artifacts for all supported architectures.
        /// </summary>
        /// <returns>A command configured to clean SDK build artifacts.</returns>
        private static Command CreateCleanSdkCommand()
        {
            var cmd = new Command("-cleansdk", "Cleans SDK build artifacts (internal).");
            cmd.SetAction(_ =>
            {
                BuildToolsId.CheckForOutOfDateTools();
                BuildFlags flags = BuildFlags.Build32bit | BuildFlags.Build64bit | BuildFlags.BuildArm64bit | BuildFlags.BuildVerbose | BuildFlags.BuildApi;
                if (!Build.BuildSolution("SystemInformer.sln", flags)) Environment.Exit(1);
                Build.ShowBuildStats();
            });
            return cmd;
        }

        /// <summary>
        /// Creates a command that verifies the required MSVC build dependencies are installed.
        /// </summary>
        /// <returns>A command configured to check for MSVC build dependencies and display build statistics.</returns>
        private static Command CreateCheckMsvcCommand()
        {
            var cmd = new Command("-check_msvc", "Check required build dependencies are installed.");
            cmd.SetAction(_ =>
            {
                BuildToolsId.CheckForOutOfDateTools();
                if (!BuildVisualStudio.CheckBuildDependencies()) Environment.Exit(1);
                Build.ShowBuildStats();
            });
            return cmd;
        }

        /// <summary>
        /// Creates a command that installs any missing MSVC build dependencies.
        /// </summary>
        /// <returns>A command configured to handle the installation of missing MSVC build dependencies.</returns>
        private static Command CreateInstallMsvcCommand()
        {
            var cmd = new Command("-install_msvc", "Installs any missing build dependencies.");
            cmd.SetAction(async _ =>
            {
                BuildToolsId.CheckForOutOfDateTools();
                if (!await BuildVisualStudio.InstallBuildDependencies()) Environment.Exit(1);
                Build.ShowBuildStats();
            });
            return cmd;
        }

        /// <summary>
        /// Creates a command that processes dynamic data using a specified argument.
        /// </summary>
        /// <returns>A command configured to handle dynamic data operations.</returns>
        private static Command CreateDynDataCommand()
        {
            var cmd = new Command("-dyndata", "Builds dynamic data.");
            var arg = new Argument<string>("arg")
            {
                Description = "Dynamic data argument (optional)",
                DefaultValueFactory = _ => string.Empty
            };
            cmd.Add(arg);
            cmd.SetAction(parseResult =>
            {
                string a = parseResult.GetValue(arg);
                BuildToolsId.CheckForOutOfDateTools();

                if (!Build.BuildDynamicData(a))
                    Environment.Exit(1);
            });
            return cmd;
        }

        /// <summary>
        /// Creates a command that generates public header files.
        /// </summary>
        /// <returns>A Command object configured to generate public header files.</returns>
        private static Command CreatePhAppPubGenCommand()
        {
            var cmd = new Command("-phapppub_gen", "Generates public header files.");
            cmd.SetAction(_ =>
            {
                BuildToolsId.CheckForOutOfDateTools();
                if (!Build.BuildPublicHeaderFiles()) Environment.Exit(1);
                Build.ShowBuildStats();
            });
            return cmd;
        }

        /// <summary>
        /// Creates a command that generates a single native header file.
        /// </summary>
        /// <returns>A configured command for generating a single native header.</returns>
        private static Command CreatePhntHeadersGenCommand()
        {
            var cmd = new Command("-phnt_headers_gen", "Builds single native header.");
            cmd.SetAction(_ =>
            {
                BuildToolsId.CheckForOutOfDateTools();
                if (!Build.BuildSingleNativeHeader()) Environment.Exit(1);
                Build.ShowBuildStats();
            });
            return cmd;
        }

        /// <summary>
        /// Creates a command that generates signature files for the build process using a specified key name.
        /// </summary>
        /// <returns>A command configured to generate signature files with the provided key name.</returns>
        private static Command CreateKphSignCommand()
        {
            var cmd = new Command("-kphsign", "Creates signature files for build.");
            var arg = new Argument<string>("keyName")
            {
                Description = "Key name"
            };
            cmd.Add(arg);
            cmd.SetAction(parseResult =>
            {
                string k = parseResult.GetValue(arg);
                BuildToolsId.CheckForOutOfDateTools();
                if (!BuildVerify.CreateSigFile("kph", k, Build.BuildCanary)) Environment.Exit(1);
            });
            return cmd;
        }

        /// <summary>
        /// Creates a command that decrypts a file using parameters specified in a configuration file.
        /// </summary>
        /// <returns>A command configured to perform file decryption.</returns>
        private static Command CreateDecryptCommand()
        {
            var cmd = new Command("-decrypt", "Decrypts a file.");
            var configOpt = new Option<string>("-config")
            {
                Description = "Config file"
            };
            cmd.Add(configOpt);
            cmd.SetAction(parseResult =>
            {
                string config = parseResult.GetValue(configOpt);
                BuildToolsId.CheckForOutOfDateTools();
                var vargs = Utils.ParseArgumentsFromFile(config);
                if (!BuildVerify.DecryptFile(vargs["-input"], vargs["-output"], vargs["-secret"], vargs["-salt"], vargs["-Iterations"]))
                    Environment.Exit(1);
            });
            return cmd;
        }

        /// <summary>
        /// Creates a command that encrypts a file using options specified in a configuration file.
        /// </summary>
        /// <returns>A command configured to perform file encryption.</returns>
        private static Command CreateEncryptCommand()
        {
            var cmd = new Command("-encrypt", "Encrypts a file.");
            var configOpt = new Option<string>("-config")
            {
                Description = "Config file"
            };
            cmd.Add(configOpt);
            cmd.SetAction(parseResult =>
            {
                string config = parseResult.GetValue(configOpt);
                BuildToolsId.CheckForOutOfDateTools();
                var vargs = Utils.ParseArgumentsFromFile(config);
                if (!BuildVerify.EncryptFile(vargs["-input"], vargs["-output"], vargs["-secret"], vargs["-salt"], vargs["-Iterations"]))
                    Environment.Exit(1);
            });
            return cmd;
        }

        /// <summary>
        /// Creates a command that exports the current export definitions.
        /// </summary>
        /// <returns>A Command object configured to perform the export operation.</returns>
        private static Command CreateReflowCommand()
        {
            var cmd = new Command("-reflow", "Exports the current export definitions.");
            cmd.SetAction(_ =>
            {
                BuildToolsId.CheckForOutOfDateTools();
                Build.ExportDefinitions(true);
            });
            return cmd;
        }

        /// <summary>
        /// Creates a command that validates the current export definitions.
        /// </summary>
        /// <param name="verboseOption">Specifies whether to enable verbose output.</param>
        /// <returns>A command configured to validate export definitions.</returns>
        private static Command CreateReflowValidCommand(Option<bool> verboseOption)
        {
            var cmd = new Command("-reflowvalid", "Validates the current export definitions.");
            cmd.SetAction(parseResult =>
            {
                bool verbose = parseResult.GetValue(verboseOption);
                BuildToolsId.CheckForOutOfDateTools();
                BuildFlags flags = BuildFlags.Release | (verbose ? BuildFlags.BuildVerbose : BuildFlags.None);
                if (!Build.BuildValidateExportDefinitions(flags)) Environment.Exit(1);
            });
            return cmd;
        }

        /// <summary>
        /// Creates a command that reverts export definitions to their previous state.
        /// </summary>
        /// <returns>A command configured to execute the revert operation.</returns>
        private static Command CreateReflowRevertCommand()
        {
            var cmd = new Command("-reflowrevert", "Revert export definitions to previous state.");
            cmd.SetAction(_ =>
            {
                BuildToolsId.CheckForOutOfDateTools();
                Build.ExportDefinitionsRevert();
            });
            return cmd;
        }

        /// <summary>
        /// Creates a command that uploads a specified file to VirusTotal for scanning.
        /// </summary>
        /// <returns>A command configured to handle file uploads to VirusTotal.</returns>
        private static Command CreateVtScanCommand()
        {
            var cmd = new Command("-vtscan", "Uploads a file to VirusTotal for scanning.");
            var fileOpt = new Option<string>("-file")
            {
                Description = "File to scan"
            };
            cmd.Add(fileOpt);
            cmd.SetAction(async parseResult =>
            {
                string file = parseResult.GetValue(fileOpt);
                BuildToolsId.CheckForOutOfDateTools();
                await BuildVirusTotal.UploadScanFile(file);
            });
            return cmd;
        }

        /// <summary>
        /// Creates a command that executes the specified DevEnv build command.
        /// </summary>
        /// <returns>A Command object configured to run the DevEnv build process.</returns>
        private static Command CreateDevEnvBuildCommand()
        {
            var arg = new Argument<string>("command")
            {
                Description = "DevEnv command"
            };
            var cmd = new Command("-devenv-build", "Runs devenv build.");
            cmd.Add(arg);
            cmd.SetAction(parseResult =>
            {
                string c = parseResult.GetValue(arg);
                BuildToolsId.CheckForOutOfDateTools();
                Build.SetupBuildEnvironment(true);
                Utils.ExecuteDevEnvCommand([c]);
                Build.ShowBuildStats();
            });
            return cmd;
        }

        /// <summary>
        /// Creates a command that builds the binary package with optional verbose output.
        /// </summary>
        /// <param name="verboseOption">Specifies whether to enable verbose output during the build process.</param>
        /// <returns>A Command object configured to build the binary package.</returns>
        private static Command CreateBinCommand(Option<bool> verboseOption)
        {
            var cmd = new Command("-bin", "Builds the binary package.");
            cmd.SetAction(parseResult =>
            {
                bool verbose = parseResult.GetValue(verboseOption);
                BuildToolsId.CheckForOutOfDateTools();
                BuildFlags flags = BuildFlags.Release | (verbose ? BuildFlags.BuildVerbose : BuildFlags.None);
                Build.SetupBuildEnvironment(true);

                if (!Build.BuildSolution("SystemInformer.sln", flags))
                    Environment.Exit(1);
                if (!Build.BuildSolution("plugins\\Plugins.sln", flags))
                    Environment.Exit(1);

                if (!Build.CopyTextFiles(true, flags))
                    Environment.Exit(1);
                if (!Build.BuildBinZip(flags))
                    Environment.Exit(1);
                if (!Build.CopyTextFiles(false, flags))
                    Environment.Exit(1);

                Build.ShowBuildStats();
            });
            return cmd;
        }

        /// <summary>
        /// Creates a command that executes pipeline build operations, including environment setup, solution builds, and
        /// post-build tasks.
        /// </summary>
        /// <param name="verboseOption">Specifies whether to enable verbose output during the build process.</param>
        /// <returns>A command configured to perform pipeline build operations.</returns>
        private static Command CreatePipelineBuildCommand(Option<bool> verboseOption)
        {
            var cmd = new Command("-pipeline-build", "Performs pipeline build operations.");
            cmd.SetAction(parseResult =>
            {
                bool verbose = parseResult.GetValue(verboseOption);
                BuildToolsId.CheckForOutOfDateTools();
                Win32.SetLowIntegrityForProcesses();

                BuildFlags flags = BuildFlags.Release | (verbose ? BuildFlags.BuildVerbose : BuildFlags.None);

                Build.SetupBuildEnvironment(true);
                Build.CopySourceLink(true);

                if (!Build.BuildSolution("SystemInformer.sln", flags))
                    Environment.Exit(1);
                if (!Build.BuildSolution("plugins\\Plugins.sln", flags))
                    Environment.Exit(1);

                Build.CopyWow64Files(flags);
                Build.ShowBuildStats();
            });
            return cmd;
        }

        /// <summary>
        /// Creates a command that packages pipeline artifacts with optional verbose output.
        /// </summary>
        /// <param name="verboseOption">Enables verbose output during packaging when set to true.</param>
        /// <returns>A command configured to package pipeline artifacts.</returns>
        private static Command CreatePipelinePackageCommand(Option<bool> verboseOption)
        {
            var cmd = new Command("-pipeline-package", "Packages pipeline artifacts.");
            cmd.SetAction(parseResult =>
            {
                bool verbose = parseResult.GetValue(verboseOption);
                BuildFlags flags = BuildFlags.Release | (verbose ? BuildFlags.BuildVerbose : BuildFlags.None);
                BuildToolsId.CheckForOutOfDateTools();
                Build.SetupBuildEnvironment(true);

                if (!Build.ResignFiles("bin")) Environment.Exit(1);
                if (!Build.CopyTextFiles(true, flags)) Environment.Exit(1);
                if (!Build.BuildBinZip(flags)) Environment.Exit(1);

                foreach (var (channel, _) in BuildConfig.Build_Channels)
                {
                    if (!Build.BuildSetupExe(channel, flags))
                        Environment.Exit(1);
                }

                if (!Build.CopyTextFiles(false, flags)) Environment.Exit(1);
                if (!Build.BuildPdbZip(false, flags)) Environment.Exit(1);

                Build.ShowBuildStats();
            });
            return cmd;
        }

        /// <summary>
        /// Creates a command that deploys pipeline artifacts with optional verbose output.
        /// </summary>
        /// <param name="verboseOption">Specifies whether to enable verbose logging during deployment.</param>
        /// <returns>A command configured to deploy pipeline artifacts.</returns>
        private static Command CreatePipelineDeployCommand(Option<bool> verboseOption)
        {
            var cmd = new Command("-pipeline-deploy", "Deploys pipeline artifacts.");
            cmd.SetAction(async parseResult =>
            {
                bool verbose = parseResult.GetValue(verboseOption);
                BuildToolsId.CheckForOutOfDateTools();
                BuildFlags flags = BuildFlags.Release | (verbose ? BuildFlags.BuildVerbose : BuildFlags.None);
                Build.SetupBuildEnvironment(true);

                if (!await BuildDeploy.BuildUpdateServerConfig()) 
                    Environment.Exit(1);

                Build.ShowBuildStats();
            });
            return cmd;
        }

        /// <summary>
        /// Creates a command that builds an MSIX store package with optional verbose output.
        /// </summary>
        /// <param name="verboseOption">Indicates whether verbose output is enabled during the build process.</param>
        /// <returns>A command configured to execute the MSIX build process.</returns>
        private static Command CreateMsixBuildCommand(Option<bool> verboseOption)
        {
            var cmd = new Command("-msix-build", "Builds MSIX store package.");
            cmd.SetAction(parseResult =>
            {
                bool verbose = parseResult.GetValue(verboseOption);
                BuildToolsId.CheckForOutOfDateTools();
                BuildFlags flags = BuildFlags.Release | BuildFlags.BuildMsix | (verbose ? BuildFlags.BuildVerbose : BuildFlags.None);
                Build.SetupBuildEnvironment(true);
                Build.CopySourceLink(true);

                if (!Build.BuildSolution("SystemInformer.sln", flags)) Environment.Exit(1);
                if (!Build.BuildSolution("plugins\\Plugins.sln", flags)) Environment.Exit(1);
                if (!Build.CopyWow64Files(flags)) Environment.Exit(1);
                if (!Build.CopyTextFiles(true, flags)) Environment.Exit(1);
                if (!Build.BuildStorePackage(flags)) Environment.Exit(1);
                if (!Build.CopyTextFiles(false, flags)) Environment.Exit(1);
                if (!Build.BuildPdbZip(true, flags)) Environment.Exit(1);

                Build.ShowBuildStats();
            });
            return cmd;
        }

        /// <summary>
        /// Creates a command for building the SDK package with configurable build options.
        /// </summary>
        /// <param name="verboseOption">Specifies whether to enable verbose output during the build process.</param>
        /// <returns>A command configured to build the SDK package with the specified options.</returns>
        private static Command CreateSdkCommand(Option<bool> verboseOption)
        {
            var cmd = new Command("-sdk", "Builds the SDK package.");
            var cmakeOpt = new Option<bool>("-cmake") { Description = "Use CMake" };
            var debugOpt = new Option<bool>("-Debug", ["-debug"]) { Description = "Debug build" };
            var releaseOpt = new Option<bool>("-release", ["-Release"]) { Description = "Release build" };
            var win32Opt = new Option<bool>("-Win32", ["-win32", "-WIN32"]) { Description = "Win32" };
            var x64Opt = new Option<bool>("-x64", ["-X64"]) { Description = "x64" };
            var arm64Opt = new Option<bool>("-arm64", ["-Arm64", "-ARM64"]) { Description = "ARM64" };

            cmd.Add(cmakeOpt); cmd.Add(debugOpt); cmd.Add(releaseOpt);
            cmd.Add(win32Opt); cmd.Add(x64Opt); cmd.Add(arm64Opt);
            cmd.SetAction(parseResult =>
            {
                bool verbose = parseResult.GetValue(verboseOption);
                bool cmake = parseResult.GetValue(cmakeOpt);
                bool debug = parseResult.GetValue(debugOpt);
                bool release = parseResult.GetValue(releaseOpt);
                bool win32 = parseResult.GetValue(win32Opt);
                bool x64 = parseResult.GetValue(x64Opt);
                bool arm64 = parseResult.GetValue(arm64Opt);
                BuildToolsId.CheckForOutOfDateTools();
                BuildFlags flags = BuildFlags.None;
                Build.SetupBuildEnvironment(false);

                if (verbose) flags |= BuildFlags.BuildVerbose;
                if (cmake) flags |= BuildFlags.BuildCMake;

                if (debug)
                {
                    flags |= BuildFlags.BuildDebug;
                    if (win32) flags |= BuildFlags.Build32bit;
                    else if (x64) flags |= BuildFlags.Build64bit;
                    else if (arm64) flags |= BuildFlags.BuildArm64bit;
                    else Environment.Exit(1);
                }
                else if (release)
                {
                    flags |= BuildFlags.BuildRelease;
                    if (win32) flags |= BuildFlags.Build32bit;
                    else if (x64) flags |= BuildFlags.Build64bit;
                    else if (arm64) flags |= BuildFlags.BuildArm64bit;
                    else Environment.Exit(1);
                }
                else Environment.Exit(1);

                if (!Build.CopyResourceFiles(flags)) Environment.Exit(1);
                if (!Build.BuildSdk(flags)) Environment.Exit(1);
                if (!Build.CopyKernelDriver(flags)) Environment.Exit(1);
                if (!Build.CopyWow64Files(flags)) Environment.Exit(1);

                if (verbose) Build.ShowBuildStats();
            });
            return cmd;
        }

        /// <summary>
        /// Creates a command that builds the debug configuration.
        /// </summary>
        /// <param name="verboseOption">Specifies whether to enable verbose output.</param>
        /// <returns>A command configured to build the debug configuration.</returns>
        private static Command CreateDebugCommand(Option<bool> verboseOption)
        {
            var cmd = new Command("-debug", "Builds the debug configuration.");
            cmd.SetAction(parseResult =>
            {
                bool verbose = parseResult.GetValue(verboseOption);
                BuildToolsId.CheckForOutOfDateTools();
                BuildFlags flags = BuildFlags.Debug | (verbose ? BuildFlags.BuildVerbose : BuildFlags.None);
                Build.SetupBuildEnvironment(true);

                if (!Build.BuildSolution("SystemInformer.sln", flags)) Environment.Exit(1);
                if (!Build.BuildSolution("plugins\\Plugins.sln", flags)) Environment.Exit(1);

                Build.ShowBuildStats();
            });
            return cmd;
        }

        /// <summary>
        /// Creates a command that builds the release configuration.
        /// </summary>
        /// <param name="verboseOption">Specifies whether to enable verbose output.</param>
        /// <returns>A command configured to build the release configuration.</returns>
        private static Command CreateReleaseCommand(Option<bool> verboseOption)
        {
            var cmd = new Command("-release", "Builds the release configuration.");
            cmd.SetAction(parseResult =>
            {
                bool verbose = parseResult.GetValue(verboseOption);
                BuildToolsId.CheckForOutOfDateTools();
                BuildFlags flags = BuildFlags.Release | (verbose ? BuildFlags.BuildVerbose : BuildFlags.None);
                Build.SetupBuildEnvironment(true);

                if (!Build.BuildSolution("SystemInformer.sln", flags)) Environment.Exit(1);
                if (!Build.BuildSolution("plugins\\Plugins.sln", flags)) Environment.Exit(1);
                if (!Build.CopyWow64Files(flags)) Environment.Exit(1);
                if (!Build.CopyTextFiles(true, flags)) Environment.Exit(1);
                if (!Build.BuildBinZip(flags)) Environment.Exit(1);

                foreach (var (channel, _) in BuildConfig.Build_Channels)
                {
                    if (!Build.BuildSetupExe(channel, flags)) Environment.Exit(1);
                }

                if (!Build.CopyTextFiles(false, flags)) Environment.Exit(1);
                if (!Build.BuildPdbZip(false, flags)) Environment.Exit(1);
                if (!Build.BuildChecksumsFile()) Environment.Exit(1);

                Build.ShowBuildStats();
            });
            return cmd;
        }

        /// <summary>
        /// Creates a command that builds the project using CMake.
        /// </summary>
        /// <param name="verboseOption">Specifies whether to enable verbose output.</param>
        /// <returns>A command configured to perform a CMake build.</returns>
        private static Command CreateCMakeBuildCommand(Option<bool> verboseOption)
        {
            var cmd = new Command("-cmake-build", "Builds using CMake.");
            var genOpt = new Option<string>("-generator") { Description = "Generator" };
            var toolOpt = new Option<string>("-toolchain") { Description = "Toolchain" };
            var confOpt = new Option<string>("-config") { Description = "Config" };
            cmd.Add(genOpt); cmd.Add(toolOpt); cmd.Add(confOpt);

            cmd.SetAction(parseResult =>
            {
                bool verbose = parseResult.GetValue(verboseOption);
                string generatorArg = parseResult.GetValue(genOpt);
                string toolchainArg = parseResult.GetValue(toolOpt);
                string configArg = parseResult.GetValue(confOpt);

                BuildToolsId.CheckForOutOfDateTools();
                BuildFlags flags = BuildFlags.Release | (verbose ? BuildFlags.BuildVerbose : BuildFlags.None);
                Build.SetupBuildEnvironment(true);

                var generator = Utils.GetGeneratorFromString(generatorArg);
                var toolchain = Utils.GetToolchainFromString(toolchainArg);

                if (!Build.BuildSolutionCMake("SystemInformer", generator, toolchain, flags)) 
                    Environment.Exit(1);

                Build.ShowBuildStats();
            });
            return cmd;
        }

        /// <summary>
        /// Creates a command that builds the binary package using CMake (clang).
        /// </summary>
        /// <param name="verboseOption">Specifies whether to enable verbose output.</param>
        /// <returns>A command configured to build the binary package using CMake.</returns>
        private static Command CreateCMakeBinCommand(Option<bool> verboseOption)
        {
            var cmd = new Command("-cmake-bin", "Builds the binary package using CMake (clang).");
            cmd.SetAction(parseResult =>
            {
                bool verbose = parseResult.GetValue(verboseOption);
                BuildToolsId.CheckForOutOfDateTools();
                BuildFlags flags = BuildFlags.Release | BuildFlags.BuildCMake | (verbose ? BuildFlags.BuildVerbose : BuildFlags.None);
                Build.SetupBuildEnvironment(true);

                if (!Build.BuildSolutionCMake("SystemInformer", BuildGenerator.Ninja, BuildToolchain.ClangMsvcAmd64, flags)) Environment.Exit(1);
                if (!Build.CopyTextFiles(true, flags)) Environment.Exit(1);
                if (!Build.BuildBinZip(flags)) Environment.Exit(1);
                if (!Build.CopyTextFiles(false, flags)) Environment.Exit(1);

                Build.ShowBuildStats();
            });
            return cmd;
        }

        /// <summary>
        /// Creates a command that builds the release configuration using CMake (clang).
        /// </summary>
        /// <param name="verboseOption">Specifies whether to enable verbose output.</param>
        /// <returns>A command configured to perform a CMake release build.</returns>
        private static Command CreateCMakeReleaseCommand(Option<bool> verboseOption)
        {
            var cmd = new Command("-cmake-release", "Builds release configuration using CMake (clang).");
            cmd.SetAction(parseResult =>
            {
                bool verbose = parseResult.GetValue(verboseOption);
                BuildToolsId.CheckForOutOfDateTools();
                BuildFlags flags = BuildFlags.Release | BuildFlags.BuildCMake | (verbose ? BuildFlags.BuildVerbose : BuildFlags.None);
                Build.SetupBuildEnvironment(true);

                if (!Build.BuildSolutionCMake("SystemInformer", BuildGenerator.Ninja, BuildToolchain.ClangMsvcAmd64, flags)) Environment.Exit(1);
                if (!Build.CopyWow64Files(flags)) Environment.Exit(1);
                if (!Build.CopyTextFiles(true, flags)) Environment.Exit(1);
                if (!Build.BuildBinZip(flags)) Environment.Exit(1);

                foreach (var (channel, _) in BuildConfig.Build_Channels)
                {
                    if (!Build.BuildSetupExe(channel, flags)) Environment.Exit(1);
                }

                if (!Build.CopyTextFiles(false, flags)) Environment.Exit(1);
                if (!Build.BuildPdbZip(false, flags)) Environment.Exit(1);
                if (!Build.BuildChecksumsFile()) Environment.Exit(1);

                Build.ShowBuildStats();
            });
            return cmd;
        }

        /// <summary>
        /// Creates a command that performs pipeline build operations using CMake (clang).
        /// </summary>
        /// <param name="verboseOption">Specifies whether to enable verbose output.</param>
        /// <returns>A command configured for CMake pipeline builds.</returns>
        private static Command CreateCMakePipelineBuildCommand(Option<bool> verboseOption)
        {
            var cmd = new Command("-cmake-pipeline-build", "Performs pipeline build operations using CMake (clang).");
            cmd.SetAction(parseResult =>
            {
                bool verbose = parseResult.GetValue(verboseOption);
                BuildFlags flags = BuildFlags.Release | BuildFlags.BuildCMake | (verbose ? BuildFlags.BuildVerbose : BuildFlags.None);

                BuildToolsId.CheckForOutOfDateTools();

                Build.SetupBuildEnvironment(true);
                Build.CopySourceLink(true);

                if (!Build.BuildSolutionCMake("SystemInformer", BuildGenerator.Ninja, BuildToolchain.ClangMsvcAmd64, flags)) 
                    Environment.Exit(1);

                Build.CopyWow64Files(flags);
                Build.ShowBuildStats();
            });
            return cmd;
        }

        /// <summary>
        /// Creates a command that packages pipeline artifacts using CMake (clang).
        /// </summary>
        /// <param name="verboseOption">Specifies whether to enable verbose output.</param>
        /// <returns>A command configured for CMake pipeline packaging.</returns>
        private static Command CreateCMakePipelinePackageCommand(Option<bool> verboseOption)
        {
            var cmd = new Command("-cmake-pipeline-package", "Packages pipeline artifacts using CMake (clang).");
            cmd.SetAction(parseResult =>
            {
                bool verbose = parseResult.GetValue(verboseOption);
                BuildToolsId.CheckForOutOfDateTools();
                BuildFlags flags = BuildFlags.Release | BuildFlags.BuildCMake | (verbose ? BuildFlags.BuildVerbose : BuildFlags.None);
                Build.SetupBuildEnvironment(true);

                if (!Build.ResignFiles("bin-cmake")) Environment.Exit(1);
                if (!Build.CopyTextFiles(true, flags)) Environment.Exit(1);
                if (!Build.BuildBinZip(flags)) Environment.Exit(1);

                foreach (var (channel, _) in BuildConfig.Build_Channels)
                {
                    if (!Build.BuildSetupExe(channel, flags)) Environment.Exit(1);
                }

                if (!Build.CopyTextFiles(false, flags)) Environment.Exit(1);
                Build.ShowBuildStats();
            });
            return cmd;
        }

        /// <summary>
        /// Creates a command that deploys pipeline artifacts using CMake (clang).
        /// </summary>
        /// <param name="verboseOption">Specifies whether to enable verbose output.</param>
        /// <returns>A command configured for CMake pipeline deployment.</returns>
        private static Command CreateCMakePipelineDeployCommand(Option<bool> verboseOption)
        {
            var cmd = new Command("-cmake-pipeline-deploy", "Deploys pipeline artifacts using CMake (clang).");
            cmd.SetAction(async parseResult =>
            {
                bool verbose = parseResult.GetValue(verboseOption);
                BuildToolsId.CheckForOutOfDateTools();
                BuildFlags flags = BuildFlags.Release | BuildFlags.BuildCMake | (verbose ? BuildFlags.BuildVerbose : BuildFlags.None);
                Build.SetupBuildEnvironment(true);

                if (!Build.BuildPdbZip(false, flags)) Environment.Exit(1);
                if (!await BuildDeploy.BuildUpdateServerConfig()) Environment.Exit(1);
                Build.ShowBuildStats();
            });
            return cmd;
        }

        /// <summary>
        /// Creates a command that checks thirdparty library versions against their latest GitHub releases.
        /// </summary>
        /// <returns>A command configured to check thirdparty library versions.</returns>
        private static Command CreateCheckThirdPartyCommand()
        {
            var cmd = new Command("-check-thirdparty", "Checks thirdparty library versions against latest GitHub releases.");
            cmd.SetAction(async _ =>
            {
                BuildToolsId.CheckForOutOfDateTools();
                await BuildThirdParty.CheckThirdPartyVersions();
                Build.ShowBuildStats();
            });
            return cmd;
        }

        // ==========================================
        // Console Helper Methods
        // ==========================================

        /// <summary>
        /// Prints a colorized message to the console.
        /// </summary>
        /// <param name="Message">The message to print.</param>
        /// <param name="Color">The console color to use.</param>
        /// <param name="Newline">Whether to append a newline.</param>
        /// <param name="Flags">Build flags for verbosity control.</param>
        public static void PrintColorMessage(string Message, ConsoleColor Color, bool Newline = true, BuildFlags Flags = BuildFlags.BuildVerbose)
        {
            if ((Flags & BuildFlags.BuildVerbose) == 0) return;

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
                    if (start < Message.Length) text.Append(colour_ansi).Append(Message, start, Message.Length - start);
                    text.Append("\e[0m");

                    if (Newline) Console.WriteLine(text.ToString());
                    else Console.Write(text.ToString());
                }
                else
                {
                    var color_reset = $"{colour_ansi}{Message}\e[0m";
                    if (Newline) Console.WriteLine(color_reset);
                    else Console.Write(color_reset);
                }
            }
            else
            {
                Console.ForegroundColor = Color;
                if (Newline) Console.WriteLine(Message);
                else Console.Write(Message);
                Console.ResetColor();
            }
        }

        /// <summary>
        /// Converts a <see cref="ConsoleColor"/> to its ANSI escape sequence.
        /// </summary>
        /// <param name="color">The console color.</param>
        /// <returns>The ANSI escape sequence string.</returns>
        public static string ToAnsiCode(ConsoleColor color)
        {
            return color switch
            {
                ConsoleColor.Black => "\e[30m",
                ConsoleColor.DarkBlue => "\e[34m",
                ConsoleColor.DarkGreen => "\e[32m",
                ConsoleColor.DarkCyan => "\e[36m",
                ConsoleColor.DarkRed => "\e[31m",
                ConsoleColor.DarkMagenta => "\e[35m",
                ConsoleColor.DarkYellow => "\e[33m",
                ConsoleColor.Gray => "\e[37m",
                ConsoleColor.DarkGray => "\e[90m",
                ConsoleColor.Blue => "\e[94m",
                ConsoleColor.Green => "\e[92m",
                ConsoleColor.Cyan => "\e[96m",
                ConsoleColor.Red => "\e[91m",
                ConsoleColor.Magenta => "\e[95m",
                ConsoleColor.Yellow => "\e[93m",
                ConsoleColor.White => "\e[97m",
                _ => "\e[0m"
            };
        }

        /// <summary>
        /// Creates a terminal hyperlink.
        /// </summary>
        /// <param name="Uri">The target URI.</param>
        /// <param name="Text">The display text.</param>
        /// <returns>A formatted string with the terminal escape sequence for a hyperlink.</returns>
        public static string CreateConsoleHyperlink(string Uri, string Text)
        {
            return $"\e]8;;{Uri}\e\\{Text}\e]8;;\e\\";
        }
    }
}
