using System;
using System.Runtime.InteropServices;

namespace ProcessHacker.Api
{
    [StructLayout(LayoutKind.Sequential)]
    public unsafe struct ObjectBasicInformation
    {
        public uint Attributes;
        public int GrantedAccess;
        public uint HandleCount;
        public uint PointerCount;
        public uint PagedPoolCharge;
        public uint NonPagedPoolCharge;
        public fixed int Reserved[3];
        public uint NameInfoSize;
        public uint TypeInfoSize;
        public uint SecurityDescriptorSize;
        public ulong CreationTime;
    }

    [System.Security.SuppressUnmanagedCodeSecurity]
    public unsafe static partial class NativeApi
    {
        [DllImport("ntdll.dll")]
        public static extern NtStatus NtWaitForSingleObject(
            [In] IntPtr Handle,
            [In] bool Alertable,
            [In, Optional] ref long Timeout
            );

        [DllImport("ntdll.dll")]
        public static extern NtStatus NtWaitForMultipleObjects(
            [In] uint Count,
            [In] IntPtr[] Handles,
            [In] WaitType WaitType,
            [In, MarshalAs(UnmanagedType.I1)] bool Alertable,
            [In, Out, Optional] ref long Timeout
            );

        [DllImport("ntdll.dll")]
        public static extern NtStatus NtSetSecurityObject(
            IntPtr Handle,
            SecurityInformation SecurityInformation,
            void* SecurityDescriptor
            );

        [DllImport("ntdll.dll")]
        public static extern NtStatus NtQuerySecurityObject(
            IntPtr Handle,
            SecurityInformation SecurityInformation,
            [Optional] void* SecurityDescriptor,
            int Length,
            int* LengthNeeded
            );

        [DllImport("ntdll.dll")]
        public static extern NtStatus NtClose(
            IntPtr Handle
            );
    }
}
