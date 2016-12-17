/*
 * Process Hacker -
 *   Search control stub
 *
 * Copyright (C) 2016 dmex
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
 *
 */

#include "toolstatus.h"
#include "commonutil.h"

BOOLEAN CreateSearchboxControl(
    _In_ UINT CommandID
    )
{
    if (SearchboxHandle = CreateWindowEx(
        WS_EX_CLIENTEDGE,
        WC_EDIT,
        NULL,
        WS_CHILD | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | ES_LEFT | ES_AUTOHSCROLL | WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
        RebarHandle,
        NULL,
        NULL,
        NULL
        ))
    {
        CreateSearchControl(RebarHandle, SearchboxHandle, CommandID);
        return TRUE;
    }

    return FALSE;
}