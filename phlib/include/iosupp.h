#ifndef _PH_IOSUPP_H
#define _PH_IOSUPP_H

VOID NTAPI PhpFileStreamDeleteProcedure(
    __in PVOID Object,
    __in ULONG Flags
    );

NTSTATUS PhpFileStreamAllocateBuffer(
    __inout PPH_FILE_STREAM FileStream
    );

NTSTATUS PhpFileStreamRead(
    __inout PPH_FILE_STREAM FileStream,
    __out_bcount(Length) PVOID Buffer,
    __in ULONG Length,
    __out_opt PULONG ReadLength
    );

NTSTATUS PhpFileStreamWrite(
    __inout PPH_FILE_STREAM FileStream,
    __in_bcount(Length) PVOID Buffer,
    __in ULONG Length
    );

NTSTATUS PhpFileStreamFlushRead(
    __inout PPH_FILE_STREAM FileStream
    );

NTSTATUS PhpFileStreamFlushWrite(
    __inout PPH_FILE_STREAM FileStream
    );

NTSTATUS PhpFileStreamSeek(
    __inout PPH_FILE_STREAM FileStream,
    __in PLARGE_INTEGER Offset,
    __in PH_SEEK_ORIGIN Origin
    );

#endif
