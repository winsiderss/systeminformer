/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     dmex    2015-2016
 *
 */

// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the current folder for more information.
//-----------------------------------------------------------------------------
// PerfCounterDefs.h
//
// Internal Interface for CLR to use Performance counters.
//-----------------------------------------------------------------------------
//
// dmex: This header has been highly modified.
// Original: https://github.com/dotnet/coreclr/blob/master/src/inc/perfcounterdefs.h

#ifndef _PERF_COUNTERS_H_
#define _PERF_COUNTERS_H_

//-----------------------------------------------------------------------------
// Name of global IPC block
#define SHARED_PERF_IPC_NAME "SharedPerfIPCBlock"

//-----------------------------------------------------------------------------
// Attributes for the IPC block
#define PERF_ATTR_ON      0x0001   // Are we even updating any counters?
#define PERF_ATTR_GLOBAL  0x0002   // Is this a global or private block?

//.............................................................................
// Tri Counter. Support for the common trio of counters (Total, Current, and Instantaneous).
typedef struct _TRICOUNT
{
    ULONG Current;                          // Current, has +, -
    ULONG Total;                            // Total, has  only +
} TRICOUNT;

//.............................................................................
// Interlocked Tri Counter. Support for the common trio of counters (Total, Current, and Instantaneous).
typedef struct _TRICOUNT_IL
{
    ULONG Current;                          // Current, has +, -
    ULONG Total;                            // Total, has only +
} TRICOUNT_IL;


//.............................................................................
// Dual Counter. Support for the (Total and Instantaneous (rate)). Helpful in cases
// where the current value is always same as the total value. ie. the counter is never
// decremented.
//.............................................................................
typedef struct _DUALCOUNT
{
    ULONG Total;
} DUALCOUNT;

//-----------------------------------------------------------------------------
// Format for the Perf Counter IPC Block
// IPC block is broken up into sections. This marks it easier to marshall
// into different perfmon objects
//
//.............................................................................
// Naming convention (by prefix):
// c - Raw count of something.
// cb- count of bytes
// time - time value.
// depth - stack depth
//-----------------------------------------------------------------------------

#define MAX_TRACKED_GENS   3  // number of generations we track


#ifndef _WIN64
#include <pshpack4.h>
#endif
typedef struct _Perf_GC
{
    size_t cGenCollections[MAX_TRACKED_GENS];   // count of collects per gen
    size_t cbPromotedMem[MAX_TRACKED_GENS - 1]; // count of promoted memory
    size_t cbPromotedFinalizationMem;           // count of memory promoted due to finalization
    size_t cProcessID;                          // process ID
    size_t cGenHeapSize[MAX_TRACKED_GENS];      // size of heaps per gen
    size_t cTotalCommittedBytes;                // total number of committed bytes.
    size_t cTotalReservedBytes;                 // bytes reserved via VirtualAlloc
    size_t cLrgObjSize;                         // size of Large Object Heap
    size_t cSurviveFinalize;                    // count of instances surviving from finalizing
    size_t cHandles;                            // count of GC handles
    size_t cbAlloc;                             // bytes allocated
    size_t cbLargeAlloc;                        // bytes allocated for Large Objects
    size_t cInducedGCs;                         // number of explicit GCs
    ULONG  timeInGC;                            // Time in GC
    ULONG  timeInGCBase;                        // must follow time in GC counter
    size_t cPinnedObj;                          // # of Pinned Objects
    size_t cSinkBlocks;                         // # of sink blocks
} Perf_GC;
#ifndef _WIN64
#include <poppack.h>
#endif

