/*
 * Process Hacker - 
 *   memory region
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
using System.Runtime.InteropServices;

using ProcessHacker.Api;
using ProcessHacker.Common.Objects;

namespace ProcessHacker.Native
{
    public class MemoryRegion : BaseObject
    {
        public static T ReadStruct<T>(IntPtr ptr)
        {
            return (T)Marshal.PtrToStructure(ptr, typeof(T));
        }

        public static implicit operator IntPtr(MemoryRegion memory)
        {
            return memory.Memory;
        }

        public static implicit operator int(MemoryRegion memory)
        {
            return memory.Size;
        }
        public static implicit operator uint(MemoryRegion memory)
        {
            return (uint)memory.Size;
        }

        public static implicit operator long(MemoryRegion memory)
        {
            return memory.Size;
        }

        public unsafe static implicit operator void*(MemoryRegion memory)
        {
            return memory.Memory.ToPointer();
        }

        /// <summary>
        /// Creates a new, invalid memory allocation. 
        /// You must set the pointer using the Memory property.
        /// </summary>
        protected MemoryRegion()
        { }

        public MemoryRegion(IntPtr memory)
            : this(memory, 0)
        { }

        public MemoryRegion(IntPtr memory, int offset)
            : this(memory, offset, 0)
        { }

        public MemoryRegion(IntPtr memory, int offset, int size)
            : this(memory.Increment(offset), size, false)
        { }

        protected MemoryRegion(IntPtr memory, int size, bool owned)
            : this(null, memory, size, owned)
        { }

        protected MemoryRegion(MemoryRegion parent, IntPtr memory, int size, bool owned) : base(owned)
        {       
            if (parent != null)
                parent.Reference();

            this.Parent = parent;
            this.Memory = memory;
            this.Size = size;
        }

        protected sealed override void DisposeObject(bool disposing)
        {
            this.Free();

            if (this.Parent != null)
                this.Parent.Dereference(disposing);

            this.Memory = IntPtr.Zero;
            this.Size = 0;
        }

        protected virtual void Free()
        { }

        /// <summary>
        /// Gets a pointer to the allocated memory.
        /// </summary>
        public IntPtr Memory { get; protected set; }

        public MemoryRegion Parent { get; private set; }

        /// <summary>
        /// Gets the size of the allocated memory.
        /// </summary>
        public virtual int Size { get; protected set; }

        public void DestroyStruct<T>()
        {
            Marshal.DestroyStructure(this.Memory, typeof(T));
        }

        public void DestroyStruct<T>(int index)
        {
            this.DestroyStruct<T>(0, index);
        }

        public void DestroyStruct<T>(int offset, int index)
        {
            if (index == 0)
            {
                Marshal.DestroyStructure(this.Memory.Increment(offset), typeof(T));
            }
            else
            {
                Marshal.DestroyStructure(
                    this.Memory.Increment(offset + Marshal.SizeOf(typeof(T)) * index),
                    typeof(T)
                    );
            }
        }

        public void Fill(int offset, int length, byte value)
        {
            NativeApi.RtlFillMemory(
                this.Memory.Increment(offset),
                length.ToIntPtr(),
                value
                );
        }

        public MemoryRegion MakeChild(int offset, int size)
        {
            return new MemoryRegion(this, this.Memory.Increment(offset), size, true);
        }

        public string ReadAnsiString(int offset)
        {
            return Marshal.PtrToStringAnsi(this.Memory.Increment(offset));
        }

        public string ReadAnsiString(int offset, int length)
        {
            return Marshal.PtrToStringAnsi(this.Memory.Increment(offset), length);
        }

        public byte[] ReadBytes(int length)
        {
            return this.ReadBytes(0, length);
        }

        public byte[] ReadBytes(int offset, int length)
        {
            byte[] buffer = new byte[length];

            this.ReadBytes(offset, buffer, 0, length);

            return buffer;
        }

        public void ReadBytes(byte[] buffer, int startIndex, int length)
        {
            this.ReadBytes(0, buffer, startIndex, length);
        }

        public void ReadBytes(int offset, byte[] buffer, int startIndex, int length)
        {
            Marshal.Copy(this.Memory.Increment(offset), buffer, startIndex, length);
        }

        /// <summary>
        /// Reads a signed integer.
        /// </summary>
        /// <param name="offset">The offset at which to begin reading.</param>
        /// <returns>The integer.</returns>
        public int ReadInt32(int offset)
        {
            return this.ReadInt32(offset, 0);
        }

        /// <summary>
        /// Reads a signed integer.
        /// </summary>
        /// <param name="offset">The offset at which to begin reading.</param>
        /// <param name="index">The index at which to begin reading, after the offset is added.</param>
        /// <returns>The integer.</returns>
        public unsafe int ReadInt32(int offset, int index)
        {
            return ((int*)((byte*)this.Memory + offset))[index];
        }

        public int[] ReadInt32Array(int offset, int count)
        {
            int[] array = new int[count];

            Marshal.Copy(this.Memory.Increment(offset), array, 0, count);

            return array;
        }

        public IntPtr ReadIntPtr(int offset)
        {
            return this.ReadIntPtr(offset, 0);
        }

        public unsafe IntPtr ReadIntPtr(int offset, int index)
        {
            return ((IntPtr*)((byte*)this.Memory + offset))[index];
        }

        public void ReadMemory(IntPtr buffer, int destOffset, int srcOffset, int length)
        {
            NativeApi.RtlMoveMemory(
                buffer.Increment(destOffset),
                this.Memory.Increment(srcOffset),
                length.ToIntPtr()
                );
        }

        /// <summary>
        /// Reads an unsigned integer.
        /// </summary>
        /// <param name="offset">The offset at which to begin reading.</param>
        /// <returns>The integer.</returns>
        public uint ReadUInt32(int offset)
        {
            return this.ReadUInt32(offset, 0);
        }

        /// <summary>
        /// Reads an unsigned integer.
        /// </summary>
        /// <param name="offset">The offset at which to begin reading.</param>
        /// <param name="index">The index at which to begin reading, after the offset is added.</param>
        /// <returns>The integer.</returns>
        public unsafe uint ReadUInt32(int offset, int index)
        {
            return ((uint*)((byte*)this.Memory + offset))[index];
        }

        /// <summary>
        /// Creates a struct from the memory allocation.
        /// </summary>
        /// <typeparam name="T">The type of the struct.</typeparam>
        /// <returns>The new struct.</returns>
        public T ReadStruct<T>() where T : struct
        {
            return (T)Marshal.PtrToStructure(this.Memory, typeof(T));
        }

        /// <summary>
        /// Creates a struct from the memory allocation.
        /// </summary>
        /// <typeparam name="T">The type of the struct.</typeparam>
        /// <param name="index">The index at which to begin reading to the struct. This is multiplied by  
        /// the size of the struct.</param>
        /// <returns>The new struct.</returns>
        public T ReadStruct<T>(int index) where T : struct
        {
            return this.ReadStruct<T>(0, index);
        }

        /// <summary>
        /// Creates a struct from the memory allocation.
        /// </summary>
        /// <typeparam name="T">The type of the struct.</typeparam>
        /// <param name="offset">The offset to add before reading.</param>
        /// <param name="index">The index at which to begin reading to the struct. This is multiplied by  
        /// the size of the struct.</param>
        /// <returns>The new struct.</returns>
        public T ReadStruct<T>(int offset, int index) where T : struct
        {
            if (index == 0)
                return (T)Marshal.PtrToStructure(this.Memory.Increment(offset), typeof(T));

            return (T)Marshal.PtrToStructure(this.Memory.Increment(offset + Marshal.SizeOf(typeof(T)) * index), typeof(T));
        }

        public string ReadUnicodeStringLength(int length)
        {
            return Marshal.PtrToStringUni(this.Memory, length);
        }

        public string ReadUnicodeString(int offset)
        {
            return Marshal.PtrToStringUni(this.Memory.Increment(offset));
        }

        public string ReadUnicodeString(int offset, int length)
        {
            return Marshal.PtrToStringUni(this.Memory.Increment(offset), length);
        }

        /// <summary>
        /// Writes a single byte to the memory allocation.
        /// </summary>
        /// <param name="offset">The offset at which to write.</param>
        /// <param name="b">The value of the byte.</param>
        public unsafe void WriteByte(int offset, byte b)
        {
            *((byte*)this.Memory + offset) = b;
        }

        public void WriteBytes(int offset, byte[] b)
        {
            Marshal.Copy(b, 0, this.Memory.Increment(offset), b.Length);
        }

        public unsafe void WriteInt16(int offset, short i)
        {
            *(short*)((byte*)this.Memory + offset) = i;
        }

        public unsafe void WriteInt32(int offset, int i)
        {
            *(int*)((byte*)this.Memory + offset) = i;
        }

        public unsafe void WriteIntPtr(int offset, IntPtr i)
        {
            *(IntPtr*)((byte*)this.Memory + offset) = i;
        }

        public void WriteMemory(int offset, IntPtr buffer, int length)
        {
            NativeApi.RtlMoveMemory(
                this.Memory.Increment(offset),
                buffer,
                length.ToIntPtr()
                );
        }

        public void WriteStruct<T>(T s) where T : struct
        {
            Marshal.StructureToPtr(s, this.Memory, false);
        }

        public void WriteStruct<T>(int index, T s) where T : struct
        {
            this.WriteStruct(0, index, s);
        }

        public void WriteStruct<T>(int offset, int index, T s) where T : struct
        {
            if (index == 0)
            {
                Marshal.StructureToPtr(s, this.Memory.Increment(offset), false);
            }
            else
            {
                Marshal.StructureToPtr(s, this.Memory.Increment(offset + Marshal.SizeOf(typeof(T)) * index), false);
            }
        }

        /// <summary>
        /// Writes a Unicode string (without a null terminator) to the allocated memory.
        /// </summary>
        /// <param name="offset">The offset to add.</param>
        /// <param name="s">The string to write.</param>
        public unsafe void WriteUnicodeString(int offset, string s)
        {
            fixed (char* ptr = s)
            {
                this.WriteMemory(offset, (IntPtr)ptr, s.Length * 2);
            }
        }

        public void Zero(int offset, int length)
        {
            NativeApi.RtlZeroMemory(
                this.Memory.Increment(offset),
                length.ToIntPtr()
                );
        }
    }
}
