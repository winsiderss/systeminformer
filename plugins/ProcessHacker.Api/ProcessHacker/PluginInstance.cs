/*
 * Process Hacker - 
 *   Plugin API
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
        private static readonly List<CallbackRegistration> _registrations = new List<CallbackRegistration>();

        public static PluginInstance Register(string name, string displayName, string author, string description, string url, bool hasOptions)
        {
            PhPluginInformation* info;

            PhPlugin* plugin = NativeApi.PhRegisterPlugin(name, IntPtr.Zero, &info);

            if (plugin != null)
            {
                info->DisplayName = (void*)NativeApi.StringToNativeUni(displayName);
                info->Author = (void*)NativeApi.StringToNativeUni(author);
                info->Description = (void*)NativeApi.StringToNativeUni(description);
                info->Url = (void*)NativeApi.StringToNativeUni(url);
                info->HasOptions = hasOptions;

                return new PluginInstance(plugin);
            }

            return null;
        }

        public PhPlugin* Plugin { get; private set; }

        private PluginInstance(PhPlugin* plugin)
        {
            this.Plugin = plugin;
        }

        public void RegisterGetProcessHighlightingColorHandler(GeneralGetHighlightingColorDelegate handler)
        {
            if (handler == null)
                throw new InvalidOperationException("Delegate handler can not be null.");

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
                throw new InvalidOperationException("Delegate handler can not be null.");

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
                throw new InvalidOperationException("Delegate handler can not be null.");

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
                throw new InvalidOperationException("Delegate handler can not be null.");

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
                throw new InvalidOperationException("Delegate handler can not be null.");

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
                throw new InvalidOperationException("Delegate handler can not be null.");

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
                throw new InvalidOperationException("Delegate handler can not be null.");

            CallbackRegistration registration = new CallbackRegistration(
                NativeApi.PhGetPluginCallback(this.Plugin, PhPluginCallback.MenuItem),
                (parameter, context) => handler(this, parameter, context)
                );

            _registrations.Add(registration);
        }
    }
}
