/*
 * Window Station Support functions
 *
 * This file is part of System Informer.
 */

#ifndef _WINSTA_H
#define _WINSTA_H

// Specifies the current server.
#define WINSTATION_CURRENT_SERVER         ((HANDLE)NULL)
#define WINSTATION_CURRENT_SERVER_HANDLE  ((HANDLE)NULL)
#define WINSTATION_CURRENT_SERVER_NAME    (NULL)

// Specifies the current session (SessionId)
#define WINSTATION_CURRENT_SESSION ((ULONG)-1)

// Specifies any-session (SessionId)
#define WINSTATION_ANY_SESSION ((ULONG)-2)

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
#define CALLBACK_LENGTH 50
#define DLLNAME_LENGTH 32
#define CDNAME_LENGTH 32
#define WDNAME_LENGTH 32
#define PDNAME_LENGTH 32
#define DEVICENAME_LENGTH 128
#define MODEMNAME_LENGTH DEVICENAME_LENGTH
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
#define CLIENTLICENSE_LENGTH 32
#define CLIENTMODEM_LENGTH 40
#define CLIENT_PRODUCT_ID_LENGTH 32
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

// Variable length data descriptor (not needed)
typedef struct _VARDATA_WIRE
{
    USHORT Size;
    USHORT Offset;
} VARDATA_WIRE, *PVARDATA_WIRE;

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

// private
typedef enum _WINSTATIONINFOCLASS
{
    WinStationCreateData, // q: WINSTATIONCREATE
    WinStationConfiguration, // qs: WINSTACONFIGWIRE + USERCONFIG
    WinStationPdParams, // qs: PDPARAMS
    WinStationWd, // q: WDCONFIG
    WinStationPd, // q: PDCONFIG2 + PDPARAMS
    WinStationPrinter, // qs: Not supported.
    WinStationClient, // q: WINSTATIONCLIENT
    WinStationModules, // q:
    WinStationInformation, // q: WINSTATIONINFORMATION
    WinStationTrace, // qs:
    WinStationBeep, // s: // 10
    WinStationEncryptionOff, // s:
    WinStationEncryptionPerm,
    WinStationNtSecurity, // s: (open secure desktop ctrl+alt+del)
    WinStationUserToken, // q: WINSTATIONUSERTOKEN
    WinStationUnused1,
    WinStationVideoData, // q: WINSTATIONVIDEODATA
    WinStationInitialProgram, // s: (set current process as initial program)
    WinStationCd, // q: CDCONFIG
    WinStationSystemTrace, // qs:
    WinStationVirtualData, // q: // 20
    WinStationClientData, // WINSTATIONCLIENTDATA
    WinStationSecureDesktopEnter, // qs:
    WinStationSecureDesktopExit, // qs:
    WinStationLoadBalanceSessionTarget, // q: ULONG
    WinStationLoadIndicator, // q: WINSTATIONLOADINDICATORDATA
    WinStationShadowInfo, // qs: WINSTATIONSHADOW
    WinStationDigProductId, // WINSTATIONPRODID
    WinStationLockedState, // BOOL
    WinStationRemoteAddress, // WINSTATIONREMOTEADDRESS
    WinStationIdleTime, // ULONG // 30
    WinStationLastReconnectType, // ULONG
    WinStationDisallowAutoReconnect, // qs: BOOLEAN
    WinStationMprNotifyInfo,
    WinStationExecSrvSystemPipe, // WCHAR[48]
    WinStationSmartCardAutoLogon, // BOOLEAN
    WinStationIsAdminLoggedOn, // BOOLEAN
    WinStationReconnectedFromId, // ULONG
    WinStationEffectsPolicy, // ULONG
    WinStationType, // ULONG
    WinStationInformationEx, // WINSTATIONINFORMATIONEX // 40
    WinStationValidationInfo
} WINSTATIONINFOCLASS;

/**
 * Retrieves general information used to create the terminal server session (protocol) to which the station belongs.
 */
typedef struct _WINSTATIONCREATE
{
    ULONG fEnableWinStation : 1;
    ULONG MaxInstanceCount;
} WINSTATIONCREATE, *PWINSTATIONCREATE;

