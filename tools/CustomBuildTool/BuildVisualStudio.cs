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
    /// Provides methods for discovering and managing installed Visual Studio instances using native setup configuration APIs.
    /// </summary>
    internal static class BuildVisualStudio
    {
        /// <summary>
        /// List of discovered Visual Studio instances on the system.
        /// </summary>
        private static readonly List<VisualStudioInstance> VisualStudioInstanceList;

        /// <summary>
        /// The selected Visual Studio instance, typically the latest version with required dependencies.
        /// </summary>
        private static VisualStudioInstance VisualStudioInstance = null;

        /// <summary>
        /// Static constructor. Discovers all Visual Studio instances using the native setup configuration API and populates <see cref="VisualStudioInstanceList"/>.
        /// </summary>
        static unsafe BuildVisualStudio()
        {
            VisualStudioInstanceList = new List<VisualStudioInstance>();

            if (NativeLibrary.TryLoad(GetLibraryPath(), out IntPtr baseAddress))
            {
                if (NativeLibrary.TryGetExport(baseAddress, "GetSetupConfiguration", out IntPtr ExportAddressPtr))
                {
                    var GetSetupConfiguration_I = (delegate* unmanaged[Stdcall]<IntPtr*, IntPtr, HRESULT>)ExportAddressPtr;
                    IntPtr SetupConfigurationInterfacePtr;
                    IntPtr SetupConfiguration2InterfacePtr;
                    IntPtr EnumSetupInstancesInterfacePtr;
                    IntPtr SetupInstancePtr;
                    IntPtr SetupInstance2Ptr;
                    uint instancesFetched;

                    if (GetSetupConfiguration_I(
                        &SetupConfigurationInterfacePtr,
                        IntPtr.Zero
                        ).Succeeded)
                    {
                        var setupInterface = *(ISetupConfigurationVTable**)SetupConfigurationInterfacePtr;

                        if (setupInterface->QueryInterface(
                            SetupConfigurationInterfacePtr,
                            IID_ISetupConfiguration2,
                            &SetupConfiguration2InterfacePtr
                            ).Succeeded)
                        {
                            var setupConfiguration = *(ISetupConfiguration2VTable**)SetupConfiguration2InterfacePtr;

                            if (setupConfiguration->EnumAllInstances(
                                SetupConfiguration2InterfacePtr,
                                &EnumSetupInstancesInterfacePtr
                                ).Succeeded)
                            {
                                var enumSetupInstances = *(IEnumSetupInstancesVTable**)EnumSetupInstancesInterfacePtr;

                                while (true)
                                {
                                    if (enumSetupInstances->Next(EnumSetupInstancesInterfacePtr, 1, &SetupInstancePtr, &instancesFetched).Failed)
                                        break;
                                    if (instancesFetched == 0)
                                        break;

                                    var setupInstance = *(ISetupInstanceVTable**)SetupInstancePtr;

                                    if (setupInstance->QueryInterface(
                                        SetupInstancePtr,
                                        IID_ISetupInstance2,
                                        &SetupInstance2Ptr
                                        ).Succeeded)
                                    {
                                        var setupInstance2 = *(ISetupInstance2VTable**)SetupInstance2Ptr;
                                        VisualStudioInstanceList.Add(new VisualStudioInstance(setupInstance2, SetupInstance2Ptr));
                                        setupInstance2->Release(SetupInstance2Ptr);
                                    }

                                    setupInstance->Release(SetupInstancePtr);
                                }

                                enumSetupInstances->Release(EnumSetupInstancesInterfacePtr);
                            }

                            setupConfiguration->Release(SetupConfigurationInterfacePtr);
                        }

                        setupInterface->Release(SetupConfigurationInterfacePtr);
                    }
                }

                NativeLibrary.Free(baseAddress);
            }

            // ESDK Begin
            if (Win32.GetEnvironmentVariable("EWDK_ROOT", out string ewdkRoot))
            {
                if (Directory.Exists(ewdkRoot))
                {
                    VisualStudioInstanceList.Add(new VisualStudioInstance("Enterprise WDK", ewdkRoot, "17.0"));
                }
            }
            else if (Win32.GetEnvironmentVariable("VCINSTALLDIR", out string vcInstallDir))
            {
                string vsPath = Path.GetFullPath(Path.Combine(vcInstallDir, "..\\..\\"));
                if (Directory.Exists(vsPath))
                {
                    VisualStudioInstanceList.Add(new VisualStudioInstance("Visual Studio (Environment)", vsPath, "17.0"));
                }
            }
            // ESDK End

            VisualStudioInstanceList.Sort((p1, p2) =>
            {
                if (Version.TryParse(p1.InstallationVersion, out Version version1) && Version.TryParse(p2.InstallationVersion, out Version version2))
                {
                    if (version1 < version2)
                        return 1;
                    else if (version1 > version2)
                        return -1;

                    return version1.CompareTo(version2);
                }

                return 1;
            });
        }

        /// <summary>
        /// Gets the preferred Visual Studio instance, typically the latest version with required dependencies.
        /// </summary>
        /// <returns>
        /// The selected <see cref="VisualStudioInstance"/> or <c>null</c> if none are available.
        /// </returns>
        public static VisualStudioInstance GetVisualStudioInstance()
        {
            if (VisualStudioInstance == null && VisualStudioInstanceList != null)
            {
                foreach (VisualStudioInstance instance in VisualStudioInstanceList)
                {
                    //if (instance.HasRequiredDependency)
                    {
                        VisualStudioInstance = instance;
                        break;
                    }
                    //else
                    //{
                    //    Program.PrintColorMessage($"[WARN] {instance.DisplayName} does not have required the packages:\n{instance.MissingDependencyList}", ConsoleColor.Yellow);
                    //}
                }
            }

            return VisualStudioInstance;
        }

        /// <summary>
        /// GUID for the ISetupConfiguration2 COM interface.
        /// </summary>
        private static readonly Guid IID_ISetupConfiguration2 = new Guid("26AAB78C-4A60-49D6-AF3B-3C35BC93365D");

        /// <summary>
        /// GUID for the ISetupInstance2 COM interface.
        /// </summary>
        private static readonly Guid IID_ISetupInstance2 = new Guid("89143C9A-05AF-49B0-B717-72E218A2185C");

        //private static readonly Guid IID_ISetupConfiguration = new Guid("42843719-DB4C-46C2-8E7C-64F1816EFD5B");
        //private static readonly Guid IID_IEnumSetupInstances = new Guid("6380BCFF-41D3-4B2E-8B2E-BF8A6810C848");
        //private static readonly Guid IID_ISetupInstance = new Guid("B41463C3-8866-43B5-BC33-2B0676F7F42E");
        //private static readonly Guid IID_ISetupPackageReference = new Guid("DA8D8A16-B2B6-4487-A2F1-594CCCCD6BF5");
        //private static readonly Guid IID_ISetupHelper = new Guid("42B21B78-6192-463E-87BF-D577838F1D5C");

        /// <summary>
        /// Gets the path to the native Visual Studio setup configuration library based on process architecture.
        /// </summary>
        /// <returns>
        /// The full path to the setup configuration native DLL.
        /// </returns>
        private static string GetLibraryPath()
        {
            string path;

            // HKEY_LOCAL_MACHINE\SOFTWARE\Classes\CLSID\{177F0C4A-1CD3-4DE7-A32C-71DBBB9FA36D}\InprocServer32
            if (Environment.Is64BitProcess)
                path = "C:\\ProgramData\\Microsoft\\VisualStudio\\Setup\\x64\\Microsoft.VisualStudio.Setup.Configuration.Native.dll";
            else
                path = "C:\\ProgramData\\Microsoft\\VisualStudio\\Setup\\x86\\Microsoft.VisualStudio.Setup.Configuration.Native.dll";

            if (!File.Exists(path))
            {
                if (Environment.Is64BitProcess)
                    path = "C:\\ProgramData\\Microsoft\\VisualStudio\\Setup\\x64\\Microsoft.VisualStudio.Setup.Configuration.NativeMethods.dll";
                else
                    path = "C:\\ProgramData\\Microsoft\\VisualStudio\\Setup\\x86\\Microsoft.VisualStudio.Setup.Configuration.NativeMethods.dll";
            }

            return path;
        }

        /// <summary>
        /// Required workloads for Visual Studio.
        /// </summary>
        private static readonly string[] RequiredWorkloads =
        [
            "Microsoft.VisualStudio.Workload.NativeDesktop"
        ];

        /// <summary>
        /// Required components for Visual Studio.
        /// </summary>
        private static readonly string[] RequiredComponents =
        [
            "Microsoft.VisualStudio.Component.VC.Tools.x86.x64",
            "Microsoft.VisualStudio.Component.VC.Tools.ARM64",
            "Microsoft.VisualStudio.Component.Windows11SDK.26100",
            "Microsoft.VisualStudio.Component.Windows10SDK",
            "Component.Microsoft.Windows.DriverKit",
            "Microsoft.Component.MSBuild",
            "Microsoft.VisualStudio.Component.VC.ATL",
            "Microsoft.VisualStudio.Component.VC.ATLMFC",
            "Microsoft.VisualStudio.Component.VC.Redist.14.Latest",
            "Microsoft.VisualStudio.Component.VC.Runtimes.x86.x64.Spectre",
            "Microsoft.VisualStudio.Component.VC.Runtimes.ARM64.Spectre",
            "Microsoft.VisualStudio.Component.VC.Runtimes.ARM64EC.Spectre",
            "Microsoft.VisualStudio.Component.NuGet",
            "Microsoft.VisualStudio.Component.Git"
        ];

        /// <summary>
        /// Recommended components for Visual Studio.
        /// </summary>
        private static readonly string[] RecommendedComponents =
        [
            "Microsoft.VisualStudio.Component.VC.CMake.Project",
            "Microsoft.VisualStudio.Component.VC.14.45.17.12.CLI.Support"
        ];

        public static bool CheckBuildDependencies(bool quiet = false)
        {
            var result = true;
            var instance = BuildVisualStudio.GetVisualStudioInstance();

            if (!quiet)
            {
                Program.PrintColorMessage("Checking Build Dependencies...\n", ConsoleColor.Cyan);
            }

            if (instance == null)
            {
                if (!quiet)
                {
                    Program.PrintColorMessage("\u274C - Visual Studio 2022/2026 - Not found", ConsoleColor.Red);
                }
                result = false;
            }
            else
            {
                if (!quiet)
                {
                    Program.PrintColorMessage($"\u2705 - {instance.DisplayName} ({instance.InstallationVersion})", ConsoleColor.Green);
                }

                if (instance.Packages.Count > 0)
                {
                    var missingWorkloads = RequiredWorkloads.Where(w => !instance.Packages.Any(p => p.Id.Equals(w, StringComparison.OrdinalIgnoreCase))).ToList();
                    var missingComponents = RequiredComponents.Where(c => !instance.Packages.Any(p => p.Id.Equals(c, StringComparison.OrdinalIgnoreCase))).ToList();

                    if (missingWorkloads.Count > 0 || missingComponents.Count > 0)
                    {
                        result = false;
                        if (!quiet)
                        {
                            Program.PrintColorMessage("Visual Studio missing components:", ConsoleColor.Yellow);
                            foreach (var w in missingWorkloads)
                                Program.PrintColorMessage($"   - Workload: {w}", ConsoleColor.Gray);
                            foreach (var c in missingComponents)
                                Program.PrintColorMessage($"   - Component: {c}", ConsoleColor.Gray);
                        }
                    }
                }
            }

            // Windows SDK check
            string sdkVersion = Utils.GetWindowsSdkVersion();
            if (string.IsNullOrWhiteSpace(sdkVersion))
            {
                if (!quiet)
                    Program.PrintColorMessage("\u274C - Windows SDK - Not found", ConsoleColor.Red);
                result = false;
            }
            else
            {
                if (!quiet)
                    Program.PrintColorMessage($"\u2705 - Windows SDK - {sdkVersion}", ConsoleColor.Green);
            }

            // Git check
            string gitPath = Utils.GetGitFilePath();
            if (string.IsNullOrWhiteSpace(gitPath))
            {
                if (!quiet)
                    Program.PrintColorMessage("\u274C - Git - Not found", ConsoleColor.Yellow);
            }
            else
            {
                if (!quiet)
                    Program.PrintColorMessage("\u2705 - Git", ConsoleColor.Green);
            }

            // .NET check
            try
            {
                Win32.CreateProcess("dotnet.exe", "--version", out string dotnetVersion, false, true);
                if (!string.IsNullOrWhiteSpace(dotnetVersion))
                {
                    if (!quiet)
                        Program.PrintColorMessage($"\u2705 - DotNET - {dotnetVersion.Trim()}", ConsoleColor.Green);
                }
                else
                {
                    if (!quiet)
                        Program.PrintColorMessage("\u274C - DotNET - Not found", ConsoleColor.Yellow);
                }
            }
            catch
            {
                if (!quiet)
                    Program.PrintColorMessage("\u274C - DotNET - Not found", ConsoleColor.Yellow);
            }

            return result;
        }

        public static async Task<bool> InstallBuildDependencies(bool minimal = false)
        {
            string installerPath = Path.Combine(Path.GetTempPath(), "vs_community.exe");

            try
            {
                if (!File.Exists(installerPath))
                {
                    Program.PrintColorMessage("Downloading Visual Studio 2022 Community installer...", ConsoleColor.Cyan);

                    var request = new HttpRequestMessage(HttpMethod.Get, "https://aka.ms/vs/17/release/vs_community.exe");
                    var response = await BuildHttpClient.SendMessageResponse(request);
                    if (response != null && response.IsSuccessStatusCode)
                    {
                        using (var fs = new FileStream(installerPath, FileMode.Create))
                        {
                            await response.Content.CopyToAsync(fs);
                        }
                    }
                }

                if (!File.Exists(installerPath))
                {
                    Program.PrintColorMessage("Failed to download Visual Studio installer.", ConsoleColor.Red);
                    return false;
                }
            }
            catch (Exception ex)
            {
                Program.PrintColorMessage($"Failed to download Visual Studio installer: {ex}", ConsoleColor.Red);
                return false;
            }

            var args = new List<string>
            {
                "--passive",
                "--wait",
                "--norestart"
            };

            foreach (var w in RequiredWorkloads)
            {
                args.Add("--add");
                args.Add(w);
            }

            foreach (var c in RequiredComponents)
            {
                args.Add("--add");
                args.Add(c);
            }

            if (!minimal)
            {
                foreach (var c in RecommendedComponents)
                {
                    args.Add("--add");
                    args.Add(c);
                }
            }

            args.Add("--add");
            args.Add("Microsoft.NetCore.Component.Runtime.10.0");

            string commandLine = string.Join(" ", args.Select(a => a.Contains(' ', StringComparison.OrdinalIgnoreCase) ? $"\"{a}\"" : a));
            Program.PrintColorMessage($"Installing Visual Studio components... {commandLine}", ConsoleColor.Cyan);

            int exitCode = Win32.CreateProcess(installerPath, commandLine, out _, false, false);

            if (exitCode == 0 || exitCode == 3010)
            {
                Program.PrintColorMessage("Visual Studio installation completed successfully.", ConsoleColor.Green);
                if (exitCode == 3010)
                    Program.PrintColorMessage("A restart is required to complete installation.", ConsoleColor.Yellow);
                return true;
            }
            else
            {
                if (exitCode == 8006)
                {
                    Program.PrintColorMessage($"The Visual Studio installation failed because it attempted to install or modify workloads while Visual Studio (or related processes) are open.", ConsoleColor.Red);
                }
                else
                {
                    Program.PrintColorMessage($"Visual Studio installation failed with exit code: {exitCode}", ConsoleColor.Red);
                }
                return false;
            }
        }
    }

    /// <summary>
    /// Represents a single Visual Studio instance and its installed packages.
    /// </summary>
    internal unsafe class VisualStudioInstance : IComparable, IComparable<VisualStudioInstance>
    {
        /// <summary>
        /// Gets the display name of the Visual Studio instance.
        /// </summary>
        public string DisplayName { get; }

        /// <summary>
        /// Gets the internal name of the Visual Studio instance.
        /// </summary>
        public string Name { get; }

        /// <summary>
        /// Gets the installation path of the Visual Studio instance.
        /// </summary>
        public string Path { get; }

        /// <summary>
        /// Gets the installation version string of the Visual Studio instance.
        /// </summary>
        public string InstallationVersion { get; }

        /// <summary>
        /// Gets the instance identifier of the Visual Studio instance.
        /// </summary>
        public string InstanceId { get; }

        /// <summary>
        /// Gets the state flags of the Visual Studio instance.
        /// </summary>
        public uint State { get; }

        /// <summary>
        /// Gets the list of installed packages for this Visual Studio instance.
        /// </summary>
        public List<VisualStudioPackage> Packages { get; }

        /// <summary>
        /// Gets a value indicating whether the instance has ARM64 build tools components installed.
        /// </summary>
        public bool HasARM64BuildToolsComponents { get; }

        /// <summary>
        /// Initializes a new instance of the <see cref="VisualStudioInstance"/> class.
        /// </summary>
        public VisualStudioInstance()
        {
            this.Packages = new List<VisualStudioPackage>();
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="VisualStudioInstance"/> class with manual parameters.
        /// </summary>
        /// <param name="Name">Display name.</param>
        /// <param name="Path">Installation path.</param>
        /// <param name="Version">Installation version.</param>
        public VisualStudioInstance(string Name, string Path, string Version)
        {
            this.Name = Name;
            this.DisplayName = Name;
            this.Path = Path;
            this.InstallationVersion = Version;
            this.Packages = new List<VisualStudioPackage>();
            this.HasARM64BuildToolsComponents = true; // Assume true for manual/ESDK instances
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="VisualStudioInstance"/> class from a native setup instance pointer.
        /// </summary>
        /// <param name="FromInstance">Pointer to the native ISetupInstance2 vtable.</param>
        /// <param name="SetupInstancePtr">Pointer to the native setup instance.</param>
        public VisualStudioInstance(ISetupInstance2VTable* FromInstance, IntPtr SetupInstancePtr)
        {
            IntPtr NamePtr;
            IntPtr PathPtr;
            IntPtr VersionPtr;
            IntPtr DisplayNamePtr;
            Windows.Win32.System.Com.SAFEARRAY* SetupPackagesArrayPtr;
            IntPtr SetupPackagePtr;
            uint SetupPackageState;

            //if (FromInstance->GetInstanceId(SetupInstancePtr, out IntPtr InstancePtr) == NativeMethods.HRESULT_S_OK && InstancePtr != IntPtr.Zero)
            //{
            //    this.InstanceId = Marshal.PtrToStringBSTR(InstancePtr);
            //}

            if (FromInstance->GetInstallationName(SetupInstancePtr, &NamePtr).Succeeded && NamePtr != IntPtr.Zero)
            {
                this.Name = Marshal.PtrToStringBSTR(NamePtr);
            }

            if (FromInstance->GetInstallationPath(SetupInstancePtr, &PathPtr).Succeeded && PathPtr != IntPtr.Zero)
            {
                this.Path = Marshal.PtrToStringBSTR(PathPtr);
            }

            if (FromInstance->GetInstallationVersion(SetupInstancePtr, &VersionPtr).Succeeded && VersionPtr != IntPtr.Zero)
            {
                this.InstallationVersion = Marshal.PtrToStringBSTR(VersionPtr);
            }

            if (FromInstance->GetDisplayName(SetupInstancePtr, 0, &DisplayNamePtr).Succeeded && DisplayNamePtr != IntPtr.Zero)
            {
                this.DisplayName = Marshal.PtrToStringBSTR(DisplayNamePtr);
            }

            if (FromInstance->GetState(SetupInstancePtr, &SetupPackageState).Succeeded)
            {
                this.State = SetupPackageState;
            }

            this.Packages = new List<VisualStudioPackage>();

            if (FromInstance->GetPackages(SetupInstancePtr, &SetupPackagesArrayPtr).Succeeded)
            {
                if (PInvoke.SafeArrayGetLBound(SetupPackagesArrayPtr, 1, out var lbound).Succeeded &&
                    PInvoke.SafeArrayGetUBound(SetupPackagesArrayPtr, 1, out var ubound).Succeeded)
                {
                    var count = ubound - lbound + 1;

                    for (int i = 0; i < count; i++)
                    {
                        if (PInvoke.SafeArrayGetElement(
                            SetupPackagesArrayPtr,
                            &i,
                            &SetupPackagePtr
                            ).Succeeded)
                        {
                            var package = *(ISetupPackageReferenceVTable**)SetupPackagePtr;

                            this.Packages.Add(new VisualStudioPackage(package, SetupPackagePtr));

                            package->Release(SetupPackagePtr);
                        }
                    }
                }
            }

            // https://learn.microsoft.com/en-us/visualstudio/install/workload-component-id-vs-community

            var found = this.Packages.FindAll(p => 
                p.Id.Equals("Microsoft.VisualStudio.Component.VC.Tools.ARM64", StringComparison.OrdinalIgnoreCase) ||
                p.Id.Equals("Microsoft.VisualStudio.Component.VC.Runtimes.ARM64.Spectre", StringComparison.OrdinalIgnoreCase));

            this.HasARM64BuildToolsComponents = found.Count >= 2;
        }

        /// <summary>
        /// Gets the latest installed Windows SDK package for this Visual Studio instance.
        /// </summary>
        /// <returns>
        /// The <see cref="VisualStudioPackage"/> representing the latest Windows SDK, or <c>null</c> if none found.
        /// </returns>
        private VisualStudioPackage GetLatestSdkPackage()
        {
            var found = this.Packages.FindAll(p => 
                p.Id.StartsWith("Microsoft.VisualStudio.Component.Windows10SDK", StringComparison.OrdinalIgnoreCase) ||                                                
                p.Id.StartsWith("Microsoft.VisualStudio.Component.Windows11SDK", StringComparison.OrdinalIgnoreCase));
        
            if (found.Count == 0)
                return null;

            found.Sort((p1, p2) =>
            {
                if (Version.TryParse(p1.Version, out Version v1) && Version.TryParse(p2.Version, out Version v2))
                    return v1.CompareTo(v2);
                else
                    return 1;
            });
        
            return found[^1];
        }

        //public string GetWindowsSdkVersion()
        //{
        //    List<string> versions = new List<string>();
        //    VisualStudioPackage package = GetLatestSdkPackage();
        //
        //    if (package == null)
        //        return string.Empty;
        //
        //    var found = this.Packages.FindAll(p =>
        //    {
        //        return p.Id.StartsWith("Microsoft.VisualStudio.Component.Windows10SDK", StringComparison.OrdinalIgnoreCase) ||
        //            p.Id.StartsWith("Microsoft.VisualStudio.Component.Windows11SDK", StringComparison.OrdinalIgnoreCase);
        //    });
        //
        //    foreach (var sdk in found)
        //    {
        //        string[] tokens = sdk.Id.Split(".", StringSplitOptions.RemoveEmptyEntries | StringSplitOptions.TrimEntries);
        //
        //        foreach (string name in tokens)
        //        {
        //            if (uint.TryParse(name, out uint version))
        //            {
        //                versions.Add(name);
        //                break;
        //            }
        //        }
        //    }
        //
        //    versions.Sort((p1, p2) =>
        //    {
        //        if (Version.TryParse(p1, out Version v1) && Version.TryParse(p2, out Version v2))
        //            return v1.CompareTo(v2);
        //        else
        //            return 1;
        //    });
        //
        //    if (versions.Count > 0)
        //    {
        //        return versions[^1];
        //    }
        //    else
        //    {
        //        return package.Id;
        //    }
        //}
        //

        /// <summary>
        /// Gets the full version string of the latest installed Windows SDK package.
        /// </summary>
        /// <returns>
        /// The version string of the latest Windows SDK package, or <c>string.Empty</c> if none found.
        /// </returns>
        public string GetWindowsSdkFullVersion()
        {
            VisualStudioPackage package = GetLatestSdkPackage();

            return package == null ? string.Empty : package.Version;
        }

        //public bool HasRequiredDependency
        //{
        //    get
        //    {
        //        bool hasbuild = false;
        //        bool hasruntimes = false;
        //        bool haswindowssdk = false;
        //
        //        hasbuild = this.Packages.Exists(p => p.Id.StartsWith("Microsoft.Component.MSBuild", StringComparison.OrdinalIgnoreCase));
        //        hasruntimes = this.Packages.Exists(p => p.Id.StartsWith("Microsoft.VisualStudio.Component.VC", StringComparison.OrdinalIgnoreCase));
        //        haswindowssdk = this.Packages.Exists(p =>
        //        {
        //            return p.Id.StartsWith("Microsoft.VisualStudio.Component.Windows10SDK", StringComparison.OrdinalIgnoreCase) ||
        //                p.Id.StartsWith("Microsoft.VisualStudio.Component.Windows11SDK", StringComparison.OrdinalIgnoreCase);
        //        });
        //
        //        return hasbuild && hasruntimes && haswindowssdk;
        //    }
        //}
        //
        //public string MissingDependencyList
        //{
        //    get
        //    {
        //        string list = string.Empty;
        //
        //        if (!this.Packages.Exists(p => p.Id.StartsWith("Microsoft.Component.MSBuild", StringComparison.OrdinalIgnoreCase)))
        //        {
        //            list += "MSBuild" + Environment.NewLine;
        //        }
        //
        //        if (!this.Packages.Exists(p => p.Id.StartsWith("Microsoft.VisualStudio.Component.VC", StringComparison.OrdinalIgnoreCase)))
        //        {
        //            list += "VC.14.Redist" + Environment.NewLine;
        //        }
        //
        //        if (!this.Packages.Exists(p =>
        //        {
        //            return p.Id.StartsWith("Microsoft.VisualStudio.Component.Windows10SDK", StringComparison.OrdinalIgnoreCase) ||
        //                p.Id.StartsWith("Microsoft.VisualStudio.Component.Windows11SDK", StringComparison.OrdinalIgnoreCase);
        //        }))
        //        {
        //            list += "Windows SDK" + Environment.NewLine;
        //        }
        //
        //        return list;
        //    }
        //}

        /// <summary>
        /// Returns the internal name of the Visual Studio instance.
        /// </summary>
        /// <returns>The internal name.</returns>
        public override string ToString()
        {
            return this.Name;
        }

        /// <summary>
        /// Compares this instance to another object by name.
        /// </summary>
        /// <param name="obj">The object to compare to.</param>
        /// <returns>
        /// A value indicating the relative order of the instances.
        /// </returns>
        public int CompareTo(object obj)
        {
            if (obj == null)
                return 1;

            if (obj is VisualStudioInstance instance)
                return string.Compare(this.Name, instance.Name, StringComparison.OrdinalIgnoreCase);
            else
                return 1;
        }

        /// <summary>
        /// Compares this instance to another <see cref="VisualStudioInstance"/> by name.
        /// </summary>
        /// <param name="obj">The instance to compare to.</param>
        /// <returns>
        /// A value indicating the relative order of the instances.
        /// </returns>
        public int CompareTo(VisualStudioInstance obj)
        {
            if (obj == null)
                return 1;

            return string.Compare(this.Name, obj.Name, StringComparison.OrdinalIgnoreCase);
        }
    }

    /// <summary>
    /// Represents a Visual Studio package/component installed in an instance.
    /// </summary>
    internal unsafe class VisualStudioPackage : IComparable, IComparable<VisualStudioPackage>
    {
        /// <summary>
        /// Gets the package/component ID.
        /// </summary>
        public string Id { get; }

        /// <summary>
        /// Gets the version string of the package/component.
        /// </summary>
        public string Version { get; }

        /// <summary>
        /// Initializes a new instance of the <see cref="VisualStudioPackage"/> class from a native package reference pointer.
        /// </summary>
        /// <param name="FromInstance">Pointer to the native ISetupPackageReference vtable.</param>
        /// <param name="SetupInstancePtr">Pointer to the native package reference.</param>
        public VisualStudioPackage(ISetupPackageReferenceVTable* FromInstance, IntPtr SetupInstancePtr)
        {
            IntPtr IdPtr;
            IntPtr VersionPtr;

            if (FromInstance->GetId(SetupInstancePtr, &IdPtr).Succeeded && IdPtr != IntPtr.Zero)
            {
                this.Id = Marshal.PtrToStringBSTR(IdPtr);
            }

            if (FromInstance->GetVersion(SetupInstancePtr, &VersionPtr).Succeeded && VersionPtr != IntPtr.Zero)
            {
                this.Version = Marshal.PtrToStringBSTR(VersionPtr);
            }
        }

        /// <summary>
        /// Returns the package/component ID.
        /// </summary>
        /// <returns>The package/component ID.</returns>
        public override string ToString()
        {
            return this.Id;
        }

        /// <summary>
        /// Compares this package to another object by ID.
        /// </summary>
        /// <param name="obj">The object to compare to.</param>
        /// <returns>
        /// A value indicating the relative order of the packages.
        /// </returns>
        public int CompareTo(object obj)
        {
            if (obj == null)
                return 1;

            if (obj is VisualStudioPackage package)
                return string.Compare(this.Id, package.Id, StringComparison.OrdinalIgnoreCase);
            else
                return 1;
        }

        /// <summary>
        /// Compares this package to another <see cref="VisualStudioPackage"/> by ID.
        /// </summary>
        /// <param name="obj">The package to compare to.</param>
        /// <returns>
        /// A value indicating the relative order of the packages.
        /// </returns>
        public int CompareTo(VisualStudioPackage obj)
        {
            if (obj == null)
                return 1;

            return string.Compare(this.Id, obj.Id, StringComparison.OrdinalIgnoreCase);
        }
    }

    [Flags]
    public enum InstanceState : uint
    {
        None = 0u,
        Local = 1u,
        Registered = 2u,
        NoRebootRequired = 4u,
        NoErrors = 8u,
        Complete = 4294967295u
    }

    [StructLayout(LayoutKind.Sequential)]
    internal readonly unsafe struct ISetupHelperVTable
    {
        public readonly delegate* unmanaged[Stdcall]<IntPtr, Guid, IntPtr*, HRESULT> QueryInterface;
        public readonly delegate* unmanaged[Stdcall]<IntPtr, uint> AddRef;
        public readonly delegate* unmanaged[Stdcall]<IntPtr, uint> Release;
        // ISetupHelper
        public readonly delegate* unmanaged[Stdcall]<IntPtr, IntPtr, uint*, uint> ParseVersion;
        public readonly delegate* unmanaged[Stdcall]<IntPtr, IntPtr, uint*, uint*, uint> ParseVersionRange;
    }

    [StructLayout(LayoutKind.Sequential)]
    internal readonly unsafe struct ISetupConfigurationVTable
    {
        public readonly delegate* unmanaged[Stdcall]<IntPtr, Guid, IntPtr*, HRESULT> QueryInterface;
        public readonly delegate* unmanaged[Stdcall]<IntPtr, uint> AddRef;
        public readonly delegate* unmanaged[Stdcall]<IntPtr, uint> Release;
        // STDMETHOD(EnumInstances)(_Out_ IEnumSetupInstances **ppEnumInstances) = 0;
        public readonly delegate* unmanaged[Stdcall]<IntPtr, IntPtr*, uint> EnumInstances;
        // STDMETHOD(GetInstanceForCurrentProcess)(_Out_ ISetupInstance ** ppInstance) = 0;
        public readonly delegate* unmanaged[Stdcall]<IntPtr, IntPtr*, uint> GetInstanceForCurrentProcess;
        // STDMETHOD(GetInstanceForPath)(_In_z_ LPCWSTR wzPath, _Out_ ISetupInstance **ppInstance) = 0;
        public readonly delegate* unmanaged[Stdcall]<IntPtr, IntPtr, IntPtr*, uint> GetInstanceForPath;
    }

    [StructLayout(LayoutKind.Sequential)]
    internal readonly unsafe struct ISetupConfiguration2VTable
    {
        public readonly delegate* unmanaged[Stdcall]<IntPtr, Guid, IntPtr*, HRESULT> QueryInterface;
        public readonly delegate* unmanaged[Stdcall]<IntPtr, uint> AddRef;
        public readonly delegate* unmanaged[Stdcall]<IntPtr, uint> Release;
        // STDMETHOD(EnumAllInstances)(_Out_ IEnumSetupInstances **ppEnumInstances) = 0;
        public readonly delegate* unmanaged[Stdcall]<IntPtr, IntPtr*, HRESULT> EnumAllInstances;
    }

    [StructLayout(LayoutKind.Sequential)]
    internal readonly unsafe struct IEnumSetupInstancesVTable
    {
        public readonly delegate* unmanaged[Stdcall]<IntPtr, Guid, IntPtr*, HRESULT> QueryInterface;
        public readonly delegate* unmanaged[Stdcall]<IntPtr, uint> AddRef;
        public readonly delegate* unmanaged[Stdcall]<IntPtr, uint> Release;
        // STDMETHOD(Next)(_In_ ULONG celt, _Out_writes_to_(celt, *pceltFetched) ISetupInstance** rgelt, _Out_opt_ _Deref_out_range_(0, celt) ULONG* pceltFetched) = 0;
        public readonly delegate* unmanaged[Stdcall]<IntPtr, uint, IntPtr*, uint*, HRESULT> Next;
        // STDMETHOD(Skip)(_In_ ULONG celt) = 0;
        public readonly delegate* unmanaged[Stdcall]<IntPtr, uint, uint> Skip;
        // STDMETHOD(Reset)(void) = 0;
        public readonly delegate* unmanaged[Stdcall]<IntPtr, uint> Reset;
        // STDMETHOD(Clone)(_Deref_out_opt_ IEnumSetupInstances **ppenum) = 0;
        public readonly delegate* unmanaged[Stdcall]<IntPtr, IntPtr*, uint> Clone;
    }

    [StructLayout(LayoutKind.Sequential)]
    internal readonly unsafe struct ISetupInstanceVTable
    {
        public readonly delegate* unmanaged[Stdcall]<IntPtr, Guid, IntPtr*, HRESULT> QueryInterface;
        public readonly delegate* unmanaged[Stdcall]<IntPtr, uint> AddRef;
        public readonly delegate* unmanaged[Stdcall]<IntPtr, uint> Release;
        // ISetupInstance
        // STDMETHOD(GetInstanceId)(_Out_ BSTR *pbstrInstanceId) = 0;
        public readonly delegate* unmanaged[Stdcall]<IntPtr, IntPtr*, uint> GetInstanceId;
        // STDMETHOD(GetInstallDate)(_Out_ LPFILETIME pInstallDate) = 0;
        public readonly delegate* unmanaged[Stdcall]<IntPtr, long*, uint> GetInstallDate;
        // STDMETHOD(GetInstallationName)(_Out_ BSTR *pbstrInstallationName) = 0;
        public readonly delegate* unmanaged[Stdcall]<IntPtr, IntPtr*, uint> GetInstallationName;
        // STDMETHOD(GetInstallationPath)(_Out_ BSTR *pbstrInstallationPath) = 0;
        public readonly delegate* unmanaged[Stdcall]<IntPtr, IntPtr*, uint> GetInstallationPath;
        // STDMETHOD(GetInstallationVersion)(_Out_ BSTR *pbstrInstallationVersion) = 0;
        public readonly delegate* unmanaged[Stdcall]<IntPtr, IntPtr*, uint> GetInstallationVersion;
        // STDMETHOD(GetDisplayName)(_In_ LCID lcid, _Out_ BSTR *pbstrDisplayName) = 0;
        public readonly delegate* unmanaged[Stdcall]<IntPtr, uint, IntPtr*, uint> GetDisplayName;
        // STDMETHOD(GetDescription)(_In_ LCID lcid, _Out_ BSTR *pbstrDescription) = 0;
        public readonly delegate* unmanaged[Stdcall]<IntPtr, uint, IntPtr*, uint> GetDescription;
        // STDMETHOD(ResolvePath)(_In_opt_z_ LPCOLESTR pwszRelativePath, _Out_ BSTR* pbstrAbsolutePath) = 0;
        public readonly delegate* unmanaged[Stdcall]<IntPtr, IntPtr, IntPtr*, uint> ResolvePath;
    }

    [StructLayout(LayoutKind.Sequential)]
    internal readonly unsafe struct ISetupInstance2VTable
    {
        public readonly delegate* unmanaged[Stdcall]<IntPtr, Guid, IntPtr*, HRESULT> QueryInterface;
        public readonly delegate* unmanaged[Stdcall]<IntPtr, uint> AddRef;
        public readonly delegate* unmanaged[Stdcall]<IntPtr, uint> Release;
        // ISetupInstance
        // STDMETHOD(GetInstanceId)(_Out_ BSTR *pbstrInstanceId) = 0;
        public readonly delegate* unmanaged[Stdcall]<IntPtr, IntPtr*, HRESULT> GetInstanceId;
        // STDMETHOD(GetInstallDate)(_Out_ LPFILETIME pInstallDate) = 0;
        public readonly delegate* unmanaged[Stdcall]<IntPtr, long*, HRESULT> GetInstallDate;
        // STDMETHOD(GetInstallationName)(_Out_ BSTR *pbstrInstallationName) = 0;
        public readonly delegate* unmanaged[Stdcall]<IntPtr, IntPtr*, HRESULT> GetInstallationName;
        // STDMETHOD(GetInstallationPath)(_Out_ BSTR *pbstrInstallationPath) = 0;
        public readonly delegate* unmanaged[Stdcall]<IntPtr, IntPtr*, HRESULT> GetInstallationPath;
        // STDMETHOD(GetInstallationVersion)(_Out_ BSTR *pbstrInstallationVersion) = 0;
        public readonly delegate* unmanaged[Stdcall]<IntPtr, IntPtr*, HRESULT> GetInstallationVersion;
        // STDMETHOD(GetDisplayName)(_In_ LCID lcid, _Out_ BSTR *pbstrDisplayName) = 0;
        public readonly delegate* unmanaged[Stdcall]<IntPtr, uint, IntPtr*, HRESULT> GetDisplayName;
        // STDMETHOD(GetDescription)(_In_ LCID lcid, _Out_ BSTR *pbstrDescription) = 0;
        public readonly delegate* unmanaged[Stdcall]<IntPtr, uint, IntPtr*, HRESULT> GetDescription;
        // STDMETHOD(ResolvePath)(_In_opt_z_ LPCOLESTR pwszRelativePath, _Out_ BSTR* pbstrAbsolutePath) = 0;
        public readonly delegate* unmanaged[Stdcall]<IntPtr, IntPtr, IntPtr*, HRESULT> ResolvePath;
        // ISetupInstance2
        // STDMETHOD(GetState)(_Out_ InstanceState *pState) = 0;
        public readonly delegate* unmanaged[Stdcall]<IntPtr, uint*, HRESULT> GetState;
        // STDMETHOD(GetPackages)(_Out_ LPSAFEARRAY *ppsaPackages) = 0;
        public readonly delegate* unmanaged[Stdcall]<IntPtr, Windows.Win32.System.Com.SAFEARRAY**, HRESULT> GetPackages;
        // STDMETHOD(GetProduct)(_Outptr_result_maybenull_ ISetupPackageReference **ppPackage) = 0;
        public readonly delegate* unmanaged[Stdcall]<IntPtr, IntPtr*, HRESULT> GetProduct;
        // STDMETHOD(GetProductPath)(_Outptr_result_maybenull_ BSTR *pbstrProductPath) = 0;
        public readonly delegate* unmanaged[Stdcall]<IntPtr, IntPtr*, HRESULT> GetProductPath;
        // STDMETHOD(GetErrors)(_Outptr_result_maybenull_ ISetupErrorState** ppErrorState) = 0;
        public readonly delegate* unmanaged[Stdcall]<IntPtr, IntPtr*, HRESULT> GetErrors;
        // STDMETHOD(IsLaunchable)(_Out_ VARIANT_BOOL* pfIsLaunchable) = 0;
        public readonly delegate* unmanaged[Stdcall]<IntPtr, short*, HRESULT> IsLaunchable;
        // STDMETHOD(IsComplete)(_Out_ VARIANT_BOOL* pfIsComplete) = 0;
        public readonly delegate* unmanaged[Stdcall]<IntPtr, short*, HRESULT> IsComplete;
        // STDMETHOD(GetProperties)(_Outptr_result_maybenull_ ISetupPropertyStore** ppProperties) = 0;
        public readonly delegate* unmanaged[Stdcall]<IntPtr, IntPtr*, HRESULT> GetProperties;
        // STDMETHOD(GetEnginePath)(_Outptr_result_maybenull_ BSTR* pbstrEnginePath) = 0;
        public readonly delegate* unmanaged[Stdcall]<IntPtr, IntPtr*, HRESULT> GetEnginePath;
    }

    [StructLayout(LayoutKind.Sequential)]
    internal readonly unsafe struct ISetupPackageReferenceVTable
    {
        public readonly delegate* unmanaged[Stdcall]<IntPtr, Guid, IntPtr*, HRESULT> QueryInterface;
        public readonly delegate* unmanaged[Stdcall]<IntPtr, uint> AddRef;
        public readonly delegate* unmanaged[Stdcall]<IntPtr, uint> Release;
        // ISetupPackageReference
        // STDMETHOD(GetId)(_Out_ BSTR* pbstrId) = 0;
        public readonly delegate* unmanaged[Stdcall]<IntPtr, IntPtr*, HRESULT> GetId;
        // STDMETHOD(GetVersion)(_Out_ BSTR* pbstrVersion) = 0;
        public readonly delegate* unmanaged[Stdcall]<IntPtr, IntPtr*, HRESULT> GetVersion;
        // STDMETHOD(GetChip)(_Out_ BSTR* pbstrChip) = 0;
        public readonly delegate* unmanaged[Stdcall]<IntPtr, IntPtr*, HRESULT> GetChip;
        // STDMETHOD(GetLanguage)(_Out_ BSTR* pbstrLanguage) = 0;
        public readonly delegate* unmanaged[Stdcall]<IntPtr, IntPtr*, HRESULT> GetLanguage;
        // STDMETHOD(GetBranch)(_Out_ BSTR* pbstrBranch) = 0;
        public readonly delegate* unmanaged[Stdcall]<IntPtr, IntPtr*, HRESULT> GetBranch;
        // STDMETHOD(GetType)(_Out_ BSTR* pbstrType) = 0;
        public readonly delegate* unmanaged[Stdcall]<IntPtr, IntPtr*, HRESULT> GetPackageType;
        // STDMETHOD(GetUniqueId)(_Out_ BSTR* pbstrUniqueId) = 0;
        public readonly delegate* unmanaged[Stdcall]<IntPtr, IntPtr*, HRESULT> GetUniqueId;
        // STDMETHOD(GetIsExtension)(_Out_ BSTR* pfIsExtension) = 0;
        public readonly delegate* unmanaged[Stdcall]<IntPtr, ushort*, HRESULT> GetIsExtension;
    }
}
