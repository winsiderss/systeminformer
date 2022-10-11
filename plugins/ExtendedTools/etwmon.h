#ifndef ETWMON_H
#define ETWMON_H

#include <evntcons.h>

typedef struct
{
    ULONG DiskNumber;
    ULONG IrpFlags;
    ULONG TransferSize;
    ULONG ResponseTime;
    ULONGLONG ByteOffset;
    ULONG_PTR FileObject;
    ULONG_PTR Irp;
    ULONGLONG HighResResponseTime;
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

#define KERNEL_FILE_KEYWORD_FILENAME 0x10
#define KERNEL_FILE_KEYWORD_FILEIO 0x20

// etwmon

VOID EtEtwMonitorInitialization(
    VOID
    );

VOID EtEtwMonitorUninitialization(
    VOID
    );

VOID EtStartEtwSession(
    VOID
    );

VOID EtStopEtwSession(
    VOID
    );

ULONG EtStartEtwRundown(
    VOID
    );

// etwstat

#define EVENT_TRACE_TYPE_TCPIP_SEND_IPV6 0x1A
#define EVENT_TRACE_TYPE_TCPIP_RECEIVE_IPV6 0x1B

#define EVENT_TRACE_TYPE_FILENAME 0x0
#define EVENT_TRACE_TYPE_FILENAME_CREATE 0x20
#define EVENT_TRACE_TYPE_FILENAME_SAME 0x21
#define EVENT_TRACE_TYPE_FILENAME_NULL 0x22
#define EVENT_TRACE_TYPE_FILENAME_DELETE 0x23
#define EVENT_TRACE_TYPE_FILENAME_RUNDOWN 0x24

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
    ULONGLONG HighResResponseTime;
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

typedef struct _ET_ETW_STACKWALK_EVENT
{
    ULONGLONG EventTimeStamp;
    ULONG StackProcess;
    ULONG StackThread;
    ULONG_PTR Stack[192];
} ET_ETW_STACKWALK_EVENT, *PET_ETW_STACKWALK_EVENT;
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
