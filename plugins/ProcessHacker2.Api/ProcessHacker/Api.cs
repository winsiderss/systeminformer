using System;
using System.Runtime.InteropServices;

namespace ProcessHacker2.Api
{
    [StructLayout(LayoutKind.Sequential)]
    public unsafe struct PhAvlLinks
    {
        public PhAvlLinks* Parent;
        public PhAvlLinks* Left;
        public PhAvlLinks* Right;
        public int Balance;
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct PhCallback
    {
        public ListEntry ListHead;
        public PhQueuedLock ListLock;
        public PhQueuedLock BusyCondition;
    }

    public delegate void PhCallbackFunction(
        [In] [Optional] IntPtr Parameter,
        [In] [Optional] IntPtr Context
        );

    [StructLayout(LayoutKind.Sequential)]
    public struct PhCallbackRegistration
    {
        public ListEntry ListEntry;
        public IntPtr Function;
        public IntPtr Context;
        public int Busy;
        public byte Unregistering;
        public byte Reserved;
        public ushort Flags;
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
        MainWindowShowing = 0,
        ProcessesUpdated = 1,
        GetProcessHighlightingColor = 2,
        GetProcessTooltipText = 3,
        ProcessPropertiesInitializing = 4,
        GetIsDotNetDirectoryNames = 5,
        NotifyEvent = 6,
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
    public unsafe struct PhPlugin
    {
        public PhAvlLinks Links;

        public void* Name;
        public void* DllBase;

        public void* DisplayName;
        public void* Author;
        public void* Description;
        public byte HasOptions;

        public PhCallback Callbacks; // PhCallback[PhPluginCallback.Maximum]
    }

    public enum PhPluginCallback
    {
        Load = 0,
        Unload = 1,
        ShowOptions = 2,
        MenuItem = 3,
        Maximum
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
        public byte HasOptions;
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

        public PhString* JobName;
        public IntPtr ConsoleHostProcessId;

        public VerifyResult VerifyResult;
        public PhString* VerifySignerName;
        public int ImportFunctions;
        public int ImportModules;

        public int BitField;

        public byte JustProcessed;
        public PhEvent Stage1Event;

        public PhPointerList* ServiceList;
        public PhQueuedLock ServiceListLock;

        public fixed short ProcessIdString[NativeApi.PhInt32StrLen + 1];
        public fixed short ParentProcessIdString[NativeApi.PhInt32StrLen + 1];
        public fixed short SessionIdString[NativeApi.PhInt32StrLen + 1];
        public fixed short IntegrityString[NativeApi.PhIntegrityStrLen + 1];

        public int BasePriority;
        public int PriorityClassWin32;
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
        IntPtr Context
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

    [StructLayout(LayoutKind.Sequential)]
    public unsafe struct PhString
    {
        public ushort Length;
        public ushort MaximumLength;
        public void* Pointer;
        public short Buffer;

        public string Read()
        {
            fixed (void* buffer = &this.Buffer)
                return Marshal.PtrToStringUni((IntPtr)buffer, this.Length / 2);
        }
    }

    [StructLayout(LayoutKind.Sequential)]
    public unsafe struct PhStringRef
    {
        public ushort Length;
        public ushort MaximumLength;
        public void* Buffer;
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

    public unsafe static partial class NativeApi
    {
        public const int PhInt32StrLen = 12;
        public const int PhInt64StrLen = 50;
        public const int PhIntegrityStrLen = 10;
        public const int PhPtrStrLen = 24;

        #region Base Support (Processor-specific)

        [DllImport("ProcessHacker.exe", CallingConvention = CallingConvention.FastCall)]
        public static extern void PhxfAddInt32(
            int* A,
            int* B,
            int Count
            );

        [DllImport("ProcessHacker.exe", CallingConvention = CallingConvention.FastCall)]
        public static extern void PhxfAddInt32U(
            int* A,
            int* B,
            int Count
            );

        [DllImport("ProcessHacker.exe", CallingConvention = CallingConvention.FastCall)]
        public static extern void PhxfDivideSingleU(
            float* A,
            float* B,
            int Count
            );

        [DllImport("ProcessHacker.exe", CallingConvention = CallingConvention.FastCall)]
        public static extern void PhxfDivideSingle2U(
            float* A,
            float B,
            int Count
            );

        #endregion

        #region Callbacks

        [DllImport("ProcessHacker.exe")]
        public static extern void PhRegisterCallback(
            PhCallback* Callback,
            IntPtr Function,
            [Optional] IntPtr Context,
            PhCallbackRegistration* Registration
            );

