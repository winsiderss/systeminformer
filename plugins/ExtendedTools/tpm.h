/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     jxy-s    2023
 *
 */

#ifndef TPM_H
#define TPM_H

#include <phdk.h>

// adapted from https://github.com/ionescu007/tpmtool

//
// TPM2.0 Session Tags
//
typedef USHORT TPM_ST;
#define TPM_ST_NO_SESSIONS ((USHORT)0x8001)
#define TPM_ST_SESSIONS    ((USHORT)0x8002)

//
// TPM2.0 Command Codes
//
typedef ULONG TPM_CC;
#define TPM_CC_NV_UndefineSpace 0x122ul
#define TPM_CC_NV_WriteLock     0x138ul
#define TPM_CC_NV_DefineSpace   0x12Aul
#define TPM_CC_NV_Write         0x137ul
#define TPM_CC_NV_Read          0x14Eul
#define TPM_CC_NV_ReadLock      0x14Ful
#define TPM_CC_NV_ReadPublic    0x169ul
#define TPM_CC_GetCapability    0x17Aul
#define TPM_CC_GetRandom        0x17Bul
#define TPM_CC_Hash             0x17Dul
#define TPM_CC_ReadClock        0x181ul

//
// TPM2.0 Response Codes
//
typedef ULONG TPM_RC;
#define TPM_RC_SUCCESS          0x0ul
#define TPM_RC_FAILURE          0x101ul
#define TPM_RC_NV_RANGE         0x146ul
#define TPM_RC_NV_LOCKED        0x148ul
#define TPM_RC_NV_AUTHORIZATION 0x149ul
#define TPM_RC_NV_UNINITIALIZED 0x14Aul
#define TPM_RC_NV_DEFINED       0x14Cul
#define TPM_RC_HANDLE_1         0x18Bul

//
// TPM2.0 Handle Types
//
typedef BYTE TPM_HT;
#define TPM_HR_PCR            ((BYTE)0)
#define TPM_HT_NV_INDEX       ((BYTE)1)
#define TPM_HT_HMAC_SESSION   ((BYTE)2)
#define TPM_HT_LOADED_SESSION TPM_HT_HMAC_SESSION
#define TPM_HT_POLICY_SESSION ((BYTE)3)
#define TPM_HT_SAVED_SESSION  TPM_HT_POLICY_SESSION
#define TPM_HT_PERMANENT      ((BYTE)0x40)
#define TPM_HT_TRANSIENT      ((BYTE)0x80)
#define TPM_HR_SHIFT          24
#define TPM_HR_NV_INDEX       (TPM_HT_NV_INDEX << TPM_HR_SHIFT)

//
// TPM2.0 Capabilities
//
typedef ULONG TPM_CAP;
#define TPM_CAP_FIRST      0x0ul,
#define TPM_CAP_ALGS       TPM_CAP_FIRST
#define TPM_CAP_HANDLES    0x1ul

//
// TPM Algorithm IDs
//
typedef USHORT TPM_ALG;
#define TPM_ALG_SHA256 ((USHORT)0x000B)

//
// TPM Attributes for Non Volatile Index Values
//
typedef ULONG TPMA_NV;

//
// Write Permissions
//
#define TPMA_NV_PPWRITE     0x00000001ul
#define TPMA_NV_OWNERWRITE  0x00000002ul
#define TPMA_NV_AUTHWRITE   0x00000004ul
#define TPMA_NV_POLICYWRITE 0x00000008ul

//
// Types
//
#define TPMA_NV_COUNTER         0x00000010ul
#define TPMA_NV_BITS            0x00000020ul
#define TPMA_NV_EXTEND          0x00000040ul
#define TPMA_NV_RESERVED_TYPE_1 0x00000080ul
#define TPMA_NV_RESERVED_TYPE_2 0x00000100ul
#define TPMA_NV_RESERVED_TYPE_3 0x00000200ul

