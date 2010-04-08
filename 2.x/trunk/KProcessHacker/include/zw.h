/*
 * Process Hacker Driver - 
 *   system calls
 * 
 * Copyright (C) 2009 wj32
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

#ifndef _ZW_H
#define _ZW_H

NTSTATUS NTAPI ZwOpenProcessToken(
    __in HANDLE ProcessHandle,
    __in ACCESS_MASK DesiredAccess,
    __out PHANDLE TokenHandle
    );

NTSTATUS NTAPI ZwQueryInformationProcess(
    __in HANDLE ProcessHandle,
    __in PROCESSINFOCLASS ProcessInformationClass,
    __out PVOID ProcessInformation,
    __in ULONG ProcessInformationLength,
    __out_opt PULONG ReturnLength
    );

NTSTATUS NTAPI ZwQueryInformationThread(
    __in HANDLE ThreadHandle,
    __in PROCESSINFOCLASS ThreadInformationClass,
    __out PVOID ThreadInformation,
    __in ULONG ThreadInformationLength,
    __out_opt PULONG ReturnLength
    );

NTSTATUS NTAPI ZwSetInformationProcess(
    __in HANDLE ProcessHandle,
    __in PROCESSINFOCLASS ProcessInformationClass,
    __in PVOID ProcessInformation,
    __in ULONG ProcessInformationLength
    );

/* NTSTATUS NTAPI ZwSetInformationThread(
    __in HANDLE ThreadHandle,
    __in THREADINFOCLASS ThreadInformationClass,
    __in PVOID ThreadInformation,
    __in ULONG ThreadInformationLength
    ); */

typedef NTSTATUS (NTAPI *_NtClose)(
    __in HANDLE Handle
    );

#endif
