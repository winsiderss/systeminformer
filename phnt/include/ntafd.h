/*
 * Ancillary Function Driver definitions
 *
 * This file is part of System Informer.
 */

#ifndef _NTAFD_H
#define _NTAFD_H

#include <WinSock2.h>
#include <MSWSock.h>
#include <tdi.h>

// private
#define AFD_DEVICE_NAME L"\\Device\\Afd"

// private // Extended Attributes
#define AfdOpenPacket      "AfdOpenPacketXX"    // AFD_OPEN_PACKET
#define AfdSwOpenPacket    "AfdSwOpenPacket"    // AFD_SWITCH_OPEN_PACKET // rev
#define AfdRioRDOpenPacket "AfdRioRDOpenPacket" // void // rev

// private
typedef struct _AFD_ENDPOINT_FLAGS
{
    union
    {
        struct
        {
            UCHAR ConnectionLess : 1;
            UCHAR : 3;
            UCHAR MessageMode : 1;
            UCHAR Raw : 1;
            UCHAR : 3;
            UCHAR Multipoint : 1;
            UCHAR C_Root : 1;
            UCHAR : 3;
            UCHAR D_Root : 1;
            UCHAR IgnoreTDI : 1;
            UCHAR : 3;
            UCHAR RioSocket : 1;
        };
        ULONG EndpointFlags;
    };
} AFD_ENDPOINT_FLAGS, *PAFD_ENDPOINT_FLAGS;

// private // Transport device names
#define DD_TCP_DEVICE_NAME      L"\\Device\\Tcp"
#define DD_TCPV6_DEVICE_NAME    L"\\Device\\Tcp6"
#define DD_UDP_DEVICE_NAME      L"\\Device\\Udp"
#define DD_UDPV6_DEVICE_NAME    L"\\Device\\Udp6"
#define DD_RAW_IP_DEVICE_NAME   L"\\Device\\RawIp"
#define DD_RAW_IPV6_DEVICE_NAME L"\\Device\\RawIp6"

// private
typedef struct _AFD_OPEN_PACKET
{
    _In_ AFD_ENDPOINT_FLAGS __f;
    _In_opt_ GROUP GroupID;
    _In_ LONG AddressFamily; // AF_*
    _In_ LONG SocketType; // SOCK_*
    _In_ LONG Protocol; // IPPROTO_*, BTHPROTO_*, HV_PROTOCOL_*, etc.
    _In_opt_ ULONG TransportDeviceNameLength; // Note: specifying a device changes the transport mode
    _Field_size_bytes_opt_(TransportDeviceNameLength) WCHAR TransportDeviceName[ANYSIZE_ARRAY];
} AFD_OPEN_PACKET, *PAFD_OPEN_PACKET;

// rev (FILE_FULL_EA_INFORMATION + AfdOpenPacket + AFD_OPEN_PACKET)
typedef struct _AFD_OPEN_PACKET_FULL_EA
{
    ULONG NextEntryOffset;
    UCHAR Flags;
    UCHAR EaNameLength; // sizeof(AfdOpenPacket) - sizeof(ANSI_NULL);
    USHORT EaValueLength; // sizeof(AFD_OPEN_PACKET)
    CHAR EaName[sizeof(AfdOpenPacket)];
    AFD_OPEN_PACKET OpenPacket;
} AFD_OPEN_PACKET_FULL_EA, *PAFD_OPEN_PACKET_FULL_EA;

// private
typedef struct _AFD_SWITCH_OPEN_PACKET
{
    HANDLE CompletionPort;
    HANDLE CompletionEvent;
} AFD_SWITCH_OPEN_PACKET, *PAFD_SWITCH_OPEN_PACKET;

//
// Since Vista, sockets can use different modes of transport: TLI, TDI, and hybrid. The mode is selected
// based on whether the caller specifies the transport device at socket creation and whether this device
// is suitable for hybrid operation. No device means TLI, which is the most common choice.
//
// The transport mode affects which structures AFD uses for IOCTLs on the socket:
//  - TLI sockets use *_TL structures (where applicable) and SOCKADDR for addresses.
//  - TDI and hybrid sockets use non-TL structures and TDI_ADDRESS_INFO for addresses.
//

