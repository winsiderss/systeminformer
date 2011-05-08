/*
 * Process Hacker - 
 *   Process Property Sheet Control
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
    public class ProcessPropertySheetPage : PropertySheetPage
    {
        private IntPtr _pageHandle;
        private IntPtr _dialogTemplate;
        private DialogProc _dialogProc;
        private PropSheetPageCallback _pagePageProc;
        private bool _layoutInitialized;

        public ProcessPropertySheetPage()
        {
            _dialogProc = new DialogProc(this.DialogProc);
            _pagePageProc = new PropSheetPageCallback(this.PropPageProc);
        }

        protected override void Dispose(bool disposing)
        {
            unsafe
            {
                NativeApi.PhFree((void*)_dialogTemplate);
            }

            base.Dispose(disposing);
        }

        public override IntPtr CreatePageHandle()
        {
            if (_pageHandle != IntPtr.Zero)
                return _pageHandle;

            PropSheetPageW psp = new PropSheetPageW();

            // *Must* be 260x260. See PhAddPropPageLayoutItem in procprp.c.
            _dialogTemplate = CreateDialogTemplate(260, 260, this.Text, 8, "MS Shell Dlg");

            psp.dwSize = Marshal.SizeOf(typeof(PropSheetPageW));
            psp.dwFlags = PropSheetPageFlags.UseCallback | PropSheetPageFlags.DlgIndirect;

            psp.pResource = _dialogTemplate;

            psp.pfnDlgProc = Marshal.GetFunctionPointerForDelegate(_dialogProc);
            psp.pfnCallback = Marshal.GetFunctionPointerForDelegate(_pagePageProc);

            _pageHandle = NativeApi.CreatePropertySheetPageW(ref psp);

            return _pageHandle;
        }

        protected override unsafe bool DialogProc(IntPtr hwndDlg, WindowMessage uMsg, IntPtr wParam, IntPtr lParam)
        {
            switch (uMsg)
            {
                case WindowMessage.InitDialog:
                    {
                        Rect initialSize;

                        initialSize.left = 0;
                        initialSize.top = 0;
                        initialSize.right = 260;
                        initialSize.bottom = 260;
                        NativeApi.MapDialogRect(hwndDlg, ref initialSize);
                        this.Size = new System.Drawing.Size(initialSize.right, initialSize.bottom);
                    }
                    break;
                case WindowMessage.ShowWindow:
                    if (!_layoutInitialized)
                    {
                        void* dialogItem;
                        void* controlItem;

                        dialogItem = NativeApi.PhAddPropPageLayoutItem(
                            hwndDlg,
                            hwndDlg,
                            (void*)0x1, // PH_PROP_PAGE_TAB_CONTROL_PARENT
                            0xf // PH_ANCHOR_ALL
                            );

                        // Resize our .NET-based control automatically as well.
                        controlItem = NativeApi.PhAddPropPageLayoutItem(
                            hwndDlg,
                            this.Handle,
                            dialogItem,
                            0xf // PH_ANCHOR_ALL
                            );

                        NativeApi.PhDoPropPageLayout(hwndDlg);

                        _layoutInitialized = true;
                    }
                    break;
            }

            return base.DialogProc(hwndDlg, uMsg, wParam, lParam);
        }
    }
}
