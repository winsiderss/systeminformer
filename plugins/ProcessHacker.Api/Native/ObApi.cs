using System;
using System.Runtime.InteropServices;

namespace ProcessHacker.Api
{
    [StructLayout(LayoutKind.Sequential)]
    public unsafe struct ObjectBasicInformation
    {
        public int Attributes;
        public int GrantedAccess;
        public int HandleCount;
        public int PointerCount;
        public int PagedPoolCharge;
        public int NonPagedPoolCharge;
        public fixed int Reserved[3];
        public int NameInfoSize;
        public int TypeInfoSize;
        public int SecurityDescriptorSize;
        public long CreationTime;
    }

    public unsafe static partial class NativeApi
    {
        [DllImport("ntdll.dll")]
        public static extern NtStatus NtWaitForSingleObject(
            IntPtr Handle,
            byte Alertable,
            [Optional] long* Timeout
            );

        [DllImport("ntdll.dll")]
        public static extern NtStatus NtWaitForMultipleObjects(
            int Count,
            IntPtr* Handles,
            WaitType WaitType,
            byte Alertable,
            [Optional] long* Timeout
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