typedef struct _WINSTACONFIGWIRE
{
    WCHAR Comment[61]; // The WinStation descriptive comment.
    CHAR OEMId[4]; // Value identifying the OEM implementor of the TermService Listener to which this session (WinStation) belongs. This can be any value defined by the implementer (OEM) of the listener.
    VARDATA_WIRE UserConfig; // VARDATA_WIRE structure defining the size and offset of the variable-length user configuration data succeeding it.
    VARDATA_WIRE NewFields; // VARDATA_WIRE structure defining the size and offset of the variable-length new data succeeding it. This field is not used and is a placeholder for any new data, if and when added.
} WINSTACONFIGWIRE, *PWINSTACONFIGWIRE;

typedef enum _CALLBACKCLASS
{
    Callback_Disable,
    Callback_Roving,
    Callback_Fixed
} CALLBACKCLASS;

// The SHADOWCLASS enumeration is used to indicate the shadow-related settings for a session running on a terminal server.
typedef enum _SHADOWCLASS
{
    Shadow_Disable, // Shadowing is disabled.
    Shadow_EnableInputNotify, // Permission is asked first from the session being shadowed. The shadower is also permitted keyboard and mouse input.
    Shadow_EnableInputNoNotify, // Permission is not asked first from the session being shadowed. The shadower is also permitted keyboard and mouse input.
    Shadow_EnableNoInputNotify, // Permission is asked first from the session being shadowed. The shadower is not permitted keyboard and mouse input and MUST observe the shadowed session.
    Shadow_EnableNoInputNoNotify // Permission is not asked first from the session being shadowed. The shadower is not permitted keyboard and mouse input and MUST observe the shadowed session.
} SHADOWCLASS;

// For a specific terminal server session, the USERCONFIG structure indicates the user and session configuration.
// https://msdn.microsoft.com/en-us/library/cc248610.aspx
typedef struct _USERCONFIG
{
    ULONG fInheritAutoLogon : 1;
    ULONG fInheritResetBroken : 1;
    ULONG fInheritReconnectSame : 1;
    ULONG fInheritInitialProgram : 1;
    ULONG fInheritCallback : 1;
    ULONG fInheritCallbackNumber : 1;
    ULONG fInheritShadow : 1;
    ULONG fInheritMaxSessionTime : 1;
    ULONG fInheritMaxDisconnectionTime : 1;
    ULONG fInheritMaxIdleTime : 1;
    ULONG fInheritAutoClient : 1;
    ULONG fInheritSecurity : 1;
    ULONG fPromptForPassword : 1;
    ULONG fResetBroken : 1;
    ULONG fReconnectSame : 1;
    ULONG fLogonDisabled : 1;
    ULONG fWallPaperDisabled : 1;
    ULONG fAutoClientDrives : 1;
    ULONG fAutoClientLpts : 1;
    ULONG fForceClientLptDef : 1;
    ULONG fRequireEncryption : 1;
    ULONG fDisableEncryption : 1;
    ULONG fUnused1 : 1;
    ULONG fHomeDirectoryMapRoot : 1;
    ULONG fUseDefaultGina : 1;
    ULONG fCursorBlinkDisabled : 1;
    ULONG fPublishedApp : 1;
    ULONG fHideTitleBar : 1;
    ULONG fMaximize : 1;
    ULONG fDisableCpm : 1;
    ULONG fDisableCdm : 1;
    ULONG fDisableCcm : 1;
    ULONG fDisableLPT : 1;
    ULONG fDisableClip : 1;
    ULONG fDisableExe : 1;
    ULONG fDisableCam : 1;
    ULONG fDisableAutoReconnect : 1;
    ULONG ColorDepth : 3;
    ULONG fInheritColorDepth : 1;
    ULONG fErrorInvalidProfile : 1;
    ULONG fPasswordIsScPin : 1;
    ULONG fDisablePNPRedir : 1;
    WCHAR UserName[USERNAME_LENGTH + 1];
    WCHAR Domain[DOMAIN_LENGTH + 1];
    WCHAR Password[PASSWORD_LENGTH + 1];
    WCHAR WorkDirectory[DIRECTORY_LENGTH + 1];
    WCHAR InitialProgram[INITIALPROGRAM_LENGTH + 1];
    WCHAR CallbackNumber[CALLBACK_LENGTH + 1];
    CALLBACKCLASS Callback;
    SHADOWCLASS Shadow;
    ULONG MaxConnectionTime;
    ULONG MaxDisconnectionTime;
    ULONG MaxIdleTime;
    ULONG KeyboardLayout;
    BYTE MinEncryptionLevel;
    WCHAR NWLogonServer[NASIFILESERVER_LENGTH + 1];
    WCHAR PublishedName[MAX_BR_NAME];
    WCHAR WFProfilePath[DIRECTORY_LENGTH + 1];
    WCHAR WFHomeDir[DIRECTORY_LENGTH + 1];
    WCHAR WFHomeDirDrive[4];
} USERCONFIG, *PUSERCONFIG;

