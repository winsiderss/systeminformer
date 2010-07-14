#define DATA_PRIVATE
#include <ph.h>

// SIDs

SID PhSeNobodySid = { SID_REVISION, 1, SECURITY_NULL_SID_AUTHORITY, { SECURITY_NULL_RID } };

SID PhSeEveryoneSid = { SID_REVISION, 1, SECURITY_WORLD_SID_AUTHORITY, { SECURITY_WORLD_RID } };

SID PhSeLocalSid = { SID_REVISION, 1, SECURITY_LOCAL_SID_AUTHORITY, { SECURITY_LOCAL_RID } };

SID PhSeCreatorOwnerSid = { SID_REVISION, 1, SECURITY_CREATOR_SID_AUTHORITY, { SECURITY_CREATOR_OWNER_RID } };
SID PhSeCreatorGroupSid = { SID_REVISION, 1, SECURITY_CREATOR_SID_AUTHORITY, { SECURITY_CREATOR_GROUP_RID } };

SID PhSeDialupSid = { SID_REVISION, 1, SECURITY_NT_AUTHORITY, { SECURITY_DIALUP_RID } };
SID PhSeNetworkSid = { SID_REVISION, 1, SECURITY_NT_AUTHORITY, { SECURITY_NETWORK_RID } };
SID PhSeBatchSid = { SID_REVISION, 1, SECURITY_NT_AUTHORITY, { SECURITY_BATCH_RID } };
SID PhSeInteractiveSid = { SID_REVISION, 1, SECURITY_NT_AUTHORITY, { SECURITY_INTERACTIVE_RID } };
SID PhSeServiceSid = { SID_REVISION, 1, SECURITY_NT_AUTHORITY, { SECURITY_SERVICE_RID } };
SID PhSeAnonymousLogonSid = { SID_REVISION, 1, SECURITY_NT_AUTHORITY, { SECURITY_ANONYMOUS_LOGON_RID } };
SID PhSeProxySid = { SID_REVISION, 1, SECURITY_NT_AUTHORITY, { SECURITY_PROXY_RID } };
SID PhSeAuthenticatedUserSid = { SID_REVISION, 1, SECURITY_NT_AUTHORITY, { SECURITY_AUTHENTICATED_USER_RID } };
SID PhSeRestrictedCodeSid = { SID_REVISION, 1, SECURITY_NT_AUTHORITY, { SECURITY_RESTRICTED_CODE_RID } };
SID PhSeTerminalServerUserSid = { SID_REVISION, 1, SECURITY_NT_AUTHORITY, { SECURITY_TERMINAL_SERVER_RID } };
SID PhSeRemoteInteractiveLogonSid = { SID_REVISION, 1, SECURITY_NT_AUTHORITY, { SECURITY_REMOTE_LOGON_RID } };
SID PhSeLocalSystemSid = { SID_REVISION, 1, SECURITY_NT_AUTHORITY, { SECURITY_LOCAL_SYSTEM_RID } };
SID PhSeLocalServiceSid = { SID_REVISION, 1, SECURITY_NT_AUTHORITY, { SECURITY_LOCAL_SERVICE_RID } };
SID PhSeNetworkServiceSid = { SID_REVISION, 1, SECURITY_NT_AUTHORITY, { SECURITY_NETWORK_SERVICE_RID } };

// Characters

BOOLEAN PhCharIsPrintable[256] =
{
    0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 1, 0, 0, /* 0 - 15 */ // TAB, LF and CR are printable
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* 16 - 31 */
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, /* ' ' - '/' */
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, /* '0' - '9' */
    1, 1, 1, 1, 1, 1, 1, /* ':' - '@' */
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, /* 'A' - 'Z' */
    1, 1, 1, 1, 1, 1, /* '[' - '`' */
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, /* 'a' - 'z' */
    1, 1, 1, 1, 0, /* '{' - 127 */ // DEL is not printable
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* 128 - 143 */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* 144 - 159 */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* 160 - 175 */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* 176 - 191 */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* 192 - 207 */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* 208 - 223 */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* 224 - 239 */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* 240 - 255 */
};

ULONG PhCharToInteger[256] =
{
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* 0 - 15 */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* 16 - 31 */
    36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, /* ' ' - '/' */
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, /* '0' - '9' */
    52, 53, 54, 55, 56, 57, 58, /* ':' - '@' */
    10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, /* 'A' - 'Z' */
    59, 60, 61, 62, 63, 64, /* '[' - '`' */
    10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, /* 'a' - 'z' */
    65, 66, 67, 68, 0, /* '{' - 127 */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* 128 - 143 */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* 144 - 159 */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* 160 - 175 */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* 176 - 191 */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* 192 - 207 */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* 208 - 223 */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* 224 - 239 */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* 240 - 255 */
};

CHAR PhIntegerToChar[69] =
"0123456789" /* 0 - 9 */
"abcdefghijklmnopqrstuvwxyz" /* 10 - 35 */
" !\"#$%&'()*+,-./" /* 36 - 51 */
":;<=>?@" /* 52 - 58 */
"[\\]^_`" /* 59 - 64 */
"{|}~" /* 65 - 68 */
;

CHAR PhIntegerToCharUpper[69] =
"0123456789" /* 0 - 9 */
"ABCDEFGHIJKLMNOPQRSTUVWXYZ" /* 10 - 35 */
" !\"#$%&'()*+,-./" /* 36 - 51 */
":;<=>?@" /* 52 - 58 */
"[\\]^_`" /* 59 - 64 */
"{|}~" /* 65 - 68 */
;

// Enums

WCHAR *PhKThreadStateNames[MaximumThreadState] =
{
    L"Initialized",
    L"Ready",
    L"Running",
    L"Standby",
    L"Terminated",
    L"Waiting",
    L"Transition",
    L"DeferredReady",
    L"GateWait"
};

WCHAR *PhKWaitReasonNames[MaximumWaitReason] =
{
    L"Executive",
    L"FreePage",
    L"PageIn",
    L"PoolAllocation",
    L"DelayExecution",
    L"Suspended",
    L"UserRequest",
    L"WrExecutive",
    L"WrFreePage",
    L"WrPageIn",
    L"WrPoolAllocation",
    L"WrDelayExecution",
    L"WrSuspended",
    L"WrUserRequest",
    L"WrEventPair",
    L"WrQueue",
    L"WrLpcReceive",
    L"WrLpcReply",
    L"WrVirtualMemory",
    L"WrPageOut",
    L"WrRendezvous",
    L"Spare2",
    L"Spare3",
    L"Spare4",
    L"Spare5",
    L"WrCalloutStack",
    L"WrKernel",
    L"WrResource",
    L"WrPushLock",
    L"WrMutex",
    L"WrQuantumEnd",
    L"WrDispatchInt",
    L"WrPreempted",
    L"WrYieldExecution",
    L"WrFastMutex",
    L"WrGuardedMutex",
    L"WrRundown"
};
