/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2010-2013
 *     dmex    2012-2023
 *
 */

#ifndef _NET_TOOLS_H_
#define _NET_TOOLS_H_

#include <phdk.h>
#include <phapppub.h>
#include <phappresource.h>
#include <settings.h>
#include <workqueue.h>
#include <mapldr.h>
#include <phnet.h>

#include "resource.h"

#define PLUGIN_NAME L"ProcessHacker.NetworkTools"
#define SETTING_NAME_ADDRESS_HISTORY (PLUGIN_NAME L".AddressHistory")
#define SETTING_NAME_PING_WINDOW_POSITION (PLUGIN_NAME L".PingWindowPosition")
#define SETTING_NAME_PING_WINDOW_SIZE (PLUGIN_NAME L".PingWindowSize")
#define SETTING_NAME_PING_MINIMUM_SCALING (PLUGIN_NAME L".PingMinScaling")
#define SETTING_NAME_PING_TIMEOUT (PLUGIN_NAME L".PingTimeout")
#define SETTING_NAME_PING_SIZE (PLUGIN_NAME L".PingSize")
#define SETTING_NAME_TRACERT_WINDOW_POSITION (PLUGIN_NAME L".TracertWindowPosition")
#define SETTING_NAME_TRACERT_WINDOW_SIZE (PLUGIN_NAME L".TracertWindowSize")
#define SETTING_NAME_TRACERT_TREE_LIST_COLUMNS (PLUGIN_NAME L".TracertTreeColumns")
#define SETTING_NAME_TRACERT_TREE_LIST_SORT (PLUGIN_NAME L".TracertTreeSort")
#define SETTING_NAME_TRACERT_MAX_HOPS (PLUGIN_NAME L".TracertMaxHops")
#define SETTING_NAME_WHOIS_WINDOW_POSITION (PLUGIN_NAME L".WhoisWindowPosition")
#define SETTING_NAME_WHOIS_WINDOW_SIZE (PLUGIN_NAME L".WhoisWindowSize")
#define SETTING_NAME_WHOIS_IPV6_SUPPORT (PLUGIN_NAME L".WhoisProtocalSupport")
#define SETTING_NAME_EXTENDED_TCP_STATS (PLUGIN_NAME L".EnableExtendedTcpStats")
#define SETTING_NAME_GEOLITE_API_KEY (PLUGIN_NAME L".MaxMindApiKey")
#define SETTING_NAME_GEOLITE_DB_TYPE (PLUGIN_NAME L".MaxMindType")

extern PPH_PLUGIN PluginInstance;
extern BOOLEAN GeoDbInitialized;
extern BOOLEAN GeoDbDatabaseType;
extern PH_STRINGREF GeoDbCityFileName;
extern PH_STRINGREF GeoDbCountryFileName;
extern PPH_STRING SearchboxText;

// ICMP Packet Length: (msdn: IcmpSendEcho2/Icmp6SendEcho2)
// The buffer must be large enough to hold at least one ICMP_ECHO_REPLY or ICMPV6_ECHO_REPLY structure
//       + the number of bytes of data specified in the RequestSize parameter.
// This buffer should also be large enough to also hold 8 more bytes of data (the size of an ICMP error message)
//       + space for an IO_STATUS_BLOCK structure.
#define ICMP_BUFFER_SIZE(EchoReplyLength, BufferLength) \
    (ULONG)(((EchoReplyLength) + (BufferLength)) + 8 + sizeof(IO_STATUS_BLOCK) + MAX_OPT_SIZE)

#define BITS_IN_ONE_BYTE 8
#define NDIS_UNIT_OF_MEASUREMENT 100

// The ICMPV6_ECHO_REPLY struct doesn't have a field to access the reply data,
// so copy the struct and add an additional Data field.
typedef struct _ICMPV6_ECHO_REPLY2
{
    IPV6_ADDRESS_EX Address;
    ULONG Status;
    unsigned int RoundTripTime;
    BYTE Data[ANYSIZE_ARRAY]; // custom
} ICMPV6_ECHO_REPLY2, *PICMPV6_ECHO_REPLY2;

typedef enum _PH_NETWORK_ACTION
{
    NETWORK_ACTION_PING = 1,
    NETWORK_ACTION_TRACEROUTE,
    NETWORK_ACTION_WHOIS,
    NETWORK_ACTION_FINISH,
    NETWORK_ACTION_PATHPING,
    MAINMENU_ACTION_PING,
    MAINMENU_ACTION_TRACERT,
    MAINMENU_ACTION_WHOIS,
    MAINMENU_ACTION_GEOIP_UPDATE,
    MENU_ACTION_COPY,
} PH_NETWORK_ACTION;

