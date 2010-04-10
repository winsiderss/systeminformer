/*
 * Process Hacker Driver - 
 *   Windows version-specific data
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

#ifndef _VERSION_H
#define _VERSION_H

#define WINDOWS_XP 51
#define WINDOWS_SERVER_2003 52
#define WINDOWS_VISTA 60
#define WINDOWS_7 61

#define KVOFF(object, offset) ((PCHAR)(object) + offset)
#define SCAN_LENGTH 0x100000
#define INIT_SCAN(scan, bytes, length, address, scanLength, displacement) \
    ( \
    ((scan).Initialized = TRUE), \
    ((scan).Bytes = (bytes)), \
    ((scan).Length = (length)), \
    ((scan).StartAddress = (address)), \
    ((scan).ScanLength = (scanLength)), \
    ((scan).Displacement = (displacement)), \
    bytes \
    )

typedef struct _KV_SCANPROC
{
    BOOLEAN Initialized;
    PUCHAR Bytes;
    ULONG Length;
    ULONG_PTR StartAddress;
    ULONG ScanLength;
    LONG Displacement;
} KV_SCANPROC, *PKV_SCANPROC;

NTSTATUS KvInit();

PVOID KvScanProc(
    PKV_SCANPROC ScanProc
    );

#ifdef EXT
#undef EXT
#endif

#ifdef _VERSION_PRIVATE
#define EXT
#define SCANNULL = { FALSE, NULL, 0, 0, 0, 0 }
#else
#define EXT extern
#define SCANNULL
#endif

EXT ULONG WindowsVersion;
EXT RTL_OSVERSIONINFOEXW RtlWindowsVersion;
EXT ACCESS_MASK ProcessAllAccess;
EXT ACCESS_MASK ThreadAllAccess;

/* Offsets */
/* Structures
 * Et: ETHREAD
 * Ep: EPROCESS
 * Oh: OBJECT_HEADER
 * Ot: OBJECT_TYPE
 * Oti: OBJECT_TYPE_INITIALIZER, offset measured from an OBJECT_TYPE
 */
EXT ULONG OffEtClientId;
EXT ULONG OffEpJob;
EXT ULONG OffEpObjectTable;
EXT ULONG OffEpProtectedProcessOff;
EXT ULONG OffEpProtectedProcessBit;
EXT ULONG OffEpRundownProtect;
EXT ULONG OffOhBody;
EXT ULONG OffOtName;
EXT ULONG OffOtIndex;
EXT ULONG OffOtiGenericMapping;

/* Functions
 */
EXT KV_SCANPROC PsTerminateProcessScan SCANNULL;
EXT KV_SCANPROC PspTerminateThreadByPointerScan SCANNULL;

#endif