// private // IOCTL function numbers
#define AFD_BIND                        0 // in: AFD_BIND_INFO_TL; out: SOCKADDR /or/ in: AFD_BIND_INFO; out: TDI_ADDRESS_INFO (depending on transport mode)
#define AFD_CONNECT                     1 // in: AFD_CONNECT_JOIN_INFO_TL or AFD_CONNECT_JOIN_INFO (depending on transport mode); out (opt): IO_STATUS_BLOCK
#define AFD_START_LISTEN                2 // in: AFD_LISTEN_INFO
#define AFD_WAIT_FOR_LISTEN             3 // out: AFD_LISTEN_RESPONSE_INFO_TL or AFD_LISTEN_RESPONSE_INFO (depending on transport mode)
#define AFD_ACCEPT                      4 // in: AFD_ACCEPT_INFO
#define AFD_RECEIVE                     5 // in: AFD_RECV_INFO
#define AFD_RECEIVE_DATAGRAM            6 // in: AFD_DATAGRAM_INFO
#define AFD_SEND                        7 // in: AFD_SEND_INFO
#define AFD_SEND_DATAGRAM               8 // in: AFD_SEND_DATAGRAM_INFO
#define AFD_POLL                        9 // in, out: AFD_POLL_INFO
#define AFD_PARTIAL_DISCONNECT          10 // in: AFD_PARTIAL_DISCONNECT_INFO
#define AFD_GET_ADDRESS                 11 // out: AFD_ADDRESS (SOCKADDR or TDI_ADDRESS_INFO, depending on transport mode)
#define AFD_QUERY_RECEIVE_INFO          12 // out: AFD_RECEIVE_INFORMATION
#define AFD_QUERY_HANDLES               13 // in: ULONG (AFD_QUERY_*); out: AFD_HANDLE_INFO
#define AFD_SET_INFORMATION             14 // in: AFD_INFORMATION
#define AFD_GET_REMOTE_ADDRESS          15 // out: AFD_ADDRESS (SOCKADDR or TDI_ADDRESS_INFO, depending on transport mode)
#define AFD_GET_CONTEXT                 16 // out: SOCK_SHARED_INFO (on Win32 level) or custom data
#define AFD_SET_CONTEXT                 17 // in: SOCK_SHARED_INFO (on Win32 level) or custom data; out: AFD_ADDRESS (SOCKADDR or TDI_ADDRESS_INFO, depending on transport mode; output buffer must be inside the input buffer)
#define AFD_SET_CONNECT_DATA            18 // in: AFD_UNACCEPTED_CONNECT_DATA_INFO; out: payload
#define AFD_SET_CONNECT_OPTIONS         19 // in: AFD_UNACCEPTED_CONNECT_DATA_INFO; out: payload
#define AFD_SET_DISCONNECT_DATA         20 // in: AFD_UNACCEPTED_CONNECT_DATA_INFO; out: payload
#define AFD_SET_DISCONNECT_OPTIONS      21 // in: AFD_UNACCEPTED_CONNECT_DATA_INFO; out: payload
#define AFD_GET_CONNECT_DATA            22 // in: AFD_UNACCEPTED_CONNECT_DATA_INFO; out: payload
#define AFD_GET_CONNECT_OPTIONS         23 // in: AFD_UNACCEPTED_CONNECT_DATA_INFO; out: payload
#define AFD_GET_DISCONNECT_DATA         24 // in: AFD_UNACCEPTED_CONNECT_DATA_INFO; out: payload
#define AFD_GET_DISCONNECT_OPTIONS      25 // in: AFD_UNACCEPTED_CONNECT_DATA_INFO; out: payload
#define AFD_SIZE_CONNECT_DATA           26 // in: AFD_UNACCEPTED_CONNECT_DATA_INFO; out: payload
#define AFD_SIZE_CONNECT_OPTIONS        27 // in: AFD_UNACCEPTED_CONNECT_DATA_INFO; out: payload
#define AFD_SIZE_DISCONNECT_DATA        28 // in: AFD_UNACCEPTED_CONNECT_DATA_INFO; out: payload
#define AFD_SIZE_DISCONNECT_OPTIONS     29 // in: AFD_UNACCEPTED_CONNECT_DATA_INFO; out: payload
#define AFD_GET_INFORMATION             30 // in, out: AFD_INFORMATION
#define AFD_TRANSMIT_FILE               31 // in: AFD_TRANSMIT_FILE_INFO; out (opt): BOOLEAN
#define AFD_SUPER_ACCEPT                32 // in: AFD_SUPER_ACCEPT_INFO; out: received data + local address + remote address (at offsets according to the input)
#define AFD_EVENT_SELECT                33 // in: AFD_EVENT_SELECT_INFO
#define AFD_ENUM_NETWORK_EVENTS         34 // in (opt): HANDLE; out: AFD_ENUM_NETWORK_EVENTS_INFO
#define AFD_DEFER_ACCEPT                35 // in: AFD_DEFER_ACCEPT_INFO
#define AFD_WAIT_FOR_LISTEN_LIFO        36 // out: AFD_LISTEN_RESPONSE_INFO_TL or AFD_LISTEN_RESPONSE_INFO (depending on transport mode)
#define AFD_SET_QOS                     37 // in: AFD_QOS_INFO
#define AFD_GET_QOS                     38 // out: AFD_QOS_INFO
#define AFD_NO_OPERATION                39 // in (opt): IO_STATUS_BLOCK
#define AFD_VALIDATE_GROUP              40 // in: AFD_VALIDATE_GROUP_INFO
#define AFD_GET_UNACCEPTED_CONNECT_DATA 41 // in: AFD_UNACCEPTED_CONNECT_DATA_INFO; out: AFD_UNACCEPTED_CONNECT_DATA_INFO (when LengthOnly is set) or received data
#define AFD_ROUTING_INTERFACE_QUERY     42 // in: TRANSPORT_ADDRESS; out: SOCKADDR
#define AFD_ROUTING_INTERFACE_CHANGE    43 // in: AFD_TRANSPORT_IOCTL_INFO
#define AFD_ADDRESS_LIST_QUERY          44 // in: USHORT (TDI_ADDRESS_TYPE_*/AF_*); out: TRANSPORT_ADDRESS
#define AFD_ADDRESS_LIST_CHANGE         45 // in: AFD_TRANSPORT_IOCTL_INFO
#define AFD_JOIN_LEAF                   46 // in: AFD_CONNECT_JOIN_INFO_TL or AFD_CONNECT_JOIN_INFO (depending on transport mode); out (opt): IO_STATUS_BLOCK
#define AFD_TRANSPORT_IOCTL             47 // in: AFD_TL_IO_CONTROL_INFO; out: variable
#define AFD_TRANSMIT_PACKETS            48 // in: AFD_TPACKETS_INFO; out (opt): BOOLEAN
#define AFD_SUPER_CONNECT               49 // in: AFD_SUPER_CONNECT_INFO_TL or AFD_SUPER_CONNECT_INFO (depending on transport mode); out: payload
#define AFD_SUPER_DISCONNECT            50 // in: AFD_SUPER_DISCONNECT_INFO
#define AFD_RECEIVE_MESSAGE             51 // in: AFD_MESSAGE_INFO
#define AFD_SEND_MESSAGE                52 // in: AFD_MESSAGE_INFO // rev // since VISTA // breaks layout; values below are different on XP
#define AFD_SWITCH_CEMENT_SAN           53 // in: AFD_SWITCH_CONTEXT_INFO
#define AFD_SWITCH_SET_EVENTS           54 // in: AFD_SWITCH_EVENT_INFO
#define AFD_SWITCH_RESET_EVENTS         55 // in: AFD_SWITCH_EVENT_INFO
#define AFD_SWITCH_CONNECT_IND          56 // in: AFD_SWITCH_CONNECT_INFO; out: AFD_SWITCH_ACCEPT_INFO
#define AFD_SWITCH_CMPL_ACCEPT          57 // in: AFD_SWITCH_CONTEXT_INFO; out: payload
#define AFD_SWITCH_CMPL_REQUEST         58 // in: AFD_SWITCH_REQUEST_INFO; out: payload
#define AFD_SWITCH_CMPL_IO              59 // in: IO_STATUS_BLOCK
#define AFD_SWITCH_REFRESH_ENDP         60 // in: AFD_SWITCH_CONTEXT_INFO
#define AFD_SWITCH_GET_PHYSICAL_ADDR    61 // deprecated
#define AFD_SWITCH_ACQUIRE_CTX          62 // in: AFD_SWITCH_ACQUIRE_CTX_INFO; out: payload
#define AFD_SWITCH_TRANSFER_CTX         63 // in: AFD_SWITCH_TRANSFER_CTX_INFO
#define AFD_SWITCH_GET_SERVICE_PID      64 // in/out: void; returns PID in IO_STATUS_BLOCK.Information
#define AFD_SWITCH_SET_SERVICE_PROCESS  65 // in/out: void
#define AFD_SWITCH_PROVIDER_CHANGE      66 // in/out: void
#define AFD_SWITCH_ADDRLIST_CHANGE      67 // in: AFD_TRANSPORT_IOCTL_INFO
#define AFD_UNBIND                      68 // in: AFD_UNBIND_INFO // rev // since VISTA
#define AFD_SQM                         69 // in: AFD_SQM_INFO // rev // since WIN7
#define AFD_RIO                         70 // in/out: AFD_RIO_COMMAND_HEADER // rev // since WIN8
#define AFD_TRANSFER_BEGIN              71 // in/out: void // rev // since TH1
#define AFD_TRANSFER_END                72 // in/out: void // rev // since TH1
#define AFD_NOTIFY                      73 // rev // since 22H2

// private // Note: different bit layout from CTL_CODE
#define FSCTL_AFD_BASE  FILE_DEVICE_NETWORK
#define _AFD_CONTROL_CODE(request, method) (FSCTL_AFD_BASE << 12 | request << 2 | method)

