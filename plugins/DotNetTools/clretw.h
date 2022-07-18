/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2011-2015
 *     dmex    2016-2021
 *
 */

#ifndef CLRETW_H
#define CLRETW_H

// Keywords

#define CLR_GC_KEYWORD 0x1
#define CLR_GCHANDLE_KEYWORD 0x2
#define CLR_ASSEMBLY_LOADER_KEYWORD 0x4
#define CLR_LOADER_KEYWORD 0x8
#define CLR_JIT_KEYWORD 0x10
#define CLR_NGEN_KEYWORD 0x20
#define CLR_STARTENUMERATION_KEYWORD 0x40
#define CLR_STOPENUMERATION_KEYWORD 0x80
#define CLR_SECURITY_KEYWORD 0x400
#define CLR_APPDOMAINRESOURCEMANAGEMENT_KEYWORD 0x800
#define CLR_JITTRACING_KEYWORD 0x1000
#define CLR_INTEROP_KEYWORD 0x2000
#define CLR_CONTENTION_KEYWORD 0x4000
#define CLR_EXCEPTION_KEYWORD 0x8000
#define CLR_THREADING_KEYWORD 0x10000
#define CLR_JITTEDMETHODILTONATIVEMAP_KEYWORD 0x20000
#define CLR_OVERRIDEANDSUPPRESSNGENEVENTS_KEYWORD 0x40000
#define CLR_TYPE_KEYWORD 0x80000
#define CLR_GCHEAPDUMP_KEYWORD 0x100000
#define CLR_GCHEAPALLOCHIGH_KEYWORD 0x200000
#define CLR_GCHEAPSURVIVALANDMOVEMENT_KEYWORD 0x400000
#define CLR_GCHEAPCOLLECT_KEYWORD 0x800000
#define CLR_GCHEAPANDTYPENAMES_KEYWORD 0x1000000
#define CLR_GCHEAPALLOCLOW_KEYWORD 0x2000000
#define CLR_GCALLOBJECTALLOCATION_KEYWORD (CLR_GCHEAPALLOCHIGH_KEYWORD | CLR_GCHEAPSURVIVALANDMOVEMENT_KEYWORD)
#define CLR_PERFTRACK_KEYWORD 0x20000000
#define CLR_STACK_KEYWORD 0x40000000
#define CLR_THREADTRANSFER_KEYWORD 0x80000000
#define CLR_DEBUGGER_KEYWORD 0x100000000
#define CLR_MONITORING_KEYWORD 0x200000000
#define CLR_CODESYMBOLS_KEYWORD 0x400000000
#define CLR_EVENTSOURCE_KEYWORD 0x800000000
#define CLR_COMPILATION_KEYWORD 0x1000000000
#define CLR_COMPILATIONDIAGNOSTIC_KEYWORD 0x2000000000
#define CLR_METHODDIAGNOSTIC_KEYWORD 0x4000000000
#define CLR_TYPEDIAGNOSTIC_KEYWORD 0x8000000000
#define CLR_JITINSTRUMENTEDDATA_KEYWORD 0x10000000000
#define CLR_PROFILER_KEYWORD 0x20000000000

#define CLR_DEFAULT_KEYWORD \
     (CLR_GC_KEYWORD | CLR_TYPE_KEYWORD | CLR_GCHEAPSURVIVALANDMOVEMENT_KEYWORD | CLR_ASSEMBLY_LOADER_KEYWORD | CLR_LOADER_KEYWORD | \
     CLR_JIT_KEYWORD | CLR_NGEN_KEYWORD | CLR_OVERRIDEANDSUPPRESSNGENEVENTS_KEYWORD | CLR_STOPENUMERATION_KEYWORD | CLR_SECURITY_KEYWORD | \
     CLR_APPDOMAINRESOURCEMANAGEMENT_KEYWORD | CLR_EXCEPTION_KEYWORD | CLR_THREADING_KEYWORD | CLR_CONTENTION_KEYWORD | \
     CLR_STACK_KEYWORD | CLR_JITTEDMETHODILTONATIVEMAP_KEYWORD | CLR_THREADTRANSFER_KEYWORD | CLR_GCHEAPANDTYPENAMES_KEYWORD | \
     CLR_CODESYMBOLS_KEYWORD | CLR_COMPILATION_KEYWORD)

#define CLR_JITSYMBOLS_KEYWORD \
    (CLR_JIT_KEYWORD | CLR_STOPENUMERATION_KEYWORD | CLR_JITTEDMETHODILTONATIVEMAP_KEYWORD | CLR_OVERRIDEANDSUPPRESSNGENEVENTS_KEYWORD | CLR_LOADER_KEYWORD)

#define CLR_GCHEAPSNAPSHOT_KEYWORD \
    (CLR_GC_KEYWORD | CLR_GCHEAPCOLLECT_KEYWORD | CLR_GCHEAPDUMP_KEYWORD | CLR_GCHEAPANDTYPENAMES_KEYWORD | CLR_TYPE_KEYWORD)

// Event IDs

#define CLRStackWalkDCStart 0
#define GCSettingsRundown 10
#define MethodDCStart_V2 141
#define MethodDCEnd_V2 142
#define MethodDCStartVerbose_V2 143
#define MethodDCEndVerbose_V2 144
#define DCStartComplete_V1 145
#define DCEndComplete_V1 146
#define DCStartInit_V1 147
#define DCEndInit_V1 148
#define MethodDCStartILToNativeMap 149
#define MethodDCEndILToNativeMap 150
#define DomainModuleDCStart_V1 151
#define DomainModuleDCEnd_V1 152
#define ModuleDCStart_V1 153
#define ModuleDCStart_V2 153
#define ModuleDCEnd_V1 154
#define ModuleDCEnd_V2 154
#define AssemblyDCStart_V1 155
#define AssemblyDCEnd_V1 156
#define AppDomainDCStart_V1 157
#define AppDomainDCEnd_V1 158
#define ThreadDC 159
#define ModuleRangeDCStart 160
#define ModuleRangeDCEnd 161
#define RuntimeInformationDCStart 187

// Opcodes

#define CLR_METHODDC_DCSTARTCOMPLETE_OPCODE 14
#define CLR_METHODDC_DCENDCOMPLETE_OPCODE 15
#define CLR_MODULEDCSTART_OPCODE 35
#define CLR_MODULEDCEND_OPCODE 36

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
#define ModuleFlags_IbcOptimized 0x10
#define ModuleFlags_ReadyToRunModule 0x20
#define ModuleFlags_PartialReadyToRun 0x40

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

// RuntimeSku
#define RuntimeSku_DesktopCLR 0x1
#define RuntimeSku_CoreCLR 0x2

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

typedef struct _ModuleLoadUnloadRundown_V2
{
    ULONG64 ModuleID;
    ULONG64 AssemblyID;
    ULONG ModuleFlags; // ModuleFlags
    ULONG Reserved1;
    WCHAR ModuleILPath[1];
    // WCHAR ModuleNativePath[1];
    // USHORT ClrInstanceID;
    // GUID ManagedPdbSignature;
    // ULONG ManagedPdbAge;
    // WCHAR ManagedPdbBuildPath[1];
    // GUID NativePdbSignature;
    // ULONG NativePdbAge;
    // WCHAR NativePdbBuildPath[1];
} ModuleLoadUnloadRundown_V2, *PModuleLoadUnloadRundown_V2;

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
    USHORT Sku; // RuntimeSku
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
