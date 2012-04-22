#ifndef __FIREWALL_H_
#define __FIREWALL_H_

#define STRICT
#define WIN32_LEAN_AND_MEAN

#include <Windows.h>
#include <WindowsX.h>
#include <wchar.h>

//http://msdn.microsoft.com/en-us/library/cc231461.aspx

//Protocol Versions: This protocol has only one interface version. 
//There are currently three policy versions, which can be tied to policies and specific policy objects, as defined in section 2.2. 
//The three versions are 0x0200, 0x0201, and 0x020A.
//<1> Protocol Versions are used as Binary Versions and Schema Versions (also called policy versions).

//Windows Vista uses version 0x0200. 
//Windows Vista SP1 and Windows Server 2008 use version 0x0201. 
//Windows 7 and Windows Server 2008 R2 use version 0x020A.

#define FW_VISTA_BINARY_VERSION 0x0200 //(FW_VERSION(2,0))
#define FW_VISTA_SCHEMA_VERSION 0x0200 //(FW_VERSION(2,0))
#define FW_SERVER2K8_BINARY_VERSION 0x0201
#define FW_SERVER2K8_SCHEMA_VERSION 0x0201
#define FW_SEVEN_BINARY_VERSION 0x020A
#define FW_SEVEN_SCHEMA_VERSION 0x020A

#define FW_ICMP_CODE_ANY (256)
#define FW_IP_PROTOCOL_ANY (256)

#define FW_PROFILE_CONFIG_LOG_FILE_SIZE_MIN 1
#define FW_PROFILE_CONFIG_LOG_FILE_SIZE_MAX 32767

#define FW_GLOBAL_CONFIG_CRL_CHECK_MAX 2
#define FW_GLOBAL_CONFIG_SA_IDLE_TIME_MAX 3600
#define FW_GLOBAL_CONFIG_SA_IDLE_TIME_MIN 300

typedef HANDLE FW_CONN_HANDLE;
typedef HANDLE FW_POLICY_STORE_HANDLE;
typedef /*[ref]*/ FW_POLICY_STORE_HANDLE *PFW_POLICY_STORE_HANDLE;
typedef HANDLE FW_PRODUCT_HANDLE;

typedef enum _FW_POLICY_STORE // custom enum by dmex
{
    FW_POLICY_STORE_MAIN = 0,
    FW_POLICY_STORE_GPRSOP = 1,
    FW_POLICY_STORE_DYNAMIC = 2,
    FW_POLICY_STORE_DEFAULT = 3
} FW_POLICY_STORE;

typedef enum _FW_POLICY_STORE_FLAGS // custom enum by dmex
{
    FW_POLICY_STORE_FLAGS_NONE,
    FW_POLICY_STORE_FLAGS_DELETE_DYNAMIC_RULES_AFTER_CLOSE,
    FW_POLICY_STORE_FLAGS_MAX
} FW_POLICY_STORE_FLAGS;

typedef enum _tag_FW_STORE_TYPE
{
    FW_STORE_TYPE_INVALID,
    FW_STORE_TYPE_GP_RSOP,
    FW_STORE_TYPE_LOCAL,
    FW_STORE_TYPE_NOT_USED_VALUE_3,
    FW_STORE_TYPE_NOT_USED_VALUE_4,
    FW_STORE_TYPE_DYNAMIC,
    FW_STORE_TYPE_GPO,
    FW_STORE_TYPE_DEFAULTS,
    FW_STORE_TYPE_MAX
} FW_STORE_TYPE;

typedef enum _tag_FW_PROFILE_TYPE //[v1_enum]
{
    FW_PROFILE_TYPE_INVALID  = 0,
    FW_PROFILE_TYPE_DOMAIN   = 0x001,
    FW_PROFILE_TYPE_STANDARD = 0x002,
    FW_PROFILE_TYPE_PRIVATE  = FW_PROFILE_TYPE_STANDARD,
    FW_PROFILE_TYPE_PUBLIC   = 0x004,
    FW_PROFILE_TYPE_ALL      = 0x7FFFFFFF,
    FW_PROFILE_TYPE_CURRENT  = 0x80000000,
    FW_PROFILE_TYPE_NONE     = FW_PROFILE_TYPE_CURRENT + 1
} FW_PROFILE_TYPE;

typedef enum _tag_FW_POLICY_ACCESS_RIGHT
{
    FW_POLICY_ACCESS_RIGHT_INVALID,
    FW_POLICY_ACCESS_RIGHT_READ,
    FW_POLICY_ACCESS_RIGHT_READ_WRITE,
    FW_POLICY_ACCESS_RIGHT_MAX
} FW_POLICY_ACCESS_RIGHT;


typedef struct _POLICYSTORE // dmex
{
    FW_STORE_TYPE StoreType;
    FW_POLICY_ACCESS_RIGHT AccessRight;
    BOOLEAN UseLocalMachine;
} POLICYSTORE, *LPPOLICYSTORE;



typedef struct _tag_FW_IPV4_SUBNET
{
    ULONG dwAddress;
    ULONG dwSubNetMask;
} FW_IPV4_SUBNET, *PFW_IPV4_SUBNET;

typedef struct _tag_FW_IPV4_SUBNET_LIST
{
    //[range(0, 1000)]
    ULONG dwNumEntries;
    //[size_is(dwNumEntries)]
    __ecount(dwNumEntries) 
    PFW_IPV4_SUBNET pSubNets;
} FW_IPV4_SUBNET_LIST, *PFW_IPV4_SUBNET_LIST;

typedef struct _tag_FW_IPV6_SUBNET
{
    UCHAR Address[16];
    //[range(0, 128)]
    ULONG dwNumPrefixBits;
} FW_IPV6_SUBNET, *PFW_IPV6_SUBNET;

typedef struct _tag_FW_IPV6_SUBNET_LIST
{
    //[range(0, 1000)]
    ULONG dwNumEntries;
    //[size_is(dwNumEntries)]
    __ecount(dwNumEntries) 
    PFW_IPV6_SUBNET pSubNets;
} FW_IPV6_SUBNET_LIST, *PFW_IPV6_SUBNET_LIST;

typedef struct _tag_FW_IPV4_ADDRESS_RANGE
{
    ULONG dwBegin;
    ULONG dwEnd;
} FW_IPV4_ADDRESS_RANGE, *PFW_IPV4_ADDRESS_RANGE;

typedef struct _tag_FW_IPV6_ADDRESS_RANGE
{
    UCHAR Begin[16];
    UCHAR End[16];
} FW_IPV6_ADDRESS_RANGE, *PFW_IPV6_ADDRESS_RANGE;

typedef struct _tag_FW_IPV4_RANGE_LIST
{
    //[range(0, 1000)]
    ULONG dwNumEntries;
    //[size_is(dwNumEntries)]
    __ecount(dwNumEntries) 
    PFW_IPV4_ADDRESS_RANGE pRanges;
} FW_IPV4_RANGE_LIST, *PFW_IPV4_RANGE_LIST;

typedef struct _tag_FW_IPV6_RANGE_LIST
{
    //[range(0, 1000)]
    ULONG dwNumEntries;
    //[size_is(dwNumEntries)]
    __ecount(dwNumEntries) 
    PFW_IPV6_ADDRESS_RANGE pRanges;
} FW_IPV6_RANGE_LIST, *PFW_IPV6_RANGE_LIST;

typedef struct _tag_FW_PORT_RANGE
{
    USHORT wBegin;
    USHORT wEnd;
} FW_PORT_RANGE, *PFW_PORT_RANGE;

typedef struct _tag_FW_PORT_RANGE_LIST
{
    //[range(0, 1000)]
    ULONG dwNumEntries;
    //[size_is(dwNumEntries)]
    __ecount(dwNumEntries) 
    PFW_PORT_RANGE pPorts;
} FW_PORT_RANGE_LIST, *PFW_PORT_RANGE_LIST;

typedef enum _tag_FW_PORT_KEYWORD
{
    FW_PORT_KEYWORD_NONE              = 0x00,
    FW_PORT_KEYWORD_DYNAMIC_RPC_PORTS = 0x01,
    FW_PORT_KEYWORD_RPC_EP            = 0x02,
    FW_PORT_KEYWORD_TEREDO_PORT       = 0x04,
    FW_PORT_KEYWORD_IP_TLS_IN         = 0x08,
    FW_PORT_KEYWORD_IP_TLS_OUT        = 0x10,
    FW_PORT_KEYWORD_MAX               = 0x20,
    FW_PORT_KEYWORD_MAX_V2_1          = 0x08
} FW_PORT_KEYWORD;

typedef struct _tag_FW_PORTS
{
    USHORT wPortKeywords;
    FW_PORT_RANGE_LIST Ports;
} FW_PORTS,*PFW_PORTS;

typedef struct _tag_FW_ICMP_TYPE_CODE
{
    UCHAR bType;
    //[range(0, 256)]
    USHORT wCode;
} FW_ICMP_TYPE_CODE, *PFW_ICMP_TYPE_CODE;

typedef struct _tag_FW_ICMP_TYPE_CODE_LIST
{
    //[range(0, 1000)]
    ULONG dwNumEntries;
    //[size_is(dwNumEntries)]
     __ecount(dwNumEntries)
     PFW_ICMP_TYPE_CODE pEntries;
} FW_ICMP_TYPE_CODE_LIST, *PFW_ICMP_TYPE_CODE_LIST;

typedef struct _tag_FW_INTERFACE_LUIDS
{
    //[range(0, 1000)]
    ULONG dwNumLUIDs;
    //[size_is(dwNumLUIDs)]
    GUID* pLUIDs;
} FW_INTERFACE_LUIDS, *PFW_INTERFACE_LUIDS;

typedef enum _tag_FW_DIRECTION
{
    FW_DIR_INVALID = 0,
    FW_DIR_IN = 1,
    FW_DIR_OUT = 2,
    FW_DIR_BOTH = 3 // MAX
} FW_DIRECTION;

typedef enum _tag_FW_INTERFACE_TYPE
{
    FW_INTERFACE_TYPE_ALL            = 0x0000,
    FW_INTERFACE_TYPE_LAN            = 0x0001,
    FW_INTERFACE_TYPE_WIRELESS       = 0x0002,
    FW_INTERFACE_TYPE_REMOTE_ACCESS  = 0x0004,
    FW_INTERFACE_TYPE_MAX            = 0x0008
} FW_INTERFACE_TYPE;

typedef enum _tag_FW_ADDRESS_KEYWORD
{
    FW_ADDRESS_KEYWORD_NONE            = 0x0000,
    FW_ADDRESS_KEYWORD_LOCAL_SUBNET    = 0x0001,
    FW_ADDRESS_KEYWORD_DNS             = 0x0002,
    FW_ADDRESS_KEYWORD_DHCP            = 0x0004,
    FW_ADDRESS_KEYWORD_WINS            = 0x0008,
    FW_ADDRESS_KEYWORD_DEFAULT_GATEWAY = 0x0010,
    FW_ADDRESS_KEYWORD_MAX             = 0x0020
}FW_ADDRESS_KEYWORD;

typedef struct _tag_FW_ADDRESSES
{
    ULONG dwV4AddressKeywords;
    ULONG dwV6AddressKeywords;
    FW_IPV4_SUBNET_LIST V4SubNets;
    FW_IPV4_RANGE_LIST V4Ranges;
    FW_IPV6_SUBNET_LIST V6SubNets;
    FW_IPV6_RANGE_LIST V6Ranges;
}FW_ADDRESSES, *PFW_ADDRESSES;