// private // IOCTLs
#define IOCTL_AFD_BIND                        _AFD_CONTROL_CODE(AFD_BIND, METHOD_NEITHER) // 0x12003
#define IOCTL_AFD_CONNECT                     _AFD_CONTROL_CODE(AFD_CONNECT, METHOD_NEITHER) // 0x12007
#define IOCTL_AFD_START_LISTEN                _AFD_CONTROL_CODE(AFD_START_LISTEN, METHOD_NEITHER) // 0x1200B
#define IOCTL_AFD_WAIT_FOR_LISTEN             _AFD_CONTROL_CODE(AFD_WAIT_FOR_LISTEN, METHOD_BUFFERED) // 0x1200C
#define IOCTL_AFD_ACCEPT                      _AFD_CONTROL_CODE(AFD_ACCEPT, METHOD_BUFFERED) // 0x12010
#define IOCTL_AFD_RECEIVE                     _AFD_CONTROL_CODE(AFD_RECEIVE, METHOD_NEITHER) // 0x12017
#define IOCTL_AFD_RECEIVE_DATAGRAM            _AFD_CONTROL_CODE(AFD_RECEIVE_DATAGRAM, METHOD_NEITHER) // 0x1201B
#define IOCTL_AFD_SEND                        _AFD_CONTROL_CODE(AFD_SEND, METHOD_NEITHER) // 0x1201F
#define IOCTL_AFD_SEND_DATAGRAM               _AFD_CONTROL_CODE(AFD_SEND_DATAGRAM, METHOD_NEITHER) // 0x12023
#define IOCTL_AFD_POLL                        _AFD_CONTROL_CODE(AFD_POLL, METHOD_BUFFERED) // 0x12024
#define IOCTL_AFD_PARTIAL_DISCONNECT          _AFD_CONTROL_CODE(AFD_PARTIAL_DISCONNECT, METHOD_NEITHER) // 0x1202B
#define IOCTL_AFD_GET_ADDRESS                 _AFD_CONTROL_CODE(AFD_GET_ADDRESS, METHOD_NEITHER) // 0x1202F
#define IOCTL_AFD_QUERY_RECEIVE_INFO          _AFD_CONTROL_CODE(AFD_QUERY_RECEIVE_INFO, METHOD_NEITHER) // 0x12033
#define IOCTL_AFD_QUERY_HANDLES               _AFD_CONTROL_CODE(AFD_QUERY_HANDLES, METHOD_NEITHER) // 0x12037
#define IOCTL_AFD_SET_INFORMATION             _AFD_CONTROL_CODE(AFD_SET_INFORMATION, METHOD_NEITHER) // 0x1203B
#define IOCTL_AFD_GET_REMOTE_ADDRESS          _AFD_CONTROL_CODE(AFD_GET_REMOTE_ADDRESS, METHOD_NEITHER) // 0x1203F
#define IOCTL_AFD_GET_CONTEXT                 _AFD_CONTROL_CODE(AFD_GET_CONTEXT, METHOD_NEITHER) // 0x12043
#define IOCTL_AFD_SET_CONTEXT                 _AFD_CONTROL_CODE(AFD_SET_CONTEXT, METHOD_NEITHER) // 0x12047
#define IOCTL_AFD_SET_CONNECT_DATA            _AFD_CONTROL_CODE(AFD_SET_CONNECT_DATA, METHOD_NEITHER) // 0x1204B
#define IOCTL_AFD_SET_CONNECT_OPTIONS         _AFD_CONTROL_CODE(AFD_SET_CONNECT_OPTIONS, METHOD_NEITHER) // 0x1204F
#define IOCTL_AFD_SET_DISCONNECT_DATA         _AFD_CONTROL_CODE(AFD_SET_DISCONNECT_DATA, METHOD_NEITHER) // 0x12053
#define IOCTL_AFD_SET_DISCONNECT_OPTIONS      _AFD_CONTROL_CODE(AFD_SET_DISCONNECT_OPTIONS, METHOD_NEITHER) // 0x12057
#define IOCTL_AFD_GET_CONNECT_DATA            _AFD_CONTROL_CODE(AFD_GET_CONNECT_DATA, METHOD_NEITHER) // 0x1205B
#define IOCTL_AFD_GET_CONNECT_OPTIONS         _AFD_CONTROL_CODE(AFD_GET_CONNECT_OPTIONS, METHOD_NEITHER) // 0x1205F
#define IOCTL_AFD_GET_DISCONNECT_DATA         _AFD_CONTROL_CODE(AFD_GET_DISCONNECT_DATA, METHOD_NEITHER) // 0x12063
#define IOCTL_AFD_GET_DISCONNECT_OPTIONS      _AFD_CONTROL_CODE(AFD_GET_DISCONNECT_OPTIONS, METHOD_NEITHER) // 0x12067
#define IOCTL_AFD_SIZE_CONNECT_DATA           _AFD_CONTROL_CODE(AFD_SIZE_CONNECT_DATA, METHOD_NEITHER) // 0x1206B
#define IOCTL_AFD_SIZE_CONNECT_OPTIONS        _AFD_CONTROL_CODE(AFD_SIZE_CONNECT_OPTIONS, METHOD_NEITHER) // 0x1206F
#define IOCTL_AFD_SIZE_DISCONNECT_DATA        _AFD_CONTROL_CODE(AFD_SIZE_DISCONNECT_DATA, METHOD_NEITHER) // 0x12073
#define IOCTL_AFD_SIZE_DISCONNECT_OPTIONS     _AFD_CONTROL_CODE(AFD_SIZE_DISCONNECT_OPTIONS, METHOD_NEITHER) // 0x12077
#define IOCTL_AFD_GET_INFORMATION             _AFD_CONTROL_CODE(AFD_GET_INFORMATION, METHOD_NEITHER) // 0x1207B
#define IOCTL_AFD_TRANSMIT_FILE               _AFD_CONTROL_CODE(AFD_TRANSMIT_FILE, METHOD_NEITHER) // 0x1207F
#define IOCTL_AFD_SUPER_ACCEPT                _AFD_CONTROL_CODE(AFD_SUPER_ACCEPT, METHOD_NEITHER) // 0x12083
#define IOCTL_AFD_EVENT_SELECT                _AFD_CONTROL_CODE(AFD_EVENT_SELECT, METHOD_NEITHER) // 0x12087
#define IOCTL_AFD_ENUM_NETWORK_EVENTS         _AFD_CONTROL_CODE(AFD_ENUM_NETWORK_EVENTS, METHOD_NEITHER) // 0x1208B
#define IOCTL_AFD_DEFER_ACCEPT                _AFD_CONTROL_CODE(AFD_DEFER_ACCEPT, METHOD_BUFFERED) // 0x1208C
#define IOCTL_AFD_WAIT_FOR_LISTEN_LIFO        _AFD_CONTROL_CODE(AFD_WAIT_FOR_LISTEN_LIFO, METHOD_BUFFERED) // 0x12090
#define IOCTL_AFD_SET_QOS                     _AFD_CONTROL_CODE(AFD_SET_QOS, METHOD_BUFFERED) // 0x12094
#define IOCTL_AFD_GET_QOS                     _AFD_CONTROL_CODE(AFD_GET_QOS, METHOD_BUFFERED) // 0x12098
#define IOCTL_AFD_NO_OPERATION                _AFD_CONTROL_CODE(AFD_NO_OPERATION, METHOD_NEITHER) // 0x1209F
#define IOCTL_AFD_VALIDATE_GROUP              _AFD_CONTROL_CODE(AFD_VALIDATE_GROUP, METHOD_BUFFERED) // 0x120A0
#define IOCTL_AFD_GET_UNACCEPTED_CONNECT_DATA _AFD_CONTROL_CODE(AFD_GET_UNACCEPTED_CONNECT_DATA, METHOD_NEITHER) // 0x120A7
#define IOCTL_AFD_ROUTING_INTERFACE_QUERY     _AFD_CONTROL_CODE(AFD_ROUTING_INTERFACE_QUERY, METHOD_NEITHER) // 0x120AB
#define IOCTL_AFD_ROUTING_INTERFACE_CHANGE    _AFD_CONTROL_CODE(AFD_ROUTING_INTERFACE_CHANGE, METHOD_BUFFERED) // 0x120AC
#define IOCTL_AFD_ADDRESS_LIST_QUERY          _AFD_CONTROL_CODE(AFD_ADDRESS_LIST_QUERY, METHOD_NEITHER) // 0x120B3
#define IOCTL_AFD_ADDRESS_LIST_CHANGE         _AFD_CONTROL_CODE(AFD_ADDRESS_LIST_CHANGE, METHOD_BUFFERED) // 0x120B4
#define IOCTL_AFD_JOIN_LEAF                   _AFD_CONTROL_CODE(AFD_JOIN_LEAF, METHOD_NEITHER) // 0x120BB
#define IOCTL_AFD_TRANSPORT_IOCTL             _AFD_CONTROL_CODE(AFD_TRANSPORT_IOCTL, METHOD_NEITHER) // 0x120BF
#define IOCTL_AFD_TRANSMIT_PACKETS            _AFD_CONTROL_CODE(AFD_TRANSMIT_PACKETS, METHOD_NEITHER) // 0x120C3
#define IOCTL_AFD_SUPER_CONNECT               _AFD_CONTROL_CODE(AFD_SUPER_CONNECT, METHOD_NEITHER) // 0x120C7
#define IOCTL_AFD_SUPER_DISCONNECT            _AFD_CONTROL_CODE(AFD_SUPER_DISCONNECT, METHOD_NEITHER) // 0x120CB
#define IOCTL_AFD_RECEIVE_MESSAGE             _AFD_CONTROL_CODE(AFD_RECEIVE_MESSAGE, METHOD_NEITHER) // 0x120CF
#define IOCTL_AFD_SEND_MESSAGE                _AFD_CONTROL_CODE(AFD_SEND_MESSAGE, METHOD_NEITHER) // 0x120D3 // rev // since VISTA
#define IOCTL_AFD_SWITCH_CEMENT_SAN           _AFD_CONTROL_CODE(AFD_SWITCH_CEMENT_SAN, METHOD_NEITHER) // 0x120D7
#define IOCTL_AFD_SWITCH_SET_EVENTS           _AFD_CONTROL_CODE(AFD_SWITCH_SET_EVENTS, METHOD_NEITHER) // 0x120DB
#define IOCTL_AFD_SWITCH_RESET_EVENTS         _AFD_CONTROL_CODE(AFD_SWITCH_RESET_EVENTS, METHOD_NEITHER) // 0x120DF
#define IOCTL_AFD_SWITCH_CONNECT_IND          _AFD_CONTROL_CODE(AFD_SWITCH_CONNECT_IND, METHOD_OUT_DIRECT) // 0x120E2
#define IOCTL_AFD_SWITCH_CMPL_ACCEPT          _AFD_CONTROL_CODE(AFD_SWITCH_CMPL_ACCEPT, METHOD_NEITHER) // 0x120E7
#define IOCTL_AFD_SWITCH_CMPL_REQUEST         _AFD_CONTROL_CODE(AFD_SWITCH_CMPL_REQUEST, METHOD_NEITHER) // 0x120EB
#define IOCTL_AFD_SWITCH_CMPL_IO              _AFD_CONTROL_CODE(AFD_SWITCH_CMPL_IO, METHOD_NEITHER) // 0x120EF
#define IOCTL_AFD_SWITCH_REFRESH_ENDP         _AFD_CONTROL_CODE(AFD_SWITCH_REFRESH_ENDP, METHOD_NEITHER) // 0x120F3
#define IOCTL_AFD_SWITCH_GET_PHYSICAL_ADDR    _AFD_CONTROL_CODE(AFD_SWITCH_GET_PHYSICAL_ADDR, METHOD_NEITHER) // 0x120F7
#define IOCTL_AFD_SWITCH_ACQUIRE_CTX          _AFD_CONTROL_CODE(AFD_SWITCH_ACQUIRE_CTX, METHOD_NEITHER) // 0x120FB
#define IOCTL_AFD_SWITCH_TRANSFER_CTX         _AFD_CONTROL_CODE(AFD_SWITCH_TRANSFER_CTX, METHOD_NEITHER) // 0x120FF
#define IOCTL_AFD_SWITCH_GET_SERVICE_PID      _AFD_CONTROL_CODE(AFD_SWITCH_GET_SERVICE_PID, METHOD_NEITHER) // 0x12103
#define IOCTL_AFD_SWITCH_SET_SERVICE_PROCESS  _AFD_CONTROL_CODE(AFD_SWITCH_SET_SERVICE_PROCESS, METHOD_NEITHER) // 0x12107
#define IOCTL_AFD_SWITCH_PROVIDER_CHANGE      _AFD_CONTROL_CODE(AFD_SWITCH_PROVIDER_CHANGE, METHOD_NEITHER) // 0x1210B
#define IOCTL_AFD_SWITCH_ADDRLIST_CHANGE      _AFD_CONTROL_CODE(AFD_SWITCH_ADDRLIST_CHANGE, METHOD_BUFFERED) // 0x1210C
#define IOCTL_AFD_UNBIND                      _AFD_CONTROL_CODE(AFD_UNBIND, METHOD_NEITHER) // 0x12113 // rev
#define IOCTL_AFD_SQM                         _AFD_CONTROL_CODE(AFD_SQM, METHOD_NEITHER) // 0x12117 // rev // since WIN7
#define IOCTL_AFD_RIO                         _AFD_CONTROL_CODE(AFD_RIO, METHOD_NEITHER) // 0x1211B // rev // since WIN8
#define IOCTL_AFD_TRANSFER_BEGIN              _AFD_CONTROL_CODE(AFD_TRANSFER_BEGIN, METHOD_NEITHER) // 0x1211F // rev // since TH1
#define IOCTL_AFD_TRANSFER_END                _AFD_CONTROL_CODE(AFD_TRANSFER_END, METHOD_NEITHER) // 0x12123 // rev
#define IOCTL_AFD_NOTIFY                      _AFD_CONTROL_CODE(AFD_NOTIFY, METHOD_NEITHER) // 0x12127 // rev // since 22H2