typedef enum _SDCLASS
{
    SdNone = 0,
    SdConsole,
    SdNetwork,
    SdAsync,
    SdOemTransport
} SDCLASS;

typedef WCHAR DEVICENAME[DEVICENAME_LENGTH + 1];
typedef WCHAR MODEMNAME[MODEMNAME_LENGTH + 1];
typedef WCHAR NASISPECIFICNAME[NASISPECIFICNAME_LENGTH + 1];
typedef WCHAR NASIUSERNAME[NASIUSERNAME_LENGTH + 1];
typedef WCHAR NASIPASSWORD[NASIPASSWORD_LENGTH + 1];
typedef WCHAR NASISESIONNAME[NASISESSIONNAME_LENGTH + 1];
typedef WCHAR NASIFILESERVER[NASIFILESERVER_LENGTH + 1];
typedef WCHAR WDNAME[WDNAME_LENGTH + 1];
typedef WCHAR WDPREFIX[WDPREFIX_LENGTH + 1];
typedef WCHAR CDNAME[CDNAME_LENGTH + 1];
typedef WCHAR DLLNAME[DLLNAME_LENGTH + 1];
typedef WCHAR PDNAME[PDNAME_LENGTH + 1];

typedef struct _NETWORKCONFIG
{
    LONG LanAdapter;
    DEVICENAME NetworkName;
    ULONG Flags;
} NETWORKCONFIG, *PNETWORKCONFIG;

typedef enum _FLOWCONTROLCLASS
{
    FlowControl_None,
    FlowControl_Hardware,
    FlowControl_Software
} FLOWCONTROLCLASS;

typedef enum _RECEIVEFLOWCONTROLCLASS
{
    ReceiveFlowControl_None,
    ReceiveFlowControl_RTS,
    ReceiveFlowControl_DTR,
} RECEIVEFLOWCONTROLCLASS;

typedef enum _TRANSMITFLOWCONTROLCLASS
{
    TransmitFlowControl_None,
    TransmitFlowControl_CTS,
    TransmitFlowControl_DSR,
} TRANSMITFLOWCONTROLCLASS;

typedef enum _ASYNCCONNECTCLASS
{
    Connect_CTS,
    Connect_DSR,
    Connect_RI,
    Connect_DCD,
    Connect_FirstChar,
    Connect_Perm,
} ASYNCCONNECTCLASS;

typedef struct _FLOWCONTROLCONFIG
{
    ULONG fEnableSoftwareTx : 1;
    ULONG fEnableSoftwareRx : 1;
    ULONG fEnableDTR : 1;
    ULONG fEnableRTS : 1;
    CHAR XonChar;
    CHAR XoffChar;
    FLOWCONTROLCLASS Type;
    RECEIVEFLOWCONTROLCLASS HardwareReceive;
    TRANSMITFLOWCONTROLCLASS HardwareTransmit;
} FLOWCONTROLCONFIG, *PFLOWCONTROLCONFIG;

typedef struct _CONNECTCONFIG
{
    ASYNCCONNECTCLASS Type;
    ULONG fEnableBreakDisconnect : 1;
} CONNECTCONFIG, *PCONNECTCONFIG;

typedef struct _ASYNCCONFIG
{
    DEVICENAME DeviceName;
    MODEMNAME ModemName;
    ULONG BaudRate;
    ULONG Parity;
    ULONG StopBits;
    ULONG ByteSize;
    ULONG fEnableDsrSensitivity : 1;
    ULONG fConnectionDriver : 1;
    FLOWCONTROLCONFIG FlowControl;
    CONNECTCONFIG Connect;
} ASYNCCONFIG, *PASYNCCONFIG;

typedef struct _NASICONFIG
{
    NASISPECIFICNAME SpecificName;
    NASIUSERNAME UserName;
    NASIPASSWORD PassWord;
    NASISESIONNAME SessionName;
    NASIFILESERVER FileServer;
    BOOLEAN GlobalSession;
} NASICONFIG, *PNASICONFIG;

typedef struct _OEMTDCONFIG
{
    LONG Adapter;
    DEVICENAME DeviceName;
    ULONG Flags;
} OEMTDCONFIG, *POEMTDCONFIG;

