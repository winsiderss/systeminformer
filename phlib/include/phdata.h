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

PSID PhSeCloudActiveDirectorySid(
    VOID
    );

// Unicode

extern CONST PH_STRINGREF PhUnicodeByteOrderMark;

// Characters

extern CONST BOOLEAN PhCharIsPrintable[256];
extern CONST BOOLEAN PhCharIsPrintableEx[256];
extern CONST ULONG PhCharToInteger[256];
extern CONST CHAR PhIntegerToChar[69];
extern CONST CHAR PhIntegerToCharUpper[69];

// UTF-16

extern CONST BOOLEAN PhIsUTF16HighSurrogateHighByte[256];
extern CONST BOOLEAN PhIsUTF16LowSurrogateHighByte[256];
extern CONST BOOLEAN PhIsUTF16StandaloneHighByte[256];
extern CONST BOOLEAN PhIsUTF16PrintableHighByte[256];

// CRC32

extern CONST ULONG PhCrc32Table[256];

// Enums

extern CONST PH_STRINGREF PhIoPriorityHintNames[];
extern CONST PH_STRINGREF PhPagePriorityNames[];
extern CONST PH_STRINGREF PhKThreadStateNames[];
extern CONST PH_STRINGREF PhKWaitReasonNames[];

#ifdef __cplusplus
}
#endif

#endif