        [DllImport("ProcessHacker.exe")]
        public static extern void PhRegisterCallbackEx(
            PhCallback* Callback,
            IntPtr Function,
            [Optional] IntPtr Context,
            ushort Flags,
            PhCallbackRegistration* Registration
            );

        [DllImport("ProcessHacker.exe")]
        public static extern void PhUnregisterCallback(
            PhCallback* Callback,
            PhCallbackRegistration* Registration
            );

        #endregion

        #region Event

        [DllImport("ProcessHacker.exe", CallingConvention = CallingConvention.FastCall)]
        public static extern void PhfInitializeEvent(
            PhEvent* Event
            );

        [DllImport("ProcessHacker.exe", CallingConvention = CallingConvention.FastCall)]
        public static extern void PhfSetEvent(
            PhEvent* Event
            );

        [DllImport("ProcessHacker.exe", CallingConvention = CallingConvention.FastCall)]
        public static extern byte PhfWaitForEvent(
            PhEvent* Event,
            long* Timeout
            );

        [DllImport("ProcessHacker.exe", CallingConvention = CallingConvention.FastCall)]
        public static extern void PhfResetEvent(
            PhEvent* Event
            );

        #endregion

        #region Heap

        [DllImport("ProcessHacker.exe")]
        public static extern void* PhAllocate(
            IntPtr Size
            );

        [DllImport("ProcessHacker.exe")]
        public static extern void PhFree(
            void* Memory
            );

        [DllImport("ProcessHacker.exe")]
        public static extern void* PhReAllocate(
            void* Memory,
            IntPtr Size
            );

        [DllImport("ProcessHacker.exe")]
        public static extern void* PhAllocatePage(
            IntPtr Size,
            [Optional] IntPtr* NewSize
            );

        [DllImport("ProcessHacker.exe")]
        public static extern void PhFreePage(
            void* Memory
            );

        #endregion

        #region Native

        [DllImport("ProcessHacker.exe")]
        public static extern NtStatus PhOpenProcess(
            IntPtr* ProcessHandle,
            int DesiredAccess,
            IntPtr ProcessId
            );

        [DllImport("ProcessHacker.exe")]
        public static extern NtStatus PhTerminateProcess(
            IntPtr ProcessHandle,
            NtStatus ExitStatus
            );

        [DllImport("ProcessHacker.exe")]
        public static extern NtStatus PhSuspendProcess(
            IntPtr ProcessHandle
            );

        [DllImport("ProcessHacker.exe")]
        public static extern NtStatus PhResumeProcess(
            IntPtr ProcessHandle
            );

        [DllImport("ProcessHacker.exe")]
        public static extern NtStatus PhGetHandleInformation(
            IntPtr ProcessHandle,
            IntPtr Handle,
            int ObjectTypeNumber,
            ObjectBasicInformation* BasicInformation,
            PhString** TypeName,
            PhString** ObjectName,
            PhString** BestObjectName
            );

        #endregion

        #region Objects

        [DllImport("ProcessHacker.exe")]
        public static extern void PhDereferenceObject(
            void* Object
            );

        [DllImport("ProcessHacker.exe")]
        public static extern byte PhDereferenceObjectDeferDelete(
            void* Object
            );

        [DllImport("ProcessHacker.exe")]
        public static extern int PhDereferenceObjectEx(
            void* Object,
            int RefCount,
            byte DeferDelete
            );

        [DllImport("ProcessHacker.exe")]
        public static extern void* PhGetObjectType(
            void* Object
            );

        [DllImport("ProcessHacker.exe")]
        public static extern void PhReferenceObject(
            void* Object
            );

        [DllImport("ProcessHacker.exe")]
        public static extern int PhReferenceObjectEx(
            void* Object,
            int RefCount
            );

        [DllImport("ProcessHacker.exe")]
        public static extern byte PhReferenceObjectSafe(
            void* Object
            );

        #endregion

        #region Plugins

        [DllImport("ProcessHacker.exe")]
        public static extern PhCallback* PhGetGeneralCallback(
            PhGeneralCallback Callback
            );

        [DllImport("ProcessHacker.exe")]
        public static extern PhCallback* PhGetPluginCallback(
            PhPlugin* Plugin,
            PhPluginCallback Callback
            );

        [DllImport("ProcessHacker.exe", CharSet = CharSet.Unicode)]
        public static extern PhPlugin* PhRegisterPlugin(
            string Name,
            IntPtr DllBase,
            [Optional] PhPluginInformation* Information
            );