#include <pshpack1.h>

// rev - a union for TLI/TDI socket addresses
typedef union _AFD_ADDRESS
{
    SOCKADDR_STORAGE TliAddress;
    TDI_ADDRESS_INFO TdiAddress;

    struct
    {
        //
        // TDI_ADDRESS_INFO includes an embedded socket address that starts at AddressType (which corresponds to sa_family).
        //
        // ---------------- | ------------------------- |
        //                  | ULONG ActivityCount       |
        //                  | ------------------------- | --------------------- |
        //                  |                           | ULONG TAAddressCount  |
        //                  |                           | --------------------- | ---------------------------- |
        //                  |                           |                       | USHORT AddressLength         |
        // TDI_ADDRESS_INFO | TRANSPORT_ADDRESS Address |                       | ---------------------------- | ---------- | ---------------- |
        //                  |                           | TA_ADDRESS Address[1] | USHORT AddressType           |            | USHORT sa_family |
        //                  |                           |                       | ---------------------------- |  SOCKADDR  | ---------------- |
        //                  |                           |                       | UCHAR Address[AddressLength] | (embedded) |       ...        |
        //                  |                           |                       |             ...              |            |                  |
        // ---------------- | ------------------------- | --------------------- | ---------------------------- | ---------- | ---------------- |
        //

        UCHAR Padding[10]; // RTL_SIZEOF_THROUGH_FIELD(TDI_ADDRESS_INFO, Address.Address[0].AddressLength)
        SOCKADDR_STORAGE EmbeddedAddress;
    } TdiAddressUnpacked;
} AFD_ADDRESS, *PAFD_ADDRESS;

#include <poppack.h>

// private // Bind share access
#define AFD_NORMALADDRUSE    0
#define AFD_REUSEADDRESS     1
#define AFD_WILDCARDADDRESS  2
#define AFD_EXCLUSIVEADDRUSE 3

// private
typedef struct _AFD_BIND_INFO
{
    ULONG ShareAccess;
    TRANSPORT_ADDRESS Address;
} AFD_BIND_INFO, *PAFD_BIND_INFO;

// private
typedef struct _AFD_BIND_INFO_TL
{
    ULONG ShareAccess;
    SOCKADDR Address;
} AFD_BIND_INFO_TL, *PAFD_BIND_INFO_TL;

// private
typedef struct _AFD_CONNECT_JOIN_INFO
{
    BOOLEAN SanActive;
    HANDLE RootEndpoint;
    HANDLE ConnectEndpoint;
    TRANSPORT_ADDRESS RemoteAddress;
} AFD_CONNECT_JOIN_INFO, *PAFD_CONNECT_JOIN_INFO;

// private
typedef struct _AFD_CONNECT_JOIN_INFO_TL
{
    BOOLEAN SanActive;
    HANDLE RootEndpoint;
    HANDLE ConnectEndpoint;
    SOCKADDR RemoteAddress;
} AFD_CONNECT_JOIN_INFO_TL, *PAFD_CONNECT_JOIN_INFO_TL;

// private
typedef struct _AFD_LISTEN_INFO
{
    BOOLEAN SanActive;
    ULONG MaximumConnectionQueue;
    BOOLEAN UseDelayedAcceptance;
} AFD_LISTEN_INFO, *PAFD_LISTEN_INFO;

// private
typedef struct _AFD_LISTEN_RESPONSE_INFO
{
    LONG Sequence;
    TRANSPORT_ADDRESS RemoteAddress;
} AFD_LISTEN_RESPONSE_INFO, *PAFD_LISTEN_RESPONSE_INFO;

// private
typedef struct _AFD_LISTEN_RESPONSE_INFO_TL
{
    LONG Sequence;
    SOCKADDR RemoteAddress;
} AFD_LISTEN_RESPONSE_INFO_TL, *PAFD_LISTEN_RESPONSE_INFO_TL;

// private
typedef struct _AFD_ACCEPT_INFO
{
    BOOLEAN SanActive;
    LONG Sequence;
    HANDLE AcceptHandle;
} AFD_ACCEPT_INFO, *PAFD_ACCEPT_INFO;

// private // AfdFlags
#define AFD_NO_FAST_IO 0x0001
#define AFD_OVERLAPPED 0x0002

// private
typedef struct _AFD_RECV_INFO
{
    _Field_size_(BufferCount) LPWSABUF BufferArray;
    ULONG BufferCount;
    ULONG AfdFlags;
    ULONG TdiFlags; // TDI_RECEIVE_*
} AFD_RECV_INFO, *PAFD_RECV_INFO;

// private
typedef struct _AFD_DATAGRAM_INFO
{
    _Field_size_(BufferCount) LPWSABUF BufferArray;
    ULONG BufferCount;
    ULONG AfdFlags;
    ULONG TdiFlags; // TDI_RECEIVE_*
    PVOID Address;
    PULONG AddressLength;
} AFD_DATAGRAM_INFO, *PAFD_DATAGRAM_INFO;

// private
typedef struct _AFD_SEND_INFO
{
    _Field_size_(BufferCount) LPWSABUF BufferArray;
    ULONG BufferCount;
    ULONG AfdFlags;
    ULONG TdiFlags; // TDI_RECEIVE_*
} AFD_SEND_INFO, *PAFD_SEND_INFO;

