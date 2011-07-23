/*
 * Process Hacker - 
 *   Callback API
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
    public unsafe class CallbackRegistration : IDisposable
    {
        private PhCallbackFunction _function;
        private PhCallback* _callback;
        private PhCallbackRegistration* _registration;

        public CallbackRegistration(PhCallback* callback, PhCallbackFunction function)
        {
            _function = function;
            _callback = callback;

            _registration = (PhCallbackRegistration*)Marshal.AllocHGlobal(Marshal.SizeOf(typeof(PhCallbackRegistration)));

            NativeApi.PhRegisterCallback(_callback, function, IntPtr.Zero, _registration);
        }

        public void Dispose()
        {
            if (_registration != null)
            {
                NativeApi.PhUnregisterCallback(_callback, _registration);

                Marshal.FreeHGlobal((IntPtr)_registration);

                _registration = null;
            }
        }
    }
}
