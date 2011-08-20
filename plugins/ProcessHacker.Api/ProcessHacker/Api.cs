/*
 * Process Hacker - 
 *   API definitions
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
using System.Drawing;

namespace ProcessHacker.Api
{
    public struct GeneralGetHighlightingColorArgs
    {
        public IntPtr Parameter;
        public Color BackColor;
        public bool Handled;
        public bool Cache;
    }

    [StructLayout(LayoutKind.Sequential)]
    public unsafe struct PhAvlLinks
    {
        public PhAvlLinks* Parent;
        public PhAvlLinks* Left;
        public PhAvlLinks* Right;
        public int Balance;
    }

    /// <summary>
    /// A callback structure.
    /// The callback object allows multiple callback functions 
    /// to be registered and notified in a thread-safe way.
    /// </summary>
    [StructLayout(LayoutKind.Sequential)]
    public struct PhCallback
    {
        /// <summary>
        /// The list of registered callbacks.
        /// </summary>
        public ListEntry ListHead;

        /// <summary>
        /// A lock protecting the callbacks list.
        /// </summary>
        public PhQueuedLock ListLock;

        /// <summary>
        /// A condition variable pulsed when the callback becomes free.
        /// </summary>
        public PhQueuedLock BusyCondition;
    }

    /// <summary>
    /// A callback function.
    /// </summary>
    /// <param name="Parameter">
    /// A value given to all callback functions being notified.
    /// </param>
    /// <param name="Context">
    /// A user-defined value passed to PhRegisterCallback().
    /// </param>
    public delegate void PhCallbackFunction(
        [In, Optional] IntPtr Parameter,
        [In, Optional] IntPtr Context
        );

    [StructLayout(LayoutKind.Sequential)]
    public struct PhCallbackRegistration
    {
        public static readonly int SizeOf;

        public ListEntry ListEntry;
        public IntPtr Function;
        public IntPtr Context;
        public int Busy;
        public byte Unregistering;
        public byte Reserved;
        public ushort Flags;

        static PhCallbackRegistration()
        {
            SizeOf = Marshal.SizeOf(typeof(PhCallbackRegistration));
        }
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct PhEvent
    {
        public IntPtr Value;
        public IntPtr EventHandle;
    }

    [StructLayout(LayoutKind.Sequential)]
    public unsafe struct PhHashEntry
    {
        public PhHashEntry* Next;
        public int Hash;
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct PhIntegerPair
    {
        public int X;
        public int Y;
    }

    public enum PhGeneralCallback
    {
        MainWindowShowing = 0, // INT ShowCommand [main thread]
        IntervalUpdate = 1, // [main thread]
        GetProcessHighlightingColor = 2, // PPH_PLUGIN_GET_HIGHLIGHTING_COLOR Data [main thread]
        GetProcessTooltipText = 3, // PPH_PLUGIN_GET_TOOLTIP_TEXT Data [main thread]
        ProcessPropertiesInitializing = 4, // PPH_PLUGIN_PROCESS_PROPCONTEXT Data [properties thread]
        GetIsDotNetDirectoryNames = 5, // PPH_PLUGIN_IS_DOT_NET_DIRECTORY_NAMES Data [process provider thread]
        NotifyEvent = 6, // PPH_PLUGIN_NOTIFY_EVENT Data [main thread]
        ServicePropertiesInitializing = 7, // PPH_PLUGIN_OBJECT_PROPERTIES Data [properties thread]
        HandlePropertiesInitializing = 8, // PPH_PLUGIN_OBJECT_PROPERTIES Data [properties thread]
        ProcessMenuInitializing = 9, // PPH_PLUGIN_MENU_INFORMATION Data [main thread]
        ServiceMenuInitializing = 10, // PPH_PLUGIN_MENU_INFORMATION Data [main thread]
        NetworkMenuInitializing = 11, // PPH_PLUGIN_MENU_INFORMATION Data [main thread]
        IconMenuInitializing = 12, // PPH_PLUGIN_MENU_INFORMATION Data [main thread]
        ThreadMenuInitializing = 13, // PPH_PLUGIN_MENU_INFORMATION Data [properties thread]
        ModuleMenuInitializing = 14, // PPH_PLUGIN_MENU_INFORMATION Data [properties thread]
        MemoryMenuInitializing = 15, // PPH_PLUGIN_MENU_INFORMATION Data [properties thread]
        HandleMenuInitializing = 16, // PPH_PLUGIN_MENU_INFORMATION Data [properties thread]

        Maximum
    }

    [StructLayout(LayoutKind.Sequential)]
    public unsafe struct PhImageVersionInfo
    {
        public PhString* CompanyName;
        public PhString* FileDescription;
        public PhString* FileVersion;
        public PhString* ProductName;
    }

    public enum PhIntegrity
    {
        Untrusted = 0,
        Low = 1,
        Medium = 2,
        MediumPlus = 3,
        High = 4,
        System = 5,
        Installer = 6,
        Protected = 7,
        Secure = 8
    }

    public enum PhItemState
    {
        Normal = 0,
        New,
        Removing
    }

    [StructLayout(LayoutKind.Sequential)]
    public unsafe struct PhPluginMenuItem
    {
        public PhPlugin* Plugin;
        public uint Id;
        public uint RealId;
        public void* Context;
        public void* OwnerWindow;
    }

    [StructLayout(LayoutKind.Sequential)]
    public unsafe struct PhPlugin
    {
        public PhAvlLinks Links;

        public void* Name;
        public void* DllBase;
        public PhString* FileName;

        public void* DisplayName;
        public void* Author;
        public void* Description;
        public int Flags;

        public PhCallback Callbacks; // PhCallback[PhPluginCallback.Maximum]
    }

    /// <summary>
    /// Plugin Callback Types
    /// </summary>
    /// <remarks>All callbacks are invoked by the main Process Hacker thread.</remarks>
    public enum PhPluginCallback
    {
        /// <summary>
        /// Load callback type.
        /// </summary>
        Load = 0,

        /// <summary>
        /// Unload callback type.
        /// </summary>
        Unload = 1,

        /// <summary>
        /// Plugin Options callback type.
        /// </summary>
        ShowOptions = 2,

        /// <summary>
        /// MenuItem callback type.
        /// </summary>
        MenuItem = 3,

        /// <summary>
        /// Unused.
        /// </summary>
        Maximum = 4
    }

    [StructLayout(LayoutKind.Sequential)]
    public unsafe struct PhPluginGetHighlightingColor
    {
        public void* Parameter;
        public int BackColor;
        public byte Handled;
        public byte Cache;
    }

    [StructLayout(LayoutKind.Sequential)]
    public unsafe struct PhPluginInformation
    {
        public void* DisplayName;
        public void* Author;
        public void* Description;
        public void* Url;     
        [MarshalAs(UnmanagedType.I1)]
        public bool HasOptions;
        [MarshalAs(UnmanagedType.I1)]
        public fixed bool Reserved1[3];
    }

    [StructLayout(LayoutKind.Sequential)]
    public unsafe struct PhPluginProcessPropContext
    {
        public void* PropContext;
        public PhProcessItem* ProcessItem;
    }

    [StructLayout(LayoutKind.Sequential)]
    public unsafe struct PhPointerList
    {
        public int Count;
        public int AllocatedCount;
        public int FreeEntry;
        public int NextEntry;
        public void** Items;
    }

    [StructLayout(LayoutKind.Sequential)]
    public unsafe struct PhProcessItem
    {
        public PhHashEntry HashEntry;
        public int State;
        public PhProcessRecord* Record;

        public IntPtr ProcessId;
        public IntPtr ParentProcessId;
        public PhString* ProcessName;
        public int SessionId;

        public long CreateTime;

        public IntPtr QueryHandle;

        public PhString* FileName;
        public PhString* CommandLine;

        public IntPtr SmallIcon;
        public IntPtr LargeIcon;
        public PhImageVersionInfo VersionInfo;

        public PhString* UserName;
        public TokenElevationType ElevationType;
        public PhIntegrity IntegrityLevel;
        public void* IntegrityString;

        public PhString* JobName;
        public IntPtr ConsoleHostProcessId;

        public VerifyResult VerifyResult;
        public PhString* VerifySignerName;
        public int ImportFunctions;
        public int ImportModules;

        public int Flags;

        public byte JustProcessed;
        public PhEvent Stage1Event;

        public PhPointerList* ServiceList;
        public PhQueuedLock ServiceListLock;

        public fixed short ProcessIdString[NativeApi.PhInt32StrLen + 1];
        public fixed short ParentProcessIdString[NativeApi.PhInt32StrLen + 1];
        public fixed short SessionIdString[NativeApi.PhInt32StrLen + 1];

        public int BasePriority;
        public int PriorityClass;
        public long KernelTime;
        public long UserTime;
        public int NumberOfHandles;
        public int NumberOfThreads;

        public float CpuKernelUsage;
        public float CpuUserUsage;
        public float CpuUsage;

        public PhUInt64Delta CpuKernelDelta;
        public PhUInt64Delta CpuUserDelta;
        public PhUInt64Delta IoReadDelta;
        public PhUInt64Delta IoWriteDelta;
        public PhUInt64Delta IoOtherDelta;

        // More...
    }

    [StructLayout(LayoutKind.Sequential)]
    public unsafe struct PhProcessNode
    {
        public PhTreeListNode Node;

        public PhHashEntry HashEntry;

        public PhItemState State;
        public IntPtr Private1;
        public int Private2;

        public IntPtr ProcessId;
        public PhProcessItem* ProcessItem;

        // Other members are not exposed.
    }

    [StructLayout(LayoutKind.Sequential)]
    public unsafe struct PhProcessRecord
    {
        public ListEntry ListEntry;
        public int RefCount;
        public int Flags;

        public IntPtr ProcessId;
        public IntPtr ParentProcessId;
        public int SessionId;
        public long CreateTime;
        public long ExitTime;

        public PhString* ProcessName;
        public PhString* FileName;
        public PhString* CommandLine;
    }

    public unsafe delegate byte PhProcessTreeFilter(
        PhProcessNode* ProcessNode,
        [In, Optional] IntPtr Context
        );

    [StructLayout(LayoutKind.Sequential)]
    public struct PhQueuedLock
    {
        public IntPtr Value;
    }

    [StructLayout(LayoutKind.Sequential)]
    public unsafe struct PhSettingCreate
    {
        public PhSettingType Type;
        public void* Name;
        public void* DefaultValue;
    }

    public enum PhSettingType
    {
        String,
        Integer,
        IntegerPair
    }

    /// <summary>
    /// Menu Location types.
    /// </summary>
    public enum PhMenuLocationType
    {
        /// <summary>
        /// The View menu.
        /// </summary>
        View = 1,

        /// <summary>
        /// The Tools menu.
        /// </summary>
        Tools = 2
    }

    [StructLayout(LayoutKind.Sequential)]
    public unsafe struct PhString : IDisposable
    {
        public ushort Length;
        public ushort MaximumLength;
        public void* Pointer;
        public short Buffer;

        public string Text
        {
            get
            {
                fixed (void* buffer = &this.Buffer)
                    return Marshal.PtrToStringUni((IntPtr)buffer, this.Length / 2);
            }
        }

        public override string ToString()
        {
            return this.Text;
        }

        public void Dispose()
        {
            fixed (void* buffer = &this)
                NativeApi.PhDereferenceObject(buffer);
        }
    }

    [StructLayout(LayoutKind.Sequential)]
    public unsafe struct PhStringRef : IDisposable
    {
        public ushort Length;
        public ushort MaximumLength;
        public void* Buffer;

        //TODO: Untested.
        public string Text
        {
            get { return Marshal.PtrToStringUni((IntPtr)this.Buffer, this.Length / 2); }
        }

        public override string ToString()
        {
            return this.Text;
        }

        public void Dispose()
        {
            fixed (void* buffer = &this)
                NativeApi.PhDereferenceObject(buffer);
        }
    }

    [StructLayout(LayoutKind.Sequential)]
    public unsafe struct PhTreeListNode
    {
        public struct Unnamed_s
        {
            public int ViewIndex;
            public int ViewState;

            public byte IsLeaf;

            public int StateTickCount;

            public byte CachedColorValid;
            public byte CachedFontValid;
            public byte CachedIconValid;

            public int DrawBackColor;
            public int DrawForeColor;
        }

        public int Flags;

        public int BackColor;
        public int ForeColor;
        public int TempBackColor;
        public int ColorFlags;
        public IntPtr Font;
        public IntPtr Icon;

        public PhStringRef* TextCache;
        public int TextCacheSize;

        public int Level;

        public Unnamed_s s;
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct PhUInt64Delta
    {
        public ulong Value;
        public ulong Delta;
    }

    public enum VerifyResult
    {
        Unknown = 0,
        NoSignature,
        Trusted,
        Expired,
        Revoked,
        Distrust,
        SecuritySettings
    }

    #region Incomplete

    [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Unicode)]
    public struct PH_OBJECT_TYPE
    {
        /// ULONG->unsigned int
        public uint Flags;

        /// UCHAR->unsigned char
        public byte OffsetOfSecurityDescriptor;
        /// UCHAR->unsigned char
        public byte Reserved1;
        /// UCHAR->unsigned char
        public byte Reserved2;
        /// UCHAR->unsigned char
        public byte Reserved3;
        /// PPH_TYPE_DELETE_PROCEDURE
        public PPH_TYPE_DELETE_PROCEDURE DeleteProcedure;
        /// PWSTR->WCHAR*
        //[MarshalAs(UnmanagedType.LPWStr)]
        public string Name;
        /// ULONG->unsigned int
        public uint NumberOfObjects;
        /// PH_FREE_LIST->_PH_FREE_LIST
        public PH_FREE_LIST FreeList;
        /// GENERIC_MAPPING->_GENERIC_MAPPING
        public GENERIC_MAPPING GenericMapping;
    }

    /// <summary>
    /// The delete procedure for an object type, called when an object of the type is being freed.
    /// </summary>
    /// <param name="Object">A pointer to the object being freed.</param>
    /// <param name="Flags">Reserved.</param>
    public delegate void PPH_TYPE_DELETE_PROCEDURE(
        [In] IntPtr Object,
        [In] uint Flags
        );

    [StructLayout(LayoutKind.Sequential)]
    public struct PH_FREE_LIST
    {
        public SLIST_HEADER ListHead;
        public uint Count;
        public uint MaximumCount;
        public IntPtr Size;
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct GENERIC_MAPPING
    {
        /// ACCESS_MASK->DWORD->unsigned int
        public uint GenericRead;

        /// ACCESS_MASK->DWORD->unsigned int
        public uint GenericWrite;

        /// ACCESS_MASK->DWORD->unsigned int
        public uint GenericExecute;

        /// ACCESS_MASK->DWORD->unsigned int
        public uint GenericAll;
    }

    [StructLayout(LayoutKind.Explicit)]
    public struct SLIST_HEADER
    {
        [FieldOffset(0)]
        public ulong Alignment;

        [FieldOffset(0)]
        public Anonymous_fd626461_7f3e_49a1_aabe_a2b90f0df936 Struct1;
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct Anonymous_fd626461_7f3e_49a1_aabe_a2b90f0df936
    {
        /// SINGLE_LIST_ENTRY->_SINGLE_LIST_ENTRY
        public SINGLE_LIST_ENTRY Next;
        /// WORD->unsigned short
        public ushort Depth;
        /// WORD->unsigned short
        public ushort Sequence;
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct SINGLE_LIST_ENTRY
    {
        /// _SINGLE_LIST_ENTRY*
        public IntPtr Next;
    }

    #endregion

    [System.Security.SuppressUnmanagedCodeSecurity]
    public unsafe static partial class NativeApi
    {
        public const int PhInt32StrLen = 12;
        public const int PhInt64StrLen = 50;
        public const int PhIntegrityStrLen = 10;
        public const int PhPtrStrLen = 24;

        #region Application

        [DllImport("ProcessHacker.exe")]
        [return: MarshalAs(UnmanagedType.I1)]
        public static extern bool PhAddProcessPropPage2(
            void* PropContext,
            IntPtr PropSheetPageHandle
            );

        [DllImport("ProcessHacker.exe")]
        public static extern void* PhAddPropPageLayoutItem(
            IntPtr hwnd,
            IntPtr Handle,
            void* ParentItem,
            int Anchor
            );

        [DllImport("ProcessHacker.exe")]
        public static extern void PhDoPropPageLayout(
            IntPtr hwnd
            );

        #endregion

        #region Base Support (Processor-specific)

        //NOTE: FastCall is not supported by the CLR.

        [DllImport("ProcessHacker.exe", CallingConvention = CallingConvention.FastCall)]
        public static extern void PhxfAddInt32(
            int* A,
            int* B,
            int Count
            );

        //NOTE: FastCall is not supported by the CLR.

        [DllImport("ProcessHacker.exe", CallingConvention = CallingConvention.FastCall)]
        public static extern void PhxfAddInt32U(
            int* A,
            int* B,
            int Count
            );

        //NOTE: FastCall is not supported by the CLR.

        [DllImport("ProcessHacker.exe", CallingConvention = CallingConvention.FastCall)]
        public static extern void PhxfDivideSingleU(
            float* A,
            float* B,
            int Count
            );

        //NOTE: FastCall is not supported by the CLR.

        [DllImport("ProcessHacker.exe", CallingConvention = CallingConvention.FastCall)]
        public static extern void PhxfDivideSingle2U(
            float* A,
            float B,
            int Count
            );

        #endregion

        #region Callbacks

        /// <summary>
        /// Registers a callback function to be notified.
        /// </summary>
        /// <param name="Callback">
        /// A pointer to a callback object.
        /// </param>
        /// <param name="Function">
        /// The callback function.
        /// </param>
        /// <param name="Context">
        /// A user-defined value to pass to the callback function.
        /// </param>
        /// <param name="Registration">
        /// A variable which receives registration information for the callback. 
        /// Do not modify the contents of this structure and do not free the storage 
        /// for this structure until you have unregistered the callback.
        /// </param>
        [DllImport("ProcessHacker.exe")]
        public static extern void PhRegisterCallback(
            [In, Out] PhCallback* Callback,
            [In, MarshalAs(UnmanagedType.FunctionPtr)] PhCallbackFunction Function,
            [In, Optional] IntPtr Context,
            [Out] PhCallbackRegistration* Registration
            );

        /// <summary>
        /// Registers a callback function to be notified.
        /// </summary>
        /// <param name="Callback">
        /// A pointer to a callback object.
        /// </param>
        /// <param name="Function">
        /// The callback function.
        /// </param>
        /// <param name="Context">
        /// A user-defined value to pass to the callback function.
        /// </param>
        /// <param name="Flags">
        /// A combination of flags controlling the callback. Set this parameter to 0.
        /// </param>
        /// <param name="Registration">
        /// A variable which receives registration information for the callback. 
        /// Do not modify the contents of this structure and do not free the 
        /// storage for this structure until you have unregistered the callback.
        /// </param>
        [DllImport("ProcessHacker.exe")]
        public static extern void PhRegisterCallbackEx(
            [In, Out] PhCallback* Callback,
            [In, MarshalAs(UnmanagedType.FunctionPtr)] PhCallbackFunction Function,
            [In, Optional] IntPtr Context,
            [In] ushort Flags,
            [Out] PhCallbackRegistration* Registration
            );

        /// <summary>
        /// Unregisters a callback function.
        /// </summary>
        /// <param name="Callback">
        /// A pointer to a callback object.
        /// </param>
        /// <param name="Registration">
        /// The structure returned by PhRegisterCallback().
        /// </param>
        [DllImport("ProcessHacker.exe")]
        public static extern void PhUnregisterCallback(
            [In, Out] PhCallback* Callback,
            [In, Out] PhCallbackRegistration* Registration
            );

        #endregion

        #region Event

        //NOTE: FastCall is not supported by the CLR.
 
        [DllImport("ProcessHacker.exe", CallingConvention = CallingConvention.FastCall)]
        public static extern void PhfInitializeEvent(
            PhEvent* Event
            );

        //NOTE: FastCall is not supported by the CLR.

        [DllImport("ProcessHacker.exe", CallingConvention = CallingConvention.FastCall)]
        public static extern void PhfSetEvent(
            PhEvent* Event
            );

        //NOTE: FastCall is not supported by the CLR.

        [DllImport("ProcessHacker.exe", CallingConvention = CallingConvention.FastCall)]
        public static extern byte PhfWaitForEvent(
            PhEvent* Event,
            long* Timeout
            );

        //NOTE: FastCall is not supported by the CLR.

        [DllImport("ProcessHacker.exe", CallingConvention = CallingConvention.FastCall)]
        public static extern void PhfResetEvent(
            PhEvent* Event
            );

        #endregion

        #region Heap

        /// <summary>
        /// Allocates a block of memory.
        /// </summary>
        /// <param name="Size">
        /// The number of bytes to allocate.
        /// </param>
        /// <returns>A pointer to the allocated block of memory.</returns>
        /// <remarks>
        /// If the function fails to allocate the block of memory, it raises an exception. 
        /// The block is guaranteed to be aligned at MEMORY_ALLOCATION_ALIGNMENT bytes.
        /// </remarks>
        [DllImport("ProcessHacker.exe")]
        public static extern void* PhAllocate(
            [In] IntPtr Size
            );

        /// <summary>
        /// Frees a block of memory allocated with PhAllocate().
        /// </summary>
        /// <param name="Memory">
        /// A pointer to a block of memory.
        /// </param>
        [DllImport("ProcessHacker.exe")]
        public static extern void PhFree(
            [In] void* Memory
            );

        /// <summary>
        /// Re-allocates a block of memory originally allocated with PhAllocate().
        /// </summary>
        /// <param name="Memory">
        /// A pointer to a block of memory.
        /// </param>
        /// <param name="Size">
        /// The new size of the memory block, in bytes.
        /// </param>
        /// <returns>
        /// A pointer to the new block of memory. 
        /// The existing contents of the memory block are copied to the new block.
        /// </returns>
        /// <remarks>
        /// If the function fails to allocate the block of memory, it raises an exception.
        /// </remarks>
        [DllImport("ProcessHacker.exe")]
        public static extern void* PhReAllocate(
            [In] void* Memory,
            [In] IntPtr Size
            );

        /// <summary>
        /// Allocates pages of memory.
        /// </summary>
        /// <param name="Size">
        /// The number of bytes to allocate. 
        /// The number of pages allocated will be large enough to contain a Size bytes.
        /// </param>
        /// <param name="NewSize">
        /// The number of bytes actually allocated. 
        /// This is a Size rounded up to the next multiple of PAGE_SIZE.
        /// </param>
        /// <returns>
        /// A pointer to the allocated block of memory, or NULL
        /// if the block could not be allocated.
        /// </returns>
        [DllImport("ProcessHacker.exe")]
        public static extern void* PhAllocatePage(
            [In] IntPtr Size,
            [Out, Optional] out IntPtr* NewSize
            );

        /// <summary>
        /// Frees pages of memory allocated with PhAllocatePage().
        /// </summary>
        /// <param name="Memory">A pointer to a block of memory.</param>
        [DllImport("ProcessHacker.exe")]
        public static extern void PhFreePage(
            [In] void* Memory
            );

        #endregion

        #region Loader

        [DllImport("kernel32.dll", SetLastError = true)]
        public static extern void* GetProcAddress(
            [In] IntPtr hModule,
            [In] String procName
            );

        #endregion

        #region Native

        /// <summary>
        /// Opens a process.
        /// </summary>
        /// <param name="ProcessHandle">A variable which receives a handle to the process.</param>
        /// <param name="DesiredAccess">The desired access to the process.</param>
        /// <param name="ProcessId">The ID of the process.</param>
        /// <returns>A NtStatus indicating the result.</returns>
        [DllImport("ProcessHacker.exe")]
        public static extern NtStatus PhOpenProcess(
            [Out] out IntPtr* ProcessHandle,
            [In] int DesiredAccess,
            [In] IntPtr ProcessId
            );

        /// <summary>
        /// Terminates a process.
        /// </summary>
        /// <param name="ProcessHandle">
        /// A handle to a process. The handle must have PROCESS_TERMINATE access.
        /// </param>
        /// <param name="ExitStatus">
        /// A status value that indicates why the process is being terminated.
        /// </param>
        /// <returns>A NtStatus indicating the result.</returns>
        [DllImport("ProcessHacker.exe")]
        public static extern NtStatus PhTerminateProcess(
            [In] IntPtr ProcessHandle,
            [In] NtStatus ExitStatus
            );

        /// <summary>
        /// Suspends a process' threads.
        /// </summary>
        /// <param name="ProcessHandle">
        /// A handle to a process. The handle must have PROCESS_SUSPEND_RESUME access.</param>
        /// <returns>A NtStatus indicating the result.</returns>
        [DllImport("ProcessHacker.exe")]
        public static extern NtStatus PhSuspendProcess(
            [In] IntPtr ProcessHandle
            );

        /// <summary>
        /// Resumes a process' threads.
        /// </summary>
        /// <param name="ProcessHandle">
        /// A handle to a process. The handle must have PROCESS_SUSPEND_RESUME access.</param>
        /// <returns>A NtStatus indicating the result.</returns>
        [DllImport("ProcessHacker.exe")]
        public static extern NtStatus PhResumeProcess(
            [In] IntPtr ProcessHandle
            );

        /// <summary>
        /// Gets information for a handle.
        /// </summary>
        /// <param name="ProcessHandle">
        /// A handle to the process in which the handle resides.
        /// </param>
        /// <param name="Handle">The handle value.</param>
        /// <param name="ObjectTypeNumber">
        /// The object type number of the handle.
        /// You can specify -1 for this parameter if the object type number is not known.
        /// </param>
        /// <param name="BasicInformation">
        /// A variable which receives basic information about the object.
        /// </param>
        /// <param name="TypeName">
        /// A variable which receives the object type name.
        /// </param>
        /// <param name="ObjectName">
        /// A variable which receives the object name.
        /// </param>
        /// <param name="BestObjectName">
        /// A variable which receives the formatted object name.
        /// </param>
        /// <returns>
        /// A NtStatus indicating the result.
        /// STATUS_INVALID_HANDLE The handle specified in ProcessHandle or Handle is invalid. 
        /// STATUS_INVALID_PARAMETER_3 The value specified in ObjectTypeNumber is invalid.
        /// </returns>
        [DllImport("ProcessHacker.exe")]
        public static extern NtStatus PhGetHandleInformation(
            [In] IntPtr ProcessHandle,
            [In] IntPtr Handle,
            [In] int ObjectTypeNumber,
            [Out, Optional] ObjectBasicInformation* BasicInformation,
            [Out, Optional] PhString** TypeName,
            [Out, Optional] PhString** ObjectName,
            [Out, Optional] PhString** BestObjectName
            );

        #endregion

        #region Objects

        /// <summary>
        /// Dereferences the specified object.
        /// </summary>
        /// <param name="Object">A pointer to the object to dereference.</param>
        /// <remarks>The object will be freed if its reference count reaches 0.</remarks>
        [DllImport("ProcessHacker.exe")]
        public static extern void PhDereferenceObject(
            [In] void* Object
            );

        /// <summary>
        /// Dereferences the specified object.
        /// </summary>
        /// <param name="Object">A pointer to the object to dereference.</param>
        /// <returns>TRUE if the object was freed, otherwise FALSE.</returns>
        /// <remarks>The object will be freed in a worker thread if its reference count reaches 0.</remarks>
        [DllImport("ProcessHacker.exe")]
        [return: MarshalAs(UnmanagedType.I1)]
        public static extern bool PhDereferenceObjectDeferDelete(
            [In] void* Object
            );

        /// <summary>
        /// Dereferences the specified object.
        /// </summary>
        /// <param name="Object">A pointer to the object to dereference.</param>
        /// <param name="RefCount">The number of references to remove.</param>
        /// <param name="DeferDelete">Whether to defer deletion of the object.</param>
        /// <returns>The new reference count of the object.</returns>
        /// <remarks>The object will be freed if its reference count reaches 0.</remarks>
        [DllImport("ProcessHacker.exe")]
        public static extern int PhDereferenceObjectEx(
            [In] void* Object,
            [In] int RefCount,
            [In, MarshalAs(UnmanagedType.I1)] bool DeferDelete
            );

        /// <summary>
        /// Gets an object's type.
        /// </summary>
        /// <param name="Object">A pointer to an object.</param>
        /// <returns>A pointer to a type object.</returns>
        [DllImport("ProcessHacker.exe")]
        public static extern void* /* TODO: PH_OBJECT_TYPE */ PhGetObjectType(
            [In] void* Object
            );

        /// <summary>
        /// References the specified object.
        /// </summary>
        /// <param name="Object">A pointer to the object to reference.</param>
        [DllImport("ProcessHacker.exe")]
        public static extern void PhReferenceObject(
            [In] void* Object
            );

        /// <summary>
        /// References the specified object.
        /// </summary>
        /// <param name="Object">A pointer to the object to reference.</param>
        /// <param name="RefCount">The number of references to add.</param>
        /// <returns>The new reference count of the object.</returns>
        [DllImport("ProcessHacker.exe")]
        public static extern int PhReferenceObjectEx(
            [In] void* Object,
            [In] int RefCount
            );

        /// <summary>
        /// Attempts to reference an object and fails if it is being destroyed.
        /// </summary>
        /// <param name="Object">The object to reference if it is not being deleted.</param>
        /// <returns>
        /// TRUE if the object was referenced, FALSE if 
        /// it was being deleted and was not referenced.
        /// </returns>
        /// <remarks>
        /// This function is useful if a reference to an object is held, 
        /// protected by a mutex, and the delete procedure of the object's 
        /// type attempts to acquire the mutex. If this function is called 
        /// while the mutex is owned, you can avoid referencing an object 
        /// that is being destroyed.
        /// </remarks>
        [DllImport("ProcessHacker.exe")]
        public static extern byte PhReferenceObjectSafe(
            void* Object
            );

        #endregion

        #region Plugins

        /// <summary>
        /// Retrieves a pointer to a general callback.
        /// </summary>
        /// <param name="Callback">The type of callback.</param>
        /// <returns>A PhCallback structure.</returns>
        /// <remarks>The program invokes general callbacks for system-wide notifications.</remarks>
        [DllImport("ProcessHacker.exe")]
        public static extern PhCallback* PhGetGeneralCallback(
            [In] PhGeneralCallback Callback
            );

        /// <summary>
        /// Retrieves a pointer to a plugin callback.
        /// </summary>
        /// <param name="Plugin">A plugin instance structure.</param>
        /// <param name="Callback">The type of callback.</param>
        /// <returns>A PhCallback structure.</returns>
        /// <remarks>The program invokes plugin callbacks for notifications specific to a plugin.</remarks>
        [DllImport("ProcessHacker.exe")]
        public static extern PhCallback* PhGetPluginCallback(
            [In] PhPlugin* Plugin,
            [In] PhPluginCallback Callback
            );

        /// <summary>
        /// Registers a plugin with the host.
        /// </summary>
        /// <param name="Name">
        /// A unique identifier for the plugin. 
        /// The function fails if another plugin has already been registered with the same name.
        /// </param>
        /// <param name="DllBase">
        /// The base address of the plugin DLL.
        /// This is passed to the DllMain function.
        /// </param>
        /// <param name="Information">
        /// A variable which receives a pointer to the plugin's additional information block. 
        /// This should be filled in after the function returns.
        /// </param>
        /// <returns>A pointer to the plugin instance structure, or NULL if the function failed.</returns>
        [DllImport("ProcessHacker.exe", CharSet = CharSet.Unicode)]
        public static extern PhPlugin* PhRegisterPlugin(
            [In] String Name,
            [In] IntPtr DllBase,
            [In, Out, Optional] PhPluginInformation** Information
            );

        /// <summary>
        /// Adds a menu item to the program's main menu.
        /// </summary>
        /// <param name="Plugin">A plugin instance structure.</param>
        /// <param name="Location">
        /// A handle to the parent menu, or one of the following:
        /// PH_MENU_ITEM_LOCATION_VIEW The "View" menu.
        /// PH_MENU_ITEM_LOCATION_TOOLS The "Tools" menu.
        /// </param>
        /// <param name="InsertAfter">
        /// The text of the menu item to insert the new menu item after. 
        /// The search is a case-insensitive prefix search that ignores prefix characters (ampersands).
        /// </param>
        /// <param name="Id">
        /// An identifier for the menu item. This should be unique within the plugin. 
        /// You may also specify the following flags:
        /// PH_MENU_ITEM_SUB_MENU The menu item has a submenu.
        /// </param>
        /// <param name="Text">Text The text of the menu item.</param>
        /// <param name="Context">A user-defined value for the menu item.</param>
        /// <returns>
        /// TRUE if the function succeeded, otherwise FALSE.
        /// If PH_MENU_ITEM_SUB_MENU is specified in \a Flags, the return value is a handle to the submenu.
        /// </returns>
        /// <remarks>
        /// The \ref PluginCallbackMenuItem callback is invoked when the menu item is chosen, 
        /// and the \ref PH_PLUGIN_MENU_ITEM structure will contain the \a Id and \a Context values passed to this function. 
        /// </remarks>
        [DllImport("ProcessHacker.exe", CharSet = CharSet.Unicode)]
        public static extern IntPtr PhPluginAddMenuItem(
            [In, Out] PhPlugin* Plugin,
            [In] PhMenuLocationType Location,
            [In] String InsertAfter,
            [In] Int32 Id,
            [In] String Text,
            [In] IntPtr Context
            );

        #endregion

        #region Process Tree

        [DllImport("ProcessHacker.exe")]
        public static extern void* PhFindProcessNode(
            [In] IntPtr ProcessId
            );

        [DllImport("ProcessHacker.exe")]
        public static extern void PhUpdateProcessNode(
            [In] void* ProcessNode
            );

        [DllImport("ProcessHacker.exe")]
        public static extern void PhSelectAndEnsureVisibleProcessNode(
            [In] PhProcessNode* ProcessNode
            );

        [DllImport("ProcessHacker.exe")]
        public static extern void* PhAddProcessTreeFilter(
            [In, MarshalAs(UnmanagedType.FunctionPtr)] PhProcessTreeFilter Filter,
            [In, Optional] IntPtr Context
            );

        [DllImport("ProcessHacker.exe")]
        public static extern void PhRemoveProcessTreeFilter(
            [In] void* Entry
            );

        [DllImport("ProcessHacker.exe")]
        public static extern void PhApplyProcessTreeFilters();

        #endregion

        #region Providers

        /// <summary>
        /// Finds and references a process item.
        /// </summary>
        /// <param name="ProcessId">The process ID of the process item.</param>
        /// <returns>The found process item.</returns>
        [DllImport("ProcessHacker.exe")]
        public static extern PhProcessItem* PhReferenceProcessItem(
            [In] IntPtr ProcessId
            );

        /// <summary>
        /// Retrieves a time value recorded by the statistics system.
        /// </summary>
        /// <param name="ProcessItem">
        /// A process item to synchronize with, or NULL if no synchronization is necessary.
        /// </param>
        /// <param name="Index">The history index.</param>
        /// <param name="Time">A variable which receives the time at a Index.</param>
        /// <returns>
        /// TRUE if the function succeeded, otherwise FALSE if a ProcessItem 
        /// was specified and a Index is too far into the past for that process item.
        /// </returns>
        [DllImport("ProcessHacker.exe")]
        public static extern byte PhGetStatisticsTime(
            [In, Optional] PhProcessItem* ProcessItem,
            [In] int Index,
            [Out] out long* Time
            );

        [DllImport("ProcessHacker.exe")]
        public static extern PhString* PhGetStatisticsTimeString(
            [In, Optional] PhProcessItem* ProcessItem,
            [In] int Index
            );

        [DllImport("ProcessHacker.exe")]
        public static extern void PhReferenceProcessRecord(
            [In] PhProcessRecord* ProcessRecord
            );

        [DllImport("ProcessHacker.exe")]
        public static extern byte PhReferenceProcessRecordSafe(
            [In] PhProcessRecord* ProcessRecord
            );

        [DllImport("ProcessHacker.exe")]
        public static extern void PhDereferenceProcessRecord(
            [In] PhProcessRecord* ProcessRecord
            );

        [DllImport("ProcessHacker.exe")]
        public static extern PhProcessRecord* PhFindProcessRecord(
            [In, Optional] IntPtr ProcessId,
            [In] long* Time
            );

        #endregion

        #region Settings

        [DllImport("ProcessHacker.exe", CharSet = CharSet.Unicode)]
        public static extern int PhGetIntegerSetting(
            [In] String Name
            );

        [DllImport("ProcessHacker.exe", CharSet = CharSet.Unicode)]
        public static extern PhIntegerPair PhGetIntegerPairSetting(
            [In] String Name
            );

        [DllImport("ProcessHacker.exe", CharSet = CharSet.Unicode)]
        public static extern PhString* PhGetStringSetting(
            [In] String Name
            );

        [DllImport("ProcessHacker.exe", CharSet = CharSet.Unicode)]
        public static extern void PhSetIntegerSetting(
            [In] String Name,
            [In] Int32 Value
            );

        [DllImport("ProcessHacker.exe", CharSet = CharSet.Unicode)]
        public static extern void PhSetIntegerPairSetting(
            [In] String Name,
            [In] PhIntegerPair Value
            );

        [DllImport("ProcessHacker.exe", CharSet = CharSet.Unicode)]
        public static extern void PhSetStringSetting(
            [In] String Name,
            [In] String Value
            );

        [DllImport("ProcessHacker.exe", CharSet = CharSet.Unicode)]
        public static extern void PhSetStringSetting2(
            [In] String Name,
            [In] PhStringRef* Value
            );

        [DllImport("ProcessHacker.exe")]
        public static extern void PhAddSettings(
            [In] PhSettingCreate* Settings,
            [In] Int32 NumberOfSettings
            );

        #endregion

        #region String

        /// <summary>
        /// Creates a string object from an existing null-terminated string.
        /// </summary>
        /// <param name="Buffer">A null-terminated Unicode string.</param>
        /// <returns>A PhString structure containing the string.</returns>
        [DllImport("ProcessHacker.exe", CharSet = CharSet.Unicode)]
        public static extern PhString* PhCreateString(
            [In] String Buffer
            );

        /// <summary>
        /// Creates a string object using a specified length.
        /// </summary>
        /// <param name="Buffer">A null-terminated Unicode string.</param>
        /// <param name="Length">The length, in bytes, of the string.</param>
        /// <returns>A PhString structure containing the string.</returns>
        [DllImport("ProcessHacker.exe", CharSet = CharSet.Unicode)]
        public static extern PhString* PhCreateStringEx(
            [In, Optional] String Buffer,
            [In] IntPtr Length
            );

        /// <summary>
        /// Obtains a reference to a zero-length string.
        /// </summary>
        /// <returns>A PhString structure containing a zero-length string.</returns>
        [DllImport("ProcessHacker.exe")]
        public static extern PhString* PhReferenceEmptyString();

        #endregion

        #region Verify

        /// <summary>
        /// Verifies a file's digital signature.
        /// </summary>
        /// <param name="FileName">A file name.</param>
        /// <param name="SignerName">
        /// A variable which receives a pointer to a string 
        /// containing the signer name. You must free the string 
        /// using PhDereferenceObject() when you nolonger need it. 
        /// Note that the signer name may be NULL if it is not valid.
        /// </param>
        /// <returns>A VerifyResult value.</returns>
        [DllImport("ProcessHacker.exe", CharSet = CharSet.Unicode)]
        public static extern VerifyResult PhVerifyFile(
            [In] String FileName,
            [In, Out] PhString** SignerName
            );

        #endregion
    }
}