// Perf_GC_Wow64 mimics in a 64 bit process, the layout of Perf_GC in a 32 bit process
// It does this by replacing all size_t by ULONG
#include <pshpack4.h>
typedef struct _Perf_GC_Wow64
{
    ULONG cGenCollections[MAX_TRACKED_GENS];    // count of collects per gen
    ULONG cbPromotedMem[MAX_TRACKED_GENS - 1];  // count of promoted memory
    ULONG cbPromotedFinalizationMem;            // count of memory promoted due to finalization
    ULONG cProcessID;                           // process ID
    ULONG cGenHeapSize[MAX_TRACKED_GENS];       // size of heaps per gen
    ULONG cTotalCommittedBytes;                 // total number of committed bytes.
    ULONG cTotalReservedBytes;                  // bytes reserved via VirtualAlloc
    ULONG cLrgObjSize;                          // size of Large Object Heap
    ULONG cSurviveFinalize;                     // count of instances surviving from finalizing
    ULONG cHandles;                             // count of GC handles
    ULONG cbAlloc;                              // bytes allocated
    ULONG cbLargeAlloc;                         // bytes allocated for Large Objects
    ULONG cInducedGCs;                          // number of explicit GCs
    ULONG timeInGC;                             // Time in GC
    ULONG timeInGCBase;                         // must follow time in GC counter
    ULONG cPinnedObj;                           // # of Pinned Objects
    ULONG cSinkBlocks;                          // # of sink blocks
} Perf_GC_Wow64;
#include <poppack.h>



#ifndef _WIN64
#include <pshpack4.h>
#endif
typedef struct Perf_Loading
{
    TRICOUNT cClassesLoaded;
    TRICOUNT_IL cAppDomains;                // Current # of AppDomains
    TRICOUNT cAssemblies;                   // Current # of Assemblies
    UNALIGNED LONGLONG timeLoading;         // % time loading
    ULONG cAsmSearchLen;                    // Avg search length for assemblies
    DUALCOUNT cLoadFailures;                // Classes Failed to load
    size_t cbLoaderHeapSize;                // Total size of heap used by the loader
    DUALCOUNT cAppDomainsUnloaded;          // Rate at which app domains are unloaded
} Perf_Loading;
#ifndef _WIN64
#include <poppack.h>
#endif

#include <pshpack4.h>
typedef struct Perf_Loading_Wow64
{
    TRICOUNT cClassesLoaded;
    TRICOUNT_IL cAppDomains;                // Current # of AppDomains
    TRICOUNT cAssemblies;                   // Current # of Assemblies
    UNALIGNED LONGLONG timeLoading;         // % time loading
    ULONG cAsmSearchLen;                    // Avg search length for assemblies
    DUALCOUNT cLoadFailures;                // Classes Failed to load
    ULONG cbLoaderHeapSize;                 // Total size of heap used by the loader
    DUALCOUNT cAppDomainsUnloaded;          // Rate at which app domains are unloaded
} Perf_Loading_Wow64;
#include <poppack.h>


#ifndef _WIN64
#include <pshpack4.h>
#endif
typedef struct Perf_Jit
{
    ULONG cMethodsJitted;                   // number of methods jitted
    TRICOUNT cbILJitted;                    // IL jitted stats
    //DUALCOUNT cbPitched;                  // Total bytes pitched
    ULONG cJitFailures;                     // # of standard Jit failures
    ULONG timeInJit;                        // Time in JIT since last sample
    ULONG timeInJitBase;                    // Time in JIT base counter
} Perf_Jit;
#ifndef _WIN64
#include <poppack.h>
#endif

#ifndef _WIN64
#include <pshpack4.h>
#endif
typedef struct Perf_Excep
{
    DUALCOUNT cThrown;                      // Number of Exceptions thrown
    ULONG cFiltersExecuted;                 // Number of Filters executed
    ULONG cFinallysExecuted;                // Number of Finallys executed
    ULONG cThrowToCatchStackDepth;          // Delta from throw to catch site on stack
} Perf_Excep;
#ifndef _WIN64
#include <poppack.h>
#endif

#ifndef _WIN64
#include <pshpack4.h>
#endif
typedef struct Perf_Interop
{
    ULONG cCCW;                             // Number of CCWs
    ULONG cStubs;                           // Number of stubs
    ULONG cMarshalling;                     // # of time marshalling args and return values.
    ULONG cTLBImports;                      // Number of tlbs we import
    ULONG cTLBExports;                      // Number of tlbs we export
} Perf_Interop;
#ifndef _WIN64
#include <poppack.h>
#endif

