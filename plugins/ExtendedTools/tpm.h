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
#define TPM_ST_RSP_COMMAND           ((USHORT)0x00C4)
#define TPM_ST_NULL                  ((USHORT)0X8000)
#define TPM_ST_NO_SESSIONS           ((USHORT)0x8001)
#define TPM_ST_SESSIONS              ((USHORT)0x8002)
#define TPM_ST_ATTEST_NV             ((USHORT)0x8014)
#define TPM_ST_ATTEST_COMMAND_AUDIT  ((USHORT)0x8015)
#define TPM_ST_ATTEST_SESSION_AUDIT  ((USHORT)0x8016)
#define TPM_ST_ATTEST_CERTIFY        ((USHORT)0x8017)
#define TPM_ST_ATTEST_QUOTE          ((USHORT)0x8018)
#define TPM_ST_ATTEST_TIME           ((USHORT)0x8019)
#define TPM_ST_ATTEST_CREATION       ((USHORT)0x801A)
#define TPM_ST_CREATION              ((USHORT)0x8021)
#define TPM_ST_VERIFIED              ((USHORT)0x8022)
#define TPM_ST_AUTH_SECRET           ((USHORT)0x8023)
#define TPM_ST_HASHCHECK             ((USHORT)0x8024)
#define TPM_ST_AUTH_SIGNED           ((USHORT)0x8025)
#define TPM_ST_FU_MANIFEST           ((USHORT)0x8029)

//
// TPM2.0 Command Codes
//
typedef ULONG TPM_CC;
#define TPM_CC_NV_UndefineSpaceSpecial    0x0000011FUL
#define TPM_CC_EvictControl               0x00000120UL
#define TPM_CC_HierarchyControl           0x00000121UL
#define TPM_CC_NV_UndefineSpace           0x00000122UL
#define TPM_CC_ChangeEPS                  0x00000124UL
#define TPM_CC_ChangePPS                  0x00000125UL
#define TPM_CC_Clear                      0x00000126UL
#define TPM_CC_ClearControl               0x00000127UL
#define TPM_CC_ClockSet                   0x00000128UL
#define TPM_CC_HierarchyChangeAuth        0x00000129UL
#define TPM_CC_NV_DefineSpace             0x0000012AUL
#define TPM_CC_PCR_Allocate               0x0000012BUL
#define TPM_CC_PCR_SetAuthPolicy          0x0000012CUL
#define TPM_CC_PP_Commands                0x0000012DUL
#define TPM_CC_SetPrimaryPolicy           0x0000012EUL
#define TPM_CC_FieldUpgradeStart          0x0000012FUL
#define TPM_CC_ClockRateAdjust            0x00000130UL
#define TPM_CC_CreatePrimary              0x00000131UL
#define TPM_CC_NV_GlobalWriteLock         0x00000132UL
#define TPM_CC_GetCommandAuditDigest      0x00000133UL
#define TPM_CC_NV_Increment               0x00000134UL
#define TPM_CC_NV_SetBits                 0x00000135UL
#define TPM_CC_NV_Extend                  0x00000136UL
#define TPM_CC_NV_Write                   0x00000137UL
#define TPM_CC_NV_WriteLock               0x00000138UL
#define TPM_CC_DictionaryAttackLockReset  0x00000139UL
#define TPM_CC_DictionaryAttackParameters 0x0000013AUL
#define TPM_CC_NV_ChangeAuth              0x0000013BUL
#define TPM_CC_PCR_Event                  0x0000013CUL
#define TPM_CC_PCR_Reset                  0x0000013DUL
#define TPM_CC_SequenceComplete           0x0000013EUL
#define TPM_CC_SetAlgorithmSet            0x0000013FUL
#define TPM_CC_SetCommandCodeAuditStatus  0x00000140UL
#define TPM_CC_FieldUpgradeData           0x00000141UL
#define TPM_CC_IncrementalSelfTest        0x00000142UL
#define TPM_CC_SelfTest                   0x00000143UL
#define TPM_CC_Startup                    0x00000144UL
#define TPM_CC_Shutdown                   0x00000145UL
#define TPM_CC_StirRandom                 0x00000146UL
#define TPM_CC_ActivateCredential         0x00000147UL
#define TPM_CC_Certify                    0x00000148UL
#define TPM_CC_PolicyNV                   0x00000149UL
#define TPM_CC_CertifyCreation            0x0000014AUL
#define TPM_CC_Duplicate                  0x0000014BUL
#define TPM_CC_GetTime                    0x0000014CUL
#define TPM_CC_GetSessionAuditDigest      0x0000014DUL
#define TPM_CC_NV_Read                    0x0000014EUL
#define TPM_CC_NV_ReadLock                0x0000014FUL
#define TPM_CC_ObjectChangeAuth           0x00000150UL
#define TPM_CC_PolicySecret               0x00000151UL
#define TPM_CC_Rewrap                     0x00000152UL
#define TPM_CC_Create                     0x00000153UL
#define TPM_CC_ECDH_ZGen                  0x00000154UL
#define TPM_CC_HMAC                       0x00000155UL
#define TPM_CC_Import                     0x00000156UL
#define TPM_CC_Load                       0x00000157UL
#define TPM_CC_Quote                      0x00000158UL
#define TPM_CC_RSA_Decrypt                0x00000159UL
#define TPM_CC_HMAC_Start                 0x0000015BUL
#define TPM_CC_SequenceUpdate             0x0000015CUL
#define TPM_CC_Sign                       0x0000015DUL
#define TPM_CC_Unseal                     0x0000015EUL
#define TPM_CC_PolicySigned               0x00000160UL
#define TPM_CC_ContextLoad                0x00000161UL
#define TPM_CC_ContextSave                0x00000162UL
#define TPM_CC_ECDH_KeyGen                0x00000163UL
#define TPM_CC_EncryptDecrypt             0x00000164UL
#define TPM_CC_FlushContext               0x00000165UL
#define TPM_CC_LoadExternal               0x00000167UL
#define TPM_CC_MakeCredential             0x00000168UL
#define TPM_CC_NV_ReadPublic              0x00000169UL
#define TPM_CC_PolicyAuthorize            0x0000016AUL
#define TPM_CC_PolicyAuthValue            0x0000016BUL
#define TPM_CC_PolicyCommandCode          0x0000016CUL
#define TPM_CC_PolicyCounterTimer         0x0000016DUL
#define TPM_CC_PolicyCpHash               0x0000016EUL
#define TPM_CC_PolicyLocality             0x0000016FUL
#define TPM_CC_PolicyNameHash             0x00000170UL
#define TPM_CC_PolicyOR                   0x00000171UL
#define TPM_CC_PolicyTicket               0x00000172UL
#define TPM_CC_ReadPublic                 0x00000173UL
#define TPM_CC_RSA_Encrypt                0x00000174UL
#define TPM_CC_StartAuthSession           0x00000176UL
#define TPM_CC_VerifySignature            0x00000177UL
#define TPM_CC_ECC_Parameters             0x00000178UL
#define TPM_CC_FirmwareRead               0x00000179UL
#define TPM_CC_GetCapability              0x0000017AUL
#define TPM_CC_GetRandom                  0x0000017BUL
#define TPM_CC_GetTestResult              0x0000017CUL
#define TPM_CC_Hash                       0x0000017DUL
#define TPM_CC_PCR_Read                   0x0000017EUL
#define TPM_CC_PolicyPCR                  0x0000017FUL
#define TPM_CC_PolicyRestart              0x00000180UL
#define TPM_CC_ReadClock                  0x00000181UL
#define TPM_CC_PCR_Extend                 0x00000182UL
#define TPM_CC_PCR_SetAuthValue           0x00000183UL
#define TPM_CC_NV_Certify                 0x00000184UL
#define TPM_CC_EventSequenceComplete      0x00000185UL
#define TPM_CC_HashSequenceStart          0x00000186UL
#define TPM_CC_PolicyPhysicalPresence     0x00000187UL
#define TPM_CC_PolicyDuplicationSelect    0x00000188UL
#define TPM_CC_PolicyGetDigest            0x00000189UL
#define TPM_CC_TestParms                  0x0000018AUL
#define TPM_CC_Commit                     0x0000018BUL
#define TPM_CC_PolicyPassword             0x0000018CUL
#define TPM_CC_ZGen_2Phase                0x0000018DUL
#define TPM_CC_EC_Ephemeral               0x0000018EUL
#define TPM_CC_PolicyNvWritten            0x0000018FUL
#define TPM_CC_PolicyTemplate             0x00000190UL
#define TPM_CC_CreateLoaded               0x00000191UL
#define TPM_CC_PolicyAuthorizeNV          0x00000192UL
#define TPM_CC_EncryptDecrypt2            0x00000193UL
#define TPM_CC_AC_GetCapability           0x00000194UL
#define TPM_CC_AC_Send                    0x00000195UL
#define TPM_CC_Policy_AC_SendSelect       0x00000196UL
#define TPM_CC_CertifyX509                0x00000197UL
#define TPM_CC_ACT_SetTimeout             0x00000198UL
#define TPM_CC_ECC_Encrypt                0x00000199UL
#define TPM_CC_ECC_Decrypt                0x0000019AUL

