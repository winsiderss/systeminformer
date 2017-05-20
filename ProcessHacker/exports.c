/*
 * Process Hacker -
 *   exported variables
 *
 * Copyright (C) 2017 dmex
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

#include <phapp.h>
#include <mainwnd.h>
#include <procprv.h>
#include <srvprv.h>
#include <netprv.h>

HFONT PhGetApplicationFont(
    VOID
    )
{
    return PhApplicationFont;
}

HWND PhGetMainWndHandle(
    VOID
    )
{
    return PhMainWndHandle;
}

PVOID PhGetImageBase(
    VOID
    )
{
    return PhLibImageBase;
}

ULONG PhGetGlobalDpi(
    VOID
    )
{
    return PhGlobalDpi;
}

SYSTEM_BASIC_INFORMATION PhGetSystemBasicInformation(
    VOID
    )
{
    return PhSystemBasicInformation;
}

ACCESS_MASK PhProcessQueryAccess(
    VOID
    )
{
    return ProcessQueryAccess;
}

ACCESS_MASK PhProcessAllAccess(
    VOID
    )
{
    return ProcessAllAccess;
}

ACCESS_MASK PhThreadQueryAccess(
    VOID
    )
{
    return ThreadQueryAccess;
}

ACCESS_MASK PhThreadSetAccess(
    VOID
    )
{
    return ThreadSetAccess;
}

ACCESS_MASK PhThreadAllAccess(
    VOID
    )
{
    return ThreadAllAccess;
}