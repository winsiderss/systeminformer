/*
 * Process Hacker -
 *   NT Basic definitions
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
    public unsafe struct AnsiString
    {
        public ushort Length;
        public ushort MaximumLength;
        public void* Buffer;
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct ClientId
    {
        public IntPtr UniqueProcess;
        public IntPtr UniqueThread;
    }

    [StructLayout(LayoutKind.Sequential)]
    public unsafe struct ObjectAttributes
    {
        public int Length;
        public IntPtr RootDirectory;
        public UnicodeString* ObjectName;
        public int Attributes;
        public void* SecurityDescriptor;
        public void* SecurityQualityOfService;
    }

    public enum ObjectFlags
    {
        Inherit = 0x2,
        Permanent = 0x10,
        Exclusive = 0x20,
        CaseInsensitive = 0x40,
        OpenIf = 0x80,
        OpenLink = 0x100,
        KernelHandle = 0x200,
        ForceAccessCheck = 0x400
    }

    [Flags]
    public enum HeapFlags : uint
    {
        NoSerialize = 0x00000001,
        Growable = 0x00000002,
        GenerateExceptions = 0x00000004,
        ZeroMemory = 0x00000008,
        ReallocInPlaceOnly = 0x00000010,
        TailCheckingEnabled = 0x00000020,
        FreeCheckingEnabled = 0x00000040,
        DisableCoalesceOnFree = 0x00000080,

        CreateAlign16 = 0x00010000,
        CreateEnableTracing = 0x00020000,
        CreateEnableExecute = 0x00040000,

        SettableUserValue = 0x00000100,
        SettableUserFlag1 = 0x00000200,
        SettableUserFlag2 = 0x00000400,
        SettableUserFlag3 = 0x00000800,
        SettableUserFlags = 0x00000e00,

        Class0 = 0x00000000, // Process heap
        Class1 = 0x00001000, // Private heap
        Class2 = 0x00002000, // Kernel heap
        Class3 = 0x00003000, // GDI heap
        Class4 = 0x00004000, // User heap
        Class5 = 0x00005000, // Console heap
        Class6 = 0x00006000, // User desktop heap
        Class7 = 0x00007000, // CSRSS shared heap
        Class8 = 0x00008000, // CSR port heap
        ClassMask = 0x0000f000
    }

    [StructLayout(LayoutKind.Sequential)]
    public unsafe struct UnicodeString
    {
        public static readonly int SizeOf;

        public ushort Length;
        public ushort MaximumLength;
        public void* Buffer;

        static UnicodeString()
        {
            SizeOf = Marshal.SizeOf(typeof(UnicodeString));
        }
    }

    public enum WaitType
    {
        All,
        Any
    }
}
