/*
 * Process Hacker -
 *   disposable object base functionality
 *
 * Copyright (C) 2011 wj32
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

/* If enabled, the object system will keep statistics. */
//#define ENABLE_STATISTICS

/* If enabled, the finalizers on objects can be enabled and disabled.
 * If disabled, the finalizers on objects can only be disabled.
 */
//#define EXTENDED_FINALIZER

/* If enabled, the object system will keep a list of live objects. */
//#define DEBUG_ENABLE_LIVE_LIST

using System;
using System.Threading;

namespace ProcessHacker.Common.Objects
{
    /// <summary>
    /// Provides methods for managing a disposable object or resource.
    /// </summary>
    /// <remarks>
    /// <para>
    /// Each disposable object starts with a reference count of one
    /// when it is created. The object is not owned by the creator;
    /// rather, it is owned by the GC (garbage collector). If the user
    /// does not dispose the object, the finalizer will be called by
    /// the GC, the reference count will be decremented and the object
    /// will be freed. If the user chooses to call Dispose, the reference
    /// count will be decremented and the object will be freed. The
    /// object is no longer owned by the GC and the finalizer will be
    /// suppressed. Any further calls to Dispose will have no effect.
    /// </para>
    /// <para>
    /// If the user chooses to use reference counting, the object
    /// functions normally with the GC. If the object's reference count
    /// is incremented after it is created and becomes 2, it will be
    /// decremented when it is finalized or disposed. Only after the
    /// object is dereferenced will the reference count become 0 and
    /// the object will be freed.
    /// </para>
    /// </remarks>
    public abstract class BaseObject : IDisposable
    {
        private const int ObjectOwned = 0x1;
        private const int ObjectOwnedByGc = 0x2;
        private const int ObjectDisposed = 0x4;
        private const int ObjectRefCountShift = 3;
        private const int ObjectRefCountMask = 0x1fffffff;
        private const int ObjectRefCountIncrement = 0x8;

        private static int _createdCount;
        private static int _freedCount;
        private static int _disposedCount;
        private static int _finalizedCount;
        private static int _referencedCount;
        private static int _dereferencedCount;

#if DEBUG_ENABLE_LIVE_LIST
        private static System.Collections.Generic.List<WeakReference<BaseObject>> _liveList =
            new System.Collections.Generic.List<WeakReference<BaseObject>>();
#endif

        /// <summary>
        /// Gets the number of disposable, owned objects that have been created.
        /// </summary>
        public static int CreatedCount { get { return _createdCount; } }
        /// <summary>
        /// Gets the number of disposable objects that have been freed.
        /// </summary>
        public static int FreedCount { get { return _freedCount; } }
        /// <summary>
        /// Gets the number of disposable objects that have been Disposed with managed = true.
        /// </summary>
        public static int DisposedCount { get { return _disposedCount; } }
        /// <summary>
        /// Gets the number of disposable objects that have been Disposed with managed = false.
        /// </summary>
        public static int FinalizedCount { get { return _finalizedCount; } }
        /// <summary>
        /// Gets the number of times disposable objects have been referenced.
        /// </summary>
        public static int ReferencedCount { get { return _referencedCount; } }
        /// <summary>
        /// Gets the number of times disposable objects have been dereferenced.
        /// </summary>
        public static int DereferencedCount { get { return _dereferencedCount; } }

#if DEBUG_ENABLE_LIVE_LIST
        public static void CleanLiveList()
        {
            var list = new System.Collections.Generic.List<WeakReference<BaseObject>>();

            foreach (var r in _liveList)
            {
                if (r.Target != null)
                    list.Add(r);
            }

            _liveList = list;
        }

        /// <summary>
        /// A stack trace collected when the object is created.
        /// </summary>
        private string _creationStackTrace;
#endif
        /// <summary>
        /// An Int32 containing various fields.
        /// </summary>
        private int _value;
#if EXTENDED_FINALIZER
        /// <summary>
        /// Whether the finalizer will run.
        /// </summary>
        private int _finalizerRegistered = 1;
#endif

        /// <summary>
        /// Initializes a disposable object.
        /// </summary>
        protected BaseObject()
            : this(true)
        { }