#ifndef _WIN64
#include <pshpack4.h>
#endif
typedef struct Perf_LocksAndThreads
{
    // Locks
    DUALCOUNT cContention;                  // # of times in AwareLock::EnterEpilogue()
    TRICOUNT cQueueLength;                  // Lenght of queue
    // Threads
    ULONG cCurrentThreadsLogical;           // Number (created - destroyed) of logical threads
    ULONG cCurrentThreadsPhysical;          // Number (created - destroyed) of OS threads
    TRICOUNT cRecognizedThreads;            // # of Threads execute in runtime's control
} Perf_LocksAndThreads;
#ifndef _WIN64
#include <poppack.h>
#endif


// IMPORTANT!!!!!!!: The first two fields in the struct have to be together
// and be the first two fields in the struct. The managed code in ChannelServices.cs
// depends on this.
#ifndef _WIN64
#include <pshpack4.h>
#endif
typedef struct Perf_Contexts
{
    // Contexts & Remoting
    DUALCOUNT cRemoteCalls;                 // # of remote calls
    ULONG cChannels;                        // Number of current channels
    ULONG cProxies;                         // Number of context proxies.
    ULONG cClasses;                         // # of Context-bound classes
    ULONG cObjAlloc;                        // # of context bound objects allocated
    ULONG cContexts;                        // The current number of contexts.
} Perf_Contexts;
#ifndef _WIN64
#include <poppack.h>
#endif


#ifndef _WIN64
#include <pshpack4.h>
#endif
typedef struct Perf_Security
{
    ULONG cTotalRTChecks;                   // Total runtime checks
    UNALIGNED LONGLONG timeAuthorize;       // % time authenticating
    ULONG cLinkChecks;                      // link time checks
    ULONG timeRTchecks;                     // % time in Runtime checks
    ULONG timeRTchecksBase;                 // % time in Runtime checks base counter
    ULONG stackWalkDepth;                   // depth of stack for security checks
} Perf_Security;
#ifndef _WIN64
#include <poppack.h>
#endif

#include <pshpack4.h>
typedef struct Perf_Security_Wow64
{
    ULONG cTotalRTChecks;                   // Total runtime checks
    UNALIGNED LONGLONG timeAuthorize;       // % time authenticating
    ULONG cLinkChecks;                      // link time checks
    ULONG timeRTchecks;                     // % time in Runtime checks
    ULONG timeRTchecksBase;                 // % time in Runtime checks base counter
    ULONG stackWalkDepth;                   // depth of stack for security checks
} Perf_Security_Wow64;
#include <poppack.h>


#ifndef _WIN64
#include <pshpack4.h>
#endif
typedef struct _PerfCounterIPCControlBlock
{
    // Versioning info
    USHORT Bytes;           // size of this entire block
    USHORT Attributes;      // attributes for this block

    // Counter Sections
    Perf_GC                   GC;
    Perf_Contexts             Context;
    Perf_Interop              Interop;
    Perf_Loading              Loading;
    Perf_Excep                Exceptions;
    Perf_LocksAndThreads      LocksAndThreads;
    Perf_Jit                  Jit;
    Perf_Security             Security;
} PerfCounterIPCControlBlock;
#ifndef _WIN64
#include <poppack.h>
#endif

#include <pshpack4.h>
typedef struct _PerfCounterIPCControlBlock_Wow64
{
    // Versioning info
    USHORT Bytes;           // size of this entire block
    USHORT Attributes;      // attributes for this block

    // Counter Sections
    Perf_GC_Wow64           GC;
    Perf_Contexts           Context;
    Perf_Interop            Interop;
    Perf_Loading_Wow64      Loading;
    Perf_Excep              Exceptions;
    Perf_LocksAndThreads    LocksAndThreads;
    Perf_Jit                Jit;
    Perf_Security_Wow64     Security;
} PerfCounterIPCControlBlock_Wow64;
#include <poppack.h>


#endif _PERF_COUNTERS_H_
