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
    public static unsafe class VisualStudio
    {
        private static readonly List<VisualStudioInstance> VisualStudioInstanceList;

        static VisualStudio()
        {
            VisualStudioInstanceList = new List<VisualStudioInstance>();

            if (NativeLibrary.TryLoad(GetLibraryPath(), out IntPtr baseAddress))
            {
                if (NativeLibrary.TryGetExport(baseAddress, "GetSetupConfiguration", out IntPtr ExportAddressPtr))
                {
                    var GetSetupConfiguration_I = (delegate* unmanaged[Stdcall]<IntPtr*, IntPtr, uint>)ExportAddressPtr;
                    IntPtr SetupConfigurationInterfacePtr;
                    IntPtr SetupConfiguration2InterfacePtr;
                    IntPtr EnumSetupInstancesInterfacePtr;
                    IntPtr SetupInstancePtr;
                    IntPtr SetupInstance2Ptr;
                    uint instancesFetched;

                    if (GetSetupConfiguration_I(
                        &SetupConfigurationInterfacePtr,
                        IntPtr.Zero
                        ) == NativeMethods.HRESULT_S_OK)
                    {
                        var setupInterface = *(ISetupConfigurationVTable**)SetupConfigurationInterfacePtr;

                        if (setupInterface->QueryInterface(
                            SetupConfigurationInterfacePtr,
                            IID_ISetupConfiguration2,
                            &SetupConfiguration2InterfacePtr
                            ) == NativeMethods.HRESULT_S_OK)
                        {
                            var setupConfiguration = *(ISetupConfiguration2VTable**)SetupConfiguration2InterfacePtr;

                            if (setupConfiguration->EnumAllInstances(
                                SetupConfiguration2InterfacePtr,
                                &EnumSetupInstancesInterfacePtr
                                ) == NativeMethods.HRESULT_S_OK)
                            {
                                var enumSetupInstances = *(IEnumSetupInstancesVTable**)EnumSetupInstancesInterfacePtr;

                                while (true)
                                {
                                    if (enumSetupInstances->Next(EnumSetupInstancesInterfacePtr, 1, &SetupInstancePtr, &instancesFetched) != NativeMethods.HRESULT_S_OK)
                                        break;
                                    if (instancesFetched == 0)
                                        break;

                                    var setupInstance = *(ISetupInstanceVTable**)SetupInstancePtr;

                                    if (setupInstance->QueryInterface(
                                        SetupInstancePtr,
                                        IID_ISetupInstance2,
                                        &SetupInstance2Ptr
                                        ) == NativeMethods.HRESULT_S_OK)
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

        public static VisualStudioInstance GetVisualStudioInstance()
        {
            if (VisualStudioInstanceList != null)
            {
                foreach (VisualStudioInstance instance in VisualStudioInstanceList)
                {
                    //if (instance.HasRequiredDependency)
                    {
                        return instance;
                    }
                    //else
                    //{
                    //    Program.PrintColorMessage($"[WARN] {instance.DisplayName} does not have required the packages:\n{instance.MissingDependencyList}", ConsoleColor.Yellow);
                    //}
                }
            }

            return null;
        }

        //private static readonly Guid IID_ISetupConfiguration = new Guid("42843719-DB4C-46C2-8E7C-64F1816EFD5B");
        private static readonly Guid IID_ISetupConfiguration2 = new Guid("26AAB78C-4A60-49D6-AF3B-3C35BC93365D");
        //private static readonly Guid IID_IEnumSetupInstances = new Guid("6380BCFF-41D3-4B2E-8B2E-BF8A6810C848");
        //private static readonly Guid IID_ISetupInstance = new Guid("B41463C3-8866-43B5-BC33-2B0676F7F42E");
        private static readonly Guid IID_ISetupInstance2 = new Guid("89143C9A-05AF-49B0-B717-72E218A2185C");
        //private static readonly Guid IID_ISetupPackageReference = new Guid("DA8D8A16-B2B6-4487-A2F1-594CCCCD6BF5");
        //private static readonly Guid IID_ISetupHelper = new Guid("42B21B78-6192-463E-87BF-D577838F1D5C");

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
    }

    public unsafe class VisualStudioInstance : IComparable, IComparable<VisualStudioInstance>
    {
        public string DisplayName { get; }
        public string Name { get; }
        public string Path { get; }
        public string InstallationVersion { get; }
        //public string InstanceId { get; }
        public InstanceState State { get; }
        public List<VisualStudioPackage> Packages { get; }
        public bool HasARM64BuildToolsComponents { get; }

        public VisualStudioInstance()
        {
            this.Packages = new List<VisualStudioPackage>();
        }

        public VisualStudioInstance(ISetupInstance2VTable* FromInstance, IntPtr SetupInstancePtr)
        {
            IntPtr NamePtr;
            IntPtr PathPtr;
            IntPtr VersionPtr;
            IntPtr DisplayNamePtr;
            IntPtr SetupPackagesArrayPtr;
            IntPtr SetupPackagePtr;
            uint SetupPackageState;

            //if (FromInstance->GetInstanceId(SetupInstancePtr, out IntPtr InstancePtr) == NativeMethods.HRESULT_S_OK && InstancePtr != IntPtr.Zero)
            //{
            //    this.InstanceId = Marshal.PtrToStringBSTR(InstancePtr);
            //}

            if (FromInstance->GetInstallationName(SetupInstancePtr, &NamePtr) == NativeMethods.HRESULT_S_OK && NamePtr != IntPtr.Zero)
            {
                this.Name = Marshal.PtrToStringBSTR(NamePtr);
            }

            if (FromInstance->GetInstallationPath(SetupInstancePtr, &PathPtr) == NativeMethods.HRESULT_S_OK && PathPtr != IntPtr.Zero)
            {
                this.Path = Marshal.PtrToStringBSTR(PathPtr);
            }

            if (FromInstance->GetInstallationVersion(SetupInstancePtr, &VersionPtr) == NativeMethods.HRESULT_S_OK && VersionPtr != IntPtr.Zero)
            {
                this.InstallationVersion = Marshal.PtrToStringBSTR(VersionPtr);
            }

            if (FromInstance->GetDisplayName(SetupInstancePtr, 0, &DisplayNamePtr) == NativeMethods.HRESULT_S_OK && DisplayNamePtr != IntPtr.Zero)
            {
                this.DisplayName = Marshal.PtrToStringBSTR(DisplayNamePtr);
            }

            if (FromInstance->GetState(SetupInstancePtr, &SetupPackageState) == NativeMethods.HRESULT_S_OK)
            {
                this.State = (InstanceState)SetupPackageState;
            }

            this.Packages = new List<VisualStudioPackage>();

            if (FromInstance->GetPackages(SetupInstancePtr, &SetupPackagesArrayPtr) == NativeMethods.HRESULT_S_OK)
            {
                if (NativeMethods.SafeArrayGetLBound(SetupPackagesArrayPtr, 1, out uint lbound) == NativeMethods.HRESULT_S_OK &&
                    NativeMethods.SafeArrayGetUBound(SetupPackagesArrayPtr, 1, out uint ubound) == NativeMethods.HRESULT_S_OK)
                {
                    uint count = ubound - lbound + 1;

                    for (uint i = 0; i < count; i++)
                    {
                        if (NativeMethods.SafeArrayGetElement(
                            SetupPackagesArrayPtr,
                            &i,
                            &SetupPackagePtr
                            ) == NativeMethods.HRESULT_S_OK)
                        {
                            var package = *(ISetupPackageReferenceVTable**)SetupPackagePtr;

                            this.Packages.Add(new VisualStudioPackage(package, SetupPackagePtr));

                            package->Release(SetupPackagePtr);
                        }
                    }
                }
            }

            // https://learn.microsoft.com/en-us/visualstudio/install/workload-component-id-vs-community

            this.HasARM64BuildToolsComponents =
                this.Packages.Any(p => p.Id.Equals("Microsoft.VisualStudio.Component.VC.Tools.ARM64", StringComparison.OrdinalIgnoreCase)) &&
                this.Packages.Any(p => p.Id.Equals("Microsoft.VisualStudio.Component.VC.Runtimes.ARM64.Spectre", StringComparison.OrdinalIgnoreCase));
        }

        //private VisualStudioPackage GetLatestSdkPackage()
        //{
        //    var found = this.Packages.FindAll(p =>
        //    {
        //        return p.Id.StartsWith("Microsoft.VisualStudio.Component.Windows10SDK", StringComparison.OrdinalIgnoreCase) ||
        //            p.Id.StartsWith("Microsoft.VisualStudio.Component.Windows11SDK", StringComparison.OrdinalIgnoreCase);
        //    });
        //
        //    if (found == null)
        //        return null;
        //    if (found.Count == 0)
        //        return null;
        //
        //    found.Sort((p1, p2) =>
        //    {
        //        if (Version.TryParse(p1.Version, out Version v1) && Version.TryParse(p2.Version, out Version v2))
        //            return v1.CompareTo(v2);
        //        else
        //            return 1;
        //    });
        //
        //    return found[^1];
        //}
        //
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
        //public string GetWindowsSdkFullVersion()
        //{
        //    VisualStudioPackage package = GetLatestSdkPackage();
        //
        //    if (package == null)
        //        return string.Empty;
        //
        //    return package.Version;
        //}
        //
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

        public override string ToString()
        {
            return this.Name;
        }

        public int CompareTo(object obj)
        {
            if (obj == null)
                return 1;

            if (obj is VisualStudioInstance instance)
                return string.Compare(this.Name, instance.Name, StringComparison.OrdinalIgnoreCase);
            else
                return 1;
        }

        public int CompareTo(VisualStudioInstance obj)
        {
            if (obj == null)
                return 1;

            return string.Compare(this.Name, obj.Name, StringComparison.OrdinalIgnoreCase);
        }
    }

    public unsafe class VisualStudioPackage : IComparable, IComparable<VisualStudioPackage>
    {
        public string Id { get; }
        public string Version { get; }

        public VisualStudioPackage(ISetupPackageReferenceVTable* FromInstance, IntPtr SetupInstancePtr)
        {
            IntPtr IdPtr;
            IntPtr VersionPtr;

            if (FromInstance->GetId(SetupInstancePtr, &IdPtr) == NativeMethods.HRESULT_S_OK && IdPtr != IntPtr.Zero)
            {
                this.Id = Marshal.PtrToStringBSTR(IdPtr);
            }

            if (FromInstance->GetVersion(SetupInstancePtr, &VersionPtr) == NativeMethods.HRESULT_S_OK && VersionPtr != IntPtr.Zero)
            {
                this.Version = Marshal.PtrToStringBSTR(VersionPtr);
            }
        }

        public override string ToString()
        {
            return this.Id;
        }

        public int CompareTo(object obj)
        {
            if (obj == null)
                return 1;

            if (obj is VisualStudioPackage package)
                return string.Compare(this.Id, package.Id, StringComparison.OrdinalIgnoreCase);
            else
                return 1;
        }

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

}
