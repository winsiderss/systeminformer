/*
 * Process Hacker -
 *   run-time library heap
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

using ProcessHacker.Api;

namespace ProcessHacker.Native
{
    public struct Heap
    {
        private IntPtr HeapHandle;
        private HeapFlags Flags;

        public Heap(HeapFlags flags)
            : this(flags, 0, 0)
        { }

        public Heap(HeapFlags flags, int reserveSize, int commitSize)
        {
            this.HeapHandle = NativeApi.RtlCreateHeap(
                flags,
                IntPtr.Zero,
                reserveSize.ToIntPtr(),
                commitSize.ToIntPtr(),
                IntPtr.Zero,
                IntPtr.Zero
                );

            this.Flags = flags;

            if (this.HeapHandle == IntPtr.Zero)
                throw new OutOfMemoryException();
        }

        public IntPtr Address
        {
            get { return this.HeapHandle; }
        }

        public IntPtr Allocate(int size)
        {
            IntPtr memory = NativeApi.RtlAllocateHeap(this.HeapHandle, this.Flags, size.ToIntPtr());

            if (memory == IntPtr.Zero)
                throw new OutOfMemoryException();

            return memory;
        }

        public int Compact()
        {
            return NativeApi.RtlCompactHeap(this.HeapHandle, this.Flags).ToInt32();
        }

        public void Destroy()
        {
            NativeApi.RtlDestroyHeap(this.HeapHandle);
        }

        public void Free(IntPtr memory)
        {
            NativeApi.RtlFreeHeap(this.HeapHandle, this.Flags, memory);
        }

        public int GetBlockSize(IntPtr memory)
        {
            return NativeApi.RtlSizeHeap(this.HeapHandle, this.Flags, memory).ToInt32();
        }

        public IntPtr Reallocate(IntPtr memory, int size)
        {
            IntPtr newMemory = NativeApi.RtlReAllocateHeap(this.HeapHandle, this.Flags, memory, size.ToIntPtr());

            if (newMemory == IntPtr.Zero)
                throw new OutOfMemoryException();

            return newMemory;
        }
    }
}