typedef enum _tag_FW_RULE_STATUS //[v1_enum]
{
    FW_RULE_STATUS_OK                                    = 0x00010000,
    FW_RULE_STATUS_PARTIALLY_IGNORED                     = 0x00020000,
    FW_RULE_STATUS_IGNORED                               = 0x00040000,
    FW_RULE_STATUS_PARSING_ERROR_NAME                    = 0x00080001,
    FW_RULE_STATUS_PARSING_ERROR_DESC                    = 0x00080002,
    FW_RULE_STATUS_PARSING_ERROR_APP                     = 0x00080003,
    FW_RULE_STATUS_PARSING_ERROR_SVC                     = 0x00080004,
    FW_RULE_STATUS_PARSING_ERROR_RMA                     = 0x00080005,
    FW_RULE_STATUS_PARSING_ERROR_RUA                     = 0x00080006,
    FW_RULE_STATUS_PARSING_ERROR_EMBD                    = 0x00080007,
    FW_RULE_STATUS_PARSING_ERROR_RULE_ID                 = 0x00080008,
    FW_RULE_STATUS_PARSING_ERROR_PHASE1_AUTH             = 0x00080009,
    FW_RULE_STATUS_PARSING_ERROR_PHASE2_CRYPTO           = 0x0008000A,    
    FW_RULE_STATUS_PARSING_ERROR_REMOTE_ENDPOINTS        = 0x0008000F,    
    FW_RULE_STATUS_PARSING_ERROR_REMOTE_ENDPOINT_FQDN    = 0x00080010,    
    FW_RULE_STATUS_PARSING_ERROR_KEY_MODULE              = 0x00080011,
    FW_RULE_STATUS_PARSING_ERROR_PHASE2_AUTH             = 0x0008000B,
    FW_RULE_STATUS_PARSING_ERROR_RESOLVE_APP             = 0x0008000C,
    FW_RULE_STATUS_PARSING_ERROR_MAINMODE_ID             = 0x0008000D,
    FW_RULE_STATUS_PARSING_ERROR_PHASE1_CRYPTO           = 0x0008000E,
    FW_RULE_STATUS_PARSING_ERROR                         = 0x00080000,
    FW_RULE_STATUS_SEMANTIC_ERROR_RULE_ID                = 0x00100010,
    FW_RULE_STATUS_SEMANTIC_ERROR_PORTS                  = 0x00100020,
    FW_RULE_STATUS_SEMANTIC_ERROR_PORT_KEYW              = 0x00100021,
    FW_RULE_STATUS_SEMANTIC_ERROR_PORT_RANGE             = 0x00100022,
    FW_RULE_STATUS_SEMANTIC_ERROR_ADDR_V4_SUBNETS        = 0x00100040,
    FW_RULE_STATUS_SEMANTIC_ERROR_ADDR_V6_SUBNETS        = 0x00100041,
    FW_RULE_STATUS_SEMANTIC_ERROR_ADDR_V4_RANGES         = 0x00100042,
    FW_RULE_STATUS_SEMANTIC_ERROR_ADDR_V6_RANGES         = 0x00100043,
    FW_RULE_STATUS_SEMANTIC_ERROR_ADDR_RANGE             = 0x00100044,
    FW_RULE_STATUS_SEMANTIC_ERROR_ADDR_MASK              = 0x00100045,
    FW_RULE_STATUS_SEMANTIC_ERROR_ADDR_PREFIX            = 0x00100046,
    FW_RULE_STATUS_SEMANTIC_ERROR_ADDR_KEYW              = 0x00100047,
    FW_RULE_STATUS_SEMANTIC_ERROR_LADDR_PROP             = 0x00100048,
    FW_RULE_STATUS_SEMANTIC_ERROR_RADDR_PROP             = 0x00100049,
    FW_RULE_STATUS_SEMANTIC_ERROR_ADDR_V6                = 0x0010004A,
    FW_RULE_STATUS_SEMANTIC_ERROR_LADDR_INTF             = 0x0010004B,
    FW_RULE_STATUS_SEMANTIC_ERROR_ADDR_V4                = 0x0010004C,
    FW_RULE_STATUS_SEMANTIC_ERROR_TUNNEL_ENDPOINT_ADDR   = 0x0010004D,
    FW_RULE_STATUS_SEMANTIC_ERROR_DTE_VER                = 0x0010004E,
    FW_RULE_STATUS_SEMANTIC_ERROR_DTE_MISMATCH_ADDR      = 0x0010004F,
    FW_RULE_STATUS_SEMANTIC_ERROR_PROFILE                = 0x00100050,
    FW_RULE_STATUS_SEMANTIC_ERROR_ICMP                   = 0x00100060,
    FW_RULE_STATUS_SEMANTIC_ERROR_ICMP_CODE              = 0x00100061,
    FW_RULE_STATUS_SEMANTIC_ERROR_IF_ID                  = 0x00100070,
    FW_RULE_STATUS_SEMANTIC_ERROR_IF_TYPE                = 0x00100071,
    FW_RULE_STATUS_SEMANTIC_ERROR_ACTION                 = 0x00100080,
    FW_RULE_STATUS_SEMANTIC_ERROR_ALLOW_BYPASS           = 0x00100081,
    FW_RULE_STATUS_SEMANTIC_ERROR_DO_NOT_SECURE          = 0x00100082,
    FW_RULE_STATUS_SEMANTIC_ERROR_ACTION_BLOCK_IS_ENCRYPTED_SECURE = 0x00100083,
    FW_RULE_STATUS_SEMANTIC_ERROR_DIR                    = 0x00100090,
    FW_RULE_STATUS_SEMANTIC_ERROR_PROT                   = 0x001000A0,
    FW_RULE_STATUS_SEMANTIC_ERROR_PROT_PROP              = 0x001000A1,
    FW_RULE_STATUS_SEMANTIC_ERROR_DEFER_EDGE_PROP        = 0x001000A2,
    FW_RULE_STATUS_SEMANTIC_ERROR_ALLOW_BYPASS_OUTBOUND  = 0x001000A3,
    FW_RULE_STATUS_SEMANTIC_ERROR_DEFER_USER_INVALID_RULE = 0x001000A4,
    FW_RULE_STATUS_SEMANTIC_ERROR_FLAGS                  = 0x001000B0,
    FW_RULE_STATUS_SEMANTIC_ERROR_FLAGS_AUTO_AUTH        = 0x001000B1,
    FW_RULE_STATUS_SEMANTIC_ERROR_FLAGS_AUTO_BLOCK       = 0x001000B2,
    FW_RULE_STATUS_SEMANTIC_ERROR_FLAGS_AUTO_DYN_RPC     = 0x001000B3,
    FW_RULE_STATUS_SEMANTIC_ERROR_FLAGS_AUTHENTICATE_ENCRYPT = 0x001000B4,
    FW_RULE_STATUS_SEMANTIC_ERROR_FLAGS_AUTH_WITH_ENC_NEGOTIATE_VER = 0x001000B5,
    FW_RULE_STATUS_SEMANTIC_ERROR_FLAGS_AUTH_WITH_ENC_NEGOTIATE = 0x001000B6,
    FW_RULE_STATUS_SEMANTIC_ERROR_FLAGS_ESP_NO_ENCAP_VER = 0x001000B7,
    FW_RULE_STATUS_SEMANTIC_ERROR_FLAGS_ESP_NO_ENCAP = 0x001000B8,
    FW_RULE_STATUS_SEMANTIC_ERROR_FLAGS_TUNNEL_AUTH_MODES_VER = 0x001000B9,
    FW_RULE_STATUS_SEMANTIC_ERROR_FLAGS_TUNNEL_AUTH_MODES = 0x001000BA,
    FW_RULE_STATUS_SEMANTIC_ERROR_FLAGS_IP_TLS_VER = 0x001000BB,
    FW_RULE_STATUS_SEMANTIC_ERROR_FLAGS_PORTRANGE_VER = 0x001000BC,
    FW_RULE_STATUS_SEMANTIC_ERROR_FLAGS_ADDRS_TRAVERSE_DEFER_VER = 0x001000BD,
    FW_RULE_STATUS_SEMANTIC_ERROR_FLAGS_AUTH_WITH_ENC_NEGOTIATE_OUTBOUND = 0x001000BE,
    FW_RULE_STATUS_SEMANTIC_ERROR_FLAGS_AUTHENTICATE_WITH_OUTBOUND_BYPASS_VER = 0x001000BF,
    FW_RULE_STATUS_SEMANTIC_ERROR_REMOTE_AUTH_LIST       = 0x001000C0, 
    FW_RULE_STATUS_SEMANTIC_ERROR_REMOTE_USER_LIST       = 0x001000C1,
    FW_RULE_STATUS_SEMANTIC_ERROR_PLATFORM               = 0x001000E0,    
    FW_RULE_STATUS_SEMANTIC_ERROR_PLATFORM_OP_VER        = 0x001000E1,    
    FW_RULE_STATUS_SEMANTIC_ERROR_PLATFORM_OP            = 0x001000E2, 
    FW_RULE_STATUS_SEMANTIC_ERROR_DTE_NOANY_ADDR         = 0x001000F0,    
    FW_RULE_STATUS_SEMANTIC_TUNNEL_EXEMPT_WITH_GATEWAY   = 0x001000F1,    
    FW_RULE_STATUS_SEMANTIC_TUNNEL_EXEMPT_VER            = 0x001000F2,
    FW_RULE_STATUS_SEMANTIC_ERROR_PHASE1_AUTH_SET_ID     = 0x00100500,
    FW_RULE_STATUS_SEMANTIC_ERROR_PHASE2_CRYPTO_SET_ID   = 0x00100510,
    FW_RULE_STATUS_SEMANTIC_ERROR_PHASE1_CRYPTO_SET_ID   = 0x00100511,
    FW_RULE_STATUS_SEMANTIC_ERROR_SET_ID                 = 0x00101000,
    FW_RULE_STATUS_SEMANTIC_ERROR_IPSEC_PHASE            = 0x00101010,
    FW_RULE_STATUS_SEMANTIC_ERROR_EMPTY_SUITES           = 0x00101020,
    FW_RULE_STATUS_SEMANTIC_ERROR_PHASE1_AUTH_METHOD     = 0x00101030,
    FW_RULE_STATUS_SEMANTIC_ERROR_PHASE2_AUTH_METHOD     = 0x00101031,
    FW_RULE_STATUS_SEMANTIC_ERROR_AUTH_METHOD_ANONYMOUS  = 0x00101032,
    FW_RULE_STATUS_SEMANTIC_ERROR_AUTH_METHOD_DUPLICATE  = 0x00101033,    
    FW_RULE_STATUS_SEMANTIC_ERROR_AUTH_METHOD_VER        = 0x00101034,
    FW_RULE_STATUS_SEMANTIC_ERROR_AUTH_SUITE_FLAGS       = 0x00101040,
    FW_RULE_STATUS_SEMANTIC_ERROR_HEALTH_CERT            = 0x00101041,
    FW_RULE_STATUS_SEMANTIC_ERROR_AUTH_SIGNCERT_VER      = 0x00101042,
    FW_RULE_STATUS_SEMANTIC_ERROR_AUTH_INTERMEDIATE_CA_VER = 0x00101043,
    FW_RULE_STATUS_SEMANTIC_ERROR_MACHINE_SHKEY          = 0x00101050,
    FW_RULE_STATUS_SEMANTIC_ERROR_CA_NAME                = 0x00101060,
    FW_RULE_STATUS_SEMANTIC_ERROR_MIXED_CERTS            = 0x00101061,
    FW_RULE_STATUS_SEMANTIC_ERROR_NON_CONTIGUOUS_CERTS   = 0x00101062,
    FW_RULE_STATUS_SEMANTIC_ERROR_MIXED_CA_TYPE_IN_BLOCK = 0x00101063,
    FW_RULE_STATUS_SEMANTIC_ERROR_MACHINE_USER_AUTH      = 0x00101070,
    FW_RULE_STATUS_SEMANTIC_ERROR_PHASE1_CRYPTO_NON_DEFAULT_ID = 0x00105000,
    FW_RULE_STATUS_SEMANTIC_ERROR_PHASE1_CRYPTO_FLAGS    = 0x00105001,
    FW_RULE_STATUS_SEMANTIC_ERROR_PHASE1_CRYPTO_TIMEOUT_MINUTES = 0x00105002,
    FW_RULE_STATUS_SEMANTIC_ERROR_PHASE1_CRYPTO_TIMEOUT_SESSIONS = 0x00105003,
    FW_RULE_STATUS_SEMANTIC_ERROR_PHASE1_CRYPTO_KEY_EXCHANGE = 0x00105004,
    FW_RULE_STATUS_SEMANTIC_ERROR_PHASE1_CRYPTO_ENCRYPTION = 0x00105005,
    FW_RULE_STATUS_SEMANTIC_ERROR_PHASE1_CRYPTO_HASH = 0x00105006,
    FW_RULE_STATUS_SEMANTIC_ERROR_PHASE1_CRYPTO_ENCRYPTION_VER = 0x00105007,
    FW_RULE_STATUS_SEMANTIC_ERROR_PHASE1_CRYPTO_HASH_VER = 0x00105008,
    FW_RULE_STATUS_SEMANTIC_ERROR_PHASE2_CRYPTO_PFS = 0x00105020,
    FW_RULE_STATUS_SEMANTIC_ERROR_PHASE2_CRYPTO_PROTOCOL = 0x00105021,
    FW_RULE_STATUS_SEMANTIC_ERROR_PHASE2_CRYPTO_ENCRYPTION = 0x00105022,
    FW_RULE_STATUS_SEMANTIC_ERROR_PHASE2_CRYPTO_HASH = 0x00105023,
    FW_RULE_STATUS_SEMANTIC_ERROR_PHASE2_CRYPTO_TIMEOUT_MINUTES = 0x00105024,
    FW_RULE_STATUS_SEMANTIC_ERROR_PHASE2_CRYPTO_TIMEOUT_KBYTES = 0x00105025,
    FW_RULE_STATUS_SEMANTIC_ERROR_PHASE2_CRYPTO_ENCRYPTION_VER = 0x00105026,
    FW_RULE_STATUS_SEMANTIC_ERROR_PHASE2_CRYPTO_HASH_VER = 0x00105027,
    FW_RULE_STATUS_SEMANTIC_ERROR_QUERY_OR_AND_CONDITIONS = 0x00106000,
    FW_RULE_STATUS_SEMANTIC_ERROR_QUERY_AND_CONDITIONS = 0x00106001,
    FW_RULE_STATUS_SEMANTIC_ERROR_QUERY_CONDITION_KEY = 0x00106002, 
    FW_RULE_STATUS_SEMANTIC_ERROR_QUERY_CONDITION_MATCH_TYPE = 0x00106003,
    FW_RULE_STATUS_SEMANTIC_ERROR_QUERY_CONDITION_DATA_TYPE  = 0x00106004,
    FW_RULE_STATUS_SEMANTIC_ERROR_QUERY_CONDITION_KEY_AND_DATA_TYPE = 0x00106005,
    FW_RULE_STATUS_SEMANTIC_ERROR_QUERY_KEYS_PROTOCOL_PORT = 0x00106006,
    FW_RULE_STATUS_SEMANTIC_ERROR_QUERY_KEY_PROFILE = 0x00106007,
    FW_RULE_STATUS_SEMANTIC_ERROR_QUERY_KEY_STATUS = 0x00106008,
    FW_RULE_STATUS_SEMANTIC_ERROR_QUERY_KEY_FILTERID = 0x00106009,
    FW_RULE_STATUS_SEMANTIC_ERROR_QUERY_KEY_APP_PATH = 0x00106010,
    FW_RULE_STATUS_SEMANTIC_ERROR_QUERY_KEY_PROTOCOL = 0x00106011,
    FW_RULE_STATUS_SEMANTIC_ERROR_QUERY_KEY_LOCAL_PORT = 0x00106012,
    FW_RULE_STATUS_SEMANTIC_ERROR_QUERY_KEY_REMOTE_PORT = 0x00106013,
    FW_RULE_STATUS_SEMANTIC_ERROR_QUERY_KEY_SVC_NAME = 0x00106015,    
    FW_RULE_STATUS_SEMANTIC_ERROR_REQUIRE_IN_CLEAR_OUT_ON_TRANSPORT = 0x00107000,   
    FW_RULE_STATUS_SEMANTIC_ERROR_TUNNEL_BYPASS_TUNNEL_IF_SECURE_ON_TRANSPORT = 0x00107001,    
    FW_RULE_STATUS_SEMANTIC_ERROR_AUTH_NOENCAP_ON_TUNNEL    = 0x00107002,    
    FW_RULE_STATUS_SEMANTIC_ERROR_AUTH_NOENCAP_ON_PSK       = 0x00107003,
    FW_RULE_STATUS_SEMANTIC_ERROR_CRYPTO_ENCR_HASH          = 0x00105040,
    FW_RULE_STATUS_SEMANTIC_ERROR_CRYPTO_ENCR_HASH_COMPAT   = 0x00105041,
    FW_RULE_STATUS_SEMANTIC_ERROR_SCHEMA_VERSION            = 0x00105050,
    FW_RULE_STATUS_SEMANTIC_ERROR                           = 0x00100000,
    FW_RULE_STATUS_RUNTIME_ERROR_PHASE1_AUTH_NOT_FOUND      = 0x00200001,
    FW_RULE_STATUS_RUNTIME_ERROR_PHASE2_AUTH_NOT_FOUND      = 0x00200002,
    FW_RULE_STATUS_RUNTIME_ERROR_PHASE2_CRYPTO_NOT_FOUND    = 0x00200003,
    FW_RULE_STATUS_RUNTIME_ERROR_AUTH_MCHN_SHKEY_MISMATCH   = 0x00200004,
    FW_RULE_STATUS_RUNTIME_ERROR_PHASE1_CRYPTO_NOT_FOUND    = 0x00200005,    
    FW_RULE_STATUS_RUNTIME_ERROR_AUTH_NOENCAP_ON_TUNNEL     = 0x00200006,    
    FW_RULE_STATUS_RUNTIME_ERROR_AUTH_NOENCAP_ON_PSK        = 0x00200007,
    FW_RULE_STATUS_RUNTIME_ERROR                            = 0x00200000,
    FW_RULE_STATUS_ERROR                                    = FW_RULE_STATUS_PARSING_ERROR | FW_RULE_STATUS_SEMANTIC_ERROR | FW_RULE_STATUS_RUNTIME_ERROR,
    FW_RULE_STATUS_ALL                                      = 0xFFFF0000
} FW_RULE_STATUS;

typedef enum _tag_FW_RULE_STATUS_CLASS
{
    FW_RULE_STATUS_CLASS_OK                 = FW_RULE_STATUS_OK,
    FW_RULE_STATUS_CLASS_PARTIALLY_IGNORED  = FW_RULE_STATUS_PARTIALLY_IGNORED,
    FW_RULE_STATUS_CLASS_IGNORED            = FW_RULE_STATUS_IGNORED,
    FW_RULE_STATUS_CLASS_PARSING_ERROR      = FW_RULE_STATUS_PARSING_ERROR,
    FW_RULE_STATUS_CLASS_SEMANTIC_ERROR     = FW_RULE_STATUS_SEMANTIC_ERROR,
    FW_RULE_STATUS_CLASS_RUNTIME_ERROR      = FW_RULE_STATUS_RUNTIME_ERROR,
    FW_RULE_STATUS_CLASS_ERROR              = FW_RULE_STATUS_ERROR,
    FW_RULE_STATUS_CLASS_ALL                = FW_RULE_STATUS_ALL
} FW_RULE_STATUS_CLASS;

