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
        private static Font _phApplicationFont;
        private static TOKEN_ELEVATION_TYPE? _phElevationType;
        private static IntPtr? _phHeapHandle;
        private static IntPtr? _phMainWindowHandle;
        private static int? _phWindowsVersion;
        private static int? _phCurrentSessionId;
        private static bool? _phElevated;

        public static Font PhApplicationFont
        {
            get
            {
                if (_phApplicationFont == null)
                {
                    void* proc = NativeApi.GetProcAddress(IntPtr.Zero, "PhApplicationFont");

                    _phApplicationFont = Font.FromHfont(((IntPtr*)proc)[0]);
                }

                return _phApplicationFont;
            }
        }

        public static int PhCurrentSessionId
        {
            get
            {
                if (!_phCurrentSessionId.HasValue)
                {
                    void* proc = NativeApi.GetProcAddress(IntPtr.Zero, "PhCurrentSessionId");

                    _phCurrentSessionId =((int*)proc)[0];
                }

                return _phCurrentSessionId.Value;
            }
        }

        public static bool PhElevated
        {
            get
            {
                if (!_phElevated.HasValue)
                {
                    void* proc = NativeApi.GetProcAddress(IntPtr.Zero, "PhElevated");

                    _phElevated = ((bool*)proc)[0];
                }

                return _phElevated.Value;
            }
        }

        public static TOKEN_ELEVATION_TYPE PhElevationType
        {
            get
            {
                if (!_phElevationType.HasValue)
                {
                    void* proc = NativeApi.GetProcAddress(IntPtr.Zero, "PhElevationType");

                    _phElevationType = ((TOKEN_ELEVATION_TYPE*)proc)[0];
                }

                return _phElevationType.Value;
            }
        }

        public static IntPtr PhHeapHandle
        {
            get
            {
                if (!_phHeapHandle.HasValue)
                {
                    void* proc = NativeApi.GetProcAddress(IntPtr.Zero, "PhHeapHandle");

                    _phHeapHandle = ((IntPtr*)proc)[0];
                }

                return _phHeapHandle.Value;
            }
        }

        public static IntPtr PhMainWindowHandle
        {
            get
            {
                if (!_phMainWindowHandle.HasValue)
                {
                    void* proc = NativeApi.GetProcAddress(IntPtr.Zero, "PhMainWndHandle");

                    _phMainWindowHandle = ((IntPtr*)proc)[0];
                }

                return _phMainWindowHandle.Value;
            }
        }

        public static int PhWindowsVersion
        {
            get
            {
                if (!_phWindowsVersion.HasValue)
                {
                    void* proc = NativeApi.GetProcAddress(IntPtr.Zero, "WindowsVersion");

                    _phWindowsVersion = ((int*)proc)[0];
                }

                return _phWindowsVersion.Value;
            }
        }
    }
}