// private
typedef struct _AFD_SEND_DATAGRAM_INFO
{
    _Field_size_(BufferCount) LPWSABUF BufferArray;
    ULONG BufferCount;
    ULONG AfdFlags;
    ULONG TdiFlags; // TDI_RECEIVE_*
    TDI_REQUEST_SEND_DATAGRAM TdiRequest;
    TDI_CONNECTION_INFORMATION TdiConnInfo;
} AFD_SEND_DATAGRAM_INFO, *PAFD_SEND_DATAGRAM_INFO;

// private // Poll event bit numbers
#define AFD_POLL_RECEIVE_BIT             0
#define AFD_POLL_RECEIVE_EXPEDITED_BIT   1
#define AFD_POLL_SEND_BIT                2
#define AFD_POLL_DISCONNECT_BIT          3
#define AFD_POLL_ABORT_BIT               4
#define AFD_POLL_LOCAL_CLOSE_BIT         5
#define AFD_POLL_CONNECT_BIT             6
#define AFD_POLL_ACCEPT_BIT              7
#define AFD_POLL_CONNECT_FAIL_BIT        8
#define AFD_POLL_QOS_BIT                 9
#define AFD_POLL_GROUP_QOS_BIT           10
#define AFD_POLL_ROUTING_IF_CHANGE_BIT   11
#define AFD_POLL_ADDRESS_LIST_CHANGE_BIT 12
#define AFD_NUM_POLL_EVENTS              13

// private // Poll event flags
#define AFD_POLL_RECEIVE             (1 << AFD_POLL_RECEIVE_BIT) // 0x0001
#define AFD_POLL_RECEIVE_EXPEDITED   (1 << AFD_POLL_RECEIVE_EXPEDITED_BIT) // 0x0002
#define AFD_POLL_SEND                (1 << AFD_POLL_SEND_BIT) // 0x0004
#define AFD_POLL_DISCONNECT          (1 << AFD_POLL_DISCONNECT_BIT) // 0x0008
#define AFD_POLL_ABORT               (1 << AFD_POLL_ABORT_BIT) // 0x0010
#define AFD_POLL_LOCAL_CLOSE         (1 << AFD_POLL_LOCAL_CLOSE_BIT) // 0x0020
#define AFD_POLL_CONNECT             (1 << AFD_POLL_CONNECT_BIT) // 0x0040
#define AFD_POLL_ACCEPT              (1 << AFD_POLL_ACCEPT_BIT) // 0x0080
#define AFD_POLL_CONNECT_FAIL        (1 << AFD_POLL_CONNECT_FAIL_BIT) // 0x0100
#define AFD_POLL_QOS                 (1 << AFD_POLL_QOS_BIT) // 0x0200
#define AFD_POLL_GROUP_QOS           (1 << AFD_POLL_GROUP_QOS_BIT) // 0x0400
#define AFD_POLL_ROUTING_IF_CHANGE   (1 << AFD_POLL_ROUTING_IF_CHANGE_BIT) // 0x0800
#define AFD_POLL_ADDRESS_LIST_CHANGE (1 << AFD_POLL_ADDRESS_LIST_CHANGE_BIT) // 0x1000
#define AFD_POLL_ALL                 ((1 << AFD_NUM_POLL_EVENTS) - 1) // 0x1FFF
#define AFD_POLL_SANCOUNTS_UPDATED   0x80000000

// private
typedef struct _AFD_POLL_HANDLE_INFO
{
    HANDLE Handle;
    ULONG PollEvents; // AFD_POLL_*
    NTSTATUS Status;
} AFD_POLL_HANDLE_INFO, *PAFD_POLL_HANDLE_INFO;

// private
typedef struct _AFD_POLL_INFO
{
    LARGE_INTEGER Timeout;
    ULONG NumberOfHandles;
    BOOLEAN Unique;
    _Field_size_(NumberOfHandles) AFD_POLL_HANDLE_INFO Handles[ANYSIZE_ARRAY];
} AFD_POLL_INFO, *PAFD_POLL_INFO;

// private // Disconnect flags
#define AFD_PARTIAL_DISCONNECT_SEND    0x01
#define AFD_PARTIAL_DISCONNECT_RECEIVE 0x02
#define AFD_ABORTIVE_DISCONNECT        0x04
#define AFD_UNCONNECT_DATAGRAM         0x08

// private
typedef struct _AFD_PARTIAL_DISCONNECT_INFO
{
    ULONG DisconnectMode;
    LARGE_INTEGER Timeout;
} AFD_PARTIAL_DISCONNECT_INFO, *PAFD_PARTIAL_DISCONNECT_INFO;

// private
typedef struct _AFD_RECEIVE_INFORMATION
{
    ULONG BytesAvailable;
    ULONG ExpeditedBytesAvailable;
} AFD_RECEIVE_INFORMATION, *PAFD_RECEIVE_INFORMATION;

// private // Handle query flags
#define AFD_QUERY_ADDRESS_HANDLE    0x01
#define AFD_QUERY_CONNECTION_HANDLE 0x02

// private
typedef struct _AFD_HANDLE_INFO
{
    HANDLE TdiAddressHandle;
    HANDLE TdiConnectionHandle;
} AFD_HANDLE_INFO, *PAFD_HANDLE_INFO;

// private // InformationType
#define AFD_INLINE_MODE                1 // s: BOOLEAN
#define AFD_NONBLOCKING_MODE           2 // s: BOOLEAN
#define AFD_MAX_SEND_SIZE              3 // q: ULONG
#define AFD_SENDS_PENDING              4 // q: ULONG
#define AFD_MAX_PATH_SEND_SIZE         5 // q: ULONG
#define AFD_RECEIVE_WINDOW_SIZE        6 // q; s: ULONG
#define AFD_SEND_WINDOW_SIZE           7 // q; s: ULONG
#define AFD_CONNECT_TIME               8 // q: ULONG (in seconds)
#define AFD_CIRCULAR_QUEUEING          9 // s: BOOLEAN
#define AFD_GROUP_ID_AND_TYPE          10 // q: AFD_GROUP_INFO
#define AFD_REPORT_PORT_UNREACHABLE    11 // s: BOOLEAN
#define AFD_REPORT_NETWORK_UNREACHABLE 12 // s: BOOLEAN // rev
#define AFD_DELIVERY_STATUS            14 // q: SIO_DELIVERY_STATUS // rev
#define AFD_CANCEL_TL                  15 // s: void // rev

// private
typedef enum _AFD_GROUP_TYPE
{
    GroupTypeNeither = 0,
    GroupTypeUnconstrained = SG_UNCONSTRAINED_GROUP,
    GroupTypeConstrained = SG_CONSTRAINED_GROUP,
} AFD_GROUP_TYPE, *PAFD_GROUP_TYPE;

// private
typedef struct _AFD_GROUP_INFO
{
    GROUP GroupID;
    AFD_GROUP_TYPE GroupType;
} AFD_GROUP_INFO, *PAFD_GROUP_INFO;

// private
typedef struct _SIO_DELIVERY_STATUS
{
    BOOLEAN DeliveryAvailable;
    ULONG PendedReceiveRequests;
} SIO_DELIVERY_STATUS, *PSIO_DELIVERY_STATUS;

// private
typedef struct _AFD_INFORMATION
{
    ULONG InformationType;
    union
    {
        BOOLEAN Boolean;
        ULONG Ulong;
        LARGE_INTEGER LargeInteger;
        AFD_GROUP_INFO GroupInfo; // rev
        SIO_DELIVERY_STATUS DeliveryStatus; // rev
    } Information;
} AFD_INFORMATION, *PAFD_INFORMATION;

// private
typedef enum _SOCKET_STATE
{
  SocketStateInitializing = -1,
  SocketStateOpen = 0,
  SocketStateBound = 1,
  SocketStateBoundSpecific = 2,
  SocketStateConnected = 3,
  SocketStateClosing = 4,
} SOCKET_STATE, *PSOCKET_STATE;

