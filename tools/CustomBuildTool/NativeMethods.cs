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
    public static unsafe partial class NativeMethods
    {
        public const uint HRESULT_S_OK = 0u;
        public const uint HRESULT_S_FALSE = 1u;

        public static readonly UIntPtr CURRENT_PROCESS = new UIntPtr(0xffffffffffffffff);
        public static readonly IntPtr CURRENT_TOKEN = new IntPtr(-5);
        public static readonly IntPtr HKEY_LOCAL_MACHINE = new IntPtr(unchecked((int)0x80000002));
        public static readonly IntPtr HKEY_CURRENT_USER = new IntPtr(unchecked((int)0x80000001));
        public static readonly uint KEY_READ = 0x20019u;
        public static readonly uint HIGH_PRIORITY_CLASS = 0x00000080;

        public static readonly uint REG_NONE = 0; // No value type
        public static readonly uint REG_SZ = 1; // Unicode nul terminated string
        public static readonly uint REG_EXPAND_SZ = 2; // Unicode nul terminated string
        public static readonly uint REG_BINARY = 3; // Free form binary
        public static readonly uint REG_DWORD = 4; // 32-bit number
        public static readonly uint REG_MULTI_SZ = 7; // Multiple Unicode strings
        public static readonly uint REG_QWORD = 11; // 64-bit number

        [LibraryImport("userenv.dll", EntryPoint = "CreateEnvironmentBlock", StringMarshalling = StringMarshalling.Utf16)]
        [return: MarshalAs(UnmanagedType.Bool)]
        public static partial bool CreateEnvironmentBlock(out IntPtr lpEnvironment, IntPtr hToken, [MarshalAs(UnmanagedType.Bool)] bool bInherit);

        [LibraryImport("userenv.dll", EntryPoint = "DestroyEnvironmentBlock", StringMarshalling = StringMarshalling.Utf16)]
        [return: MarshalAs(UnmanagedType.Bool)]
        public static partial bool DestroyEnvironmentBlock(IntPtr lpEnvironment);

        [LibraryImport("kernel32.dll")]
        [return: MarshalAs(UnmanagedType.Bool)]
        public static unsafe partial bool GetFileInformationByHandleEx(SafeFileHandle hFile, int FileInformationClass, void* lpFileInformation, uint dwBufferSize);
        [LibraryImport("kernel32.dll")]
        [return: MarshalAs(UnmanagedType.Bool)]
        public static unsafe partial bool SetFileInformationByHandle(SafeFileHandle hFile, int FileInformationClass, void* lpFileInformation, uint dwBufferSize);

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

        [StructLayout(LayoutKind.Sequential)]
        public struct FILETIME
        {
            public int dwLowDateTime;
            public int dwHighDateTime;

            public DateTime DateTime
            {
                get
                {
                    long fileTime = ((long)dwHighDateTime << 32) + dwLowDateTime;

                    return DateTime.FromFileTime(fileTime);
                }
            }

            public long FileTime
            {
                get { return ((long)dwHighDateTime << 32) + dwLowDateTime; }
            }
        }

        [StructLayout(LayoutKind.Sequential)]
        public struct WIN32_FILE_ATTRIBUTE_DATA
        {
            public uint FileAttributes;
            public FILETIME CreationTime;
            public FILETIME LastAccessTime;
            public FILETIME LastWriteTime;
            public uint FileSizeHigh;
            public uint FileSizeLow;
        }

        [StructLayout(LayoutKind.Sequential)]
        public ref struct FILE_BASIC_INFO
        {
            public long CreationTime;
            public long LastAccessTime;
            public long LastWriteTime;
            public long ChangeTime;
            public uint FileAttributes;
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

        [LibraryImport("kernel32.dll", EntryPoint = "ExitProcess")]
        public static partial void ExitProcess(int exitCode);

        [LibraryImport("kernel32.dll", SetLastError = true)]
        [return: MarshalAs(UnmanagedType.Bool)]
        public static partial bool SetPriorityClass(UIntPtr handle, uint priorityClass);
    }
}
