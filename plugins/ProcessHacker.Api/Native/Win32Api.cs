/*
 * Process Hacker - 
 *   Win32 definitions
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

namespace ProcessHacker.Api
{
    [StructLayout(LayoutKind.Sequential, Pack = 1)]
    public struct DlgTemplate
    {
        public int style;
        public int dwExtendedStyle;
        public short cdit;
        public short x;
        public short y;
        public short cx;
        public short cy;
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct NmHdr
    {
        public IntPtr hwndFrom;
        public IntPtr idFrom;
        public uint code;
    }

    public enum PropSheetNotification : uint
    {
        First = ~200u + 1, // -200
        SetActive = First - 0,
        KillActive = First - 1
    }

    public enum PropSheetPageCallbackMessage
    {
        AddRef = 0,
        Release = 1,
        Create = 2
    }

    [Flags]
    public enum PropSheetPageFlags
    {
        Default = 0x0,
        DlgIndirect = 0x1,
        UseHIcon = 0x2,
        UseIconID = 0x4,
        UseTitle = 0x8,
        RtlReading = 0x10,

        HasHelp = 0x20,
        UseRefParent = 0x40,
        UseCallback = 0x80,
        Premature = 0x400,

        HideHeader = 0x800,
        UseHeaderTitle = 0x1000,
        UseHeaderSubTitle = 0x2000,
        UseFusionContext = 0x4000
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct PropSheetPageW
    {
        public int dwSize;
        public PropSheetPageFlags dwFlags;
        public IntPtr hInstance;

        public IntPtr pResource; // pszTemplate
        public IntPtr hIcon;
        public IntPtr pszTitle;
        public IntPtr pfnDlgProc;
        public IntPtr lParam;
        public IntPtr pfnCallback;
        public IntPtr pcRefParent;
        public IntPtr pszHeaderTitle;
        public IntPtr pszHeaderSubTitle;
        public IntPtr hActCtx;
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct Rect
    {
        public int left;
        public int top;
        public int right;
        public int bottom;
    }

    public enum WindowMessage : uint
    {
        Null = 0x00,
        Create = 0x01,
        Destroy = 0x02,
        Move = 0x03,
        Size = 0x05,
        Activate = 0x06,
        SetFocus = 0x07,
        KillFocus = 0x08,
        Enable = 0x0a,
        SetRedraw = 0x0b,
        SetText = 0x0c,
        GetText = 0x0d,
        GetTextLength = 0x0e,
        Paint = 0x0f,
        Close = 0x10,
        QueryEndSession = 0x11,
        Quit = 0x12,
        QueryOpen = 0x13,
        EraseBkgnd = 0x14,
        SysColorChange = 0x15,
        EndSession = 0x16,
        SystemError = 0x17,
        ShowWindow = 0x18,
        CtlColor = 0x19,
        WinIniChange = 0x1a,
        SettingChange = 0x1a,
        DevModeChange = 0x1b,
        ActivateApp = 0x1c,
        FontChange = 0x1d,
        TimeChange = 0x1e,
        CancelMode = 0x1f,
        SetCursor = 0x20,
        MouseActivate = 0x21,
        ChildActivate = 0x22,
        QueueSync = 0x23,
        GetMinMaxInfo = 0x24,
        PaintIcon = 0x26,
        IconEraseBkgnd = 0x27,
        NextDlgCtl = 0x28,
        SpoolerStatus = 0x2a,
        DrawIcon = 0x2b,
        MeasureItem = 0x2c,
        DeleteItem = 0x2d,
        VKeyToItem = 0x2e,
        CharToItem = 0x2f,

        SetFont = 0x30,
        GetFont = 0x31,
        SetHotkey = 0x32,
        GetHotkey = 0x33,
        QueryDragIcon = 0x37,
        CompareItem = 0x39,
        Compacting = 0x41,
        WindowPosChanging = 0x46,
        WindowPosChanged = 0x47,
        Power = 0x48,
        CopyData = 0x4a,
        CancelJournal = 0x4b,
        Notify = 0x4e,
        InputLangChangeRequest = 0x50,
        InputLangChange = 0x51,
        TCard = 0x52,
        Help = 0x53,
        UserChanged = 0x54,
        NotifyFormat = 0x55,
        ContextMenu = 0x7b,
        StyleChanging = 0x7c,
        StyleChanged = 0x7d,
        DisplayChange = 0x7e,
        GetIcon = 0x7f,
        SetIcon = 0x80,

        NcCreate = 0x81,
        NcDestroy = 0x82,
        NcCalcSize = 0x83,
        NcHitTest = 0x84,
        NcPaint = 0x85,
        NcActivate = 0x86,
        GetDlgCode = 0x87,
        NcMouseMove = 0xa0,
        NcLButtonDown = 0xa1,
        NcLButtonUp = 0xa2,
        NcLButtonDblClk = 0xa3,
        NcRButtonDown = 0xa4,
        NcRButtonUp = 0xa5,
        NcRButtonDblClk = 0xa6,
        NcMButtonDown = 0xa7,
        NcMButtonUp = 0xa8,
        NcMButtonDblClk = 0xa9,

        KeyDown = 0x100,
        KeyUp = 0x101,
        Char = 0x102,
        DeadChar = 0x103,
        SysKeyDown = 0x104,
        SysKeyUp = 0x105,
        SysChar = 0x106,
        SysDeadChar = 0x107,

        ImeStartComposition = 0x10d,
        ImeEndComposition = 0x10e,
        ImeComposition = 0x10f,
        ImeKeyLast = 0x10f,

        InitDialog = 0x110,
        Command = 0x111,
        SysCommand = 0x112,
        Timer = 0x113,
        HScroll = 0x114,
        VScroll = 0x115,
        InitMenu = 0x116,
        InitMenuPopup = 0x117,
        MenuSelect = 0x11f,
        MenuChar = 0x120,
        EnterIdle = 0x121,

        CtlColorMsgBox = 0x132,
        CtlColorEdit = 0x133,
        CtlColorListBox = 0x134,
        CtlColorBtn = 0x135,
        CtlColorDlg = 0x136,
        CtlColorScrollbar = 0x137,
        CtlColorStatic = 0x138,

        MouseMove = 0x200,
        LButtonDown = 0x201,
        LButtonUp = 0x202,
        LButtonDblClk = 0x203,
        RButtonDown = 0x204,
        RButtonUp = 0x205,
        RButtonDblClk = 0x206,
        MButtonDown = 0x207,
        MButtonUp = 0x208,
        MButtonDblClk = 0x209,
        MouseWheel = 0x20a,

        ParentNotify = 0x210,
        EnterMenuLoop = 0x211,
        ExitMenuLoop = 0x212,
        NextMenu = 0x213,
        Sizing = 0x214,
        CaptureChanged = 0x215,
        Moving = 0x216,
        PowerBroadcast = 0x218,
        DeviceChange = 0x219,

        MdiCreate = 0x220,
        MdiDestroy = 0x221,
        MdiActivate = 0x222,
        MdiRestore = 0x223,
        MdiNext = 0x224,
        MdiMaximize = 0x225,
        MdiTile = 0x226,
        MdiCascade = 0x227,
        MdiIconArrange = 0x228,
        MdiGetActive = 0x229,
        MdiSetMenu = 0x230,
        EnterSizeMove = 0x231,
        ExitSizeMove = 0x232,
        DropFiles = 0x233,
        MdiRefreshMenu = 0x234,

        ImeSetContext = 0x281,
        ImeNotify = 0x282,
        ImeControl = 0x283,
        ImeCompositionFull = 0x284,
        ImeSelect = 0x285,
        ImeChar = 0x286,
        ImeKeyDown = 0x290,
        ImeKeyUp = 0x291,

        NcMouseHover = 0x2a0,
        MouseHover = 0x2a1,
        NcMouseLeave = 0x2a2,
        MouseLeave = 0x2a3,

        WtsSessionChange = 0x2b1,

        TabletFirst = 0x2c0,
        TabletLast = 0x2df,

        Cut = 0x300,
        Copy = 0x301,
        Paste = 0x302,
        Clear = 0x303,
        Undo = 0x304,

        RenderFormat = 0x305,
        RenderAllFormats = 0x306,
        DestroyClipboard = 0x307,
        DrawClipboard = 0x308,
        PaintClipboard = 0x309,
        VScrollClipboard = 0x30a,
        SizeClipboard = 0x30b,
        AskCbFormatName = 0x30c,
        ChangeCbChain = 0x30d,
        HScrollClipboard = 0x30e,
        QueryNewPalette = 0x30f,
        PaletteIsChanging = 0x310,
        PaletteChanged = 0x311,

        Hotkey = 0x312,
        Print = 0x317,
        PrintClient = 0x318,

        DwmSendIconicThumbnail = 0x323,
        DwmSendIconicLivePreviewBitmap = 0x326,

        HandheldFirst = 0x358,
        HandheldLast = 0x35f,
        PenWinFirst = 0x380,
        PenWinLast = 0x38f,
        CoalesceFirst = 0x390,
        CoalesceLast = 0x39f,
        DdeInitiate = 0x3e0,
        DdeTerminate = 0x3e1,
        DdeAdvise = 0x3e2,
        DdeUnadvise = 0x3e3,
        DdeAck = 0x3e4,
        DdeData = 0x3e5,
        DdeRequest = 0x3e6,
        DdePoke = 0x3e7,
        DdeExecute = 0x3e8,

        User = 0x400,

        Reflect = User + 0x1c00,

        BcmSetShield = 0x160c,

        App = 0x8000
    }

    public delegate int PropSheetPageCallback(IntPtr hwnd, PropSheetPageCallbackMessage uMsg, IntPtr ppsp);
    public delegate bool DialogProc(IntPtr hwndDlg, WindowMessage uMsg, IntPtr wParam, IntPtr lParam);

    [System.Security.SuppressUnmanagedCodeSecurity]
    public static partial class NativeApi
    {
        [DllImport("comctl32.dll")]
        public static extern IntPtr CreatePropertySheetPageW(
            ref PropSheetPageW lppsp
            );

        [DllImport("user32.dll")]
        public static extern bool MapDialogRect(
            IntPtr hDlg,
            ref Rect lpRect
            );

        [DllImport("user32.dll")]
        public static extern IntPtr GetParent(
            IntPtr hWnd
            );

        [DllImport("user32.dll")]
        public static extern IntPtr SetParent(
            IntPtr hWndChild,
            IntPtr hWndNewParent
            );
    }
}