//
// TPM2.0 Response Codes
//
typedef ULONG TPM_RC;
#define TPM_RC_SUCCESS                                  0x000
#define TPM_RC_BAD_TAG                                  0x01E
#define RC_VER1                                         0x100
#define TPM_RC_INITIALIZE         ((TPM_RC)(RC_VER1 + 0x000))
#define TPM_RC_FAILURE            ((TPM_RC)(RC_VER1 + 0x001))
#define TPM_RC_SEQUENCE           ((TPM_RC)(RC_VER1 + 0x003))
#define TPM_RC_PRIVATE            ((TPM_RC)(RC_VER1 + 0x00B))
#define TPM_RC_HMAC               ((TPM_RC)(RC_VER1 + 0x019))
#define TPM_RC_DISABLED           ((TPM_RC)(RC_VER1 + 0x020))
#define TPM_RC_EXCLUSIVE          ((TPM_RC)(RC_VER1 + 0x021))
#define TPM_RC_AUTH_TYPE          ((TPM_RC)(RC_VER1 + 0x024))
#define TPM_RC_AUTH_MISSING       ((TPM_RC)(RC_VER1 + 0x025))
#define TPM_RC_POLICY             ((TPM_RC)(RC_VER1 + 0x026))
#define TPM_RC_PCR                ((TPM_RC)(RC_VER1 + 0x027))
#define TPM_RC_PCR_CHANGED        ((TPM_RC)(RC_VER1 + 0x028))
#define TPM_RC_UPGRADE            ((TPM_RC)(RC_VER1 + 0x02D))
#define TPM_RC_TOO_MANY_CONTEXTS  ((TPM_RC)(RC_VER1 + 0x02E))
#define TPM_RC_AUTH_UNAVAILABLE   ((TPM_RC)(RC_VER1 + 0x02F))
#define TPM_RC_REBOOT             ((TPM_RC)(RC_VER1 + 0x030))
#define TPM_RC_UNBALANCED         ((TPM_RC)(RC_VER1 + 0x031))
#define TPM_RC_COMMAND_SIZE       ((TPM_RC)(RC_VER1 + 0x042))
#define TPM_RC_COMMAND_CODE       ((TPM_RC)(RC_VER1 + 0x043))
#define TPM_RC_AUTHSIZE           ((TPM_RC)(RC_VER1 + 0x044))
#define TPM_RC_AUTH_CONTEXT       ((TPM_RC)(RC_VER1 + 0x045))
#define TPM_RC_NV_RANGE           ((TPM_RC)(RC_VER1 + 0x046))
#define TPM_RC_NV_SIZE            ((TPM_RC)(RC_VER1 + 0x047))
#define TPM_RC_NV_LOCKED          ((TPM_RC)(RC_VER1 + 0x048))
#define TPM_RC_NV_AUTHORIZATION   ((TPM_RC)(RC_VER1 + 0x049))
#define TPM_RC_NV_UNINITIALIZED   ((TPM_RC)(RC_VER1 + 0x04A))
#define TPM_RC_NV_SPACE           ((TPM_RC)(RC_VER1 + 0x04B))
#define TPM_RC_NV_DEFINED         ((TPM_RC)(RC_VER1 + 0x04C))
#define TPM_RC_BAD_CONTEXT        ((TPM_RC)(RC_VER1 + 0x050))
#define TPM_RC_CPHASH             ((TPM_RC)(RC_VER1 + 0x051))
#define TPM_RC_PARENT             ((TPM_RC)(RC_VER1 + 0x052))
#define TPM_RC_NEEDS_TEST         ((TPM_RC)(RC_VER1 + 0x053))
#define TPM_RC_NO_RESULT          ((TPM_RC)(RC_VER1 + 0x054))
#define TPM_RC_SENSITIVE          ((TPM_RC)(RC_VER1 + 0x055))
#define RC_MAX_FM0                ((TPM_RC)(RC_VER1 + 0x07F))
#define RC_FMT1                                         0x080
#define TPM_RC_ASYMMETRIC         ((TPM_RC)(RC_FMT1 + 0x001))
#define TPM_RC_ATTRIBUTES         ((TPM_RC)(RC_FMT1 + 0x002))
#define TPM_RC_HASH               ((TPM_RC)(RC_FMT1 + 0x003))
#define TPM_RC_VALUE              ((TPM_RC)(RC_FMT1 + 0x004))
#define TPM_RC_HIERARCHY          ((TPM_RC)(RC_FMT1 + 0x005))
#define TPM_RC_KEY_SIZE           ((TPM_RC)(RC_FMT1 + 0x007))
#define TPM_RC_MGF                ((TPM_RC)(RC_FMT1 + 0x008))
#define TPM_RC_MODE               ((TPM_RC)(RC_FMT1 + 0x009))
#define TPM_RC_TYPE               ((TPM_RC)(RC_FMT1 + 0x00A))
#define TPM_RC_HANDLE             ((TPM_RC)(RC_FMT1 + 0x00B))
#define TPM_RC_KDF                ((TPM_RC)(RC_FMT1 + 0x00C))
#define TPM_RC_RANGE              ((TPM_RC)(RC_FMT1 + 0x00D))
#define TPM_RC_AUTH_FAIL          ((TPM_RC)(RC_FMT1 + 0x00E))
#define TPM_RC_NONCE              ((TPM_RC)(RC_FMT1 + 0x00F))
#define TPM_RC_PP                 ((TPM_RC)(RC_FMT1 + 0x010))
#define TPM_RC_SCHEME             ((TPM_RC)(RC_FMT1 + 0x012))
#define TPM_RC_SIZE               ((TPM_RC)(RC_FMT1 + 0x015))
#define TPM_RC_SYMMETRIC          ((TPM_RC)(RC_FMT1 + 0x016))
#define TPM_RC_TAG                ((TPM_RC)(RC_FMT1 + 0x017))
#define TPM_RC_SELECTOR           ((TPM_RC)(RC_FMT1 + 0x018))
#define TPM_RC_INSUFFICIENT       ((TPM_RC)(RC_FMT1 + 0x01A))
#define TPM_RC_SIGNATURE          ((TPM_RC)(RC_FMT1 + 0x01B))
#define TPM_RC_KEY                ((TPM_RC)(RC_FMT1 + 0x01C))
#define TPM_RC_POLICY_FAIL        ((TPM_RC)(RC_FMT1 + 0x01D))
#define TPM_RC_INTEGRITY          ((TPM_RC)(RC_FMT1 + 0x01F))
#define TPM_RC_TICKET             ((TPM_RC)(RC_FMT1 + 0x020))
#define TPM_RC_RESERVED_BITS      ((TPM_RC)(RC_FMT1 + 0x021))
#define TPM_RC_BAD_AUTH           ((TPM_RC)(RC_FMT1 + 0x022))
#define TPM_RC_EXPIRED            ((TPM_RC)(RC_FMT1 + 0x023))
#define TPM_RC_POLICY_CC          ((TPM_RC)(RC_FMT1 + 0x024))
#define TPM_RC_BINDING            ((TPM_RC)(RC_FMT1 + 0x025))
#define TPM_RC_CURVE              ((TPM_RC)(RC_FMT1 + 0x026))
#define TPM_RC_ECC_POINT          ((TPM_RC)(RC_FMT1 + 0x027))
#define RC_WARN                                         0x900
#define TPM_RC_CONTEXT_GAP        ((TPM_RC)(RC_WARN + 0x001))
#define TPM_RC_OBJECT_MEMORY      ((TPM_RC)(RC_WARN + 0x002))
#define TPM_RC_SESSION_MEMORY     ((TPM_RC)(RC_WARN + 0x003))
#define TPM_RC_MEMORY             ((TPM_RC)(RC_WARN + 0x004))
#define TPM_RC_SESSION_HANDLES    ((TPM_RC)(RC_WARN + 0x005))
#define TPM_RC_OBJECT_HANDLES     ((TPM_RC)(RC_WARN + 0x006))
#define TPM_RC_LOCALITY           ((TPM_RC)(RC_WARN + 0x007))
#define TPM_RC_YIELDED            ((TPM_RC)(RC_WARN + 0x008))
#define TPM_RC_CANCELED           ((TPM_RC)(RC_WARN + 0x009))
#define TPM_RC_TESTING            ((TPM_RC)(RC_WARN + 0x00A))
#define TPM_RC_REFERENCE_H0       ((TPM_RC)(RC_WARN + 0x010))
#define TPM_RC_REFERENCE_H1       ((TPM_RC)(RC_WARN + 0x011))
#define TPM_RC_REFERENCE_H2       ((TPM_RC)(RC_WARN + 0x012))
#define TPM_RC_REFERENCE_H3       ((TPM_RC)(RC_WARN + 0x013))
#define TPM_RC_REFERENCE_H4       ((TPM_RC)(RC_WARN + 0x014))
#define TPM_RC_REFERENCE_H5       ((TPM_RC)(RC_WARN + 0x015))
#define TPM_RC_REFERENCE_H6       ((TPM_RC)(RC_WARN + 0x016))
#define TPM_RC_REFERENCE_S0       ((TPM_RC)(RC_WARN + 0x018))
#define TPM_RC_REFERENCE_S1       ((TPM_RC)(RC_WARN + 0x019))
#define TPM_RC_REFERENCE_S2       ((TPM_RC)(RC_WARN + 0x01A))
#define TPM_RC_REFERENCE_S3       ((TPM_RC)(RC_WARN + 0x01B))
#define TPM_RC_REFERENCE_S4       ((TPM_RC)(RC_WARN + 0x01C))
#define TPM_RC_REFERENCE_S5       ((TPM_RC)(RC_WARN + 0x01D))
#define TPM_RC_REFERENCE_S6       ((TPM_RC)(RC_WARN + 0x01E))
#define TPM_RC_NV_RATE            ((TPM_RC)(RC_WARN + 0x020))
#define TPM_RC_LOCKOUT            ((TPM_RC)(RC_WARN + 0x021))
#define TPM_RC_RETRY              ((TPM_RC)(RC_WARN + 0x022))
#define TPM_RC_NV_UNAVAILABLE     ((TPM_RC)(RC_WARN + 0x023))
#define TPM_RC_NOT_USED            ((TPM_RC)(RC_WARN + 0x7F))
#define TPM_RC_H                                        0x000
#define TPM_RC_P                                        0x040
#define TPM_RC_S                                        0x800
#define TPM_RC_1                                        0x100
#define TPM_RC_2                                        0x200
#define TPM_RC_3                                        0x300
#define TPM_RC_4                                        0x400
#define TPM_RC_5                                        0x500
#define TPM_RC_6                                        0x600
#define TPM_RC_7                                        0x700
#define TPM_RC_8                                        0x800
#define TPM_RC_9                                        0x900
#define TPM_RC_A                                        0xA00
#define TPM_RC_B                                        0xB00
#define TPM_RC_C                                        0xC00
#define TPM_RC_D                                        0xD00
#define TPM_RC_E                                        0xE00
#define TPM_RC_F                                        0xF00
#define TPM_RC_N_MASK                                   0xF00

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
#define TPM_HT_PERSISTENT     ((BYTE)0x81)
#define TPM_HR_SHIFT          24
#define TPM_HR_NV_INDEX       (TPM_HT_NV_INDEX << TPM_HR_SHIFT)

