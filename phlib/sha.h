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

VOID A_SHAInit(
    _Out_ A_SHA_CTX *Context
    );

VOID A_SHAUpdate(
    _Inout_ A_SHA_CTX *Context,
    _In_reads_bytes_(Length) UCHAR *Input,
    _In_ ULONG Length
    );

VOID A_SHAFinal(
    _Inout_ A_SHA_CTX *Context,
    _Out_writes_bytes_(20) UCHAR *Hash
    );

#endif
