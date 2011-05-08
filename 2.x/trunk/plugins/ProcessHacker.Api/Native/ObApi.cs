/*
 * Process Hacker - 
 *   NT Objects
 * 
 * Copyright (C) 2011 wj32
 * Copyright (C) 2011 dmex
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
            [In] IntPtr Handle,
            [In] SecurityInformation SecurityInformation,
            [In, Out, Optional] void* SecurityDescriptor,
            [In] int Length,
            [In, Out] int* LengthNeeded
            );

        [DllImport("ntdll.dll")]
        public static extern NtStatus NtClose(
            [In] IntPtr Handle
            );
    }
}