typedef enum _tag_FW_OBJECT_CTRL_FLAG
{                          
    FW_OBJECT_CTRL_FLAG_INCLUDE_METADATA = 0x0001,
} FW_OBJECT_CTRL_FLAG;

typedef enum _tag_FW_ENFORCEMENT_STATE
{
    FW_ENFORCEMENT_STATE_INVALID,
    FW_ENFORCEMENT_STATE_FULL,
    FW_ENFORCEMENT_STATE_WF_OFF_IN_PROFILE,
    FW_ENFORCEMENT_STATE_CATEGORY_OFF,
    FW_ENFORCEMENT_STATE_DISABLED_OBJECT,
    FW_ENFORCEMENT_STATE_INACTIVE_PROFILE,
    FW_ENFORCEMENT_STATE_LOCAL_ADDRESS_RESOLUTION_EMPTY,
    FW_ENFORCEMENT_STATE_REMOTE_ADDRESS_RESOLUTION_EMPTY,
    FW_ENFORCEMENT_STATE_LOCAL_PORT_RESOLUTION_EMPTY,
    FW_ENFORCEMENT_STATE_REMOTE_PORT_RESOLUTION_EMPTY,
    FW_ENFORCEMENT_STATE_INTERFACE_RESOLUTION_EMPTY,
    FW_ENFORCEMENT_STATE_APPLICATION_RESOLUTION_EMPTY,
    FW_ENFORCEMENT_STATE_REMOTE_MACHINE_EMPTY,
    FW_ENFORCEMENT_STATE_REMOTE_USER_EMPTY,
    FW_ENFORCEMENT_STATE_LOCAL_GLOBAL_OPEN_PORTS_DISALLOWED,
    FW_ENFORCEMENT_STATE_LOCAL_AUTHORIZED_APPLICATIONS_DISALLOWED,
    FW_ENFORCEMENT_STATE_LOCAL_FIREWALL_RULES_DISALLOWED,
    FW_ENFORCEMENT_STATE_LOCAL_CONSEC_RULES_DISALLOWED,
    FW_ENFORCEMENT_STATE_MISMATCHED_PLATFORM,
    FW_ENFORCEMENT_STATE_OPTIMIZED_OUT,
    FW_ENFORCEMENT_STATE_MAX
} FW_ENFORCEMENT_STATE;

typedef struct _tag_FW_OBJECT_METADATA
{
    ULONGLONG qwFilterContextID;
    //[range(0, 100)]
    ULONG dwNumEntries;
    //[size_is(dwNumEntries)]
    __ecount(dwNumEntries) 
    FW_ENFORCEMENT_STATE *pEnforcementStates;
} FW_OBJECT_METADATA, *PFW_OBJECT_METADATA;

typedef enum _tag_FW_OS_PLATFORM_OP
{    
    FW_OS_PLATFORM_OP_EQ,    
    FW_OS_PLATFORM_OP_GTEQ,    
    FW_OS_PLATFORM_OP_MAX,
} FW_OS_PLATFORM_OP;

typedef struct _tag_FW_OS_PLATFORM
{
    UCHAR bPlatform;
    UCHAR bMajorVersion;
    UCHAR bMinorVersion;
    UCHAR Reserved;
} FW_OS_PLATFORM, *PFW_OS_PLATFORM;

typedef struct _tag_FW_OS_PLATFORM_LIST
{
    //[range(0, 1000)]
    ULONG dwNumEntries;
    //[size_is(dwNumEntries)]
    __ecount(dwNumEntries) 
    PFW_OS_PLATFORM pPlatforms;
} FW_OS_PLATFORM_LIST, *PFW_OS_PLATFORM_LIST;

typedef enum _tag_FW_RULE_ORIGIN_TYPE
{
    FW_RULE_ORIGIN_INVALID,
    FW_RULE_ORIGIN_LOCAL,
    FW_RULE_ORIGIN_GP,
    FW_RULE_ORIGIN_DYNAMIC,
    FW_RULE_ORIGIN_AUTOGEN,
    FW_RULE_ORIGIN_HARDCODED,
    FW_RULE_ORIGIN_MAX
} FW_RULE_ORIGIN_TYPE;

//http://msdn.microsoft.com/en-us/library/cc231521.aspx
typedef enum _tag_FW_ENUM_RULES_FLAGS
{
    // This value signifies that no specific flag is used. 
    // It is defined for IDL definitions and code to add readability, instead of using the number 0.
    FW_ENUM_RULES_FLAG_NONE                 = 0x0000,
    // Resolves rule description strings to user-friendly, localizable strings if they are in the following format: @file.dll,-<resID>. resID refers to the resource ID in the indirect string. Please see [MSDN-SHLoadIndirectString] for further documentation on the string format.
    FW_ENUM_RULES_FLAG_RESOLVE_NAME         = 0x0001,
    // Resolves rule description strings to user-friendly, localizable strings if they are in the following format: @file.dll,-<resID>. resID refers to the resource ID in the indirect string. Please see [MSDN-SHLoadIndirectString] for further documentation on the string format.
    FW_ENUM_RULES_FLAG_RESOLVE_DESCRIPTION  = 0x0002,
    // If this flag is set, the server MUST inspect the wszLocalApplication field of each FW_RULE structure and replace all environment variables in the string with their corresponding values. See [MSDN-ExpandEnvironmentStrings] for more details about environment-variable strings.
    FW_ENUM_RULES_FLAG_RESOLVE_APPLICATION  = 0x0004,
    // Resolves keywords in addresses and ports to the actual addresses and ports (dynamic store only).
    FW_ENUM_RULES_FLAG_RESOLVE_KEYWORD      = 0x0008,
    // Resolves the GPO name for the GP_RSOP rules.
    FW_ENUM_RULES_FLAG_RESOLVE_GPO_NAME     = 0x0010,
    // If this flag is set, the server MUST only return objects where at least one FW_ENFORCEMENT_STATE entry in the object's metadata is equal to FW_ENFORCEMENT_STATE_FULL. This flag is available for the dynamic store only.
    FW_ENUM_RULES_FLAG_EFFECTIVE            = 0x0020,
    // Includes the metadata object information, represented by the FW_OBJECT_METADATA structure, in the enumerated objects.
    FW_ENUM_RULES_FLAG_INCLUDE_METADATA     = 0x0040,
    // This value and greater values are invalid and MUST NOT be used. It is defined for simplicity in writing IDL definitions and code.
    FW_ENUM_RULES_FLAG_MAX                  = 0x0080
} FW_ENUM_RULES_FLAGS;

typedef enum _tag_FW_RULE_ACTION
{
    FW_RULE_ACTION_INVALID = 0,
    FW_RULE_ACTION_ALLOW_BYPASS,
    FW_RULE_ACTION_BLOCK,
    FW_RULE_ACTION_ALLOW,
    FW_RULE_ACTION_MAX
} FW_RULE_ACTION;

typedef enum _tag_FW_RULE_FLAGS
{
    FW_RULE_FLAGS_NONE                                = 0x0000,
    FW_RULE_FLAGS_ACTIVE                              = 0x0001,
    FW_RULE_FLAGS_AUTHENTICATE                        = 0x0002,
    FW_RULE_FLAGS_AUTHENTICATE_WITH_ENCRYPTION        = 0x0004,
    FW_RULE_FLAGS_ROUTEABLE_ADDRS_TRAVERSE            = 0x0008,
    FW_RULE_FLAGS_LOOSE_SOURCE_MAPPED                 = 0x0010,
    FW_RULE_FLAGS_AUTH_WITH_NO_ENCAPSULATION          = 0x0020,
    FW_RULE_FLAGS_AUTH_WITH_ENC_NEGOTIATE            = 0x0040,
    FW_RULE_FLAGS_ROUTEABLE_ADDRS_TRAVERSE_DEFER_APP  = 0x0080,
    FW_RULE_FLAGS_ROUTEABLE_ADDRS_TRAVERSE_DEFER_USER = 0x0100,
    FW_RULE_FLAGS_AUTHENTICATE_BYPASS_OUTBOUND        = 0x0200,
    FW_RULE_FLAGS_MAX                                 = 0x0400,
    FW_RULE_FLAGS_MAX_V2_1                           = 0x0020,
    FW_RULE_FLAGS_MAX_V2_9                           = 0x0040,
} FW_RULE_FLAGS;

typedef struct _tag_FW_RULE2_0
{
    struct _tag_FW_RULE2_0 *pNext;
    USHORT wSchemaVersion;
    //[string, range(1,10001), ref]
    __range(1, 10001)__nullterminated PWCHAR wszRuleId;
    //[string, range(1,10001)]
    __range(1, 10001)__nullterminated PWCHAR wszName;
    //[string, range(1,10001)]
    __range(1, 10001)__nullterminated PWCHAR wszDescription;
    FW_PROFILE_TYPE dwProfiles;
    //[range(FW_DIR_INVALID, FW_DIR_OUT)]
    FW_DIRECTION Direction;
    //[range(0,256)]
    USHORT wIpProtocol;
    //[switch_type(unsigned short), switch_is(wIpProtocol)]
    union
    {
        //[case(6,17)]
        struct
        {
            FW_PORTS LocalPorts;
            FW_PORTS RemotePorts;
        };
        //[case(1)]
        FW_ICMP_TYPE_CODE_LIST V4TypeCodeList;
        //[case(58)]
        FW_ICMP_TYPE_CODE_LIST V6TypeCodeList;
    };
    FW_ADDRESSES LocalAddresses;
    FW_ADDRESSES RemoteAddresses;
    FW_INTERFACE_LUIDS LocalInterfaceIds;
    ULONG dwLocalInterfaceTypes;
    //[string, range(1,10001)]
    __range(1, 10001)__nullterminated PWCHAR wszLocalApplication;
    //[string, range(1,10001)]
    __range(1, 10001)__nullterminated PWCHAR wszLocalService;
    //[range(FW_RULE_ACTION_INVALID, FW_RULE_ACTION_MAX)]
    FW_RULE_ACTION Action;
    FW_ENUM_RULES_FLAGS wFlags;
    //[string, range(1,10001)]
    __range(1, 10001)__nullterminated PWCHAR wszRemoteMachineAuthorizationList;
    //[string, range(1,10001)]
    __range(1, 10001)__nullterminated PWCHAR wszRemoteUserAuthorizationList;
    //[string, range(1,10001)]
    __range(1, 10001)__nullterminated PWCHAR wszEmbeddedContext;
    FW_OS_PLATFORM_LIST PlatformValidityList;
    FW_RULE_STATUS Status;
    //[range(FW_RULE_ORIGIN_INVALID, FW_RULE_ORIGIN_MAX)]
    FW_RULE_ORIGIN_TYPE Origin;
    //[string, range(1,10001)]
    __range(1, 10001)__nullterminated PWCHAR wszGPOName;
    ULONG Reserved;
} FW_RULE2_0, *PFW_RULE2_0;

typedef struct _tag_FW_RULE
{
    struct _tag_FW_RULE *pNext;
    USHORT wSchemaVersion;
    //[string, range(1,512), ref]
    __nullterminated PWCHAR wszRuleId;
    //[string, range(1,10001)]
    __nullterminated PWCHAR wszName;
    //[string, range(1,10001)]
    __nullterminated PWCHAR wszDescription;
    FW_PROFILE_TYPE dwProfiles;
    //[range(FW_DIR_INVALID, FW_DIR_OUT)]
    FW_DIRECTION Direction;
    //[range(0,256)]
    USHORT wIpProtocol;
    //[switch_type(unsigned short), switch_is(wIpProtocol)]
    union 
    {
        //[case(6,17)]
        struct
        {
            FW_PORTS LocalPorts;
            FW_PORTS RemotePorts;
        };
        //[case(1)]
        FW_ICMP_TYPE_CODE_LIST V4TypeCodeList;
        //[case(58)]
        FW_ICMP_TYPE_CODE_LIST V6TypeCodeList;
    };
    FW_ADDRESSES LocalAddresses;
    FW_ADDRESSES RemoteAddresses;
    FW_INTERFACE_LUIDS LocalInterfaceIds;
    ULONG dwLocalInterfaceTypes;
    //[string, range(1,10001)]
    __nullterminated PWCHAR wszLocalApplication;
    //[string, range(1,10001)]
    __nullterminated PWCHAR wszLocalService;
    //[range(FW_RULE_ACTION_INVALID, FW_RULE_ACTION_MAX)]
    FW_RULE_ACTION Action;
    FW_ENUM_RULES_FLAGS wFlags;
    //[string, range(1,10001)]
    __nullterminated PWCHAR wszRemoteMachineAuthorizationList;
    //[string, range(1,10001)]
    __nullterminated PWCHAR wszRemoteUserAuthorizationList;
    //[string, range(1,10001)]
    __nullterminated PWCHAR wszEmbeddedContext;
    FW_OS_PLATFORM_LIST PlatformValidityList;
    FW_RULE_STATUS Status;
    //[range(FW_RULE_ORIGIN_INVALID, FW_RULE_ORIGIN_MAX)]
    FW_RULE_ORIGIN_TYPE Origin;
    //[string, range(1,10001)]
    __nullterminated PWCHAR wszGPOName;
    ULONG Reserved;
    //[size_is((Reserved & FW_OBJECT_CTRL_FLAG_INCLUDE_METADATA) ? 1 : 0)]
    PFW_OBJECT_METADATA pMetaData;
} FW_RULE, *PFW_RULE;

typedef enum _tag_FW_PROFILE_CONFIG
{   
    FW_PROFILE_CONFIG_INVALID,
    FW_PROFILE_CONFIG_ENABLE_FW,
    FW_PROFILE_CONFIG_DISABLE_STEALTH_MODE,
    FW_PROFILE_CONFIG_SHIELDED,
    FW_PROFILE_CONFIG_DISABLE_UNICAST_RESPONSES_TO_MULTICAST_BROADCAST,
    FW_PROFILE_CONFIG_LOG_DROPPED_PACKETS,
    FW_PROFILE_CONFIG_LOG_SUCCESS_CONNECTIONS,
    FW_PROFILE_CONFIG_LOG_IGNORED_RULES,
    FW_PROFILE_CONFIG_LOG_MAX_FILE_SIZE,
    FW_PROFILE_CONFIG_LOG_FILE_PATH,
    FW_PROFILE_CONFIG_DISABLE_INBOUND_NOTIFICATIONS,
    FW_PROFILE_CONFIG_AUTH_APPS_ALLOW_USER_PREF_MERGE,
    FW_PROFILE_CONFIG_GLOBAL_PORTS_ALLOW_USER_PREF_MERGE,
    FW_PROFILE_CONFIG_ALLOW_LOCAL_POLICY_MERGE,
    FW_PROFILE_CONFIG_ALLOW_LOCAL_IPSEC_POLICY_MERGE,
    FW_PROFILE_CONFIG_DISABLED_INTERFACES,
    FW_PROFILE_CONFIG_DEFAULT_OUTBOUND_ACTION,
    FW_PROFILE_CONFIG_DEFAULT_INBOUND_ACTION,
    FW_PROFILE_CONFIG_MAX
} FW_PROFILE_CONFIG;

typedef enum _FW_GLOBAL_CONFIG_IPSEC_EXEMPT_VALUES
{
    FW_GLOBAL_CONFIG_IPSEC_EXEMPT_NONE           =  0x0000,
    FW_GLOBAL_CONFIG_IPSEC_EXEMPT_NEIGHBOR_DISC  =  0x0001,
    FW_GLOBAL_CONFIG_IPSEC_EXEMPT_ICMP           =  0x0002,
    FW_GLOBAL_CONFIG_IPSEC_EXEMPT_ROUTER_DISC    =  0x0004,    
    FW_GLOBAL_CONFIG_IPSEC_EXEMPT_DHCP           =  0x0008,
    FW_GLOBAL_CONFIG_IPSEC_EXEMPT_MAX            =  0x0010,
    FW_GLOBAL_CONFIG_IPSEC_EXEMPT_MAX_V2_0       =  0x0004
} FW_GLOBAL_CONFIG_IPSEC_EXEMPT_VALUES;

