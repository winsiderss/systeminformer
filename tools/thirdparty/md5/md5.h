#ifndef _MD5_H
#define _MD5_H

typedef struct
{
    ULONG i[2];
    ULONG buf[4];
    UCHAR in[64];
    UCHAR digest[16];
} MD5_CTX;

VOID MD5Init(
    _Out_ MD5_CTX *Context
    );

VOID MD5Update(
    _Inout_ MD5_CTX *Context,
    _In_reads_bytes_(Length) UCHAR *Input,
    _In_ ULONG Length
    );

VOID MD5Final(
    _Inout_ MD5_CTX *Context
    );

#endif
