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
        private HCERTSTORE StoreHandle;
        private readonly X509Store CertStore;
        private bool Disposed;

        private MemoryCertificateStore(HCERTSTORE handle)
        {
            this.StoreHandle = handle;
            this.CertStore = new X509Store(handle);
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

        public IntPtr Handle => this.CertStore.StoreHandle;
        public void Add(X509Certificate2 certificate) => this.CertStore.Add(certificate);
        public void Add(X509Certificate2Collection collection) => this.CertStore.AddRange(collection);
        public X509Certificate2Collection Certificates => this.CertStore.Certificates;

        private void FreeHandle()
        {
            if (this.StoreHandle != default)
            {
                PInvoke.CertCloseStore(this.StoreHandle, 0);
                this.StoreHandle = default;
            }
        }

        private void Dispose(bool disposing)
        {
            if (this.Disposed)
                return;

            if (disposing)
            {
                this.CertStore.Dispose();
                FreeHandle();
            }

            this.Disposed = true;
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