// private
typedef struct _SOCK_SHARED_INFO
{
    SOCKET_STATE State;
    LONG AddressFamily; // AF_*
    LONG SocketType; // SOCK_*
    LONG Protocol; // IPPROTO_*, BTHPROTO_*, HV_PROTOCOL_*, etc.
    LONG LocalAddressLength;
    LONG RemoteAddressLength;
    LINGER LingerInfo;
    ULONG SendTimeout; // in milliseconds
    ULONG ReceiveTimeout; // in milliseconds
    ULONG ReceiveBufferSize;
    ULONG SendBufferSize;
    union
    {
        struct
        {
            UCHAR Listening : 1;
            UCHAR Broadcast : 1;
            UCHAR Debug : 1;
            UCHAR OobInline : 1;
            UCHAR ReuseAddresses : 1;
            UCHAR ExclusiveAddressUse : 1;
            UCHAR NonBlocking : 1;
            UCHAR DontUseWildcard : 1;
            UCHAR ReceiveShutdown : 1;
            UCHAR SendShutdown : 1;
            UCHAR ConditionalAccept : 1;
            UCHAR IsSANSocket : 1;
            UCHAR fIsTLI : 1;
            UCHAR Rio : 1;
            UCHAR ReceiveBufferSizeSet : 1;
            UCHAR SendBufferSizeSet : 1;
        };
        WORD Flags;
    };
    ULONG CreationFlags; // WSA_FLAG_*
    ULONG CatalogEntryId;
    ULONG ServiceFlags1; // XP1_*
    ULONG ProviderFlags; // PFL_*
    GROUP GroupID;
    AFD_GROUP_TYPE GroupType;
    LONG GroupPriority;
    LONG LastError;
    union
    {
        HWND AsyncSelecthWnd;
        ULONGLONG AsyncSelectWnd64;
    };
    ULONG AsyncSelectSerialNumber;
    ULONG AsyncSelectwMsg;
    LONG AsyncSelectlEvent;
    LONG DisabledAsyncSelectEvents;
    GUID ProviderId;
} SOCK_SHARED_INFO, *PSOCK_SHARED_INFO;

// private
typedef struct _AFD_UNACCEPTED_CONNECT_DATA_INFO
{
    LONG Sequence;
    ULONG ConnectDataLength;
    BOOLEAN LengthOnly;
} AFD_UNACCEPTED_CONNECT_DATA_INFO, *PAFD_UNACCEPTED_CONNECT_DATA_INFO;

// private // Transmit flags
#define AFD_TF_DISCONNECT         0x01
#define AFD_TF_REUSE_SOCKET       0x02
#define AFD_TF_WRITE_BEHIND       0x04
#define AFD_TF_USE_DEFAULT_WORKER 0x00
#define AFD_TF_USE_SYSTEM_THREAD  0x10
#define AFD_TF_USE_KERNEL_APC     0x20
#define AFD_TF_WORKER_KIND_MASK   0x30

// private
typedef struct _AFD_TRANSMIT_FILE_INFO
{
    LARGE_INTEGER Offset;
    LARGE_INTEGER WriteLength;
    ULONG SendPacketLength;
    HANDLE FileHandle;
    PVOID Head;
    ULONG HeadLength;
    PVOID Tail;
    ULONG TailLength;
    ULONG Flags; // AFD_TF_*
} AFD_TRANSMIT_FILE_INFO, *PAFD_TRANSMIT_FILE_INFO;

// private
typedef struct _AFD_SUPER_ACCEPT_INFO
{
    BOOLEAN SanActive;
    BOOLEAN FixAddressAlignment;
    HANDLE AcceptHandle;
    ULONG ReceiveDataLength;
    ULONG LocalAddressLength;
    ULONG RemoteAddressLength;
} AFD_SUPER_ACCEPT_INFO, *PAFD_SUPER_ACCEPT_INFO;

// private
typedef struct _AFD_EVENT_SELECT_INFO
{
    HANDLE Event;
    ULONG PollEvents; // AFD_POLL_*
} AFD_EVENT_SELECT_INFO, *PAFD_EVENT_SELECT_INFO;

// private
typedef struct _AFD_ENUM_NETWORK_EVENTS_INFO
{
    ULONG PollEvents; // AFD_POLL_*
    NTSTATUS EventStatus[AFD_NUM_POLL_EVENTS];
} AFD_ENUM_NETWORK_EVENTS_INFO, *PAFD_ENUM_NETWORK_EVENTS_INFO;

// private
typedef struct _AFD_DEFER_ACCEPT_INFO
{
    LONG Sequence;
    BOOLEAN Reject;
} AFD_DEFER_ACCEPT_INFO, *PAFD_DEFER_ACCEPT_INFO;

// private
typedef struct _AFD_QOS_INFO
{
    QOS Qos;
    BOOLEAN GroupQos;
} AFD_QOS_INFO, *PAFD_QOS_INFO;

// private
typedef struct _AFD_VALIDATE_GROUP_INFO
{
    GROUP GroupID;
    TRANSPORT_ADDRESS RemoteAddress;
} AFD_VALIDATE_GROUP_INFO, *PAFD_VALIDATE_GROUP_INFO;

// private
typedef struct _AFD_TRANSPORT_IOCTL_INFO
{
    HANDLE Handle;
    _Field_size_bytes_(InputBufferLength) PVOID InputBuffer;
    ULONG InputBufferLength;
    ULONG IoControlCode;
    ULONG AfdFlags;
    ULONG PollEvent;
} AFD_TRANSPORT_IOCTL_INFO, *PAFD_TRANSPORT_IOCTL_INFO;

// private
typedef enum TL_IO_CONTROL_TYPE
{
    TlEndpointIoControlType = 0,
    TlSetSockOptIoControlType = 1,
    TlGetSockOptIoControlType = 2,
    TlSocketIoControlType = 3,
} TL_IO_CONTROL_TYPE, *PTL_IO_CONTROL_TYPE;

// private
typedef struct _AFD_TL_IO_CONTROL_INFO
{
    TL_IO_CONTROL_TYPE Type;
    ULONG Level; // SOL_* or IPPROTO_*
    ULONG IoControlCode; // SIO_*, SO_*, IP_*, IPV6_*, TCP_*, UDP_*, etc. (depending on type and level)
    BOOLEAN EndpointIoctl;
    _Field_size_bytes_(InputBufferLength) PVOID InputBuffer;
    SIZE_T InputBufferLength;
} AFD_TL_IO_CONTROL_INFO, *PAFD_TL_IO_CONTROL_INFO;

// private
typedef struct _AFD_TPACKETS_INFO
{
    _Field_size_(ElementCount) PTRANSMIT_PACKETS_ELEMENT ElementArray;
    ULONG ElementCount;
    ULONG SendSize;
    ULONG Flags;
} AFD_TPACKETS_INFO, *PAFD_TPACKETS_INFO;

// private
typedef struct _AFD_SUPER_CONNECT_INFO
{
    BOOLEAN SanActive;
    TRANSPORT_ADDRESS RemoteAddress;
} AFD_SUPER_CONNECT_INFO, *PAFD_SUPER_CONNECT_INFO;

// rev
typedef struct _AFD_SUPER_CONNECT_INFO_TL
{
    BOOLEAN SanActive;
    SOCKADDR RemoteAddress;
} AFD_SUPER_CONNECT_INFO_TL, *PAFD_SUPER_CONNECT_INFO_TL;

// private
typedef struct _AFD_SUPER_DISCONNECT_INFO
{
    ULONG Flags; // same as partial disconnect
} AFD_SUPER_DISCONNECT_INFO, *PAFD_SUPER_DISCONNECT_INFO;

// private
typedef struct _AFD_MESSAGE_INFO
{
    AFD_DATAGRAM_INFO dgi;
    _Field_size_bytes_(ControlLength) PVOID ControlBuffer;
    PULONG ControlLength;
    PULONG MsgFlags;
} AFD_MESSAGE_INFO, *PAFD_MESSAGE_INFO;

// private
typedef struct _AFD_SWITCH_CONTEXT
{
    LONG EventsActive;
    LONG RcvCount;
    LONG ExpCount;
    LONG SndCount;
    BOOLEAN SelectFlag;
} AFD_SWITCH_CONTEXT, *PAFD_SWITCH_CONTEXT;

// private
typedef struct _AFD_SWITCH_CONTEXT_INFO
{
    HANDLE SocketHandle;
    PAFD_SWITCH_CONTEXT SwitchContext;
} AFD_SWITCH_CONTEXT_INFO, *PAFD_SWITCH_CONTEXT_INFO;

// private
typedef struct _AFD_SWITCH_EVENT_INFO
{
    HANDLE SocketHandle;
    PAFD_SWITCH_CONTEXT SwitchContext;
    ULONG EventBit; // AFD_POLL_*_BIT
    NTSTATUS Status;
} AFD_SWITCH_EVENT_INFO, *PAFD_SWITCH_EVENT_INFO;

