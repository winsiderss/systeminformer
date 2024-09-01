using Windows.Win32.Security.Cryptography;

namespace CustomBuildTool
{
    internal unsafe sealed class MemoryCertificateStore : IDisposable
    {
        private HCERTSTORE _handle;
        private readonly X509Store _store;

        private MemoryCertificateStore(HCERTSTORE handle)
        {
            _handle = handle;
            try
            {
                _store = new X509Store((nint)_handle.Value);
            }
            catch
            {
                //We need to manually clean up the handle here. If we throw here for whatever reason,
                //we'll leak the handle because we'll have a partially constructed object that won't get
                //a finalizer called on or anything to dispose of.
                FreeHandle();
                throw;
            }
        }

        public unsafe static MemoryCertificateStore Create()
        {
            byte[] buffer = Encoding.UTF8.GetBytes("Memory");
            GCHandle handle = GCHandle.Alloc(buffer, GCHandleType.Pinned);
            IntPtr address = handle.AddrOfPinnedObject();

            try
            {
                var MemoryCertStore = PInvoke.CertOpenStore(
                    new PCSTR((byte*)address.ToPointer()), 
                    0, 
                    (HCRYPTPROV_LEGACY)0,
                    0,
                    null
                    );

                return new MemoryCertificateStore(MemoryCertStore);
            }
            finally
            {
                handle.Free();
            }
        }

        public void Close() => Dispose(true);
        void IDisposable.Dispose() => Dispose(true);
        ~MemoryCertificateStore() => Dispose(false);


        public IntPtr Handle => _store.StoreHandle;
        public void Add(X509Certificate2 certificate) => _store.Add(certificate);
        public void Add(X509Certificate2Collection collection) => _store.AddRange(collection);
        public X509Certificate2Collection Certificates => _store.Certificates;

        private void Dispose(bool disposing)
        {
            GC.SuppressFinalize(this);

            if (disposing)
            {
                _store.Dispose();
            }

            FreeHandle();
        }

        private unsafe void FreeHandle()
        {
            if (this._handle.Value != null)
            {
                PInvoke.CertCloseStore(this._handle, 0);
                this._handle = default;
            }
        }
    }
}