//
// TPM2.0 Capabilities
//
typedef ULONG TPM_CAP;
#define TPM_CAP_FIRST            0x00000000UL
#define TPM_CAP_ALGS             TPM_CAP_FIRST
#define TPM_CAP_HANDLES          0x00000001UL
#define TPM_CAP_COMMANDS         0x00000002UL
#define TPM_CAP_PP_COMMANDS      0x00000003UL
#define TPM_CAP_AUDIT_COMMANDS   0x00000004UL
#define TPM_CAP_PCRS             0x00000005UL
#define TPM_CAP_TPM_PROPERTIES   0x00000006UL
#define TPM_CAP_PCR_PROPERTIES   0x00000007UL
#define TPM_CAP_ECC_CURVES       0x00000008UL
#define TPM_CAP_LAST             0x00000008UL
#define TPM_CAP_VENDOR_PROPERTY  0x00000100UL

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
#define TPM_PT_NONE                                0x00000000
#define PT_GROUP                                   0x00000100
#define PT_FIXED                               (PT_GROUP * 1)
#define TPM_PT_FAMILY_INDICATOR      ((TPM_PT)(PT_FIXED + 0))
#define TPM_PT_LEVEL                 ((TPM_PT)(PT_FIXED + 1))
#define TPM_PT_REVISION              ((TPM_PT)(PT_FIXED + 2))
#define TPM_PT_DAY_OF_YEAR           ((TPM_PT)(PT_FIXED + 3))
#define TPM_PT_YEAR                  ((TPM_PT)(PT_FIXED + 4))
#define TPM_PT_MANUFACTURER          ((TPM_PT)(PT_FIXED + 5))
#define TPM_PT_VENDOR_STRING_1       ((TPM_PT)(PT_FIXED + 6))
#define TPM_PT_VENDOR_STRING_2       ((TPM_PT)(PT_FIXED + 7))
#define TPM_PT_VENDOR_STRING_3       ((TPM_PT)(PT_FIXED + 8))
#define TPM_PT_VENDOR_STRING_4       ((TPM_PT)(PT_FIXED + 9))
#define TPM_PT_VENDOR_TPM_TYPE      ((TPM_PT)(PT_FIXED + 10))
#define TPM_PT_FIRMWARE_VERSION_1   ((TPM_PT)(PT_FIXED + 11))
#define TPM_PT_FIRMWARE_VERSION_2   ((TPM_PT)(PT_FIXED + 12))
#define TPM_PT_INPUT_BUFFER         ((TPM_PT)(PT_FIXED + 13))
#define TPM_PT_HR_TRANSIENT_MIN     ((TPM_PT)(PT_FIXED + 14))
#define TPM_PT_HR_PERSISTENT_MIN    ((TPM_PT)(PT_FIXED + 15))
#define TPM_PT_HR_LOADED_MIN        ((TPM_PT)(PT_FIXED + 16))
#define TPM_PT_ACTIVE_SESSIONS_MAX  ((TPM_PT)(PT_FIXED + 17))
#define TPM_PT_PCR_COUNT            ((TPM_PT)(PT_FIXED + 18))
#define TPM_PT_PCR_SELECT_MIN       ((TPM_PT)(PT_FIXED + 19))
#define TPM_PT_CONTEXT_GAP_MAX      ((TPM_PT)(PT_FIXED + 20))
#define TPM_PT_NV_COUNTERS_MAX      ((TPM_PT)(PT_FIXED + 22))
#define TPM_PT_NV_INDEX_MAX         ((TPM_PT)(PT_FIXED + 23))
#define TPM_PT_MEMORY               ((TPM_PT)(PT_FIXED + 24))
#define TPM_PT_CLOCK_UPDATE         ((TPM_PT)(PT_FIXED + 25))
#define TPM_PT_CONTEXT_HASH         ((TPM_PT)(PT_FIXED + 26))
#define TPM_PT_CONTEXT_SYM          ((TPM_PT)(PT_FIXED + 27))
#define TPM_PT_CONTEXT_SYM_SIZE     ((TPM_PT)(PT_FIXED + 28))
#define TPM_PT_ORDERLY_COUNT        ((TPM_PT)(PT_FIXED + 29))
#define TPM_PT_MAX_COMMAND_SIZE     ((TPM_PT)(PT_FIXED + 30))
#define TPM_PT_MAX_RESPONSE_SIZE    ((TPM_PT)(PT_FIXED + 31))
#define TPM_PT_MAX_DIGEST           ((TPM_PT)(PT_FIXED + 32))
#define TPM_PT_MAX_OBJECT_CONTEXT   ((TPM_PT)(PT_FIXED + 33))
#define TPM_PT_MAX_SESSION_CONTEXT  ((TPM_PT)(PT_FIXED + 34))
#define TPM_PT_PS_FAMILY_INDICATOR  ((TPM_PT)(PT_FIXED + 35))
#define TPM_PT_PS_LEVEL             ((TPM_PT)(PT_FIXED + 36))
#define TPM_PT_PS_REVISION          ((TPM_PT)(PT_FIXED + 37))
#define TPM_PT_PS_DAY_OF_YEAR       ((TPM_PT)(PT_FIXED + 38))
#define TPM_PT_PS_YEAR              ((TPM_PT)(PT_FIXED + 39))
#define TPM_PT_SPLIT_MAX            ((TPM_PT)(PT_FIXED + 40))
#define TPM_PT_TOTAL_COMMANDS       ((TPM_PT)(PT_FIXED + 41))
#define TPM_PT_LIBRARY_COMMANDS     ((TPM_PT)(PT_FIXED + 42))
#define TPM_PT_VENDOR_COMMANDS      ((TPM_PT)(PT_FIXED + 43))
#define TPM_PT_NV_BUFFER_MAX        ((TPM_PT)(PT_FIXED + 44))
#define PT_VAR                                 (PT_GROUP * 2)
#define TPM_PT_PERMANENT               ((TPM_PT)(PT_VAR + 0))
#define TPM_PT_STARTUP_CLEAR           ((TPM_PT)(PT_VAR + 1))
#define TPM_PT_HR_NV_INDEX             ((TPM_PT)(PT_VAR + 2))
#define TPM_PT_HR_LOADED               ((TPM_PT)(PT_VAR + 3))
#define TPM_PT_HR_LOADED_AVAIL         ((TPM_PT)(PT_VAR + 4))
#define TPM_PT_HR_ACTIVE               ((TPM_PT)(PT_VAR + 5))
#define TPM_PT_HR_ACTIVE_AVAIL         ((TPM_PT)(PT_VAR + 6))
#define TPM_PT_HR_TRANSIENT_AVAIL      ((TPM_PT)(PT_VAR + 7))
#define TPM_PT_HR_PERSISTENT           ((TPM_PT)(PT_VAR + 8))
#define TPM_PT_HR_PERSISTENT_AVAIL     ((TPM_PT)(PT_VAR + 9))
#define TPM_PT_NV_COUNTERS            ((TPM_PT)(PT_VAR + 10))
#define TPM_PT_NV_COUNTERS_AVAIL      ((TPM_PT)(PT_VAR + 11))
#define TPM_PT_ALGORITHM_SET          ((TPM_PT)(PT_VAR + 12))
#define TPM_PT_LOADED_CURVES          ((TPM_PT)(PT_VAR + 13))
#define TPM_PT_LOCKOUT_COUNTER        ((TPM_PT)(PT_VAR + 14))
#define TPM_PT_MAX_AUTH_FAIL          ((TPM_PT)(PT_VAR + 15))
#define TPM_PT_LOCKOUT_INTERVAL       ((TPM_PT)(PT_VAR + 16))
#define TPM_PT_LOCKOUT_RECOVERY       ((TPM_PT)(PT_VAR + 17))
#define TPM_PT_NV_WRITE_RECOVERY      ((TPM_PT)(PT_VAR + 18))
#define TPM_PT_AUDIT_COUNTER_0        ((TPM_PT)(PT_VAR + 19))
#define TPM_PT_AUDIT_COUNTER_1        ((TPM_PT)(PT_VAR + 20))

