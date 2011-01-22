using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using System.Text;

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
            newValue = value->Read();
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