typedef enum _FW_GLOBAL_CONFIG_PRESHARED_KEY_ENCODING_VALUES
{
    FW_GLOBAL_CONFIG_PRESHARED_KEY_ENCODING_NONE       = 0,
    FW_GLOBAL_CONFIG_PRESHARED_KEY_ENCODING_UTF_8,
    FW_GLOBAL_CONFIG_PRESHARED_KEY_ENCODING_MAX
} FW_GLOBAL_CONFIG_PRESHARED_KEY_ENCODING_VALUES;

typedef enum _FW_GLOBAL_CONFIG_IPSEC_THROUGH_NAT_VALUES
{
    FW_GLOBAL_CONFIG_IPSEC_THROUGH_NAT_NEVER                      = 0,
    FW_GLOBAL_CONFIG_IPSEC_THROUGH_NAT_SERVER_BEHIND_NAT,
    FW_GLOBAL_CONFIG_IPSEC_THROUGH_NAT_SERVER_AND_CLIENT_BEHIND_NAT,
    FW_GLOBAL_CONFIG_IPSEC_THROUGH_NAT_MAX
} FW_GLOBAL_CONFIG_IPSEC_THROUGH_NAT_VALUES;

typedef enum _tag_FW_GLOBAL_CONFIG
{   
    FW_GLOBAL_CONFIG_INVALID,
    FW_GLOBAL_CONFIG_POLICY_VERSION_SUPPORTED,
    FW_GLOBAL_CONFIG_CURRENT_PROFILE,
    FW_GLOBAL_CONFIG_DISABLE_STATEFUL_FTP,
    FW_GLOBAL_CONFIG_DISABLE_STATEFUL_PPTP,
    FW_GLOBAL_CONFIG_SA_IDLE_TIME,
    FW_GLOBAL_CONFIG_PRESHARED_KEY_ENCODING,
    FW_GLOBAL_CONFIG_IPSEC_EXEMPT,
    FW_GLOBAL_CONFIG_CRL_CHECK,
    FW_GLOBAL_CONFIG_IPSEC_THROUGH_NAT,
    FW_GLOBAL_CONFIG_POLICY_VERSION,
    FW_GLOBAL_CONFIG_BINARY_VERSION_SUPPORTED,
    FW_GLOBAL_CONFIG_IPSEC_TUNNEL_REMOTE_MACHINE_AUTHORIZATION_LIST,
    FW_GLOBAL_CONFIG_IPSEC_TUNNEL_REMOTE_USER_AUTHORIZATION_LIST,
    FW_GLOBAL_CONFIG_MAX
} FW_GLOBAL_CONFIG;

typedef enum _FW_CONFIG_FLAGS
{
    FW_CONFIG_FLAG_RETURN_DEFAULT_IF_NOT_FOUND  = 0x0001
} FW_CONFIG_FLAGS;

typedef struct tag_FW_NETWORK
{
   __nullterminated PWCHAR pszName;
   FW_PROFILE_TYPE ProfileType;
} FW_NETWORK, *PFW_NETWORK;

typedef struct tag_FW_ADAPTER
{
   __nullterminated PWCHAR pszFriendlyName;
   GUID Guid;
} FW_ADAPTER, *PFW_ADAPTER;

typedef struct tag_FW_DIAG_APP
{
   __nullterminated PWCHAR pszAppPath;
} FW_DIAG_APP, *PFW_DIAG_APP;

typedef enum tag_FW_RULE_CATEGORY //[v1_enum]
{
   FW_RULE_CATEGORY_BOOT,
   FW_RULE_CATEGORY_STEALTH,
   FW_RULE_CATEGORY_FIREWALL,
   FW_RULE_CATEGORY_CONSEC,
   FW_RULE_CATEGORY_MAX
} FW_RULE_CATEGORY, *PFW_RULE_CATEGORY;

typedef struct tag_FW_PRODUCT
{
   ULONG dwFlags;
   ULONG dwNumRuleCategories;
   //[size_is(dwNumRuleCategories), unique]
   FW_RULE_CATEGORY* pRuleCategories;
   //[string, ref]
   __nullterminated PWCHAR pszDisplayName;
   //[string, unique]
   __nullterminated PWCHAR pszPathToSignedProductExe;
} FW_PRODUCT, *PFW_PRODUCT;

typedef enum _tag_FW_IP_VERSION
{
    FW_IP_VERSION_INVALID,
    FW_IP_VERSION_V4,
    FW_IP_VERSION_V6,
    FW_IP_VERSION_MAX
} FW_IP_VERSION;

typedef enum _tag_FW_IPSEC_PHASE
{
    FW_IPSEC_PHASE_INVALID,
    FW_IPSEC_PHASE_1,
    FW_IPSEC_PHASE_2,
    FW_IPSEC_PHASE_MAX
} FW_IPSEC_PHASE;

typedef enum _tag_FW_CS_RULE_FLAGS
{
    FW_CS_RULE_FLAGS_NONE                        = 0x00,
    FW_CS_RULE_FLAGS_ACTIVE                      = 0x01,
    FW_CS_RULE_FLAGS_DTM                         = 0x02,
    FW_CS_RULE_FLAGS_TUNNEL_BYPASS_IF_ENCRYPTED  = 0x08,
    FW_CS_RULE_FLAGS_OUTBOUND_CLEAR              = 0x10,
    FW_CS_RULE_FLAGS_APPLY_AUTHZ                 = 0x20,
    FW_CS_RULE_FLAGS_MAX                         = 0x40,
    FW_CS_RULE_FLAGS_MAX_V2_1                    = 0x02,
    FW_CS_RULE_FLAGS_MAX_V2_8                    = 0x04
} FW_CS_RULE_FLAGS;

typedef enum _tag_FW_CS_RULE_ACTION
{
    FW_CS_RULE_ACTION_INVALID,
    FW_CS_RULE_ACTION_SECURE_SERVER,
    FW_CS_RULE_ACTION_BOUNDARY,
    FW_CS_RULE_ACTION_SECURE,
    FW_CS_RULE_ACTION_DO_NOT_SECURE,
    FW_CS_RULE_ACTION_MAX
} FW_CS_RULE_ACTION;

typedef enum _tag_FW_KEY_MODULE_
{    
    FW_KEY_MODULE_DEFAULT,    
    FW_KEY_MODULE_IKEv1,    
    FW_KEY_MODULE_AUTHIP,    
    FW_KEY_MODULE_MAX
} FW_KEY_MODULE;

typedef struct _tag_FW_CS_RULE2_10
{    
    struct _tag_FW_CS_RULE2_0 *pNext;    
    USHORT wSchemaVersion;    
   // [string, range(1,512), ref]    
    __nullterminated PWCHAR wszRuleId;    
    //[string, range(1,10001)]   
    __nullterminated PWCHAR wszName;    
    //[string, range(1,10001)]    
    __nullterminated PWCHAR wszDescription;    
    FW_PROFILE_TYPE dwProfiles;    
    FW_ADDRESSES Endpoint1;    
    FW_ADDRESSES Endpoint2;    
    FW_INTERFACE_LUIDS LocalInterfaceIds;    
    ULONG dwLocalInterfaceTypes;    
    ULONG dwLocalTunnelEndpointV4;    
    UCHAR LocalTunnelEndpointV6[16];    
    ULONG dwRemoteTunnelEndpointV4;    
    UCHAR RemoteTunnelEndpointV6[16];    
    FW_PORTS Endpoint1Ports;    
    FW_PORTS Endpoint2Ports;    
    //[range(0,256)]    
    USHORT wIpProtocol;    
    //[string, range(1,1001)]    
    __nullterminated PWCHAR wszPhase1AuthSet;    
    //[string, range(1,1001)]   
    __nullterminated PWCHAR wszPhase2CryptoSet;    
    //[string, range(1,1001)]    
    __nullterminated PWCHAR wszPhase2AuthSet;    
    //[range(FW_CS_RULE_ACTION_SECURE_SERVER, FW_CS_RULE_ACTION_MAX)]    
    FW_CS_RULE_ACTION   Action;   
    FW_ENUM_RULES_FLAGS wFlags;    
    //[string, range(1,10001)]    
    __nullterminated PWCHAR wszEmbeddedContext;    
    FW_OS_PLATFORM_LIST PlatformValidityList;    
    //[range(FW_RULE_ORIGIN_INVALID, FW_RULE_ORIGIN_MAX-1)]    
    FW_RULE_ORIGIN_TYPE Origin;    
    //[string, range(1,10001)]    wchar_t*              wszGPOName;    
    FW_RULE_STATUS        Status; 
    //[string, range(1,512)] 
    __nullterminated PWCHAR wszRemoteTunnelEndpointFqdn;  
    FW_ADDRESSES RemoteTunnelEndpoints;  
    FW_KEY_MODULE KeyModule;
} FW_CS_RULE2_10, *PFW_CS_RULE2_10;

typedef struct _tag_FW_CS_RULE2_0
{
    struct _tag_FW_CS_RULE2_0 *pNext;
    USHORT wSchemaVersion;
    //[string, range(1,512), ref]
    __nullterminated PWCHAR wszRuleId;
    //[string, range(1,10001)]
    __nullterminated PWCHAR wszName;
    //[string, range(1,10001)]
    __nullterminated PWCHAR wszDescription;
    FW_PROFILE_TYPE dwProfiles;
    FW_ADDRESSES Endpoint1;
    FW_ADDRESSES Endpoint2;
    FW_INTERFACE_LUIDS LocalInterfaceIds;
    ULONG dwLocalInterfaceTypes;
    ULONG dwLocalTunnelEndpointV4;
    UCHAR LocalTunnelEndpointV6[16];
    ULONG dwRemoteTunnelEndpointV4;
    UCHAR RemoteTunnelEndpointV6[16];
    FW_PORTS Endpoint1Ports;
    FW_PORTS Endpoint2Ports;
    //[range(0,256)]
    USHORT wIpProtocol;
    //[string, range(1,1001)]
    __nullterminated PWCHAR wszPhase1AuthSet;
    //[string, range(1,1001)]
    __nullterminated PWCHAR wszPhase2CryptoSet;
    //[string, range(1,1001)]
    __nullterminated PWCHAR wszPhase2AuthSet;
    //[range(FW_CS_RULE_ACTION_SECURE_SERVER, FW_CS_RULE_ACTION_MAX - 1)]
    FW_CS_RULE_ACTION   Action;
    FW_ENUM_RULES_FLAGS wFlags;
    //[string, range(1,10001)]
    __nullterminated PWCHAR wszEmbeddedContext;
    FW_OS_PLATFORM_LIST PlatformValidityList;
    //[range(FW_RULE_ORIGIN_INVALID, FW_RULE_ORIGIN_MAX-1)]
    FW_RULE_ORIGIN_TYPE Origin;
    //[string, range(1,10001)]
    __nullterminated PWCHAR wszGPOName;
    FW_RULE_STATUS Status;
} FW_CS_RULE2_0, *PFW_CS_RULE2_0;

typedef struct _tag_FW_CS_RULE
{
    struct _tag_FW_CS_RULE *pNext;
    USHORT wSchemaVersion;
    //[string, range(1,10001), ref]
    __nullterminated PWCHAR wszRuleId;
    //[string, range(1,10001)]
    __nullterminated PWCHAR wszName;
    //[string, range(1,10001)]
    __nullterminated PWCHAR wszDescription;
    FW_PROFILE_TYPE dwProfiles;
    FW_ADDRESSES Endpoint1;
    FW_ADDRESSES Endpoint2;
    FW_INTERFACE_LUIDS LocalInterfaceIds;
    ULONG dwLocalInterfaceTypes;
    ULONG dwLocalTunnelEndpointV4;
    UCHAR LocalTunnelEndpointV6[16];
    ULONG dwRemoteTunnelEndpointV4;
    UCHAR RemoteTunnelEndpointV6[16];
    FW_PORTS Endpoint1Ports;
    FW_PORTS Endpoint2Ports;
    //[range(0,256)]
    USHORT wIpProtocol;
    //[string, range(1,10001)]
    __nullterminated PWCHAR wszPhase1AuthSet;
    //[string, range(1,10001)]
    __nullterminated PWCHAR wszPhase2CryptoSet;
    //[string, range(1,10001)]
    __nullterminated PWCHAR wszPhase2AuthSet;
    //[range(FW_CS_RULE_ACTION_SECURE_SERVER, FW_CS_RULE_ACTION_MAX - 1)]
    FW_CS_RULE_ACTION Action;
    USHORT wFlags;
    //[string, range(1,10001)]
    __nullterminated PWCHAR wszEmbeddedContext;
    FW_OS_PLATFORM_LIST PlatformValidityList;
    //[range(FW_RULE_ORIGIN_INVALID, FW_RULE_ORIGIN_MAX-1)]
    FW_RULE_ORIGIN_TYPE Origin;
    //[string, range(1,10001)]
    __nullterminated PWCHAR wszGPOName;
    FW_RULE_STATUS Status;
    //[string, range(1,512)]
    __nullterminated PWCHAR wszMMParentRuleId;
    ULONG Reserved;
    //[size_is((Reserved & FW_OBJECT_CTRL_FLAG_INCLUDE_METADATA) ? 1 : 0)]
    PFW_OBJECT_METADATA pMetaData;
} FW_CS_RULE, *PFW_CS_RULE;

typedef enum _tag_FW_AUTH_METHOD
{
    FW_AUTH_METHOD_INVALID,
    FW_AUTH_METHOD_ANONYMOUS,
    FW_AUTH_METHOD_MACHINE_KERB,
    FW_AUTH_METHOD_MACHINE_SHKEY,
    FW_AUTH_METHOD_MACHINE_NTLM,
    FW_AUTH_METHOD_MACHINE_CERT,
    FW_AUTH_METHOD_USER_KERB,
    FW_AUTH_METHOD_USER_CERT,
    FW_AUTH_METHOD_USER_NTLM,
    FW_AUTH_METHOD_MAX
} FW_AUTH_METHOD;

typedef enum _tag_FW_AUTH_SUITE_FLAGS
{
    FW_AUTH_SUITE_FLAGS_NONE                                 = 0x0000,
    FW_AUTH_SUITE_FLAGS_CERT_EXCLUDE_CA_NAME                 = 0x0001,
    FW_AUTH_SUITE_FLAGS_HEALTH_CERT                          = 0x0002,
    FW_AUTH_SUITE_FLAGS_PERFORM_CERT_ACCOUNT_MAPPING         = 0x0004,
    FW_AUTH_SUITE_FLAGS_CERT_SIGNING_ECDSA256                = 0x0008,
    FW_AUTH_SUITE_FLAGS_CERT_SIGNING_ECDSA384                = 0x0010,
    FW_AUTH_SUITE_FLAGS_MAX_V2_1                             = 0x0020,
    FW_AUTH_SUITE_FLAGS_INTERMEDIATE_CA                      = 0x0020,
    FW_AUTH_SUITE_FLAGS_MAX                                   = 0x0040
} FW_AUTH_SUITE_FLAGS;

