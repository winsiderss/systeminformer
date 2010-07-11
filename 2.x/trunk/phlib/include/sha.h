#ifndef _SHA_H
#define _SHA_H

typedef struct
{
    ULONG flag;
    UCHAR hash[20];
    ULONG state[5];
    ULONG count[2];
    UCHAR buffer[64];
} A_SHA_CTX;

VOID NTAPI A_SHAInit(
    __out A_SHA_CTX *Context
    );

VOID NTAPI A_SHAUpdate(
    __inout A_SHA_CTX *Context,
    __in_bcount(Length) UCHAR *Input,
    __in ULONG Length
    );

VOID NTAPI A_SHAFinal(
    __inout A_SHA_CTX *Context,
    __out_bcount(20) UCHAR *Hash
    );

#endif
