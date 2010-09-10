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
    public struct PhPlugin
    {
        public PhAvlLinks Links;

        public IntPtr Name; // PWSTR
        public IntPtr DllBase;

        public IntPtr DisplayName; // PWSTR
        public IntPtr Author; // PWSTR
        public IntPtr Description; // PWSTR
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
    public struct PhPluginInformation
    {
        public IntPtr DisplayName;
        public IntPtr Author;
        public IntPtr Description; 
        public byte HasOptions;
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct PhQueuedLock
    {
        public IntPtr Value;
    }

    public unsafe static partial class NativeApi
    {
        #region Callbacks

        [DllImport("ProcessHacker.exe")]
        public static extern void PhRegisterCallback(
            [In] PhCallback* Callback,
            [In] IntPtr Function,
            [In] [Optional] IntPtr Context,
            [In] PhCallbackRegistration* Registration
            );

        [DllImport("ProcessHacker.exe")]
        public static extern void PhRegisterCallbackEx(
            [In] PhCallback* Callback,
            [In] IntPtr Function,
            [In] [Optional] IntPtr Context,
            [In] ushort Flags,
            [In] PhCallbackRegistration* Registration
            );

        [DllImport("ProcessHacker.exe")]
        public static extern void PhUnregisterCallback(
            [In] PhCallback* Callback,
            [In] PhCallbackRegistration* Registration
            );

        #endregion

        #region Plugins

        [DllImport("ProcessHacker.exe")]
        public static extern PhCallback* PhGetGeneralCallback(
            [In] PhGeneralCallback Callback
            );

        [DllImport("ProcessHacker.exe")]
        public static extern PhCallback* PhGetPluginCallback(
            [In] PhPlugin* Plugin,
            [In] PhPluginCallback Callback
            );

        [DllImport("ProcessHacker.exe", CharSet = CharSet.Unicode)]
        public static extern PhPlugin* PhRegisterPlugin(
            [In] string Name,
            [In] IntPtr DllBase,
            [In] [Optional] PhPluginInformation* Information
            );

        #endregion
    }
}
