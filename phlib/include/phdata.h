#ifndef _PH_PHDATA_H
#define _PH_PHDATA_H

#ifdef __cplusplus
extern "C" {
#endif

// SIDs

extern SID PhSeNobodySid;

extern SID PhSeEveryoneSid;

extern SID PhSeLocalSid;

extern SID PhSeCreatorOwnerSid;
extern SID PhSeCreatorGroupSid;

extern SID PhSeDialupSid;
extern SID PhSeNetworkSid;
extern SID PhSeBatchSid;
extern SID PhSeInteractiveSid;
extern SID PhSeServiceSid;
extern SID PhSeAnonymousLogonSid;
extern SID PhSeProxySid;
extern SID PhSeAuthenticatedUserSid;
extern SID PhSeRestrictedCodeSid;
extern SID PhSeTerminalServerUserSid;
extern SID PhSeRemoteInteractiveLogonSid;
extern SID PhSeLocalSystemSid;
extern SID PhSeLocalServiceSid;
extern SID PhSeNetworkServiceSid;

// Unicode

extern PH_STRINGREF PhUnicodeByteOrderMark;

// Characters

extern BOOLEAN PhCharIsPrintable[256];
extern ULONG PhCharToInteger[256];
extern CHAR PhIntegerToChar[69];
extern CHAR PhIntegerToCharUpper[69];

// CRC32

extern ULONG PhCrc32Table[256];

// Enums

extern WCHAR *PhIoPriorityHintNames[MaxIoPriorityTypes];
extern WCHAR *PhPagePriorityNames[MEMORY_PRIORITY_NORMAL + 1];
extern WCHAR *PhKThreadStateNames[MaximumThreadState];
extern WCHAR *PhKWaitReasonNames[MaximumWaitReason];

#ifdef __cplusplus
}
#endif

#endif
