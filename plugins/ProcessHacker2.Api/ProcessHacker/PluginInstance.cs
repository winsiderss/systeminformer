using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;

namespace ProcessHacker2.Api
{
    public delegate void PluginLoadDelegate(PluginInstance instance);

    public unsafe class PluginInstance
    {
        private PhPlugin* _plugin;
        private List<CallbackRegistration> _registrations = new List<CallbackRegistration>();

        public static PluginInstance Register(string name, string displayName, string author, string description, bool hasOptions)
        {
            PhPlugin* plugin;
            PhPluginInformation info;

            info = new PhPluginInformation();

            if (displayName != null)
                info.DisplayName = Marshal.StringToHGlobalUni(displayName);
            if (author != null)
                info.Author = Marshal.StringToHGlobalUni(author);
            if (description != null)
                info.Description = Marshal.StringToHGlobalUni(description);

            info.HasOptions = hasOptions ? (byte)1 : (byte)0;

            plugin = NativeApi.PhRegisterPlugin(name, IntPtr.Zero, &info);

            if (plugin != null)
            {
                return new PluginInstance(plugin);
            }
            else
            {
                if (info.DisplayName != IntPtr.Zero)
                    Marshal.FreeHGlobal(info.DisplayName);
                if (info.Author != IntPtr.Zero)
                    Marshal.FreeHGlobal(info.Author);
                if (info.Description != IntPtr.Zero)
                    Marshal.FreeHGlobal(info.Description);

                return null;
            }
        }

        private PluginInstance(PhPlugin* plugin)
        {
            _plugin = plugin;
        }

        public void RegisterLoadHandler(PluginLoadDelegate handler)
        {
            CallbackRegistration registration;

            registration = new CallbackRegistration(
                NativeApi.PhGetPluginCallback(_plugin, PhPluginCallback.Load),
                (parameter, context) => handler(this)
                );

            _registrations.Add(registration);
        }
    }
}
