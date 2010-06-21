/*
 * Process Hacker - 
 *   winsta definitions
 * 
 * Copyright (C) 2010 wj32
 * 
 * This file is part of Process Hacker.
 * 
 * Process Hacker is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Process Hacker is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Process Hacker.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef WINSTA_H
#define WINSTA_H

// begin_msdn:http://msdn.microsoft.com/en-us/library/cc248779%28PROT.10%29.aspx

// Access rights

#define WINSTATION_QUERY 0x00000001 // WinStationQueryInformation
#define WINSTATION_SET 0x00000002 // WinStationSetInformation
#define WINSTATION_RESET 0x00000004 // WinStationReset
#define WINSTATION_VIRTUAL 0x00000008 //read/write direct data
#define WINSTATION_SHADOW 0x00000010 // WinStationShadow
#define WINSTATION_LOGON 0x00000020 // logon to WinStation
#define WINSTATION_LOGOFF 0x00000040 // WinStationLogoff
#define WINSTATION_MSG 0x00000080 // WinStationMsg
#define WINSTATION_CONNECT 0x00000100 // WinStationConnect
#define WINSTATION_DISCONNECT 0x00000200 // WinStationDisconnect
#define WINSTATION_GUEST_ACCESS WINSTATION_LOGON

#define WINSTATION_CURRENT_GUEST_ACCESS (WINSTATION_VIRTUAL | WINSTATION_LOGOFF)
#define WINSTATION_USER_ACCESS (WINSTATION_GUEST_ACCESS | WINSTATION_QUERY | WINSTATION_CONNECT)
#define WINSTATION_CURRENT_USER_ACCESS \
    (WINSTATION_SET | WINSTATION_RESET | WINSTATION_VIRTUAL | \
    WINSTATION_LOGOFF | WINSTATION_DISCONNECT)
#define WINSTATION_ALL_ACCESS (STANDARD_RIGHTS_REQUIRED | WINSTATION_QUERY | \
    WINSTATION_SET | WINSTATION_RESET | WINSTATION_VIRTUAL | \
    WINSTATION_SHADOW | WINSTATION_LOGON | WINSTATION_MSG | \
    WINSTATION_CONNECT | WINSTATION_DISCONNECT)

#define WDPREFIX_LENGTH 12
#define STACK_ADDRESS_LENGTH 128
#define MAX_BR_NAME 65
#define DIRECTORY_LENGTH 256
#define INITIALPROGRAM_LENGTH 256
#define USERNAME_LENGTH 20
#define DOMAIN_LENGTH 17
#define PASSWORD_LENGTH 14
#define NASISPECIFICNAME_LENGTH 14
#define NASIUSERNAME_LENGTH 47
#define NASIPASSWORD_LENGTH 24
#define NASISESSIONNAME_LENGTH 16
#define NASIFILESERVER_LENGTH 47

#define CLIENTDATANAME_LENGTH 7
#define CLIENTNAME_LENGTH 20
#define CLIENTADDRESS_LENGTH 30
#define IMEFILENAME_LENGTH 32
#define DIRECTORY_LENGTH 256
#define CLIENTLICENSE_LENGTH 32
#define CLIENTMODEM_LENGTH 40
#define CLIENT_PRODUCT_ID_LENGTH  32
#define MAX_COUNTER_EXTENSIONS 2    
#define WINSTATIONNAME_LENGTH 32

#define TERMSRV_TOTAL_SESSIONS 1     
#define TERMSRV_DISC_SESSIONS 2     
#define TERMSRV_RECON_SESSIONS 3     
#define TERMSRV_CURRENT_ACTIVE_SESSIONS 4   
#define TERMSRV_CURRENT_DISC_SESSIONS 5   
#define TERMSRV_PENDING_SESSIONS 6   
#define TERMSRV_SUCC_TOTAL_LOGONS 7   
#define TERMSRV_SUCC_LOCAL_LOGONS 8   
#define TERMSRV_SUCC_REMOTE_LOGONS 9   
#define TERMSRV_SUCC_SESSION0_LOGONS 10   
#define TERMSRV_CURRENT_TERMINATING_SESSIONS 11
#define TERMSRV_CURRENT_LOGGEDON_SESSIONS 12

typedef RTL_TIME_ZONE_INFORMATION TS_TIME_ZONE_INFORMATION, *PTS_TIME_ZONE_INFORMATION;

typedef WCHAR WINSTATIONNAME[WINSTATIONNAME_LENGTH + 1];

typedef enum _WINSTATIONSTATECLASS
{
    State_Active = 0,
    State_Connected = 1,
    State_ConnectQuery = 2,
    State_Shadow = 3,
    State_Disconnected = 4,
    State_Idle = 5,
    State_Listen = 6,
    State_Reset = 7,
    State_Down = 8,
    State_Init = 9
} WINSTATIONSTATECLASS;

typedef struct _SESSIONIDW
{
    union
    {
        ULONG SessionId;
        ULONG LogonId;
    };
    WINSTATIONNAME WinStationName;
    WINSTATIONSTATECLASS State;
} SESSIONIDW, *PSESSIONIDW;

typedef enum _WINSTATIONINFOCLASS
{
    WinStationCreateData,
    WinStationConfiguration,
    WinStationPdParams,
    WinStationWd,
    WinStationPd,
    WinStationPrinter,
    WinStationClient,
    WinStationModules,
    WinStationInformation,
    WinStationTrace,
    WinStationBeep,
    WinStationEncryptionOff,
    WinStationEncryptionPerm,
    WinStationNtSecurity,
    WinStationUserToken,
    WinStationUnused1,
    WinStationVideoData,
    WinStationInitialProgram,
    WinStationCd,
    WinStationSystemTrace,
    WinStationVirtualData,
    WinStationClientData,
    WinStationSecureDesktopEnter,
    WinStationSecureDesktopExit,
    WinStationLoadBalanceSessionTarget,
    WinStationLoadIndicator,
    WinStationShadowInfo,
    WinStationDigProductId,
    WinStationLockedState,
    WinStationRemoteAddress,
    WinStationIdleTime,
    WinStationLastReconnectType,
    WinStationDisallowAutoReconnect,
    WinStationUnused2,
    WinStationUnused3,
    WinStationUnused4,
    WinStationUnused5,
    WinStationReconnectedFromId,
    WinStationEffectsPolicy,
    WinStationType,
    WinStationInformationEx
} WINSTATIONINFOCLASS;

// WinStationCreateData
typedef struct _WINSTATIONCREATE
{
    ULONG fEnableWinStation : 1;
    ULONG MaxInstanceCount;
} WINSTATIONCREATE, *PWINSTATIONCREATE;

// WinStationClient
typedef struct _WINSTATIONCLIENT
{
    ULONG fTextOnly : 1;
    ULONG fDisableCtrlAltDel : 1;
    ULONG fMouse : 1;
    ULONG fDoubleClickDetect : 1;
    ULONG fINetClient : 1;
    ULONG fPromptForPassword : 1;
    ULONG fMaximizeShell : 1;
    ULONG fEnableWindowsKey : 1;
    ULONG fRemoteConsoleAudio : 1;
    ULONG fPasswordIsScPin : 1;
    ULONG fNoAudioPlayback : 1;
    ULONG fUsingSavedCreds : 1;
    WCHAR ClientName[CLIENTNAME_LENGTH + 1];
    WCHAR Domain[DOMAIN_LENGTH + 1];
    WCHAR UserName[USERNAME_LENGTH + 1];
    WCHAR Password[PASSWORD_LENGTH + 1];
    WCHAR WorkDirectory[DIRECTORY_LENGTH + 1];
    WCHAR InitialProgram[INITIALPROGRAM_LENGTH + 1];
    ULONG SerialNumber;
    BYTE EncryptionLevel;
    ULONG ClientAddressFamily;
    WCHAR ClientAddress[CLIENTADDRESS_LENGTH + 1];
    USHORT HRes;
    USHORT VRes;
    USHORT ColorDepth;
    USHORT ProtocolType;
    ULONG KeyboardLayout;
    ULONG KeyboardType;
    ULONG KeyboardSubType;
    ULONG KeyboardFunctionKey;
    WCHAR ImeFileName[IMEFILENAME_LENGTH + 1];
    WCHAR ClientDirectory[DIRECTORY_LENGTH + 1];
    WCHAR ClientLicense[CLIENTLICENSE_LENGTH + 1];
    WCHAR ClientModem[CLIENTMODEM_LENGTH + 1];
    ULONG ClientBuildNumber;
    ULONG ClientHardwareId;
    USHORT ClientProductId;
    USHORT OutBufCountHost;
    USHORT OutBufCountClient;
    USHORT OutBufLength;
    WCHAR AudioDriverName[9];
    TS_TIME_ZONE_INFORMATION ClientTimeZone;
    ULONG ClientSessionId;
    WCHAR ClientDigProductId[CLIENT_PRODUCT_ID_LENGTH];
    ULONG PerformanceFlags;
    ULONG ActiveInputLocale;
} WINSTATIONCLIENT, *PWINSTATIONCLIENT;

typedef struct _TSHARE_COUNTERS
{
    ULONG Reserved;
} TSHARE_COUNTERS, *PTSHARE_COUNTERS;

typedef struct _PROTOCOLCOUNTERS
{
    ULONG WdBytes;             
    ULONG WdFrames;            
    ULONG WaitForOutBuf;       
    ULONG Frames;              
    ULONG Bytes;               
    ULONG CompressedBytes;     
    ULONG CompressFlushes;     
    ULONG Errors;              
    ULONG Timeouts;            
    ULONG AsyncFramingError;   
    ULONG AsyncOverrunError;   
    ULONG AsyncOverflowError;  
    ULONG AsyncParityError;    
    ULONG TdErrors;            
    USHORT ProtocolType;       
    USHORT Length;             
    union
    {
        TSHARE_COUNTERS TShareCounters;
        ULONG Reserved[100];
    } Specific;
} PROTOCOLCOUNTERS, *PPROTOCOLCOUNTERS;

typedef struct _THINWIRECACHE
{
    ULONG CacheReads;
    ULONG CacheHits;
} THINWIRECACHE, *PTHINWIRECACHE;

#define MAX_THINWIRECACHE 4

typedef struct _RESERVED_CACHE
{
    THINWIRECACHE ThinWireCache[MAX_THINWIRECACHE];
} RESERVED_CACHE, *PRESERVED_CACHE;

typedef struct _TSHARE_CACHE
{
    ULONG Reserved;
} TSHARE_CACHE, *PTSHARE_CACHE;

typedef struct CACHE_STATISTICS
{
    USHORT ProtocolType;    
    USHORT Length;          
    union
    {
        RESERVED_CACHE ReservedCacheStats;
        TSHARE_CACHE TShareCacheStats;
        ULONG Reserved[20];
    } Specific;
} CACHE_STATISTICS, *PCACHE_STATISTICS;

typedef struct _PROTOCOLSTATUS
{
    PROTOCOLCOUNTERS Output;
    PROTOCOLCOUNTERS Input;
    CACHE_STATISTICS Cache;
    ULONG AsyncSignal;     
    ULONG AsyncSignalMask; 
} PROTOCOLSTATUS, *PPROTOCOLSTATUS;

// WinStationInformation
typedef struct _WINSTATIONINFORMATION
{
    WINSTATIONSTATECLASS ConnectState;
    WINSTATIONNAME WinStationName;
    ULONG LogonId;
    LARGE_INTEGER ConnectTime;
    LARGE_INTEGER DisconnectTime;
    LARGE_INTEGER LastInputTime;
    LARGE_INTEGER LogonTime;
    PROTOCOLSTATUS Status;
    WCHAR Domain[DOMAIN_LENGTH + 1];
    WCHAR UserName[USERNAME_LENGTH + 1];
    LARGE_INTEGER CurrentTime;
} WINSTATIONINFORMATION, *PWINSTATIONINFORMATION;

// WinStationUserToken
typedef struct _WINSTATIONUSERTOKEN
{
    HANDLE ProcessId;
    HANDLE ThreadId;
    HANDLE UserToken;
} WINSTATIONUSERTOKEN, *PWINSTATIONUSERTOKEN;

// WinStationVideoData
typedef struct _WINSTATIONVIDEODATA
{
    USHORT HResolution;
    USHORT VResolution;
    USHORT fColorDepth;
} WINSTATIONVIDEODATA, *PWINSTATIONVIDEODATA;

// WinStationDigProductId
typedef struct _WINSTATIONPRODID
{
    WCHAR DigProductId[CLIENT_PRODUCT_ID_LENGTH];
    WCHAR ClientDigProductId[CLIENT_PRODUCT_ID_LENGTH];
    WCHAR OuterMostDigProductId[CLIENT_PRODUCT_ID_LENGTH];
    ULONG CurrentSessionId;
    ULONG ClientSessionId;
    ULONG OuterMostSessionId;
} WINSTATIONPRODID, *PWINSTATIONPRODID;

// (fixed struct layout)
// WinStationRemoteAddress
typedef struct _WINSTATIONREMOTEADDRESS
{
    union
    {
        USHORT sin_family;
        struct
        {
            USHORT sin_family;
            USHORT sin_port;
            ULONG sin_addr;
            UCHAR sin_zero[8];
        } ipv4;
        struct
        {
            USHORT sin_family;
            USHORT sin6_port;
            ULONG sin6_flowinfo;
            USHORT sin6_addr[8];
            ULONG sin6_scope_id;
        } ipv6;
    };
} WINSTATIONREMOTEADDRESS, *PWINSTATIONREMOTEADDRESS;

#define TS_PROCESS_INFO_MAGIC_NT4 0x23495452

typedef struct _TS_PROCESS_INFORMATION_NT4
{
    ULONG MagicNumber;
    ULONG LogonId;
    PVOID ProcessSid;
    ULONG Pad;
} TS_PROCESS_INFORMATION_NT4, *PTS_PROCESS_INFORMATION_NT4;

#define SIZEOF_TS4_SYSTEM_THREAD_INFORMATION 64
#define SIZEOF_TS4_SYSTEM_PROCESS_INFORMATION 136

typedef struct _TS_SYS_PROCESS_INFORMATION
{
    ULONG NextEntryOffset;
    ULONG NumberOfThreads;
    LARGE_INTEGER SpareLi1;
    LARGE_INTEGER SpareLi2;
    LARGE_INTEGER SpareLi3;
    LARGE_INTEGER CreateTime;
    LARGE_INTEGER UserTime;
    LARGE_INTEGER KernelTime;
    UNICODE_STRING ImageName;
    LONG BasePriority;
    ULONG UniqueProcessId;
    ULONG InheritedFromUniqueProcessId;
    ULONG HandleCount;
    ULONG SessionId;
    ULONG SpareUl3;
    SIZE_T PeakVirtualSize;
    SIZE_T VirtualSize;
    ULONG PageFaultCount;
    ULONG PeakWorkingSetSize;
    ULONG WorkingSetSize;
    SIZE_T QuotaPeakPagedPoolUsage;
    SIZE_T QuotaPagedPoolUsage;
    SIZE_T QuotaPeakNonPagedPoolUsage;
    SIZE_T QuotaNonPagedPoolUsage;
    SIZE_T PagefileUsage;
    SIZE_T PeakPagefileUsage;
    SIZE_T PrivatePageCount;
} TS_SYS_PROCESS_INFORMATION, *PTS_SYS_PROCESS_INFORMATION;

typedef struct _TS_ALL_PROCESSES_INFO
{
    PTS_SYS_PROCESS_INFORMATION pTsProcessInfo;
    ULONG SizeOfSid;
    PSID pSid;
} TS_ALL_PROCESSES_INFO, *PTS_ALL_PROCESSES_INFO;

typedef struct _TS_COUNTER_HEADER
{
    DWORD dwCounterID;
    BOOLEAN bResult;
} TS_COUNTER_HEADER, *PTS_COUNTER_HEADER;

typedef struct _TS_COUNTER
{
    TS_COUNTER_HEADER CounterHead;
    DWORD dwValue;
    LARGE_INTEGER StartTime;
} TS_COUNTER, *PTS_COUNTER;

// Flags for WinStationShutdownSystem
#define WSD_LOGOFF 0x1
#define WSD_SHUTDOWN 0x2
#define WSD_REBOOT 0x4
#define WSD_POWEROFF 0x8

// Flags for WinStationWaitSystemEvent
#define WEVENT_NONE 0x0
#define WEVENT_CREATE 0x1
#define WEVENT_DELETE 0x2
#define WEVENT_RENAME 0x4
#define WEVENT_CONNECT 0x8
#define WEVENT_DISCONNECT 0x10
#define WEVENT_LOGON 0x20
#define WEVENT_LOGOFF 0x40
#define WEVENT_STATECHANGE 0x80
#define WEVENT_LICENSE 0x100
#define WEVENT_ALL 0x7fffffff
#define WEVENT_FLUSH 0x80000000

// Hotkey modifiers for WinStationShadow
#define KBDSHIFT 0x1
#define KBDCTRL 0x2
#define KBDALT 0x4

// In the functions below, memory returned can be freed using LocalFree.
// NULL can be specified for server handles to indicate the local server.
// -1 can be specified for session IDs to indicate the current session ID.

#define LOGONID_CURRENT (-1)

BOOLEAN
NTAPI
WinStationServerPing(
    __in_opt HANDLE ServerHandle
    );

BOOLEAN
NTAPI
WinStationEnumerateW(
    __in_opt HANDLE ServerHandle,
    __out PSESSIONIDW *SessionIds,
    __out PULONG Count
    );

BOOLEAN
NTAPI
WinStationQueryInformationW(
    __in_opt HANDLE ServerHandle,
    __in ULONG SessionId,
    __in WINSTATIONINFOCLASS WinStationInformationClass,
    __out PVOID WinStationInformation,
    __in ULONG WinStationInformationLength,
    __out PULONG ReturnLength
    );

BOOLEAN
NTAPI
WinStationSendMessageW(
    __in_opt HANDLE ServerHandle,
    __in ULONG SessionId,
    __in PWSTR Title,
    __in ULONG TitleLength,
    __in PWSTR Message,
    __in ULONG MessageLength,
    __in ULONG Style,
    __in ULONG Timeout,
    __out PULONG Response,
    __in BOOLEAN DoNotWait
    );

BOOLEAN
NTAPI
WinStationNameFromLogonIdW(
    __in_opt HANDLE ServerHandle,
    __in ULONG SessionId,
    __out_ecount(WINSTATIONNAME_LENGTH + 1) PWSTR WinStationName
    );

BOOLEAN
NTAPI
WinStationConnectW(
    __in_opt HANDLE ServerHandle,
    __in ULONG TargetSessionId,
    __in ULONG CurrentSessionId,
    __in_opt PWSTR Password,
    __in BOOLEAN Wait
    );

BOOLEAN
NTAPI
WinStationDisconnect(
    __in_opt HANDLE ServerHandle,
    __in ULONG SessionId,
    __in BOOLEAN Wait
    );

BOOLEAN
NTAPI
WinStationReset(
    __in_opt HANDLE ServerHandle,
    __in ULONG SessionId,
    __in BOOLEAN Wait
    );

BOOLEAN
NTAPI
WinStationShutdownSystem(
    __in_opt HANDLE ServerHandle,
    __in ULONG ShutdownFlags // WSD_*
    );

BOOLEAN
NTAPI
WinStationWaitSystemEvent(
    __in_opt HANDLE ServerHandle,
    __in ULONG EventMask, // WEVENT_*
    __out PULONG EventFlags
    );

BOOLEAN
NTAPI
WinStationShadow(
    __in_opt HANDLE ServerHandle,
    __in PWSTR TargetServerName,
    __in ULONG TargetSessionId,
    __in UCHAR HotKeyVk,
    __in USHORT HotkeyModifiers // KBD*
    );

BOOLEAN
NTAPI
WinStationEnumerateProcesses(
    __in_opt HANDLE ServerHandle,
    __out PVOID *Processes
    );

BOOLEAN
NTAPI
WinStationTerminateProcess(
    __in_opt HANDLE ServerHandle,
    __in HANDLE ProcessId,
    __in NTSTATUS ExitStatus
    );

BOOLEAN
NTAPI
WinStationGetAllProcesses(
    __in_opt HANDLE ServerHandle,
    __in ULONG Level,
    __out PULONG NumberOfProcesses,
    __out PTS_ALL_PROCESSES_INFO *Processes
    );

BOOLEAN
NTAPI
WinStationGetProcessSid(
    __in_opt HANDLE ServerHandle,
    __in HANDLE ProcessId,
    __in PLARGE_INTEGER CreateTime,
    __out PSID Sid,
    __inout PULONG SidLength
    );

BOOLEAN
NTAPI
WinStationGetTermSrvCountersValue(
    __in_opt HANDLE ServerHandle,
    __in ULONG Count,
    __inout PTS_COUNTER Counters // set counter IDs before calling
    );

BOOLEAN
NTAPI
WinStationShadowStop(
    __in_opt HANDLE ServerHandle,
    __in ULONG SessionId,
    __in BOOLEAN Wait // ignored
    );

// typedefs

typedef BOOLEAN (NTAPI *_WinStationConnectW)(
    __in_opt HANDLE ServerHandle,
    __in ULONG TargetSessionId,
    __in ULONG CurrentSessionId,
    __in_opt PWSTR Password,
    __in BOOLEAN Wait
    );

// end_msdn

#endif