#include <pshpack1.h>

//
// TPM2.0 Session Attributes
//
typedef union _TPMA_SESSION
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
typedef union _TPM_HANDLE
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
typedef union _TPMU_HA
{
    BYTE Sha256[32];
} TPMU_HA;

//
// Definition of a TPM2.0 Hash Agile Structure (Algorithm and Digest)
//
typedef struct _TPMT_HA
{
    TPM_ALG HashAlg;
    TPMU_HA Digest;
} TPMT_HA;

//
// Definition of a Hash Digest Buffer, which encodes a TPM2.0 Hash Agile
//
typedef struct _TPM2B_DIGEST
{
    USHORT BufferSize;
    TPMT_HA Buffer;
} TPM2B_DIGEST;

//
// Definition of a maximum sized buffer
//
typedef struct _TPM2B_MAX_BUFFER
{
    USHORT Size;
    BYTE Buffer[1024];
} TPM2B_MAX_BUFFER;

//
// Architecturally Defined Permanent Handles
//
#define TPM_RH_FIRST        { 0x00, 0x00, 0x00, TPM_HT_PERMANENT }
#define TPM_RH_SRK          { 0x00, 0x00, 0x00, TPM_HT_PERMANENT }
#define TPM_RH_OWNER        { 0x01, 0x00, 0x00, TPM_HT_PERMANENT }
#define TPM_RH_REVOKE       { 0x02, 0x00, 0x00, TPM_HT_PERMANENT }
#define TPM_RH_TRANSPORT    { 0x03, 0x00, 0x00, TPM_HT_PERMANENT }
#define TPM_RH_OPERATOR     { 0x04, 0x00, 0x00, TPM_HT_PERMANENT }
#define TPM_RH_ADMIN        { 0x05, 0x00, 0x00, TPM_HT_PERMANENT }
#define TPM_RH_EK           { 0x06, 0x00, 0x00, TPM_HT_PERMANENT }
#define TPM_RH_NULL         { 0x07, 0x00, 0x00, TPM_HT_PERMANENT }
#define TPM_RH_UNASSIGNED   { 0x08, 0x00, 0x00, TPM_HT_PERMANENT }
#define TPM_RS_PW           { 0x09, 0x00, 0x00, TPM_HT_PERMANENT }
#define TPM_RH_LOCKOUT      { 0x0A, 0x00, 0x00, TPM_HT_PERMANENT }
#define TPM_RH_ENDORSEMENT  { 0x0B, 0x00, 0x00, TPM_HT_PERMANENT }
#define TPM_RH_PLATFORM     { 0x0C, 0x00, 0x00, TPM_HT_PERMANENT }
#define TPM_RH_PLATFORM_NV  { 0x0D, 0x00, 0x00, TPM_HT_PERMANENT }
#define TPM_RH_AUTH_00      { 0x10, 0x00, 0x00, TPM_HT_PERMANENT }
#define TPM_RH_AUTH_FF      { 0x0F, 0x10, 0x00, TPM_HT_PERMANENT }
#define TPM_RH_LAST         { 0x0F, 0x10, 0x00, TPM_HT_PERMANENT }
extern const TPM_RH TpmRHOwner;
extern const TPM_RH TpmRHNull;
extern const TPM_RH TpmRSPassword;

