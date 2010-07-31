/*
 * Process Hacker - 
 *   function import module
 * 
 * Copyright (C) 2009-2010 wj32
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

#define NTIMPORT_PRIVATE
#include <ph.h>

#define InitProc(DllHandle, ProcName) ((ProcName) = (_##ProcName)GetProcAddress(DllHandle, #ProcName))

BOOLEAN PhInitializeImports()
{
    PVOID ntdll;

    ntdll = GetModuleHandle(L"ntdll.dll");

#if !(PHNT_VERSION >= PHNT_WS03)
    InitProc(ntdll, NtGetNextProcess);
    InitProc(ntdll, NtGetNextThread);
#endif

#if !(PHNT_VERSION >= PHNT_VISTA)
    InitProc(ntdll, NtQueryInformationEnlistment);
    InitProc(ntdll, NtQueryInformationResourceManager);
    InitProc(ntdll, NtQueryInformationTransaction);
    InitProc(ntdll, NtQueryInformationTransactionManager);
#endif

    return TRUE;
}