// Retrieves transport protocol driver parameters.
typedef struct _PDPARAMS
{
    SDCLASS SdClass; // Stack driver class. Indicates which one of the union's structures is valid.
    union
    {
        NETWORKCONFIG Network; // Configuration of network drivers. Used if SdClass is SdNetwork.
        ASYNCCONFIG Async; // Configuration of async (modem) driver. Used if SdClass is SdAsync.
        NASICONFIG Nasi; // Reserved.
        OEMTDCONFIG OemTd; // Configuration of OEM transport driver. Used if SdClass is SdOemTransport.
    };
} PDPARAMS, *PPDPARAMS;

// The WinStation (session) driver configuration.
typedef struct _WDCONFIG
{
    WDNAME WdName; // The descriptive name of the WinStation driver.
    DLLNAME WdDLL; // The driver's image name.
    DLLNAME WsxDLL; // Used by the Terminal Services service to communicate with the WinStation driver.
    ULONG WdFlag; // Driver flags.
    ULONG WdInputBufferLength; // Length, in bytes, of the input buffer used by the driver. Defaults to 2048.
    DLLNAME CfgDLL; // Configuration DLL used by Terminal Services administrative tools for configuring the driver.
    WDPREFIX WdPrefix; // Used as the prefix of the WinStation name generated for the connected sessions with this WinStation driver.
} WDCONFIG, *PWDCONFIG;

// The protocol driver's software configuration.
typedef struct _PDCONFIG2
{
    PDNAME PdName;
    SDCLASS SdClass;
    DLLNAME PdDLL;
    ULONG PdFlag;
    ULONG OutBufLength;
    ULONG OutBufCount;
    ULONG OutBufDelay;
    ULONG InteractiveDelay;
    ULONG PortNumber;
    ULONG KeepAliveTimeout;
} PDCONFIG2, *PPDCONFIG2;

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

// Retrieves information on the session.
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

// Retrieves the user's token in the session. Caller requires WINSTATION_ALL_ACCESS permission.
typedef struct _WINSTATIONUSERTOKEN
{
    HANDLE ProcessId;
    HANDLE ThreadId;
    HANDLE UserToken;
} WINSTATIONUSERTOKEN, *PWINSTATIONUSERTOKEN;

// Retrieves resolution and color depth of the session.
typedef struct _WINSTATIONVIDEODATA
{
    USHORT HResolution;
    USHORT VResolution;
    USHORT fColorDepth;
} WINSTATIONVIDEODATA, *PWINSTATIONVIDEODATA;

typedef enum _CDCLASS
{
    CdNone, // No connection driver.
    CdModem, // Connection driver is a modem.
    CdClass_Maximum,
} CDCLASS;

// Connection driver configuration. It is used for connecting via modem to a server.
typedef struct _CDCONFIG
{
    CDCLASS CdClass; // Connection driver type.
    CDNAME CdName; // Connection driver descriptive name.
    DLLNAME CdDLL; // Connection driver image name.
    ULONG CdFlag; // Connection driver flags. Connection driver specific.
} CDCONFIG, *PCDCONFIG;

// The name has the following form:
// name syntax : xxxyyyy<null>
typedef CHAR CLIENTDATANAME[CLIENTDATANAME_LENGTH + 1];
typedef CHAR* PCLIENTDATANAME;

typedef struct _WINSTATIONCLIENTDATA
{
    CLIENTDATANAME DataName; // Identifies the type of data sent in this WINSTATIONCLIENTDATA structure. The definition is dependent on the caller and on the client receiving it. This MUST be a data name following a format similar to that of the CLIENTDATANAME data type.
    BOOLEAN fUnicodeData; // TRUE indicates data is in Unicode format; FALSE otherwise.
} WINSTATIONCLIENTDATA, *PWINSTATIONCLIENTDATA;

typedef enum _LOADFACTORTYPE
{
    ErrorConstraint, // An error occurred while obtaining constraint data.
    PagedPoolConstraint, // The amount of paged pool is the constraint.
    NonPagedPoolConstraint, // The amount of non-paged pool is the constraint.
    AvailablePagesConstraint, // The amount of available pages is the constraint.
    SystemPtesConstraint, // The number of system page table entries (PTEs) is the constraint.
    CPUConstraint // CPU usage is the constraint.
} LOADFACTORTYPE;

