namespace CustomBuildTool
{
    internal sealed unsafe class MemoryCertificateStore : IDisposable
    {
        private HCERTSTORE _handle;
        private readonly X509Store _store;
        private bool _disposed = false;

        private MemoryCertificateStore(HCERTSTORE handle)
        {
            _handle = handle;
            try
            {
                _store = new X509Store((nint)_handle.Value);
            }
            catch
            {
                FreeHandle();
                throw;
            }
        }

        public static MemoryCertificateStore Create()
        {
            fixed (byte* p = "Memory"u8)
            {
                var MemoryCertStore = PInvoke.CertOpenStore(
                    new PCSTR(p),
                    0,
                    (HCRYPTPROV_LEGACY)0,
                    0,
                    null
                    );

                return new MemoryCertificateStore(MemoryCertStore);
            }
        }

        public IntPtr Handle => _store.StoreHandle;
        public void Add(X509Certificate2 certificate) => _store.Add(certificate);
        public void Add(X509Certificate2Collection collection) => _store.AddRange(collection);
        public X509Certificate2Collection Certificates => _store.Certificates;

        private unsafe void FreeHandle()
        {
            if (this._handle.Value != null)
            {
                PInvoke.CertCloseStore(this._handle, 0);
                this._handle = default;
            }
        }

        private void Dispose(bool disposing)
        {
            if (!_disposed)
            {
                if (disposing)
                {
                    _store.Dispose();
                }

                FreeHandle();
                _disposed = true;
            }
        }

        public void Dispose()
        {
            Dispose(true);
            GC.SuppressFinalize(this);
        }

        ~MemoryCertificateStore()
        {
            Dispose(false);
        }
    }
}
