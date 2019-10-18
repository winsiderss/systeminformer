/*
 * Process Hacker Toolchain - 
 *   Build script
 * 
 * Copyright (C) 2017 dmex
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
using System.Diagnostics;
using System.IO;
using System.Runtime.InteropServices;

namespace CustomBuildTool
{
    [ComImport, Guid("177F0C4A-1CD3-4DE7-A32C-71DBBB9FA36D")]
    public class SetupConfigurationClass
    {

    }

    [ComImport, CoClass(typeof(SetupConfigurationClass)), Guid("42843719-DB4C-46C2-8E7C-64F1816EFD5B")]
    public interface SetupConfiguration : ISetupConfiguration2, ISetupConfiguration
    {

    }

    [ComImport, Guid("6380BCFF-41D3-4B2E-8B2E-BF8A6810C848"), InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
    public interface IEnumSetupInstances
    {
        void Next(
            [MarshalAs(UnmanagedType.U4)] [In] int celt,
            [MarshalAs(UnmanagedType.LPArray, ArraySubType = UnmanagedType.Interface)] [Out] ISetupInstance[] rgelt,
            [MarshalAs(UnmanagedType.U4)] out int pceltFetched
            );

        void Skip([MarshalAs(UnmanagedType.U4)] [In] int celt);
        void Reset();
        IEnumSetupInstances Clone();
    }

    [ComImport, Guid("42843719-DB4C-46C2-8E7C-64F1816EFD5B"), InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
    public interface ISetupConfiguration
    {
        IEnumSetupInstances EnumInstances();
        ISetupInstance GetInstanceForCurrentProcess();
        ISetupInstance GetInstanceForPath([In] string path);
    }

    [ComImport, Guid("26AAB78C-4A60-49D6-AF3B-3C35BC93365D"), InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
    public interface ISetupConfiguration2 : ISetupConfiguration
    {
        IEnumSetupInstances EnumAllInstances();
    }

    [ComImport, Guid("46DCCD94-A287-476A-851E-DFBC2FFDBC20"), InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
    public interface ISetupErrorState
    {
        ISetupFailedPackageReference[] GetFailedPackages();
        ISetupPackageReference[] GetSkippedPackages();
    }

    [ComImport, Guid("E73559CD-7003-4022-B134-27DC650B280F"), InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
    public interface ISetupFailedPackageReference : ISetupPackageReference
    {
        new string GetId();
        new string GetVersion();
        new string GetChip();
        new string GetLanguage();
        new string GetBranch();
        new string GetType();
        new string GetUniqueId();
        new bool GetIsExtension();
    }

    [ComImport, Guid("42B21B78-6192-463E-87BF-D577838F1D5C"), InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
    public interface ISetupHelper
    {
        /// <summary>
        /// Parses a dotted quad version string into a 64-bit unsigned integer.
        /// </summary>
        /// <param name="version">The dotted quad version string to parse, e.g. 1.2.3.4.</param>
        /// <returns>A 64-bit unsigned integer representing the version. You can compare this to other versions.</returns>
        ulong ParseVersion([In] string version);

        /// <summary>
        /// Parses a dotted quad version string into a 64-bit unsigned integer.
        /// </summary>
        /// <param name="versionRange">The string containing 1 or 2 dotted quad version strings to parse, e.g. [1.0,) that means 1.0.0.0 or newer.</param>
        /// <param name="minVersion">A 64-bit unsigned integer representing the minimum version, which may be 0. You can compare this to other versions.</param>
        /// <param name="maxVersion">A 64-bit unsigned integer representing the maximum version, which may be MAXULONGLONG. You can compare this to other versions.</param>
        void ParseVersionRange([In] string versionRange, out ulong minVersion, out ulong maxVersion);
    }

    [ComImport, Guid("B41463C3-8866-43B5-BC33-2B0676F7F42E"), InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
    public interface ISetupInstance
    {
        string GetInstanceId();
        System.Runtime.InteropServices.ComTypes.FILETIME GetInstallDate();
        string GetInstallationName();
        string GetInstallationPath();
        string GetInstallationVersion();
        string GetDisplayName([In] int lcid = 0);
        string GetDescription([In] int lcid = 0);
        string ResolvePath([In] string pwszRelativePath = null);
    }

    [ComImport, Guid("89143C9A-05AF-49B0-B717-72E218A2185C"), InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
    public interface ISetupInstance2 : ISetupInstance
    {
        new string GetInstanceId();
        new System.Runtime.InteropServices.ComTypes.FILETIME GetInstallDate();
        new string GetInstallationName();
        new string GetInstallationPath();
        new string GetInstallationVersion();
        new string GetDisplayName([MarshalAs(UnmanagedType.U4)] [In] int lcid = 0);
        new string GetDescription([MarshalAs(UnmanagedType.U4)] [In] int lcid = 0);
        new string ResolvePath([MarshalAs(21)] [In] string pwszRelativePath = null);
        InstanceState GetState();
        ISetupPackageReference[] GetPackages();
        ISetupPackageReference GetProduct();
        string GetProductPath();
        ISetupErrorState GetErrors();
        bool IsLaunchable();
        bool IsComplete();
        ISetupPropertyStore GetProperties();
        string GetEnginePath();
    }

    [ComImport, Guid("DA8D8A16-B2B6-4487-A2F1-594CCCCD6BF5"), InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
    public interface ISetupPackageReference
    {
        string GetId();
        string GetVersion();
        string GetChip();
        string GetLanguage();
        string GetBranch();
        string GetType();
        string GetUniqueId();
        bool GetIsExtension();
    }

    [ComImport, Guid("c601c175-a3be-44bc-91f6-4568d230fc83"), InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
    public interface ISetupPropertyStore
    {
        [return: MarshalAs(UnmanagedType.SafeArray, SafeArraySubType = VarEnum.VT_BSTR)]
        string[] GetNames();

        object GetValue([MarshalAs(UnmanagedType.LPWStr)] [In] string pwszName);
    }
}
