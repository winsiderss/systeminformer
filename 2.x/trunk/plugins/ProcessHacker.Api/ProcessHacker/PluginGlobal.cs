/*
 * Process Hacker - 
 *   Settings API
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
    public static unsafe class PluginGlobal
    {
        public struct Setting
        {
            public PhSettingType Type;
            public string Name;
            public string DefaultValue;
        }

        public static void AddSettings(Setting[] settings)
        {
            PhSettingCreate* create = stackalloc PhSettingCreate[settings.Length];

            for (int i = 0; i < settings.Length; i++)
            {
                create[i].Type = settings[i].Type;
                create[i].Name = (void*)Marshal.StringToHGlobalUni(settings[i].Name);
                create[i].DefaultValue = (void*)Marshal.StringToHGlobalUni(settings[i].DefaultValue);
            }

            NativeApi.PhAddSettings(create, settings.Length);

            for (int i = 0; i < settings.Length; i++)
            {
                Marshal.FreeHGlobal((IntPtr)create[i].Name);
                Marshal.FreeHGlobal((IntPtr)create[i].DefaultValue);
            }
        }

        public static int GetIntegerSetting(string name)
        {
            return NativeApi.PhGetIntegerSetting(name);
        }

        public static PhIntegerPair GetIntegerPairSetting(string name)
        {
            return NativeApi.PhGetIntegerPairSetting(name);
        }

        public static string GetStringSetting(string name)
        {
            PhString* value;
            string newValue;

            value = NativeApi.PhGetStringSetting(name);
            newValue = value->Text;
            NativeApi.PhDereferenceObject(value);

            return newValue;
        }

        public static void SetIntegerSetting(string name, int value)
        {
            NativeApi.PhSetIntegerSetting(name, value);
        }

        public static void SetIntegerPairSetting(string name, PhIntegerPair value)
        {
            NativeApi.PhSetIntegerPairSetting(name, value);
        }

        public static void SetStringSetting(string name, string value)
        {
            PhStringRef sr;

            fixed (char* buffer = value)
            {
                sr.Buffer = buffer;
                sr.Length = (ushort)(value.Length * 2);

                NativeApi.PhSetStringSetting2(name, &sr);
            }
        }
    }
}
