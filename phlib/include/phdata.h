/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2016
 *     dmex    2023
 *
 */

#ifndef _PH_PHDATA_H
#define _PH_PHDATA_H

#ifdef __cplusplus
extern "C" {
#endif

// SIDs

extern CONST SID PhSeNobodySid;

extern CONST SID PhSeEveryoneSid;

extern CONST SID PhSeLocalSid;

extern CONST SID PhSeCreatorOwnerSid;
extern CONST SID PhSeCreatorGroupSid;

extern CONST SID PhSeDialupSid;
extern CONST SID PhSeNetworkSid;
extern CONST SID PhSeBatchSid;
extern CONST SID PhSeInteractiveSid;
extern CONST SID PhSeServiceSid;
extern CONST SID PhSeAnonymousLogonSid;
extern CONST SID PhSeProxySid;
extern CONST SID PhSeAuthenticatedUserSid;
extern CONST SID PhSeRestrictedCodeSid;
extern CONST SID PhSeTerminalServerUserSid;
extern CONST SID PhSeRemoteInteractiveLogonSid;
extern CONST SID PhSeLocalSystemSid;
extern CONST SID PhSeLocalServiceSid;
extern CONST SID PhSeNetworkServiceSid;

PSID PhSeAdministratorsSid(
    VOID
    );

PSID PhSeUsersSid(
    VOID
    );

PSID PhSeAnyPackageSid(
    VOID
    );

PSID PhSeInternetExplorerSid(
    VOID
    );

// Unicode

extern CONST PH_STRINGREF PhUnicodeByteOrderMark;

// Characters

extern CONST BOOLEAN PhCharIsPrintable[256];
extern CONST ULONG PhCharToInteger[256];
extern CONST CHAR PhIntegerToChar[69];
extern CONST CHAR PhIntegerToCharUpper[69];

// CRC32

extern CONST ULONG PhCrc32Table[256];

// Enums

extern CONST PH_STRINGREF PhIoPriorityHintNames[MaxIoPriorityTypes];
extern CONST PH_STRINGREF PhPagePriorityNames[MEMORY_PRIORITY_NORMAL + 1];
extern CONST PH_STRINGREF PhKThreadStateNames[MaximumThreadState];
extern CONST PH_STRINGREF PhKWaitReasonNames[MaximumWaitReason];

#ifdef __cplusplus
}
#endif

#endif
