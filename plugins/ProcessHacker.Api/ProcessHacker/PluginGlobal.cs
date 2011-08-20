/*
 * Process Hacker - 
 *   Global Plugin Properties
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
using System.Drawing;

namespace ProcessHacker.Api
{
    public enum TOKEN_ELEVATION_TYPE
    {
        TokenElevationTypeDefault = 1,
        TokenElevationTypeFull,
        TokenElevationTypeLimited,
    }

    public static unsafe class PluginGlobal
    {
        public static Font PhApplicationFont
        {
            get
            {
                void* proc = NativeApi.GetProcAddress(IntPtr.Zero, "PhApplicationFont");

                return Font.FromHfont(((IntPtr*)proc)[0]);
            }
        }

        public static int PhCurrentSessionId
        {
            get
            {
                void* proc = NativeApi.GetProcAddress(IntPtr.Zero, "PhCurrentSessionId");

                return ((int*)proc)[0];
            }
        }

        public static bool PhElevated
        {
            get
            {
                void* proc = NativeApi.GetProcAddress(IntPtr.Zero, "PhElevated");

                return ((bool*)proc)[0];
            }
        }

        public static TOKEN_ELEVATION_TYPE PhElevationType
        {
            get
            {
                void* proc = NativeApi.GetProcAddress(IntPtr.Zero, "PhElevationType");

                return ((TOKEN_ELEVATION_TYPE*)proc)[0];
            }
        }

        public static IntPtr PhHeapHandle
        {
            get
            {
                void* proc = NativeApi.GetProcAddress(IntPtr.Zero, "PhHeapHandle");

                return ((IntPtr*)proc)[0];
            }
        }

        public static int PhKphFeatures
        {
            get
            {
                void* proc = NativeApi.GetProcAddress(IntPtr.Zero, "PhKphFeatures");

                return ((int*)proc)[0];
            }
        }

        public static IntPtr PhKphHandle
        {
            get
            {
                void* proc = NativeApi.GetProcAddress(IntPtr.Zero, "PhKphHandle");

                return ((IntPtr*)proc)[0];
            }
        }

        public static IntPtr PhMainWindowHandle
        {
            get
            {
                void* proc = NativeApi.GetProcAddress(IntPtr.Zero, "PhMainWndHandle");

                return ((IntPtr*)proc)[0];
            }
        }

        public static int WindowsVersion
        {
            get
            {
                void* proc = NativeApi.GetProcAddress(IntPtr.Zero, "WindowsVersion");

                return ((int*)proc)[0];
            }
        }
    }
}
