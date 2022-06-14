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
// IPCHeader.h
//
// Define the LegacyPrivate header format for IPC memory mapped files.
//-----------------------------------------------------------------------------
//
// dmex: This header has been highly modified.
// Original: https://github.com/dotnet/coreclr/blob/master/src/ipcman/ipcheader.h

#ifndef _IPC_HEADER_H_
#define _IPC_HEADER_H_

#include "perfcounterdefs.h"
#include "ipcenums.h"

// Current version of the IPC Block
#define VER_IPC_BLOCK 0x4
// Legacy version of the IPC Block
#define VER_LEGACYPRIVATE_IPC_BLOCK 0x2
#define VER_LEGACYPUBLIC_IPC_BLOCK 0x3

//-----------------------------------------------------------------------------
// Entry in the IPC Directory. Ensure binary compatibility across versions
// if we add (or remove) entries. If we remove an block, the entry should
// be EMPTY_ENTRY_OFFSET
//-----------------------------------------------------------------------------

const ULONG EMPTY_ENTRY_OFFSET  = 0xFFFFFFFF;
const ULONG EMPTY_ENTRY_SIZE    = 0;

typedef struct IPCEntry
{
    ULONG Offset;     // offset of the IPC Block from the end of the Full IPC Header
    ULONG Size;       // size (in bytes) of the block
} IPCEntry;

// Newer versions of the CLR use Flags field
#define IPC_FLAG_USES_FLAGS 0x1
#define IPC_FLAG_INITIALIZED 0x2
#define IPC_FLAG_X86 0x4

// In hindsight, we should have made the offsets be absolute, but we made them
// relative to the end of the FullIPCHeader.
// The problem is that as future versions added new Entries to the directory,
// the header size grew.
// Thus we make IPCEntry::Offset is relative to IPC_ENTRY_OFFSET_BASE, which
// corresponds to sizeof(PrivateIPCHeader) for an v1.0 /v1.1 build.
#define IPC_ENTRY_OFFSET_BASE_X86 0x14
#define IPC_ENTRY_OFFSET_BASE_X64 0x0


/******************************************************************************
 * The CLR opens memory mapped files to expose perfcounter values and other
 * information to other processes. Historically there have been three memory
 * mapped files: the BlockTable, the LegacyPrivateBlock, and the
 * LegacyPublicBlock.
 *
 * BlockTable - The block table was designed to work with multiple runtimes
 *     running side by side in the same process (SxS in-proc). We have defined
 *     semantics using interlocked operations that allow a runtime to allocate
 *     a block from the block table in a thread-safe manner.
 *
 * LegacyPrivateBlock - The legacy private block was used by older versions
 *     of the runtime to expose various information to the debugger. The
 *     legacy private block is not compatible with in-proc SxS, and thus it
 *     must be removed in the near future. Currently it is being used to expose
 *     information about AppDomains to the debugger. We will need to keep the
 *     code that knows how to read the legacy private block as long as we
 *     continue to support .NET 3.5 SP1.
 *
 * LegacyPublicBlock - The legacy public block was used by older versions
 *     of the runtime to expose perfcounter values. The legacy public block is
 *     not compatible with in-proc SxS, and thus it has been removed. We will
 *     need to keep the code that knows how to read the legacy public block as
 *     long as we continue to support .NET 3.5 SP1.
 ******************************************************************************/

/**************************************** BLOCK TABLE ****************************************/

#define IPC_BLOCK_TABLE_SIZE     65536
#define IPC_BLOCK_SIZE           2048
#define IPC_NUM_BLOCKS_IN_TABLE  32

C_ASSERT(IPC_BLOCK_TABLE_SIZE == IPC_NUM_BLOCKS_IN_TABLE * IPC_BLOCK_SIZE);

typedef struct _IPCHeader
{
    // Chunk header
    volatile LONG Counter;  // *** Volatile<LONG> ***   value of 0 is special; means that this block has never been touched before by a writer
    ULONG RuntimeId;        // value of 0 is special; means that chunk is currently free (runtime ids are always greater than 0)
    ULONG Reserved1;
    ULONG Reserved2;

    // Standard header
    USHORT Version;      // version of the IPC Block
    USHORT Flags;        // flags field
    ULONG  blockSize;    // Size of the entire shared memory block
    USHORT BuildYear;    // stamp for year built
    USHORT BuildNumber;  // stamp for Month/Day built
    ULONG  NumEntries;   // Number of entries in the table

    // Directory
    IPCEntry EntryTable[eIPC_MAX]; // entry describing each client's block
} IPCHeader;

#define SXSPUBLIC_IPC_SIZE_NO_PADDING         (sizeof(IPCHeader) + sizeof(PerfCounterIPCControlBlock))
#define SXSPUBLIC_WOW64_IPC_SIZE_NO_PADDING   (sizeof(IPCHeader) + sizeof(PerfCounterIPCControlBlock_Wow64))

