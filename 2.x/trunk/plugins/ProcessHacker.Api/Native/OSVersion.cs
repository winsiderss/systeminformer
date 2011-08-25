/*
 * Process Hacker -
 *   operating system version information
 *
 * Copyright (C) 2011 wj32
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

using ProcessHacker.Api;

namespace System
{
    public enum OSArch
    {
        Unknown,
        I386,
        Amd64
    }

    public enum WindowsVersion
    {
        /// <summary>
        /// Windows XP.
        /// </summary>
        XP = 51,

        /// <summary>
        /// Windows Server 2003.
        /// </summary>
        Server2003 = 52,

        /// <summary>
        /// Windows Vista, Windows Server 2008.
        /// </summary>
        Vista = 60,

        /// <summary>
        /// Windows 7, Windows Server 2008 R2.
        /// </summary>
        Seven = 61,

        /// <summary>
        /// An unknown version of Windows.
        /// </summary>
        Unknown = 0
    }

    public static class OSVersion
    {
        public static OSArch Architecture { get; private set; }
        public static WindowsVersion WindowsVersion { get; private set; }
        public static int PlatformSize { get; private set; }
        public static string VersionString { get; private set; }
        public static int Bits { get; private set; }
        public static bool HasExtendedTaskbar { get; private set; }
        public static bool HasTaskDialogs { get; private set; }
        public static bool HasThemes { get; private set; }
        public static bool HasUac { get; private set; }

        static OSVersion()
        {
            WindowsVersion = (WindowsVersion)PluginGlobal.PhWindowsVersion;

            if (IsAboveOrEqual(WindowsVersion.XP))
            {
                HasThemes = true;
            }

            if (IsAboveOrEqual(WindowsVersion.Vista))
            {
                HasTaskDialogs = true;
                HasUac = true;
            }

            if (IsAboveOrEqual(WindowsVersion.Seven))
            {
                HasExtendedTaskbar = true;
            }

            VersionString = Environment.OSVersion.VersionString;

            PlatformSize = IntPtr.Size;
            Bits = PlatformSize * 8;
            Architecture = PlatformSize == 4 ? OSArch.I386 : OSArch.Amd64;
        }

        public static string BitsString
        {
            get { return Bits + "-" + "bit"; }
        }

        public static bool IsAmd64
        {
            get { return Architecture == OSArch.Amd64; }
        }

        public static bool IsI386
        {
            get { return Architecture == OSArch.I386; }
        }

        public static bool IsAbove(WindowsVersion version)
        {
            return WindowsVersion > version;
        }

        public static bool IsAboveOrEqual(WindowsVersion version)
        {
            return WindowsVersion >= version;
        }

        public static bool IsBelowOrEqual(WindowsVersion version)
        {
            return WindowsVersion <= version;
        }

        public static bool IsBelow(WindowsVersion version)
        {
            return WindowsVersion < version;
        }

        public static bool IsEqualTo(WindowsVersion version)
        {
            return WindowsVersion == version;
        }
    }
}