//
// Modify Flags
//
#define TPMA_NV_POLICY_DELETE 0x00000400ul
#define TPMA_NV_WRITELOCKED   0x00000800ul
#define TPMA_NV_WRITEALL      0x00001000ul
#define TPMA_NV_WRITEDEFINE   0x00002000ul
#define TPMA_NV_WRITE_STCLEAR 0x00004000ul
#define TPMA_NV_GLOBALLOCK    0x00008000ul

//
// Read Permissions
//
#define TPMA_NV_PPREAD     0x00010000ul
#define TPMA_NV_OWNERREAD  0x00020000ul
#define TPMA_NV_AUTHREAD   0x00040000ul
#define TPMA_NV_POLICYREAD 0x00080000ul

//
// Additional Flags
//
#define TPMA_NV_RESERVED_FLAG_1 0x00100000ul
#define TPMA_NV_RESERVED_FLAG_2 0x00200000ul
#define TPMA_NV_RESERVED_FLAG_3 0x00400000ul
#define TPMA_NV_RESERVED_FLAG_4 0x00800000ul
#define TPMA_NV_RESERVED_FLAG_5 0x01000000ul
#define TPMA_NV_NO_DA           0x02000000ul
#define TPMA_NV_ORDERLY         0x04000000ul
#define TPMA_NV_CLEAR_STCLEAR   0x08000000ul
#define TPMA_NV_READLOCKED      0x10000000ul
#define TPMA_NV_WRITTEN         0x20000000ul
#define TPMA_NV_PLATFORMCREATE  0x40000000ul
#define TPMA_NV_READ_STCLEAR    0x80000000ul

//
// TPM2.0 Property Types
//
typedef ULONG TPM_PT;
#define TPM_PT_NONE  0x0ul
#define TPM_PT_FIXED 0x100ul

#include <pshpack1.h>

//
// TPM2.0 Session Attributes
//
typedef union
{
    struct
    {
        BYTE ContinueSession : 1;
        BYTE AuditExclusive : 1;
        BYTE AuditReset : 1;
        BYTE : 2;
        BYTE Decrypt : 1;
        BYTE Encrypt : 1;
        BYTE Audit : 1;
    };
    BYTE Value;
} TPMA_SESSION;

//
// TPM2.0 Yes/No Boolean
//
typedef BYTE TPMI_YES_NO;

//
// Definition of a TPM2.0 Handle
//
typedef union
{
    struct
    {
        BYTE Index[3];
        TPM_HT Type;
    };
    ULONG Value;
} TPM_HANDLE, TPMI_RH_NV_INDEX, TPMI_RH_PROVISION, TPMI_RH_HIERARCHY,
  TPMI_SH_AUTH_SESSION, TPMI_RH_NV_AUTH, TPM_RH, TPM_NV_INDEX;
C_ASSERT(sizeof(TPM_HANDLE) == sizeof(ULONG));

#define TPM_MAX_CAP_BUFFER     1024
#define TPM_MAX_CAP_DATA       (TPM_MAX_CAP_BUFFER - sizeof(ULONG) - sizeof(ULONG))
#define TPM_MAX_CAP_HANDLES    (TPM_MAX_CAP_DATA / sizeof(TPM_HANDLE))

//
// TPM2.0 Hash Algorithm Sizes
// Only SHA-256 supported for now
//
typedef union
{
    BYTE Sha256[32];
} TPMU_HA;

//
// Definition of a TPM2.0 Hash Agile Structure (Algorithm and Digest)
//
typedef struct
{
    TPM_ALG HashAlg;
    TPMU_HA Digest;
} TPMT_HA;

//
// Definition of a Hash Digest Buffer, which encodes a TPM2.0 Hash Agile
//
typedef struct
{
    USHORT BufferSize;
    TPMT_HA Buffer;
} TPM2B_DIGEST;

//
// Definition of a maximum sized buffer
//
typedef struct
{
    USHORT Size;
    BYTE Buffer[1024];
} TPM2B_MAX_BUFFER;