#define SXSPUBLIC_IPC_PAD_SIZE                (IPC_BLOCK_SIZE - SXSPUBLIC_IPC_SIZE_NO_PADDING)
#define SXSPUBLIC_WOW64_IPC_PAD_SIZE          (IPC_BLOCK_SIZE - SXSPUBLIC_WOW64_IPC_SIZE_NO_PADDING)

#ifndef _WIN64
#include <pshpack4.h>
#endif
typedef struct _IPCControlBlock
{
    // Header
    IPCHeader Header;

    // Client blocks
    PerfCounterIPCControlBlock PerfIpcBlock;

    // Padding
    BYTE Padding[SXSPUBLIC_IPC_PAD_SIZE];
} IPCControlBlock;
#ifndef _WIN64
#include <poppack.h>
#endif

#include <pshpack4.h>
typedef struct _IPCControlBlock_Wow64
{
    // Header
    IPCHeader Header;

    // Client blocks
    PerfCounterIPCControlBlock_Wow64 PerfIpcBlock;

    // Padding
    BYTE Padding[SXSPUBLIC_WOW64_IPC_PAD_SIZE];
} IPCControlBlock_Wow64;
#include <poppack.h>

C_ASSERT(sizeof(IPCControlBlock) == IPC_BLOCK_SIZE);
C_ASSERT(sizeof(IPCControlBlock_Wow64) == IPC_BLOCK_SIZE);

#ifndef _WIN64
#include <pshpack4.h>
#endif
typedef struct IPCControlBlockTable
{
    IPCControlBlock Blocks[IPC_NUM_BLOCKS_IN_TABLE];
} IPCControlBlockTable;
#ifndef _WIN64
#include <poppack.h>
#endif

#include <pshpack4.h>
typedef struct IPCControlBlockTable_Wow64
{
    IPCControlBlock_Wow64 Blocks[IPC_NUM_BLOCKS_IN_TABLE];
} IPCControlBlockTable_Wow64;
#include <poppack.h>

C_ASSERT(sizeof(IPCControlBlockTable) == IPC_BLOCK_TABLE_SIZE);
C_ASSERT(sizeof(IPCControlBlockTable_Wow64) == IPC_BLOCK_TABLE_SIZE);


#ifndef _WIN64
#include <pshpack4.h>
#endif
typedef struct LegacyPrivateIPCHeader
{
    USHORT    Version;      // version of the IPC Block
    USHORT    Flags;        // flags field
    ULONG     BlockSize;    // Size of the entire shared memory block
    HINSTANCE hInstance;    // Instance of module that created this header
    USHORT    BuildYear;    // stamp for year built
    USHORT    BuildNumber;  // stamp for Month/Day built
    ULONG     NumEntries;   // Number of entries in the table
} LegacyPrivateIPCHeader;
#ifndef _WIN64
#include <poppack.h>
#endif

#include <pshpack4.h>
typedef struct LegacyPrivateIPCHeader_Wow64
{
    USHORT Version;       // version of the IPC Block
    USHORT Flags;         // flags field
    ULONG  BlockSize;     // Size of the entire shared memory block
    ULONG  hInstance;     // Instance of module that created this header
    USHORT BuildYear;     // stamp for year built
    USHORT BuildNumber;   // stamp for Month/Day built
    ULONG  NumEntries;    // Number of entries in the table
} LegacyPrivateIPCHeader_Wow64;
#include <poppack.h>

#ifndef _WIN64
#include <pshpack4.h>
#endif
typedef struct FullIPCHeaderLegacyPrivate
{
    // Header
    LegacyPrivateIPCHeader Header;

    // Directory
    IPCEntry EntryTable[eLegacyPrivateIPC_MAX]; // entry describing each client's block
} FullIPCHeaderLegacyPrivate;
#ifndef _WIN64
#include <poppack.h>
#endif

#include <pshpack4.h>
typedef struct FullIPCHeaderLegacyPrivate_Wow64
{
    // Header
    LegacyPrivateIPCHeader_Wow64 Header;

    // Directory
    IPCEntry EntryTable[eLegacyPrivateIPC_MAX]; // entry describing each client's block
} FullIPCHeaderLegacyPrivate_Wow64;
#include <poppack.h>

#ifndef _WIN64
#include <pshpack4.h>
#endif
typedef struct FullIPCHeaderLegacyPublic
{
    // Header
    LegacyPrivateIPCHeader Header;

    // Directory
    IPCEntry EntryTable[eLegacyPublicIPC_MAX]; // entry describing each client's block
} FullIPCHeaderLegacyPublic;
#ifndef _WIN64
#include <poppack.h>
#endif

