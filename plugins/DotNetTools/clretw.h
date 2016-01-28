/*
 * Process Hacker .NET Tools
 *
 * Copyright (C) 2011-2015 wj32
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

#ifndef CLRETW_H
#define CLRETW_H

// Keywords

#define CLR_LOADER_KEYWORD 0x8
#define CLR_STARTENUMERATION_KEYWORD 0x40

// Event IDs

#define DCStartComplete_V1 145
#define ModuleDCStart_V1 153
#define AssemblyDCStart_V1 155
#define AppDomainDCStart_V1 157
#define RuntimeInformationDCStart 187

// Opcodes

#define CLR_METHODDC_DCSTARTCOMPLETE_OPCODE 14
#define CLR_MODULEDCSTART_OPCODE 35

// Bit maps

// AppDomainFlags
#define AppDomainFlags_Default 0x1
#define AppDomainFlags_Executable 0x2
#define AppDomainFlags_Shared 0x4

// AssemblyFlags
#define AssemblyFlags_DomainNeutral 0x1
#define AssemblyFlags_Dynamic 0x2
#define AssemblyFlags_Native 0x4
#define AssemblyFlags_Collectible 0x8

// ModuleFlags
#define ModuleFlags_DomainNeutral 0x1
#define ModuleFlags_Native 0x2
#define ModuleFlags_Dynamic 0x4
#define ModuleFlags_Manifest 0x8

// StartupMode
#define StartupMode_ManagedExe 0x1
#define StartupMode_HostedCLR 0x2
#define StartupMode_IjwDll 0x4
#define StartupMode_ComActivated 0x8
#define StartupMode_Other 0x10

// StartupFlags
#define StartupFlags_CONCURRENT_GC 0x1
#define StartupFlags_LOADER_OPTIMIZATION_SINGLE_DOMAIN 0x2
#define StartupFlags_LOADER_OPTIMIZATION_MULTI_DOMAIN 0x4
#define StartupFlags_LOADER_SAFEMODE 0x10
#define StartupFlags_LOADER_SETPREFERENCE 0x100
#define StartupFlags_SERVER_GC 0x1000
#define StartupFlags_HOARD_GC_VM 0x2000
#define StartupFlags_SINGLE_VERSION_HOSTING_INTERFACE 0x4000
#define StartupFlags_LEGACY_IMPERSONATION 0x10000
#define StartupFlags_DISABLE_COMMITTHREADSTACK 0x20000
#define StartupFlags_ALWAYSFLOW_IMPERSONATION 0x40000
#define StartupFlags_TRIM_GC_COMMIT 0x80000
#define StartupFlags_ETW 0x100000
#define StartupFlags_SERVER_BUILD 0x200000
#define StartupFlags_ARM 0x400000

// Templates

#include <pshpack1.h>

typedef struct _DCStartEnd
{
    USHORT ClrInstanceID;
} DCStartEnd, *PDCStartEnd;

typedef struct _ModuleLoadUnloadRundown_V1
{
    ULONG64 ModuleID;
    ULONG64 AssemblyID;
    ULONG ModuleFlags; // ModuleFlags
    ULONG Reserved1;
    WCHAR ModuleILPath[1];
    // WCHAR ModuleNativePath[1];
    // USHORT ClrInstanceID;
} ModuleLoadUnloadRundown_V1, *PModuleLoadUnloadRundown_V1;

typedef struct _AssemblyLoadUnloadRundown_V1
{
    ULONG64 AssemblyID;
    ULONG64 AppDomainID;
    ULONG64 BindingID;
    ULONG AssemblyFlags; // AssemblyFlags
    WCHAR FullyQualifiedAssemblyName[1];
    // USHORT ClrInstanceID;
} AssemblyLoadUnloadRundown_V1, *PAssemblyLoadUnloadRundown_V1;

typedef struct _AppDomainLoadUnloadRundown_V1
{
    ULONG64 AppDomainID;
    ULONG AppDomainFlags; // AppDomainFlags
    WCHAR AppDomainName[1];
    // ULONG AppDomainIndex;
    // USHORT ClrInstanceID;
} AppDomainLoadUnloadRundown_V1, *PAppDomainLoadUnloadRundown_V1;

typedef struct _RuntimeInformationRundown
{
    USHORT ClrInstanceID;
    USHORT Sku;
    USHORT BclMajorVersion;
    USHORT BclMinorVersion;
    USHORT BclBuildNumber;
    USHORT BclQfeNumber;
    USHORT VMMajorVersion;
    USHORT VMMinorVersion;
    USHORT VMBuildNumber;
    USHORT VMQfeNumber;
    ULONG StartupFlags; // StartupFlags
    UCHAR StartupMode; // StartupMode
    WCHAR CommandLine[1];
    // GUID ComObjectGuid;
    // WCHAR RuntimeDllPath[1];
} RuntimeInformationRundown, *PRuntimeInformationRundown;

#include <poppack.h>

#endif
