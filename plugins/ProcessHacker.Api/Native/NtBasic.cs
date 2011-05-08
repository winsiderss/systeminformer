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

    [StructLayout(LayoutKind.Sequential)]
    public unsafe struct UnicodeString
    {
        public ushort Length;
        public ushort MaximumLength;
        public void* Buffer;
    }

    public enum WaitType
    {
        All,
        Any
    }
}