#include <pshpack4.h>
typedef struct FullIPCHeaderLegacyPublic_Wow64
{
    // Header
    LegacyPrivateIPCHeader_Wow64 Header;

    // Directory
    IPCEntry EntryTable[eLegacyPublicIPC_MAX]; // entry describing each client's block
} FullIPCHeaderLegacyPublic_Wow64;
#include <poppack.h>


//-----------------------------------------------------------------------------
// LegacyPrivate (per process) IPC Block for COM+ apps
//-----------------------------------------------------------------------------
#ifndef _WIN64
#include <pshpack4.h>
#endif
typedef struct _LegacyPrivateIPCControlBlock
{
    FullIPCHeaderLegacyPrivate          FullIPCHeader;

    // Client blocks
    PerfCounterIPCControlBlock          PerfIpcBlock;        // no longer used but kept for compat
    AppDomainEnumerationIPCBlock        AppDomainBlock;
    WCHAR                               InstancePath[MAX_PATH];
} LegacyPrivateIPCControlBlock;
#ifndef _WIN64
#include <poppack.h>
#endif

#include <pshpack4.h>
typedef struct _LegacyPrivateIPCControlBlock_Wow64
{
    FullIPCHeaderLegacyPrivate_Wow64     FullIPCHeader;

    // Client blocks
    PerfCounterIPCControlBlock_Wow64     PerfIpcBlock;        // no longer used but kept for compat
    AppDomainEnumerationIPCBlock_Wow64   AppDomainBlock;
    WCHAR                                InstancePath[MAX_PATH];
} LegacyPrivateIPCControlBlock_Wow64;
#include <poppack.h>


//-----------------------------------------------------------------------------
// LegacyPublic (per process) IPC Block for CLR apps
//-----------------------------------------------------------------------------
#ifndef _WIN64
#include <pshpack4.h>
#endif
typedef struct LegacyPublicIPCControlBlock
{
    FullIPCHeaderLegacyPublic  FullIPCHeaderLegacyPublic;

    // Client blocks
    PerfCounterIPCControlBlock PerfIpcBlock;
} LegacyPublicIPCControlBlock;
#ifndef _WIN64
#include <poppack.h>
#endif

#include <pshpack4.h>
typedef struct _LegacyPublicIPCControlBlock_Wow64
{
    FullIPCHeaderLegacyPublic_Wow64  FullIPCHeaderLegacyPublic;

    // Client blocks
    PerfCounterIPCControlBlock_Wow64 PerfIpcBlock;
} LegacyPublicIPCControlBlock_Wow64;
#include <poppack.h>