        /// <summary>
        /// Initializes a disposable object.
        /// </summary>
        /// <param name="owned">Whether the resource is owned.</param>
        protected BaseObject(bool owned)
        {
            _value = ObjectOwned + ObjectOwnedByGc + ObjectRefCountIncrement;

            // Don't need to finalize the object if it doesn't need to be disposed.
            if (!owned)
            {
#if EXTENDED_FINALIZER
                this.DisableFinalizer();
#else
                GC.SuppressFinalize(this);
#endif
                _value = 0;
            }

#if ENABLE_STATISTICS
            if (owned)
                Interlocked.Increment(ref _createdCount);
#endif

#if DEBUG_ENABLE_LIVE_LIST
            _creationStackTrace = Environment.StackTrace;

            _liveList.Add(new WeakReference<BaseObject>(this));
#endif
        }

        /// <summary>
        /// Ensures that the GC does not own the object.
        /// </summary>
        ~BaseObject()
        {
            // Get rid of GC ownership if still present.
            this.Dispose(false);

#if ENABLE_STATISTICS
            Interlocked.Increment(ref _finalizedCount);
            // Dispose just incremented this value, but it
            // shouldn't have been incremented.
            Interlocked.Decrement(ref _disposedCount);
#endif
        }

        /// <summary>
        /// Ensures that the GC does not own the object.
        /// </summary>
        public void Dispose()
        {
            this.Dispose(true);
        }

        /// <summary>
        /// Ensures that the GC does not own the object.
        /// </summary>
        /// <param name="managed">Whether to dispose managed resources.</param>
        public void Dispose(bool managed)
        {
            int value;

            if ((_value & ObjectOwned) == 0)
                return;

            // Only proceed if the object is owned by the GC, and
            // clear the owned by GC flag (all atomically).
            do
            {
                value = _value;

                if ((value & ObjectOwnedByGc) == 0)
                    return;
            }
            while (Interlocked.CompareExchange(
                ref _value,
                value - ObjectOwnedByGc,
                value
                ) != value);

            // Decrement the reference count.
            this.Dereference(managed);

            // Disable the finalizer.
            if (managed)
            {
#if EXTENDED_FINALIZER
                this.DisableFinalizer();
#else
                GC.SuppressFinalize(this);
#endif
            }

#if ENABLE_STATISTICS
            // Stats.
            Interlocked.Increment(ref _disposedCount);

            // The dereferenced count should count the number of times
            // the user has called Dereference, so decrement it
            // because we just called it.
            Interlocked.Decrement(ref _dereferencedCount);
#endif
        }

        /// <summary>
        /// Disposes the resources of the object. This method must not be
        /// called directly; instead, override this method in a derived class.
        /// </summary>
        /// <param name="disposing">Whether or not to dispose managed objects.</param>
        protected abstract void DisposeObject(bool disposing);

        /// <summary>
        /// Gets whether the object has been freed.
        /// </summary>
        public bool Disposed
        {
            get { return (_value & ObjectDisposed) != 0; }
        }

        /// <summary>
        /// Gets whether the object will be freed.
        /// </summary>
        public bool Owned
        {
            get { return (_value & ObjectOwned) != 0; }
        }

        /// <summary>
        /// Gets whether the object is owned by the garbage collector.
        /// </summary>
        public bool OwnedByGc
        {
            get { return (_value & ObjectOwnedByGc) != 0; }
        }

        /// <summary>
        /// Gets the current reference count of the object.
        /// </summary>
        /// <remarks>
        /// This information is for debugging purposes ONLY. DO NOT
        /// base memory management logic upon this value.
        /// </remarks>
        public int ReferenceCount
        {
            get { return (_value >> ObjectRefCountShift) & ObjectRefCountMask; }
        }

#if EXTENDED_FINALIZER
        /// <summary>
        /// Disables the finalizer if it is not already disabled.
        /// </summary>
        private void DisableFinalizer()
        {
            int oldFinalizerRegistered;

            oldFinalizerRegistered = Interlocked.CompareExchange(ref _finalizerRegistered, 0, 1);

            if (oldFinalizerRegistered == 1)
            {
                GC.SuppressFinalize(this);
            }
        }
#endif

        /// <summary>
        /// Declares that the object should no longer be owned.
        /// </summary>
        protected void DisableOwnership(bool dispose)
        {
            int value;

            if (dispose)
                this.Dispose();

#if EXTENDED_FINALIZER
            this.DisableFinalizer();
#else
            GC.SuppressFinalize(this);
#endif
            do
            {
                value = _value;
            } while (Interlocked.CompareExchange(
                ref _value,
                value & ~ObjectOwned,
                value
                ) != value);

#if ENABLE_STATISTICS
            // If the object didn't get disposed, pretend the object
            // never got created.
            if (!dispose)
                Interlocked.Decrement(ref _createdCount);
#endif
        }

