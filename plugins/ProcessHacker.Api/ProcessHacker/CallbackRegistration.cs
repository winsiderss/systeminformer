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
using ProcessHacker.Native;
using System.Runtime.InteropServices;

namespace ProcessHacker.Api
{
    public unsafe class CallbackRegistration : IDisposable
    {
        private readonly PhCallbackFunction CallbackFunction;
        private readonly PhCallback* Callback;
        private IntPtr CallbackAlloc;
        private PhCallbackRegistration* RegistrationCallback;

        public CallbackRegistration(PhCallback* callback, PhCallbackFunction function)
        {
            this.Callback = callback;
            this.CallbackFunction = function;
            this.CallbackAlloc = MemoryAlloc.PrivateHeap.Allocate(PhCallbackRegistration.SizeOf);

            NativeApi.PhRegisterCallback(this.Callback, this.CallbackFunction, IntPtr.Zero, this.CallbackAlloc);

            this.RegistrationCallback = (PhCallbackRegistration*)this.CallbackAlloc;
        }

        public void Dispose()
        {
            if (this.CallbackAlloc != null && this.RegistrationCallback != null)
            {
                // Unregister the callback.
                NativeApi.PhUnregisterCallback(this.Callback, this.RegistrationCallback);
                
                // Free the callback.
                MemoryAlloc.PrivateHeap.Free(this.CallbackAlloc);

                this.RegistrationCallback = null;
            }
        }
    }
}
