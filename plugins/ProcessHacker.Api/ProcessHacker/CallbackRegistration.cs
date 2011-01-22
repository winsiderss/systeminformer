using System;
using System.Runtime.InteropServices;

namespace ProcessHacker.Api
{
    public unsafe class CallbackRegistration : IDisposable
    {
        private PhCallback* _callback;
        private PhCallbackRegistration* _registration;
        private PhCallbackFunction _function;
        private IntPtr _functionNative;

        public CallbackRegistration(PhCallback* callback, PhCallbackFunction function)
        {
            _callback = callback;
            _function = function;

            _registration = (PhCallbackRegistration*)Marshal.AllocHGlobal(Marshal.SizeOf(typeof(PhCallbackRegistration)));

            _functionNative = Marshal.GetFunctionPointerForDelegate(function);

            NativeApi.PhRegisterCallback(_callback, _functionNative, IntPtr.Zero, _registration);
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