        #endregion

        #region Process Tree

        [DllImport("ProcessHacker.exe")]
        public static extern void* PhFindProcessNode(
            IntPtr ProcessId
            );

        [DllImport("ProcessHacker.exe")]
        public static extern void PhUpdateProcessNode(
            void* ProcessNode
            );

        [DllImport("ProcessHacker.exe")]
        public static extern void PhSelectAndEnsureVisibleProcessNode(
            PhProcessNode* ProcessNode
            );

        [DllImport("ProcessHacker.exe")]
        public static extern void* PhAddProcessTreeFilter(
            IntPtr Filter,
            [Optional] IntPtr Context
            );

        [DllImport("ProcessHacker.exe")]
        public static extern void PhRemoveProcessTreeFilter(
            void* Entry
            );

        [DllImport("ProcessHacker.exe")]
        public static extern void PhApplyProcessTreeFilters();

        #endregion

        #region Providers

        [DllImport("ProcessHacker.exe")]
        public static extern PhProcessItem* PhReferenceProcessItem(
            IntPtr ProcessId
            );

        [DllImport("ProcessHacker.exe")]
        public static extern byte PhGetStatisticsTime(
            [Optional] PhProcessItem* ProcessItem,
            int Index,
            long* Time
            );

        [DllImport("ProcessHacker.exe")]
        public static extern PhString* PhGetStatisticsTimeString(
            [Optional] PhProcessItem* ProcessItem,
            int Index
            );

        [DllImport("ProcessHacker.exe")]
        public static extern void PhReferenceProcessRecord(
            PhProcessRecord* ProcessRecord
            );

        [DllImport("ProcessHacker.exe")]
        public static extern byte PhReferenceProcessRecordSafe(
            PhProcessRecord* ProcessRecord
            );

        [DllImport("ProcessHacker.exe")]
        public static extern void PhDereferenceProcessRecord(
            PhProcessRecord* ProcessRecord
            );

        [DllImport("ProcessHacker.exe")]
        public static extern PhProcessRecord* PhFindProcessRecord(
            [Optional] IntPtr ProcessId,
            long* Time
            );

        #endregion

        #region Settings

        [DllImport("ProcessHacker.exe", CharSet = CharSet.Unicode)]
        public static extern int PhGetIntegerSetting(
            string Name
            );

        [DllImport("ProcessHacker.exe", CharSet = CharSet.Unicode)]
        public static extern PhIntegerPair PhGetIntegerPairSetting(
            string Name
            );

        [DllImport("ProcessHacker.exe", CharSet = CharSet.Unicode)]
        public static extern PhString* PhGetStringSetting(
            string Name
            );

        [DllImport("ProcessHacker.exe", CharSet = CharSet.Unicode)]
        public static extern void PhSetIntegerSetting(
            string Name,
            int Value
            );

        [DllImport("ProcessHacker.exe", CharSet = CharSet.Unicode)]
        public static extern void PhSetIntegerPairSetting(
            string Name,
            PhIntegerPair Value
            );

        [DllImport("ProcessHacker.exe", CharSet = CharSet.Unicode)]
        public static extern void PhSetStringSetting(
            string Name,
            string Value
            );

        [DllImport("ProcessHacker.exe", CharSet = CharSet.Unicode)]
        public static extern void PhSetStringSetting2(
            string Name,
            PhStringRef* Value
            );

        [DllImport("ProcessHacker.exe")]
        public static extern void PhAddSettings(
            PhSettingCreate* Settings,
            int NumberOfSettings
            );

        #endregion

        #region String

        [DllImport("ProcessHacker.exe")]
        public static extern PhString* PhCreateString(
            void* Buffer
            );

        [DllImport("ProcessHacker.exe", CharSet = CharSet.Unicode)]
        public static extern PhString* PhCreateString(
            string Buffer
            );

        [DllImport("ProcessHacker.exe", CharSet = CharSet.Unicode)]
        public static extern PhString* PhCreateStringEx(
            [Optional] void* Buffer,
            IntPtr Length
            );

        [DllImport("ProcessHacker.exe")]
        public static extern PhString* PhReferenceEmptyString();

        #endregion

        #region Verify

        [DllImport("ProcessHacker.exe", CharSet = CharSet.Unicode)]
        public static extern VerifyResult PhVerifyFile(
            string FileName,
            PhString** SignerName
            );

        #endregion
    }
}
