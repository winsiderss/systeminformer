/*
 * Window Station Support functions
 *
 * This file is part of System Informer.
 */

#ifndef _WINSTA_H
#define _WINSTA_H

//
// Sessions
//

// Specifies the current server.
#define WINSTATION_CURRENT_SERVER         ((HANDLE)NULL)
#define WINSTATION_CURRENT_SERVER_HANDLE  ((HANDLE)NULL)
#define WINSTATION_CURRENT_SERVER_NAME    (NULL)
#define SERVERNAME_CURRENT ((PWSTR)NULL)

// Specifies the current session (SessionId)
#define WINSTATION_CURRENT_SESSION ((ULONG)-1)
#define LOGONID_CURRENT (-1)

// Specifies any-session (SessionId)
#define WINSTATION_ANY_SESSION ((ULONG)-2)

//
// Access rights
//

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

typedef CHAR WINSTATIONNAMEA[WINSTATIONNAME_LENGTH + 1];
typedef WCHAR WINSTATIONNAME[WINSTATIONNAME_LENGTH + 1];

/**
 * The VARDATA_WIRE structure defines the size and offset of the variable-length data.
 *
 * \sa https://learn.microsoft.com/en-us/openspecs/windows_protocols/ms-tsts/93e88241-d30d-407b-832c-3d440026e6f2
 */
typedef struct _VARDATA_WIRE
{
    USHORT Size;
    USHORT Offset;
} VARDATA_WIRE, *PVARDATA_WIRE;

/**
 * The WINSTATIONSTATECLASS enumeration specifies the connection state of a session.
 */
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

/**
 * The WINSTATION_CURRENT_SESSION_CAPABILITY_CLASS enumeration identifies a capability of the current session.
 */
typedef enum _WINSTATION_CURRENT_SESSION_CAPABILITY_CLASS
{
    /** Indicates whether the current session is a Remote Desktop session. */
    WinStationCurrentSessionCapabilityRemoteDesktop = 1
} WINSTATION_CURRENT_SESSION_CAPABILITY_CLASS;

/**
 * The WINSTATION_ENFORCEMENT_CORE_INFORMATION_CLASS enumeration identifies licensing-enforcement information.
 */
typedef enum _WINSTATION_ENFORCEMENT_CORE_INFORMATION_CLASS
{
    /** Retrieves the identifier of the session currently under licensing arbitration, or ULONG_MAX if no session is under arbitration. */
    WinStationEnforcementCoreSessionUnderArbitration = 0,
    /** Retrieves the maximum number of Remote Desktop sessions allowed by the applicable policy. */
    WinStationEnforcementCoreMaximumSessions = 1
} WINSTATION_ENFORCEMENT_CORE_INFORMATION_CLASS;

/**
 * The SESSIONIDW structure contains information about a session.
 */
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

typedef struct _SESSIONIDA
{
    union
    {
        ULONG SessionId;
        ULONG LogonId;
    };
    WINSTATIONNAMEA WinStationName;
    WINSTATIONSTATECLASS State;
} SESSIONIDA, *PSESSIONIDA;

/**
 * The WINSTATION_PROPERTY_VALUE_TYPE enumeration identifies the representation of a connection property value.
 */
typedef enum _WINSTATION_PROPERTY_VALUE_TYPE
{
    WinStationPropertyValueInvalid = 0,
    WinStationPropertyValueDword = 1,
    WinStationPropertyValueString = 2,
    WinStationPropertyValueBinary = 3,
    WinStationPropertyValueGuid = 4
} WINSTATION_PROPERTY_VALUE_TYPE;

/**
 * The WINSTATION_PROPERTY_VALUE structure contains a connection property value.
 */
typedef struct _WINSTATION_PROPERTY_VALUE
{
    USHORT Type;
    USHORT Reserved;
    union
    {
        ULONG Dword;
        struct
        {
            ULONG Length;
            PWSTR Buffer;
        } String;
        struct
        {
            ULONG Length;
            PVOID Buffer;
        } Binary;
        GUID Guid;
    };
} WINSTATION_PROPERTY_VALUE, *PWINSTATION_PROPERTY_VALUE;

/**
 * The WINSTATION_USER_CERTIFICATE structure contains a user certificate returned by the Terminal Services service.
 */
typedef struct _WINSTATION_USER_CERTIFICATE
{
    ULONG Type;
    ULONG DataLength;
    PVOID Data;
} WINSTATION_USER_CERTIFICATE, *PWINSTATION_USER_CERTIFICATE;

// The complete CRED_PROV_CREDENTIAL definition is provided by usermgr.h.
typedef struct _CRED_PROV_CREDENTIAL CRED_PROV_CREDENTIAL, *PCRED_PROV_CREDENTIAL;

/**
 * The WINSTATIONINFOCLASS enumeration indicates the class of data for which to either query or set on the server.
 *
 * \sa https://learn.microsoft.com/en-us/openspecs/windows_protocols/ms-tsts/f333c223-de8a-46e1-a83e-79cbdab92371
 * \sa https://learn.microsoft.com/en-us/openspecs/windows_protocols/ms-tsts/1bba9ff2-71d3-49a3-bb26-2e5f6fcab3ee
 * \sa https://learn.microsoft.com/en-us/openspecs/windows_protocols/ms-tsts/2a5ee131-a1dd-44c7-9880-98df708061ea
 */
typedef enum _WINSTATIONINFOCLASS
{
    WinStationCreateData,                   // q: WINSTATIONCREATE
    WinStationConfiguration,                // qs: WINSTACONFIGWIRE + USERCONFIG
    WinStationPdParams,                     // qs: PDPARAMSWIRE + PDPARAMS
    WinStationWd,                           // q: WDCONFIG
    WinStationPd,                           // q: PDCONFIG2 + PDPARAMS
    WinStationPrinter,                      // qs: Not supported.
    WinStationClient,                       // q: VARDATA_WIRE + WINSTATIONCLIENT
    WinStationModules,                      // q: UCHAR[]
    WinStationInformation,                  // q: WINSTATIONINFORMATION
    WinStationTrace,                        // s: TS_TRACE
    WinStationBeep,                         // s: BEEPINPUT // 10
    WinStationEncryptionOff,                // s: VOID
    WinStationEncryptionPerm,               // s: VOID
    WinStationNtSecurity,                   // s: VOID (SessionId WINSTATION_CURRENT_SESSION)
    WinStationUserToken,                    // q: WINSTATIONUSERTOKEN
    WinStationUnused1,                      // qs: Not supported.
    WinStationVideoData,                    // q: WINSTATIONVIDEODATA
    WinStationInitialProgram,               // s: VOID (set current process as initial program)
    WinStationCd,                           // q: CDCONFIG
    WinStationSystemTrace,                  // s: TS_TRACE
    WinStationVirtualData,                  // q: UCHAR[] // 20
    WinStationClientData,                   // s: VARDATA_WIRE + WINSTATIONCLIENTDATA
    WinStationSecureDesktopEnter,           // qs: VOID
    WinStationSecureDesktopExit,            // qs: VOID
    WinStationLoadBalanceSessionTarget,     // q: ULONG
    WinStationLoadIndicator,                // q: WINSTATIONLOADINDICATORDATA
    WinStationShadowInfo,                   // qs: WINSTATIONSHADOW
    WinStationDigProductId,                 // q: WINSTATIONPRODID
    WinStationLockedState,                  // qs: BOOL
    WinStationRemoteAddress,                // q: WINSTATIONREMOTEADDRESS
    WinStationIdleTime,                     // q: ULONG // 30
    WinStationLastReconnectType,            // q: ULONG
    WinStationDisallowAutoReconnect,        // qs: BOOLEAN
    WinStationMprNotifyInfo,                // q: UCHAR[]
    WinStationExecSrvSystemPipe,            // q: WINSTATIONEXECSRVSYSTEMPIPE
    WinStationSmartCardAutoLogon,           // q: BOOLEAN
    WinStationIsAdminLoggedOn,              // q: BOOLEAN
    WinStationReconnectedFromId,            // q: ULONG
    WinStationEffectsPolicy,                // q: ULONG
    WinStationType,                         // q: ULONG
    WinStationInformationEx,                // q: VARDATA_WIRE + WINSTATIONINFORMATIONEX // 40
    WinStationValidationInfo,               // q: UCHAR[]
    WinStationActivityId,                   // q: GUID
    MaxWinStationInfoClass
} WINSTATIONINFOCLASS;

/**
 * Retrieves general information used to create the terminal server session (protocol) to which the station belongs.
 */
typedef struct _WINSTATIONCREATE
{
    ULONG fEnableWinStation : 1;
    ULONG MaxInstanceCount;
} WINSTATIONCREATE, *PWINSTATIONCREATE;

/**
 * The WINSTACONFIGWIRE structure defines the wire-format WinStation configuration data.
 *
 * \sa https://learn.microsoft.com/en-us/openspecs/windows_protocols/ms-tsts/7485038b-d5a2-4663-9fba-9646b971a80c
 */
typedef struct _WINSTACONFIGWIRE
{
    WCHAR Comment[61]; // The WinStation descriptive comment.
    CHAR OEMId[4]; // Value identifying the OEM implementer of the TermService Listener to which this session (WinStation) belongs. This can be any value defined by the implementer (OEM) of the listener.
    VARDATA_WIRE UserConfig; // VARDATA_WIRE structure defining the size and offset of the variable-length user configuration data succeeding it.
    VARDATA_WIRE NewFields; // VARDATA_WIRE structure defining the size and offset of the variable-length new data succeeding it. This field is not used and is a placeholder for any new data, if and when added.
} WINSTACONFIGWIRE, *PWINSTACONFIGWIRE;

/**
 * The CALLBACKCLASS enumeration specifies the callback configuration for a session.
 */
typedef enum _CALLBACKCLASS
{
    Callback_Disable,
    Callback_Roving,
    Callback_Fixed
} CALLBACKCLASS;

/**
 * The SHADOWCLASS enumeration is used to indicate the shadow-related settings for a session running on a terminal server.
 */
typedef enum _SHADOWCLASS
{
    Shadow_Disable, // Shadowing is disabled.
    Shadow_EnableInputNotify, // Permission is asked first from the session being shadowed. The shadower is also permitted keyboard and mouse input.
    Shadow_EnableInputNoNotify, // Permission is not asked first from the session being shadowed. The shadower is also permitted keyboard and mouse input.
    Shadow_EnableNoInputNotify, // Permission is asked first from the session being shadowed. The shadower is not permitted keyboard and mouse input and MUST observe the shadowed session.
    Shadow_EnableNoInputNoNotify // Permission is not asked first from the session being shadowed. The shadower is not permitted keyboard and mouse input and MUST observe the shadowed session.
} SHADOWCLASS;

/**
 * The USERCONFIG structure indicates the user and session configuration for a specific terminal server session.
 *
 * \sa https://learn.microsoft.com/en-us/openspecs/windows_protocols/ms-tsts/dba750b8-cb35-4e88-9811-e2a1f8a10701
 */
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

/**
 * The SDCLASS enumeration specifies the type of session device.
 */
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

/**
 * The NETWORKCONFIG structure defines the configuration for a network transport.
 */
typedef struct _NETWORKCONFIG
{
    LONG LanAdapter;
    DEVICENAME NetworkName;
    ULONG Flags;
} NETWORKCONFIG, *PNETWORKCONFIG;

/**
 * The FLOWCONTROLCLASS enumeration specifies the flow-control mode for an asynchronous transport.
 */
typedef enum _FLOWCONTROLCLASS
{
    FlowControl_None,
    FlowControl_Hardware,
    FlowControl_Software
} FLOWCONTROLCLASS;

/**
 * The RECEIVEFLOWCONTROLCLASS enumeration specifies the signal used for receive flow control.
 */
typedef enum _RECEIVEFLOWCONTROLCLASS
{
    ReceiveFlowControl_None,
    ReceiveFlowControl_RTS,
    ReceiveFlowControl_DTR,
} RECEIVEFLOWCONTROLCLASS;

/**
 * The TRANSMITFLOWCONTROLCLASS enumeration specifies the signal used for transmit flow control.
 */
typedef enum _TRANSMITFLOWCONTROLCLASS
{
    TransmitFlowControl_None,
    TransmitFlowControl_CTS,
    TransmitFlowControl_DSR,
} TRANSMITFLOWCONTROLCLASS;

/**
 * The ASYNCCONNECTCLASS enumeration specifies the condition used to establish an asynchronous connection.
 */
typedef enum _ASYNCCONNECTCLASS
{
    Connect_CTS,
    Connect_DSR,
    Connect_RI,
    Connect_DCD,
    Connect_FirstChar,
    Connect_Perm,
} ASYNCCONNECTCLASS;

/**
 * The FLOWCONTROLCONFIG structure defines the flow control configuration for an async transport.
 */
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

/**
 * The CONNECTCONFIG structure defines the connection configuration for an async transport.
 *
 * \sa https://learn.microsoft.com/en-us/openspecs/windows_protocols/ms-tsts/5169a8e0-1996-4767-a2f2-1f4a974052f7
 */
typedef struct _CONNECTCONFIG
{
    ASYNCCONNECTCLASS Type;
    ULONG fEnableBreakDisconnect : 1;
} CONNECTCONFIG, *PCONNECTCONFIG;

/**
 * The ASYNCCONFIG structure defines the configuration for an async transport.
 *
 * \sa https://learn.microsoft.com/en-us/openspecs/windows_protocols/ms-tsts/9d04f2cf-3687-4632-9c4c-201736762395
 */
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

/**
 * The NASICONFIG structure defines the configuration for a NetWare Asynchronous Services Interface (NASI) transport.
 *
 * \sa https://learn.microsoft.com/en-us/openspecs/windows_protocols/ms-tsts/03332168-d6ea-4395-9706-e7118ce5f2be
 */
typedef struct _NASICONFIG
{
    NASISPECIFICNAME SpecificName;
    NASIUSERNAME UserName;
    NASIPASSWORD PassWord;
    NASISESIONNAME SessionName;
    NASIFILESERVER FileServer;
    BOOLEAN GlobalSession;
} NASICONFIG, *PNASICONFIG;

/**
 * The OEMTDCONFIG structure defines the configuration for an OEM transport driver.
 *
 * \sa https://learn.microsoft.com/en-us/openspecs/windows_protocols/ms-tsts/60d78330-81f1-4df2-a9b0-951ee11b8b80
 */
typedef struct _OEMTDCONFIG
{
    LONG Adapter;
    DEVICENAME DeviceName;
    ULONG Flags;
} OEMTDCONFIG, *POEMTDCONFIG;

/**
 * The PDPARAMS structure represents the transport protocol driver parameters.
 *
 * \sa https://learn.microsoft.com/en-us/openspecs/windows_protocols/ms-tsts/ff0e2998-fb8b-4dff-ab64-8427fad556eb
 */
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

/**
 * The WDCONFIG structure represents the WinStation (session) driver configuration.
 *
 * \sa https://learn.microsoft.com/en-us/openspecs/windows_protocols/ms-tsts/d1bf099b-eb54-4ed9-a723-0b1062dbc128
 */
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

/**
 * The PDCONFIG2 structure represents the protocol driver's software configuration.
 *
 * \sa https://learn.microsoft.com/en-us/openspecs/windows_protocols/ms-tsts/74204022-eb7c-4454-b3d0-24f642c892a4
 */
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

/**
 * The WINSTATIONCLIENT structure defines the client-requested configuration when connecting to a session.
 *
 * \sa https://learn.microsoft.com/en-us/openspecs/windows_protocols/ms-tsts/673d8ac0-f557-48cb-98a6-49925160d729
 */
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

/**
 * The TSHARE_COUNTERS structure is reserved.
 */
typedef struct _TSHARE_COUNTERS
{
    ULONG Reserved;
} TSHARE_COUNTERS, *PTSHARE_COUNTERS;

/**
 * The PROTOCOLCOUNTERS structure contains the counters for a protocol.
 *
 * \sa https://learn.microsoft.com/en-us/openspecs/windows_protocols/ms-tsts/921a5036-7c0b-4680-a94f-45cb1f1737e8
 */
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

/**
 * The THINWIRECACHE structure contains statistics for a thinwire cache.
 *
 * \sa https://learn.microsoft.com/en-us/openspecs/windows_protocols/ms-tsts/21eb52ec-8178-4392-8096-7c9803001f2e
 */
typedef struct _THINWIRECACHE
{
    ULONG CacheReads;
    ULONG CacheHits;
} THINWIRECACHE, *PTHINWIRECACHE;

#define MAX_THINWIRECACHE 4

/**
 * The RESERVED_CACHE structure contains an array of thinwire cache statistics.
 *
 * \sa https://learn.microsoft.com/en-us/openspecs/windows_protocols/ms-tsts/63e14457-372a-4661-8cf6-c30f40d3989c
 */
typedef struct _RESERVED_CACHE
{
    THINWIRECACHE ThinWireCache[MAX_THINWIRECACHE];
} RESERVED_CACHE, *PRESERVED_CACHE;

/**
 * The TSHARE_CACHE structure contains statistics for a TShare cache.
 *
 * \sa https://learn.microsoft.com/en-us/openspecs/windows_protocols/ms-tsts/d43f0692-263a-4a22-ae92-75ca335805e7
 */
typedef struct _TSHARE_CACHE
{
    ULONG Reserved;
} TSHARE_CACHE, *PTSHARE_CACHE;

/**
 * The CACHE_STATISTICS structure represents the cache statistics on the protocol.
 *
 * \sa https://learn.microsoft.com/en-us/openspecs/windows_protocols/ms-tsts/81203ca2-e58b-4681-affa-924e59671b5c
 */
typedef struct CACHE_STATISTICS
{
    USHORT ProtocolType;                        // Protocol type.
    USHORT Length;                              // Length of data in the protocol-specific area. Can be up to 20 * sizeof(ULONG) in size.
    union
    {
        RESERVED_CACHE ReservedCacheStats;      // Not used.
        TSHARE_CACHE TShareCacheStats;          // Protocol cache statistics.
        ULONG Reserved[20];                     // Reserved for future use.
    } Specific;
} CACHE_STATISTICS, *PCACHE_STATISTICS;

/**
 * The PROTOCOLSTATUS structure represents the status of the protocol used by the session.
 *
 * \sa https://learn.microsoft.com/en-us/openspecs/windows_protocols/ms-tsts/c9066753-acbd-4678-9a72-8fb1b080bd09
 */
typedef struct _PROTOCOLSTATUS
{
    PROTOCOLCOUNTERS Output;    // A PROTOCOLCOUNTERS structure containing the output protocol counters.
    PROTOCOLCOUNTERS Input;     // A PROTOCOLCOUNTERS structure containing the input protocol counters.
    CACHE_STATISTICS Cache;     // A CACHE_STATISTICS structure containing statistics for the cache.
    ULONG AsyncSignal;          // Indicator of async signal, such as MS_CTS_ON, for async protocols.
    ULONG AsyncSignalMask;      // Mask of async signal events, such as EV_CTS, for async protocols.
} PROTOCOLSTATUS, *PPROTOCOLSTATUS;

/**
 * The WINSTATIONINFORMATION structure retrieves the state, connect time, last input time, and so on, for a session.
 *
 * \sa https://learn.microsoft.com/en-us/openspecs/windows_protocols/ms-tsts/c2566d8b-7016-440b-b7e0-0d07c3b2418f
 */
typedef struct _WINSTATIONINFORMATION
{
    WINSTATIONSTATECLASS ConnectState;      // The current connect state of the session.
    WINSTATIONNAME WinStationName;          // The name of the session.
    ULONG LogonId;                          // The session identifier of the session.
    LARGE_INTEGER ConnectTime;              // The time of the most recent connection to the session.
    LARGE_INTEGER DisconnectTime;           // The time of the most recent disconnection from the session.
    LARGE_INTEGER LastInputTime;            // The time the session last received input.
    LARGE_INTEGER LogonTime;                // The time of the logon to the session.
    PROTOCOLSTATUS Status;                  // The status of the protocol.
    WCHAR Domain[DOMAIN_LENGTH + 1];        // The user's domain name.
    WCHAR UserName[USERNAME_LENGTH + 1];    // The user's user name.
    LARGE_INTEGER CurrentTime;              // The current time in the session.
} WINSTATIONINFORMATION, *PWINSTATIONINFORMATION;

/**
 * The WINSTATION_SESSION_INFO_1W structure contains level-1 information for a session returned by WinStationGetAllSessionsW.
 */
typedef struct _WINSTATION_SESSION_INFO_1W
{
    ULONG ExecEnvId;
    WINSTATIONSTATECLASS State;
    WINSTATIONNAME WinStationName;
    ULONG SessionId;
    WCHAR HostName[WINSTATIONNAME_LENGTH + 1];
    WCHAR UserName[WINSTATIONNAME_LENGTH + 1];
    WCHAR DomainName[WINSTATIONNAME_LENGTH + 1];
    WCHAR FarmName[WINSTATIONNAME_LENGTH + 1];
} WINSTATION_SESSION_INFO_1W, *PWINSTATION_SESSION_INFO_1W;