//
// Architecturally Defined Permanent Handles
//
#define TPM_RH_OWNER TPM_RH{ 1, 0, 0, TPM_HT_PERMANENT }
#define TPM_RH_NULL TPM_RH{ 7, 0, 0, TPM_HT_PERMANENT }
#define TPM_RS_PW TPM_RH{ 9, 0, 0, TPM_HT_PERMANENT }

//
// TPM2.0 Handle List
//
typedef struct
{
    ULONG Count;
    TPM_HANDLE Handle[TPM_MAX_CAP_HANDLES];
} TPML_HANDLE, *PTPML_HANDLE;

//
// TPM2.0 Ticket for Hash Check
//
typedef struct
{
    TPM_ST Tag;
    TPMI_RH_HIERARCHY Hierarchy;
    TPM2B_DIGEST Digest;
} TPMT_TK_HASHCHECK;

//
// TPM2.0 Union of capability data returned by TPM2_CC_GetCapabilities
//
typedef union
{
    TPML_HANDLE Handles;
} TPMU_CAPABILITIES, *PTPMU_CAPABILITIES;

//
// TPM2.0 Payload for each capablity returned by TPM2_CC_GetCapabilities
//
typedef struct
{
    TPM_CAP Capability;
    TPMU_CAPABILITIES Data;
} TPMS_CAPABILITY_DATA, *PTPMS_CAPABILITY_DATA;

//
// TPM2.0 Payload for TPM2_CC_ReadClock
//
typedef struct
{
    ULONG64 Clock;
    ULONG ResetCount;
    ULONG RestartCount;
    TPMI_YES_NO Safe;
} TPMS_CLOCK_INFO;

typedef struct
{
    ULONG64 Time;
    TPMS_CLOCK_INFO ClockInfo;
} TPMS_TIME_INFO;

//
// Definition of the header of any TPM2.0 Command
//
typedef struct
{
    TPM_ST SessionTag;
    ULONG Size;
    TPM_CC CommandCode;
} TPM_CMD_HEADER, *PTPM_CMD_HEADER;

//
// Definition of the header of any TPM2.0 Response
//
typedef struct
{
    TPM_ST SessionTag;
    ULONG Size;
    TPM_RC ResponseCode;
} TPM_REPLY_HEADER, *PTPM_REPLY_HEADER;
C_ASSERT(sizeof(TPM_CMD_HEADER) == sizeof(TPM_REPLY_HEADER));

//
// Attached to any TPM2.0 Response with TPM_ST_SESSIONS when an authorization
// session with no nonce was sent.
//
typedef struct
{
    USHORT NonceSize;
    TPMA_SESSION SessionAttributes;
    USHORT HmacSize;
} TPMS_AUTH_RESPONSE_NO_NONCE;

//
// Attached to any TPM2.0 Command with TPM_ST_SESSIONS and an authorization
// session with no nonce but with an optional HMAC/password present.
//
typedef struct
{
    ULONG SessionSize;
    TPMI_SH_AUTH_SESSION SessionHandle;
    USHORT NonceSize;
    TPMA_SESSION SessionAttributes;
    USHORT PasswordSize;
    BYTE Password[1];
} TPMS_AUTH_COMMAND_NO_NONCE, *PTPMS_AUTH_COMMAND_NO_NONCE;

//
// NV_ReadLock
//
typedef struct
{
    TPM_CMD_HEADER Header;
    TPMI_RH_PROVISION AuthHandle;
    TPMI_RH_NV_INDEX NvIndex;
    TPMS_AUTH_COMMAND_NO_NONCE AuthSession;
} TPM_NV_READ_LOCK_CMD_HEADER, *PTPM_NV_READ_LOCK_CMD_HEADER;

typedef struct
{
    TPM_REPLY_HEADER Header;
    ULONG ParameterSize;
    TPMS_AUTH_RESPONSE_NO_NONCE AuthSession;
} TPM_NV_READ_LOCK_REPLY, *PTPM_NV_READ_LOCK_REPLY;