// private
typedef struct _AFD_SWITCH_CONNECT_INFO
{
    HANDLE ListenHandle;
    PAFD_SWITCH_CONTEXT SwitchContext;
    TRANSPORT_ADDRESS RemoteAddress;
} AFD_SWITCH_CONNECT_INFO, *PAFD_SWITCH_CONNECT_INFO;

// private
typedef struct _AFD_SWITCH_ACCEPT_INFO
{
    HANDLE AcceptHandle;
    ULONG ReceiveLength;
} AFD_SWITCH_ACCEPT_INFO, *PAFD_SWITCH_ACCEPT_INFO;

// private
typedef struct _AFD_SWITCH_REQUEST_INFO
{
    HANDLE SocketHandle;
    PAFD_SWITCH_CONTEXT SwitchContext;
    PVOID RequestContext;
    NTSTATUS RequestStatus;
    ULONG DataOffset;
} AFD_SWITCH_REQUEST_INFO, *PAFD_SWITCH_REQUEST_INFO;

// private
typedef struct _AFD_SWITCH_ACQUIRE_CTX_INFO
{
    HANDLE SocketHandle;
    PAFD_SWITCH_CONTEXT SwitchContext;
    PVOID SocketCtxBuf;
    ULONG SocketCtxBufSize;
} AFD_SWITCH_ACQUIRE_CTX_INFO, *PAFD_SWITCH_ACQUIRE_CTX_INFO;

// private
typedef struct _AFD_SWITCH_TRANSFER_CTX_INFO
{
    HANDLE SocketHandle;
    PAFD_SWITCH_CONTEXT SwitchContext;
    PVOID RequestContext;
    PVOID SocketCtxBuf;
    ULONG SocketCtxBufSize;
    _Field_size_(RcvBufferCount) LPWSABUF RcvBufferArray;
    ULONG RcvBufferCount;
    NTSTATUS Status;
} AFD_SWITCH_TRANSFER_CTX_INFO, *PAFD_SWITCH_TRANSFER_CTX_INFO;

// private
typedef struct _AFD_UNBIND_INFO
{
    LONG AddressFamily; // AF_*
    LONG Protocol; // IPPROTO_*, BTHPROTO_*, HV_PROTOCOL_*, etc.
} AFD_UNBIND_INFO, *PAFD_UNBIND_INFO;

// rev // SQM Control Codes
#define AFD_SQM_CONTROL_CODE_STORE 1
#define AFD_SQM_CONTROL_CODE_SWEEP -1

// private
typedef struct _AFD_SQM_CONTROL
{
    ULONG Code; // AFD_SQM_CONTROL_CODE_*
} AFD_SQM_CONTROL, *PAFD_SQM_CONTROL;

// private
typedef struct _WINSOCK_SQM_SOCKTYPE_DESC
{
    ULONG StreamTCP : 1;
    ULONG StreamOther : 1;
    ULONG DatagramUDP : 1;
    ULONG DatagramUDPMulticast : 1;
    ULONG DatagramOther : 1;
    ULONG RawTCP : 1;
    ULONG RawUDP : 1;
    ULONG RawOther : 1;
    ULONG OtherSocketTypes : 1;
    ULONG IPv6Socket : 1;
    ULONG ProviderLoadedWSD : 1;
    ULONG ProviderLoadedLSP : 1;
    ULONG ProviderLoadedMultiLSP : 1;
    ULONG NonMswsockBSP : 1;
} WINSOCK_SQM_SOCKTYPE_DESC, *PWINSOCK_SQM_SOCKTYPE_DESC;

// private
typedef struct _WINSOCK_SQM_SOCKTYPE_COUNTS
{
    ULONG MaxStreamSocketsConn;
    ULONG MaxStreamSocketsListen;
    ULONG MaxDatagramSockets;
    ULONG MaxRawSockets;
    ULONG MaxOtherSockets;
} WINSOCK_SQM_SOCKTYPE_COUNTS, *PWINSOCK_SQM_SOCKTYPE_COUNTS;

// private
typedef struct _WINSOCK_SQM_NONCORE_FUNC
{
    ULONG Select : 1;
    ULONG WSAPoll : 1;
    ULONG StreamWSAEventSelect : 1;
    ULONG StreamWSAAsyncSelect : 1;
    ULONG TransmitFile : 1;
    ULONG StreamTransmitPackets : 1;
    ULONG DisconnectEx : 1;
    ULONG SocketReuse : 1;
    ULONG StreamDuplicateSocket : 1;
    ULONG ReadWriteFile : 1;
    ULONG OOBSendRecv : 1;
    ULONG StreamSoSndTimeO : 1;
    ULONG StreamSoRcvTimeO : 1;
    ULONG MsgWaitAll : 1;
    ULONG StreamNonCoreSOLOptions : 1;
    ULONG StreamNonCoreTCPOptions : 1;
    ULONG StreamNonCoreIPOptions : 1;
    ULONG StreamNonCoreOtherOptions : 1;
    ULONG StreamNonCoreIOCTLs : 1;
    ULONG StreamNonCoreWSASocket : 1;
    ULONG NonCoreRecv : 1;
    ULONG NonCoreSend : 1;
    ULONG StreamNonCoreWSAIoctl : 1;
    ULONG NonCoreWSAAccept : 1;
    ULONG StreamNonCoreWSAConnect : 1;
    ULONG StreamWSAJoinLeaf : 1;
    ULONG StreamWSAGetQoSByName : 1;
    ULONG WSASendDisconnect : 1;
    ULONG WSARecvDisconnect : 1;
} WINSOCK_SQM_NONCORE_FUNC, *PWINSOCK_SQM_NONCORE_FUNC;

// private
typedef struct _WINSOCK_SQM_DEPRECATION_LIST
{
    ULONG bEnumProtocols : 1;
    ULONG bGetNameByType : 1;
    ULONG bGetService : 1;
    ULONG bGetTypeByName : 1;
    ULONG bSetService : 1;
    ULONG bWSACancelBlockingCall : 1;
    ULONG bWSAIsBlocking : 1;
    ULONG bWSASetBlockingHook : 1;
    ULONG bWSAUnhookBlockingHook : 1;
    ULONG bGetAddressByName : 1;
    ULONG bGetHostByAddr : 1;
    ULONG bGetHostByName : 1;
    ULONG bGetHostName : 1;
    ULONG bGetProtoByName : 1;
    ULONG bGetProtoByNumber : 1;
    ULONG bGetServByName : 1;
    ULONG bGetServByPort : 1;
    ULONG bInetAddr : 1;
    ULONG bInetNtoa : 1;
    ULONG bWSAAsyncGetHostByAddr : 1;
    ULONG bWSAAsyncGetHostByName : 1;
    ULONG bWSAAsyncGetProtoByName : 1;
    ULONG bWSAAsyncGetProtoByNumber : 1;
    ULONG bWSAAsyncGetServByName : 1;
    ULONG bWSAAsyncGetServByPort : 1;
    ULONG bWSACancelAsyncRequest : 1;
    ULONG bWSARecvEx : 1;
} WINSOCK_SQM_DEPRECATION_LIST, *PWINSOCK_SQM_DEPRECATION_LIST;

// private
typedef struct _WINSOCK_SQM_SOCK_OPTIONS
{
    ULONG SoSndBuf : 1;
    ULONG SoRcvBuf : 1;
    ULONG SoSndBuf0 : 1;
    ULONG SoRcvBuf0 : 1;
    ULONG SoKeepAliveOn : 1;
    ULONG SoReuseAddrOn : 1;
    ULONG SoExclusiveAddrUseOn : 1;
    ULONG SoLingerOn : 1;
    ULONG SoLingerOn0 : 1;
    ULONG SoRandomizePortOn : 1;
    ULONG SoPortScaleOn : 1;
    ULONG SoCondAcceptOn : 1;
    ULONG SoOobInlineOn : 1;
    ULONG SoOther : 1;
    ULONG TCPNoDelayOn : 1;
    ULONG TCPOther : 1;
    ULONG IPV6V6OnlyOff : 1;
    ULONG IPV6ProtectLevel : 1;
    ULONG IPOther : 1;
    ULONG LevelOther : 1;
    ULONG ISBQuery : 1;
    ULONG ISBNotify : 1;
    ULONG AddressQuery : 1;
    ULONG AddressNotify : 1;
    ULONG RouteQuery : 1;
    ULONG RouteNotify : 1;
    ULONG FionbioOn : 1;
    ULONG Fionread : 1;
    ULONG Siocatmark : 1;
    ULONG Siokeepalivevals : 1;
    ULONG Siosetcompatmode : 1;
    ULONG Sioportreserve : 1;
} WINSOCK_SQM_SOCK_OPTIONS, *PWINSOCK_SQM_SOCK_OPTIONS;