// The WINSTATIONLOADINDICATORDATA structure defines data used for the load balancing of a server.
typedef struct _WINSTATIONLOADINDICATORDATA
{
    ULONG RemainingSessionCapacity; // The estimated number of additional sessions that can be supported given the CPU constraint.
    LOADFACTORTYPE LoadFactor; // Indicates the most constrained current resource.
    ULONG TotalSessions; // The total number of sessions.
    ULONG DisconnectedSessions; // The number of disconnected sessions.
    LARGE_INTEGER IdleCPU; // This is always set to 0.
    LARGE_INTEGER TotalCPU; // This is always set to 0.
    ULONG RawSessionCapacity; // The raw number of sessions capacity.
    ULONG reserved[9]; // Reserved.
} WINSTATIONLOADINDICATORDATA, *PWINSTATIONLOADINDICATORDATA;

typedef enum _SHADOWSTATECLASS
{
    State_NoShadow, // No shadow operations are currently being performed on this session.
    State_Shadowing, // The session is shadowing a different session. The current session is referred to as a shadow client.
    State_Shadowed // The session is being shadowed by a different session. The current session is referred to as a shadow target.
} SHADOWSTATECLASS;

#define PROTOCOL_CONSOLE 0
#define PROTOCOL_OTHERS 1
#define PROTOCOL_RDP 2

// Retrieves the current shadow state of a session.
typedef struct _WINSTATIONSHADOW
{
    SHADOWSTATECLASS ShadowState; // Specifies the current state of shadowing.
    SHADOWCLASS ShadowClass; // Specifies the type of shadowing.
    ULONG SessionId; // Specifies the session ID of the session.
    ULONG ProtocolType; // Specifies the type of protocol on the session. Can be one of PROTOCOL_* values.
} WINSTATIONSHADOW, *PWINSTATIONSHADOW;

// Retrieves the client product ID and current product ID of the session.
typedef struct _WINSTATIONPRODID
{
    WCHAR DigProductId[CLIENT_PRODUCT_ID_LENGTH];
    WCHAR ClientDigProductId[CLIENT_PRODUCT_ID_LENGTH];
    WCHAR OuterMostDigProductId[CLIENT_PRODUCT_ID_LENGTH];
    ULONG CurrentSessionId;
    ULONG ClientSessionId;
    ULONG OuterMostSessionId;
} WINSTATIONPRODID, *PWINSTATIONPRODID;

