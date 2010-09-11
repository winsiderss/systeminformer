using System;
using System.Runtime.InteropServices;

namespace ProcessHacker2.Api
{
    public unsafe class ProcessTreeFilter : IDisposable
    {
        public static void Apply()
        {
            NativeApi.PhApplyProcessTreeFilters();
        }

        private void* _filter;
        private PhProcessTreeFilter _function;
        private IntPtr _functionNative;

        public ProcessTreeFilter(PhProcessTreeFilter function)
        {
            _function = function;

            _functionNative = Marshal.GetFunctionPointerForDelegate(function);

            _filter = NativeApi.PhAddProcessTreeFilter(_functionNative, IntPtr.Zero);
        }

        public void Dispose()
        {
            if (_filter != null)
            {
                NativeApi.PhRemoveProcessTreeFilter(_filter);
                _filter = null;
            }
        }
    }
}