//class IPCHeaderLockHolder
//{
//    LONG Counter;
//    BOOL HaveLock;
//    IPCHeader & Header;
//
//  public:
//
//    IPCHeaderLockHolder(IPCHeader & header) : HaveLock(FALSE), Header(header) {}
//
//    BOOL TryGetLock()
//    {
//        _ASSERTE(!HaveLock);
//        LONG oldCounter = Header.Counter;
//        if ((oldCounter & 1) != 0)
//            return FALSE;
//        Counter = oldCounter + 1;
//        if (InterlockedCompareExchange((LONG *)(&(Header.Counter)), Counter, oldCounter) != oldCounter)
//            return FALSE;
//        HaveLock = TRUE;
//
//        return TRUE;
//    }
//
//    BOOL TryGetLock(ULONG numRetries)
//    {
//        ULONG dwSwitchCount = 0;
//
//        for (;;)
//        {
//            if (TryGetLock())
//                return TRUE;
//
//            if (numRetries == 0)
//                return FALSE;
//
//            --numRetries;
//            __SwitchToThread(0, ++dwSwitchCount);
//       }
//    }
//
//    void FreeLock()
//    {
//        _ASSERTE(HaveLock);
//        _ASSERTE(Header.Counter == Counter);
//        ++Counter;
//        Counter = (Counter == 0) ? 2 : Counter;
//        Header.Counter = Counter;
//        HaveLock = FALSE;
//    }
//
//    ~IPCHeaderLockHolder()
//    {
//        if (HaveLock)
//            FreeLock();
//    }
//};
//
//class IPCHeaderReadHelper
//{
//    IPCHeader CachedHeader;
//    IPCHeader * pUnreliableHeader;
//    BOOL IsOpen;
//
//  public:
//
//    IPCHeaderReadHelper() : pUnreliableHeader(NULL), IsOpen(FALSE) {}
//
//    BOOL TryOpenHeader(IPCHeader * header)
//    {
//        _ASSERTE(!IsOpen);
//
//        pUnreliableHeader = header;
//
//        // Read the counter and the runtime ID from the header
//        CachedHeader.Counter = pUnreliableHeader->Counter;
//        if ((CachedHeader.Counter & 1) != 0)
//            return FALSE;
//        CachedHeader.RuntimeId = pUnreliableHeader->RuntimeId;
//
//        // If runtime ID is 0, then this block is not allocated by
//        // a runtime, and thus there is no further work to do
//        if (CachedHeader.RuntimeId == 0)
//        {
//            IsOpen = TRUE;
//            return TRUE;
//        }
//
//        // Read the rest of the values from the header
//        CachedHeader.Reserved1   = pUnreliableHeader->Reserved1;
//        CachedHeader.Reserved2   = pUnreliableHeader->Reserved2;
//        CachedHeader.Version     = pUnreliableHeader->Version;
//        CachedHeader.Flags       = pUnreliableHeader->Flags;
//        CachedHeader.blockSize   = pUnreliableHeader->blockSize;
//        CachedHeader.BuildYear   = pUnreliableHeader->BuildYear;
//        CachedHeader.BuildNumber = pUnreliableHeader->BuildNumber;
//        CachedHeader.numEntries  = pUnreliableHeader->numEntries;
//
//        // Verify that the header did not change during the read
//        LONG counter = pUnreliableHeader->Counter;
//        if (CachedHeader.Counter != counter)
//            return FALSE;
//
//        // Since we know we got a clean read of numEntries, we
//        // should be able to assert this with confidence
//        if (CachedHeader.numEntries == 0)
//        {
//            _ASSERTE(!"numEntries from IPCBlock is zero");
//            return FALSE;
//        }
//        else if (CachedHeader.numEntries > eIPC_MAX)
//        {
//            _ASSERTE(!"numEntries from IPCBlock is too big");
//            return FALSE;
//        }
//
//        if (CachedHeader.blockSize == 0)
//        {
//            _ASSERTE(!"blockSize from IPCBlock is zero");
//            return FALSE;
//        }
//        else if (CachedHeader.blockSize > IPC_BLOCK_SIZE)
//        {
//            _ASSERTE(!"blockSize from IPCBlock is too big");
//            return FALSE;
//        }
//
//        // Copy the table
//        for (ULONG i = 0; i < CachedHeader.numEntries; ++i)
//        {
//            CachedHeader.EntryTable[i].Offset = pUnreliableHeader->EntryTable[i].Offset;
//            CachedHeader.EntryTable[i].Size = pUnreliableHeader->EntryTable[i].Size;
//            if (i == eIPC_PerfCounters)
//            {
//                if(!((SIZE_T)CachedHeader.EntryTable[i].Offset < IPC_BLOCK_SIZE) && ((SIZE_T)CachedHeader.EntryTable[i].Offset + CachedHeader.EntryTable[i].Size <= IPC_BLOCK_SIZE))
//                {
//                    _ASSERTE(!"PerfCounter section offset + size is too large");
//                    return FALSE;
//                }
//            }
//        }
//
//        // If eIPC_MAX > numEntries, then mark the left over
//        // slots in EntryTable as "empty".
//        for (ULONG i = CachedHeader.numEntries; i < eIPC_MAX; ++i)
//        {
//            CachedHeader.EntryTable[i].Offset = EMPTY_ENTRY_OFFSET;
//            CachedHeader.EntryTable[i].Size = EMPTY_ENTRY_SIZE;
//        }
//
//        // Verify again that the header did not change during the read
//        counter = pUnreliableHeader->Counter;
//        if (CachedHeader.Counter != counter)
//            return FALSE;
//
//        IsOpen = TRUE;
//        return TRUE;
//    }
//
//
//    BOOL HeaderHasChanged()
//    {
//        _ASSERTE(IsOpen);
//        LONG counter = pUnreliableHeader->Counter;
//        return (CachedHeader.Counter != counter) ? TRUE : FALSE;
//    }
//
//    BOOL IsSentinal()
//    {
//        _ASSERTE(IsOpen);
//        return (CachedHeader.Counter == 0);
//    }
//
//
//    BOOL UseWow64Structs()
//    {
//        _ASSERTE(IsOpen);
//#if !defined(_TARGET_X86_)
//        return ((CachedHeader.Flags & IPC_FLAG_X86) != 0) ? TRUE : FALSE;
//#else
//        return FALSE;
//#endif
//    }
//
//    void * GetUnreliableSection(EIPCClient eClient)
//    {
//        if (!IsOpen)
//        {
//            _ASSERTE(!"IPCHeaderReadHelper is not open");
//            return NULL;
//        }
//
//        if (eClient < 0 || eClient >= eIPC_MAX)
//        {
//            _ASSERTE(!"eClient is out of bounds");
//            return NULL;
//        }
//
//        if (CachedHeader.EntryTable[eClient].Offset == EMPTY_ENTRY_OFFSET)
//        {
//            _ASSERTE(!"Section is empty");
//            return NULL;
//        }
//
//        return (BYTE*)pUnreliableHeader + (SIZE_T)CachedHeader.EntryTable[eClient].Offset;
//    }
//};

#endif // _IPC_HEADER_H_
