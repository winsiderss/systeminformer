using System;
using System.Collections.Generic;
using System.Drawing;
using System.Runtime.InteropServices;

namespace ProcessHacker2.Api
{
    public struct GeneralGetHighlightingColorArgs
    {
        public IntPtr Parameter;
        public Color BackColor;
        public bool Handled;
        public bool Cache;
    }

    public delegate void GeneralGetHighlightingColorDelegate(PluginInstance instance, ref GeneralGetHighlightingColorArgs args);

    public delegate void SimplePluginDelegate(PluginInstance instance);

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
                info.DisplayName = (void*)Marshal.StringToHGlobalUni(displayName);
            if (author != null)
                info.Author = (void*)Marshal.StringToHGlobalUni(author);
            if (description != null)
                info.Description = (void*)Marshal.StringToHGlobalUni(description);

            info.HasOptions = hasOptions ? (byte)1 : (byte)0;

            plugin = NativeApi.PhRegisterPlugin(name, IntPtr.Zero, &info);

            if (plugin != null)
            {
                return new PluginInstance(plugin);
            }
            else
            {
                if (info.DisplayName != null)
                    Marshal.FreeHGlobal((IntPtr)info.DisplayName);
                if (info.Author != null)
                    Marshal.FreeHGlobal((IntPtr)info.Author);
                if (info.Description != null)
                    Marshal.FreeHGlobal((IntPtr)info.Description);

                return null;
            }
        }

        private PluginInstance(PhPlugin* plugin)
        {
            _plugin = plugin;
        }

        public void RegisterGetProcessHighlightingColorHandler(GeneralGetHighlightingColorDelegate handler)
        {
            CallbackRegistration registration;

            registration = new CallbackRegistration(
                NativeApi.PhGetGeneralCallback(PhGeneralCallback.GetProcessHighlightingColor),
                (parameter, context) =>
                {
                    PhPluginGetHighlightingColor* pargs = (PhPluginGetHighlightingColor*)parameter;
                    GeneralGetHighlightingColorArgs args;

                    args.Parameter = (IntPtr)pargs->Parameter;
                    args.BackColor = ColorTranslator.FromWin32(pargs->BackColor);
                    args.Handled = pargs->Handled != 0;
                    args.Cache = pargs->Cache != 0;

                    handler(this, ref args);

                    pargs->BackColor = ColorTranslator.ToWin32(args.BackColor);
                    pargs->Handled = (byte)(args.Handled ? 1 : 0);
                    pargs->Cache = (byte)(args.Cache ? 1 : 0);
                }
                );

            _registrations.Add(registration);
        }

        public void RegisterLoadHandler(SimplePluginDelegate handler)
        {
            CallbackRegistration registration;

            registration = new CallbackRegistration(
                NativeApi.PhGetPluginCallback(_plugin, PhPluginCallback.Load),
                (parameter, context) => handler(this)
                );

            _registrations.Add(registration);
        }

        public void RegisterMainWindowShowingHandler(SimplePluginDelegate handler)
        {
            CallbackRegistration registration;

            registration = new CallbackRegistration(
                NativeApi.PhGetGeneralCallback(PhGeneralCallback.MainWindowShowing),
                (parameter, context) => handler(this)
                );

            _registrations.Add(registration);
        }

        public void RegisterProcessesUpdatedHandler(SimplePluginDelegate handler)
        {
            CallbackRegistration registration;

            registration = new CallbackRegistration(
                NativeApi.PhGetGeneralCallback(PhGeneralCallback.ProcessesUpdated),
                (parameter, context) => handler(this)
                );

            _registrations.Add(registration);
        }
    }
}