//
// TPM2.0 Handle List
//
typedef struct _TPML_HANDLE
{
    ULONG Count;
    TPM_HANDLE Handle[TPM_MAX_CAP_HANDLES];
} TPML_HANDLE, *PTPML_HANDLE;

//
// TPM2.0 Ticket for Hash Check
//
typedef struct _TPMT_TK_HASHCHECK
{
    TPM_ST Tag;
    TPMI_RH_HIERARCHY Hierarchy;
    TPM2B_DIGEST Digest;
} TPMT_TK_HASHCHECK;

//
// TPM2.0 Union of capability data returned by TPM2_CC_GetCapabilities
//
typedef union _TPMU_CAPABILITIES
{
    TPML_HANDLE Handles;
} TPMU_CAPABILITIES, *PTPMU_CAPABILITIES;

//
// TPM2.0 Payload for each capablity returned by TPM2_CC_GetCapabilities
//
typedef struct _TPMS_CAPABILITY_DATA
{
    TPM_CAP Capability;
    TPMU_CAPABILITIES Data;
} TPMS_CAPABILITY_DATA, *PTPMS_CAPABILITY_DATA;

//
// TPM2.0 Payload for TPM2_CC_ReadClock
//
typedef struct _TPMS_CLOCK_INFO
{
    ULONG64 Clock;
    ULONG ResetCount;
    ULONG RestartCount;
    TPMI_YES_NO Safe;
} TPMS_CLOCK_INFO;