#define UPDATE_MENUITEM 1005
#define NTM_RECEIVEDTRACE (WM_APP + 1)
#define NTM_RECEIVEDWHOIS (WM_APP + 2)
#define NTM_RECEIVEDFINISH (WM_APP + 3)
//#define WM_TRACERT_UPDATE (WM_APP + 4)
#define WM_TRACERT_HOSTNAME (WM_APP + 5)
#define WM_TRACERT_COUNTRY (WM_APP + 6)

#define PH_SHOWDIALOG (WM_APP + 10)
#define PH_SHOWINSTALL (WM_APP + 11)
#define PH_SHOWERROR (WM_APP + 12)

typedef struct _NETWORK_PING_CONTEXT
{
    HWND WindowHandle;
    HWND ParentWindowHandle;
    HWND StatusHandle;
    HWND PingGraphHandle;
    HFONT FontHandle;

    ULONG Timeout;
    ULONG MinPingScaling;
    volatile LONG HashFailCount;
    volatile LONG UnknownAddrCount;
    volatile LONG PingSentCount;
    volatile LONG PingRecvCount;
    volatile LONG PingLossCount;
    FLOAT CurrentPingMs;
    FLOAT PingMinMs;
    FLOAT PingMaxMs;

    PH_NETWORK_ACTION Action;
    PH_LAYOUT_MANAGER LayoutManager;
    PH_WORK_QUEUE PingWorkQueue;
    PH_GRAPH_STATE PingGraphState;
    PH_CIRCULAR_BUFFER_FLOAT PingHistory;
    PH_CALLBACK_REGISTRATION ProcessesUpdatedRegistration;

    PH_IP_ENDPOINT RemoteEndpoint;
    ULONG RemoteAddressStringLength;
    WCHAR RemoteAddressString[INET6_ADDRSTRLEN];
} NETWORK_PING_CONTEXT, *PNETWORK_PING_CONTEXT;

// ping.c

VOID ShowPingWindow(
    _In_ HWND ParentWindowHandle,
    _In_ PPH_NETWORK_ITEM NetworkItem
    );

VOID ShowPingWindowFromAddress(
    _In_ HWND ParentWindowHandle,
    _In_ PH_IP_ENDPOINT RemoteEndpoint
    );

// whois.c

typedef struct _NETWORK_WHOIS_CONTEXT
{
    HWND WindowHandle;
    HWND ParentWindowHandle;
    HWND RichEditHandle;
    HFONT FontHandle;

    BOOLEAN Ipv6Support;
    PH_NETWORK_ACTION Action;
    PH_LAYOUT_MANAGER LayoutManager;

    PH_IP_ENDPOINT RemoteEndpoint;
    ULONG RemoteAddressStringLength;
    WCHAR RemoteAddressString[INET6_ADDRSTRLEN];
} NETWORK_WHOIS_CONTEXT, *PNETWORK_WHOIS_CONTEXT;

// TDM_NAVIGATE_PAGE can not be called from other threads without comctl32.dll throwing access violations
// after navigating to the page and you press keys such as ctrl, alt, home and insert. (dmex)
#define TaskDialogNavigatePage(WindowHandle, Config) \
    assert(HandleToUlong(NtCurrentThreadId()) == GetWindowThreadProcessId((WindowHandle), NULL)); \
    SendMessage((WindowHandle), TDM_NAVIGATE_PAGE, 0, (LPARAM)(Config));

VOID ShowWhoisWindow(
    _In_ HWND ParentWindowHandle,
    _In_ PPH_NETWORK_ITEM NetworkItem
    );

VOID ShowWhoisWindowFromAddress(
    _In_ HWND ParentWindowHandle,
    _In_ PH_IP_ENDPOINT RemoteEndpoint
    );

// tracert.c

typedef struct _NETWORK_TRACERT_CONTEXT
{
    HWND WindowHandle;
    HWND ParentWindowHandle;
    HWND SearchboxHandle;
    HWND TreeNewHandle;
    HFONT FontHandle;
    HFONT TreeNewFont;
    PH_LAYOUT_MANAGER LayoutManager;

    ULONG Timeout;
    ULONG MaximumHops;
    BOOLEAN Cancel;
    PH_WORK_QUEUE WorkQueue;

    ULONG TreeNewSortColumn;
    PH_SORT_ORDER TreeNewSortOrder;
    PPH_HASHTABLE NodeHashtable;
    PPH_LIST NodeList;
    PPH_LIST NodeRootList;

    PH_IP_ENDPOINT RemoteEndpoint;
    ULONG RemoteAddressStringLength;
    WCHAR RemoteAddressString[INET6_ADDRSTRLEN];
} NETWORK_TRACERT_CONTEXT, *PNETWORK_TRACERT_CONTEXT;