//
// NV_WriteLock
//
typedef struct
{
    TPM_CMD_HEADER Header;
    TPMI_RH_PROVISION AuthHandle;
    TPMI_RH_NV_INDEX NvIndex;
    TPMS_AUTH_COMMAND_NO_NONCE AuthSession;
} TPM_NV_WRITE_LOCK_CMD_HEADER, *PTPM_NV_WRITE_LOCK_CMD_HEADER;

typedef struct
{
    TPM_REPLY_HEADER Header;
    ULONG ParameterSize;
    TPMS_AUTH_RESPONSE_NO_NONCE AuthSession;
} TPM_NV_WRITE_LOCK_REPLY, *PTPM_NV_WRITE_LOCK_REPLY;

//
// NV_UndefineSpace
//
typedef struct
{
    TPM_CMD_HEADER Header;
    TPMI_RH_PROVISION AuthHandle;
    TPMI_RH_NV_INDEX NvIndex;
    TPMS_AUTH_COMMAND_NO_NONCE AuthSession;
} TPM_NV_UNDEFINE_SPACE_CMD_HEADER, *PTPM_NV_UNDEFINE_SPACE_CMD_HEADER;

typedef struct
{
    TPM_REPLY_HEADER Header;
    ULONG ParameterSize;
    TPMS_AUTH_RESPONSE_NO_NONCE AuthSession;
} TPM_NV_UNDEFINE_SPACE_REPLY, *PTPM_NV_UNDEFINE_SPACE_REPLY;

//
// NV_DefineSpace
//
typedef struct
{
    TPM_CMD_HEADER Header;
    TPMI_RH_PROVISION AuthHandle;
    TPMS_AUTH_COMMAND_NO_NONCE AuthSession;
} TPM_NV_DEFINE_SPACE_CMD_HEADER, *PTPM_NV_DEFINE_SPACE_CMD_HEADER;

typedef struct
{
    USHORT AuthSize;
    BYTE Data[1];
} TPM_NV_DEFINE_SPACE_CMD_BODY, *PTPM_NV_DEFINE_SPACE_CMD_BODY;

typedef struct
{
    USHORT NvPublicSize;
    TPMI_RH_NV_INDEX NvIndex;
    TPM_ALG NameAlg;
    TPMA_NV Attributes;
    USHORT AuthPolicySize;
    USHORT DataSize;
} TPM_NV_DEFINE_SPACE_CMD_FOOTER, *PTPM_NV_DEFINE_SPACE_CMD_FOOTER;

typedef struct
{
    TPM_REPLY_HEADER Header;
    ULONG ParameterSize;
    TPMS_AUTH_RESPONSE_NO_NONCE AuthSession;
} TPM_NV_DEFINE_SPACE_REPLY, *PTPM_NV_DEFINE_SPACE_REPLY;

//
// NV_ReadPublic
//
typedef struct
{
    TPM_CMD_HEADER Header;
    TPMI_RH_NV_INDEX NvIndex;
} TPM_NV_READ_PUBLIC_CMD_HEADER, *PTPM_NV_READ_PUBLIC_CMD_HEADER;

typedef struct
{
    TPM_REPLY_HEADER Header;
    TPM_NV_DEFINE_SPACE_CMD_FOOTER NvPublic;
    TPM2B_DIGEST Name;
} TPM_NV_READ_PUBLIC_REPLY, *PTPM_NV_READ_PUBLIC_REPLY;

//
// NV_Write
//
typedef struct
{
    TPM_CMD_HEADER Header;
    TPMI_RH_NV_AUTH AuthHandle;
    TPMI_RH_NV_INDEX NvIndex;
    TPMS_AUTH_COMMAND_NO_NONCE AuthSession;
    //
    // Variable Size Auth Session HMAC
    //
    // ....
    //
    // TPM_NV_WRITE_CMD_FOOTER;
} TPM_NV_WRITE_CMD_HEADER, *PTPM_NV_WRITE_CMD_HEADER;

