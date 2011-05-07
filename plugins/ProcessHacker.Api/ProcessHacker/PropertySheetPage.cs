using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Data;
using System.Text;
using System.Windows.Forms;
using System.Runtime.InteropServices;

namespace ProcessHacker.Api
{
    public partial class PropertySheetPage : UserControl
    {
        private static List<PropertySheetPage> _keepAliveList = new List<PropertySheetPage>();

        public unsafe static IntPtr CreateDialogTemplate(int width, int height, string title, short fontSize, string fontName)
        {
            DlgTemplate* template;
            int templateStructSize;
            int offset;

            templateStructSize = Marshal.SizeOf(typeof(DlgTemplate));
            template = (DlgTemplate*)NativeApi.PhAllocate((IntPtr)(templateStructSize + 2 + 2 + (title.Length + 1) * 2 + 2 + (fontName.Length + 1) * 2));
            template->style = 0x40; // DS_SETFONT
            template->dwExtendedStyle = 0;
            template->cdit = 0;
            template->x = 0;
            template->y = 0;
            template->cx = (short)width;
            template->cy = (short)height;

            offset = templateStructSize;

            // No menu
            *(short*)((byte*)template + offset) = 0;
            offset += 2;

            // No class
            *(short*)((byte*)template + offset) = 0;
            offset += 2;

            // Title
            fixed (char* titlePtr = title)
                NativeApi.RtlMoveMemory((byte*)template + offset, titlePtr, (IntPtr)((title.Length + 1) * 2));
            offset += (title.Length + 1) * 2;

            // Font size
            *(short*)((byte*)template + offset) = fontSize;
            offset += 2;

            // Font name
            fixed (char* fontNamePtr = fontName)
                NativeApi.RtlMoveMemory((byte*)template + offset, fontNamePtr, (IntPtr)((fontName.Length + 1) * 2));
            offset += (fontName.Length + 1) * 2;

            return (IntPtr)template;
        }

        public event EventHandler PageGotFocus;
        public event EventHandler PageLostFocus;

        private IContainer components = null;
        private IntPtr _dialogWindowHandle;
        private IntPtr _dialogWindowParentHandle;

        public PropertySheetPage()
        {
            components = new System.ComponentModel.Container();
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
        }

        /// <summary> 
        /// Clean up any resources being used.
        /// </summary>
        /// <param name="disposing">true if managed resources should be disposed; otherwise, false.</param>
        protected override void Dispose(bool disposing)
        {
            if (disposing && (components != null))
            {
                components.Dispose();
            }

            base.Dispose(disposing);
        }

        public virtual IntPtr CreatePageHandle()
        {
            throw new NotImplementedException();
        }

        protected unsafe virtual bool DialogProc(IntPtr hwndDlg, WindowMessage uMsg, IntPtr wParam, IntPtr lParam)
        {
            switch (uMsg)
            {
                case WindowMessage.InitDialog:
                    {
                        _dialogWindowHandle = hwndDlg;
                        _dialogWindowParentHandle = NativeApi.GetParent(hwndDlg);
                        NativeApi.SetParent(this.Handle, hwndDlg);
                    }
                    break;
                case WindowMessage.Notify:
                    return OnDialogNotify(hwndDlg, wParam, lParam);
            }

            return false;
        }

        private unsafe bool OnDialogNotify(IntPtr hwndDlg, IntPtr wParam, IntPtr lParam)
        {
            NmHdr* hdr = (NmHdr*)lParam;

            if (hdr->hwndFrom != _dialogWindowParentHandle)
                return false;

            switch (hdr->code)
            {
                case (uint)PropSheetNotification.SetActive:
                    if (this.PageGotFocus != null)
                        this.PageGotFocus(this, new EventArgs());
                    break;
                case (uint)PropSheetNotification.KillActive:
                    if (this.PageLostFocus != null)
                        this.PageLostFocus(this, new EventArgs());
                    break;
            }

            return false;
        }

        protected unsafe int PropPageProc(IntPtr hwnd, PropSheetPageCallbackMessage uMsg, IntPtr ppsp)
        {
            switch (uMsg)
            {
                case PropSheetPageCallbackMessage.AddRef:
                    _keepAliveList.Add(this);
                    break;
                case PropSheetPageCallbackMessage.Release:
                    _keepAliveList.Remove(this);
                    this.Dispose();
                    break;
            }

            return 1;
        }
    }
}
