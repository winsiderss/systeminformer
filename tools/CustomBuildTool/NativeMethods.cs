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
    public static class NativeMethods
    {
        [StructLayout(LayoutKind.Explicit)]
        public struct LARGE_INTEGER
        {
            [FieldOffset(0)]
            public uint LowPart;
            [FieldOffset(4)]
            public uint HighPart;
            [FieldOffset(0)]
            public ulong QuadPart;
        }
    }
}