typedef struct _TPMS_TIME_INFO
{
    ULONG64 Time;
    TPMS_CLOCK_INFO ClockInfo;
} TPMS_TIME_INFO;

//
// Definition of the header of any TPM2.0 Command
//
typedef struct _TPM_CMD_HEADER
{
    TPM_ST SessionTag;
    ULONG Size;
    TPM_CC CommandCode;
} TPM_CMD_HEADER, *PTPM_CMD_HEADER;

//
// Definition of the header of any TPM2.0 Response
//
typedef struct _TPM_REPLY_HEADER
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
typedef struct _TPMS_AUTH_RESPONSE_NO_NONCE
{
    USHORT NonceSize;
    TPMA_SESSION SessionAttributes;
    USHORT HmacSize;
} TPMS_AUTH_RESPONSE_NO_NONCE;

//
// Attached to any TPM2.0 Command with TPM_ST_SESSIONS and an authorization
// session with no nonce but with an optional HMAC/password present.
//
typedef struct _TPMS_AUTH_COMMAND_NO_NONCE
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
typedef struct _TPM_NV_READ_LOCK_CMD_HEADER
{
    TPM_CMD_HEADER Header;
    TPMI_RH_PROVISION AuthHandle;
    TPMI_RH_NV_INDEX NvIndex;
    TPMS_AUTH_COMMAND_NO_NONCE AuthSession;
} TPM_NV_READ_LOCK_CMD_HEADER, *PTPM_NV_READ_LOCK_CMD_HEADER;

