/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     jxy-s   2026
 *
 */

#ifndef _DOTNETTOOLSINTF_H
#define _DOTNETTOOLSINTF_H

#define DOTNETTOOLS_PLUGIN_NAME L"DotNetTools"
#define DOTNETTOOLS_INTERFACE_VERSION 1

typedef enum _DOTNETTOOLS_ASSEMBLY_STATUS
{
    DotNetToolsAssembliesOk,
    DotNetToolsAssembliesNotDotNet,     // The process is not running a CLR.
    DotNetToolsAssembliesWow64,         // A 32-bit target, which this interface will not reach.
    DotNetToolsAssembliesFailed         // The runtime is there but would not answer.
} DOTNETTOOLS_ASSEMBLY_STATUS;

/**
 * One assembly loaded into a process, with the application domain it belongs to.
 *
 * \remarks The strings belong to the enumeration and are only valid for the duration of the
 * callback; copy anything worth keeping.
 */
typedef struct _DOTNETTOOLS_ASSEMBLY
{
    ULONG AppDomainType;            // DN_CLR_APPDOMAIN_TYPE: dynamic, shared or system.
    ULONG32 AppDomainNumber;
    ULONG64 AppDomainId;
    PPH_STRING AppDomainName;

    BOOLEAN IsDynamic;              // Emitted at run time rather than loaded from a file.
    BOOLEAN IsReflection;
    BOOLEAN IsDynamicModule;        // The module itself, as the runtime flags it.
    BOOLEAN IsMemoryStream;         // Loaded from memory rather than from a file.
    BOOLEAN IsMainModule;

    PVOID BaseAddress;
    ULONG64 AssemblyId;
    ULONG64 ModuleId;

    PPH_STRING AssemblyName;
    PPH_STRING DisplayName;
    PPH_STRING ModuleName;
    PPH_STRING NativeFileName;      // The precompiled native image, when the module has one.
    GUID Mvid;                      // Module version id, which identifies the exact build.
} DOTNETTOOLS_ASSEMBLY, *PDOTNETTOOLS_ASSEMBLY;

typedef _Function_class_(DOTNETTOOLS_ASSEMBLY_CALLBACK)
BOOLEAN NTAPI DOTNETTOOLS_ASSEMBLY_CALLBACK(
    _In_ PDOTNETTOOLS_ASSEMBLY Assembly,
    _In_opt_ PVOID Context
    );
typedef DOTNETTOOLS_ASSEMBLY_CALLBACK* PDOTNETTOOLS_ASSEMBLY_CALLBACK;

/**
 * Enumerates the assemblies loaded into a process, application domain by application domain.
 *
 * \param ProcessId The process to inspect.
 * \param Callback Called for each assembly; return FALSE to stop.
 * \param Context Passed to the callback.
 * \return What happened, so a caller can tell "no assemblies" from "could not ask".
 *
 * \remarks Reads the target's runtime through the debugging data access layer, which loads that
 * runtime's DAC into this process and takes a moment on the first call for a given process.
 *
 * A 32-bit target is refused rather than served: the application reaches one through phsvc, and
 * starting phsvc prompts for elevation, which is not something an enumeration should do on its
 * caller's behalf.
 */
typedef DOTNETTOOLS_ASSEMBLY_STATUS (NTAPI* PDOTNETTOOLS_ENUM_ASSEMBLIES)(
    _In_ HANDLE ProcessId,
    _In_ PDOTNETTOOLS_ASSEMBLY_CALLBACK Callback,
    _In_opt_ PVOID Context
    );

typedef struct _DOTNETTOOLS_INTERFACE
{
    ULONG Version;
    PDOTNETTOOLS_ENUM_ASSEMBLIES EnumProcessAssemblies;
} DOTNETTOOLS_INTERFACE, *PDOTNETTOOLS_INTERFACE;

#endif