/**
 * The WINSTATION_SESSION_INFO_EXW structure contains a fixed-size extended Unicode session record returned by WinStationEnumerateExW.
 *
 * Fields whose semantics are not exposed by the current client wrapper retain neutral names; their offsets and sizes are part of the verified ABI.
 */
typedef struct _WINSTATION_SESSION_INFO_EXW
{
    ULONG Value0;
    ULONG Value4;
    ULONG Value8;
    UCHAR ValueC;
    UCHAR ReservedD[3];
    UCHAR Value10[16];
    WCHAR WinStationName[WINSTATIONNAME_LENGTH + 1];
    USHORT Reserved62;
    UCHAR Level3Data[0x3F4];
} WINSTATION_SESSION_INFO_EXW, *PWINSTATION_SESSION_INFO_EXW;

C_ASSERT(sizeof(WINSTATION_SESSION_INFO_EXW) == 0x458);

/**
 * The TS_TRACE structure specifies fields used for configuring tracing operations in TS binaries if they are checked.
 *
 * \sa https://learn.microsoft.com/en-us/openspecs/windows_protocols/ms-tsts/ad349575-5369-4830-a174-1920a7af8d5f
 */
typedef struct _TS_TRACE
{
    WCHAR TraceFile[256];   // Specifies the file name, if any, to which to write debug information.
    BOOLEAN Debugger;       // Specifies whether debugger is attached.
    BOOLEAN Timestamp;      // Specifies whether to append time stamp to the traces logged.
    ULONG TraceClass;       // Classes of tracing to log. They enable tracing for the various terminal server binaries/functionalities.
    ULONG TraceEnable;      // Type of tracing calls log.
    WCHAR TraceOption[64];  // Trace option string. This can be an empty string meaning collect trace for all files and lines in those files.
} TS_TRACE, *PTS_TRACE;

/**
 * The BEEPINPUT structure performs a beep in the session.
 *
 * \sa https://learn.microsoft.com/en-us/openspecs/windows_protocols/ms-tsts/2faf4c5b-3a79-491a-9d1a-145f46a797d1
 */
typedef struct _BEEPINPUT
{
    /**
     * If the session ID is 0, this can be any of the values that can be passed to the standard MessageBeep function.
     * If the session ID is not 0, a frequency and duration is chosen by the server to send as a beep to the session.
     */
    ULONG Type;
} BEEPINPUT, *PBEEPINPUT;

/**
 * The WINSTATIONUSERTOKEN structure retrieves the user's token for the session.
 *
 * \remarks Caller requires WINSTATION_ALL_ACCESS permission.
 * \sa https://learn.microsoft.com/en-us/openspecs/windows_protocols/ms-tsts/07f9831c-6331-43e5-ba27-d3d58772eb4c
 */
typedef struct _WINSTATIONUSERTOKEN
{
    HANDLE ProcessId;   // Specifies the Process ID.
    HANDLE ThreadId;    // Specifies the calling thread.
    HANDLE UserToken;   // Returns the user token that is currently logged on to the session.
} WINSTATIONUSERTOKEN, *PWINSTATIONUSERTOKEN;

/**
 * The WINSTATIONVIDEODATA structure defines the resolution and color depth of a session.
 *
 * \sa https://learn.microsoft.com/en-us/openspecs/windows_protocols/ms-tsts/5f95f657-89d2-472d-b4ab-b0595618dbd1
 */
typedef struct _WINSTATIONVIDEODATA
{
    USHORT HResolution; // Specifies the horizontal resolution, in pixels.
    USHORT VResolution; // Specifies the vertical resolution, in pixels.
    USHORT ColorDepth;  // Specifies the color depth.
} WINSTATIONVIDEODATA, *PWINSTATIONVIDEODATA;

/**
 * The CDCLASS enumeration specifies the type of connection driver.
 */
typedef enum _CDCLASS
{
    CdNone, // No connection driver.
    CdModem, // Connection driver is a modem.
    CdClass_Maximum,
} CDCLASS;

/**
 * The CDCONFIG structure defines the configuration used for connecting via modem to a server.
 *
 * \sa https://learn.microsoft.com/en-us/openspecs/windows_protocols/ms-tsts/f88900e1-c159-4f02-b3ae-84f05eec212f
 */
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

/**
 * The WINSTATIONCLIENTDATA structure defines the client data for a session.
 *
 * \sa https://learn.microsoft.com/en-us/openspecs/windows_protocols/ms-tsts/89006ca0-681b-43a9-92c2-843813898144
 */
typedef struct _WINSTATIONCLIENTDATA
{
    CLIENTDATANAME DataName; // Identifies the type of data sent in this WINSTATIONCLIENTDATA structure.
    BOOLEAN fUnicodeData; // TRUE indicates data is in Unicode format; FALSE otherwise.
} WINSTATIONCLIENTDATA, *PWINSTATIONCLIENTDATA;

/**
 * The LOADFACTORTYPE enumeration identifies the resource constraint used to calculate a server load factor.
 */
typedef enum _LOADFACTORTYPE
{
    ErrorConstraint, // An error occurred while obtaining constraint data.
    PagedPoolConstraint, // The amount of paged pool is the constraint.
    NonPagedPoolConstraint, // The amount of non-paged pool is the constraint.
    AvailablePagesConstraint, // The amount of available pages is the constraint.
    SystemPtesConstraint, // The number of system page table entries (PTEs) is the constraint.
    CPUConstraint // CPU usage is the constraint.
} LOADFACTORTYPE;

/**
 * The WINSTATIONLOADINDICATORDATA structure defines data used for the load balancing of a server.
 *
 * \sa https://learn.microsoft.com/en-us/openspecs/windows_protocols/ms-tsts/91459fa1-77e8-4987-a6f0-fe7dd3e62bfc
 */
typedef struct _WINSTATIONLOADINDICATORDATA
{
    ULONG RemainingSessionCapacity;     // The estimated number of additional sessions that can be supported given the CPU constraint.
    LOADFACTORTYPE LoadFactor;          // Indicates the most constrained current resource.
    ULONG TotalSessions;                // The total number of sessions.
    ULONG DisconnectedSessions;         // The number of disconnected sessions.
    LARGE_INTEGER IdleCPU;              // This is always set to 0.
    LARGE_INTEGER TotalCPU;             // This is always set to 0.
    ULONG RawSessionCapacity;           // The raw number of sessions capacity.
    ULONG reserved[9];                  // Reserved.
} WINSTATIONLOADINDICATORDATA, *PWINSTATIONLOADINDICATORDATA;

/**
 * The SHADOWSTATECLASS enumeration specifies WinStation shadow states.
 *
 * \sa https://learn.microsoft.com/en-us/openspecs/windows_protocols/ms-tsts/c55fbd8f-d7e3-4efe-9ca6-d0985dab9602
 */
typedef enum _SHADOWSTATECLASS
{
    State_NoShadow, // No shadow operations are currently being performed on this session.
    State_Shadowing, // The session is shadowing a different session. The current session is referred to as a shadow client.
    State_Shadowed // The session is being shadowed by a different session. The current session is referred to as a shadow target.
} SHADOWSTATECLASS;

#define PROTOCOL_CONSOLE 0
#define PROTOCOL_OTHERS 1
#define PROTOCOL_RDP 2

/**
 * The WINSTATIONSHADOW structure retrieves the current shadow state of a session.
 *
 * \sa https://learn.microsoft.com/en-us/openspecs/windows_protocols/ms-tsts/d9d020ca-a979-45e0-9ccb-3f044d18084a
 */
typedef struct _WINSTATIONSHADOW
{
    SHADOWSTATECLASS ShadowState; // Specifies the current state of shadowing.
    SHADOWCLASS ShadowClass; // Specifies the type of shadowing.
    ULONG SessionId; // Specifies the session ID of the session.
    ULONG ProtocolType; // Specifies the type of protocol on the session. Can be one of PROTOCOL_* values.
} WINSTATIONSHADOW, *PWINSTATIONSHADOW;

/**
 * The WINSTATIONPRODID structure retrieves the client product ID and current product ID of the session.
 *
 * \sa https://learn.microsoft.com/en-us/openspecs/windows_protocols/ms-tsts/95c517f6-0428-4f81-ba5d-e0ec4e427771
 */
typedef struct _WINSTATIONPRODID
{
    WCHAR DigProductId[CLIENT_PRODUCT_ID_LENGTH];
    WCHAR ClientDigProductId[CLIENT_PRODUCT_ID_LENGTH];
    WCHAR OuterMostDigProductId[CLIENT_PRODUCT_ID_LENGTH];
    ULONG CurrentSessionId;
    ULONG ClientSessionId;
    ULONG OuterMostSessionId;
} WINSTATIONPRODID, *PWINSTATIONPRODID;

/**
 * The WINSTATIONREMOTEADDRESS structure retrieves the remote IP address of the terminal server client in the session.
 *
 * \sa https://learn.microsoft.com/en-us/openspecs/windows_protocols/ms-tsts/f79be277-2f34-4340-9b37-23b08e2f070b
 */
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

/**
 * The WINSTATIONEXECSRVSYSTEMPIPE structure defines the pipe name used for executing a process in a session.
 *
 * \sa https://learn.microsoft.com/en-us/openspecs/windows_protocols/ms-tsts/51e3f898-75b9-4a4b-974a-4648cc8187e1
 */
typedef struct _WINSTATIONEXECSRVSYSTEMPIPE
{
    WCHAR PipeName[48];
} WINSTATIONEXECSRVSYSTEMPIPE, *PWINSTATIONEXECSRVSYSTEMPIPE;

//
// WinStationInformationEx
//

/**
 * The WINSTATIONINFORMATIONEX_LEVEL1 structure contains extended information about a session (Level 1).
 *
 * \sa https://learn.microsoft.com/en-us/openspecs/windows_protocols/ms-tsts/93e15720-3367-4638-842f-87d558d7124f
 */
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

/**
 * The WINSTATIONINFORMATIONEX_LEVEL2 structure contains extended information about a session (Level 2).
 *
 * \sa https://learn.microsoft.com/en-us/openspecs/windows_protocols/ms-tsts/c2642a8b-3e5e-440f-90e8-07b469903b41
 */
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

/**
 * The WINSTATIONINFORMATIONEX_LEVEL union contains extended information about a session for various levels.
 */
typedef union _WINSTATIONINFORMATIONEX_LEVEL
{
    WINSTATIONINFORMATIONEX_LEVEL1 WinStationInfoExLevel1;
    WINSTATIONINFORMATIONEX_LEVEL2 WinStationInfoExLevel2;
} WINSTATIONINFORMATIONEX_LEVEL, *PWINSTATIONINFORMATIONEX_LEVEL;

/**
 * The WINSTATIONINFORMATIONEX structure defines the extended information for a session.
 *
 * \sa https://learn.microsoft.com/en-us/openspecs/windows_protocols/ms-tsts/3b334d70-3d7f-43b5-9f5b-10f54519504a
 */
typedef struct _WINSTATIONINFORMATIONEX
{
    ULONG Level;
    WINSTATIONINFORMATIONEX_LEVEL Data;
} WINSTATIONINFORMATIONEX, *PWINSTATIONINFORMATIONEX;

#define TS_PROCESS_INFO_MAGIC_NT4 0x23495452

/**
 * The TS_PROCESS_INFORMATION_NT4 structure contains process information for NT4 terminal server.
 */
typedef struct _TS_PROCESS_INFORMATION_NT4
{
    ULONG MagicNumber;
    ULONG LogonId;
    PSID ProcessSid;
    ULONG Pad;
} TS_PROCESS_INFORMATION_NT4, *PTS_PROCESS_INFORMATION_NT4;

#define SIZEOF_TS4_SYSTEM_THREAD_INFORMATION 64
#define SIZEOF_TS4_SYSTEM_PROCESS_INFORMATION 136

/**
 * The TS_SYS_PROCESS_INFORMATION structure contains information about a process in a terminal server session.
 * This structure is similar to SYSTEM_PROCESS_INFORMATION.
 */
_Struct_size_bytes_(NextEntryOffset)
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

/**
 * The TS_ALL_PROCESSES_INFO structure contains a list of processes in a terminal server session.
 */
typedef struct _TS_ALL_PROCESSES_INFO
{
    PTS_SYS_PROCESS_INFORMATION TsProcessInfo;
    ULONG SizeOfSid;
    PSID Sid;
} TS_ALL_PROCESSES_INFO, *PTS_ALL_PROCESSES_INFO;

/**
 * The TS_COUNTER_HEADER structure defines the header for a terminal services counter.
 *
 * \sa https://learn.microsoft.com/en-us/openspecs/windows_protocols/ms-tsts/15f606e7-1473-4f9e-8686-35c9a721df26
 */
typedef struct _TS_COUNTER_HEADER
{
    ULONG CounterID;
    BOOLEAN Result;
} TS_COUNTER_HEADER, *PTS_COUNTER_HEADER;

/**
 * The TS_COUNTER structure defines a terminal services counter.
 *
 * \sa https://learn.microsoft.com/en-us/openspecs/windows_protocols/ms-tsts/38c35d96-b04f-4638-963b-638062828694
 */