VOID ShowTracertWindow(
    _In_ HWND ParentWindowHandle,
    _In_ PPH_NETWORK_ITEM NetworkItem
    );

VOID ShowTracertWindowFromAddress(
    _In_ HWND ParentWindowHandle,
    _In_ PH_IP_ENDPOINT RemoteEndpoint
    );

// options.c

INT_PTR CALLBACK OptionsDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

// country.c
typedef struct _NETWORK_EXTENSION
{
    LIST_ENTRY ListEntry;
    PPH_NETWORK_ITEM NetworkItem;

    union
    {
        BOOLEAN Flags;
        struct
        {
            BOOLEAN CountryValid : 1;
            BOOLEAN LocalValid : 1;
            BOOLEAN RemoteValid : 1;
            BOOLEAN StatsEnabled : 1;
            BOOLEAN Spare : 4;
        };
    };

    PH_STRINGREF LocalServiceName;
    PH_STRINGREF RemoteServiceName;

    PPH_STRING RemoteCountryName;
    INT CountryIconIndex;

    ULONG64 NumberOfBytesOut;
    ULONG64 NumberOfBytesIn;
    ULONG64 NumberOfLostPackets;
    ULONG SampleRtt;

    PPH_STRING BytesIn;
    PPH_STRING BytesOut;
    PPH_STRING PacketLossText;
    PPH_STRING LatencyText;
} NETWORK_EXTENSION, *PNETWORK_EXTENSION;

typedef enum _NETWORK_COLUMN_ID
{
    NETWORK_COLUMN_ID_NONE,
    NETWORK_COLUMN_ID_REMOTE_COUNTRY,
    NETWORK_COLUMN_ID_LOCAL_SERVICE,
    NETWORK_COLUMN_ID_REMOTE_SERVICE,
    NETWORK_COLUMN_ID_BYTES_IN,
    NETWORK_COLUMN_ID_BYTES_OUT,
    NETWORK_COLUMN_ID_PACKETLOSS,
    NETWORK_COLUMN_ID_LATENCY
} NETWORK_COLUMN_ID;

// country.c

VOID FreeGeoLiteDb(
    VOID
    );

_Success_(return)
BOOLEAN LookupCountryCode(
    _In_ PH_IP_ADDRESS RemoteAddress,
    _Out_ ULONG *CountryNameId,
    _Out_ PPH_STRING *CountryName
    );

_Success_(return)
BOOLEAN LookupSockInAddr4CountryCode(
    _In_ IN_ADDR RemoteAddress,
    _Out_ ULONG *CountryNameId,
    _Out_ PPH_STRING *CountryName
    );

_Success_(return)
BOOLEAN LookupSockInAddr6CountryCode(
    _In_ IN6_ADDR RemoteAddress,
    _Out_ ULONG *CountryNameId,
    _Out_ PPH_STRING *CountryName
    );

INT LookupCountryIcon(
    _In_ ULONG Name
    );

VOID DrawCountryIcon(
    _In_ HDC hdc,
    _In_ RECT rect,
    _In_ INT Index
    );

VOID NetworkToolsGeoDbFlushCache(
    VOID
    );

typedef struct _PH_UPDATER_CONTEXT
{
    HWND DialogHandle;
    HWND ParentWindowHandle;
    WNDPROC DefaultWindowProc;
    ULONG ErrorCode;
} PH_UPDATER_CONTEXT, *PPH_UPDATER_CONTEXT;

BOOLEAN GeoLiteCheckUpdatePlatformSupported(
    VOID
    );

NTSTATUS GeoLiteUpdateThread(
    _In_ PPH_UPDATER_CONTEXT Context
    );

VOID ShowGeoLiteUpdateDialog(
    _In_opt_ HWND ParentWindowHandle
    );

// pages.c

extern PH_EVENT InitializedEvent;

VOID ShowDbCheckForUpdatesDialog(
    _In_ PPH_UPDATER_CONTEXT Context
    );

VOID ShowDbCheckingForUpdatesDialog(
    _In_ PPH_UPDATER_CONTEXT Context
    );

VOID ShowDbInstallRestartDialog(
    _In_ PPH_UPDATER_CONTEXT Context
    );

VOID ShowDbUpdateFailedDialog(
    _In_ PPH_UPDATER_CONTEXT Context
    );

// ports.c
typedef struct _RESOLVED_PORT
{
    PWSTR Name;
    USHORT Port;
} RESOLVED_PORT;

extern CONST RESOLVED_PORT ResolvedPortsTable[6265];

BOOLEAN LookupPortServiceName(
    _In_ ULONG Port,
    _Out_ PPH_STRINGREF ServiceName
    );

#endif
