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
#define DOTNETTOOLS_INTERFACE_VERSION 2

typedef enum _DOTNETTOOLS_ASSEMBLY_STATUS
{
    DotNetToolsAssembliesOk,
    DotNetToolsAssembliesNotDotNet,
    DotNetToolsAssembliesWow64,
    DotNetToolsAssembliesFailed
} DOTNETTOOLS_ASSEMBLY_STATUS;

typedef struct _DOTNETTOOLS_ASSEMBLY
{
    ULONG AppDomainType;
    ULONG32 AppDomainNumber;
    ULONG64 AppDomainId;
    PPH_STRING AppDomainName;

    BOOLEAN IsDynamic;
    BOOLEAN IsReflection;
    BOOLEAN IsDynamicModule;
    BOOLEAN IsMemoryStream;
    BOOLEAN IsMainModule;

    PVOID BaseAddress;
    ULONG64 AssemblyId;
    ULONG64 ModuleId;

    PPH_STRING AssemblyName;
    PPH_STRING DisplayName;
    PPH_STRING ModuleName;
    PPH_STRING NativeFileName;
    GUID Mvid;
} DOTNETTOOLS_ASSEMBLY, *PDOTNETTOOLS_ASSEMBLY;

typedef _Function_class_(DOTNETTOOLS_ASSEMBLY_CALLBACK)
BOOLEAN NTAPI DOTNETTOOLS_ASSEMBLY_CALLBACK(
    _In_ PDOTNETTOOLS_ASSEMBLY Assembly,
    _In_opt_ PVOID Context
    );
typedef DOTNETTOOLS_ASSEMBLY_CALLBACK* PDOTNETTOOLS_ASSEMBLY_CALLBACK;

// UnreadableAppDomains receives the number of application domains whose assembly list could not be
// read. Above zero the enumeration is partial, which a status of DotNetToolsAssembliesOk does not say.
typedef DOTNETTOOLS_ASSEMBLY_STATUS (NTAPI* PDOTNETTOOLS_ENUM_ASSEMBLIES)(
    _In_ HANDLE ProcessId,
    _In_ PDOTNETTOOLS_ASSEMBLY_CALLBACK Callback,
    _In_opt_ PVOID Context,
    _Out_opt_ PULONG UnreadableAppDomains
    );

typedef struct _DOTNETTOOLS_INTERFACE
{
    ULONG Version;
    PDOTNETTOOLS_ENUM_ASSEMBLIES EnumProcessAssemblies;
} DOTNETTOOLS_INTERFACE, *PDOTNETTOOLS_INTERFACE;

#endif