typedef struct _tag_FW_AUTH_SUITE
{
    //[range(FW_AUTH_METHOD_INVALID+1, FW_AUTH_METHOD_MAX)]
    FW_AUTH_METHOD Method;
    USHORT wFlags;
    //[switch_type(FW_AUTH_METHOD), switch_is(Method)]
    union
    {
        //[case(FW_AUTH_METHOD_MACHINE_CERT,FW_AUTH_METHOD_USER_CERT)]
        struct
        {
            //[ref, string]
            __nullterminated PWCHAR wszCAName;
        };
        //[case(FW_AUTH_METHOD_MACHINE_SHKEY)]
        struct
        {
            //[ref, string]
            __nullterminated PWCHAR wszSHKey;
        };
    };
} FW_AUTH_SUITE, *PFW_AUTH_SUITE;

typedef struct _tag_FW_AUTH_SET
{
    struct _tag_FW_AUTH_SET* pNext;
    USHORT wSchemaVersion;
    //[range(FW_IPSEC_PHASE_INVALID+1, FW_IPSEC_PHASE_MAX-1)]
    FW_IPSEC_PHASE IpSecPhase;
    //[string, range(1,255), ref]
    __nullterminated PWCHAR wszSetId;
    //[string, range(1,10001)]
    __nullterminated PWCHAR wszName;
    //[string, range(1,10001)]
    __nullterminated PWCHAR wszDescription;
    //[string, range(1,10001)]
    __nullterminated PWCHAR wszEmbeddedContext;
    //[range(0, 1000)]
    ULONG dwNumSuites;
    //[size_is(dwNumSuites)]
    PFW_AUTH_SUITE pSuites;
    //[range(FW_RULE_ORIGIN_INVALID, FW_RULE_ORIGIN_MAX-1)]
    FW_RULE_ORIGIN_TYPE Origin;
    //[string, range(1,10001)]
    __nullterminated PWCHAR wszGPOName;
    FW_RULE_STATUS Status;
    ULONG dwAuthSetFlags;
} FW_AUTH_SET, *PFW_AUTH_SET;

typedef enum _tag_FW_CRYPTO_KEY_EXCHANGE_TYPE
{
    FW_CRYPTO_KEY_EXCHANGE_NONE = 0,
    FW_CRYPTO_KEY_EXCHANGE_DH1,
    FW_CRYPTO_KEY_EXCHANGE_DH2,
    FW_CRYPTO_KEY_EXCHANGE_ECDH256,
    FW_CRYPTO_KEY_EXCHANGE_ECDH384,
    FW_CRYPTO_KEY_EXCHANGE_DH2048,    
    FW_CRYPTO_KEY_EXCHANGE_DH14 = FW_CRYPTO_KEY_EXCHANGE_DH2048,
    FW_CRYPTO_KEY_EXCHANGE_MAX
} FW_CRYPTO_KEY_EXCHANGE_TYPE;

typedef enum _tag_FW_CRYPTO_ENCRYPTION_TYPE
{
    FW_CRYPTO_ENCRYPTION_NONE,
    FW_CRYPTO_ENCRYPTION_DES,
    FW_CRYPTO_ENCRYPTION_3DES,
    FW_CRYPTO_ENCRYPTION_AES128,
    FW_CRYPTO_ENCRYPTION_AES192,
    FW_CRYPTO_ENCRYPTION_AES256,
    FW_CRYPTO_ENCRYPTION_AES_GCM128,
    FW_CRYPTO_ENCRYPTION_AES_GCM192,
    FW_CRYPTO_ENCRYPTION_AES_GCM256,
    FW_CRYPTO_ENCRYPTION_MAX,
    FW_CRYPTO_ENCRYPTION_MAX_V2_0 = FW_CRYPTO_ENCRYPTION_AES_GCM128
} FW_CRYPTO_ENCRYPTION_TYPE;

typedef enum _tag_FW_CRYPTO_HASH_TYPE
{
    FW_CRYPTO_HASH_NONE,
    FW_CRYPTO_HASH_MD5,
    FW_CRYPTO_HASH_SHA1,
    FW_CRYPTO_HASH_SHA256,
    FW_CRYPTO_HASH_SHA384,
    FW_CRYPTO_HASH_AES_GMAC128,
    FW_CRYPTO_HASH_AES_GMAC192,
    FW_CRYPTO_HASH_AES_GMAC256,
    FW_CRYPTO_HASH_MAX,
    FW_CRYPTO_HASH_MAX_V2_0 = FW_CRYPTO_HASH_SHA256
} FW_CRYPTO_HASH_TYPE;

typedef enum _tag_FW_CRYPTO_PROTOCOL_TYPE
{
    FW_CRYPTO_PROTOCOL_INVALID,
    FW_CRYPTO_PROTOCOL_AH,
    FW_CRYPTO_PROTOCOL_ESP,
    FW_CRYPTO_PROTOCOL_BOTH,
    FW_CRYPTO_PROTOCOL_AUTH_NO_ENCAP,
    FW_CRYPTO_PROTOCOL_MAX,
    FW_CRYPTO_PROTOCOL_MAX_2_1 = (FW_CRYPTO_PROTOCOL_BOTH + 1)
} FW_CRYPTO_PROTOCOL_TYPE;

typedef struct _tag_FW_PHASE1_CRYPTO_SUITE
{
    //[range(FW_CRYPTO_KEY_EXCHANGE_NONE, FW_CRYPTO_KEY_EXCHANGE_MAX-1)]
    FW_CRYPTO_KEY_EXCHANGE_TYPE KeyExchange;
    //[range(FW_CRYPTO_ENCRYPTION_NONE+1, FW_CRYPTO_ENCRYPTION_MAX-1)]
    FW_CRYPTO_ENCRYPTION_TYPE Encryption;
    //[range(FW_CRYPTO_HASH_NONE+1, FW_CRYPTO_HASH_MAX-1)]
    FW_CRYPTO_HASH_TYPE Hash;
    ULONG dwP1CryptoSuiteFlags;
} FW_PHASE1_CRYPTO_SUITE, *PFW_PHASE1_CRYPTO_SUITE;

typedef struct _tag_FW_PHASE2_CRYPTO_SUITE
{
    //[range(FW_CRYPTO_PROTOCOL_INVALID+1, FW_CRYPTO_PROTOCOL_MAX-1)]
    FW_CRYPTO_PROTOCOL_TYPE Protocol;
    FW_CRYPTO_HASH_TYPE AhHash;
    FW_CRYPTO_HASH_TYPE EspHash;
    FW_CRYPTO_ENCRYPTION_TYPE Encryption;
    ULONG dwTimeoutMinutes;
    ULONG dwTimeoutKBytes;
    ULONG dwP2CryptoSuiteFlags;
} FW_PHASE2_CRYPTO_SUITE, *PFW_PHASE2_CRYPTO_SUITE;

typedef enum _tag_FW_PHASE1_CRYPTO_FLAGS
{
    FW_PHASE1_CRYPTO_FLAGS_NONE                     = 0x00,
    FW_PHASE1_CRYPTO_FLAGS_DO_NOT_SKIP_DH           = 0x01,
    FW_PHASE1_CRYPTO_FLAGS_MAX                      = 0x02
} FW_PHASE1_CRYPTO_FLAGS;

typedef enum _tag_FW_PHASE2_CRYPTO_PFS
{
    FW_PHASE2_CRYPTO_PFS_INVALID,
    FW_PHASE2_CRYPTO_PFS_DISABLE,
    FW_PHASE2_CRYPTO_PFS_PHASE1,
    FW_PHASE2_CRYPTO_PFS_DH1,
    FW_PHASE2_CRYPTO_PFS_DH2,
    FW_PHASE2_CRYPTO_PFS_DH2048,
    FW_PHASE2_CRYPTO_PFS_ECDH256,
    FW_PHASE2_CRYPTO_PFS_ECDH384,
    FW_PHASE2_CRYPTO_PFS_MAX
} FW_PHASE2_CRYPTO_PFS;

typedef struct _tag_FW_CRYPTO_SET
{
    struct _tag_FW_CRYPTO_SET* pNext;
    USHORT wSchemaVersion;
    //[range(FW_IPSEC_PHASE_INVALID+1, FW_IPSEC_PHASE_MAX-1)]
    FW_IPSEC_PHASE IpSecPhase;
    //[string, range(1,255), ref]
    __range(1, 255)__nullterminated PWCHAR wszSetId;
    //[string, range(1,10001)]
    __range(1, 10001)__nullterminated PWCHAR wszName;
    //[string, range(1,10001)]
    __range(1, 10001)__nullterminated PWCHAR wszDescription;
    //[string, range(1,10001)]
    __range(1, 10001)__nullterminated PWCHAR wszEmbeddedContext;
    //[switch_type(FW_IPSEC_PHASE), switch_is(IpSecPhase)]
    union
    {
        //[case(FW_IPSEC_PHASE_1)]
        struct
        {
            USHORT wFlags;       
            //[range(0, 1000)]
            ULONG dwNumPhase1Suites;
            //[size_is(dwNumPhase1Suites)]
            PFW_PHASE1_CRYPTO_SUITE pPhase1Suites;
            ULONG dwTimeoutMinutes;
            ULONG dwTimeoutSessions;
        };
        //[case(FW_IPSEC_PHASE_2)]
        struct
        {
            FW_PHASE2_CRYPTO_PFS Pfs;
            //[range(0, 1000)]
            ULONG dwNumPhase2Suites;
            //[size_is(dwNumPhase2Suites)]
            PFW_PHASE2_CRYPTO_SUITE pPhase2Suites;
        };
    };
    //[range(FW_RULE_ORIGIN_INVALID, FW_RULE_ORIGIN_MAX-1)]
    FW_RULE_ORIGIN_TYPE Origin;
    //[string, range(1,10001)]
    __nullterminated PWCHAR wszGPOName;
    FW_RULE_STATUS Status;
    ULONG dwCryptoSetFlags;
} FW_CRYPTO_SET, *PFW_CRYPTO_SET;

typedef struct _tag_FW_BYTE_BLOB
{
    //[range(0, 10000)]
    ULONG dwSize;
    //[size_is(dwSize)]
    PUCHAR Blob;
} FW_BYTE_BLOB, *PFW_BYTE_BLOB;

typedef struct _tag_FW_COOKIE_PAIR
{
    ULONGLONG Initiator;
    ULONGLONG Responder;
} FW_COOKIE_PAIR, *PFW_COOKIE_PAIR;

typedef enum _tag_FW_PHASE1_KEY_MODULE_TYPE
{
    FW_PHASE1_KEY_MODULE_INVALID,
    FW_PHASE1_KEY_MODULE_IKE,
    FW_PHASE1_KEY_MODULE_AUTH_IP,
    FW_PHASE1_KEY_MODULE_MAX
} FW_PHASE1_KEY_MODULE_TYPE;

typedef struct _tag_FW_CERT_INFO
{
    FW_BYTE_BLOB SubjectName;
    //[range(FW_AUTH_SUITE_FLAGS_NONE, FW_AUTH_SUITE_FLAGS_MAX-1)]
    ULONG dwCertFlags;
} FW_CERT_INFO, *PFW_CERT_INFO;

typedef struct _tag_FW_AUTH_INFO
{
    //[range(FW_AUTH_METHOD_INVALID + 1, FW_AUTH_METHOD_MAX)]
    FW_AUTH_METHOD  AuthMethod;
    //[switch_type(FW_AUTH_METHOD), switch_is(AuthMethod)]
    union
    {
        //[case(FW_AUTH_METHOD_MACHINE_CERT,FW_AUTH_METHOD_USER_CERT)]
        struct
        {
            FW_CERT_INFO MyCert;
            FW_CERT_INFO PeerCert;
        };
        //[case(FW_AUTH_METHOD_MACHINE_KERB,FW_AUTH_METHOD_USER_KERB)]
        struct
        {
            //[string, range(1,10001)]
            __nullterminated PWCHAR wszMyId;
            //[string, range(1,10001)]
            __nullterminated PWCHAR wszPeerId;
        };
    };
    ULONG dwAuthInfoFlags;
} FW_AUTH_INFO, *PFW_AUTH_INFO;

typedef struct _tag_FW_ENDPOINTS
{
    //[range(FW_IP_VERSION_INVALID+1, FW_IP_VERSION_MAX-1)]
    FW_IP_VERSION IpVersion;
    ULONG dwSourceV4Address;
    ULONG dwDestinationV4Address;
    UCHAR SourceV6Address[16];
    UCHAR DestinationV6Address[16];
} FW_ENDPOINTS, *PFW_ENDPOINTS;

typedef struct _tag_FW_PHASE1_SA_DETAILS
{
    ULONGLONG SaId;
    //[range(FW_PHASE1_KEY_MODULE_INVALID+1, FW_PHASE1_KEY_MODULE_MAX-1)]
    FW_PHASE1_KEY_MODULE_TYPE KeyModuleType;
    FW_ENDPOINTS Endpoints;
    FW_PHASE1_CRYPTO_SUITE SelectedProposal;
    ULONG dwProposalLifetimeKBytes;
    ULONG dwProposalLifetimeMinutes;
    ULONG dwProposalMaxNumPhase2;
    FW_COOKIE_PAIR CookiePair;
    PFW_AUTH_INFO pFirstAuth;
    PFW_AUTH_INFO pSecondAuth;
    ULONG dwP1SaFlags;
} FW_PHASE1_SA_DETAILS, *PFW_PHASE1_SA_DETAILS;

typedef enum _tag_FW_PHASE2_TRAFFIC_TYPE
{
    FW_PHASE2_TRAFFIC_TYPE_INVALID,
    FW_PHASE2_TRAFFIC_TYPE_TRANSPORT,
    FW_PHASE2_TRAFFIC_TYPE_TUNNEL,
    FW_PHASE2_TRAFFIC_TYPE_MAX
} FW_PHASE2_TRAFFIC_TYPE;

typedef struct _tag_FW_PHASE2_SA_DETAILS
{
    ULONGLONG SaId;
    //[range(FW_DIR_INVALID+1, FW_DIR_MAX-1)]
    FW_DIRECTION Direction;
    FW_ENDPOINTS Endpoints;
    USHORT wLocalPort;
    USHORT wRemotePort;
    USHORT wIpProtocol;
    FW_PHASE2_CRYPTO_SUITE SelectedProposal;
    FW_PHASE2_CRYPTO_PFS Pfs;
    GUID TransportFilterId;
    ULONG dwP2SaFlags;
} FW_PHASE2_SA_DETAILS, *PFW_PHASE2_SA_DETAILS;

typedef
    //[switch_type(FW_PROFILE_CONFIG)]
union   _FW_PROFILE_CONFIG_VALUE
{
    //[case(FW_PROFILE_CONFIG_LOG_FILE_PATH)]
    //[string, range(1,10001)]
    __nullterminated PWCHAR wszStr;
    //[case(FW_PROFILE_CONFIG_DISABLED_INTERFACES)]
    PFW_INTERFACE_LUIDS pDisabledInterfaces;
    //[case(FW_PROFILE_CONFIG_ENABLE_FW,
    //FW_PROFILE_CONFIG_DISABLE_STEALTH_MODE,
    //    FW_PROFILE_CONFIG_SHIELDED,
    //    FW_PROFILE_CONFIG_DISABLE_UNICAST_RESPONSES_TO_MULTICAST_BROADCAST,
    //    FW_PROFILE_CONFIG_LOG_DROPPED_PACKETS,
    //    FW_PROFILE_CONFIG_LOG_SUCCESS_CONNECTIONS,
    //    FW_PROFILE_CONFIG_LOG_IGNORED_RULES,
    //    FW_PROFILE_CONFIG_LOG_MAX_FILE_SIZE,
    //    FW_PROFILE_CONFIG_DISABLE_INBOUND_NOTIFICATIONS,
    //    FW_PROFILE_CONFIG_AUTH_APPS_ALLOW_USER_PREF_MERGE,
    //    FW_PROFILE_CONFIG_GLOBAL_PORTS_ALLOW_USER_PREF_MERGE,
    //    FW_PROFILE_CONFIG_ALLOW_LOCAL_POLICY_MERGE, 
    //    FW_PROFILE_CONFIG_ALLOW_LOCAL_IPSEC_POLICY_MERGE,
    //    FW_PROFILE_CONFIG_DEFAULT_OUTBOUND_ACTION,
    //    FW_PROFILE_CONFIG_DEFAULT_INBOUND_ACTION)]
    PULONG pdwVal;
} FW_PROFILE_CONFIG_VALUE, *PFW_PROFILE_CONFIG_VALUE;

