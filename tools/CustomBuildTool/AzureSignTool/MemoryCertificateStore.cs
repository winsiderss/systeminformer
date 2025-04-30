/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     dmex
 *
 */

namespace CustomBuildTool
{
    internal sealed unsafe class MemoryCertificateStore : IDisposable
    {
        private HCERTSTORE _handle;
        private readonly X509Store _store;
        private bool _disposed;

        private MemoryCertificateStore(HCERTSTORE handle)
        {
            _handle = handle;
            _store = new X509Store(handle);
        }

        public static MemoryCertificateStore Create()
        {
            fixed (byte* p = "Memory"u8)
            {
                var memoryCertStore = PInvoke.CertOpenStore(
                    new PCSTR(p),
                    0,
                    default,
                    0,
                    null
                    );

                if (memoryCertStore == default)
                {
                    throw new Win32Exception("Failed to create memory certificate store.");
                }

                return new MemoryCertificateStore(memoryCertStore);
            }
        }

        public IntPtr Handle => _store.StoreHandle;
        public void Add(X509Certificate2 certificate) => _store.Add(certificate);
        public void Add(X509Certificate2Collection collection) => _store.AddRange(collection);
        public X509Certificate2Collection Certificates => _store.Certificates;

        private void FreeHandle()
        {
            if (_handle != default)
            {
                PInvoke.CertCloseStore(_handle, 0);
                _handle = default;
            }
        }

        private void Dispose(bool disposing)
        {
            if (_disposed)
                return;

            if (disposing)
            {
                _store.Dispose();
                FreeHandle();
            }

            _disposed = true;
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