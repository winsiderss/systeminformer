/*
 * Process Hacker -
 *   IntPtr extension functions
 *
 * Copyright (C) 2011 wj32
 * Copyright (C) 2009 Flavio Erlich
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

namespace ProcessHacker.Native
{
    public static class IntPtrExtensions
    {
        public static IntPtr Align(this IntPtr ptr, int alignment)
        {
            return ptr.Increment(alignment - 1).And((alignment - 1).ToIntPtr().Not());
        }

        public static IntPtr And(this IntPtr ptr, int value)
        {
            if (OSVersion.Architecture == OSArch.I386)
                return new IntPtr(ptr.ToInt32() & value);

            return new IntPtr(ptr.ToInt64() & value);
        }

        public static IntPtr And(this IntPtr ptr, IntPtr value)
        {
            if (OSVersion.Architecture == OSArch.I386)
                return new IntPtr(ptr.ToInt32() & value.ToInt32());

            return new IntPtr(ptr.ToInt64() & value.ToInt64());
        }

        public static int CompareTo(this IntPtr ptr, IntPtr ptr2)
        {
            if (ptr.ToUInt64() > ptr2.ToUInt64())
                return 1;

            if (ptr.ToUInt64() < ptr2.ToUInt64())
                return -1;

            return 0;
        }

        public static int CompareTo(this IntPtr ptr, int ptr2)
        {
            return ptr.CompareTo((uint)ptr2);
        }

        public static int CompareTo(this IntPtr ptr, uint ptr2)
        {
            if (ptr.ToUInt64() > ptr2)
                return 1;

            if (ptr.ToUInt64() < ptr2)
                return -1;

            return 0;
        }

        public static IntPtr Decrement(this IntPtr ptr, IntPtr ptr2)
        {
            if (OSVersion.Architecture == OSArch.I386)
                return new IntPtr(ptr.ToInt32() - ptr2.ToInt32());

            return new IntPtr(ptr.ToInt64() - ptr2.ToInt64());
        }

        public static IntPtr Decrement(this IntPtr ptr, int value)
        {
            return Increment(ptr, -value);
        }

        public static IntPtr Decrement(this IntPtr ptr, long value)
        {
            return Increment(ptr, -value);
        }

        public static T ElementAt<T>(this IntPtr ptr, int index)
        {
            var offset = Marshal.SizeOf(typeof(T)) * index;
            var offsetPtr = ptr.Increment(offset);
            return (T)Marshal.PtrToStructure(offsetPtr, typeof(T));
        }

        public static bool Equals(this IntPtr ptr, IntPtr ptr2)
        {
            return ptr == ptr2;
        }

        public static bool Equals(this IntPtr ptr, int value)
        {
            return ptr.ToInt32() == value;
        }

        public static bool Equals(this IntPtr ptr, uint value)
        {
            return ptr.ToUInt32() == value;
        }

        public static bool Equals(this IntPtr ptr, long value)
        {
            return ptr.ToInt64() == value;
        }

        public static bool Equals(this IntPtr ptr, ulong value)
        {
            return ptr.ToUInt64() == value;
        }

        public static IntPtr Increment(this IntPtr ptr, int value)
        {
            unchecked
            {
                if (OSVersion.Architecture == OSArch.I386)
                    return new IntPtr(ptr.ToInt32() + value);

                return new IntPtr(ptr.ToInt64() + value);
            }
        }

        public static IntPtr Increment(this IntPtr ptr, long value)
        {
            unchecked
            {
                if (OSVersion.Architecture == OSArch.I386)
                    return new IntPtr((int)(ptr.ToInt32() + value));

                return new IntPtr(ptr.ToInt64() + value);
            }
        }

        public static IntPtr Increment(this IntPtr ptr, IntPtr ptr2)
        {
            unchecked
            {
                if (OSVersion.Architecture == OSArch.I386)
                    return new IntPtr(ptr.ToInt32() + ptr2.ToInt32());

                return new IntPtr(ptr.ToInt64() + ptr2.ToInt64());
            }
        }

        public static IntPtr Increment<T>(this IntPtr ptr)
        {
            return ptr.Increment(Marshal.SizeOf(typeof(T)));
        }

        public static bool IsGreaterThanOrEqualTo(this IntPtr ptr, IntPtr ptr2)
        {
            return ptr.CompareTo(ptr2) >= 0;
        }

        public static bool IsLessThanOrEqualTo(this IntPtr ptr, IntPtr ptr2)
        {
            return ptr.CompareTo(ptr2) <= 0;
        }

        public static IntPtr Not(this IntPtr ptr)
        {
            if (OSVersion.Architecture == OSArch.I386)
                return new IntPtr(~ptr.ToInt32());

            return new IntPtr(~ptr.ToInt64());
        }

        public static IntPtr Or(this IntPtr ptr, IntPtr value)
        {
            if (OSVersion.Architecture == OSArch.I386)
                return new IntPtr(ptr.ToInt32() | value.ToInt32());

            return new IntPtr(ptr.ToInt64() | value.ToInt64());
        }

        public static unsafe uint ToUInt32(this IntPtr ptr)
        {
            // Avoid sign-extending the pointer - we want it zero-extended.
            void* voidPtr = (void*)ptr;

            return (uint)voidPtr;
        }

        public static unsafe ulong ToUInt64(this IntPtr ptr)
        {
            // Avoid sign-extending the pointer - we want it zero-extended.
            void* voidPtr = (void*)ptr;

            return (ulong)voidPtr;
        }

        public static IntPtr ToIntPtr(this int value)
        {
            return new IntPtr(value);
        }

        public static IntPtr ToIntPtr(this uint value)
        {
            unchecked
            {
                return new IntPtr((int)value);
            }
        }

        public static IntPtr ToIntPtr(this long value)
        {
            unchecked
            {
                if (value > 0 && value <= 0xffffffff)
                    return new IntPtr((int)value);
            }

            return new IntPtr(value);
        }

        public static IntPtr ToIntPtr(this ulong value)
        {
            unchecked
            {
                return ((long)value).ToIntPtr();
            }
        }

        public static IntPtr Xor(this IntPtr ptr, IntPtr value)
        {
            if (OSVersion.Architecture == OSArch.I386)
                return new IntPtr(ptr.ToInt32() ^ value.ToInt32());

            return new IntPtr(ptr.ToInt64() ^ value.ToInt64());
        }
    }
}
