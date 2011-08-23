/*
 * Process Hacker - 
 *   Runtime Library functions
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

using ProcessHacker.Native;

namespace ProcessHacker.Api
{
    [StructLayout(LayoutKind.Explicit)]
    public struct LargeInteger
    {
        [FieldOffset(0)]
        public long QuadPart;

        [FieldOffset(0)]
        public uint LowPart;
        [FieldOffset(4)]
        public int HighPart;
    }

    [StructLayout(LayoutKind.Sequential)]
    public unsafe struct ListEntry
    {
        public ListEntry* Flink;
        public ListEntry* Blink;
    }

    [System.Security.SuppressUnmanagedCodeSecurity]
    public unsafe static partial class NativeApi
    {
        #region Doubly-linked lists

        public static void InitializeListHead(
            ListEntry* ListHead
            )
        {
            ListHead->Flink = ListHead->Blink = ListHead;
        }

        public static bool IsListEmpty(
            ListEntry* ListHead
            )
        {
            return ListHead->Flink == ListHead->Blink;
        }

        public static bool RemoveEntryList(
            ListEntry* Entry
            )
        {
            ListEntry* blink;
            ListEntry* flink;

            flink = Entry->Flink;
            blink = Entry->Blink;
            blink->Flink = flink;
            flink->Blink = blink;

            return flink == blink;
        }

        public static ListEntry* RemoveHeadList(
            ListEntry* ListHead
            )
        {
            ListEntry* flink;
            ListEntry* entry;

            entry = ListHead->Flink;
            flink = entry->Flink;
            ListHead->Flink = flink;
            flink->Blink = ListHead;

            return entry;
        }

        public static ListEntry* RemoveTailList(
            ListEntry* ListHead
            )
        {
            ListEntry* blink;
            ListEntry* entry;

            entry = ListHead->Blink;
            blink = entry->Blink;
            ListHead->Blink = blink;
            blink->Flink = ListHead;

            return entry;
        }

        public static void InsertTailList(
            ListEntry* ListHead,
            ListEntry* Entry
            )
        {
            ListEntry* blink;

            blink = ListHead->Blink;
            Entry->Flink = ListHead;
            Entry->Blink = blink;
            blink->Flink = Entry;
            ListHead->Blink = Entry;
        }

        public static void InsertHeadList(
            ListEntry* ListHead,
            ListEntry* Entry
            )
        {
            ListEntry* flink;

            flink = ListHead->Flink;
            Entry->Flink = flink;
            Entry->Blink = ListHead;
            flink->Blink = Entry;
            ListHead->Flink = Entry;
        }

        #endregion

        #region Errors

        [DllImport("ntdll.dll")]
        public static extern uint RtlNtStatusToDosError(
            [In] NtStatus Status
            );

        #endregion

        #region Heaps

        [DllImport("ntdll.dll")]
        public static extern IntPtr RtlAllocateHeap(
            [In] IntPtr HeapHandle,
            [In] HeapFlags Flags,
            [In] IntPtr Size
            );

        [DllImport("ntdll.dll")]
        public static extern IntPtr RtlCompactHeap(
            [In] IntPtr HeapHandle,
            [In] HeapFlags Flags
            );

        [DllImport("ntdll.dll")]
        public static extern IntPtr RtlCreateHeap(
            [In] HeapFlags Flags,
            [In] [Optional] IntPtr HeapBase,
            [In] [Optional] IntPtr ReserveSize,
            [In] [Optional] IntPtr CommitSize,
            [In] [Optional] IntPtr Lock,
            [In] [Optional] IntPtr Parameters
            );

        [DllImport("ntdll.dll")]
        public static extern IntPtr RtlDestroyHeap(
            [In] IntPtr HeapHandle
            );

        [DllImport("ntdll.dll")]
        [return: MarshalAs(UnmanagedType.U1)]
        public static extern bool RtlFreeHeap(
            [In] IntPtr HeapHandle,
            [In] HeapFlags Flags,
            [In] IntPtr BaseAddress
            );

        [DllImport("ntdll.dll")]
        public static extern int RtlGetProcessHeaps(
            [In] int NumberOfHeaps,
            IntPtr[] ProcessHeaps
            );

        [DllImport("ntdll.dll")]
        [return: MarshalAs(UnmanagedType.U1)]
        public static extern bool RtlLockHeap(
            [In] IntPtr HeapHandle
            );

        [DllImport("ntdll.dll")]
        public static extern void RtlProtectHeap(
            [In] IntPtr HeapHandle,
            [In] bool MakeReadOnly
            );

        [DllImport("ntdll.dll")]
        public static extern IntPtr RtlReAllocateHeap(
            [In] IntPtr HeapHandle,
            [In] HeapFlags Flags,
            [In] IntPtr BaseAddress,
            [In] IntPtr Size
            );

        [DllImport("ntdll.dll")]
        public static extern IntPtr RtlSizeHeap(
            [In] IntPtr HeapHandle,
            [In] HeapFlags Flags,
            [In] IntPtr BaseAddress
            );

        [DllImport("ntdll.dll")]
        [return: MarshalAs(UnmanagedType.U1)]
        public static extern bool RtlUnlockHeap(
            [In] IntPtr HeapHandle
            );

        [DllImport("ntdll.dll")]
        public static extern NtStatus RtlZeroHeap(
            [In] IntPtr HeapHandle,
            [In] HeapFlags Flags
            );

        #endregion

        #region Memory

        [DllImport("ntdll.dll")]
        public static extern IntPtr RtlCompareMemory(
            [In] IntPtr Source1,
            [In] IntPtr Source2,
            [In] IntPtr Length
            );

        [DllImport("ntdll.dll")]
        public static extern void RtlFillMemory(
            [In] IntPtr Destination,
            [In] IntPtr Length,
            [In] byte Fill
            );

        [DllImport("ntdll.dll")]
        public static extern void RtlMoveMemory(
            [In] IntPtr Destination,
            [In] IntPtr Source,
            [In] IntPtr Length
            );

        [DllImport("ntdll.dll")]
        public static extern void RtlMoveMemory(
            [In] IntPtr Destination,
            [In] void* Source,
            [In] IntPtr Length
            );

        [DllImport("ntdll.dll")]
        public static extern void RtlMoveMemory(
            [In] void* Destination,
            [In] void* Source,
            [In] IntPtr Length
            );

        [DllImport("ntdll.dll")]
        public static extern void RtlZeroMemory(
            [In] IntPtr Destination,
            [In] IntPtr Length
            );

        [DllImport("ntdll.dll")]
        public static extern void RtlZeroMemory(
            [In] void* Destination,
            [In] IntPtr Length
            );

        #endregion

        #region Strings

        [DllImport("ntdll.dll")]
        public static extern void RtlInitUnicodeString(
            UnicodeString* DestinationString,
            [Optional] void* SourceString
            );

        [DllImport("ntdll.dll")]
        [return: MarshalAs(UnmanagedType.I1)]
        public static extern bool RtlCreateUnicodeString(
            UnicodeString* DestinationString,
            void* SourceString
            );

        [DllImport("ntdll.dll")]
        public static extern void RtlFreeUnicodeString(
            UnicodeString* DestinationString
            );

        [DllImport("ntdll.dll")]
        public static extern int RtlCompareUnicodeString(
            UnicodeString* String1,
            UnicodeString* String2,
            [MarshalAs(UnmanagedType.I1)]
            bool CaseInSensitive
            );

        [DllImport("ntdll.dll")]
        [return: MarshalAs(UnmanagedType.I1)]
        public static extern bool RtlEqualUnicodeString(
            UnicodeString* String1,
            UnicodeString* String2,
            [MarshalAs(UnmanagedType.I1)]
            bool CaseInSensitive
            );

        [DllImport("ntdll.dll")]
        [return: MarshalAs(UnmanagedType.I1)]
        public static extern bool RtlPrefixUnicodeString(
            UnicodeString* String1,
            UnicodeString* String2,
            [MarshalAs(UnmanagedType.I1)] 
            bool CaseInSensitive
            );

        // This function is based on Marshal.StringToHGlobalUni with improvements.
        public static IntPtr StringToNativeUni(String s)
        {
            if (string.IsNullOrEmpty(s))
                return IntPtr.Zero;

            int nb = (s.Length + 1) * 2;

            // Overflow checking 
            if (nb < s.Length)
                throw new ArgumentOutOfRangeException("s");

            IntPtr len = new IntPtr(nb);
            IntPtr hglobal = MemoryAlloc.PrivateHeap.Allocate(nb);

            if (hglobal == IntPtr.Zero)
                throw new OutOfMemoryException();

            fixed (void* p = s)
                RtlMoveMemory(hglobal, p, len);

            return hglobal;
        } 

        #endregion
    }
}