typedef struct
{
    USHORT Size;
    BYTE Data[1];
} TPM_NV_WRITE_CMD_BODY, *PTPM_NV_WRITE_CMD_BODY;

typedef struct
{
    USHORT Offset;
} TPM_NV_WRITE_CMD_FOOTER, *PTPM_NV_WRITE_CMD_FOOTER;

typedef struct
{
    TPM_REPLY_HEADER Header;
    ULONG ParameterSize;
    TPMS_AUTH_RESPONSE_NO_NONCE AuthSession;
} TPM_NV_WRITE_REPLY, *PTPM_NV_WRITE_REPLY;

//
// NV_Read
//
typedef struct
{
    TPM_CMD_HEADER Header;
    TPMI_RH_NV_AUTH AuthHandle;
    TPMI_RH_NV_INDEX NvIndex;
    TPMS_AUTH_COMMAND_NO_NONCE AuthSession;
    //
    // Variable Size Auth Session HMAC
    //
    // ....
    //
    // TPM_NV_READ_CMD_FOOTER;
} TPM_NV_READ_CMD_HEADER, *PTPM_NV_READ_CMD_HEADER;

typedef struct
{
    USHORT Size;
    USHORT Offset;
} TPM_NV_READ_CMD_FOOTER, *PTPM_NV_READ_CMD_FOOTER;

typedef struct
{
    TPM_REPLY_HEADER Header;
    ULONG ParameterSize;
    USHORT DataSize;
    BYTE Data[1];
    //
    // TPMS_AUTH_RESPONSE_NO_NONCE AuthSession;
    //
} TPM_NV_READ_REPLY, *PTPM_NV_READ_REPLY;

//
// Get_Capabilities
//
typedef struct
{
    TPM_CMD_HEADER Header;
    TPM_CAP Capability;
    TPM_PT Property;
    ULONG PropertyCount;
} TPM_GET_CAPABILITY_CMD_HEADER, *PTPM_GET_CAPABILITY_CMD_HEADER;

typedef struct
{
    TPM_REPLY_HEADER Header;
    TPMI_YES_NO MoreData;
    TPMS_CAPABILITY_DATA Data;
} TPM_GET_CAPABILITY_REPLY, *PTPM_GET_CAPABILITY_REPLY;

// GetRandom
typedef struct
{
    TPM_CMD_HEADER Header;
    USHORT BytesRequested;
} TPM_GET_RANDOM_CMD_HEADER, * PTPM_GET_RANDOM_CMD_HEADER;

typedef struct
{
    TPM_REPLY_HEADER Header;
    TPM2B_DIGEST RandomBytes;
} TPM_GET_RANDOM_REPLY;

// ReadClock
typedef struct
{
    TPM_CMD_HEADER Header;
} TPM_READ_CLOCK_CMD_HEADER, *PTPM_READ_CLOCK_CMD_HEADER;

typedef struct
{
    TPM_REPLY_HEADER Header;
    TPMS_TIME_INFO TimeInfo;
} TPM_READ_CLOCK_REPLY;

//
// Hash
//
typedef struct
{
    TPM_CMD_HEADER Header;
    //
    // Variable Size Hash Input
    //
    // ....
    //
    // TPM_HASH_CMD_FOOTER;
} TPM_HASH_CMD_HEADER, *PTPM_HASH_CMD_HEADER;

typedef struct
{
    USHORT Size;
    BYTE Buffer[1];
} TPM_HASH_CMD_BODY, *PTPM_HASH_CMD_BODY;

typedef struct
{
    TPM_ALG AlgHash;
    TPMI_RH_HIERARCHY Hierarchy;
} TPM_HASH_CMD_FOOTER, *PTPM_HASH_CMD_FOOTER;

typedef struct
{
    TPM_REPLY_HEADER Header;
    TPM2B_DIGEST OutHash;
    TPMT_TK_HASHCHECK Validation;
} TPM_HASH_CMD_REPLY;

#include <poppack.h>

#endif
