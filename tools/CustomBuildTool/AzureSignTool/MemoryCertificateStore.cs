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
    /// <summary>
    /// Represents an in-memory certificate store for managing X.509 certificates. Provides methods to add certificates
    /// and retrieve the store handle and certificate collection.
    /// </summary>
    /// <remarks>Use this class to create and manage certificates in a temporary, memory-backed store.
    /// Certificates added to the store are not persisted and are lost when the store is disposed. The class is not
    /// thread-safe; concurrent access should be synchronized externally if required. Implements IDisposable to ensure
    /// proper release of unmanaged resources.</remarks>
    internal sealed unsafe class MemoryCertificateStore : IDisposable
    {
        private HCERTSTORE StoreHandle;
        private readonly X509Store CertStore;
        private bool Disposed;
        
        /// <summary>
        /// 
        /// </summary>
        /// <param name="handle"></param>
        private MemoryCertificateStore(HCERTSTORE handle)
        {
            this.StoreHandle = handle;
            this.CertStore = new X509Store(handle);
        }

        /// <summary>
        /// Creates a new in-memory certificate store.
        /// </summary>
        /// <remarks>Use this method to create a certificate store that exists only in memory and is not
        /// persisted to disk. The store can be used for temporary certificate operations where persistence is not
        /// required.</remarks>
        /// <returns>A new instance of MemoryCertificateStore representing the created memory certificate store.</returns>
        /// <exception cref="Win32Exception">Thrown if the memory certificate store cannot be created.</exception>
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

        /// <summary>
        /// Gets the native handle for the underlying certificate store.
        /// </summary>
        /// <remarks>Use this handle when interoperating with native APIs that require a certificate store
        /// handle. The validity and lifetime of the handle are tied to the certificate store instance; ensure the store
        /// remains open while using the handle.</remarks>
        public IntPtr Handle => this.CertStore.StoreHandle;
        public void Add(X509Certificate2 certificate) => this.CertStore.Add(certificate);
        public void Add(X509Certificate2Collection collection) => this.CertStore.AddRange(collection);
        public X509Certificate2Collection Certificates => this.CertStore.Certificates;

        /// <summary>
        /// Releases the certificate store handle if it is currently allocated.
        /// </summary>
        /// <remarks>This method should be called to ensure that unmanaged resources associated with the
        /// certificate store are properly released. After calling this method, the store handle is reset and cannot be
        /// used until reinitialized.</remarks>
        private void FreeHandle()
        {
            if (this.StoreHandle != default)
            {
                PInvoke.CertCloseStore(this.StoreHandle, 0);
                this.StoreHandle = default;
            }
        }

        /// <summary>
        /// Releases the resources used by the object. If called with disposing set to <see langword="true"/>, both
        /// managed and unmanaged resources are released; otherwise, only unmanaged resources are released.
        /// </summary>
        /// <remarks>This method is typically called by the public Dispose method and the finalizer.
        /// Callers should ensure that Dispose is not called multiple times on the same object.</remarks>
        /// <param name="disposing">A value indicating whether to release both managed and unmanaged resources (<see langword="true"/>) or only
        /// unmanaged resources (<see langword="false"/>).</param>
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