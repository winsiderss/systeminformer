using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using System.Drawing;

namespace ProcessHacker.Api
{
    public delegate void SimplePluginDelegate(PluginInstance instance);
    public delegate void GeneralGetHighlightingColorDelegate(PluginInstance instance, ref GeneralGetHighlightingColorArgs args);
    public delegate void PluginParamsCallbackDelegate(PluginInstance instance, IntPtr Parameter, IntPtr Context);

    public unsafe class PluginInstance
    {
        public static PluginInstance Register(string name, string displayName, string author, string description, string url, bool hasOptions)
        {
            PhPluginInformation* info;

            PhPlugin* plugin = NativeApi.PhRegisterPlugin(name, IntPtr.Zero, &info);

            if (plugin != null)
            {
                info->DisplayName = (void*)Marshal.StringToHGlobalUni(displayName);
                info->Author = (void*)Marshal.StringToHGlobalUni(author);
                info->Description = (void*)Marshal.StringToHGlobalUni(description);
                info->Url = (void*)Marshal.StringToHGlobalUni(url);
                info->HasOptions = hasOptions;

                return new PluginInstance(plugin);
            }

            return null;
        }

        public PhPlugin* Plugin { get; private set; }
        private List<CallbackRegistration> _registrations = new List<CallbackRegistration>();

        private PluginInstance(PhPlugin* plugin)
        {
            this.Plugin = plugin;
        }

        public void RegisterGetProcessHighlightingColorHandler(GeneralGetHighlightingColorDelegate handler)
        {
            if (handler == null)
                throw new InvalidOperationException("Delegate handler can not be null");

            CallbackRegistration registration = new CallbackRegistration(
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

        /// <summary>
        /// Registers a callback for Plugin load notifications.
        /// </summary>
        /// <param name="handler">The delegate to invoke.</param>
        public void RegisterLoadHandler(SimplePluginDelegate handler)
        {
            if (handler == null)
                throw new InvalidOperationException("Delegate handler can not be null");

            CallbackRegistration registration = new CallbackRegistration(
                NativeApi.PhGetPluginCallback(this.Plugin, PhPluginCallback.Load),
                (parameter, context) => handler(this)
                );

            _registrations.Add(registration);
        }

        /// <summary>
        /// Registers a callback for Plugin unload notifications.
        /// </summary>
        /// <param name="handler">The delegate to invoke.</param>
        public void RegisterUnLoadHandler(SimplePluginDelegate handler)
        {
            if (handler == null)
                throw new InvalidOperationException("Delegate handler can not be null");

            CallbackRegistration registration = new CallbackRegistration(
                NativeApi.PhGetPluginCallback(this.Plugin, PhPluginCallback.Unload),
                (parameter, context) => handler(this)
                );

            _registrations.Add(registration);
        }

        /// <summary>
        /// Registers a callback for a (one-time) Main Window showing notification.
        /// </summary>
        /// <param name="handler">The delegate to invoke.</param>
        public void RegisterMainWindowShowingHandler(SimplePluginDelegate handler)
        {
            if (handler == null)
                throw new InvalidOperationException("Delegate handler can not be null");

            CallbackRegistration registration = new CallbackRegistration(
                NativeApi.PhGetGeneralCallback(PhGeneralCallback.MainWindowShowing),
                (parameter, context) => handler(this)
                );

            _registrations.Add(registration);
        }

        /// <summary>
        /// Registers a callback for the Options button, found on the plugin Options Window.
        /// </summary>
        /// <remarks>
        /// Registering a Options Window handler enables the options button, 
        /// Allowing the user to easily configure your plugin.
        /// </remarks>
        /// <param name="handler">The delegate to invoke.</param>
        public void RegisterOptionsWindowHandler(PluginParamsCallbackDelegate handler)
        {
            if (handler == null)
                throw new InvalidOperationException("Delegate handler can not be null");

            CallbackRegistration registration = new CallbackRegistration(
                NativeApi.PhGetPluginCallback(this.Plugin, PhPluginCallback.ShowOptions),
                (parameter, context) => handler(this, parameter, context)
                );

            _registrations.Add(registration);
        }

        /// <summary>
        /// Registers a callback for interval update notifications.
        /// </summary>
        /// <remarks>
        /// Registering for Interval udpates allows you to use the interval settings
        /// set by the user to update your display if required.
        /// </remarks>
        /// <param name="handler">The delegate to invoke.</param>
        public void RegisterIntervalUpdateHandler(SimplePluginDelegate handler)
        {
            if (handler == null)
                throw new InvalidOperationException("Delegate handler can not be null");

            CallbackRegistration registration = new CallbackRegistration(
                NativeApi.PhGetGeneralCallback(PhGeneralCallback.IntervalUpdate),
                (parameter, context) => handler(this)
                );

            _registrations.Add(registration);
        }

        /// <summary>
        /// Registers a Menu Item handler on the View menu.
        /// </summary>
        /// <param name="handler">The delegate to invoke.</param>
        public void RegisterMenuItemHandler(PluginParamsCallbackDelegate handler)
        {
            if (handler == null)
                throw new InvalidOperationException("Delegate handler can not be null");

            CallbackRegistration registration = new CallbackRegistration(
                NativeApi.PhGetPluginCallback(this.Plugin, PhPluginCallback.MenuItem),
                (parameter, context) => handler(this, parameter, context)
                );

            _registrations.Add(registration);
        }
    }
}