        /// <summary>
        /// Decrements the reference count of the object.
        /// </summary>
        /// <returns>The old reference count.</returns>
        /// <remarks>
        /// <para>
        /// DO NOT call Dereference if you have not called Reference.
        /// Call Dispose instead.
        /// </para>
        /// <para>
        /// If you are calling Dereference from a finalizer, call
        /// Dereference(false).
        /// </para>
        /// </remarks>
        public int Dereference()
        {
            return this.Dereference(true);
        }

        /// <summary>
        /// Decrements the reference count of the object.
        /// </summary>
        /// <param name="managed">Whether to dispose managed resources.</param>
        /// <returns>The new reference count.</returns>
        /// <remarks>
        /// <para>If you are calling this method from a finalizer, set
        /// <paramref name="managed" /> to false.</para>
        /// </remarks>
        public int Dereference(bool managed)
        {
            return this.Dereference(1, managed);
        }

        /// <summary>
        /// Decreases the reference count of the object.
        /// </summary>
        /// <param name="count">The number of times to dereference the object.</param>
        /// <returns>The new reference count.</returns>
        public int Dereference(int count)
        {
            return this.Dereference(count, true);
        }

        /// <summary>
        /// Decreases the reference count of the object.
        /// </summary>
        /// <param name="count">The number of times to dereference the object.</param>
        /// <param name="managed">Whether to dispose managed resources.</param>
        /// <returns>The new reference count.</returns>
        public int Dereference(int count, bool managed)
        {
            int value;
            int newRefCount;

            // Initial parameter validation.
            if (count == 0)
                return this.ReferenceCount;
            if (count < 0)
                throw new ArgumentException("Cannot dereference a negative number of times.");

            if ((_value & ObjectOwned) == 0)
                return 0;

#if ENABLE_STATISTICS
            // Statistics.
            Interlocked.Add(ref _dereferencedCount, count);
#endif
            // Decrease the reference count.
            value = Interlocked.Add(ref _value, -ObjectRefCountIncrement * count);
            newRefCount = (value >> ObjectRefCountShift) & ObjectRefCountMask;

            // Should not ever happen.
            if (newRefCount < 0)
                throw new InvalidOperationException("Reference count cannot be negative.");

            // Dispose the object if the reference count is 0.
            if (newRefCount == 0)
            {
                // If the dispose object method throws an exception, nothing bad
                // should happen if it does not invalidate any state.
                this.DisposeObject(managed);

                // Set the disposed flag.
                do
                {
                    value = _value;
                } while (Interlocked.CompareExchange(
                    ref _value,
                    value | ObjectDisposed,
                    value
                    ) != value);

#if ENABLE_STATISTICS
                Interlocked.Increment(ref _freedCount);
#endif
            }

            return newRefCount;
        }

#if EXTENDED_FINALIZER
        /// <summary>
        /// Enables the finalizer if it is not already enabled.
        /// </summary>
        private void EnableFinalizer()
        {
            int oldFinalizerRegistered;

            oldFinalizerRegistered = Interlocked.CompareExchange(ref _finalizerRegistered, 1, 0);

            if (oldFinalizerRegistered == 0)
            {
                GC.ReRegisterForFinalize(this);
            }
        }
#endif

        /// <summary>
        /// Increments the reference count of the object.
        /// </summary>
        /// <returns>The new reference count.</returns>
        /// <remarks>
        /// <para>
        /// You must call Dereference once (when you are finished with the
        /// object) to match each call to Reference. Do not call Dispose.
        /// </para>
        /// </remarks>
        public int Reference()
        {
            return this.Reference(1);
        }

        /// <summary>
        /// Increases the reference count of the object.
        /// </summary>
        /// <param name="count">The number of times to reference the object.</param>
        /// <returns>The new reference count.</returns>
        public int Reference(int count)
        {
            int value;

            // Don't do anything if the object isn't owned.
            if ((_value & ObjectOwned) == 0)
                return 0;
            // Parameter validation.
            if (count == 0)
                return this.ReferenceCount;
            if (count < 0)
                throw new ArgumentException("Cannot reference a negative number of times.");

#if ENABLE_STATISTICS
            Interlocked.Add(ref _referencedCount, count);
#endif
            value = Interlocked.Add(ref _value, ObjectRefCountIncrement * count);

            return (value >> ObjectRefCountShift) & ObjectRefCountMask;
        }
    }
}
