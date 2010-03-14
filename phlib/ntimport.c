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
#include <ntimport.h>
#include <ph.h>

#define GetProc(DllName, ProcName) GetProcAddress(GetModuleHandle(L##DllName), (ProcName))
#define InitProc(DllName, ProcName) ((ProcName) = (_##ProcName)GetProc(DllName, #ProcName))
#define InitProcReq(DllName, ProcName) \
    if (!InitProc(DllName, ProcName)) \
    { \
        PhShowError( \
            NULL, \
            L"Process Hacker cannot run on your operating system. Unable to find %S in %S.", \
            #ProcName, \
            DllName \
            ); \
        return FALSE; \
    }

BOOLEAN PhInitializeImports()
{
    InitProc("ntdll.dll", NtCreateDebugObject);
    InitProc("ntdll.dll", NtCreateKeyedEvent);
    InitProc("ntdll.dll", NtDebugActiveProcess);
    InitProc("ntdll.dll", NtGetNextProcess);
    InitProc("ntdll.dll", NtGetNextThread);
    InitProc("ntdll.dll", NtIsProcessInJob);
    InitProc("ntdll.dll", NtOpenKeyedEvent);
    InitProc("ntdll.dll", NtQueryInformationEnlistment);
    InitProc("ntdll.dll", NtQueryInformationResourceManager);
    InitProc("ntdll.dll", NtQueryInformationTransaction);
    InitProc("ntdll.dll", NtQueryInformationTransactionManager);
    InitProc("ntdll.dll", NtReleaseKeyedEvent);
    InitProc("ntdll.dll", NtRemoveProcessDebug);
    InitProc("ntdll.dll", NtResumeProcess);
    InitProc("ntdll.dll", NtSetInformationDebugObject);
    InitProc("ntdll.dll", NtSuspendProcess);
    InitProc("ntdll.dll", NtWaitForKeyedEvent);

    return TRUE;
}
