#ifndef PHNET_H
#define PHNET_H

#include <inaddr.h>
#include <in6addr.h>

#define PH_IPV4_NETWORK_TYPE 0x1
#define PH_IPV6_NETWORK_TYPE 0x2
#define PH_NETWORK_TYPE_MASK 0x3

#define PH_TCP_PROTOCOL_TYPE 0x10
#define PH_UDP_PROTOCOL_TYPE 0x20
#define PH_PROTOCOL_TYPE_MASK 0x30

#define PH_NO_NETWORK_PROTOCOL 0x0
#define PH_TCP4_NETWORK_PROTOCOL (PH_IPV4_NETWORK_TYPE | PH_TCP_PROTOCOL_TYPE)
#define PH_TCP6_NETWORK_PROTOCOL (PH_IPV6_NETWORK_TYPE | PH_TCP_PROTOCOL_TYPE)
#define PH_UDP4_NETWORK_PROTOCOL (PH_IPV4_NETWORK_TYPE | PH_UDP_PROTOCOL_TYPE)
#define PH_UDP6_NETWORK_PROTOCOL (PH_IPV6_NETWORK_TYPE | PH_UDP_PROTOCOL_TYPE)

typedef struct _PH_IP_ADDRESS
{
    ULONG Type;
    union
    {
        ULONG Ipv4;
        struct in_addr InAddr;
        UCHAR Ipv6[16];
        struct in6_addr In6Addr;
    };
} PH_IP_ADDRESS, *PPH_IP_ADDRESS;

FORCEINLINE BOOLEAN PhIpAddressEquals(
    __in PPH_IP_ADDRESS Address1,
    __in PPH_IP_ADDRESS Address2
    )
{
    if ((Address1->Type | Address2->Type) == 0) // don't check addresses if both are invalid
        return TRUE;
    if (Address1->Type != Address2->Type)
        return FALSE;

    if (Address1->Type == PH_IPV4_NETWORK_TYPE)
    {
        return Address1->Ipv4 == Address2->Ipv4;
    }
    else
    {
#ifdef _M_IX86
        return
            *(PULONG)(Address1->Ipv6) == *(PULONG)(Address2->Ipv6) &&
            *(PULONG)(Address1->Ipv6 + 4) == *(PULONG)(Address2->Ipv6 + 4) &&
            *(PULONG)(Address1->Ipv6 + 8) == *(PULONG)(Address2->Ipv6+ 8) &&
            *(PULONG)(Address1->Ipv6 + 12) == *(PULONG)(Address2->Ipv6 + 12);
#else
        return
            *(PULONG64)(Address1->Ipv6) == *(PULONG64)(Address2->Ipv6) &&
            *(PULONG64)(Address1->Ipv6 + 8) == *(PULONG64)(Address2->Ipv6 + 8);
#endif
    }
}

FORCEINLINE ULONG PhHashIpAddress(
    __in PPH_IP_ADDRESS Address
    )
{
    ULONG hash = 0;

    if (Address->Type == 0)
        return 0;

    hash = Address->Type | (Address->Type << 16);

    if (Address->Type == PH_IPV4_NETWORK_TYPE)
    {
        hash ^= Address->Ipv4;
    }
    else
    {
        hash += *(PULONG)(Address->Ipv6);
        hash ^= *(PULONG)(Address->Ipv6 + 4);
        hash += *(PULONG)(Address->Ipv6 + 8);
        hash ^= *(PULONG)(Address->Ipv6 + 12);
    }

    return hash;
}

FORCEINLINE BOOLEAN PhIsIpAddressNull(
    __in PPH_IP_ADDRESS Address
    )
{
    if (Address->Type == 0)
    {
        return TRUE;
    }
    else if (Address->Type == PH_IPV4_NETWORK_TYPE)
    {
        return Address->Ipv4 == 0;
    }
    else if (Address->Type == PH_IPV6_NETWORK_TYPE)
    {
#ifdef _M_IX86
        return (*(PULONG)(Address->Ipv6) | *(PULONG)(Address->Ipv6 + 4) |
            *(PULONG)(Address->Ipv6 + 8) | *(PULONG)(Address->Ipv6 + 12)) == 0;
#else
        return (*(PULONG64)(Address->Ipv6) | *(PULONG64)(Address->Ipv6 + 8)) == 0;
#endif
    }
    else
    {
        return TRUE;
    }
}

typedef struct _PH_IP_ENDPOINT
{
    PH_IP_ADDRESS Address;
    ULONG Port;
} PH_IP_ENDPOINT, *PPH_IP_ENDPOINT;

FORCEINLINE BOOLEAN PhIpEndpointEquals(
    __in PPH_IP_ENDPOINT Endpoint1,
    __in PPH_IP_ENDPOINT Endpoint2
    )
{
    return
        PhIpAddressEquals(&Endpoint1->Address, &Endpoint2->Address) &&
        Endpoint1->Port == Endpoint2->Port;
}

FORCEINLINE ULONG PhHashIpEndpoint(
    __in PPH_IP_ENDPOINT Endpoint
    )
{
    return PhHashIpAddress(&Endpoint->Address) ^ Endpoint->Port;
}

#endif