// Retrieves the remote IP address of the terminal server client in the session.
typedef struct _WINSTATIONREMOTEADDRESS
{
    USHORT sin_family;
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

// WinStationInformationEx

// private
typedef struct _WINSTATIONINFORMATIONEX_LEVEL1
{
    ULONG SessionId;
    WINSTATIONSTATECLASS SessionState;
    LONG SessionFlags;
    WINSTATIONNAME WinStationName;
    WCHAR UserName[USERNAME_LENGTH + 1];
    WCHAR DomainName[DOMAIN_LENGTH + 1];
    LARGE_INTEGER LogonTime;
    LARGE_INTEGER ConnectTime;
    LARGE_INTEGER DisconnectTime;
    LARGE_INTEGER LastInputTime;
    LARGE_INTEGER CurrentTime;
    PROTOCOLSTATUS ProtocolStatus;
} WINSTATIONINFORMATIONEX_LEVEL1, *PWINSTATIONINFORMATIONEX_LEVEL1;

// private
typedef struct _WINSTATIONINFORMATIONEX_LEVEL2
{
    ULONG SessionId;
    WINSTATIONSTATECLASS SessionState;
    LONG SessionFlags;
    WINSTATIONNAME WinStationName;
    WCHAR SamCompatibleUserName[USERNAME_LENGTH + 1];
    WCHAR SamCompatibleDomainName[DOMAIN_LENGTH + 1];
    LARGE_INTEGER LogonTime;
    LARGE_INTEGER ConnectTime;
    LARGE_INTEGER DisconnectTime;
    LARGE_INTEGER LastInputTime;
    LARGE_INTEGER CurrentTime;
    PROTOCOLSTATUS ProtocolStatus;
    WCHAR UserName[257];
    WCHAR DomainName[256];
} WINSTATIONINFORMATIONEX_LEVEL2, *PWINSTATIONINFORMATIONEX_LEVEL2;

// private
typedef union _WINSTATIONINFORMATIONEX_LEVEL
{
    WINSTATIONINFORMATIONEX_LEVEL1 WinStationInfoExLevel1;
    WINSTATIONINFORMATIONEX_LEVEL2 WinStationInfoExLevel2;
} WINSTATIONINFORMATIONEX_LEVEL, *PWINSTATIONINFORMATIONEX_LEVEL;

// private
typedef struct _WINSTATIONINFORMATIONEX
{
    ULONG Level;
    WINSTATIONINFORMATIONEX_LEVEL Data;
} WINSTATIONINFORMATIONEX, *PWINSTATIONINFORMATIONEX;

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
    LARGE_INTEGER CycleTime;
    LARGE_INTEGER CreateTime;
    LARGE_INTEGER UserTime;
    LARGE_INTEGER KernelTime;
    UNICODE_STRING ImageName;
    KPRIORITY BasePriority;
    ULONG UniqueProcessId;
    ULONG InheritedFromUniqueProcessId;
    ULONG HandleCount;
    ULONG SessionId;
    ULONG UniqueProcessKey;
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

// begin_rev
// Flags for WinStationRegisterConsoleNotification
#define WNOTIFY_ALL_SESSIONS 0x1
// end_rev

// In the functions below, memory returned can be freed using LocalFree. NULL can be specified for
// server handles to indicate the local server. -1 can be specified for session IDs to indicate the
// current session ID.

#define LOGONID_CURRENT (-1)
#define SERVERNAME_CURRENT ((PWSTR)NULL)

// rev
NTSYSAPI
BOOLEAN
NTAPI
WinStationFreeMemory(
    _In_ PVOID Buffer
    );

// rev
NTSYSAPI
HANDLE
NTAPI
WinStationOpenServerW(
    _In_opt_ PCWSTR ServerName
    );

// rev
NTSYSAPI
BOOLEAN
NTAPI
WinStationCloseServer(
    _In_ HANDLE ServerHandle
    );

// rev
NTSYSAPI
BOOLEAN
NTAPI
WinStationServerPing(
    _In_opt_ HANDLE ServerHandle
    );

// rev
NTSYSAPI
BOOLEAN
NTAPI
WinStationGetTermSrvCountersValue(
    _In_opt_ HANDLE ServerHandle,
    _In_ ULONG Count,
    _Inout_ PTS_COUNTER Counters // set counter IDs before calling
    );

NTSYSAPI
BOOLEAN
NTAPI
WinStationShutdownSystem(
    _In_opt_ HANDLE ServerHandle,
    _In_ ULONG ShutdownFlags // WSD_*
    );

// rev
NTSYSAPI
BOOLEAN
NTAPI
WinStationWaitSystemEvent(
    _In_opt_ HANDLE ServerHandle,
    _In_ ULONG EventMask, // WEVENT_*
    _Out_ PULONG EventFlags
    );

// rev
NTSYSAPI
BOOLEAN
NTAPI
WinStationRegisterConsoleNotification(
    _In_opt_ HANDLE ServerHandle,
    _In_ HWND WindowHandle,
    _In_ ULONG Flags
    );

// rev
NTSYSAPI
BOOLEAN
NTAPI
WinStationUnRegisterConsoleNotification(
    _In_opt_ HANDLE ServerHandle,
    _In_ HWND WindowHandle
    );

// Sessions

// rev
NTSYSAPI
BOOLEAN
NTAPI
WinStationEnumerateW(
    _In_opt_ HANDLE ServerHandle,
    _Out_ PSESSIONIDW *SessionIds,
    _Out_ PULONG Count
    );

NTSYSAPI
BOOLEAN
NTAPI
WinStationQueryInformationW(
    _In_opt_ HANDLE ServerHandle,
    _In_ ULONG SessionId,
    _In_ WINSTATIONINFOCLASS WinStationInformationClass,
    _Out_writes_bytes_(WinStationInformationLength) PVOID pWinStationInformation,
    _In_ ULONG WinStationInformationLength,
    _Out_ PULONG pReturnLength
    );

// rev
NTSYSAPI
BOOLEAN
NTAPI
WinStationSetInformationW(
    _In_opt_ HANDLE ServerHandle,
    _In_ ULONG SessionId,
    _In_ WINSTATIONINFOCLASS WinStationInformationClass,
    _In_reads_bytes_(WinStationInformationLength) PVOID pWinStationInformation,
    _In_ ULONG WinStationInformationLength
    );

// rev
NTSYSAPI
BOOLEAN
NTAPI
WinStationQueryCurrentSessionInformation(
    _In_ WINSTATIONINFOCLASS WinStationInformationClass,
    _In_reads_bytes_(WinStationInformationLength) PVOID pWinStationInformation,
    _In_ ULONG WinStationInformationLength
    );

NTSYSAPI
BOOLEAN
NTAPI
WinStationNameFromLogonIdW(
    _In_opt_ HANDLE ServerHandle,
    _In_ ULONG SessionId,
    _Out_writes_(WINSTATIONNAME_LENGTH + 1) PWSTR pWinStationName
    );

// rev
NTSYSAPI
BOOLEAN
NTAPI
LogonIdFromWinStationNameW(
    _In_opt_ HANDLE ServerHandle,
    _In_ PCWSTR pWinStationName,
    _Out_ PULONG SessionId
    );

// rev
NTSYSAPI
BOOLEAN
NTAPI
WinStationSendMessageW(
    _In_opt_ HANDLE ServerHandle,
    _In_ ULONG SessionId,
    _In_ PCWSTR Title,
    _In_ ULONG TitleLength,
    _In_ PCWSTR Message,
    _In_ ULONG MessageLength,
    _In_ ULONG Style,
    _In_ ULONG Timeout,
    _Out_ PULONG Response,
    _In_ BOOLEAN DoNotWait
    );

NTSYSAPI
BOOLEAN
NTAPI
WinStationConnectW(
    _In_opt_ HANDLE ServerHandle,
    _In_ ULONG SessionId,
    _In_ ULONG TargetSessionId,
    _In_opt_ PCWSTR pPassword,
    _In_ BOOLEAN bWait
    );

NTSYSAPI
BOOLEAN
NTAPI
WinStationDisconnect(
    _In_opt_ HANDLE ServerHandle,
    _In_ ULONG SessionId,
    _In_ BOOLEAN bWait
    );

// rev
NTSYSAPI
BOOLEAN
NTAPI
WinStationReset(
    _In_opt_ HANDLE ServerHandle,
    _In_ ULONG SessionId,
    _In_ BOOLEAN bWait
    );

// rev
NTSYSAPI
BOOLEAN
NTAPI
WinStationShadow(
    _In_opt_ HANDLE ServerHandle,
    _In_ PCWSTR TargetServerName,
    _In_ ULONG TargetSessionId,
    _In_ UCHAR HotKeyVk,
    _In_ USHORT HotkeyModifiers // KBD*
    );

// rev
NTSYSAPI
BOOLEAN
NTAPI
WinStationShadowStop(
    _In_opt_ HANDLE ServerHandle,
    _In_ ULONG SessionId,
    _In_ BOOLEAN bWait // ignored
    );

// Processes

// rev
NTSYSAPI
BOOLEAN
NTAPI
WinStationEnumerateProcesses(
    _In_opt_ HANDLE ServerHandle,
    _Out_ PVOID *Processes
    );

#define WINSTATION_PROCESS_LEVEL 0

// rev
NTSYSAPI
BOOLEAN
NTAPI
WinStationGetAllProcesses(
    _In_opt_ HANDLE ServerHandle,
    _In_ ULONG Level,
    _Out_ PULONG NumberOfProcesses,
    _Out_ PTS_ALL_PROCESSES_INFO *Processes
    );

// rev
NTSYSAPI
BOOLEAN
NTAPI
WinStationFreeGAPMemory(
    _In_ ULONG Level,
    _In_ PTS_ALL_PROCESSES_INFO Processes,
    _In_ ULONG NumberOfProcesses
    );

// rev
NTSYSAPI
BOOLEAN
NTAPI
WinStationTerminateProcess(
    _In_opt_ HANDLE ServerHandle,
    _In_ ULONG ProcessId,
    _In_ ULONG ExitCode
    );

NTSYSAPI
BOOLEAN
NTAPI
WinStationGetProcessSid(
    _In_opt_ HANDLE ServerHandle,
    _In_ ULONG ProcessId,
    _In_ FILETIME ProcessStartTime,
    _Out_ PVOID pProcessUserSid,
    _Inout_ PULONG dwSidSize
    );

// Services isolation

#if (PHNT_VERSION >= PHNT_VISTA)

// rev
NTSYSAPI
BOOLEAN
NTAPI
WinStationSwitchToServicesSession(
    VOID
    );

// rev
NTSYSAPI
BOOLEAN
NTAPI
WinStationRevertFromServicesSession(
    VOID
    );

#endif

// Misc.
NTSYSAPI
BOOLEAN
NTAPI
_WinStationWaitForConnect(
    VOID
    );

// rev
NTSYSAPI
HANDLE
NTAPI
WinStationVirtualOpen(
    _In_opt_ HANDLE ServerHandle,
    _In_ ULONG SessionId,
    _In_ PCSTR Name
    );

// rev
NTSYSAPI
HANDLE
NTAPI
WinStationVirtualOpenEx(
    _In_opt_ HANDLE ServerHandle,
    _In_ ULONG SessionId,
    _In_ PCSTR Name,
    _In_ ULONG Flags
    );

// rev
NTSYSAPI
BOOLEAN
NTAPI
WinStationIsCurrentSessionRemoteable(
    _Out_ PBOOLEAN IsRemoteable
    );

EXTERN_C DECLSPEC_SELECTANY CONST GUID PROPERTY_TYPE_GET_MONITOR_CONFIG = { 0x865D5285, 0xF70A, 0x4ECF, { 0x8B, 0x28, 0x51, 0x2F, 0xE0, 0xAA, 0x2D, 0x53 } };
EXTERN_C DECLSPEC_SELECTANY CONST GUID PROPERTY_TYPE_CORRELATIONID_GUID = { 0x9A363F8E, 0x1902, 0x40DA, { 0xA2, 0xCC, 0x56, 0x4F, 0x09, 0x40, 0xAD, 0xE3 } };

typedef struct _TS_PROPERTY_INFORMATION
{
    ULONG Length;
    PVOID Buffer;
} TS_PROPERTY_INFORMATION, *PTS_PROPERTY_INFORMATION;

// rev
NTSYSAPI
BOOLEAN
NTAPI
WinStationGetConnectionProperty(
    _In_ ULONG SessionId,
    _In_ PCGUID PropertyType,
    _Out_ PTS_PROPERTY_INFORMATION PropertyBuffer
    );

// rev
NTSYSAPI
BOOLEAN
NTAPI
WinStationFreePropertyValue(
    _In_ PVOID PropertyBuffer
    );

// rev
NTSYSAPI
BOOLEAN
NTAPI
WinStationIsSessionRemoteable(
    _In_opt_ HANDLE ServerHandle,
    _In_ ULONG SessionId,
    _Out_ PBOOLEAN IsRemote
    );

// rev
NTSYSAPI
BOOLEAN
NTAPI
WinStationSetAutologonPassword(
    _In_ PCSTR KeyName,
    _In_ PCSTR Password
    );

typedef enum _SessionType
{
    SESSIONTYPE_UNKNOWN = 0,
    SESSIONTYPE_SERVICES,
    SESSIONTYPE_LISTENER,
    SESSIONTYPE_REGULARDESKTOP,
    SESSIONTYPE_ALTERNATESHELL,
    SESSIONTYPE_REMOTEAPP,
    SESSIONTYPE_MEDIACENTEREXT
} SESSIONTYPE;

// rev
typedef struct _TS_USER_SESSION
{
    ULONG Version;
    ULONG SessionId;
    ULONG Unknown;
    SESSIONTYPE State;
    ULONG field5;
} TS_USER_SESSION, *PTS_USER_SESSION;

// rev
NTSYSAPI
BOOLEAN
NTAPI
WinStationGetAllUserSessions(
    _In_opt_ HANDLE ServerHandle,
    _In_ PSID Sid,
    _Out_ PVOID* Processes, // LocalFree
    _Out_ PULONG NumberOfProcesses
    );

// rev
typedef struct _TS_SESSION_VIRTUAL_ADDRESS
{
  USHORT AddressFamily;
  USHORT AddressLength;
  BYTE Address[20];
} TS_SESSION_VIRTUAL_ADDRESS, *PTS_SESSION_VIRTUAL_ADDRESS;
typedef USHORT ADDRESS_FAMILY;

// rev
NTSYSAPI
BOOLEAN
NTAPI
WinStationQuerySessionVirtualIP(
    _In_opt_ HANDLE ServerHandle,
    _In_ ULONG SessionId,
    _In_ ADDRESS_FAMILY Family,
    _Out_ TS_SESSION_VIRTUAL_ADDRESS* SessionVirtualIP
    );

// rev
NTSYSAPI
BOOLEAN
NTAPI
WinStationGetDeviceId(
    _In_opt_ HANDLE ServerHandle,
    _In_ ULONG SessionId,
    _Out_ PCHAR* Buffer, // CHAR DeviceId[MAX_PATH + 1];
    _In_ SIZE_T BufferLength
    );

// rev
NTSYSAPI
BOOLEAN
NTAPI
WinStationGetLoggedOnCount(
    _Out_ PULONG LoggedOnUserCount,
    _Out_ PULONG LoggedOnDeviceCount
    );

#endif
