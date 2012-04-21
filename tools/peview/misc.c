/*
 * Process Hacker -
 *   PE viewer
 *
 * Copyright (C) 2011 wj32
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

#define CINTERFACE
#define COBJMACROS
#include <peview.h>
#include <shobjidl.h>
#undef CINTERFACE
#undef COBJMACROS
#include <cpysave.h>

static GUID CLSID_ShellLink_I = { 0x00021401, 0x0000, 0x0000, { 0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46 } };
static GUID IID_IShellLinkW_I = { 0x000214f9, 0x0000, 0x0000, { 0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46 } };
static GUID IID_IPersistFile_I = { 0x0000010b, 0x0000, 0x0000, { 0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46 } };

PPH_STRING PvResolveShortcutTarget(
    __in PPH_STRING ShortcutFileName
    )
{
    PPH_STRING targetFileName;
    IShellLinkW *shellLink;
    IPersistFile *persistFile;
    WCHAR path[MAX_PATH];

    targetFileName = NULL;

    if (SUCCEEDED(CoCreateInstance(&CLSID_ShellLink_I, NULL, CLSCTX_INPROC_SERVER, &IID_IShellLinkW_I, &shellLink)))
    {
        if (SUCCEEDED(IShellLinkW_QueryInterface(shellLink, &IID_IPersistFile_I, &persistFile)))
        {
            if (SUCCEEDED(IPersistFile_Load(persistFile, ShortcutFileName->Buffer, STGM_READ)) &&
                SUCCEEDED(IShellLinkW_Resolve(shellLink, NULL, SLR_NO_UI)))
            {
                if (SUCCEEDED(IShellLinkW_GetPath(shellLink, path, MAX_PATH, NULL, 0)))
                {
                    targetFileName = PhCreateString(path);
                }
            }

            IPersistFile_Release(persistFile);
        }

        IShellLinkW_Release(shellLink);
    }

    return targetFileName;
}

// Copied from appsup.c

VOID PvCopyListView(
    __in HWND ListViewHandle
    )
{
    PPH_STRING text;

    text = PhGetListViewText(ListViewHandle);
    PhSetClipboardString(ListViewHandle, &text->sr);
    PhDereferenceObject(text);
}

VOID PvHandleListViewNotifyForCopy(
    __in LPARAM lParam,
    __in HWND ListViewHandle
    )
{
    if (((LPNMHDR)lParam)->hwndFrom == ListViewHandle)
    {
        LPNMLVKEYDOWN keyDown = (LPNMLVKEYDOWN)lParam;

        switch (keyDown->wVKey)
        {
        case 'C':
            if (GetKeyState(VK_CONTROL) < 0)
                PvCopyListView(ListViewHandle);
            break;
        }
    }
}