typedef struct _TPM_NV_READ_LOCK_REPLY
{
    TPM_REPLY_HEADER Header;
    ULONG ParameterSize;
    TPMS_AUTH_RESPONSE_NO_NONCE AuthSession;
} TPM_NV_READ_LOCK_REPLY, *PTPM_NV_READ_LOCK_REPLY;

//
// NV_WriteLock
//
typedef struct _TPM_NV_WRITE_LOCK_CMD_HEADER
{
    TPM_CMD_HEADER Header;
    TPMI_RH_PROVISION AuthHandle;
    TPMI_RH_NV_INDEX NvIndex;
    TPMS_AUTH_COMMAND_NO_NONCE AuthSession;
} TPM_NV_WRITE_LOCK_CMD_HEADER, *PTPM_NV_WRITE_LOCK_CMD_HEADER;

typedef struct _TPM_NV_WRITE_LOCK_REPLY
{
    TPM_REPLY_HEADER Header;
    ULONG ParameterSize;
    TPMS_AUTH_RESPONSE_NO_NONCE AuthSession;
} TPM_NV_WRITE_LOCK_REPLY, *PTPM_NV_WRITE_LOCK_REPLY;

//
// NV_UndefineSpace
//
typedef struct _TPM_NV_UNDEFINE_SPACE_CMD_HEADER
{
    TPM_CMD_HEADER Header;
    TPMI_RH_PROVISION AuthHandle;
    TPMI_RH_NV_INDEX NvIndex;
    TPMS_AUTH_COMMAND_NO_NONCE AuthSession;
} TPM_NV_UNDEFINE_SPACE_CMD_HEADER, *PTPM_NV_UNDEFINE_SPACE_CMD_HEADER;

typedef struct _TPM_NV_UNDEFINE_SPACE_REPLY
{
    TPM_REPLY_HEADER Header;
    ULONG ParameterSize;
    TPMS_AUTH_RESPONSE_NO_NONCE AuthSession;
} TPM_NV_UNDEFINE_SPACE_REPLY, *PTPM_NV_UNDEFINE_SPACE_REPLY;

//
// NV_DefineSpace
//
typedef struct _TPM_NV_DEFINE_SPACE_CMD_HEADER
{
    TPM_CMD_HEADER Header;
    TPMI_RH_PROVISION AuthHandle;
    TPMS_AUTH_COMMAND_NO_NONCE AuthSession;
} TPM_NV_DEFINE_SPACE_CMD_HEADER, *PTPM_NV_DEFINE_SPACE_CMD_HEADER;

