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
    internal static unsafe class BuildVisualStudio
    {
        private static readonly List<VisualStudioInstance> VisualStudioInstanceList;
        private static VisualStudioInstance VisualStudioInstance = null;

        static BuildVisualStudio()
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

    internal unsafe class VisualStudioInstance : IComparable, IComparable<VisualStudioInstance>
    {
        public string DisplayName { get; }
        public string Name { get; }
        public string Path { get; }
        public string InstallationVersion { get; }
        //public string InstanceId { get; }
        public uint State { get; }
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

        private VisualStudioPackage GetLatestSdkPackage()
        {
            var found = this.Packages.FindAll(p => p.Id.StartsWith("Microsoft.VisualStudio.Component.Windows10SDK", StringComparison.OrdinalIgnoreCase) ||
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

    internal unsafe class VisualStudioPackage : IComparable, IComparable<VisualStudioPackage>
    {
        public string Id { get; }
        public string Version { get; }

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
