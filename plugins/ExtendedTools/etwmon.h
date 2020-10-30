#ifndef ETWMON_H
#define ETWMON_H

#include <evntcons.h>

typedef struct
{
    ULONG DiskNumber;
    ULONG IrpFlags;
    ULONG TransferSize;
    ULONG ResponseTime;
    ULONG64 ByteOffset;
    ULONG_PTR FileObject;
    ULONG_PTR Irp;
    ULONG64 HighResResponseTime;
    ULONG IssuingThreadId; // since WIN8 (ETW_DISKIO_READWRITE_V3)
} DiskIo_TypeGroup1;

typedef struct
{
    ULONG_PTR FileObject;
    WCHAR FileName[1];
} FileIo_Name;

typedef struct
{
    ULONGLONG FileObject;
    WCHAR FileName[1];
} FileIo_Name_Wow64;

typedef struct
{
    ULONG PID;
    ULONG size;
    IN_ADDR daddr;
    IN_ADDR saddr;
    USHORT dport;
    USHORT sport;
} TcpIpOrUdpIp_IPV4_Header;

typedef struct
{
    ULONG PID;
    ULONG size;
    IN6_ADDR daddr;
    IN6_ADDR saddr;
    USHORT dport;
    USHORT sport;
} TcpIpOrUdpIp_IPV6_Header;

// etwmon

VOID EtEtwMonitorInitialization(
    VOID
    );

VOID EtEtwMonitorUninitialization(
    VOID
    );

ULONG EtEtwControlEtwSession(
    _In_ ULONG ControlCode
    );

VOID EtFlushEtwSession(
    VOID
    );

// etwstat

typedef enum _ET_ETW_EVENT_TYPE
{
    EtEtwDiskReadType = 1,
    EtEtwDiskWriteType,
    EtEtwFileNameType,
    EtEtwFileCreateType,
    EtEtwFileDeleteType,
    EtEtwFileRundownType,
    EtEtwNetworkReceiveType,
    EtEtwNetworkSendType
} ET_ETW_EVENT_TYPE;

typedef struct _ET_ETW_DISK_EVENT
{
    ET_ETW_EVENT_TYPE Type;
    CLIENT_ID ClientId;
    ULONG IrpFlags;
    ULONG TransferSize;
    PVOID FileObject;
    ULONG64 HighResResponseTime;
} ET_ETW_DISK_EVENT, *PET_ETW_DISK_EVENT;

typedef struct _ET_ETW_FILE_EVENT
{
    ET_ETW_EVENT_TYPE Type;
    PVOID FileObject;
    PH_STRINGREF FileName;
} ET_ETW_FILE_EVENT, *PET_ETW_FILE_EVENT;

typedef struct _ET_ETW_NETWORK_EVENT
{
    ET_ETW_EVENT_TYPE Type;
    CLIENT_ID ClientId;
    ULONG ProtocolType;
    ULONG TransferSize;
    PH_IP_ENDPOINT LocalEndpoint;
    PH_IP_ENDPOINT RemoteEndpoint;
} ET_ETW_NETWORK_EVENT, *PET_ETW_NETWORK_EVENT;

// etwstat

VOID EtProcessDiskEvent(
    _In_ PET_ETW_DISK_EVENT Event
    );

VOID EtProcessNetworkEvent(
    _In_ PET_ETW_NETWORK_EVENT Event
    );

HANDLE EtThreadIdToProcessId(
    _In_ HANDLE ThreadId
    );

// etwdisk

VOID EtDiskProcessDiskEvent(
    _In_ PET_ETW_DISK_EVENT Event
    );

VOID EtDiskProcessFileEvent(
    _In_ PET_ETW_FILE_EVENT Event
    );

#endif
