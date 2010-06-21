#ifndef WINSTA_H
#define WINSTA_H

// begin_msdn:http://msdn.microsoft.com/en-us/library/cc248779%28PROT.10%29.aspx

typedef struct _WINSTATIONREMOTEADDRESS
{
    unsigned short sin_family;
    union
    {
        struct
        {
            USHORT sin_port;
            ULONG sin_addr;
            UCHAR sin_zero[8];
        } ipv4;
        struct
        {
            USHORT sin6_port;
            ULONG sin6_flowinfo;
            USHORT sin6_addr[8];
            ULONG sin6_scope_id;
        } ipv6;
    };
} WINSTATIONREMOTEADDRESS, *PWINSTATIONREMOTEADDRESS;

// end_msdn

#endif
