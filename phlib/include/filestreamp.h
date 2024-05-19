/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2010-2016
 *
 */

#ifndef _PH_FILESTREAMP_H
#define _PH_FILESTREAMP_H

VOID NTAPI PhpFileStreamDeleteProcedure(
    _In_ PVOID Object,
    _In_ ULONG Flags
    );

NTSTATUS PhpAllocateBufferFileStream(
    _Inout_ PPH_FILE_STREAM FileStream
    );

NTSTATUS PhpReadFileStream(
    _Inout_ PPH_FILE_STREAM FileStream,
    _Out_writes_bytes_(Length) PVOID Buffer,
    _In_ ULONG Length,
    _Out_opt_ PULONG ReadLength
    );

NTSTATUS PhpWriteFileStream(
    _Inout_ PPH_FILE_STREAM FileStream,
    _In_reads_bytes_(Length) PVOID Buffer,
    _In_ ULONG Length
    );

NTSTATUS PhpFlushReadFileStream(
    _Inout_ PPH_FILE_STREAM FileStream
    );

NTSTATUS PhpFlushWriteFileStream(
    _Inout_ PPH_FILE_STREAM FileStream
    );

NTSTATUS PhpSeekFileStream(
    _Inout_ PPH_FILE_STREAM FileStream,
    _In_ PLARGE_INTEGER Offset,
    _In_ PH_SEEK_ORIGIN Origin
    );

#endif