typedef struct _tag_FW_MM_RULE
{
    struct _tag_FW_MM_RULE *pNext;
    USHORT wSchemaVersion;
    //[string, range(1,512), ref]
    __nullterminated PWCHAR wszRuleId;
    //[string, range(1,10001)]
    __nullterminated PWCHAR wszName;
    //[string, range(1,10001)]
    __nullterminated PWCHAR wszDescription;
    FW_PROFILE_TYPE dwProfiles;
    FW_ADDRESSES Endpoint1;
    FW_ADDRESSES Endpoint2;
    //[string, range(1,255)]
    PWCHAR wszPhase1AuthSet;
    //[string, range(1,255)]
    PWCHAR wszPhase1CryptoSet;
    USHORT wFlags;
    //[string, range(1,10001)]
    __nullterminated PWCHAR wszEmbeddedContext;
    FW_OS_PLATFORM_LIST PlatformValidityList;
    //[range(FW_RULE_ORIGIN_INVALID, FW_RULE_ORIGIN_MAX-1)]
    FW_RULE_ORIGIN_TYPE Origin;
    //[string, range(1,10001)]
    __nullterminated PWCHAR wszGPOName;
    FW_RULE_STATUS Status;
    ULONG Reserved;
    //[size_is((Reserved & FW_OBJECT_CTRL_FLAG_INCLUDE_METADATA) ? 1 : 0)]
    PFW_OBJECT_METADATA pMetaData;
} FW_MM_RULE, *PFW_MM_RULE;

typedef enum _tag_FW_MATCH_KEY
{
    FW_MATCH_KEY_PROFILE,
    FW_MATCH_KEY_STATUS,
    FW_MATCH_KEY_OBJECTID,
    FW_MATCH_KEY_FILTERID,
    FW_MATCH_KEY_APP_PATH, //The APP Path
    FW_MATCH_KEY_PROTOCOL,
    FW_MATCH_KEY_LOCAL_PORT,
    FW_MATCH_KEY_REMOTE_PORT,
    FW_MATCH_KEY_GROUP,
    FW_MATCH_KEY_SVC_NAME,
    FW_MATCH_KEY_DIRECTION,
    FW_MATCH_KEY_MAX
} FW_MATCH_KEY;

typedef enum _tag_FW_DATA_TYPE
{
    FW_DATA_TYPE_EMPTY,
    FW_DATA_TYPE_UINT8,
    FW_DATA_TYPE_UINT16,
    FW_DATA_TYPE_UINT32,
    FW_DATA_TYPE_UINT64,
    FW_DATA_TYPE_UNICODE_STRING
} FW_DATA_TYPE;

typedef struct _tag_FW_MATCH_VALUE
{
    FW_DATA_TYPE type;
    //[switch_type(FW_DATA_TYPE), switch_is(type)]
    union
    {
       //[case(FW_DATA_TYPE_UINT8)]
       UCHAR uInt8;
       //[case(FW_DATA_TYPE_UINT16)]
       ULONG uInt16;
       //[case(FW_DATA_TYPE_UINT32)]
       ULONG uInt32;
       //[case(FW_DATA_TYPE_UINT64)]
       ULONGLONG uInt64;
       //[case(FW_DATA_TYPE_UNICODE_STRING)]
       struct
       {
           //[string, range(1,10001)]
           PWCHAR wszString;
       };
    };
} FW_MATCH_VALUE;

typedef enum _tag_FW_MATCH_TYPE
{
    FW_MATCH_TYPE_TRAFFIC_MATCH,
    FW_MATCH_TYPE_EQUAL,
    FW_MATCH_TYPE_MAX
} FW_MATCH_TYPE;

typedef struct _tag_FW_QUERY_CONDITION
{
    FW_MATCH_KEY matchKey;
    FW_MATCH_TYPE matchType;
    FW_MATCH_VALUE matchValue;
} FW_QUERY_CONDITION, *PFW_QUERY_CONDITION;

typedef struct _tag_FW_QUERY_CONDITIONS
{
    ULONG dwNumEntries;
    //[size_is(dwNumEntries)]
    FW_QUERY_CONDITION *AndedConditions;
} FW_QUERY_CONDITIONS, *PFW_QUERY_CONDITIONS;

typedef struct _tag_FW_QUERY
{
    USHORT wSchemaVersion;
    ULONG dwNumEntries;
    //[size_is(dwNumEntries)]
    FW_QUERY_CONDITIONS *ORConditions;
    FW_RULE_STATUS Status;
} FW_QUERY, *PFW_QUERY;

//uuid(6b5bdd1e-528c-422c-af8c-a4079be4fe48)
//version(1.0)

//fw_free(PVOID)
//fw_allocate(size_t size)

//    unsigned long RRPC_FWOpenPolicyStore(
//        [in] FW_CONN_HANDLE rpcConnHandle,
//        [in] unsigned short BinaryVersion,
//        [in, range(FW_STORE_TYPE_INVALID+1, FW_STORE_TYPE_MAX-1)]
//        FW_STORE_TYPE StoreType,
//        [in, range(FW_POLICY_ACCESS_RIGHT_INVALID+1, FW_POLICY_ACCESS_RIGHT_MAX-1)]
//        FW_POLICY_ACCESS_RIGHT AccessRight,
//        [in]  unsigned long dwFlags,
//        [out] PFW_POLICY_STORE_HANDLE phPolicyStore
//        );

typedef ULONG (WINAPI *_FWOpenPolicyStore)(
    __in USHORT BinaryVersion, 
    __in PWCHAR wszMachineOrGPO, 
    __in FW_STORE_TYPE StoreType, 
    __in FW_POLICY_ACCESS_RIGHT AccessRight, 
    __in FW_POLICY_STORE_FLAGS dwFlags, 
    __out PFW_POLICY_STORE_HANDLE phPolicy
    );

//    unsigned long RRPC_FWClosePolicyStore(
//        [in] FW_CONN_HANDLE rpcConnHandle,
//        [in, out] PFW_POLICY_STORE_HANDLE phPolicyStore
//        );

typedef ULONG (WINAPI *_FWClosePolicyStore)(
    __inout PFW_POLICY_STORE_HANDLE phPolicyStore
    );

//    unsigned long
//           RRPC_FWRestoreDefaults(
//        [in] FW_CONN_HANDLE rpcConnHandle
//         );

typedef ULONG (WINAPI *_FWRestoreDefaults)(
    __in const __nullterminated PWCHAR wszMachineOrGPO // why is the C# version using this?
    );

//    unsigned long
//           RRPC_FWGetGlobalConfig(
//        [in] FW_CONN_HANDLE           rpcConnHandle,
//        [in] unsigned short                     BinaryVersion,
//        [in] FW_STORE_TYPE            StoreType,
//        [in, range(FW_GLOBAL_CONFIG_INVALID+1, FW_GLOBAL_CONFIG_MAX-1)]
//          FW_GLOBAL_CONFIG            configID,
//        [in] unsigned long                    dwFlags,
//        [in, out, unique, size_is(cbData), length_is(*pcbTransmittedLen)]
//        unsigned char*                       pBuffer,
//        [in] unsigned long                  cbData,
//        [in,out]  unsigned long           *pcbTransmittedLen,
//        [out] unsigned long               *pcbRequired
//        );

typedef ULONG (WINAPI *_FWGetGlobalConfig)(
    __in USHORT wBinaryVersion, 
    __in __nullterminated PWCHAR wszMachineOrGPO, // why is the C# version using this?????
    __in FW_STORE_TYPE StoreType, 
    __in FW_GLOBAL_CONFIG configId, 
    __in FW_CONFIG_FLAGS dwFlags, 
    __inout PUCHAR pBuffer, 
    __out ULONG pdwBufSize
    );

//   unsigned long
//          RRPC_FWSetGlobalConfig(
//        [in] FW_CONN_HANDLE             rpcConnHandle,
//        [in] unsigned short                       BinaryVersion,
//        [in] FW_STORE_TYPE              StoreType,
//        [in, range(FW_GLOBAL_CONFIG_INVALID+1, FW_GLOBAL_CONFIG_MAX-1)]
//           FW_GLOBAL_CONFIG             configID,
//        [in, unique, size_is(dwBufSize)]
//        unsigned char *lpBuffer,
//        [in, range(0, 10*1024)] unsigned long dwBufSize
//        );

typedef ULONG (WINAPI *_FWSetGlobalConfig)(
    __in USHORT wBinaryVersion, 
    __in __nullterminated PWCHAR wszMachineOrGPO, 
    __in FW_STORE_TYPE StoreType, 
    __in FW_GLOBAL_CONFIG configId, 
    __inout PUCHAR lpBuffer, 
    __in ULONG dwBufSize
    );

//   unsigned long
//          RRPC_FWAddFirewallRule(
//        [in] FW_CONN_HANDLE          rpcConnHandle,
//        [in] FW_POLICY_STORE_HANDLE  hPolicyStore,
//        [in] PFW_RULE2_0             pRule
//        );

typedef ULONG (WINAPI *_FWAddFirewallRule)(
    __in FW_POLICY_STORE_HANDLE hPolicyStore, 
    __in PFW_RULE2_0 pRule // C# uses ref so do we need __inout ??
    );

//    unsigned long
//           RRPC_FWSetFirewallRule(
//        [in] FW_CONN_HANDLE          rpcConnHandle,
//        [in] FW_POLICY_STORE_HANDLE  hPolicyStore,
//        [in] PFW_RULE2_0             pRule
//        );

typedef ULONG (WINAPI *_FWSetFirewallRule)(
    __in FW_POLICY_STORE_HANDLE hPolicyStore, 
    __in PFW_RULE2_0 pRule
    );

//    unsigned long
//           RRPC_FWDeleteFirewallRule(
//        [in] FW_CONN_HANDLE          rpcConnHandle,
//        [in] FW_POLICY_STORE_HANDLE hPolicyStore,
//        [in, string, ref] const wchar_t* wszRuleID
//        );

typedef ULONG (WINAPI *_FWDeleteFirewallRule)(
    __in FW_POLICY_STORE_HANDLE hPolicyStore, 
    __in const __nullterminated PWCHAR wszRuleID
    );

//    unsigned long
//           RRPC_FWDeleteAllFirewallRules(
//        [in] FW_CONN_HANDLE         rpcConnHandle,
//        [in] FW_POLICY_STORE_HANDLE hPolicyStore
//        );

typedef ULONG (WINAPI *_FWDeleteAllFirewallRules)(
    __in FW_POLICY_STORE_HANDLE hPolicyStore
    );

//    unsigned long
//           RRPC_FWEnumFirewallRules(
//        [in] FW_CONN_HANDLE          rpcConnHandle,
//        [in] FW_POLICY_STORE_HANDLE  hPolicyStore,
//        [in] unsigned long           dwFilteredByStatus,
//        [in] unsigned long           dwProfileFilter,
//        [in] unsigned short          wFlags,
//        [out, ref] unsigned long     *pdwNumRules,
//        [out] PFW_RULE2_0           *ppRules
//        );

typedef ULONG (WINAPI *_FWEnumFirewallRules)(
    __in FW_POLICY_STORE_HANDLE hPolicyStore, 
    __in FW_RULE_STATUS_CLASS dwFilteredByStatus, 
    __in FW_PROFILE_TYPE dwProfileFilter, 
    __in FW_ENUM_RULES_FLAGS wFlags,
    __out PULONG pdwNumRules, 
    __out_ecount(pdwNumRules) PFW_RULE2_0 *ppFwRules
    );

/////////////////////////////////////////

//    unsigned long
//           RRPC_FWGetConfig(
//        [in] FW_CONN_HANDLE           rpcConnHandle,
//        [in] FW_POLICY_STORE_HANDLE   hPolicyStore,
//        [in, range(FW_PROFILE_CONFIG_ENABLE_FW, FW_PROFILE_CONFIG_MAX-1)]
//           FW_PROFILE_CONFIG             configID,
//        [in] FW_PROFILE_TYPE          Profile,
//        [in] unsigned long                    dwFlags,
//        [in, out, unique, size_is(cbData), length_is(*pcbTransmittedLen)]
//        unsigned char*                       pBuffer, 
//        [in] unsigned long                  cbData,
//        [in,out]  unsigned long           *pcbTransmittedLen,
//        [out] unsigned long               *pcbRequired
//        );

typedef ULONG (WINAPI *_FWGetConfig)(
    __in FW_POLICY_STORE_HANDLE hPolicyStore, 
    __in FW_PROFILE_CONFIG configId, 
    __in FW_PROFILE_TYPE Profile, 
    __in FW_CONFIG_FLAGS dwFlags, 
    __inout PCHAR pBuffer, 
    __in ULONG pdwBufSize
    );

//    unsigned long
//           RRPC_FWSetConfig(
//        [in] FW_CONN_HANDLE            rpcConnHandle,
//        [in] FW_POLICY_STORE_HANDLE    hPolicyStore,
//        [in, range(FW_PROFILE_CONFIG_ENABLE_FW, FW_PROFILE_CONFIG_MAX-1)]
//           FW_PROFILE_CONFIG              configID,
//        [in] FW_PROFILE_TYPE          Profile, 
//        [in, switch_is(configID)] FW_PROFILE_CONFIG_VALUE  pConfig,
//        [in, range(0, 10*1024)] unsigned long  dwBufSize
//        );

typedef ULONG (WINAPI *_FWSetConfig)(
    __in FW_POLICY_STORE_HANDLE hPolicyStore, 
    __in FW_PROFILE_CONFIG configId, 
    __in FW_PROFILE_TYPE Profile, 
    __in FW_PROFILE_CONFIG_VALUE pConfig, 
    __in ULONG dwBufSize
    );

//    unsigned long
//           RRPC_FWAddConnectionSecurityRule(
//        [in] FW_CONN_HANDLE              rpcConnHandle,
//        [in] FW_POLICY_STORE_HANDLE      hPolicy,
//        [in] PFW_CS_RULE2_0              pRule
//        );

typedef ULONG (WINAPI *_FWAddConnectionSecurityRule)(
    __in FW_POLICY_STORE_HANDLE hPolicy, 
    __in PFW_CS_RULE2_0 pRule
    );

//    unsigned long
//           RRPC_FWSetConnectionSecurityRule(
//        [in] FW_CONN_HANDLE              rpcConnHandle,
//        [in] FW_POLICY_STORE_HANDLE      hPolicy,
//        [in] PFW_CS_RULE2_0              pRule
//        );

typedef ULONG (WINAPI *_FWSetConnectionSecurityRule)(
    __in FW_POLICY_STORE_HANDLE hPolicy, 
    __in PFW_CS_RULE2_0 pRule
    );

//    unsigned long
//           RRPC_FWDeleteConnectionSecurityRule(
//        [in] FW_CONN_HANDLE             rpcConnHandle,
//        [in] FW_POLICY_STORE_HANDLE     hPolicy,
//        [in, string, ref] wchar_t        *pRuleId
//        );

