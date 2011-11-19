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

#include <peview.h>
#include <cpysave.h>

// Copied from appsup.c

VOID PvCopyListView(
    __in HWND ListViewHandle
    )
{
    PPH_STRING text;

    text = PhGetListViewText(ListViewHandle);
    PhSetClipboardStringEx(ListViewHandle, text->Buffer, text->Length);
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