// private
typedef struct _WINSOCK_SQM_MISC_BEHAVIOR
{
    ULONG SendEverPended : 1;
    ULONG RecvEverPended : 1;
    ULONG ShutdownSend : 1;
    ULONG ShutdownRecv : 1;
    ULONG WSASocketProtocol : 1;
} WINSOCK_SQM_MISC_BEHAVIOR, *PWINSOCK_SQM_MISC_BEHAVIOR;

// private
typedef struct _AFD_SQM_INFO
{
    AFD_SQM_CONTROL Control;
    WCHAR AppName[16];
    WCHAR LSPName[16];
    ULONG AppVersion;
    WINSOCK_SQM_SOCKTYPE_DESC SocketTypeDescriptor;
    WINSOCK_SQM_SOCKTYPE_COUNTS SocketTypeCounts;
    WINSOCK_SQM_NONCORE_FUNC NonCoreFunctions;
    WINSOCK_SQM_DEPRECATION_LIST DeprecationList;
    WINSOCK_SQM_SOCK_OPTIONS SocketOptions;
    WINSOCK_SQM_MISC_BEHAVIOR MiscBehavior;
} AFD_SQM_INFO, *PAFD_SQM_INFO;

// private
typedef enum _AFD_RIO_COMMAND
{
    AfdRioCommandIdCreateCq = 0,         // in: AFD_RIO_COMMAND_CREATE_CQ; out: AFD_RIO_COMMAND_CREATE_CQ_RESULT
    AfdRioCommandIdDestroyCq = 1,        // in: AFD_RIO_COMMAND_DESTROY_CQ
    AfdRioCommandIdNotifyCq = 2,         // in: AFD_RIO_COMMAND_NOTIFY_CQ
    AfdRioCommandIdCreateRqPair = 3,     // in: AFD_RIO_COMMAND_CREATE_RQ_PAIR
    AfdRioCommandIdRegisterBuffer = 4,   // in: AFD_RIO_COMMAND_REGISTER_BUFFER; out: AFD_RIO_COMMAND_REGISTER_BUFFER_RESULT
    AfdRioCommandIdDeregisterBuffer = 5, // in: AFD_RIO_COMMAND_DEREGISTER_BUFFER
    AfdRioCommandIdPokeSend = 6,         // in: AFD_RIO_COMMAND_POKE_SEND
    AfdRioCommandIdPokeReceive = 7,      // in: AFD_RIO_COMMAND_POKE_RECEIVE
    AfdRioCommandIdResizeCq = 8,         // in: AFD_RIO_COMMAND_RESIZE_CQ
    AfdRioCommandIdResizeRqPair = 9,     // in: AFD_RIO_COMMAND_RESIZE_RQ_PAIR
    AfdRioCommandIdMaximum = 10,
} AFD_RIO_COMMAND, *PAFD_RIO_COMMAND;

// private
typedef struct _AFD_RIO_COMMAND_HEADER
{
    AFD_RIO_COMMAND Command;
} AFD_RIO_COMMAND_HEADER, *PAFD_RIO_COMMAND_HEADER;

// private
typedef enum _AFD_RIO_NOTIFICATION_COMPLETION_TYPE
{
    AfdRioNoCompletion = 0,
    AfdRioEventCompletion = 1,
    AfdRioIocpCompletion = 2,
} AFD_RIO_NOTIFICATION_COMPLETION_TYPE, *PAFD_RIO_NOTIFICATION_COMPLETION_TYPE;

// private
typedef struct _AFD_RIO_COMMAND_CREATE_CQ
{
    AFD_RIO_COMMAND_HEADER Header;
    ULONG QSize;
    AFD_RIO_NOTIFICATION_COMPLETION_TYPE NotificationType;
    ULONGLONG NotificationHandle;
    ULONGLONG NotificationContext;
    ULONGLONG NotificationContext2;
    ULONG BufferSize;
    ULONGLONG Buffer;
} AFD_RIO_COMMAND_CREATE_CQ, *PAFD_RIO_COMMAND_CREATE_CQ;

// private
typedef struct _AFD_RIO_COMMAND_CREATE_CQ_RESULT
{
    ULONG CqId;
} AFD_RIO_COMMAND_CREATE_CQ_RESULT, *PAFD_RIO_COMMAND_CREATE_CQ_RESULT;

// private
typedef struct _AFD_RIO_COMMAND_DESTROY_CQ
{
    AFD_RIO_COMMAND_HEADER Header;
    ULONG CqId;
} AFD_RIO_COMMAND_DESTROY_CQ, *PAFD_RIO_COMMAND_DESTROY_CQ;

// private
typedef struct _AFD_RIO_COMMAND_NOTIFY_CQ
{
    AFD_RIO_COMMAND_HEADER Header;
    ULONG Index;
} AFD_RIO_COMMAND_NOTIFY_CQ, *PAFD_RIO_COMMAND_NOTIFY_CQ;

// private
typedef struct _AFD_RIO_COMMAND_CREATE_RQ_PAIR
{
    AFD_RIO_COMMAND_HEADER Header;
    ULONG SendCompletionQueue;
    ULONG ReceiveCompletionQueue;
    ULONG SendQueueEntryCount;
    ULONG SendQueueBufferSize;
    ULONGLONG SendQueueBuffer;
    ULONG ReceiveQueueEntryCount;
    ULONG ReceiveQueueBufferSize;
    ULONGLONG ReceiveQueueBuffer;
    ULONGLONG EndpointHandle;
    ULONGLONG SocketContext;
} AFD_RIO_COMMAND_CREATE_RQ_PAIR, *PAFD_RIO_COMMAND_CREATE_RQ_PAIR;

// private
typedef struct _AFD_RIO_COMMAND_REGISTER_BUFFER
{
    AFD_RIO_COMMAND_HEADER Header;
    ULONGLONG Buffer;
    ULONG BufferLength;
} AFD_RIO_COMMAND_REGISTER_BUFFER, *PAFD_RIO_COMMAND_REGISTER_BUFFER;

// private
typedef struct _AFD_RIO_COMMAND_REGISTER_BUFFER_RESULT
{
    ULONG BufferId;
} AFD_RIO_COMMAND_REGISTER_BUFFER_RESULT, *PAFD_RIO_COMMAND_REGISTER_BUFFER_RESULT;

// private
typedef struct _AFD_RIO_COMMAND_DEREGISTER_BUFFER
{
    AFD_RIO_COMMAND_HEADER Header;
    ULONG BufferId;
} AFD_RIO_COMMAND_DEREGISTER_BUFFER, *PAFD_RIO_COMMAND_DEREGISTER_BUFFER;

// private
typedef struct _AFD_RIO_COMMAND_POKE_SEND
{
    AFD_RIO_COMMAND_HEADER Header;
} AFD_RIO_COMMAND_POKE_SEND, *PAFD_RIO_COMMAND_POKE_SEND;

// private
typedef struct _AFD_RIO_COMMAND_POKE_RECEIVE
{
    AFD_RIO_COMMAND_HEADER Header;
} AFD_RIO_COMMAND_POKE_RECEIVE, *PAFD_RIO_COMMAND_POKE_RECEIVE;

// private
typedef struct _AFD_RIO_COMMAND_RESIZE_CQ
{
    AFD_RIO_COMMAND_HEADER Header;
    ULONG CqId;
    ULONG QSize;
    ULONG BufferSize;
    ULONGLONG Buffer;
} AFD_RIO_COMMAND_RESIZE_CQ, *PAFD_RIO_COMMAND_RESIZE_CQ;

// private
typedef struct _AFD_RIO_COMMAND_RESIZE_RQ_PAIR
{
    AFD_RIO_COMMAND_HEADER Header;
    ULONG SendQueueEntryCount;
    ULONG SendQueueBufferSize;
    ULONGLONG SendQueueBuffer;
    ULONG ReceiveQueueEntryCount;
    ULONG ReceiveQueueBufferSize;
    ULONGLONG ReceiveQueueBuffer;
} AFD_RIO_COMMAND_RESIZE_RQ_PAIR, *PAFD_RIO_COMMAND_RESIZE_RQ_PAIR;

#endif
