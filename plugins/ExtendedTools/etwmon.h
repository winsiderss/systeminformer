#ifndef ETWMON_H
#define ETWMON_H

#include <evntcons.h>

typedef ULONG wmi_uint32;
typedef ULONG wmi_IPAddrV4;
typedef IN6_ADDR wmi_IPAddrV6;
typedef USHORT wmi_Port;

typedef struct
{
    wmi_uint32 LowPart;
    wmi_uint32 HighPart;
} wmi_uint64;

typedef struct
{
    wmi_uint32 DiskNumber;
    wmi_uint32 IrpFlags;
    wmi_uint32 TransferSize;
    wmi_uint32 ResponseTime;
    wmi_uint64 ByteOffset;
    // Other members not included.
} DiskIo_TypeGroup1;

typedef struct
{
    wmi_uint32 PID;
    wmi_uint32 size;
    wmi_IPAddrV4 daddr;
    wmi_IPAddrV4 saddr;
    wmi_Port dport;
    wmi_Port sport;
} TcpIpOrUdpIp_IPV4_Header;

typedef struct
{
    wmi_uint32 PID;
    wmi_uint32 size;
    wmi_IPAddrV6 daddr;
    wmi_IPAddrV6 saddr;
    wmi_Port dport;
    wmi_Port sport;
} TcpIpOrUdpIp_IPV6_Header;

// etwmon

VOID EtEtwMonitorInitialization();

VOID EtEtwMonitorUninitialization();

VOID EtStartEtwSession();

VOID EtStopEtwSession();

VOID EtFlushEtwSession();

// etwstat

typedef enum _ET_ETW_EVENT_TYPE
{
    EtEtwDiskReadType = 1,
    EtEtwDiskWriteType,
    EtEtwNetworkReceiveType,
    EtEtwNetworkSendType
} ET_ETW_EVENT_TYPE;

typedef struct _ET_ETW_DISK_EVENT
{
    ET_ETW_EVENT_TYPE Type;
    CLIENT_ID ClientId;
    ULONG TransferSize;
} ET_ETW_DISK_EVENT, *PET_ETW_DISK_EVENT;

typedef struct _ET_ETW_NETWORK_EVENT
{
    ET_ETW_EVENT_TYPE Type;
    CLIENT_ID ClientId;
    ULONG ProtocolType;
    ULONG TransferSize;
    PH_IP_ENDPOINT LocalEndpoint;
    PH_IP_ENDPOINT RemoteEndpoint;
} ET_ETW_NETWORK_EVENT, *PET_ETW_NETWORK_EVENT;

VOID EtProcessDiskEvent(
    __in PET_ETW_DISK_EVENT Event
    );

VOID EtProcessNetworkEvent(
    __in PET_ETW_NETWORK_EVENT Event
    );

#endif
