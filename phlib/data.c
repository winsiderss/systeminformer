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

// Strings

CHAR PhIntegerToChar[16] = "0123456789abcdef";
CHAR PhIntegerToCharUpper[16] = "0123456789ABCDEF";

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