typedef ULONG (WINAPI *_FWDeleteConnectionSecurityRule)(
    __in FW_POLICY_STORE_HANDLE hPolicy, 
    __in __nullterminated PWCHAR pRuleId
    );

//    unsigned long
//           RRPC_FWDeleteAllConnectionSecurityRules(
//        [in] FW_CONN_HANDLE          rpcConnHandle,
//        [in] FW_POLICY_STORE_HANDLE     hPolicy
//        );

typedef ULONG (WINAPI *_FWDeleteAllConnectionSecurityRules)(
    __in FW_POLICY_STORE_HANDLE hPolicy
    );

//    unsigned long
//           RRPC_FWEnumConnectionSecurityRules(
//        [in] FW_CONN_HANDLE          rpcConnHandle,
//        [in] FW_POLICY_STORE_HANDLE  hPolicy,
//        [in] unsigned long           dwFilteredByStatus,
//        [in] unsigned long           dwProfileFilter,
//        [in] unsigned short          wFlags,
//        [out, ref] unsigned long *   pdwNumRules,
//        [out] PFW_CS_RULE2_0*        ppRules
//        );

typedef ULONG (WINAPI *_FWEnumConnectionSecurityRules)(
    __in FW_POLICY_STORE_HANDLE hPolicy, 
    __in FW_RULE_STATUS_CLASS dwFilteredByStatus, 
    __in FW_PROFILE_TYPE dwProfileFilter, 
    __in FW_ENUM_RULES_FLAGS wFlags, 
    __out PULONG pdwNumRules, 
    __out_ecount(pdwNumRules) PFW_CS_RULE2_0* ppRules
    );
 
//    unsigned long
//           RRPC_FWAddAuthenticationSet(
//        [in] FW_CONN_HANDLE          rpcConnHandle,
//        [in] FW_POLICY_STORE_HANDLE  hPolicy,
//        [in] PFW_AUTH_SET            pAuth
//        );

typedef ULONG (WINAPI *_FWAddAuthenticationSet)(
    __in FW_POLICY_STORE_HANDLE hPolicy, 
    __in PFW_AUTH_SET pAuth
    );

//    unsigned long
//           RRPC_FWSetAuthenticationSet(
//        [in] FW_CONN_HANDLE          rpcConnHandle,
//        [in] FW_POLICY_STORE_HANDLE  hPolicy,
//        [in] PFW_AUTH_SET            pAuth
//        );

typedef ULONG (WINAPI *_FWSetAuthenticationSet)(
    __in FW_POLICY_STORE_HANDLE hPolicy, 
    __in PFW_AUTH_SET pAuth
    );

//    unsigned long
//           RRPC_FWDeleteAuthenticationSet(
//        [in] FW_CONN_HANDLE            rpcConnHandle,
//        [in] FW_POLICY_STORE_HANDLE    hPolicy,
//        [in, range(FW_IPSEC_PHASE_INVALID+1, FW_IPSEC_PHASE_MAX-1)]
//           FW_IPSEC_PHASE            IpSecPhase,
//        [in, string, ref] const wchar_t* wszSetId
//        );

typedef ULONG (WINAPI *_FWDeleteAuthenticationSet)(
    __in FW_POLICY_STORE_HANDLE hPolicy, 
    __in FW_IPSEC_PHASE IpSecPhase, 
    __in const __nullterminated PWCHAR wszSetId
    );

//    unsigned long
//           RRPC_FWDeleteAllAuthenticationSets(
//        [in] FW_CONN_HANDLE          rpcConnHandle,
//        [in] FW_POLICY_STORE_HANDLE  hPolicy,
//        [in, range(FW_IPSEC_PHASE_INVALID+1, FW_IPSEC_PHASE_MAX-1)]
//           FW_IPSEC_PHASE          IpSecPhase
//        );

typedef ULONG (WINAPI *_FWDeleteAllAuthenticationSets)(
    __in FW_POLICY_STORE_HANDLE hPolicy, 
    __in FW_IPSEC_PHASE IpSecPhase
    );

//    unsigned long
//           RRPC_FWEnumAuthenticationSets(
//        [in] FW_CONN_HANDLE            rpcConnHandle,
//        [in] FW_POLICY_STORE_HANDLE    hPolicy,
//        [in, range(FW_IPSEC_PHASE_INVALID+1, FW_IPSEC_PHASE_MAX-1)]
//           FW_IPSEC_PHASE            IpSecPhase,
//        [in] unsigned long                     dwFilteredByStatus,
//        [in] unsigned short                      wFlags,
//        [out] unsigned long*                   pdwNumAuthSets,
//        [out] PFW_AUTH_SET*       ppAuth
//        );

typedef ULONG (WINAPI *_FWEnumAuthenticationSets)(
    __in FW_POLICY_STORE_HANDLE hPolicy, 
    __in FW_IPSEC_PHASE IpSecPhase, 
    __in FW_RULE_STATUS_CLASS dwFilteredByStatus, 
    __in FW_ENUM_RULES_FLAGS wFlags, 
    __out PULONG dwNumAuthSets, 
    __out_ecount(dwNumAuthSets) PFW_AUTH_SET* ppAuthSets
    );

//    unsigned long
//           RRPC_FWAddCryptoSet(
//        [in] FW_CONN_HANDLE          rpcConnHandle,
//        [in] FW_POLICY_STORE_HANDLE  hPolicy,
//        [in] PFW_CRYPTO_SET     pCrypto
//        );

typedef ULONG (WINAPI *_FWAddCryptoSet)(
    __in FW_POLICY_STORE_HANDLE hPolicy, 
    __in PFW_CRYPTO_SET pCrypto
    );

//    unsigned long
//           RRPC_FWSetCryptoSet(
//        [in] FW_CONN_HANDLE          rpcConnHandle,
//        [in] FW_POLICY_STORE_HANDLE  hPolicy,
//        [in] PFW_CRYPTO_SET     pCrypto
//        );

typedef ULONG (WINAPI *_FWSetCryptoSet)(
    __in FW_POLICY_STORE_HANDLE hPolicy, 
    __in PFW_CRYPTO_SET pCrypto
    );

//    unsigned long
//           RRPC_FWDeleteCryptoSet(
//        [in] FW_CONN_HANDLE            rpcConnHandle,
//        [in] FW_POLICY_STORE_HANDLE    hPolicy,
//        [in, range(FW_IPSEC_PHASE_INVALID+1, FW_IPSEC_PHASE_MAX-1)]
//            FW_IPSEC_PHASE            IpSecPhase,
//        [in, string, ref] const wchar_t* wszSetId
//        );

typedef ULONG (WINAPI *_FWDeleteCryptoSet)(
    __in FW_POLICY_STORE_HANDLE hPolicy, 
    __in FW_IPSEC_PHASE IpSecPhase,
    __in const __nullterminated PWCHAR wszSetId
    );

//    unsigned long
//           RRPC_FWDeleteAllCryptoSets(
//        [in] FW_CONN_HANDLE          rpcConnHandle,
//        [in] FW_POLICY_STORE_HANDLE  hPolicy,
//        [in, range(FW_IPSEC_PHASE_INVALID+1, FW_IPSEC_PHASE_MAX-1)]
//            FW_IPSEC_PHASE          IpSecPhase
//        );

typedef ULONG (WINAPI *_FWDeleteAllCryptoSets)(
    __in FW_POLICY_STORE_HANDLE hPolicy, 
    __in FW_IPSEC_PHASE IpSecPhase
    );

//    unsigned long
//           RRPC_FWEnumCryptoSets(
//        [in] FW_CONN_HANDLE             rpcConnHandle,
//        [in] FW_POLICY_STORE_HANDLE     hPolicy,
//        [in, range(FW_IPSEC_PHASE_INVALID+1, FW_IPSEC_PHASE_MAX-1)]
//            FW_IPSEC_PHASE             IpSecPhase,
//        [in] unsigned long                      dwFilteredByStatus,
//        [in] unsigned short                       wFlags,
//        [out, ref] unsigned long*               pdwNumSets,
//        [out] PFW_CRYPTO_SET*      ppCryptoSets
//        );

typedef ULONG (WINAPI *_FWEnumCryptoSets)(
    __in FW_POLICY_STORE_HANDLE hPolicy, 
    __in FW_IPSEC_PHASE IpSecPhase, 
    __in FW_RULE_STATUS_CLASS dwFilteredByStatus, 
    __in FW_ENUM_RULES_FLAGS wFlags, 
    __out PULONG pdwNumSets, 
    __out_ecount(pdwNumSets) PFW_CRYPTO_SET* ppCryptoSets
    );
 
//    unsigned long
//           RRPC_FWEnumPhase1SAs(
//        [in] FW_CONN_HANDLE                   rpcConnHandle,
//        [in] FW_POLICY_STORE_HANDLE           hPolicy,
//        [in,unique] PFW_ENDPOINTS             pEndpoints,
//        [out,ref]  unsigned long*                     pdwNumSAs,
//        [out,size_is( , *pdwNumSAs)]  PFW_PHASE1_SA_DETAILS* ppSAs
//        );

typedef ULONG (WINAPI *_FWEnumPhase1SAs)(
    __in FW_POLICY_STORE_HANDLE hPolicy, 
    __in PFW_ENDPOINTS pEndpoints, 
    __out PULONG pdwNumSAs, 
    __out_ecount(pdwNumSAs) PFW_PHASE1_SA_DETAILS* ppSAs
    );

//    unsigned long
//           RRPC_FWEnumPhase2SAs(
//        [in] FW_CONN_HANDLE                   rpcConnHandle,
//        [in] FW_POLICY_STORE_HANDLE           hPolicy,
//        [in, unique] PFW_ENDPOINTS            pEndpoints,
//        [out,ref]  unsigned long*                     pdwNumSAs,
//        [out,size_is( , *pdwNumSAs)]  PFW_PHASE2_SA_DETAILS* ppSAs
//        );

typedef ULONG (WINAPI *_FWEnumPhase2SAs)(
    __in FW_POLICY_STORE_HANDLE hPolicy, 
    __in PFW_ENDPOINTS pEndpoints, 
    __out PULONG pdwNumSAs, 
    __out_ecount(pdwNumSAs) PFW_PHASE2_SA_DETAILS* ppSAs
    );

//    unsigned long
//           RRPC_FWDeletePhase1SAs(
//        [in] FW_CONN_HANDLE             rpcConnHandle,
//        [in] FW_POLICY_STORE_HANDLE     hPolicy,
//        [in, unique] PFW_ENDPOINTS       pEndpoints
//        );

typedef ULONG (WINAPI *_FWDeletePhase1SAs)(
    __in FW_POLICY_STORE_HANDLE hPolicy, 
    __in PFW_ENDPOINTS pEndpoints
    );

//    unsigned long
//           RRPC_FWDeletePhase2SAs(
//        [in] FW_CONN_HANDLE             rpcConnHandle,
//        [in] FW_POLICY_STORE_HANDLE     hPolicy,
//        [in, unique] PFW_ENDPOINTS       pEndpoints
//        );

typedef ULONG (WINAPI *_FWDeletePhase2SAs)(
    __in FW_POLICY_STORE_HANDLE hPolicy, 
    __in PFW_ENDPOINTS pEndpoints
    );

//     unsigned long
//            RRPC_FWEnumProducts(
//        [in] FW_CONN_HANDLE rpcConnHandle,
//        [in] FW_POLICY_STORE_HANDLE hPolicyStore,
//        [out] unsigned long* pdwNumProducts,
//        [out, size_is(,*pdwNumProducts)] PFW_PRODUCT* ppProducts
//        );

typedef ULONG (WINAPI *_FWEnumProducts)(
    __in FW_POLICY_STORE_HANDLE hPolicy, 
    __out PULONG pdwNumProducts, 
    __out_ecount(pdwNumProducts) PFW_PRODUCT* ppProducts
    );

/////////////////////////////////////////

typedef ULONG (WINAPI *_FWFreeProducts)(
    __in PFW_PRODUCT pProducts
    );

/////////////////////////////////////////
 
//    unsigned long
//    RRPC_FWAddMainModeRule(
//        [in] FW_CONN_HANDLE          rpcConnHandle,
//        [in] FW_POLICY_STORE_HANDLE  hPolicy,
//        [in] PFW_MM_RULE             pMMRule,
//        [out] FW_RULE_STATUS *       pStatus
//        );

typedef ULONG (WINAPI *_FWAddMainModeRule)(
    __in FW_POLICY_STORE_HANDLE hPolicy, 
    __in PFW_MM_RULE pMMRule, 
    __out FW_RULE_STATUS* pStatus
    );

//    unsigned long
//    RRPC_FWSetMainModeRule(
//        [in] FW_CONN_HANDLE          rpcConnHandle,
//        [in] FW_POLICY_STORE_HANDLE  hPolicy,
//        [in] PFW_MM_RULE             pMMRule,
//        [out] FW_RULE_STATUS *       pStatus
//        );

typedef ULONG (WINAPI *_FWSetMainModeRule)(
    __in FW_POLICY_STORE_HANDLE hPolicy, 
    __in PFW_MM_RULE pMMRule, 
    __out FW_RULE_STATUS* pStatus
    );

//    unsigned long
//    RRPC_FWDeleteMainModeRule(
//        [in] FW_CONN_HANDLE            rpcConnHandle,
//        [in] FW_POLICY_STORE_HANDLE    hPolicy,
//        [in, string, ref] LPWSTR       pRuleId
//        );

typedef ULONG (WINAPI *_FWDeleteMainModeRule)(
    __in FW_POLICY_STORE_HANDLE hPolicy, 
    __in LPWSTR pRuleId
    );

//    unsigned long
//    RRPC_FWDeleteAllMainModeRules(
//        [in] FW_CONN_HANDLE          rpcConnHandle,
//        [in] FW_POLICY_STORE_HANDLE  hPolicy
//        );

typedef ULONG (WINAPI *_FWDeleteAllMainModeRules)(
    __in FW_POLICY_STORE_HANDLE hPolicy
    );

//    unsigned long
//    RRPC_FWEnumMainModeRules(
//        [in] FW_CONN_HANDLE             rpcConnHandle,
//        [in] FW_POLICY_STORE_HANDLE     hPolicy,
//        [in] unsigned long              dwFilteredByStatus,
//        [in] unsigned long              dwProfileFilter,
//        [in] unsigned short             wFlags,
//        [out, ref] unsigned long       *pdwNumRules,
//        [out] PFW_MM_RULE  *            ppMMRules
//        );

typedef ULONG (WINAPI *_FWEnumMainModeRules)(
    __in FW_POLICY_STORE_HANDLE hPolicy,
    __in FW_RULE_STATUS_CLASS dwFilteredByStatus,
    __in FW_PROFILE_TYPE dwProfileFilter,
    __in FW_ENUM_RULES_FLAGS wFlags,
    __out PULONG pdwNumRules,
    __out_ecount(pdwNumRules) PFW_MM_RULE* ppMMRules
    );

//////////////////////////////////////////////////

//    unsigned long
//    RRPC_FWQueryFirewallRules(
//        [in] FW_CONN_HANDLE             rpcConnHandle,
//        [in] FW_POLICY_STORE_HANDLE     hPolicy,
//        [in] PFW_QUERY                  pQuery,
//        [in] unsigned short             wFlags,
//        [out, ref] unsigned long       *pdwNumRules,
//        [out] PFW_RULE  *               ppRules
//        );

