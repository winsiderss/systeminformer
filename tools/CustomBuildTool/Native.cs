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
    public unsafe static partial class NativeMethods
    {
        public const uint HRESULT_S_OK = 0u;
        public const uint HRESULT_S_FALSE = 1u;

        public static readonly IntPtr HKEY_LOCAL_MACHINE = new IntPtr(0x80000002);
        public static readonly IntPtr HKEY_CURRENT_USER = new IntPtr(0x80000001);
        public static readonly uint KEY_READ = 0x20019u;

        public static readonly uint REG_NONE = 0; // No value type
        public static readonly uint REG_SZ = 1; // Unicode nul terminated string
        public static readonly uint REG_EXPAND_SZ = 2; // Unicode nul terminated string
        public static readonly uint REG_BINARY = 3; // Free form binary
        public static readonly uint REG_DWORD = 4; // 32-bit number
        public static readonly uint REG_MULTI_SZ = 7; // Multiple Unicode strings
        public static readonly uint REG_QWORD = 11; // 64-bit number

        [LibraryImport("advapi32.dll", EntryPoint = "RegOpenKeyExW", StringMarshalling = StringMarshalling.Utf16)]
        public static partial uint RegOpenKeyEx(IntPtr RootKeyHandle, string KeyName, uint Options, uint AccessMask, IntPtr* KeyHandle);

        [LibraryImport("advapi32.dll", EntryPoint = "RegQueryValueExW", StringMarshalling = StringMarshalling.Utf16)]
        public static partial uint RegQueryValueEx(IntPtr KeyHandle, string ValueName, IntPtr Reserved, int* DataType, void* DataBuffer, int* DataLength);

        [LibraryImport("advapi32.dll", EntryPoint = "RegSetKeyValueW", StringMarshalling = StringMarshalling.Utf16)]
        public static partial uint RegSetKeyValue(IntPtr KeyHandle, string SubKeyName, string ValueName, uint DataType, IntPtr DataBuffer, int DataLength);

        [LibraryImport("advapi32.dll", EntryPoint = "RegCloseKey")]
        public static partial uint RegCloseKey(IntPtr KeyHandle);

        [LibraryImport("oleaut32.dll", EntryPoint = "SafeArrayGetLBound")]
        public static partial uint SafeArrayGetLBound(IntPtr psa, uint nDim, out uint lLBound);

        [LibraryImport("oleaut32.dll", EntryPoint = "SafeArrayGetUBound")]
        public static partial uint SafeArrayGetUBound(IntPtr psa, uint nDim, out uint lUBound);

        [LibraryImport("oleaut32.dll", EntryPoint = "SafeArrayGetElement")]
        public static partial uint SafeArrayGetElement(IntPtr psa, uint* rgIndices, IntPtr* pv);

        public const uint GetFileExInfoStandard = 0;

        [StructLayout(LayoutKind.Sequential)]
        public struct FILETIME
        {
            public int dwLowDateTime;
            public int dwHighDateTime;
        }

        [StructLayout(LayoutKind.Sequential)]
        public struct WIN32_FILE_ATTRIBUTE_DATA
        {
            internal uint FileAttributes;
            internal FILETIME CreationTime;
            internal FILETIME LastAccessTime;
            internal FILETIME LastWriteTime;
            internal uint FileSizeHigh;
            internal uint FileSizeLow;
        }

        [LibraryImport("kernel32.dll", EntryPoint = "GetFileAttributesExW", StringMarshalling = StringMarshalling.Utf16)]
        [return: MarshalAs(UnmanagedType.Bool)]
        public static partial bool GetFileAttributesEx(string FileName, uint InfoLevelId, out WIN32_FILE_ATTRIBUTE_DATA FileInformation);

        [LibraryImport("kernel32.dll", EntryPoint = "GetErrorMode")]
        public static partial uint GetErrorMode();

        public const uint SEM_NOGPFAULTERRORBOX = 0x0002;
        public const uint SEM_NOOPENFILEERRORBOX = 0x8000;

        [LibraryImport("kernel32.dll", EntryPoint = "SetErrorMode")]
        public static partial uint SetErrorMode(uint uMode);
    }

    [StructLayout(LayoutKind.Sequential)]
    public readonly unsafe struct ISetupHelperVTable
    {
        public readonly delegate* unmanaged[Stdcall]<IntPtr, Guid, IntPtr*, uint> QueryInterface;
        public readonly delegate* unmanaged[Stdcall]<IntPtr, uint> AddRef;
        public readonly delegate* unmanaged[Stdcall]<IntPtr, uint> Release;
        // ISetupHelper
        public readonly delegate* unmanaged[Stdcall]<IntPtr, IntPtr, uint*, uint> ParseVersion;
        public readonly delegate* unmanaged[Stdcall]<IntPtr, IntPtr, uint*, uint*, uint> ParseVersionRange;
    }

    [StructLayout(LayoutKind.Sequential)]
    public readonly unsafe struct ISetupConfigurationVTable
    {
        public readonly delegate* unmanaged[Stdcall]<IntPtr, Guid, IntPtr*, uint> QueryInterface;
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
    public readonly unsafe struct ISetupConfiguration2VTable
    {
        public readonly delegate* unmanaged[Stdcall]<IntPtr, Guid, IntPtr*, uint> QueryInterface;
        public readonly delegate* unmanaged[Stdcall]<IntPtr, uint> AddRef;
        public readonly delegate* unmanaged[Stdcall]<IntPtr, uint> Release;
        // STDMETHOD(EnumAllInstances)(_Out_ IEnumSetupInstances **ppEnumInstances) = 0;
        public readonly delegate* unmanaged[Stdcall]<IntPtr, IntPtr*, uint> EnumAllInstances;
    }

    [StructLayout(LayoutKind.Sequential)]
    public readonly unsafe struct IEnumSetupInstancesVTable
    {
        public readonly delegate* unmanaged[Stdcall]<IntPtr, Guid, IntPtr*, uint> QueryInterface;
        public readonly delegate* unmanaged[Stdcall]<IntPtr, uint> AddRef;
        public readonly delegate* unmanaged[Stdcall]<IntPtr, uint> Release;
        // STDMETHOD(Next)(_In_ ULONG celt, _Out_writes_to_(celt, *pceltFetched) ISetupInstance** rgelt, _Out_opt_ _Deref_out_range_(0, celt) ULONG* pceltFetched) = 0;
        public readonly delegate* unmanaged[Stdcall]<IntPtr, uint, IntPtr*, uint*, uint> Next;
        // STDMETHOD(Skip)(_In_ ULONG celt) = 0;
        public readonly delegate* unmanaged[Stdcall]<IntPtr, uint, uint> Skip;
        // STDMETHOD(Reset)(void) = 0;
        public readonly delegate* unmanaged[Stdcall]<IntPtr, uint> Reset;
        // STDMETHOD(Clone)(_Deref_out_opt_ IEnumSetupInstances **ppenum) = 0;
        public readonly delegate* unmanaged[Stdcall]<IntPtr, IntPtr*, uint> Clone;
    }

    [StructLayout(LayoutKind.Sequential)]
    public readonly unsafe struct ISetupInstanceVTable
    {
        public readonly delegate* unmanaged[Stdcall]<IntPtr, Guid, IntPtr*, uint> QueryInterface;
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
    public readonly unsafe struct ISetupInstance2VTable
    {
        public readonly delegate* unmanaged[Stdcall]<IntPtr, Guid, IntPtr*, uint> QueryInterface;
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
        // ISetupInstance2
        // STDMETHOD(GetState)(_Out_ InstanceState *pState) = 0;
        public readonly delegate* unmanaged[Stdcall]<IntPtr, uint*, uint> GetState;
        // STDMETHOD(GetPackages)(_Out_ LPSAFEARRAY *ppsaPackages) = 0;
        public readonly delegate* unmanaged[Stdcall]<IntPtr, IntPtr*, uint> GetPackages;
        // STDMETHOD(GetProduct)(_Outptr_result_maybenull_ ISetupPackageReference **ppPackage) = 0;
        public readonly delegate* unmanaged[Stdcall]<IntPtr, IntPtr*, uint> GetProduct;
        // STDMETHOD(GetProductPath)(_Outptr_result_maybenull_ BSTR *pbstrProductPath) = 0;
        public readonly delegate* unmanaged[Stdcall]<IntPtr, IntPtr*, uint> GetProductPath;
        // STDMETHOD(GetErrors)(_Outptr_result_maybenull_ ISetupErrorState** ppErrorState) = 0;
        public readonly delegate* unmanaged[Stdcall]<IntPtr, IntPtr*, uint> GetErrors;
        // STDMETHOD(IsLaunchable)(_Out_ VARIANT_BOOL* pfIsLaunchable) = 0;
        public readonly delegate* unmanaged[Stdcall]<IntPtr, short*, uint> IsLaunchable;
        // STDMETHOD(IsComplete)(_Out_ VARIANT_BOOL* pfIsComplete) = 0;
        public readonly delegate* unmanaged[Stdcall]<IntPtr, short*, uint> IsComplete;
        // STDMETHOD(GetProperties)(_Outptr_result_maybenull_ ISetupPropertyStore** ppProperties) = 0;
        public readonly delegate* unmanaged[Stdcall]<IntPtr, IntPtr*, uint> GetProperties;
        // STDMETHOD(GetEnginePath)(_Outptr_result_maybenull_ BSTR* pbstrEnginePath) = 0;
        public readonly delegate* unmanaged[Stdcall]<IntPtr, IntPtr*, uint> GetEnginePath;
    }

    [StructLayout(LayoutKind.Sequential)]
    public readonly unsafe struct ISetupPackageReferenceVTable
    {
        public readonly delegate* unmanaged[Stdcall]<IntPtr, Guid, IntPtr*, uint> QueryInterface;
        public readonly delegate* unmanaged[Stdcall]<IntPtr, uint> AddRef;
        public readonly delegate* unmanaged[Stdcall]<IntPtr, uint> Release;
        // ISetupPackageReference
        // STDMETHOD(GetId)(_Out_ BSTR* pbstrId) = 0;
        public readonly delegate* unmanaged[Stdcall]<IntPtr, IntPtr*, uint> GetId;
        // STDMETHOD(GetVersion)(_Out_ BSTR* pbstrVersion) = 0;
        public readonly delegate* unmanaged[Stdcall]<IntPtr, IntPtr*, uint> GetVersion;
        // STDMETHOD(GetChip)(_Out_ BSTR* pbstrChip) = 0;
        public readonly delegate* unmanaged[Stdcall]<IntPtr, IntPtr*, uint> GetChip;
        // STDMETHOD(GetLanguage)(_Out_ BSTR* pbstrLanguage) = 0;
        public readonly delegate* unmanaged[Stdcall]<IntPtr, IntPtr*, uint> GetLanguage;
        // STDMETHOD(GetBranch)(_Out_ BSTR* pbstrBranch) = 0;
        public readonly delegate* unmanaged[Stdcall]<IntPtr, IntPtr*, uint> GetBranch;
        // STDMETHOD(GetType)(_Out_ BSTR* pbstrType) = 0;
        public readonly delegate* unmanaged[Stdcall]<IntPtr, IntPtr*, uint> GetPackageType;
        // STDMETHOD(GetUniqueId)(_Out_ BSTR* pbstrUniqueId) = 0;
        public readonly delegate* unmanaged[Stdcall]<IntPtr, IntPtr*, uint> GetUniqueId;
        // STDMETHOD(GetIsExtension)(_Out_ BSTR* pfIsExtension) = 0;
        public readonly delegate* unmanaged[Stdcall]<IntPtr, ushort*, uint> GetIsExtension;
    }
}