typedef struct _TPM_NV_DEFINE_SPACE_CMD_BODY
{
    USHORT AuthSize;
    BYTE Data[1];
} TPM_NV_DEFINE_SPACE_CMD_BODY, *PTPM_NV_DEFINE_SPACE_CMD_BODY;

typedef struct _TPM_NV_DEFINE_SPACE_CMD_FOOTER
{
    USHORT NvPublicSize;
    TPMI_RH_NV_INDEX NvIndex;
    TPM_ALG NameAlg;
    TPMA_NV Attributes;
    USHORT AuthPolicySize;
    USHORT DataSize;
} TPM_NV_DEFINE_SPACE_CMD_FOOTER, *PTPM_NV_DEFINE_SPACE_CMD_FOOTER;

typedef struct _TPM_NV_DEFINE_SPACE_REPLY
{
    TPM_REPLY_HEADER Header;
    ULONG ParameterSize;
    TPMS_AUTH_RESPONSE_NO_NONCE AuthSession;
} TPM_NV_DEFINE_SPACE_REPLY, *PTPM_NV_DEFINE_SPACE_REPLY;

//
// NV_ReadPublic
//
typedef struct _TPM_NV_READ_PUBLIC_CMD_HEADER
{
    TPM_CMD_HEADER Header;
    TPMI_RH_NV_INDEX NvIndex;
} TPM_NV_READ_PUBLIC_CMD_HEADER, *PTPM_NV_READ_PUBLIC_CMD_HEADER;

typedef struct _TPM_NV_READ_PUBLIC_REPLY
{
    TPM_REPLY_HEADER Header;
    TPM_NV_DEFINE_SPACE_CMD_FOOTER NvPublic;
    TPM2B_DIGEST Name;
} TPM_NV_READ_PUBLIC_REPLY, *PTPM_NV_READ_PUBLIC_REPLY;

//
// NV_Write
//
typedef struct _TPM_NV_WRITE_CMD_HEADER
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

typedef struct _TPM_NV_WRITE_CMD_BODY
{
    USHORT Size;
    BYTE Data[1];
} TPM_NV_WRITE_CMD_BODY, *PTPM_NV_WRITE_CMD_BODY;

typedef struct _TPM_NV_WRITE_CMD_FOOTER
{
    USHORT Offset;
} TPM_NV_WRITE_CMD_FOOTER, *PTPM_NV_WRITE_CMD_FOOTER;

typedef struct _TPM_NV_WRITE_REPLY
{
    TPM_REPLY_HEADER Header;
    ULONG ParameterSize;
    TPMS_AUTH_RESPONSE_NO_NONCE AuthSession;
} TPM_NV_WRITE_REPLY, *PTPM_NV_WRITE_REPLY;

//
// NV_Read
//
typedef struct _TPM_NV_READ_CMD_HEADER
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

typedef struct _TPM_NV_READ_CMD_FOOTER
{
    USHORT Size;
    USHORT Offset;
} TPM_NV_READ_CMD_FOOTER, *PTPM_NV_READ_CMD_FOOTER;

typedef struct _TPM_NV_READ_REPLY
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
typedef struct _TPM_GET_CAPABILITY_CMD_HEADER
{
    TPM_CMD_HEADER Header;
    TPM_CAP Capability;
    TPM_PT Property;
    ULONG PropertyCount;
} TPM_GET_CAPABILITY_CMD_HEADER, *PTPM_GET_CAPABILITY_CMD_HEADER;

typedef struct _TPM_GET_CAPABILITY_REPLY
{
    TPM_REPLY_HEADER Header;
    TPMI_YES_NO MoreData;
    TPMS_CAPABILITY_DATA Data;
} TPM_GET_CAPABILITY_REPLY, *PTPM_GET_CAPABILITY_REPLY;

// GetRandom
typedef struct _TPM_GET_RANDOM_CMD_HEADER
{
    TPM_CMD_HEADER Header;
    USHORT BytesRequested;
} TPM_GET_RANDOM_CMD_HEADER, * PTPM_GET_RANDOM_CMD_HEADER;

typedef struct _TPM_GET_RANDOM_REPLY
{
    TPM_REPLY_HEADER Header;
    TPM2B_DIGEST RandomBytes;
} TPM_GET_RANDOM_REPLY;

// ReadClock
typedef struct _TPM_READ_CLOCK_CMD_HEADER
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
typedef struct _TPM_HASH_CMD_HEADER
{
    TPM_CMD_HEADER Header;
    //
    // Variable Size Hash Input
    //
    // ....
    //
    // TPM_HASH_CMD_FOOTER;
} TPM_HASH_CMD_HEADER, *PTPM_HASH_CMD_HEADER;

typedef struct _TPM_HASH_CMD_BODY
{
    USHORT Size;
    BYTE Buffer[1];
} TPM_HASH_CMD_BODY, *PTPM_HASH_CMD_BODY;

typedef struct _TPM_HASH_CMD_FOOTER
{
    TPM_ALG AlgHash;
    TPMI_RH_HIERARCHY Hierarchy;
} TPM_HASH_CMD_FOOTER, *PTPM_HASH_CMD_FOOTER;

typedef struct _TPM_HASH_CMD_REPLY
{
    TPM_REPLY_HEADER Header;
    TPM2B_DIGEST OutHash;
    TPMT_TK_HASHCHECK Validation;
} TPM_HASH_CMD_REPLY;

#include <poppack.h>

#endif