typedef ULONG (WINAPI *_FWQueryFirewallRules)(
    __in FW_POLICY_STORE_HANDLE hPolicy, 
    __in PFW_QUERY pQuery, 
    __in FW_ENUM_RULES_FLAGS wFlags, 
    __out PULONG pdwNumRules, 
    __out_ecount(pdwNumRules) PFW_RULE* ppFwRules
    );

//    unsigned long
//    RRPC_FWQueryConnectionSecurityRules(
//        [in] FW_CONN_HANDLE             rpcConnHandle,
//        [in] FW_POLICY_STORE_HANDLE     hPolicy,
//        [in] PFW_QUERY                  pQuery,
//        [in] unsigned short             wFlags,
//        [out, ref] unsigned long       *pdwNumRules,
//        [out] PFW_CS_RULE  *            ppRules
//        );

typedef ULONG (WINAPI *_FWQueryConnectionSecurityRules)(
    __in FW_POLICY_STORE_HANDLE hPolicy, 
    __in PFW_QUERY pQuery, 
    __in FW_ENUM_RULES_FLAGS wFlags, 
    __out PULONG pdwNumRules, 
    __out_ecount(pdwNumRules) PFW_CS_RULE* ppRules
    );

//    unsigned long
//    RRPC_FWQueryMainModeRules(
//        [in] FW_CONN_HANDLE             rpcConnHandle,
//        [in] FW_POLICY_STORE_HANDLE     hPolicy,
//        [in] PFW_QUERY                  pQuery,
//        [in] unsigned short             wFlags,
//        [out, ref] unsigned long       *pdwNumRules,
//        [out] PFW_MM_RULE  *            ppMMRules
//        );

typedef ULONG (WINAPI *_FWQueryMainModeRules)(
    __in FW_POLICY_STORE_HANDLE hPolicy, 
    __in PFW_QUERY pQuery, 
    __in FW_ENUM_RULES_FLAGS wFlags, 
    __out PULONG pdwNumRules, 
    __out_ecount(pdwNumRules) PFW_MM_RULE* ppMMRules
    );

//    unsigned long
//    RRPC_FWQueryAuthenticationSets(
//        [in] FW_CONN_HANDLE             rpcConnHandle,
//        [in] FW_POLICY_STORE_HANDLE     hPolicy,
//        [in, range(FW_IPSEC_PHASE_INVALID+1, FW_IPSEC_PHASE_MAX-1)]
//          FW_IPSEC_PHASE             IPsecPhase,
//        [in] PFW_QUERY                  pQuery,
//        [in] unsigned short             wFlags,
//        [out, ref] unsigned long       *pdwNumSets,
//        [out] PFW_AUTH_SET *            ppAuthSets
//        );

typedef ULONG (WINAPI *_FWQueryAuthenticationSets)(
    __in FW_POLICY_STORE_HANDLE hPolicy, 
    __in FW_IPSEC_PHASE IPsecPhase, 
    __in PFW_QUERY pQuery,
    __in FW_ENUM_RULES_FLAGS wFlags, 
    __out PULONG pdwNumSets, 
    __out_ecount(pdwNumSets) PFW_AUTH_SET* ppAuthSets
    );

//    unsigned long
//    RRPC_FWQueryCryptoSets(
//        [in] FW_CONN_HANDLE             rpcConnHandle,
//        [in] FW_POLICY_STORE_HANDLE     hPolicy,
//        [in, range(FW_IPSEC_PHASE_INVALID+1, FW_IPSEC_PHASE_MAX-1)]
//          FW_IPSEC_PHASE             IPsecPhase,
//        [in] PFW_QUERY                  pQuery,
//        [in] unsigned short             wFlags,
//        [out, ref] unsigned long       *pdwNumSets,
//        [out] PFW_CRYPTO_SET *          ppCryptoSets
//        );

typedef ULONG (WINAPI *_FWQueryCryptoSets)(
    __in FW_POLICY_STORE_HANDLE hPolicy, 
    __in FW_IPSEC_PHASE IPsecPhase, 
    __in PFW_QUERY pQuery,
    __in FW_ENUM_RULES_FLAGS wFlags, 
    __out PULONG pdwNumSets, 
    __out_ecount(pdwNumSets) PFW_CRYPTO_SET* ppCryptoSets
    );

//    unsigned long
//    RRPC_FWEnumNetworks(
//        [in] FW_CONN_HANDLE rpcConnHandle,
//        [in] FW_POLICY_STORE_HANDLE hPolicyStore,
//        [out] unsigned long       *pdwNumNetworks,
//        [out, size_is(,*pdwNumNetworks)] PFW_NETWORK* ppNetworks
//        );

typedef ULONG (WINAPI *_FWEnumNetworks)(
    __in FW_POLICY_STORE_HANDLE hPolicy, 
    __out PULONG pdwNumNetworks, 
    __out_ecount(pdwNumNetworks) PFW_NETWORK* ppNetworks
    );

//    unsigned long
//    RRPC_FWEnumAdapters(
//        [in] FW_CONN_HANDLE rpcConnHandle,
//        [in] FW_POLICY_STORE_HANDLE hPolicyStore,
//        [out] unsigned long       *pdwNumAdapters,
//        [out, size_is(,*pdwNumAdapters)] PFW_ADAPTER* ppAdapters
//        );

typedef ULONG (WINAPI *_FWEnumAdapters)(
    __in FW_POLICY_STORE_HANDLE hPolicy, 
    __out PULONG pdwNumAdapters, 
    __out_ecount(pdwNumAdapters) PFW_ADAPTER* ppAdapters
    );

////////////////////////////////////////

typedef ULONG (WINAPI *_FWFreeMainModeRules)(
    __in PFW_MM_RULE pMMRules
    );

typedef ULONG (WINAPI *_FWFreeAdapters)(
    __in PFW_ADAPTER pAdapters
    );

typedef ULONG (WINAPI *_FWFreeFirewallRules)(
    __in PFW_RULE2_0 pFwRules
    );

typedef ULONG (WINAPI *_FWFreeNetworks)(
    __in PFW_NETWORK pNetworks
    );

////////////////////////////////////////


typedef ULONG (WINAPI *_FWStatusMessageFromStatusCode)(
    __in FW_RULE_STATUS StatusCode, 
    __out_ecount(pcchMsg) __typefix(PWCHAR) PWCHAR pszMsg, 
    __out PULONG pcchMsg
    );

typedef ULONG (WINAPI *_FWVerifyFirewallRule)(
    __in USHORT wBinaryVersion, 
    __inout PFW_RULE pRule
    );

typedef ULONG (WINAPI *_FWExportPolicy)(
    __in CONST __nullterminated PWCHAR wszMachineOrGPO,
    __in BOOLEAN fGPO, 
    __in CONST __nullterminated PWCHAR wszFilePath, 
    __out PBOOLEAN fSomeInfoLost
    );

typedef ULONG (WINAPI *_FWImportPolicy)(
    __in const __nullterminated PWCHAR wszMachineOrGPO, 
    __in BOOLEAN fGPO, 
    __in CONST __nullterminated PWCHAR wszFilePath, 
    __out PBOOLEAN fSomeInfoLost
    );

typedef ULONG (WINAPI *_FWRestoreGPODefaults)(
    __in CONST __nullterminated PWCHAR wszGPOPath
    );

typedef ULONG (WINAPI *_FwIsGroupPolicyEnforced)(
    __in CONST __nullterminated PWCHAR wszMachine, 
    __in ULONG Profiles, 
    __out PBOOLEAN dwEnforced
    );

typedef ULONG (WINAPI *_FWChangeNotificationCreate)(
    __in HANDLE hEvent, //WaitHandle
    __out PHANDLE hNewNotifyObject
    );

typedef ULONG (WINAPI *_FWChangeNotificationDestroy)(
    __out PHANDLE hNotifyObject
    );

typedef enum _FW_TRANSACTIONAL_STATE
{
    FW_TRANSACTIONAL_STATE_NONE,
    FW_TRANSACTIONAL_STATE_NO_FLUSH,
    FW_TRANSACTIONAL_STATE_MAX
} FW_TRANSACTIONAL_STATE;


//typedef ULONG (WINAPI *_FWChangeTransactionalState(IntPtr hPolicy, FW_TRANSACTIONAL_STATE TransactionState);
//typedef ULONG (WINAPI *_FWRevertTransaction(IntPtr hPolicy);


//    unsigned long
//    RRPC_FWGetGlobalConfig2_10(
//        [in] FW_CONN_HANDLE           rpcConnHandle,
//        [in] unsigned short           BinaryVersion,
//        [in] FW_STORE_TYPE            StoreType,
//        [in, range(FW_GLOBAL_CONFIG_INVALID+1, FW_GLOBAL_CONFIG_MAX-1)]
//          FW_GLOBAL_CONFIG            configID,
//        [in] unsigned long            dwFlags,
//        [in, out, unique, size_is(cbData), length_is(*pcbTransmittedLen)]
//          BYTE*                       pBuffer,
//        [in] unsigned long          cbData,
//        [in,out]  unsigned long    *pcbTransmittedLen,
//        [out] unsigned long        *pcbRequired,
//        [out] FW_RULE_ORIGIN_TYPE * pOrigin
//        );

//    unsigned long
//    RRPC_FWGetConfig2_10(
//        [in] FW_CONN_HANDLE           rpcConnHandle,
//        [in] FW_POLICY_STORE_HANDLE   hPolicyStore,
//        [in, range(FW_PROFILE_CONFIG_ENABLE_FW, FW_PROFILE_CONFIG_MAX-1)]
//          FW_PROFILE_CONFIG             configID,
//        [in] FW_PROFILE_TYPE          Profile,
//        [in] unsigned long            dwFlags,
//        [in, out, unique, size_is(cbData), length_is(*pcbTransmittedLen)] 
//          BYTE*                       pBuffer,
//        [in] unsigned long          cbData,
//        [in,out]  unsigned long    *pcbTransmittedLen,
//        [out] unsigned long        *pcbRequired,
//        [out] FW_RULE_ORIGIN_TYPE * pOrigin
//        );
//
//    unsigned long
//    RRPC_FWAddFirewallRule2_10(
//        [in] FW_CONN_HANDLE          rpcConnHandle,
//        [in] FW_POLICY_STORE_HANDLE  hPolicyStore,
//        [in] PFW_RULE                pRule,
//        [out] FW_RULE_STATUS *       pStatus
//        );

//    unsigned long
//    RRPC_FWSetFirewallRule2_10(
//        [in] FW_CONN_HANDLE          rpcConnHandle,
//        [in] FW_POLICY_STORE_HANDLE  hPolicyStore,
//        [in] PFW_RULE                pRule,
//        [out] FW_RULE_STATUS *       pStatus
//        );
//
//    unsigned long
//    RRPC_FWEnumFirewallRules2_10(
//        [in] FW_CONN_HANDLE          rpcConnHandle,
//        [in] FW_POLICY_STORE_HANDLE  hPolicyStore,
//        [in] unsigned long           dwFilteredByStatus,
//        [in] unsigned long           dwProfileFilter,
//        [in] unsigned short          wFlags,
//        [out, ref] unsigned long    *pdwNumRules,
//        [out] PFW_RULE              *ppRules
//        );
//
//    unsigned long
//    RRPC_FWAddConnectionSecurityRule2_10(
//        [in] FW_CONN_HANDLE              rpcConnHandle,
//        [in] FW_POLICY_STORE_HANDLE      hPolicy,
//        [in] PFW_CS_RULE                 pRule,
//        [out] FW_RULE_STATUS *           pStatus
//        );
//
//    unsigned long
//    RRPC_FWSetConnectionSecurityRule2_10(
//        [in] FW_CONN_HANDLE              rpcConnHandle,
//        [in] FW_POLICY_STORE_HANDLE      hPolicy,
//        [in] PFW_CS_RULE                 pRule,
//        [out] FW_RULE_STATUS *           pStatus
//        );
//
//    unsigned long
//    RRPC_FWEnumConnectionSecurityRules2_10(
//        [in] FW_CONN_HANDLE          rpcConnHandle,
//        [in] FW_POLICY_STORE_HANDLE  hPolicyStore,
//        [in] unsigned long           dwFilteredByStatus,
//        [in] unsigned long           dwProfileFilter,
//        [in] unsigned short          wFlags,
//        [out, ref] unsigned long    *pdwNumRules,
//        [out] PFW_CS_RULE           *ppRules
//        );
//
//    unsigned long
//    RRPC_FWAddAuthenticationSet2_10(
//        [in] FW_CONN_HANDLE              rpcConnHandle,
//        [in] FW_POLICY_STORE_HANDLE      hPolicy,
//        [in] PFW_AUTH_SET                pAuth,
//        [out] FW_RULE_STATUS *           pStatus
//        );
//
//    unsigned long
//    RRPC_FWSetAuthenticationSet2_10(
//        [in] FW_CONN_HANDLE              rpcConnHandle,
//        [in] FW_POLICY_STORE_HANDLE      hPolicy,
//        [in] PFW_AUTH_SET                pAuth,
//        [out] FW_RULE_STATUS *           pStatus
//        );
//
//    unsigned long
//    RRPC_FWEnumAuthenticationSets2_10(
//        [in] FW_CONN_HANDLE          rpcConnHandle,
//        [in] FW_POLICY_STORE_HANDLE  hPolicyStore,
//        [in, range(FW_IPSEC_PHASE_INVALID+1, FW_IPSEC_PHASE_MAX-1)]             
//        FW_IPSEC_PHASE  IpSecPhase,        
//        [in] unsigned long           dwFilteredByStatus,
//        [in] unsigned short          wFlags,
//        [out, ref] unsigned long    *pdwNumAuthSets,
//        [out] PFW_AUTH_SET          *ppAuth 
//        );
//
//    unsigned long
//    RRPC_FWAddCryptoSet2_10(
//        [in] FW_CONN_HANDLE              rpcConnHandle,
//        [in] FW_POLICY_STORE_HANDLE      hPolicy,
//        [in] PFW_CRYPTO_SET              pCrypto,
//        [out] FW_RULE_STATUS *           pStatus
//        );
//
//    unsigned long
//    RRPC_FWSetCryptoSet2_10(
//        [in] FW_CONN_HANDLE              rpcConnHandle,
//        [in] FW_POLICY_STORE_HANDLE      hPolicy,
//        [in] PFW_CRYPTO_SET              pCrypto,
//        [out] FW_RULE_STATUS *           pStatus
//        );
//
//    unsigned long
//    RRPC_FWEnumCryptoSets2_10(
//        [in] FW_CONN_HANDLE          rpcConnHandle,
//        [in] FW_POLICY_STORE_HANDLE  hPolicyStore,        
//        [in, range(FW_IPSEC_PHASE_INVALID+1, FW_IPSEC_PHASE_MAX-1)]             FW_IPSEC_PHASE  IpSecPhase,
//        [in] unsigned long           dwFilteredByStatus,
//        [in] unsigned short          wFlags,
//        [out, ref] unsigned long    *pdwNumSets,
//        [out] PFW_AUTH_SET          *ppCrytoSets
//        );
//
//}

typedef BOOLEAN (CALLBACK *PFW_WALK_RULES)(
    __in PFW_RULE2_0 FwRule,
    __in PVOID Context
    );

BOOLEAN IsFirewallApiInitialized(
    VOID
    );

BOOLEAN InitializeFirewallApi(
    VOID
    );

VOID FreeFirewallApi(
    VOID
    );

VOID EnumerateFirewallRules(
    __in FW_POLICY_STORE Store,
    __in FW_PROFILE_TYPE Type,
    __in FW_DIRECTION Direction,
    __in PFW_WALK_RULES Callback,
    __in PVOID Context
    );

#endif //__FIREWALL_H_