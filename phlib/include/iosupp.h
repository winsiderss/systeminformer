#ifndef _PH_IOSUPP_H
#define _PH_IOSUPP_H

VOID NTAPI PhpFileStreamDeleteProcedure(
    __in PVOID Object,
    __in ULONG Flags
    );

NTSTATUS PhpAllocateBufferFileStream(
    __inout PPH_FILE_STREAM FileStream
    );

NTSTATUS PhpReadFileStream(
    __inout PPH_FILE_STREAM FileStream,
    __out_bcount(Length) PVOID Buffer,
    __in ULONG Length,
    __out_opt PULONG ReadLength
    );

NTSTATUS PhpWriteFileStream(
    __inout PPH_FILE_STREAM FileStream,
    __in_bcount(Length) PVOID Buffer,
    __in ULONG Length
    );

NTSTATUS PhpFlushReadFileStream(
    __inout PPH_FILE_STREAM FileStream
    );

NTSTATUS PhpFlushWriteFileStream(
    __inout PPH_FILE_STREAM FileStream
    );

NTSTATUS PhpSeekFileStream(
    __inout PPH_FILE_STREAM FileStream,
    __in PLARGE_INTEGER Offset,
    __in PH_SEEK_ORIGIN Origin
    );

#endif
