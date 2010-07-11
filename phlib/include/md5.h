#ifndef _MD5_H
#define _MD5_H

typedef struct
{
    ULONG i[2];
    ULONG buf[4];
    UCHAR in[64];
    UCHAR digest[16];
} MD5_CTX;

VOID NTAPI MD5Init(
    __out MD5_CTX *Context
    );

VOID NTAPI MD5Update(
    __inout MD5_CTX *Context,
    __in_bcount(Length) UCHAR *Input,
    __in ULONG Length
    );

VOID NTAPI MD5Final(
    __inout MD5_CTX *Context
    );

#endif