typedef struct _TS_COUNTER
{
    TS_COUNTER_HEADER CounterHead;
    ULONG Value;
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
/**
 * WNOTIFY_THIS_SESSION
 *
 * Specifies that only session notifications involving the session attached
 * to by the window identified by the WindowHandle are to be received.
 */
#define WNOTIFY_THIS_SESSION 0x0
/**
 * WNOTIFY_ALL_SESSIONS
 *
 * Specifies that all session notifications are to be received.
 */
#define WNOTIFY_ALL_SESSIONS 0x1
// end_rev

// begin_rev
// NotificationMask flags for WinStationRegisterConsoleNotificationEx(2) and
// WinStationRegisterSessionNotificationEx. Each selected bit causes a
// WM_WTSSESSION_CHANGE (0x02B1) message to be posted with the corresponding
// WTS_* status code as wParam. Specify WNOTIFY_MASK_ALL (-1) for every event.
// App-container callers are restricted to (WNOTIFY_MASK_CONSOLE_CONNECT |
// WNOTIFY_MASK_CONSOLE_DISCONNECT | WNOTIFY_MASK_REMOTE_DISCONNECT |
// WNOTIFY_MASK_REMOTE_CONNECT).
#define WNOTIFY_MASK_SESSION_CREATE 0x0001    // WTS_SESSION_CREATE (10)
#define WNOTIFY_MASK_REMOTE_CONNECT 0x0002    // WTS_REMOTE_CONNECT (3)
#define WNOTIFY_MASK_REMOTE_DISCONNECT 0x0004 // WTS_REMOTE_DISCONNECT (4)
#define WNOTIFY_MASK_SESSION_LOGON 0x0008     // WTS_SESSION_LOGON (5)
#define WNOTIFY_MASK_SESSION_LOGOFF 0x0010    // WTS_SESSION_LOGOFF (6)
#define WNOTIFY_MASK_REMOTE_CONTROL 0x0020    // WTS_SESSION_REMOTE_CONTROL (9)
#define WNOTIFY_MASK_REMOTE_CONTROL2 0x0040   // WTS_SESSION_REMOTE_CONTROL (9)
#define WNOTIFY_MASK_SESSION_TERMINATE 0x0080 // WTS_SESSION_TERMINATE (11)
#define WNOTIFY_MASK_CONSOLE_CONNECT 0x0100   // WTS_CONSOLE_CONNECT (1)
#define WNOTIFY_MASK_CONSOLE_DISCONNECT 0x0200 // WTS_CONSOLE_DISCONNECT (2)
#define WNOTIFY_MASK_SESSION_LOCK 0x0400      // WTS_SESSION_LOCK (7)
#define WNOTIFY_MASK_SESSION_UNLOCK 0x0800    // WTS_SESSION_UNLOCK (8)
#define WNOTIFY_MASK_SESSION_STATE 0x1000     // status code 15 (0xF)
#define WNOTIFY_MASK_ALL 0xffffffff
// end_rev

// In the functions below, memory returned can be freed using LocalFree. NULL can be specified for
// server handles to indicate the local server. -1 can be specified for session IDs to indicate the
// current session ID.

// rev
/**
 * The WinStationFreeMemory routine frees memory allocated by a Remote Desktop Services function.
 *
 * \param Buffer Pointer to the memory to free.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/wtsapi32/nf-wtsapi32-wtsfreememory
 */
NTSYSAPI
BOOLEAN
NTAPI
WinStationFreeMemory(
    _In_ PVOID Buffer
    );

// rev
/**
 * The WinStationOpenServerW routine opens a handle to the specified Remote Desktop Session Host (RD Session Host) server.
 *
 * \param ServerName Pointer to a null-terminated string specifying the NetBIOS name of the RD Session Host server.
 * \return HANDLE If the function succeeds, the return value is a handle to the specified server.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/wtsapi32/nf-wtsapi32-wtsopenserverw
 */
NTSYSAPI
HANDLE
NTAPI
WinStationOpenServerW(
    _In_opt_ PCWSTR ServerName
    );

// rev
/**
 * The WinStationOpenServerExW routine opens a handle to the specified Remote Desktop Session Host (RD Session Host) server
 * or Remote Desktop Virtualization Host (RD Virtualization Host) server.
 *
 * \param ServerName Pointer to a null-terminated string specifying the NetBIOS name of the RD Session Host server.
 * \return HANDLE If the function succeeds, the return value is a handle to the specified server.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/wtsapi32/nf-wtsapi32-wtsopenserverexw
 */
NTSYSAPI
HANDLE
NTAPI
WinStationOpenServerExW(
    _In_opt_ PCWSTR ServerName
    );

// rev
/**
 * The WinStationCloseServer routine closes an open handle to a Remote Desktop Session Host (RD Session Host) server.
 *
 * \param ServerHandle A handle to an RD Session Host server opened by a call to the WinStationOpenServerW function.
 * \return BOOLEAN Nonzero if the function succeeds, or zero otherwise. To get extended error information, call GetLastError.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/wtsapi32/nf-wtsapi32-wtscloseserver
 */
NTSYSAPI
BOOLEAN
NTAPI
WinStationCloseServer(
    _In_ HANDLE ServerHandle
    );

// rev
/**
 * Tests whether a Remote Desktop Session Host server is reachable and responding.
 *
 * \param ServerHandle A handle to an RD Session Host server, or WINSTATION_CURRENT_SERVER for the local server.
 * \return BOOLEAN Nonzero if the server responds successfully, or zero otherwise. To get extended error information, call GetLastError.
 */
NTSYSAPI
BOOLEAN
NTAPI
WinStationServerPing(
    _In_opt_ HANDLE ServerHandle
    );

// rev
/**
 * Retrieves terminal-services counter values from a Remote Desktop Session Host server.
 *
 * \param ServerHandle A handle to an RD Session Host server, or WINSTATION_CURRENT_SERVER for the local server.
 * \param Count The number of elements in the Counters array.
 * \param Counters An array of counters. On input, set the CounterID member of each element to the requested counter identifier. On output, each element receives its result status, value, and start time.
 * \return BOOLEAN Nonzero if the request succeeds, or zero otherwise. To get extended error information, call GetLastError.
 */
NTSYSAPI
BOOLEAN
NTAPI
WinStationGetTermSrvCountersValue(
    _In_opt_ HANDLE ServerHandle,
    _In_ ULONG Count,
    _Inout_updates_(Count) PTS_COUNTER Counters
    );

// rev
/**
 * The WinStationShutdownSystem routine shuts down (and optionally restarts) the specified Remote Desktop Session Host (RD Session Host) server.
 *
 * \param ServerHandle Handle to an RD Session Host server, or specify WINSTATION_CURRENT_SERVER to indicate the server on which your application is running.
 * \param ShutdownFlags Indicates the type of shutdown. This parameter can be one of the WSD_* values.
 * \return BOOLEAN Nonzero if the function succeeds, or zero otherwise. To get extended error information, call GetLastError.
 * \remarks To shut down or restart the system, the calling process must have the SE_SHUTDOWN_NAME privilege enabled.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/wtsapi32/nf-wtsapi32-wtsshutdownsystem
 */
NTSYSAPI
BOOLEAN
NTAPI
WinStationShutdownSystem(
    _In_opt_ HANDLE ServerHandle,
    _In_ ULONG ShutdownFlags // WSD_*
    );

// rev
/**
 * Waits for a terminal-services system event on a Remote Desktop Session Host server.
 *
 * \param ServerHandle A handle to an RD Session Host server, or WINSTATION_CURRENT_SERVER for the local server.
 * \param EventMask A bitwise combination of WEVENT_* values that identifies the events for which to wait.
 * \param EventFlags A pointer to a variable that receives the WEVENT_* flags for the events that occurred.
 * \return BOOLEAN Nonzero if the function succeeds, or zero otherwise. To get extended error information, call GetLastError.
 */
NTSYSAPI
BOOLEAN
NTAPI
WinStationWaitSystemEvent(
    _In_opt_ HANDLE ServerHandle,
    _In_ ULONG EventMask,
    _Out_ PULONG EventFlags
    );

// rev
/**
 * The WinStationRegisterConsoleNotification routine registers a window to receive session-change notifications.
 *
 * \param[in] ServerHandle A handle to the server. NULL specifies the current server.
 * \param[in] WindowHandle A handle to the window that receives session-change notifications.
 * \param[in] NotificationScope WNOTIFY_THIS_SESSION (0) for the calling process session, or WNOTIFY_ALL_SESSIONS (1) for all sessions.
 * \return Nonzero if the registration succeeds; otherwise, zero. To get extended error information, call GetLastError.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/wtsapi32/nf-wtsapi32-wtsregistersessionnotificationex
 */
NTSYSAPI
BOOLEAN
NTAPI
WinStationRegisterConsoleNotification(
    _In_opt_ HANDLE ServerHandle,
    _In_ HWND WindowHandle,
    _In_ ULONG NotificationScope
    );

// rev
/**
 * Unregisters a window from receiving session-change notifications from a Remote Desktop Session Host server.
 *
 * \param ServerHandle A handle to an RD Session Host server, or WINSTATION_CURRENT_SERVER for the local server.
 * \param WindowHandle A handle to the window that was registered by WinStationRegisterConsoleNotification.
 * \return BOOLEAN Nonzero if the function succeeds, or zero otherwise. To get extended error information, call GetLastError.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/wtsapi32/nf-wtsapi32-wtsunregistersessionnotificationex
 */
NTSYSAPI
BOOLEAN
NTAPI
WinStationUnRegisterConsoleNotification(
    _In_opt_ HANDLE ServerHandle,
    _In_ HWND WindowHandle
    );

// rev
/**
 * Enumerates the sessions on a Remote Desktop Session Host server.
 *
 * \param ServerHandle A handle to an RD Session Host server, or WINSTATION_CURRENT_SERVER for the local server.
 * \param SessionIds A pointer that receives an allocated array of SESSIONIDW structures.
 * \param Count A pointer to a variable that receives the number of elements in the returned array.
 * \return BOOLEAN Nonzero if the function succeeds, or zero otherwise. To get extended error information, call GetLastError.
 */
NTSYSAPI
BOOLEAN
NTAPI
WinStationEnumerateW(
    _In_opt_ HANDLE ServerHandle,
    _Out_ PSESSIONIDW *SessionIds,
    _Out_ PULONG Count
    );

/**
 * The WinStationQueryInformationW routine retrieves information about a window station.
 *
 * \param ServerHandle A handle to an RD Session Host server. Specify a handle opened by the WinStationOpenServerW function, or specify WINSTATION_CURRENT_SERVER to indicate the server on which your application is running.
 * \param SessionId A Remote Desktop Services session identifier.
 * To indicate the session in which the calling application is running (or the current session) specify WINSTATION_CURRENT_SESSION.
 * Only specify WINSTATION_CURRENT_SESSION when obtaining session information on the local server.
 * If WINSTATION_CURRENT_SESSION is specified when querying session information on a remote server, the returned session information will be inconsistent. Do not use the returned data.
 * \param WinStationInformationClass A value from the TOKEN_INFORMATION_CLASS enumerated type identifying the type of information to be retrieved.
 * \param WinStationInformation Pointer to a caller-allocated buffer that receives the requested information about the token.
 * \param WinStationInformationLength Length, in bytes, of the caller-allocated TokenInformation buffer.
 * \param ReturnLength Pointer to a caller-allocated variable that receives the actual length, in bytes, of the information returned in the TokenInformation buffer.
 * \return BOOLEAN Nonzero if the function succeeds, or zero otherwise. To get extended error information, call GetLastError.
 * \sa https://learn.microsoft.com/en-us/previous-versions/aa383827(v=vs.85)
 */
NTSYSAPI
BOOLEAN
NTAPI
WinStationQueryInformationW(
    _In_opt_ HANDLE ServerHandle,
    _In_ ULONG SessionId,
    _In_ WINSTATIONINFOCLASS WinStationInformationClass,
    _Out_writes_bytes_(WinStationInformationLength) PVOID WinStationInformation,
    _In_ ULONG WinStationInformationLength,
    _Out_ PULONG ReturnLength
    );

// rev
/**
 * Sets information for a session on a Remote Desktop Session Host server.
 *
 * \param ServerHandle A handle to an RD Session Host server, or WINSTATION_CURRENT_SERVER for the local server.
 * \param SessionId The identifier of the session to modify.
 * \param WinStationInformationClass The class of information to set.
 * \param WinStationInformation A pointer to a buffer containing the information to set. Its format depends on WinStationInformationClass.
 * \param WinStationInformationLength The size of the WinStationInformation buffer, in bytes.
 * \return BOOLEAN Nonzero if the function succeeds, or zero otherwise. To get extended error information, call GetLastError.
 */
NTSYSAPI
BOOLEAN
NTAPI
WinStationSetInformationW(
    _In_opt_ HANDLE ServerHandle,
    _In_ ULONG SessionId,
    _In_ WINSTATIONINFOCLASS WinStationInformationClass,
    _In_reads_bytes_(WinStationInformationLength) PVOID WinStationInformation,
    _In_ ULONG WinStationInformationLength
    );

// rev
/**
 * Retrieves information about the session associated with the calling process.
 *
 * \param WinStationInformationClass The class of information to retrieve.
 * \param WinStationInformation A caller-allocated buffer that receives the requested information. Its format depends on WinStationInformationClass.
 * \param WinStationInformationLength The size of the WinStationInformation buffer, in bytes.
 * \param ReturnLength A pointer to a variable that receives the number of bytes written or required.
 * \return BOOLEAN Nonzero if the function succeeds, or zero otherwise. To get extended error information, call GetLastError.
 */
NTSYSAPI
BOOLEAN
NTAPI
WinStationQueryCurrentSessionInformation(
    _In_ WINSTATIONINFOCLASS WinStationInformationClass,
    _Out_writes_bytes_(WinStationInformationLength) PVOID WinStationInformation,
    _In_ ULONG WinStationInformationLength,
    _Out_ PULONG ReturnLength
    );

/**
 * Retrieves the WinStation name associated with a session identifier.
 *
 * \param ServerHandle A handle to an RD Session Host server, or WINSTATION_CURRENT_SERVER for the local server.
 * \param SessionId The identifier of the session.
 * \param WinStationName A caller-allocated buffer that receives the null-terminated WinStation name.
 * \return BOOLEAN Nonzero if the function succeeds, or zero otherwise. To get extended error information, call GetLastError.
 */
NTSYSAPI
BOOLEAN
NTAPI
WinStationNameFromLogonIdW(
    _In_opt_ HANDLE ServerHandle,
    _In_ ULONG SessionId,
    _Out_writes_(WINSTATIONNAME_LENGTH + 1) PWSTR WinStationName
    );

// rev
/**
 * Retrieves the session identifier associated with a WinStation name.
 *
 * \param ServerHandle A handle to an RD Session Host server, or WINSTATION_CURRENT_SERVER for the local server.
 * \param WinStationName The null-terminated WinStation name.
 * \param SessionId A pointer to a variable that receives the session identifier.
 * \return BOOLEAN Nonzero if the function succeeds, or zero otherwise. To get extended error information, call GetLastError.
 */
NTSYSAPI
BOOLEAN
NTAPI
LogonIdFromWinStationNameW(
    _In_opt_ HANDLE ServerHandle,
    _In_ PCWSTR WinStationName,
    _Out_ PULONG SessionId
    );

// rev
/**
 * Displays a message box in a session on a Remote Desktop Session Host server.
 *
 * \param ServerHandle A handle to an RD Session Host server, or WINSTATION_CURRENT_SERVER for the local server.
 * \param SessionId The identifier of the session in which to display the message.
 * \param Title The message-box title.
 * \param TitleLength The length of Title, in bytes.
 * \param Message The message-box text.
 * \param MessageLength The length of Message, in bytes.
 * \param Style The message-box style flags.
 * \param Timeout The time, in seconds, before the message box times out.
 * \param Response A pointer to a variable that receives the user response.
 * \param DoNotWait TRUE to return without waiting for a response; otherwise, FALSE.
 * \return BOOLEAN Nonzero if the function succeeds, or zero otherwise. To get extended error information, call GetLastError.
 */
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

/**
 * Connects one session to another session on a Remote Desktop Session Host server.
 *
 * \param ServerHandle A handle to an RD Session Host server, or WINSTATION_CURRENT_SERVER for the local server.
 * \param SessionId The identifier of the source session.
 * \param TargetSessionId The identifier of the target session.
 * \param Password An optional password for the target session.
 * \param bWait TRUE to wait for the operation to complete; otherwise, FALSE.
 * \return BOOLEAN Nonzero if the function succeeds, or zero otherwise. To get extended error information, call GetLastError.
 */
NTSYSAPI
BOOLEAN
NTAPI
WinStationConnectW(
    _In_opt_ HANDLE ServerHandle,
    _In_ ULONG SessionId,
    _In_ ULONG TargetSessionId,
    _In_opt_ PCWSTR Password,
    _In_ BOOLEAN bWait
    );

/**
 * Disconnects a session from a Remote Desktop Session Host server.
 *
 * \param ServerHandle A handle to an RD Session Host server, or WINSTATION_CURRENT_SERVER for the local server.
 * \param SessionId The identifier of the session to disconnect.
 * \param bWait TRUE to wait for the operation to complete; otherwise, FALSE.
 * \return BOOLEAN Nonzero if the function succeeds, or zero otherwise. To get extended error information, call GetLastError.
 */
NTSYSAPI
BOOLEAN
NTAPI
WinStationDisconnect(
    _In_opt_ HANDLE ServerHandle,
    _In_ ULONG SessionId,
    _In_ BOOLEAN bWait
    );

// rev
/**
 * Resets a session on a Remote Desktop Session Host server.
 *
 * \param ServerHandle A handle to an RD Session Host server, or WINSTATION_CURRENT_SERVER for the local server.
 * \param SessionId The identifier of the session to reset.
 * \param bWait TRUE to wait for the operation to complete; otherwise, FALSE.
 * \return BOOLEAN Nonzero if the function succeeds, or zero otherwise. To get extended error information, call GetLastError.
 */
NTSYSAPI
BOOLEAN
NTAPI
WinStationReset(
    _In_opt_ HANDLE ServerHandle,
    _In_ ULONG SessionId,
    _In_ BOOLEAN bWait
    );

// rev
/**
 * Starts remote-control shadowing of a target session.
 *
 * \param ServerHandle A handle to an RD Session Host server, or WINSTATION_CURRENT_SERVER for the local server.
 * \param TargetServerName The name of the server that hosts the target session.
 * \param TargetSessionId The identifier of the session to shadow.
 * \param HotKeyVk The virtual-key code used to stop shadowing.
 * \param HotkeyModifiers A bitwise combination of KBD* modifier flags for the shadow-stop hot key.
 * \return BOOLEAN Nonzero if the function succeeds, or zero otherwise. To get extended error information, call GetLastError.
 */
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
/**
 * Stops remote-control shadowing for a session.
 *
 * \param ServerHandle A handle to an RD Session Host server, or WINSTATION_CURRENT_SERVER for the local server.
 * \param SessionId The identifier of the session for which to stop shadowing.
 * \param bWait Reserved. The current implementation ignores this parameter.
 * \return BOOLEAN Nonzero if the function succeeds, or zero otherwise. To get extended error information, call GetLastError.
 */
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
/**
 * Enumerates processes running in sessions on a Remote Desktop Session Host server.
 *
 * \param ServerHandle A handle to an RD Session Host server, or WINSTATION_CURRENT_SERVER for the local server.
 * \param Processes A pointer that receives an allocated process-information buffer.
 * \return BOOLEAN Nonzero if the function succeeds, or zero otherwise. To get extended error information, call GetLastError.
 */
NTSYSAPI
BOOLEAN
NTAPI
WinStationEnumerateProcesses(
    _In_opt_ HANDLE ServerHandle,
    _Out_ PVOID *Processes
    );

#define WINSTATION_PROCESS_LEVEL 0

// rev
/**
 * Retrieves extended information about processes running on a Remote Desktop Session Host server.
 *
 * \param ServerHandle A handle to an RD Session Host server, or WINSTATION_CURRENT_SERVER for the local server.
 * \param Level The information level of the requested process data.
 * \param NumberOfProcesses A pointer to a variable that receives the number of process records.
 * \param Processes A pointer that receives an allocated array of TS_ALL_PROCESSES_INFO structures.
 * \return BOOLEAN Nonzero if the function succeeds, or zero otherwise. To get extended error information, call GetLastError.
 */
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
/**
 * Frees process information returned by WinStationGetAllProcesses.
 *
 * \param Level The information level used to obtain the process data.
 * \param Processes The process-information array to free.
 * \param NumberOfProcesses The number of records in the process-information array.
 * \return BOOLEAN Nonzero if the memory is released successfully, or zero otherwise.
 */
NTSYSAPI
BOOLEAN
NTAPI
WinStationFreeGAPMemory(
    _In_ ULONG Level,
    _In_ PTS_ALL_PROCESSES_INFO Processes,
    _In_ ULONG NumberOfProcesses
    );

// rev
/**
 * Terminates a process on a Remote Desktop Session Host server.
 *
 * \param ServerHandle A handle to an RD Session Host server, or WINSTATION_CURRENT_SERVER for the local server.
 * \param ProcessId The identifier of the process to terminate.
 * \param ExitCode The exit code assigned to the terminated process.
 * \return BOOLEAN Nonzero if the function succeeds, or zero otherwise. To get extended error information, call GetLastError.
 */
NTSYSAPI
BOOLEAN
NTAPI
WinStationTerminateProcess(
    _In_opt_ HANDLE ServerHandle,
    _In_ ULONG ProcessId,
    _In_ ULONG ExitCode
    );

/**
 * The WinStationGetProcessSid routine retrieves the user SID for a process.
 *
 * \param[in] ServerHandle A handle to the server. NULL specifies the current server.
 * \param[in] ProcessId The process identifier.
 * \param[in] ProcessStartTime The process creation time used to identify the process instance.
 * \param[out] ProcessUserSid A caller-allocated buffer that receives the SID.
 * \param[in,out] SidSize On input, the size of the SID buffer, in bytes. On output, receives the required or written size.
 * \return Nonzero if the operation succeeds; otherwise, zero.
 */
NTSYSAPI
BOOLEAN
NTAPI
WinStationGetProcessSid(
    _In_opt_ HANDLE ServerHandle,
    _In_ ULONG ProcessId,
    _In_ FILETIME ProcessStartTime,
    _Out_writes_bytes_(*SidSize) PSID ProcessUserSid,
    _Inout_ PULONG SidSize
    );

//
// Services isolation
//

// rev
/**
 * Switches the interactive desktop to the services session.
 *
 * \return BOOLEAN Nonzero if the function succeeds, or zero otherwise. To get extended error information, call GetLastError.
 */
NTSYSAPI
BOOLEAN
NTAPI
WinStationSwitchToServicesSession(
    VOID
    );

// rev
/**
 * Reverts the interactive desktop from the services session.
 *
 * \return BOOLEAN Nonzero if the function succeeds, or zero otherwise. To get extended error information, call GetLastError.
 */
NTSYSAPI
BOOLEAN
NTAPI
WinStationRevertFromServicesSession(
    VOID
    );

// Misc.
/**
 * Provides the legacy WinStation wait-for-connect operation.
 *
 * \return BOOLEAN Nonzero if the function succeeds, or zero otherwise. To get extended error information, call GetLastError.
 */
NTSYSAPI
BOOLEAN
NTAPI
_WinStationWaitForConnect(
    VOID
    );

// rev
/**
 * Opens a virtual channel in a session.
 *
 * \param ServerHandle A handle to an RD Session Host server, or WINSTATION_CURRENT_SERVER for the local server.
 * \param SessionId The identifier of the session.
 * \param Name The ANSI name of the virtual channel.
 * \return HANDLE A handle to the virtual channel if the function succeeds, or NULL otherwise. To get extended error information, call GetLastError.
 */
NTSYSAPI
HANDLE
NTAPI
WinStationVirtualOpen(
    _In_opt_ HANDLE ServerHandle,
    _In_ ULONG SessionId,
    _In_ PCSTR Name
    );

// rev
/**
 * Opens a virtual channel in a session with additional channel options.
 *
 * \param[in] ServerHandle A handle to an RD Session Host server, or WINSTATION_CURRENT_SERVER for the local server.
 * \param[in] SessionId The identifier of the session.
 * \param[in] Name The ANSI name of the virtual channel.
 * \param[in] ChannelOptions Virtual-channel open options, using the same 32-bit option format as WTSVirtualChannelOpenEx.
 * \return A handle to the virtual channel if the function succeeds, or NULL otherwise. To get extended error information, call GetLastError.
 */
NTSYSAPI
HANDLE
NTAPI
WinStationVirtualOpenEx(
    _In_opt_ HANDLE ServerHandle,
    _In_ ULONG SessionId,
    _In_ PCSTR Name,
    _In_ ULONG ChannelOptions
    );

// rev
/**
 * Determines whether the current session can be remotely controlled.
 *
 * \param IsRemoteable A pointer to a variable that receives TRUE if the current session is remoteable; otherwise, FALSE.
 * \return BOOLEAN Nonzero if the function succeeds, or zero otherwise. To get extended error information, call GetLastError.
 */
NTSYSAPI
BOOLEAN
NTAPI
WinStationIsCurrentSessionRemoteable(
    _Out_ PBOOLEAN IsRemoteable
    );

EXTERN_C DECLSPEC_SELECTANY CONST GUID PROPERTY_TYPE_GET_MONITOR_CONFIG = { 0x865D5285, 0xF70A, 0x4ECF, { 0x8B, 0x28, 0x51, 0x2F, 0xE0, 0xAA, 0x2D, 0x53 } };
EXTERN_C DECLSPEC_SELECTANY CONST GUID PROPERTY_TYPE_CORRELATIONID_GUID = { 0x9A363F8E, 0x1902, 0x40DA, { 0xA2, 0xCC, 0x56, 0x4F, 0x09, 0x40, 0xAD, 0xE3 } };

/**
 * The TS_PROPERTY_INFORMATION structure contains information about a terminal services property.
 */
typedef struct _TS_PROPERTY_INFORMATION
{
    ULONG Length;
    PVOID Buffer;
} TS_PROPERTY_INFORMATION, *PTS_PROPERTY_INFORMATION;

// rev
/**
 * The WinStationGetConnectionProperty routine retrieves a connection property for a session.
 *
 * \param[in] SessionId The identifier of the session.
 * \param[in] PropertyType A pointer to the GUID that identifies the property.
 * \param[out] PropertyValue Receives a service-allocated property value. Free the value with WinStationFreePropertyValue.
 * \return Nonzero if the operation succeeds; otherwise, zero.
 */
NTSYSAPI
BOOLEAN
NTAPI
WinStationGetConnectionProperty(
    _In_ ULONG SessionId,
    _In_ PCGUID PropertyType,
    _Outptr_result_maybenull_ PWINSTATION_PROPERTY_VALUE *PropertyValue
    );

// rev
/**
 * The WinStationFreePropertyValue routine frees a property value returned by a connection-property query.
 *
 * \param[in] PropertyValue The property value to free.
 * \return Nonzero if the operation succeeds; otherwise, zero.
 */
NTSYSAPI
BOOLEAN
NTAPI
WinStationFreePropertyValue(
    _In_ PWINSTATION_PROPERTY_VALUE PropertyValue
    );

// rev
/**
 * Determines whether a specified session can be remotely controlled.
 *
 * \param ServerHandle A handle to an RD Session Host server, or WINSTATION_CURRENT_SERVER for the local server.
 * \param SessionId The identifier of the session.
 * \param IsRemote A pointer to a variable that receives TRUE if the session is remoteable; otherwise, FALSE.
 * \return BOOLEAN Nonzero if the function succeeds, or zero otherwise. To get extended error information, call GetLastError.
 */
NTSYSAPI
BOOLEAN
NTAPI
WinStationIsSessionRemoteable(
    _In_opt_ HANDLE ServerHandle,
    _In_ ULONG SessionId,
    _Out_ PBOOLEAN IsRemote
    );

// rev
/**
 * Stores an automatic-logon password for a WinStation configuration entry.
 *
 * \param KeyName The ANSI name of the configuration key.
 * \param Password The ANSI password to store.
 * \return BOOLEAN Nonzero if the function succeeds, or zero otherwise. To get extended error information, call GetLastError.
 */
NTSYSAPI
BOOLEAN
NTAPI
WinStationSetAutologonPassword(
    _In_ PCSTR KeyName,
    _In_ PCSTR Password
    );

/**
 * The EXECENVDATAEX_LEVEL1 structure contains information about an execution environment (Level 1).
 */
typedef struct _EXECENVDATAEX_LEVEL1
{
    LONG ExecEnvId;
    LONG State;
    LONG AbsSessionId;
    PWSTR SessionName;
    PWSTR HostName;
    PWSTR UserName;
    PWSTR DomainName;
    PWSTR FarmName;
} EXECENVDATAEX_LEVEL1, *PEXECENVDATAEX_LEVEL1;

/**
 * The EXECENVDATAEX_PAYLOAD union contains the payload for execution environment data.
 */
typedef union _EXECENVDATAEX_PAYLOAD
{
    UCHAR Data[1];
    EXECENVDATAEX_LEVEL1 Level1;
    // define level 2/3/4 here
} EXECENVDATAEX_PAYLOAD;

/**
 * The EXECENVDATAEX structure defines the execution environment data.
 */
typedef struct _EXECENVDATAEX
{
    ULONG Level;
    EXECENVDATAEX_PAYLOAD Payload;
} EXECENVDATAEX, *PEXECENVDATAEX;

// typedef struct _EXECENVDATAEX
// {
//     ULONG Level;
//     union
//     {
//         EXECENVDATAEX_LEVEL1 ExecEnvEnum_Level1;
//     };
// } EXECENVDATAEX, *PEXECENVDATAEX;

// rev
/**
 * Retrieves extended information for all sessions on a Remote Desktop Session Host server.
 *
 * \param ServerHandle A handle to an RD Session Host server, or WINSTATION_CURRENT_SERVER for the local server.
 * \param Level The requested information level. This parameter must be 1.
 * \param SessionData A pointer that receives an allocated array of EXECENVDATAEX structures.
 * \param Count A pointer to a variable that receives the number of session records.
 * \return BOOLEAN Nonzero if the function succeeds, or zero otherwise. To get extended error information, call GetLastError.
 */
NTSYSAPI
BOOLEAN
NTAPI
WinStationGetAllSessionsEx(
    _In_opt_ HANDLE ServerHandle,
    _In_ ULONG Level, // Must be 1
    _Out_ PEXECENVDATAEX *SessionData,
    _Out_ PULONG Count
    );

// rev
/**
 * Frees session information returned by WinStationGetAllSessionsEx.
 *
 * \param SessionData The session-information array to free.
 * \param Count The number of records in the session-information array.
 */
NTSYSAPI
VOID
WINAPI
WinStationFreeEXECENVDATAEX(
    _In_opt_ PEXECENVDATAEX SessionData,
    _In_ ULONG Count
    );

/**
 * The SESSIONTYPE enumeration identifies the purpose of a session.
 */
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

/**
 * The TS_USER_SESSION structure contains information about a user session.
 */
typedef struct _TS_USER_SESSION
{
    ULONG Version; // always 1
    ULONG SessionId;
    ULONG State; // WTS connect state (WTS_CONNECTSTATE_CLASS)
    SESSIONTYPE SessionType;
    ULONG ChildSessionId; // child session id (parent session id for RemoteApp/agent session types); 0xFFFFFFFF if none
} TS_USER_SESSION, *PTS_USER_SESSION;

// rev
/**
 * The WinStationGetAllUserSessions routine retrieves session records associated with a user security identifier.
 *
 * \param[in] ServerHandle A handle to an RD Session Host server, or WINSTATION_CURRENT_SERVER for the local server.
 * \param[in] Sid The security identifier of the user whose sessions are requested.
 * \param[out] Sessions Receives a LocalAlloc-allocated array of session records. Free the array with WinStationFreeUserSessionInfo or LocalFree.
 * \param[out] Count Receives the number of returned records.
 * \return Nonzero if the function succeeds; otherwise, zero. To get extended error information, call GetLastError.
 */
NTSYSAPI
BOOLEAN
NTAPI
WinStationGetAllUserSessions(
    _In_opt_ HANDLE ServerHandle,
    _In_ PSID Sid,
    _Outptr_result_buffer_(*Count) PTS_USER_SESSION *Sessions,
    _Out_ PULONG Count
    );

/**
 * The TS_SESSION_VIRTUAL_ADDRESS structure defines the virtual address for a session.
 */
typedef struct _TS_SESSION_VIRTUAL_ADDRESS
{
  USHORT AddressFamily;
  USHORT AddressLength;
  BYTE Address[20];
} TS_SESSION_VIRTUAL_ADDRESS, *PTS_SESSION_VIRTUAL_ADDRESS;
typedef USHORT ADDRESS_FAMILY;

// rev
/**
 * Retrieves the virtual IP address assigned to a session.
 *
 * \param ServerHandle A handle to an RD Session Host server, or WINSTATION_CURRENT_SERVER for the local server.
 * \param SessionId The identifier of the session.
 * \param Family The address family of the requested virtual address.
 * \param SessionVirtualIP A pointer to a TS_SESSION_VIRTUAL_ADDRESS structure that receives the address.
 * \return BOOLEAN Nonzero if the function succeeds, or zero otherwise. To get extended error information, call GetLastError.
 */
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
/**
 * Retrieves the device identifier associated with a session.
 *
 * \param ServerHandle A handle to an RD Session Host server, or WINSTATION_CURRENT_SERVER for the local server.
 * \param SessionId The identifier of the session.
 * \param Buffer A pointer that receives the device-identifier string buffer.
 * \param BufferLength The size of the device-identifier buffer, in bytes.
 * \return BOOLEAN Nonzero if the function succeeds, or zero otherwise. To get extended error information, call GetLastError.
 */
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
/**
 * Retrieves the numbers of logged-on users and devices.
 *
 * \param LoggedOnUserCount A pointer to a variable that receives the number of logged-on users.
 * \param LoggedOnDeviceCount A pointer to a variable that receives the number of logged-on devices.
 * \return BOOLEAN Nonzero if the function succeeds, or zero otherwise. To get extended error information, call GetLastError.
 */
NTSYSAPI
BOOLEAN
NTAPI
WinStationGetLoggedOnCount(
    _Out_ PULONG LoggedOnUserCount,
    _Out_ PULONG LoggedOnDeviceCount
    );

// rev
/**
 * The WinStationSetRenderHint routine is used by an application that is displaying content that
 * can be optimized for displaying in a remote session to identify the region of a window that is the actual content.
 * In the remote session, this content will be encoded, sent to the client, then decoded and displayed.
 *
 * \param[out] RenderHintID The address of a value that identifies the rendering hint affected by this call.
 * If a new hint is being created, this value must contain zero.
 * This function will return a unique rendering hint identifier which is used for subsequent calls, such as clearing the hint.
 * \param[in] WindowHandle The handle of window linked to lifetime of the rendering hint. This window is used in situations where a hint target is removed without the hint being explicitly cleared.
 * \param[in] RenderHintType Specifies the type of hint represented by this call.
 * \param[in] HintDataLength The size in bytes, of the HintData buffer.
 * \param[in] HintData Additional data for the hint. The format of this data is dependent upon the value passed in the renderHintType parameter.
 * \return BOOLEAN Nonzero if the function succeeds, or zero otherwise.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/wtshintapi/nf-wtshintapi-wtssetrenderhint
 */
NTSYSAPI
BOOLEAN
NTAPI
WinStationSetRenderHint(
    _In_opt_ PULONG64 RenderHintID,
    _In_ HWND WindowHandle,
    _In_ ULONG RenderHintType,
    _In_ ULONG HintDataLength,
    _In_ PBYTE HintData
    );

// rev
/**
 * The WinStationActiveSessionExists routine determines whether an active session exists without enumerating sessions or obtaining additional information from Local Session Manager.
 *
 * \param[out] ActiveSessionExists Receives TRUE if an active session exists; otherwise, FALSE.
 * \return Nonzero if the operation succeeds; otherwise, zero.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/wtsapi32/nf-wtsapi32-wtsactivesessionexists
 */
NTSYSAPI
BOOLEAN
NTAPI
WinStationActiveSessionExists(
    _Out_ PBOOL ActiveSessionExists
    );

typedef PVOID HSERVERLICENSING;

/**
 * The LCPOLICYINFOGENERICA structure contains license policy information (ANSI).
 */
typedef struct _LCPOLICYINFOGENERICA
{
    ULONG Version;
    PSTR Name;
    PSTR Value;
} LCPOLICYINFOGENERICA, *PLCPOLICYINFOGENERICA;

/**
 * The LCPOLICYINFOGENERICW structure contains license policy information (Unicode).
 */
typedef struct _LCPOLICYINFOGENERICW
{
    ULONG Version;
    PWSTR Name;
    PWSTR Value;
} LCPOLICYINFOGENERICW, *PLCPOLICYINFOGENERICW;

/**
 * The SERVERLICENSING_AADINFOW structure contains AAD info for server licensing.
 */
typedef struct _SERVERLICENSING_AADINFOW
{
    PWSTR TenantId; // The tenant identifier.
    PWSTR Token; // The token.
    ULONG Data; // Data.
} SERVERLICENSING_AADINFOW, *PSERVERLICENSING_AADINFOW;

// Export declarations recovered from the current winsta.dll export table.
// Deprecated compatibility exports retain their historical ABI; opaque legacy
// payloads are intentionally typed as PVOID where neither side dereferences them.

/**
 * The LogonIdFromWinStationNameA routine is a deprecated entry point for resolving a session identifier from an ANSI WinStation name.
 *
 * \param[in] ServerHandle A handle to the server. NULL specifies the current server.
 * \param[in] WinStationName The win station name.
 * \param[out] SessionId The session identifier.
 * \return Nonzero if the operation succeeds; otherwise, zero.
 * \remarks The current implementation does not perform the operation; it sets the last-error code to ERROR_INVALID_FUNCTION and returns FALSE.
 */
DECLSPEC_DEPRECATED
NTSYSAPI
BOOLEAN
NTAPI
LogonIdFromWinStationNameA(
    _In_opt_ HANDLE ServerHandle,
    _In_ PCSTR WinStationName,
    _Out_ PULONG SessionId
    );

/**
 * The RemoteAssistancePrepareSystemRestore routine is a deprecated entry point for preparing System Restore for a Remote Assistance operation.
 *
 * \param[in] RestoreInformation The restore information.
 * \return Nonzero if the operation succeeds; otherwise, zero.
 * \remarks The current implementation does not perform the operation; it sets the last-error code to ERROR_INVALID_FUNCTION and returns FALSE.
 */
NTSYSAPI
BOOLEAN
NTAPI
RemoteAssistancePrepareSystemRestore(
    _In_opt_ PVOID RestoreInformation
    );

/**
 * The ServerGetInternetConnectorStatus routine is a deprecated entry point for retrieving Internet Connector licensing status.
 *
 * \param[in] ServerHandle A handle to the server. NULL specifies the current server.
 * \param[out] Enabled Receives the enabled.
 * \return Nonzero if the operation succeeds; otherwise, zero.
 * \remarks The current implementation does not perform the operation; it sets the last-error code to ERROR_INVALID_FUNCTION and returns FALSE.
 */
DECLSPEC_DEPRECATED
NTSYSAPI
BOOLEAN
NTAPI
ServerGetInternetConnectorStatus(
    _In_opt_ HANDLE ServerHandle,
    _Out_opt_ PBOOLEAN Enabled
    );

/**
 * The ServerLicensingClose routine closes a server-licensing context and releases its resources.
 *
 * \param[in] LicensingHandle A handle to the server licensing context.
 */
NTSYSAPI
VOID
NTAPI
ServerLicensingClose(
    _In_opt_ HSERVERLICENSING LicensingHandle
    );

/**
 * The ServerLicensingDeactivateCurrentPolicy routine deactivates the policy currently active in a server-licensing context.
 *
 * \param[in] LicensingHandle A handle to the server licensing context.
 * \return Nonzero if the operation succeeds; otherwise, zero.
 */
NTSYSAPI
BOOLEAN
NTAPI
ServerLicensingDeactivateCurrentPolicy(
    _In_opt_ HSERVERLICENSING LicensingHandle
    );

/**
 * The ServerLicensingFreePolicyInformation routine frees policy information returned by a server-licensing query and clears the caller's pointer.
 *
 * \param[in,out] PolicyInformation Supplies the initial policy information and receives the updated value.
 */
NTSYSAPI
VOID
NTAPI
ServerLicensingFreePolicyInformation(
    _Inout_opt_ PLCPOLICYINFOGENERICW *PolicyInformation
    );

/**
 * The ServerLicensingGetAadInfo routine retrieves Microsoft Entra ID licensing information from a server-licensing context.
 *
 * \param[in] LicensingHandle A handle to the server licensing context.
 * \param[out] TenantId Receives the tenant id.
 * \param[out] Token Receives the token.
 * \param[out] Data Receives the data.
 * \return An HRESULT value indicating the result of the operation.
 */
NTSYSAPI
HRESULT
NTAPI
ServerLicensingGetAadInfo(
    _In_opt_ HSERVERLICENSING LicensingHandle,
    _Outptr_result_maybenull_ PWSTR *TenantId,
    _Outptr_result_maybenull_ PWSTR *Token,
    _Out_opt_ PULONG Data
    );

/**
 * The ServerLicensingGetAvailablePolicyIds routine retrieves the identifiers of policies available from a server-licensing context.
 *
 * \param[in] LicensingHandle A handle to the server licensing context.
 * \param[out] PolicyIds Receives the policy ids.
 * \param[out] PolicyCount Receives the policy count.
 * \return Nonzero if the operation succeeds; otherwise, zero.
 */
NTSYSAPI
BOOLEAN
NTAPI
ServerLicensingGetAvailablePolicyIds(
    _In_opt_ HSERVERLICENSING LicensingHandle,
    _Outptr_result_buffer_(*PolicyCount) PULONG *PolicyIds,
    _Out_ PULONG PolicyCount
    );

/**
 * The ServerLicensingGetPolicy routine retrieves policy data from a server-licensing context.
 *
 * \param[in] LicensingHandle A handle to the server licensing context.
 * \param[in] PolicyId An optional pointer to the policy GUID. If NULL, the built-in GUID for the information class is used.
 * \return Nonzero if the operation succeeds; otherwise, zero.
 */
NTSYSAPI
BOOLEAN
NTAPI
ServerLicensingGetPolicy(
    _In_opt_ HSERVERLICENSING LicensingHandle,
    _In_ ULONG PolicyId
    );

/**
 * The ServerLicensingGetPolicyInformationA routine retrieves ANSI policy information from a server-licensing context.
 *
 * \param[in] LicensingHandle A handle to the server licensing context.
 * \param[in] PolicyId The numeric identifier of the policy to query.
 * \param[in,out] PolicyInformationVersion On input, the requested policy-information format version (0 or 1); on output, the version used by the service. Values greater than 1 are reduced to 1.
 * \param[out] PolicyInformation Receives the policy information.
 * \return Nonzero if the operation succeeds; otherwise, zero.
 */
NTSYSAPI
BOOLEAN
NTAPI
ServerLicensingGetPolicyInformationA(
    _In_opt_ HSERVERLICENSING LicensingHandle,
    _In_ ULONG PolicyId,
    _Inout_ PULONG PolicyInformationVersion,
    _Outptr_result_maybenull_ PLCPOLICYINFOGENERICA *PolicyInformation
    );

/**
 * The ServerLicensingGetPolicyInformationW routine retrieves Unicode policy information from a server-licensing context.
 *
 * \param[in] LicensingHandle A handle to the server licensing context.
 * \param[in] PolicyId The numeric identifier of the policy to query.
 * \param[in,out] PolicyInformationVersion On input, the requested policy-information format version (0 or 1); on output, the version used by the service. Values greater than 1 are reduced to 1.
 * \param[out] PolicyInformation Receives the policy information.
 * \return Nonzero if the operation succeeds; otherwise, zero.
 */
NTSYSAPI
BOOLEAN
NTAPI
ServerLicensingGetPolicyInformationW(
    _In_opt_ HSERVERLICENSING LicensingHandle,
    _In_ ULONG PolicyId,
    _Inout_ PULONG PolicyInformationVersion,
    _Outptr_result_maybenull_ PLCPOLICYINFOGENERICW *PolicyInformation
    );

/**
 * The ServerLicensingLoadPolicy routine provides an exported alias for a Terminal Services client operation.
 *
 * \return Nonzero if the operation succeeds; otherwise, zero.
 */
NTSYSAPI
BOOLEAN
NTAPI
ServerLicensingLoadPolicy(
    VOID
    );

/**
 * The ServerLicensingOpenA routine opens an ANSI server-licensing context for the specified server.
 *
 * \param[in] ServerName The server name.
 * \return A server licensing handle if the operation succeeds; otherwise, NULL.
 */
NTSYSAPI
HSERVERLICENSING
NTAPI
ServerLicensingOpenA(
    _In_opt_ PCSTR ServerName
    );

/**
 * The ServerLicensingOpenW routine opens a Unicode server-licensing context for the specified server.
 *
 * \return A server licensing handle if the operation succeeds; otherwise, NULL.
 */
NTSYSAPI
HSERVERLICENSING
NTAPI
ServerLicensingOpenW(
    VOID
    );

/**
 * The ServerLicensingSetAadInfo routine sets Microsoft Entra ID licensing information for a server-licensing context.
 *
 * \param[in] LicensingHandle A handle to the server licensing context.
 * \param[in] AadInfo The aad info.
 * \return An HRESULT value indicating the result of the operation.
 */
NTSYSAPI
HRESULT
NTAPI
ServerLicensingSetAadInfo(
    _In_opt_ HSERVERLICENSING LicensingHandle,
    _In_opt_ PSERVERLICENSING_AADINFOW AadInfo
    );

/**
 * The ServerLicensingSetPolicy routine sets policy data in a server-licensing context.
 *
 * \param[in] LicensingHandle A handle to the server licensing context.
 * \param[in] PolicyId An optional pointer to the policy GUID. If NULL, the built-in GUID for the information class is used.
 * \param[in] PolicyValue The policy value.
 * \return Nonzero if the operation succeeds; otherwise, zero.
 */
NTSYSAPI
BOOLEAN
NTAPI
ServerLicensingSetPolicy(
    _In_opt_ HSERVERLICENSING LicensingHandle,
    _In_ ULONG PolicyId,
    _In_ PULONG PolicyValue
    );

/**
 * The ServerLicensingUnloadPolicy routine provides an exported alias for a Terminal Services client operation.
 *
 * \return Nonzero if the operation succeeds; otherwise, zero.
 */
NTSYSAPI
BOOLEAN
NTAPI
ServerLicensingUnloadPolicy(
    VOID
    );

/**
 * The ServerQueryInetConnectorInformationA routine is a deprecated entry point for querying ANSI Internet Connector information.
 *
 * \param[in] ServerHandle A handle to the server. NULL specifies the current server.
 * \param[out] Information Receives the information.
 * \return Nonzero if the operation succeeds; otherwise, zero.
 * \remarks The current implementation does not perform the operation; it sets the last-error code to ERROR_INVALID_FUNCTION and returns FALSE.
 */
DECLSPEC_DEPRECATED
NTSYSAPI
BOOLEAN
NTAPI
ServerQueryInetConnectorInformationA(
    _In_opt_ HANDLE ServerHandle,
    _Out_opt_ PVOID Information
    );

/**
 * The ServerQueryInetConnectorInformationW routine is a deprecated entry point for querying Unicode Internet Connector information.
 *
 * \param[in] ServerHandle A handle to the server. NULL specifies the current server.
 * \param[out] Information Receives the information.
 * \return Nonzero if the operation succeeds; otherwise, zero.
 * \remarks The current implementation does not perform the operation; it sets the last-error code to ERROR_INVALID_FUNCTION and returns FALSE.
 */
DECLSPEC_DEPRECATED
NTSYSAPI
BOOLEAN
NTAPI
ServerQueryInetConnectorInformationW(
    _In_opt_ HANDLE ServerHandle,
    _Out_opt_ PVOID Information
    );

/**
 * The ServerSetInternetConnectorStatus routine is a deprecated entry point for setting Internet Connector licensing status.
 *
 * \param[in] ServerHandle A handle to the server. NULL specifies the current server.
 * \param[in] Enabled The enabled.
 * \return Nonzero if the operation succeeds; otherwise, zero.
 * \remarks The current implementation does not perform the operation; it sets the last-error code to ERROR_INVALID_FUNCTION and returns FALSE.
 */
DECLSPEC_DEPRECATED
NTSYSAPI
BOOLEAN
NTAPI
ServerSetInternetConnectorStatus(
    _In_opt_ HANDLE ServerHandle,
    _In_ BOOLEAN Enabled
    );

/**
 * The WTSRegisterSessionNotificationEx routine registers a window to receive session-change notifications from a specified server.
 *
 * \param[in] ServerHandle A handle to the server. NULL specifies the current server.
 * \param[in] WindowHandle A handle to the notification window.
 * \param[in] Flags Flags that control the operation.
 * \return Nonzero if the operation succeeds; otherwise, zero.
 */
NTSYSAPI
BOOL
WINAPI
WTSRegisterSessionNotificationEx(
    _In_ HANDLE ServerHandle,
    _In_ HWND WindowHandle,
    _In_ DWORD Flags
    );

/**
 * The WTSUnRegisterSessionNotificationEx routine unregisters a window from receiving session-change notifications from a specified server.
 *
 * \param[in] ServerHandle A handle to the server. NULL specifies the current server.
 * \param[in] WindowHandle A handle to the notification window.
 * \return Nonzero if the operation succeeds; otherwise, zero.
 */
NTSYSAPI
BOOL
WINAPI
WTSUnRegisterSessionNotificationEx(
    _In_ HANDLE ServerHandle,
    _In_ HWND WindowHandle
    );

/**
 * The WinStationActivateLicense routine is a deprecated entry point for activating a Terminal Services license.
 *
 * \param[in] ServerHandle A handle to the server. NULL specifies the current server.
 * \param[in] License The license.
 * \return Nonzero if the operation succeeds; otherwise, zero.
 * \remarks The current implementation does not perform the operation; it sets the last-error code to ERROR_INVALID_FUNCTION and returns FALSE.
 */
DECLSPEC_DEPRECATED
NTSYSAPI
BOOLEAN
NTAPI
WinStationActivateLicense(
    _In_opt_ HANDLE ServerHandle,
    _In_opt_ PVOID License
    );

/**
 * The WinStationAutoReconnect routine is a deprecated entry point for automatically reconnecting a disconnected session.
 *
 * \param[in] ReconnectInformation The reconnect information.
 * \return Nonzero if the operation succeeds; otherwise, zero.
 * \remarks The current implementation does not perform the operation; it sets the last-error code to ERROR_INVALID_FUNCTION and returns FALSE.
 */
DECLSPEC_DEPRECATED
NTSYSAPI
BOOLEAN
NTAPI
WinStationAutoReconnect(
    _In_opt_ PVOID ReconnectInformation
    );

/**
 * The WinStationBroadcastSystemMessage routine broadcasts a system message to selected sessions on a Remote Desktop Session Host server.
 *
 * \param[in] ServerHandle A handle to the server. NULL specifies the current server.
 * \param[in] SessionId The session identifier.
 * \param[in] Message The message.
 * \param[in] WParam The w param.
 * \param[in] LParam The l param.
 * \param[in,out] Recipients Supplies the initial recipients and receives the updated value.
 * \param[in] RecipientCount The recipient count.
 * \param[in] MessageData The message data.
 * \param[in] Reserved A message-dependent input buffer (not actually reserved). It is marshalled by
 * AllocateBSMRpcBuffer according to the Message value: for some messages it is a null-terminated
 * wide string, for others a length-prefixed structure; the buffer is copied into the cross-session
 * RPC request. May be NULL when the message carries no pointer payload.
 * \param[out] Response Receives the response.
 * \return Nonzero if the operation succeeds; otherwise, zero.
 */
NTSYSAPI
BOOLEAN
NTAPI
WinStationBroadcastSystemMessage(
    _In_opt_ HANDLE ServerHandle,
    _In_ ULONG SessionId,
    _In_ ULONG Message,
    _In_ ULONG WParam,
    _In_ ULONG LParam,
    _Inout_ PULONG Recipients,
    _In_ ULONG RecipientCount,
    _In_opt_ PVOID MessageData,
    _In_opt_ PVOID Reserved,
    _Out_opt_ PULONG Response
    );

/**
 * The WinStationCheckAccess routine is a deprecated entry point for checking access to a WinStation operation.
 *
 * \param[in] ServerHandle A handle to the server. NULL specifies the current server.
 * \param[in] SessionId The session identifier.
 * \param[in] DesiredAccess The desired access.
 * \return Nonzero if the operation succeeds; otherwise, zero.
 * \remarks The current implementation does not perform the operation; it sets the last-error code to ERROR_INVALID_FUNCTION and returns FALSE.
 */
DECLSPEC_DEPRECATED
NTSYSAPI
BOOLEAN
NTAPI
WinStationCheckAccess(
    _In_opt_ HANDLE ServerHandle,
    _In_ ULONG SessionId,
    _In_ ACCESS_MASK DesiredAccess
    );

/**
 * The WinStationCheckLoopBack routine is a deprecated entry point for checking whether a session connection is a loopback connection.
 *
 * \param[in] ServerHandle A handle to the server. NULL specifies the current server.
 * \param[in] SourceSessionId The source session identifier.
 * \param[in] TargetSessionId The target session identifier.
 * \param[out] LoopBack Receives the loop back.
 * \return Nonzero if the operation succeeds; otherwise, zero.
 * \remarks The current implementation does not perform the operation; it sets the last-error code to ERROR_INVALID_FUNCTION and returns FALSE.
 */
DECLSPEC_DEPRECATED
NTSYSAPI
BOOLEAN
NTAPI
WinStationCheckLoopBack(
    _In_opt_ HANDLE ServerHandle,
    _In_ ULONG SourceSessionId,
    _In_ ULONG TargetSessionId,
    _Out_opt_ PBOOLEAN LoopBack
    );

/**
 * The WinStationConnectA routine is a deprecated entry point for connecting sessions using an ANSI password.
 *
 * \param[in] ConnectInformation The connect information.
 * \return Nonzero if the operation succeeds; otherwise, zero.
 * \remarks The current implementation does not perform the operation; it sets the last-error code to ERROR_INVALID_FUNCTION and returns FALSE.
 */
DECLSPEC_DEPRECATED
NTSYSAPI
BOOLEAN
NTAPI
WinStationConnectA(
    _In_opt_ PVOID ConnectInformation
    );

/**
 * The WinStationConnectAndLockDesktop routine connects to a session and locks its desktop.
 *
 * \param[in] ServerHandle A handle to the server. NULL specifies the current server.
 * \param[in] SessionId The session identifier.
 * \return Nonzero if the operation succeeds; otherwise, zero.
 */
NTSYSAPI
BOOLEAN
NTAPI
WinStationConnectAndLockDesktop(
    _In_opt_ HANDLE ServerHandle,
    _In_ ULONG SessionId
    );

/**
 * The WinStationConnectCallback routine is a deprecated entry point for completing a callback connection.
 *
 * \param[in] Parameter1 An opaque operation-specific parameter.
 * \param[in] Parameter2 An opaque operation-specific parameter.
 * \param[in] Parameter3 An opaque operation-specific parameter.
 * \param[in] Parameter4 An opaque operation-specific parameter.
 * \param[in] Parameter5 An opaque operation-specific parameter.
 * \return Nonzero if the operation succeeds; otherwise, zero.
 * \remarks The current implementation does not perform the operation; it sets the last-error code to ERROR_INVALID_FUNCTION and returns FALSE.
 */
DECLSPEC_DEPRECATED
NTSYSAPI
BOOLEAN
NTAPI
WinStationConnectCallback(
    _In_opt_ PVOID Parameter1,
    _In_opt_ PVOID Parameter2,
    _In_opt_ PVOID Parameter3,
    _In_opt_ PVOID Parameter4,
    _In_opt_ PVOID Parameter5
    );

/**
 * The WinStationConnectEx routine is a deprecated entry point for connecting sessions using extended connection parameters.
 *
 * \param[in] Parameter1 An opaque operation-specific parameter.
 * \param[in] Parameter2 An opaque operation-specific parameter.
 * \return Nonzero if the operation succeeds; otherwise, zero.
 * \remarks The current implementation does not perform the operation; it sets the last-error code to ERROR_INVALID_FUNCTION and returns TRUE.
 */
DECLSPEC_DEPRECATED
NTSYSAPI
BOOLEAN
NTAPI
WinStationConnectEx(
    _In_opt_ PVOID Parameter1,
    _In_opt_ PVOID Parameter2
    );

/**
 * The WinStationConsumeCacheSession routine consumes a cached session for use by the caller.
 *
 * \return An operation-specific status or result value.
 */
NTSYSAPI
ULONG
NTAPI
WinStationConsumeCacheSession(
    VOID
    );

/**
 * The WinStationCreateAgentSessionTransport routine creates an agent-session transport and returns its named-pipe path.
 *
 * \param[out] TransportName A caller-allocated buffer that receives the null-terminated transport name.
 * \param[in] TransportNameLength The capacity of TransportName, in WCHAR elements.
 * \return An HRESULT indicating whether the transport was created.
 */
NTSYSAPI
HRESULT
NTAPI
WinStationCreateAgentSessionTransport(
    _Out_writes_z_(TransportNameLength) PWSTR TransportName,
    _In_ ULONG TransportNameLength
    );

/**
 * The WinStationCreateChildSessionTransport routine creates a child-session transport and returns its named-pipe path.
 *
 * \param[out] TransportName A caller-allocated buffer that receives the null-terminated transport name.
 * \param[in] TransportNameLength The capacity of TransportName, in WCHAR elements.
 * \return An HRESULT indicating whether the transport was created.
 */
NTSYSAPI
HRESULT
NTAPI
WinStationCreateChildSessionTransport(
    _Out_writes_z_(TransportNameLength) PWSTR TransportName,
    _In_ ULONG TransportNameLength
    );

/**
 * The WinStationEnableChildSessions routine enables or disables child-session support.
 *
 * \param[in] Enable The enable.
 * \return Nonzero if the operation succeeds; otherwise, zero.
 */
NTSYSAPI
BOOLEAN
NTAPI
WinStationEnableChildSessions(
    _In_ BOOLEAN Enable
    );

/**
 * The WinStationEnumerateA routine enumerates sessions on a Remote Desktop Session Host server and returns ANSI session records.
 *
 * \param[in] ServerHandle A handle to the server. NULL specifies the current server.
 * \param[out] SessionIds Receives the session ids.
 * \param[out] Count Receives the number of returned entries.
 * \return Nonzero if the operation succeeds; otherwise, zero.
 */
NTSYSAPI
BOOLEAN
NTAPI
WinStationEnumerateA(
    _In_opt_ HANDLE ServerHandle,
    _Outptr_result_buffer_(*Count) PSESSIONIDA *SessionIds,
    _Out_ PULONG Count
    );

/**
 * The WinStationEnumerateContainerSessions routine enumerates sessions associated with a container.
 *
 * \param[in] ServerHandle A handle to the server. NULL specifies the current server.
 * \param[out] Sessions Receives the sessions.
 * \return Nonzero if the operation succeeds; otherwise, zero.
 */
NTSYSAPI
BOOLEAN
NTAPI
WinStationEnumerateContainerSessions(
    _In_opt_ HANDLE ServerHandle,
    _Outptr_result_maybenull_ PVOID *Sessions
    );

/**
 * The WinStationEnumerateExW routine enumerates sessions on a Remote Desktop Session Host server and returns extended Unicode session records.
 *
 * \param[in] ServerHandle A handle to the server. NULL specifies the current server.
 * \param[out] Sessions Receives the sessions.
 * \param[out] Count Receives the number of returned entries.
 * \return Nonzero if the operation succeeds; otherwise, zero.
 */
NTSYSAPI
BOOLEAN
NTAPI
WinStationEnumerateExW(
    _In_opt_ HANDLE ServerHandle,
    _Outptr_result_buffer_(*Count) PWINSTATION_SESSION_INFO_EXW *Sessions,
    _Out_ PULONG Count
    );

/**
 * The WinStationEnumerateLicenses routine is a deprecated entry point for enumerating installed Terminal Services licenses.
 *
 * \param[in] ServerHandle A handle to the server. NULL specifies the current server.
 * \param[out] Licenses Receives the licenses.
 * \param[out] Count Receives the number of returned entries.
 * \return Nonzero if the operation succeeds; otherwise, zero.
 * \remarks The current implementation does not perform the operation; it sets the last-error code to ERROR_INVALID_FUNCTION and returns FALSE.
 */
DECLSPEC_DEPRECATED
NTSYSAPI
BOOLEAN
NTAPI
WinStationEnumerateLicenses(
    _In_opt_ HANDLE ServerHandle,
    _Outptr_result_maybenull_ PVOID *Licenses,
    _Out_opt_ PULONG Count
    );

/**
 * The WinStationEnumerate_IndexedA routine is a deprecated entry point for enumerating an indexed range of ANSI session records.
 *
 * \param[in] ServerHandle A handle to the server. NULL specifies the current server.
 * \param[in] Index The index.
 * \param[out] Information Receives the information.
 * \return Nonzero if the operation succeeds; otherwise, zero.
 * \remarks The current implementation does not perform the operation; it sets the last-error code to ERROR_INVALID_FUNCTION and returns FALSE.
 */
DECLSPEC_DEPRECATED
NTSYSAPI
BOOLEAN
NTAPI
WinStationEnumerate_IndexedA(
    _In_opt_ HANDLE ServerHandle,
    _In_ ULONG Index,
    _Out_opt_ PVOID Information
    );

/**
 * The WinStationEnumerate_IndexedW routine is a deprecated entry point for enumerating an indexed range of Unicode session records.
 *
 * \param[in] ServerHandle A handle to the server. NULL specifies the current server.
 * \param[in] Index The index.
 * \param[out] Information Receives the information.
 * \return Nonzero if the operation succeeds; otherwise, zero.
 * \remarks The current implementation does not perform the operation; it sets the last-error code to ERROR_INVALID_FUNCTION and returns FALSE.
 */
DECLSPEC_DEPRECATED
NTSYSAPI
BOOLEAN
NTAPI
WinStationEnumerate_IndexedW(
    _In_opt_ HANDLE ServerHandle,
    _In_ ULONG Index,
    _Out_opt_ PVOID Information
    );

/**
 * The WinStationFreeConsoleNotification routine provides an exported alias for a Terminal Services client operation.
 *
 * \param[in] ServerHandle A handle to the server. NULL specifies the current server.
 * \param[in] WindowHandle A handle to the notification window.
 * \return Nonzero if the operation succeeds; otherwise, zero.
 */
NTSYSAPI
BOOLEAN
NTAPI
WinStationFreeConsoleNotification(
    _In_opt_ HANDLE ServerHandle,
    _In_ HWND WindowHandle
    );

/**
 * The WinStationFreeSessionNotification routine provides an exported alias for a Terminal Services client operation.
 *
 * \param[in] ServerHandle A handle to the server. NULL specifies the current server.
 * \param[in] WindowHandle A handle to the notification window.
 * \return Nonzero if the operation succeeds; otherwise, zero.
 */
NTSYSAPI
BOOLEAN
NTAPI
WinStationFreeSessionNotification(
    _In_opt_ HANDLE ServerHandle,
    _In_ HWND WindowHandle
    );

/**
 * The WinStationFreeUserCertificates routine frees a user-certificate object returned by WinStationGetUserCertificates, including its certificate-data buffer.
 *
 * \param[in] Certificates The certificates.
 * \return Nonzero if the operation succeeds; otherwise, zero.
 */
NTSYSAPI
BOOLEAN
NTAPI
WinStationFreeUserCertificates(
    _In_opt_ PWINSTATION_USER_CERTIFICATE Certificates
    );

/**
 * The WinStationFreeUserCredentials routine securely clears and frees a credential object returned by WinStationGetUserCredentials.
 *
 * \param[in] Credentials The credentials.
 * \return An operation-specific status or result value.
 */
NTSYSAPI
ULONG
NTAPI
WinStationFreeUserCredentials(
    _In_opt_ PCRED_PROV_CREDENTIAL Credentials
    );

/**
 * The WinStationFreeUserSessionInfo routine frees user-session information allocated by WinStationGetAllUserSessions.
 *
 * \param[in] SessionInformation The session-record array to free.
 * \return Nonzero if the operation succeeds; otherwise, zero. A NULL pointer is rejected with ERROR_INVALID_PARAMETER.
 */
NTSYSAPI
BOOLEAN
NTAPI
WinStationFreeUserSessionInfo(
    _Frees_ptr_ PTS_USER_SESSION SessionInformation
    );

/**
 * The WinStationGenerateLicense routine is a deprecated entry point for generating Terminal Services license data.
 *
 * \param[in] ServerHandle A handle to the server. NULL specifies the current server.
 * \param[in] Request The request.
 * \param[out] License Receives the license.
 * \return Nonzero if the operation succeeds; otherwise, zero.
 * \remarks The current implementation does not perform the operation; it sets the last-error code to ERROR_INVALID_FUNCTION and returns FALSE.
 */
DECLSPEC_DEPRECATED
NTSYSAPI
BOOLEAN
NTAPI
WinStationGenerateLicense(
    _In_opt_ HANDLE ServerHandle,
    _In_opt_ PVOID Request,
    _Out_opt_ PVOID License
    );

/**
 * The WinStationGetAllSessionsW routine retrieves Unicode information for all sessions on a Remote Desktop Session Host server.
 *
 * \param[in] ServerHandle A handle to the server. NULL specifies the current server.
 * \param[in,out] Level On input, the requested information level; on output, the level accepted by the service.
 * \param[out] Sessions Receives a LocalAlloc-allocated array of session records. Free the array with LocalFree.
 * \param[out] Count Receives the number of returned entries.
 * \return Nonzero if the operation succeeds; otherwise, zero.
 */
NTSYSAPI
BOOLEAN
NTAPI
WinStationGetAllSessionsW(
    _In_opt_ HANDLE ServerHandle,
    _Inout_ PULONG Level,
    _Outptr_result_buffer_(*Count) PWINSTATION_SESSION_INFO_1W *Sessions,
    _Out_ PULONG Count
    );

/**
 * The WinStationGetChildSessionId routine retrieves the child-session identifier associated with the current session.
 *
 * \param[out] SessionId The session identifier.
 * \return Nonzero if the operation succeeds; otherwise, zero.
 */
NTSYSAPI
BOOLEAN
NTAPI
WinStationGetChildSessionId(
    _Out_ PULONG SessionId
    );

/**
 * The WinStationGetCurrentSessionCapabilities routine queries a capability of the current session.
 *
 * \param[in] CapabilityClass The capability to query. The audited implementation accepts only WinStationCurrentSessionCapabilityRemoteDesktop.
 * \param[out] CapabilityValue Receives TRUE if the current session has the requested capability; otherwise, FALSE.
 * \return Nonzero if the operation succeeds; otherwise, zero. An unsupported capability class fails with ERROR_INVALID_PARAMETER.
 */
NTSYSAPI
BOOLEAN
NTAPI
WinStationGetCurrentSessionCapabilities(
    _In_ WINSTATION_CURRENT_SESSION_CAPABILITY_CLASS CapabilityClass,
    _Out_ PBOOL CapabilityValue
    );

/**
 * The WinStationGetCurrentSessionConnectionProperty routine retrieves an allocated connection-property value for the current session.
 *
 * \param[in] PropertyId A pointer to the GUID that identifies the connection property.
 * \param[out] PropertyValue Receives a service-allocated property value. Free the value with WinStationFreePropertyValue.
 * \return An operation-specific status or result value.
 */
NTSYSAPI
BOOLEAN
NTAPI
WinStationGetCurrentSessionConnectionProperty(
    _In_ PCGUID PropertyId,
    _Outptr_result_maybenull_ PWINSTATION_PROPERTY_VALUE *PropertyValue
    );

/**
 * The WinStationGetCurrentSessionTerminalName routine retrieves the terminal name of the current session.
 *
 * \param[out] TerminalName A pointer to a buffer of at least 32 WCHARs that receives the terminal name.
 * \return BOOLEAN TRUE if the operation succeeds; otherwise FALSE.
 */
NTSYSAPI
BOOLEAN
NTAPI
WinStationGetCurrentSessionTerminalName(
    _Out_writes_(32) PWSTR TerminalName
    );

/**
 * The WinStationGetInitialApplication routine retrieves the initial command line and working directory for a session.
 *
 * \param[in] SessionId The session identifier. Specify LOGONID_CURRENT for the caller's session.
 * \param[out] CommandLine Receives a WinStation-allocated null-terminated command line.
 * \param[out] WorkingDirectory Receives a WinStation-allocated null-terminated working directory.
 * \param[out] ApplicationAllowed Receives whether the initial application is permitted by policy.
 * \param[out] Maximize Receives whether the application should be maximized.
 * \return Nonzero if the operation succeeds; otherwise, zero.
 * \remarks Free CommandLine and WorkingDirectory with WinStationFreeMemory.
 */
NTSYSAPI
BOOLEAN
NTAPI
WinStationGetInitialApplication(
    _In_ ULONG SessionId,
    _Outptr_result_z_ PWSTR *CommandLine,
    _Outptr_result_z_ PWSTR *WorkingDirectory,
    _Out_ PBOOL ApplicationAllowed,
    _Out_ PBOOL Maximize
    );

/**
 * The WinStationGetLanAdapterNameA routine is a deprecated entry point for retrieving an ANSI LAN-adapter name.
 *
 * \param[in] ServerHandle A handle to the server. NULL specifies the current server.
 * \param[in] SessionId The session identifier.
 * \param[out] Name Receives the name.
 * \param[in] NameLength The size of the name, in bytes.
 * \return Nonzero if the operation succeeds; otherwise, zero.
 * \remarks The current implementation does not perform the operation; it sets the last-error code to ERROR_INVALID_FUNCTION and returns FALSE.
 */
DECLSPEC_DEPRECATED
NTSYSAPI
BOOLEAN
NTAPI
WinStationGetLanAdapterNameA(
    _In_opt_ HANDLE ServerHandle,
    _In_ ULONG SessionId,
    _Out_writes_(NameLength) PSTR Name,
    _In_ ULONG NameLength
    );

/**
 * The WinStationGetLanAdapterNameW routine is a deprecated entry point for retrieving a Unicode LAN-adapter name.
 *
 * \param[in] ServerHandle A handle to the server. NULL specifies the current server.
 * \param[in] SessionId The session identifier.
 * \param[out] Name Receives the name.
 * \param[in] NameLength The size of the name, in bytes.
 * \return Nonzero if the operation succeeds; otherwise, zero.
 * \remarks The current implementation does not perform the operation; it sets the last-error code to ERROR_INVALID_FUNCTION and returns FALSE.
 */
DECLSPEC_DEPRECATED
NTSYSAPI
BOOLEAN
NTAPI
WinStationGetLanAdapterNameW(
    _In_opt_ HANDLE ServerHandle,
    _In_ ULONG SessionId,
    _Out_writes_(NameLength) PWSTR Name,
    _In_ ULONG NameLength
    );

/**
 * The WinStationGetLastWinlogonNotification routine retrieves the Winlogon notification currently blocked by a subscriber in a session.
 *
 * \param[in] SessionId The session identifier.
 * \param[out] NotificationId Receives the Winlogon notification identifier.
 * \param[out] SubscriberName Receives the null-terminated notification subscriber name. The caller supplies space for at least 260 WCHAR elements.
 * \param[out] IsCritical Receives TRUE if the subscriber is marked as critical; otherwise, FALSE.
 * \param[out] NotificationTickCount Receives the GetTickCount64 value recorded for the notification.
 * \return Nonzero if the operation succeeds; otherwise, zero.
 */
NTSYSAPI
BOOLEAN
NTAPI
WinStationGetLastWinlogonNotification(
    _In_ ULONG SessionId,
    _Out_ PULONG NotificationId,
    _Out_writes_z_(260) PWSTR SubscriberName,
    _Out_ PBOOL IsCritical,
    _Out_ PULONGLONG NotificationTickCount
    );

/**
 * The WinStationGetMachinePolicy routine is a deprecated entry point for retrieving Terminal Services machine policy.
 *
 * \param[in] ServerHandle A handle to the server. NULL specifies the current server.
 * \param[out] Policy Receives the policy.
 * \return Nonzero if the operation succeeds; otherwise, zero.
 * \remarks The current implementation does not perform the operation; it sets the last-error code to ERROR_INVALID_FUNCTION and returns FALSE.
 */
DECLSPEC_DEPRECATED
NTSYSAPI
BOOLEAN
NTAPI
WinStationGetMachinePolicy(
    _In_opt_ HANDLE ServerHandle,
    _Out_opt_ PVOID Policy
    );

/**
 * The WinStationGetParentSessionId routine retrieves the parent-session identifier associated with the current session.
 *
 * \param[in] SessionId The session identifier.
 * \param[out] ParentSessionId Receives the parent session identifier.
 * \return Nonzero if the operation succeeds; otherwise, zero.
 */
NTSYSAPI
BOOLEAN
NTAPI
WinStationGetParentSessionId(
    _In_ ULONG SessionId,
    _Out_ PULONG ParentSessionId
    );

/**
 * The WinStationGetRedirectAuthInfo routine retrieves certificate and symmetric-key material used to redirect a logon.
 *
 * \param[in] RedirectionGuid The redirection operation identifier.
 * \param[out] CertificateLength Receives the certificate size, in bytes.
 * \param[out] Certificate Receives a service-allocated certificate byte buffer. Free the buffer with WinStationFreeMemory.
 * \param[out] SymmetricAlgorithm Receives a service-allocated null-terminated symmetric-algorithm identifier. Free the string with WinStationFreeMemory.
 * \param[out] SymmetricKeyLength Receives the symmetric-key size, in bytes.
 * \param[out] SymmetricKey Receives a service-allocated symmetric-key byte buffer. Free the buffer with WinStationFreeMemory.
 * \return Zero if the operation succeeds; otherwise, an error status.
 */
NTSYSAPI
ULONG
NTAPI
WinStationGetRedirectAuthInfo(
    _In_ const GUID *RedirectionGuid,
    _Out_ PULONG CertificateLength,
    _Outptr_result_bytebuffer_(*CertificateLength) PBYTE *Certificate,
    _Outptr_result_z_ PWSTR *SymmetricAlgorithm,
    _Out_ PULONG SymmetricKeyLength,
    _Outptr_result_bytebuffer_(*SymmetricKeyLength) PBYTE *SymmetricKey
    );

/**
 * The WinStationGetRestrictedLogonInfo routine determines whether restricted logon was requested for the calling session and returns the authentication identifier of its security-filter client token.
 *
 * \param[out] RestrictedLogonRequested Receives TRUE if restricted logon was requested; otherwise, FALSE.
 * \param[out] AuthenticationId Receives the token authentication identifier. The value is zero when restricted logon was not requested.
 * \return Zero if the operation succeeds; otherwise, a Win32 error code.
 */
NTSYSAPI
ULONG
NTAPI
WinStationGetRestrictedLogonInfo(
    _Out_ PBOOL RestrictedLogonRequested,
    _Out_ PLUID AuthenticationId
    );

/**
 * The WinStationGetSessionIds routine retrieves session identifiers associated with the current session context.
 *
 * \param[in] State The state.
 * \param[out] SessionIds Receives the session ids.
 * \param[in,out] BufferLength The size of the buffer, in bytes.
 * \return Nonzero if the operation succeeds; otherwise, zero.
 */
NTSYSAPI
BOOLEAN
NTAPI
WinStationGetSessionIds(
    _In_ ULONG State,
    _Out_writes_bytes_to_opt_(*BufferLength, *BufferLength) PULONG SessionIds,
    _Inout_ PULONG BufferLength
    );

/**
 * The WinStationGetSpecialSessionFlags routine retrieves special-session flags for the specified session.
 *
 * \param[in] SessionId The session identifier.
 * \param[out] Flags Flags that control the operation.
 * \return Nonzero if the operation succeeds; otherwise, zero.
 */
NTSYSAPI
BOOLEAN
NTAPI
WinStationGetSpecialSessionFlags(
    _In_ ULONG SessionId,
    _Out_ PULONG Flags
    );

/**
 * The WinStationGetUserCertificates routine retrieves the user-certificate data associated with a session.
 *
 * \param[out] Certificates Receives the certificates.
 * \return Nonzero if the operation succeeds; otherwise, zero.
 */
NTSYSAPI
BOOLEAN
NTAPI
WinStationGetUserCertificates(
    _Outptr_result_maybenull_ PWINSTATION_USER_CERTIFICATE *Certificates
    );

/**
 * The WinStationGetUserCredentials routine retrieves the credential-provider credentials associated with a session.
 *
 * \param[out] Credentials Receives the credentials.
 * \return An operation-specific status or result value.
 */
NTSYSAPI
ULONG
NTAPI
WinStationGetUserCredentials(
    _Outptr_result_maybenull_ PCRED_PROV_CREDENTIAL *Credentials
    );

/**
 * The WinStationGetUserProfile routine retrieves the user-profile path associated with a session.
 *
 * \param[in] SessionId The session identifier.
 * \param[out] UserName Receives the user name.
 * \param[out] Domain Receives the domain.
 * \param[out] ProfilePath Receives the profile path.
 * \return An operation-specific status or result value.
 */
NTSYSAPI
ULONG
NTAPI
WinStationGetUserProfile(
    _In_ ULONG SessionId,
    _Outptr_result_maybenull_ PWSTR *UserName,
    _Outptr_result_maybenull_ PWSTR *Domain,
    _Outptr_result_maybenull_ PWSTR *ProfilePath
    );

/**
 * The WinStationInstallLicense routine is a deprecated entry point for installing Terminal Services license data.
 *
 * \param[in] ServerHandle A handle to the server. NULL specifies the current server.
 * \param[in] License The license.
 * \return Nonzero if the operation succeeds; otherwise, zero.
 * \remarks The current implementation does not perform the operation; it sets the last-error code to ERROR_INVALID_FUNCTION and returns FALSE.
 */
DECLSPEC_DEPRECATED
NTSYSAPI
BOOLEAN
NTAPI
WinStationInstallLicense(
    _In_opt_ HANDLE ServerHandle,
    _In_opt_ PVOID License
    );

/**
 * The WinStationIsBoundToCacheTerminal routine determines whether a session is bound to a cache terminal.
 *
 * \param[in] SessionId The session identifier.
 * \return An operation-specific status or result value.
 */
NTSYSAPI
ULONG
NTAPI
WinStationIsBoundToCacheTerminal(
    _In_ ULONG SessionId
    );

/**
 * The WinStationIsChildSessionsEnabled routine determines whether child-session support is enabled.
 *
 * \param[out] Enabled Receives the enabled.
 * \return Nonzero if the operation succeeds; otherwise, zero.
 */
NTSYSAPI
BOOLEAN
NTAPI
WinStationIsChildSessionsEnabled(
    _Out_ PBOOLEAN Enabled
    );

/**
 * The WinStationIsHelpAssistantSession routine determines whether the specified session is a Help Assistant session.
 *
 * \param[in] ServerHandle A handle to the server. NULL specifies the current server.
 * \param[in] SessionId The session identifier.
 * \return Nonzero if the operation succeeds; otherwise, zero.
 */
NTSYSAPI
BOOLEAN
NTAPI
WinStationIsHelpAssistantSession(
    _In_opt_ HANDLE ServerHandle,
    _In_ ULONG SessionId
    );

/**
 * The WinStationIsSessionPermitted routine determines whether creation or use of a session is permitted.
 *
 * \return An operation-specific status or result value.
 */
NTSYSAPI
ULONG
NTAPI
WinStationIsSessionPermitted(
    VOID
    );

/**
 * The WinStationNameFromLogonIdA routine is a deprecated entry point for resolving an ANSI WinStation name from a session identifier.
 *
 * \param[in] ServerHandle A handle to the server. NULL specifies the current server.
 * \param[in] SessionId The session identifier.
 * \param[out] WinStationName Receives the win station name.
 * \param[in] NameLength The size of the name, in bytes.
 * \return Nonzero if the operation succeeds; otherwise, zero.
 * \remarks The current implementation does not perform the operation; it sets the last-error code to ERROR_INVALID_FUNCTION and returns FALSE.
 */
DECLSPEC_DEPRECATED
NTSYSAPI
BOOLEAN
NTAPI
WinStationNameFromLogonIdA(
    _In_opt_ HANDLE ServerHandle,
    _In_ ULONG SessionId,
    _Out_writes_(NameLength) PSTR WinStationName,
    _In_ ULONG NameLength
    );

/**
 * The WinStationNegotiateSession routine retrieves session-negotiation values from the Remote Desktop Session Host service.
 *
 * \param[in] ServerHandle A handle to the server. NULL specifies the current server.
 * \param[in] SessionId The session identifier.
 * \param[out] NegotiationFlags Receives the negotiation flags returned by the service.
 * \param[out] ValueCount Receives the number of ULONG elements in Values.
 * \param[out] Values Receives a LocalAlloc-allocated array of ULONG values.
 * \param[out] Result Receives the negotiation result returned by the service.
 * \return A Win32 error code. ERROR_SUCCESS indicates success.
 * \remarks Free Values with LocalFree.
 */
NTSYSAPI
ULONG
NTAPI
WinStationNegotiateSession(
    _In_opt_ HANDLE ServerHandle,
    _In_ ULONG SessionId,
    _Out_ PULONG NegotiationFlags,
    _Out_ PULONG ValueCount,
    _Outptr_result_buffer_(*ValueCount) PULONG *Values,
    _Out_ PULONG Result
    );

/**
 * The WinStationNtsdDebug routine is a deprecated entry point for starting or controlling NTSD debugging for a session process.
 *
 * \param[in] Parameter1 An opaque operation-specific parameter.
 * \param[in] Parameter2 An opaque operation-specific parameter.
 * \param[in] Parameter3 An opaque operation-specific parameter.
 * \param[in] Parameter4 An opaque operation-specific parameter.
 * \param[in] Parameter5 An opaque operation-specific parameter.
 * \return Nonzero if the operation succeeds; otherwise, zero.
 * \remarks The current implementation does not perform the operation; it sets the last-error code to ERROR_INVALID_FUNCTION and returns FALSE.
 */
DECLSPEC_DEPRECATED
NTSYSAPI
BOOLEAN
NTAPI
WinStationNtsdDebug(
    _In_opt_ PVOID Parameter1,
    _In_opt_ PVOID Parameter2,
    _In_opt_ PVOID Parameter3,
    _In_opt_ PVOID Parameter4,
    _In_opt_ PVOID Parameter5
    );

/**
 * The WinStationOpenServerA routine opens an ANSI handle to a Remote Desktop Session Host server.
 *
 * \param[in, optional] ServerName The server name.
 * \return A handle if the operation succeeds; otherwise, NULL.
 */
NTSYSAPI
HANDLE
NTAPI
WinStationOpenServerA(
    _In_opt_ PCSTR ServerName
    );

/**
 * The WinStationOpenServerExA routine opens an extended ANSI handle to a Remote Desktop Session Host server.
 *
 * \param[in, optional] ServerName The server name.
 * \return A handle if the operation succeeds; otherwise, NULL.
 */
NTSYSAPI
HANDLE
NTAPI
WinStationOpenServerExA(
    _In_opt_ PCSTR ServerName
    );

/**
 * The WinStationPreCreateGlassReplacementSession routine prepares creation of a glass-replacement session for an application.
 *
 * \param[in, optional] InitialCommand The initial command line.
 * \param[in] Arg2 Operation-specific parameter.
 * \param[out] SessionId Receives the session identifier.
 * \return BOOLEAN TRUE if the operation succeeds; otherwise, FALSE.
 */
NTSYSAPI
BOOLEAN
NTAPI
WinStationPreCreateGlassReplacementSession(
    _In_opt_ PCWSTR InitialCommand,
    _In_ ULONG Arg2,
    _Out_ PULONG SessionId
    );

/**
 * The WinStationPreCreateGlassReplacementSessionEx routine prepares creation of an extended glass-replacement session for an application.
 *
 * \param[in, optional] InitialCommand The initial command line.
 * \param[in] Arg2 Operation-specific parameter.
 * \param[in] Arg3 Operation-specific parameter.
 * \param[out] SessionId Receives the session identifier.
 * \return BOOLEAN TRUE if the operation succeeds; otherwise, FALSE.
 */
NTSYSAPI
BOOLEAN
NTAPI
WinStationPreCreateGlassReplacementSessionEx(
    _In_opt_ PCWSTR InitialCommand,
    _In_ ULONG Arg2,
    _In_ ULONG Arg3,
    _Out_ PULONG SessionId
    );

/**
 * The WinStationQueryAllowConcurrentConnections routine is a deprecated entry point for querying whether concurrent session connections are allowed.
 *
 * \param[out] Allowed Receives the allowed.
 * \return Nonzero if the operation succeeds; otherwise, zero.
 * \remarks The current implementation does not perform the operation; it sets the last-error code to ERROR_INVALID_FUNCTION and returns FALSE.
 */
DECLSPEC_DEPRECATED
NTSYSAPI
BOOLEAN
NTAPI
WinStationQueryAllowConcurrentConnections(
    _Out_opt_ PBOOLEAN Allowed
    );

/**
 * The WinStationQueryEnforcementCore routine queries a licensing-enforcement policy value.
 *
 * \param[in] ServerHandle A handle to the server. NULL specifies the current server.
 * \param[in] PolicyId An optional pointer to the policy GUID. If NULL, the built-in GUID for the information class is used.
 * \param[in] InformationClass The licensing-enforcement information class to query.
 * \param[out] Buffer Receives the 32-bit value selected by InformationClass.
 * \param[in] BufferLength The size of the buffer, in bytes.
 * \param[out] ReturnLength Receives the number of bytes written or required.
 * \return Nonzero if the operation succeeds; otherwise, zero. Unsupported information classes fail with ERROR_INVALID_PARAMETER; buffers smaller than sizeof(ULONG) fail with ERROR_INSUFFICIENT_BUFFER.
 */
NTSYSAPI
BOOLEAN
NTAPI
WinStationQueryEnforcementCore(
    _In_opt_ HANDLE ServerHandle,
    _In_opt_ PCGUID PolicyId,
    _In_ WINSTATION_ENFORCEMENT_CORE_INFORMATION_CLASS InformationClass,
    _Out_ PULONG Buffer,
    _In_ ULONG BufferLength,
    _Out_ PULONG ReturnLength
    );

/**
 * The WinStationQueryInformationA routine is a deprecated entry point for querying ANSI WinStation information.
 *
 * \param[in] ServerHandle A handle to the server. NULL specifies the current server.
 * \param[in] SessionId The session identifier.
 * \param[in] WinStationInformationClass The win station information class.
 * \param[out] WinStationInformation Receives the win station information.
 * \param[in] WinStationInformationLength The size of the win station information, in bytes.
 * \param[out] ReturnLength Receives the number of bytes written or required.
 * \return Nonzero if the operation succeeds; otherwise, zero.
 * \remarks The current implementation does not perform the operation; it sets the last-error code to ERROR_INVALID_FUNCTION and returns FALSE.
 */
DECLSPEC_DEPRECATED
NTSYSAPI
BOOLEAN
NTAPI
WinStationQueryInformationA(
    _In_opt_ HANDLE ServerHandle,
    _In_ ULONG SessionId,
    _In_ WINSTATIONINFOCLASS WinStationInformationClass,
    _Out_writes_bytes_(WinStationInformationLength) PVOID WinStationInformation,
    _In_ ULONG WinStationInformationLength,
    _Out_ PULONG ReturnLength
    );

/**
 * The WinStationQueryLicense routine is a deprecated entry point for querying Terminal Services license information.
 *
 * \param[in] ServerHandle A handle to the server. NULL specifies the current server.
 * \param[in] SessionId The session identifier.
 * \param[out] LicenseInformation Receives the license information.
 * \return Nonzero if the operation succeeds; otherwise, zero.
 * \remarks The current implementation does not perform the operation; it sets the last-error code to ERROR_INVALID_FUNCTION and returns FALSE.
 */
DECLSPEC_DEPRECATED
NTSYSAPI
BOOLEAN
NTAPI
WinStationQueryLicense(
    _In_opt_ HANDLE ServerHandle,
    _In_ ULONG SessionId,
    _Out_opt_ PVOID LicenseInformation
    );

/**
 * The WinStationQueryLogonCredentialsW routine is a deprecated entry point for querying Unicode session logon credentials.
 *
 * \param[in] ServerHandle A handle to the server. NULL specifies the current server.
 * \param[in] SessionId The session identifier.
 * \param[out] Credentials Receives the credentials.
 * \return Nonzero if the operation succeeds; otherwise, zero.
 * \remarks The current implementation does not perform the operation; it sets the last-error code to ERROR_INVALID_FUNCTION and returns FALSE.
 */
DECLSPEC_DEPRECATED
NTSYSAPI
BOOLEAN
NTAPI
WinStationQueryLogonCredentialsW(
    _In_opt_ HANDLE ServerHandle,
    _In_ ULONG SessionId,
    _Out_opt_ PVOID Credentials
    );

/**
 * The WinStationQueryUpdateRequired routine is a deprecated entry point for querying whether a Terminal Services update is required.
 *
 * \param[in] ServerHandle A handle to the server. NULL specifies the current server.
 * \param[out] UpdateRequired Receives the update required.
 * \return Nonzero if the operation succeeds; otherwise, zero.
 * \remarks The current implementation does not perform the operation; it sets the last-error code to ERROR_INVALID_FUNCTION and returns FALSE.
 */
DECLSPEC_DEPRECATED
NTSYSAPI
BOOLEAN
NTAPI
WinStationQueryUpdateRequired(
    _In_opt_ HANDLE ServerHandle,
    _Out_opt_ PBOOLEAN UpdateRequired
    );

/**
 * The WinStationRcmShadow2 routine creates a shadow invitation for a session.
 *
 * \param[in] SourceSessionId The source session identifier.
 * \param[in] TargetSessionId The target session identifier.
 * \param[in] ShadowOptions A 32-bit value passed to the shadow-invitation creation operation.
 * \param[out] Response Receives the shadow invitation result.
 * \param[out] Invitation Receives the null-terminated shadow invitation string.
 * \param[in] InvitationLength The capacity of Invitation, in WCHAR elements.
 * \return Nonzero if the operation succeeds; otherwise, zero.
 */
NTSYSAPI
BOOLEAN
NTAPI
WinStationRcmShadow2(
    _In_ ULONG SourceSessionId,
    _In_ ULONG TargetSessionId,
    _In_ ULONG ShadowOptions,
    _Out_ PULONG Response,
    _Out_writes_z_(InvitationLength) PWSTR Invitation,
    _In_ ULONG InvitationLength
    );

/**
 * The WinStationRedirectErrorMessage routine reports an error message during redirected logon.
 *
 * \param[in] ErrorCode The error code.
 * \param[in] MessageId The message id.
 * \return An operation-specific status or result value.
 */
NTSYSAPI
ULONG
NTAPI
WinStationRedirectErrorMessage(
    _In_ ULONG ErrorCode,
    _In_ ULONG MessageId
    );

/**
 * The WinStationRedirectLogonBeginPainting routine notifies redirected logon that the session can begin painting.
 *
 * \return An operation-specific status or result value.
 */
NTSYSAPI
ULONG
NTAPI
WinStationRedirectLogonBeginPainting(
    VOID
    );

/**
 * The WinStationRedirectLogonError routine reports a redirected-logon error and optionally displays an associated message.
 *
 * \param[in] ErrorCode The primary error code.
 * \param[in] SubErrorCode The secondary error code.
 * \param[in, optional] Message A null-terminated Unicode error message.
 * \param[in, optional] Caption A null-terminated Unicode caption for the message.
 * \param[in] Type Flags controlling how the message is presented.
 * \param[out, optional] Response Receives the response selected for the message.
 * \return An HRESULT-style status value.
 */
NTSYSAPI
ULONG
NTAPI
WinStationRedirectLogonError(
    _In_ ULONG ErrorCode,
    _In_ ULONG SubErrorCode,
    _In_opt_ PCWSTR Message,
    _In_opt_ PCWSTR Caption,
    _In_ ULONG Type,
    _Out_opt_ PULONG Response
    );

/**
 * The WinStationRedirectLogonMessage routine displays a message during redirected logon.
 *
 * \param[in, optional] Message A null-terminated Unicode message.
 * \param[in, optional] Caption A null-terminated Unicode caption for the message.
 * \param[in] Type Flags controlling how the message is presented.
 * \param[out, optional] Response Receives the response selected for the message.
 * \return An HRESULT-style status value.
 */
NTSYSAPI
ULONG
NTAPI
WinStationRedirectLogonMessage(
    _In_opt_ PCWSTR Message,
    _In_opt_ PCWSTR Caption,
    _In_ ULONG Type,
    _Out_opt_ PULONG Response
    );

/**
 * The WinStationRedirectLogonStatus routine reports a redirected-logon status message and retrieves the resulting status value.
 *
 * \param[in] StatusMessage A null-terminated Unicode status message.
 * \param[out] Status Receives the resulting 32-bit status value.
 * \return An HRESULT indicating whether the operation succeeded.
 */
NTSYSAPI
HRESULT
NTAPI
WinStationRedirectLogonStatus(
    _In_ PCWSTR StatusMessage,
    _Out_ PULONG Status
    );

/**
 * The WinStationRedirectShellExecute routine requests shell execution during redirected logon.
 *
 * \param[in] ServerHandle A handle to the server. NULL specifies the current server.
 * \param[in] SessionId The session identifier.
 * \param[in] CommandLine The command line.
 * \return Nonzero if the operation succeeds; otherwise, zero.
 */
NTSYSAPI
BOOLEAN
NTAPI
WinStationRedirectShellExecute(
    _In_opt_ HANDLE ServerHandle,
    _In_ ULONG SessionId,
    _In_ PCWSTR CommandLine
    );

/**
 * The WinStationRemoveLicense routine is a deprecated entry point for removing Terminal Services license data.
 *
 * \param[in] ServerHandle A handle to the server. NULL specifies the current server.
 * \param[in] License The license.
 * \return Nonzero if the operation succeeds; otherwise, zero.
 * \remarks The current implementation does not perform the operation; it sets the last-error code to ERROR_INVALID_FUNCTION and returns FALSE.
 */
NTSYSAPI
BOOLEAN
NTAPI
WinStationRemoveLicense(
    _In_opt_ HANDLE ServerHandle,
    _In_opt_ PVOID License
    );

/**
 * The WinStationRenameA routine is a deprecated entry point for renaming a WinStation using an ANSI name.
 *
 * \param[in] ServerHandle A handle to the server. NULL specifies the current server.
 * \param[in] SessionId The session identifier.
 * \param[in] NewName The new name.
 * \return Nonzero if the operation succeeds; otherwise, zero.
 * \remarks The current implementation does not perform the operation; it sets the last-error code to ERROR_INVALID_FUNCTION and returns FALSE.
 */
DECLSPEC_DEPRECATED
NTSYSAPI
BOOLEAN
NTAPI
WinStationRenameA(
    _In_opt_ HANDLE ServerHandle,
    _In_ ULONG SessionId,
    _In_ PCSTR NewName
    );

/**
 * The WinStationRenameW routine is a deprecated entry point for renaming a WinStation using a Unicode name.
 *
 * \param[in] ServerHandle A handle to the server. NULL specifies the current server.
 * \param[in] SessionId The session identifier.
 * \param[in] NewName The new name.
 * \return Nonzero if the operation succeeds; otherwise, zero.
 * \remarks The current implementation does not perform the operation; it sets the last-error code to ERROR_INVALID_FUNCTION and returns FALSE.
 */
DECLSPEC_DEPRECATED
NTSYSAPI
BOOLEAN
NTAPI
WinStationRenameW(
    _In_opt_ HANDLE ServerHandle,
    _In_ ULONG SessionId,
    _In_ PCWSTR NewName
    );

/**
 * The WinStationSetPoolCount routine is a deprecated entry point for setting the configured WinStation pool count.
 *
 * \param[in] ServerHandle A handle to the server. NULL specifies the current server.
 * \param[in] PoolId The pool id.
 * \param[in] PoolCount The pool count.
 * \return Nonzero if the operation succeeds; otherwise, zero.
 * \remarks The current implementation does not perform the operation; it sets the last-error code to ERROR_INVALID_FUNCTION and returns FALSE.
 */
DECLSPEC_DEPRECATED
NTSYSAPI
BOOLEAN
NTAPI
WinStationSetPoolCount(
    _In_opt_ HANDLE ServerHandle,
    _In_ ULONG PoolId,
    _In_ ULONG PoolCount
    );

/**
 * The WinStationRegisterConsoleNotificationEx routine registers a window to receive selected session-change notifications.
 *
 * \param[in] ServerHandle A handle to the server. NULL specifies the current server.
 * \param[in] WindowHandle A handle to the window that receives session-change notifications.
 * \param[in] NotificationScope WNOTIFY_THIS_SESSION (0) for the calling process session, or WNOTIFY_ALL_SESSIONS (1) for all sessions.
 * \param[in] NotificationMask A mask selecting the session-change notifications to receive.
 * \return Nonzero if the registration succeeds; otherwise, zero. To get extended error information, call GetLastError.
 */
NTSYSAPI
BOOLEAN
NTAPI
WinStationRegisterConsoleNotificationEx(
    _In_opt_ HANDLE ServerHandle,
    _In_ HWND WindowHandle,
    _In_ ULONG NotificationScope,
    _In_ ULONG NotificationMask
    );

/**
 * The WinStationRegisterConsoleNotificationEx2 routine registers a window to receive selected session-change notifications.
 *
 * \param[in] ServerHandle A handle to the server. NULL specifies the current server.
 * \param[in] WindowHandle A handle to the window that receives session-change notifications.
 * \param[in] NotificationScope WNOTIFY_THIS_SESSION (0) for the calling process session, or WNOTIFY_ALL_SESSIONS (1) for all sessions.
 * \param[in] NotificationMask A mask selecting the session-change notifications to receive.
 * \return Nonzero if the registration succeeds; otherwise, zero. To get extended error information, call GetLastError.
 */
NTSYSAPI
BOOLEAN
NTAPI
WinStationRegisterConsoleNotificationEx2(
    _In_opt_ HANDLE ServerHandle,
    _In_ HWND WindowHandle,
    _In_ ULONG NotificationScope,
    _In_ ULONG NotificationMask
    );

// rev
/**
 * The WinStationRegisterCurrentSessionNotificationEvent routine registers an event object for notifications concerning the current session.
 *
 * \param[in] ServerHandle A handle to the server. NULL specifies the current server.
 * \param[in] EventMask The event mask.
 * \param[in] EventHandle A handle to the notification event.
 * \return Nonzero if the operation succeeds; otherwise, zero.
 */
NTSYSAPI
BOOLEAN
NTAPI
WinStationRegisterCurrentSessionNotificationEvent(
    _In_opt_ HANDLE ServerHandle,
    _In_ ULONG EventMask,
    _In_ HANDLE EventHandle
    );

// rev
/**
 * The WinStationRegisterNotificationEvent routine registers an event object for notifications concerning a specified session.
 *
 * \param[in] ServerHandle A handle to the server. NULL specifies the current server.
 * \param[in] SessionId The session identifier.
 * \param[in] EventMask The event mask.
 * \param[in] EventHandle A handle to the notification event.
 * \return Nonzero if the operation succeeds; otherwise, zero.
 */
NTSYSAPI
BOOLEAN
NTAPI
WinStationRegisterNotificationEvent(
    _In_opt_ HANDLE ServerHandle,
    _In_ ULONG SessionId,
    _In_ ULONG EventMask,
    _In_ HANDLE EventHandle
    );

/**
 * The WinStationRegisterSessionNotification routine registers a window to receive session-change notifications.
 *
 * \param[in] ServerHandle A handle to the server. NULL specifies the current server.
 * \param[in] WindowHandle A handle to the window that receives session-change notifications.
 * \param[in] NotificationScope WNOTIFY_THIS_SESSION (0) for the calling process session, or WNOTIFY_ALL_SESSIONS (1) for all sessions.
 * \return Nonzero if the registration succeeds; otherwise, zero. To get extended error information, call GetLastError.
 */
NTSYSAPI
BOOLEAN
NTAPI
WinStationRegisterSessionNotification(
    _In_opt_ HANDLE ServerHandle,
    _In_ HWND WindowHandle,
    _In_ ULONG NotificationScope
    );

/**
 * The WinStationRegisterSessionNotificationEx routine registers a window to receive selected session-change notifications.
 *
 * \param[in] ServerHandle A handle to the server. NULL specifies the current server.
 * \param[in] WindowHandle A handle to the window that receives session-change notifications.
 * \param[in] NotificationScope WNOTIFY_THIS_SESSION (0) for the calling process session, or WNOTIFY_ALL_SESSIONS (1) for all sessions.
 * \param[in] NotificationMask A mask selecting the session-change notifications to receive.
 * \return Nonzero if the registration succeeds; otherwise, zero. To get extended error information, call GetLastError.
 */
NTSYSAPI
BOOLEAN
NTAPI
WinStationRegisterSessionNotificationEx(
    _In_opt_ HANDLE ServerHandle,
    _In_ HWND WindowHandle,
    _In_ ULONG NotificationScope,
    _In_ ULONG NotificationMask
    );

/**
 * The WinStationReportLoggedOnCompleted routine reports that the session logon sequence has completed.
 *
 * \return Nonzero if the operation succeeds; otherwise, zero.
 */
NTSYSAPI
BOOLEAN
NTAPI
WinStationReportLoggedOnCompleted(
    VOID
    );

// begin_rev
// UIResult values for WinStationReportUIResult. The call forwards a
// TS_WINLOGON_REPLY (Result, SelectedSessionId) to the Local Session Manager
// (lsm!RpcReportWinlogonReply), which routes it to the session-arbitration
// reconnect or session-bumping dialog response handlers.
/**
 * WINSTATION_UIRESULT_CANCEL
 *
 * The user cancelled the dialog. The pending logon request is terminated
 * (reconnect is abandoned / the other user is not bumped).
 */
#define WINSTATION_UIRESULT_CANCEL 0x0
/**
 * WINSTATION_UIRESULT_PROCEED
 *
 * The user confirmed the dialog. Session arbitration reconnects to (or logs
 * off and bumps) the session identified by the Reserved/SelectedSessionId
 * parameter.
 */
#define WINSTATION_UIRESULT_PROCEED 0x1
// end_rev

// rev
/**
 * The WinStationReportUIResult routine reports the result of a session logon user-interface dialog.
 *
 * \param[in] SessionId The session identifier.
 * \param[in] UIResult The dialog result, one of the WINSTATION_UIRESULT_* values.
 * \param[in] Reserved The selected session identifier for a reconnect/session-bump dialog when
 * UIResult is WINSTATION_UIRESULT_PROCEED; 0xFFFFFFFF requests creation of a new session. Ignored otherwise.
 * \return Nonzero if the operation succeeds; otherwise, zero.
 */
NTSYSAPI
BOOLEAN
NTAPI
WinStationReportUIResult(
    _In_ ULONG SessionId,
    _In_ ULONG UIResult, // WINSTATION_UIRESULT_*
    _In_ ULONG Reserved // SelectedSessionId, or 0xFFFFFFFF for a new session
    );

// rev
/**
 * The WinStationSendMessageA routine displays an ANSI message box in a session on a Remote Desktop Session Host server.
 *
 * \param[in] ServerHandle A handle to the server. NULL specifies the current server.
 * \param[in] SessionId The session identifier.
 * \param[in] Title The title.
 * \param[in] TitleLength The size of the title, in bytes.
 * \param[in] Message The message.
 * \param[in] MessageLength The size of the message, in bytes.
 * \param[in] Style The style.
 * \param[in] Timeout The timeout.
 * \param[out] Response Receives the response.
 * \param[in] DoNotWait The do not wait.
 * \return Nonzero if the operation succeeds; otherwise, zero.
 */
NTSYSAPI
BOOLEAN
NTAPI
WinStationSendMessageA(
    _In_opt_ HANDLE ServerHandle,
    _In_ ULONG SessionId,
    _In_reads_bytes_(TitleLength) PSTR Title,
    _In_ ULONG TitleLength,
    _In_reads_bytes_(MessageLength) PSTR Message,
    _In_ ULONG MessageLength,
    _In_ ULONG Style,
    _In_ ULONG Timeout,
    _Out_ PULONG Response,
    _In_ BOOLEAN DoNotWait
    );

// rev
/**
 * The WinStationSendWindowMessage routine sends a window message to a session.
 *
 * \param[in] ServerHandle A handle to the server. NULL specifies the current server.
 * \param[in] SessionId The session identifier.
 * \param[in] WindowHandle A handle to the notification window.
 * \param[in] Reserved An operation-specific reserved parameter.
 * \param[in] Message The message.
 * \param[in] wParam The w param.
 * \param[in] lParam The l param.
 * \param[in] Timeout The timeout.
 * \return Nonzero if the operation succeeds; otherwise, zero.
 */
NTSYSAPI
BOOLEAN
NTAPI
WinStationSendWindowMessage(
    _In_opt_ HANDLE ServerHandle,
    _In_ ULONG SessionId,
    _In_ ULONG WindowHandle,
    _In_ ULONG Reserved,
    _In_ ULONG Message,
    _In_ ULONG wParam,
    _In_opt_ PVOID lParam,
    _In_ ULONG Timeout
    );

/**
 * The WinStationSetInformationA routine is a deprecated entry point for setting ANSI WinStation information.
 *
 * \param[in] ServerHandle A handle to the server. NULL specifies the current server.
 * \param[in] SessionId The session identifier.
 * \param[in] WinStationInformationClass The win station information class.
 * \param[in] WinStationInformation The win station information.
 * \param[in] WinStationInformationLength The size of the win station information, in bytes.
 * \return Nonzero if the operation succeeds; otherwise, zero.
 * \remarks The current implementation does not perform the operation; it sets the last-error code to ERROR_INVALID_FUNCTION and returns FALSE.
 */
DECLSPEC_DEPRECATED
NTSYSAPI
BOOLEAN
NTAPI
WinStationSetInformationA(
    _In_opt_ HANDLE ServerHandle,
    _In_ ULONG SessionId,
    _In_ WINSTATIONINFOCLASS WinStationInformationClass,
    _In_reads_bytes_(WinStationInformationLength) PVOID WinStationInformation,
    _In_ ULONG WinStationInformationLength
    );

/**
 * The WinStationSetLastWinlogonNotification routine records the Winlogon notification currently blocked by a subscriber.
 *
 * \param[in] NotificationId The Winlogon notification identifier.
 * \param[in] SubscriberName The name of the notification subscriber.
 * \param[in] IsCritical TRUE if the subscriber is marked as critical; otherwise, FALSE.
 * \return Nonzero if the operation succeeds; otherwise, zero.
 */
NTSYSAPI
BOOLEAN
NTAPI
WinStationSetLastWinlogonNotification(
    _In_ ULONG NotificationId,
    _In_ PCWSTR SubscriberName,
    _In_ BOOL IsCritical
    );

// rev
/**
 * The WinStationShadowAccessCheck routine checks whether the caller is permitted to shadow a target session.
 *
 * \param[in] SessionId The session identifier.
 * \param[out] AccessGranted Receives the access granted.
 * \return Nonzero if the operation succeeds; otherwise, zero.
 */
NTSYSAPI
BOOLEAN
NTAPI
WinStationShadowAccessCheck(
    _In_ ULONG SessionId,
    _Out_ PBOOLEAN AccessGranted
    );

/**
 * The WinStationShadowStop2 routine stops the current session-shadowing operation.
 *
 * \return Nonzero if the operation succeeds; otherwise, zero.
 */
NTSYSAPI
BOOLEAN
NTAPI
WinStationShadowStop2(
    VOID
    );

// rev
/**
 * The WinStationSystemShutdownStarted routine notifies Terminal Services that system shutdown has started.
 *
 * \param[in] ShutdownFlags The shutdown flags.
 * \return Nonzero if the operation succeeds; otherwise, zero.
 */
NTSYSAPI
BOOLEAN
NTAPI
WinStationSystemShutdownStarted(
    _In_ ULONG ShutdownFlags
    );

// rev
/**
 * The WinStationSystemShutdownWait routine waits for a pending Terminal Services system-shutdown operation.
 *
 * \param[in] Milliseconds The milliseconds.
 * \param[out] Result Receives the result.
 * \return Nonzero if the operation succeeds; otherwise, zero.
 */
NTSYSAPI
BOOLEAN
NTAPI
WinStationSystemShutdownWait(
    _In_ ULONG Milliseconds,
    _Out_opt_ PULONG Result
    );

// rev
/**
 * The WinStationTerminateGlassReplacementSession routine terminates a glass-replacement session.
 *
 * \param[in] SessionId The session identifier.
 * \return Nonzero if the operation succeeds; otherwise, zero.
 */
NTSYSAPI
BOOLEAN
NTAPI
WinStationTerminateGlassReplacementSession(
    _In_ ULONG SessionId
    );

// rev
/**
 * The WinStationUnRegisterNotificationEvent routine unregisters a notification event registration.
 *
 * \param[in] NotificationHandle The notification handle.
 * \return Nonzero if the operation succeeds; otherwise, zero.
 */
NTSYSAPI
BOOLEAN
NTAPI
WinStationUnRegisterNotificationEvent(
    _In_ HANDLE NotificationHandle
    );

// rev
/**
 * The WinStationUnRegisterSessionNotification routine provides an exported alias for a Terminal Services client operation.
 *
 * \param[in] ServerHandle A handle to the server. NULL specifies the current server.
 * \param[in] WindowHandle A handle to the notification window.
 * \return Nonzero if the operation succeeds; otherwise, zero.
 */
NTSYSAPI
BOOLEAN
NTAPI
WinStationUnRegisterSessionNotification(
    _In_opt_ HANDLE ServerHandle,
    _In_ HWND WindowHandle
    );

/**
 * The WinStationUserLoginAccessCheck routine checks whether a user is permitted to log on through Terminal Services and returns the associated access-check data.
 *
 * \param[in] ServerHandle A handle to the server. NULL specifies the current server.
 * \param[out] Reserved1 An operation-specific reserved parameter.
 * \param[out] Reserved2 An operation-specific reserved parameter.
 * \param[out] Reserved3 An operation-specific reserved parameter.
 * \return Nonzero if the operation succeeds; otherwise, zero.
 */
DECLSPEC_DEPRECATED
NTSYSAPI
BOOLEAN
NTAPI
WinStationUserLoginAccessCheck(
    _In_opt_ HANDLE ServerHandle,
    _Out_ PVOID *Reserved1,
    _Out_ PVOID *Reserved2,
    _Out_ PVOID *Reserved3
    );

// rev
/**
 * The WinStationVerify routine verifies connection properties for a session.
 *
 * \param[in] SessionId The session identifier.
 * \param[in] VerificationType A GUID identifying the verification operation.
 * \param[in] InputPropertyCount The number of input properties.
 * \param[in, optional] InputProperties An array of input property values.
 * \param[in] OutputPropertyCount The number of output property slots.
 * \param[in,out, optional] OutputProperties An array that receives output property values.
 * \return Nonzero if the operation succeeds; otherwise, zero.
 */
NTSYSAPI
BOOLEAN
NTAPI
WinStationVerify(
    _In_ ULONG SessionId,
    _In_ const GUID *VerificationType,
    _In_ ULONG InputPropertyCount,
    _In_reads_opt_(InputPropertyCount) const WINSTATION_PROPERTY_VALUE *InputProperties,
    _In_ ULONG OutputPropertyCount,
    _Inout_updates_opt_(OutputPropertyCount) PWINSTATION_PROPERTY_VALUE OutputProperties
    );

// rev
/**
 * The _NWLogonQueryAdmin routine is a deprecated entry point for querying the legacy NetWare-logon administrator setting.
 *
 * \return Nonzero if the operation succeeds; otherwise, zero.
 * \remarks The current implementation is deprecated and does not perform the operation.
 */
DECLSPEC_DEPRECATED
NTSYSAPI
BOOLEAN
NTAPI
_NWLogonQueryAdmin(
    VOID
    );

// rev
/**
 * The _NWLogonSetAdmin routine is a deprecated entry point for setting the legacy NetWare-logon administrator setting.
 *
 * \return Nonzero if the operation succeeds; otherwise, zero.
 * \remarks The current implementation is deprecated and does not perform the operation.
 */
DECLSPEC_DEPRECATED
NTSYSAPI
BOOLEAN
NTAPI
_NWLogonSetAdmin(
    VOID
    );

// rev
/**
 * The _WinStationAnnoyancePopup routine is a deprecated entry point for displaying the legacy licensing reminder dialog.
 *
 * \param[in] Parameter1 An opaque operation-specific parameter.
 * \param[in] Parameter2 An opaque operation-specific parameter.
 * \return Nonzero if the operation succeeds; otherwise, zero.
 * \remarks The current implementation does not perform the operation; it sets the last-error code to ERROR_INVALID_FUNCTION and returns FALSE.
 */
DECLSPEC_DEPRECATED
NTSYSAPI
BOOLEAN
NTAPI
_WinStationAnnoyancePopup(
    _In_opt_ PVOID Parameter1,
    _In_opt_ PVOID Parameter2
    );

// rev
/**
 * The _WinStationBeepOpen routine provides the legacy WinStation beep-open compatibility entry point.
 *
 * \param[in] Parameter1 An opaque operation-specific parameter.
 * \param[in] Parameter2 An opaque operation-specific parameter.
 * \param[in] Parameter3 An opaque operation-specific parameter.
 * \return Nonzero if the operation succeeds; otherwise, zero.
 */
NTSYSAPI
BOOLEAN
NTAPI
_WinStationBeepOpen(
    _In_opt_ PVOID Parameter1,
    _In_opt_ PVOID Parameter2,
    _In_opt_ PVOID Parameter3
    );

// rev
/**
 * The _WinStationBreakPoint routine is a deprecated entry point for requesting a diagnostic breakpoint in Terminal Services.
 *
 * \param[in] Parameter1 An opaque operation-specific parameter.
 * \param[in] Parameter2 An opaque operation-specific parameter.
 * \param[in] Parameter3 An opaque operation-specific parameter.
 * \return Nonzero if the operation succeeds; otherwise, zero.
 * \remarks The current implementation does not perform the operation; it sets the last-error code to ERROR_INVALID_FUNCTION and returns FALSE.
 */
DECLSPEC_DEPRECATED
NTSYSAPI
BOOLEAN
NTAPI
_WinStationBreakPoint(
    _In_opt_ PVOID Parameter1,
    _In_opt_ PVOID Parameter2,
    _In_opt_ PVOID Parameter3
    );

// rev
/**
 * The _WinStationCallback routine is a deprecated entry point for initiating a legacy WinStation callback operation.
 *
 * \param[in] Parameter1 An opaque operation-specific parameter.
 * \param[in] Parameter2 An opaque operation-specific parameter.
 * \param[in] Parameter3 An opaque operation-specific parameter.
 * \return Nonzero if the operation succeeds; otherwise, zero.
 * \remarks The current implementation does not perform the operation; it sets the last-error code to ERROR_INVALID_FUNCTION and returns FALSE.
 */
DECLSPEC_DEPRECATED
NTSYSAPI
BOOLEAN
NTAPI
_WinStationCallback(
    _In_opt_ PVOID Parameter1,
    _In_opt_ PVOID Parameter2,
    _In_opt_ PVOID Parameter3
    );

// rev
/**
 * The _WinStationCheckForApplicationName routine is a deprecated entry point for checking a configured initial application name.
 *
 * \param[in] Parameter1 An opaque operation-specific parameter.
 * \param[in] Parameter2 An opaque operation-specific parameter.
 * \param[in] Parameter3 An opaque operation-specific parameter.
 * \param[in] Parameter4 An opaque operation-specific parameter.
 * \param[in] Parameter5 An opaque operation-specific parameter.
 * \param[in] Parameter6 An opaque operation-specific parameter.
 * \param[in] Parameter7 An opaque operation-specific parameter.
 * \param[in] Parameter8 An opaque operation-specific parameter.
 * \param[in] Parameter9 An opaque operation-specific parameter.
 * \param[in] Parameter10 An opaque operation-specific parameter.
 * \param[in] Parameter11 An opaque operation-specific parameter.
 * \param[in] Parameter12 An opaque operation-specific parameter.
 * \return Nonzero if the operation succeeds; otherwise, zero.
 * \remarks The current implementation does not perform the operation; it sets the last-error code to ERROR_INVALID_FUNCTION and returns FALSE.
 */
DECLSPEC_DEPRECATED
NTSYSAPI
BOOLEAN
NTAPI
_WinStationCheckForApplicationName(
    _In_opt_ PVOID Parameter1,
    _In_opt_ PVOID Parameter2,
    _In_opt_ PVOID Parameter3,
    _In_opt_ PVOID Parameter4,
    _In_opt_ PVOID Parameter5,
    _In_opt_ PVOID Parameter6,
    _In_opt_ PVOID Parameter7,
    _In_opt_ PVOID Parameter8,
    _In_opt_ PVOID Parameter9,
    _In_opt_ PVOID Parameter10,
    _In_opt_ PVOID Parameter11,
    _In_opt_ PVOID Parameter12
    );

// rev
/**
 * The _WinStationFUSCanRemoteUserDisconnect routine is a deprecated entry point for checking whether Fast User Switching permits disconnection of a remote user.
 *
 * \param[in] Parameter1 An opaque operation-specific parameter.
 * \param[in] Parameter2 An opaque operation-specific parameter.
 * \param[in] Parameter3 An opaque operation-specific parameter.
 * \return Nonzero if the operation succeeds; otherwise, zero.
 * \remarks The current implementation does not perform the operation; it sets the last-error code to ERROR_INVALID_FUNCTION and returns FALSE.
 */
DECLSPEC_DEPRECATED
NTSYSAPI
BOOLEAN
NTAPI
_WinStationFUSCanRemoteUserDisconnect(
    _In_opt_ PVOID Parameter1,
    _In_opt_ PVOID Parameter2,
    _In_opt_ PVOID Parameter3
    );

// rev
/**
 * The _WinStationGetApplicationInfo routine is a deprecated entry point for retrieving application information for a session.
 *
 * \param[in] Parameter1 An opaque operation-specific parameter.
 * \param[in] Parameter2 An opaque operation-specific parameter.
 * \param[in] Parameter3 An opaque operation-specific parameter.
 * \param[in] Parameter4 An opaque operation-specific parameter.
 * \return Nonzero if the operation succeeds; otherwise, zero.
 * \remarks The current implementation does not perform the operation; it sets the last-error code to ERROR_INVALID_FUNCTION and returns FALSE.
 */
DECLSPEC_DEPRECATED
NTSYSAPI
BOOLEAN
NTAPI
_WinStationGetApplicationInfo(
    _In_opt_ PVOID Parameter1,
    _In_opt_ PVOID Parameter2,
    _In_opt_ PVOID Parameter3,
    _In_opt_ PVOID Parameter4
    );

// rev
/**
 * The _WinStationNotifyDisconnectPipe routine is a deprecated entry point for notifying Terminal Services that a session pipe disconnected.
 *
 * \return Nonzero if the operation succeeds; otherwise, zero.
 * \remarks The current implementation does not perform the operation; it sets the last-error code to ERROR_INVALID_FUNCTION and returns FALSE.
 */
DECLSPEC_DEPRECATED
NTSYSAPI
BOOLEAN
NTAPI
_WinStationNotifyDisconnectPipe(
    VOID
    );

// rev
/**
 * The _WinStationNotifyLogoff routine is a deprecated entry point for notifying Terminal Services of session logoff.
 *
 * \return Nonzero if the operation succeeds; otherwise, zero.
 * \remarks The current implementation does not perform the operation; it sets the last-error code to ERROR_INVALID_FUNCTION and returns FALSE.
 */
DECLSPEC_DEPRECATED
NTSYSAPI
BOOLEAN
NTAPI
_WinStationNotifyLogoff(
    VOID
    );

// rev
/**
 * The _WinStationNotifyLogon routine is a deprecated entry point for notifying Terminal Services of session logon.
 *
 * \return Nonzero if the operation succeeds; otherwise, zero.
 * \remarks The current implementation does not perform the operation; it sets the last-error code to ERROR_INVALID_FUNCTION and returns FALSE.
 */
DECLSPEC_DEPRECATED
NTSYSAPI
BOOLEAN
NTAPI
_WinStationNotifyLogon(
    VOID
    );

// rev
/**
 * The _WinStationNotifyNewSession routine is a deprecated entry point for notifying Terminal Services that a new session was created.
 *
 * \param[in] Parameter1 An opaque operation-specific parameter.
 * \param[in] Parameter2 An opaque operation-specific parameter.
 * \return Nonzero if the operation succeeds; otherwise, zero.
 * \remarks The current implementation does not perform the operation; it sets the last-error code to ERROR_INVALID_FUNCTION and returns FALSE.
 */
DECLSPEC_DEPRECATED
NTSYSAPI
BOOLEAN
NTAPI
_WinStationNotifyNewSession(
    _In_opt_ PVOID Parameter1,
    _In_opt_ PVOID Parameter2
    );

// rev
/**
 * The _WinStationOpenSessionDirectory routine is a deprecated entry point for opening the directory associated with a session.
 *
 * \return Nonzero if the operation succeeds; otherwise, zero.
 * \remarks The current implementation does not perform the operation; it sets the last-error code to ERROR_INVALID_FUNCTION and returns FALSE.
 */
DECLSPEC_DEPRECATED
NTSYSAPI
BOOLEAN
NTAPI
_WinStationOpenSessionDirectory(
    VOID
    );

// rev
/**
 * The _WinStationReInitializeSecurity routine is a deprecated entry point for reinitializing Terminal Services security state.
 *
 * \param[in] Parameter1 An opaque operation-specific parameter.
 * \return Nonzero if the operation succeeds; otherwise, zero.
 * \remarks The current implementation does not perform the operation; it sets the last-error code to ERROR_INVALID_FUNCTION and returns FALSE.
 */
DECLSPEC_DEPRECATED
NTSYSAPI
BOOLEAN
NTAPI
_WinStationReInitializeSecurity(
    _In_opt_ PVOID Parameter1
    );

// rev
/**
 * The _WinStationReadRegistry routine provides an exported alias for a Terminal Services client operation.
 *
 * \param[in] Parameter1 An opaque operation-specific parameter (ignored).
 * \return Nonzero if the operation succeeds; otherwise, zero.
 * \remarks The current implementation ignores Parameter1 and unconditionally returns TRUE. It shares
 * its code with _WinStationSessionInitialized.
 */
NTSYSAPI
BOOLEAN
NTAPI
_WinStationReadRegistry(
    _In_opt_ PVOID Parameter1
    );

/**
 * The _WinStationSessionInitialized routine provides an exported alias for a Terminal Services client operation.
 *
 * \return Nonzero if the operation succeeds; otherwise, zero.
 */
DECLSPEC_DEPRECATED
NTSYSAPI
BOOLEAN
NTAPI
_WinStationSessionInitialized(
    VOID
    );

// rev: Terminal Services shadow-target structures. Both are passed across the
// WinStation RPC interface (and forwarded winsta -> termsrv) purely as fixed-size,
// size_is()-marshalled byte buffers; neither winsta.dll nor termsrv.dll dereferences
// their members (the interior layout lives in RegWinStationQueryExW's registry mapping
// and the WSX transport plugin), so byte-exact field offsets are NOT recoverable from
// these binaries. They are declared here as correctly-sized named buffers.

/**
 * The ICA_STACK_ADDRESS structure holds a Terminal Services (ICA) transport stack address.
 * Fixed length STACK_ADDRESS_LENGTH (0x80) bytes. Interior layout is transport-defined.
 */
typedef struct _ICA_STACK_ADDRESS
{
    UCHAR Address[STACK_ADDRESS_LENGTH]; // 0x80
} ICA_STACK_ADDRESS, *PICA_STACK_ADDRESS;

/**
 * The WINSTATIONCONFIG2W structure is the in-memory ("2W") WinStation/listener
 * configuration produced by winsta!RegWinStationQueryExW from the registry. It
 * aggregates the documented sub-config blocks (USERCONFIGW, WDCONFIGW, PDCONFIG2W,
 * CDCONFIGW, WINSTATIONCLIENTW, ...). Fixed length 0x28E8 bytes.
 */
#define WINSTATIONCONFIG2W_LENGTH 0x28E8 // 10472
typedef struct _WINSTATIONCONFIG2W
{
    UCHAR Reserved[WINSTATIONCONFIG2W_LENGTH]; // 0x28E8
} WINSTATIONCONFIG2W, *PWINSTATIONCONFIG2W;

// rev
/**
 * The _WinStationShadowTarget routine configures the target side of a session-shadowing connection.
 *
 * \param[in] hServer The h server.
 * \param[in] SessionId The session identifier.
 * \param[in] pConfig The p config.
 * \param[in] pAddress The p address.
 * \param[in] pModuleData The p module data.
 * \param[in] ModuleDataLength The size of the module data, in bytes.
 * \param[in] pThinwireData The p thinwire data.
 * \param[in] ThinwireDataLength The size of the thinwire data, in bytes.
 * \param[in] pClientName The p client name.
 * \param[in] ClientNameLength The capacity associated with pClientName, in WCHAR elements.
 * \return Nonzero if the operation succeeds; otherwise, zero.
 */
NTSYSAPI
BOOLEAN
NTAPI
_WinStationShadowTarget(
    _In_opt_ HANDLE hServer,
    _In_ ULONG SessionId,
    _In_reads_bytes_(0x28E8) PWINSTATIONCONFIG2W pConfig,
    _In_reads_bytes_(0x80) PICA_STACK_ADDRESS pAddress,
    _In_reads_bytes_(ModuleDataLength) const BYTE *pModuleData,
    _In_ ULONG ModuleDataLength,
    _In_reads_bytes_(ThinwireDataLength) const BYTE *pThinwireData,
    _In_ ULONG ThinwireDataLength,
    _In_reads_opt_(ClientNameLength) PCWSTR pClientName,
    _In_ ULONG ClientNameLength
    );

// rev
/**
 * The _WinStationShadowTarget2 routine configures the target side of a session-shadowing connection using extended shadow data.
 *
 * \param[in] hServer The h server.
 * \param[in] SessionId The session identifier.
 * \param[in] pConfig The p config.
 * \param[in] ConfigSize The size of the config, in bytes.
 * \param[in] pAddress The p address.
 * \param[in] AddressSize The size of the address, in bytes.
 * \param[in] pModuleData The p module data.
 * \param[in] ModuleDataLength The size of the module data, in bytes.
 * \param[in] pThinwireData The p thinwire data.
 * \param[in] ThinwireDataLength The size of the thinwire data, in bytes.
 * \param[in] pClientName The p client name.
 * \param[in] ClientNameLength The capacity associated with pClientName, in WCHAR elements.
 * \return Nonzero if the operation succeeds; otherwise, zero.
 */
NTSYSAPI
BOOLEAN
NTAPI
_WinStationShadowTarget2(
    _In_opt_ HANDLE hServer,
    _In_ ULONG SessionId,
    _In_reads_bytes_(ConfigSize) PWINSTATIONCONFIG2W pConfig,
    _In_ ULONG ConfigSize,
    _In_reads_bytes_(AddressSize) PICA_STACK_ADDRESS pAddress,
    _In_ ULONG AddressSize,
    _In_reads_bytes_(ModuleDataLength) const BYTE *pModuleData,
    _In_ ULONG ModuleDataLength,
    _In_reads_bytes_(ThinwireDataLength) const BYTE *pThinwireData,
    _In_ ULONG ThinwireDataLength,
    _In_reads_opt_(ClientNameLength) PCWSTR pClientName,
    _In_ ULONG ClientNameLength
    );

// rev
/**
 * The _WinStationShadowTargetSetup routine provides an exported alias for a Terminal Services client operation.
 *
 * \param[in] Parameter1 An opaque operation-specific parameter.
 * \param[in] Parameter2 An opaque operation-specific parameter.
 * \return Nonzero if the operation succeeds; otherwise, zero.
 */
NTSYSAPI
BOOLEAN
NTAPI
_WinStationShadowTargetSetup(
    _In_opt_ PVOID Parameter1,
    _In_opt_ PVOID Parameter2
    );

// rev
/**
 * The _WinStationUpdateClientCachedCredentials routine is a deprecated entry point for updating cached client credentials for a session.
 *
 * \param[in] Parameter1 An opaque operation-specific parameter.
 * \param[in] Parameter2 An opaque operation-specific parameter.
 * \param[in] Parameter3 An opaque operation-specific parameter.
 * \param[in] Parameter4 An opaque operation-specific parameter.
 * \param[in] Parameter5 An opaque operation-specific parameter.
 * \param[in] Parameter6 An opaque operation-specific parameter.
 * \param[in] Parameter7 An opaque operation-specific parameter.
 * \return Nonzero if the operation succeeds; otherwise, zero.
 * \remarks The current implementation does not perform the operation; it sets the last-error code to ERROR_INVALID_FUNCTION and returns FALSE.
 */
DECLSPEC_DEPRECATED
NTSYSAPI
BOOLEAN
NTAPI
_WinStationUpdateClientCachedCredentials(
    _In_opt_ PVOID Parameter1,
    _In_opt_ PVOID Parameter2,
    _In_opt_ PVOID Parameter3,
    _In_opt_ PVOID Parameter4,
    _In_opt_ PVOID Parameter5,
    _In_opt_ PVOID Parameter6,
    _In_opt_ PVOID Parameter7
    );

// rev
/**
 * The _WinStationUpdateSettings routine is a deprecated entry point for updating WinStation settings.
 *
 * \param[in] Parameter1 An opaque operation-specific parameter.
 * \param[in] Parameter2 An opaque operation-specific parameter.
 * \param[in] Parameter3 An opaque operation-specific parameter.
 * \return Nonzero if the operation succeeds; otherwise, zero.
 * \remarks The current implementation does not perform the operation; it sets the last-error code to ERROR_INVALID_FUNCTION and returns FALSE.
 */
DECLSPEC_DEPRECATED
NTSYSAPI
BOOLEAN
NTAPI
_WinStationUpdateSettings(
    _In_opt_ PVOID Parameter1,
    _In_opt_ PVOID Parameter2,
    _In_opt_ PVOID Parameter3
    );

// rev
/**
 * The _WinStationUpdateUserConfig routine is a deprecated entry point for updating a user's WinStation configuration.
 *
 * \param[in] Parameter1 An opaque operation-specific parameter.
 * \return Nonzero if the operation succeeds; otherwise, zero.
 * \remarks The current implementation does not perform the operation; it sets the last-error code to ERROR_INVALID_FUNCTION and returns FALSE.
 */
DECLSPEC_DEPRECATED
NTSYSAPI
BOOLEAN
NTAPI
_WinStationUpdateUserConfig(
    _In_opt_ PVOID Parameter1
    );

// rev
/**
 * The _WinStationWaitForConnectEx routine waits for a WinStation connection using extended connection parameters.
 *
 * \param[in] ConnectionGuid Pointer to the GUID identifying the connection to wait for. The call
 * waits for the Local Session Manager to start and forwards this GUID to lsm!RpcConnectTerminalEx.
 * \return Nonzero if the operation succeeds; otherwise, zero.
 */
NTSYSAPI
BOOLEAN
NTAPI
_WinStationWaitForConnectEx(
    _In_ PGUID ConnectionGuid
    );

#endif
