/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     jxy-s    2026
 *
 */

#include "onlnchk.h"
#include <kphuser.h>
#include <mapldr.h>
#include <bcrypt.h>

#include <winsqlite/winsqlite3.h>

typedef struct _SCAN_FILE_ID
{
    GUID VolumeGuid;
    FILE_ID_128 FileId;
} SCAN_FILE_ID, *PSCAN_FILE_ID;

typedef struct _SCAN_HASH
{
    SCAN_FILE_ID FileId;
    PPH_STRING FileName;
    PH_QUEUED_LOCK Lock;
    NTSTATUS Status;
    PPH_STRING Sha256;
} SCAN_HASH, *PSCAN_HASH;

typedef struct _SCAN_ITEM
{
    SLIST_ENTRY Entry;
    SCAN_TYPE Type;
    BOOLEAN Abort;
    ULONG Flags;
    PH_QUEUED_LOCK Lock;
    LARGE_INTEGER Expiry;
    PSCAN_HASH FileHash;
    PPH_STRING Result;
    PPH_STRING PreviousResult;
    PSCAN_COMPLETE_CALLBACK Callback;
    PVOID CallbackContext;
} SCAN_ITEM, *PSCAN_ITEM;

static PPH_OBJECT_TYPE ScanItemObjectType;
static PPH_OBJECT_TYPE ScanHashObjectType;
static PH_WORK_QUEUE ScanItemWorkQueue;
static PH_WORK_QUEUE ScanItemWorkPriorityQueue;
static SLIST_HEADER ScanItemQueueListHead;
static SLIST_HEADER ScanItemPriorityQueueListHead;
static PH_QUEUED_LOCK ScanHashHashtableLock = PH_QUEUED_LOCK_INIT;
static PPH_HASHTABLE ScanHashHashtable = NULL;
static PPH_STRING ScanVirusTotalPAT = NULL;
static PPH_STRING ScanHybridAnalysisPAT = NULL;
static PPH_STRING ScanScanningString = NULL;
static PPH_STRING ScanUnauthorizedString = NULL;
static PPH_STRING ScanCleanString = NULL;
static PPH_STRING ScanRateLimitedString = NULL;
static PPH_STRING ScanUnknownString = NULL;
static PPH_STRING ScanFileTooLarge = NULL;
static const LONG64 ScanOKExpMin = (10LL * 24 * 60 * 60 * 10000000); // 10 days
static const LONG64 ScanOKExpMax = (14LL * 24 * 60 * 60 * 10000000); // 14 days
static const LONG64 ScanRateLmtExpMin = (15LL * 60 * 10000000); // 15 minutes
static const LONG64 ScanRateLmtExpMax = (60LL * 60 * 10000000); // 1 hour
static const LONG64 ScanNoResponseExpMin = (1LL * 24 * 60 * 60 * 10000000); // 1 day
static const LONG64 ScanNoResponseExpMax = (2LL * 24 * 60 * 60 * 10000000); // 2 days

static typeof(&BCryptOpenAlgorithmProvider) BCryptOpenAlgorithmProvider_I = NULL;
static typeof(&BCryptCloseAlgorithmProvider) BCryptCloseAlgorithmProvider_I = NULL;
static typeof(&BCryptDestroyHash) BCryptDestroyHash_I = NULL;
static typeof(&BCryptCreateMultiHash) BCryptCreateMultiHash_I = NULL;
static typeof(&BCryptProcessMultiOperations) BCryptProcessMultiOperations_I = NULL;

static sqlite3* ScanDB = NULL;
static PH_QUEUED_LOCK ScanDBLock = PH_QUEUED_LOCK_INIT;
static sqlite3_stmt* ScanDBInsertVirusTotal = NULL;
static sqlite3_stmt* ScanDBQueryVirusTotal = NULL;
static sqlite3_stmt* ScanDBInsertHybridAnalysis = NULL;
static sqlite3_stmt* ScanDBQueryHybridAnalysis = NULL;
static typeof(&sqlite3_open_v2) sqlite3_open_v2_I = NULL;
static typeof(&sqlite3_close_v2) sqlite3_close_v2_I = NULL;
static typeof(&sqlite3_exec) sqlite3_exec_I = NULL;
static typeof(&sqlite3_prepare_v2) sqlite3_prepare_v2_I = NULL;
static typeof(&sqlite3_finalize) sqlite3_finalize_I = NULL;
static typeof(&sqlite3_bind_int64) sqlite3_bind_int64_I = NULL;
static typeof(&sqlite3_bind_text16) sqlite3_bind_text16_I = NULL;
static typeof(&sqlite3_step) sqlite3_step_I = NULL;
static typeof(&sqlite3_reset) sqlite3_reset_I = NULL;
static typeof(&sqlite3_column_int64) sqlite3_column_int64_I = NULL;
static typeof(&sqlite3_column_text16) sqlite3_column_text16_I = NULL;

typedef struct _SQL_STMT
{
    sqlite3_stmt** Smt;
    const char* Sql;
} SQL_STMT, *PSQL_STMT;

const char* ScanDBSQL =
"PRAGMA encoding = \"UTF-16le\";"
"CREATE TABLE IF NOT EXISTS virus_total("
"    sha256 TEXT PRIMARY KEY,"
"    http_status INTEGER,"
"    expiry INTEGER,"
"    expiry_iso TEXT,"
"    malicious INTEGER,"
"    undetected INTEGER"
");"
"CREATE TABLE IF NOT EXISTS hybrid_analysis("
"    sha256 TEXT PRIMARY KEY,"
"    http_status INTEGER,"
"    expiry INTEGER,"
"    expiry_iso TEXT,"
"    multiscan_result INTEGER,"
"    vx_family INTEGER"
");"
;

const char* ScanDBVersion[] =
{
    // version 1
    "ALTER TABLE hybrid_analysis ADD COLUMN threat_score INTEGER; "
    "ALTER TABLE hybrid_analysis ADD COLUMN verdict TEXT;",
};

static const SQL_STMT ScanDBSQLSmts[] =
{
    {
        &ScanDBInsertVirusTotal,
        "INSERT OR REPLACE INTO virus_total("
        "    sha256,"
        "    http_status,"
        "    expiry,"
        "    expiry_iso,"
        "    malicious,"
        "    undetected"
        ") "
        "VALUES(?, ?, ?, ?, ?, ?);"
    },
    {
        &ScanDBQueryVirusTotal,
        "SELECT http_status, expiry, malicious, undetected "
        "FROM virus_total "
        "WHERE sha256 = ?;"
    },
    {
        &ScanDBInsertHybridAnalysis,
        "INSERT OR REPLACE INTO hybrid_analysis("
        "    sha256,"
        "    http_status,"
        "    expiry,"
        "    expiry_iso,"
        "    multiscan_result,"
        "    vx_family,"
        "    threat_score,"
        "    verdict"
        ") "
        "VALUES(?, ?, ?, ?, ?, ?, ?, ?);"
    },
    {
        &ScanDBQueryHybridAnalysis,
        "SELECT http_status, expiry, multiscan_result, vx_family "
        "FROM hybrid_analysis "
        "WHERE sha256 = ?;"
    }
};

LONG64 MakeExpiry(
    _In_ PLARGE_INTEGER SystemTime,
    _In_ LONG64 Min,
    _In_ LONG64 Max
    )
{
    ULONG64 rand;
    LONG64 expiry;

    NT_ASSERT(Min < Max);

    rand = PhGenerateRandomNumber64();
    expiry = SystemTime->QuadPart + Min + (rand % (Max - Min));

    return expiry;
}

VOID SetScanResult(
    _In_ PSCAN_ITEM Item,
    _In_ PPH_STRING Result
    )
{
    PhAcquireQueuedLockExclusive(&Item->Lock);
    PhMoveReference(&Item->Result, Result);
    PhReleaseQueuedLockExclusive(&Item->Lock);
}

PPH_STRING ReferenceScanResult(
    _In_ PSCAN_CONTEXT Context,
    _In_ SCAN_TYPE Type
    )
{
    PPH_STRING result;

    if (!Context->ScanItems[Type])
        return PhReferenceEmptyString();

    PhAcquireQueuedLockShared(&Context->ScanItems[Type]->Lock);
    result = PhReferenceObject(Context->ScanItems[Type]->Result);
    PhReleaseQueuedLockShared(&Context->ScanItems[Type]->Lock);

    return result;
}

BOOLEAN QueryDBVirusTotal(
    _In_ PPH_STRING Hash,
    _Out_ PULONG HttpStatus,
    _Out_ PLARGE_INTEGER Expiry,
    _Out_ PULONG64 Malicious,
    _Out_ PULONG64 Undetected
    )
{
    BOOLEAN result = FALSE;

    *HttpStatus = 0;
    Expiry->QuadPart = 0;
    *Malicious = 0;
    *Undetected = 0;

    PhAcquireQueuedLockExclusive(&ScanDBLock);
    if (ScanDBQueryVirusTotal)
    {
        sqlite3_bind_text16_I(ScanDBQueryVirusTotal, 1, Hash->Buffer, (int)Hash->Length, SQLITE_TRANSIENT);
        if (sqlite3_step_I(ScanDBQueryVirusTotal) == SQLITE_ROW)
        {
            *HttpStatus = (ULONG)sqlite3_column_int64_I(ScanDBQueryVirusTotal, 0);
            Expiry->QuadPart = sqlite3_column_int64_I(ScanDBQueryVirusTotal, 1);
            *Malicious = (ULONG64)sqlite3_column_int64_I(ScanDBQueryVirusTotal, 2);
            *Undetected = (ULONG64)sqlite3_column_int64_I(ScanDBQueryVirusTotal, 3);
            result = TRUE;
        }
        sqlite3_step_I(ScanDBQueryVirusTotal);
        sqlite3_reset_I(ScanDBQueryVirusTotal);
    }
    PhReleaseQueuedLockExclusive(&ScanDBLock);

    return result;
}

VOID UpdateDBVirusTotal(
    _In_ PPH_STRING Hash,
    _In_ ULONG HttpStatus,
    _In_ PLARGE_INTEGER Expiry,
    _In_ ULONG64 Malicious,
    _In_ ULONG64 Undetected
    )
{
    SYSTEMTIME systemTime;
    PPH_STRING iso;

    PhLargeIntegerToLocalSystemTime(&systemTime, Expiry);
    iso = PhFormatLocalSystemTimeISO(&systemTime);

    PhAcquireQueuedLockExclusive(&ScanDBLock);
    if (ScanDBInsertVirusTotal)
    {
        sqlite3_bind_text16_I(ScanDBInsertVirusTotal, 1, Hash->Buffer, (int)Hash->Length, SQLITE_TRANSIENT);
        sqlite3_bind_int64_I(ScanDBInsertVirusTotal, 2, HttpStatus);
        sqlite3_bind_int64_I(ScanDBInsertVirusTotal, 3, Expiry->QuadPart);
        sqlite3_bind_text16_I(ScanDBInsertVirusTotal, 4, iso->Buffer, (int)iso->Length, SQLITE_TRANSIENT);
        sqlite3_bind_int64_I(ScanDBInsertVirusTotal, 5, Malicious);
        sqlite3_bind_int64_I(ScanDBInsertVirusTotal, 6, Undetected);
        sqlite3_step_I(ScanDBInsertVirusTotal);
        sqlite3_reset_I(ScanDBInsertVirusTotal);
    }
    PhReleaseQueuedLockExclusive(&ScanDBLock);

    PhDereferenceObject(iso);
}

VOID ProcessVirusTotal(
    _In_ PSCAN_ITEM Item
    )
{
    LARGE_INTEGER systemTime;
    ULONG httpStatus;
    LARGE_INTEGER expiry;
    ULONG64 malicious;
    ULONG64 undetected;
    PVIRUSTOTAL_FILE_REPORT report = NULL;

    PhQuerySystemTime(&systemTime);

    if (!FlagOn(Item->Flags, SCAN_FLAG_RESCAN))
    {
        if (QueryDBVirusTotal(Item->FileHash->Sha256, &httpStatus, &expiry, &malicious, &undetected) &&
            expiry.QuadPart > systemTime.QuadPart)
        {
            if (httpStatus == 200) // OK
            {
                WriteNoFence64(&Item->Expiry.QuadPart, expiry.QuadPart);
                SetScanResult(Item, PhFormatString(L"%llu/%llu", malicious, undetected));
            }
            else if (httpStatus == 429) // Too many requests
            {
                WriteNoFence64(&Item->Expiry.QuadPart, expiry.QuadPart);
                SetScanResult(Item, PhReferenceObject(ScanRateLimitedString));
            }
            else
            {
                WriteNoFence64(&Item->Expiry.QuadPart, expiry.QuadPart);
                SetScanResult(Item, PhReferenceObject(ScanUnknownString));
            }

            goto CleanupExit;
        }
    }

    if (FlagOn(Item->Flags, SCAN_FLAG_LOCAL_ONLY))
        goto CleanupExit;

    if (!NT_SUCCESS(VirusTotalRequestFileReport(Item->FileHash->Sha256, ScanVirusTotalPAT, &report)))
        goto CleanupExit;

    if (report->HttpStatus == 200) // OK
    {
        httpStatus = report->HttpStatus;
        malicious = report->Malicious;
        undetected = report->Undetected;
        expiry.QuadPart = MakeExpiry(&systemTime, ScanOKExpMin, ScanOKExpMax);

        WriteNoFence64(&Item->Expiry.QuadPart, expiry.QuadPart);
        SetScanResult(Item, PhFormatString(L"%llu/%llu", malicious, undetected));

        UpdateDBVirusTotal(
            Item->FileHash->Sha256,
            httpStatus,
            &expiry,
            malicious,
            undetected
            );
    }
    else if (report->HttpStatus == 429) // Too many requests
    {
        httpStatus = report->HttpStatus;
        malicious = 0;
        undetected = 0;
        expiry.QuadPart = MakeExpiry(&systemTime, ScanRateLmtExpMin, ScanRateLmtExpMax);

        WriteNoFence64(&Item->Expiry.QuadPart, expiry.QuadPart);
        SetScanResult(Item, PhReferenceObject(ScanRateLimitedString));

        UpdateDBVirusTotal(Item->FileHash->Sha256, httpStatus, &expiry, malicious, undetected);
    }
    else if (report->HttpStatus == 401 || report->HttpStatus == 403) // Unauthorized/Forbidden
    {
        SetScanResult(Item, PhReferenceObject(ScanUnauthorizedString));
    }
    else
    {
        httpStatus = report->HttpStatus;
        malicious = 0;
        undetected = 0;
        expiry.QuadPart = MakeExpiry(&systemTime, ScanNoResponseExpMin, ScanNoResponseExpMax);

        WriteNoFence64(&Item->Expiry.QuadPart, expiry.QuadPart);
        SetScanResult(Item, PhReferenceObject(ScanUnknownString));

        UpdateDBVirusTotal(
            Item->FileHash->Sha256,
            httpStatus,
            &expiry,
            malicious,
            undetected
            );
    }

CleanupExit:

    if (report)
        VirusTotalFreeFileReport(report);
}

BOOLEAN QueryDBHybridAnalysis(
    _In_ PPH_STRING Hash,
    _Out_ PULONG HttpStatus,
    _Out_ PLARGE_INTEGER Expiry,
    _Out_ PULONG64 MultiscanResult,
    _Out_ PPH_STRING* VxFamily
    )
{
    BOOLEAN result = FALSE;

    *HttpStatus = 0;
    Expiry->QuadPart = 0;
    *MultiscanResult = 0;
    *VxFamily = NULL;

    PhAcquireQueuedLockExclusive(&ScanDBLock);
    if (ScanDBQueryHybridAnalysis)
    {
        sqlite3_bind_text16_I(ScanDBQueryHybridAnalysis, 1, Hash->Buffer, (int)Hash->Length, SQLITE_TRANSIENT);
        if (sqlite3_step_I(ScanDBQueryHybridAnalysis) == SQLITE_ROW)
        {
            *HttpStatus = (ULONG)sqlite3_column_int64_I(ScanDBQueryHybridAnalysis, 0);
            Expiry->QuadPart = sqlite3_column_int64_I(ScanDBQueryHybridAnalysis, 1);
            *MultiscanResult = (ULONG64)sqlite3_column_int64_I(ScanDBQueryHybridAnalysis, 2);
            *VxFamily = PhCreateString(sqlite3_column_text16_I(ScanDBQueryHybridAnalysis, 3));
            result = TRUE;
        }
        sqlite3_step_I(ScanDBQueryHybridAnalysis);
        sqlite3_reset_I(ScanDBQueryHybridAnalysis);
    }
    PhReleaseQueuedLockExclusive(&ScanDBLock);

    return result;
}

VOID UpdateDBHybridAnalysis(
    _In_ PPH_STRING Hash,
    _In_ ULONG HttpStatus,
    _In_ PLARGE_INTEGER Expiry,
    _In_ ULONG64 MultiscanResult,
    _In_ PPH_STRING VxFamily,
    _In_ ULONG64 ThreatScore,
    _In_ PPH_STRING Verdict
    )
{
    SYSTEMTIME systemTime;
    PPH_STRING iso;

    PhLargeIntegerToLocalSystemTime(&systemTime, Expiry);
    iso = PhFormatLocalSystemTimeISO(&systemTime);

    PhAcquireQueuedLockExclusive(&ScanDBLock);
    if (ScanDBInsertHybridAnalysis)
    {
        sqlite3_bind_text16_I(ScanDBInsertHybridAnalysis, 1, Hash->Buffer, (int)Hash->Length, SQLITE_TRANSIENT);
        sqlite3_bind_int64_I(ScanDBInsertHybridAnalysis, 2, HttpStatus);
        sqlite3_bind_int64_I(ScanDBInsertHybridAnalysis, 3, Expiry->QuadPart);
        sqlite3_bind_text16_I(ScanDBInsertHybridAnalysis, 4, iso->Buffer, (int)iso->Length, SQLITE_TRANSIENT);
        sqlite3_bind_int64_I(ScanDBInsertHybridAnalysis, 5, MultiscanResult);
        sqlite3_bind_text16_I(ScanDBInsertHybridAnalysis, 6, VxFamily->Buffer, (int)VxFamily->Length, SQLITE_TRANSIENT);
        sqlite3_bind_int64_I(ScanDBInsertHybridAnalysis, 7, ThreatScore);
        sqlite3_bind_text16_I(ScanDBInsertHybridAnalysis, 8, Verdict->Buffer, (int)Verdict->Length, SQLITE_TRANSIENT);
        sqlite3_step_I(ScanDBInsertHybridAnalysis);
        sqlite3_reset_I(ScanDBInsertHybridAnalysis);
    }
    PhReleaseQueuedLockExclusive(&ScanDBLock);

    PhDereferenceObject(iso);
}

VOID ProcessHybridAnalysis(
    _In_ PSCAN_ITEM Item
    )
{
    LARGE_INTEGER systemTime;
    ULONG httpStatus;
    LARGE_INTEGER expiry;
    ULONG64 multiscanResult;
    PPH_STRING vxFamily = NULL;
    ULONG64 threatScore;
    PPH_STRING verdict = NULL;
    PHYBRIDANALYSIS_FILE_REPORT report = NULL;

    PhQuerySystemTime(&systemTime);

    if (!FlagOn(Item->Flags, SCAN_FLAG_RESCAN))
    {
        if (QueryDBHybridAnalysis(Item->FileHash->Sha256, &httpStatus, &expiry, &multiscanResult, &vxFamily) &&
            expiry.QuadPart > systemTime.QuadPart)
        {
            if (httpStatus == 200) // OK
            {
                WriteNoFence64(&Item->Expiry.QuadPart, expiry.QuadPart);
                if (vxFamily->Length == 0)
                    SetScanResult(Item, PhReferenceObject(ScanCleanString));
                else
                    SetScanResult(Item, PhFormatString(L"%llu%% %ls", multiscanResult, PhGetString(vxFamily)));
            }
            else if (httpStatus == 429) // Too many requests
            {
                WriteNoFence64(&Item->Expiry.QuadPart, expiry.QuadPart);
                SetScanResult(Item, PhReferenceObject(ScanRateLimitedString));
            }
            else
            {
                WriteNoFence64(&Item->Expiry.QuadPart, expiry.QuadPart);
                SetScanResult(Item, PhReferenceObject(ScanUnknownString));
            }

            goto CleanupExit;
        }
    }

    if (FlagOn(Item->Flags, SCAN_FLAG_LOCAL_ONLY))
        goto CleanupExit;

    PhClearReference(&vxFamily);

    if (!NT_SUCCESS(HybridAnalysisRequestFileReport(Item->FileHash->Sha256, ScanHybridAnalysisPAT, &report)))
        goto CleanupExit;

    if (report->HttpStatus == 200) // OK
    {
        httpStatus = report->HttpStatus;
        multiscanResult = report->MultiscanResult;
        vxFamily = PhReferenceObject(report->VxFamily);
        threatScore = report->ThreatScore;
        verdict = PhReferenceObject(report->Verdict);
        expiry.QuadPart = MakeExpiry(&systemTime, ScanOKExpMin, ScanOKExpMax);

        WriteNoFence64(&Item->Expiry.QuadPart, expiry.QuadPart);
        if (vxFamily->Length == 0)
            SetScanResult(Item, PhReferenceObject(ScanCleanString));
        else
            SetScanResult(Item, PhFormatString(L"%llu%% %ls", multiscanResult, PhGetString(vxFamily)));

        UpdateDBHybridAnalysis(
            Item->FileHash->Sha256,
            httpStatus,
            &expiry,
            multiscanResult,
            vxFamily,
            threatScore,
            verdict
            );
    }
    else if (report->HttpStatus == 429) // Too many requests
    {
        httpStatus = report->HttpStatus;
        multiscanResult = 0;
        vxFamily = PhReferenceEmptyString();
        threatScore = 0;
        verdict = PhReferenceEmptyString();
        expiry.QuadPart = MakeExpiry(&systemTime, ScanRateLmtExpMin, ScanRateLmtExpMax);

        WriteNoFence64(&Item->Expiry.QuadPart, expiry.QuadPart);
        SetScanResult(Item, PhReferenceObject(ScanRateLimitedString));

        UpdateDBHybridAnalysis(
            Item->FileHash->Sha256,
            httpStatus,
            &expiry,
            multiscanResult,
            vxFamily,
            threatScore,
            verdict
            );
    }
    else if (report->HttpStatus == 401 || report->HttpStatus == 403) // Unauthorized/Forbidden
    {
        SetScanResult(Item, PhReferenceObject(ScanUnauthorizedString));
    }
    else
    {
        httpStatus = report->HttpStatus;
        multiscanResult = 0;
        vxFamily = PhReferenceEmptyString();
        threatScore = 0;
        verdict = PhReferenceEmptyString();
        expiry.QuadPart = MakeExpiry(&systemTime, ScanNoResponseExpMin, ScanNoResponseExpMax);

        WriteNoFence64(&Item->Expiry.QuadPart, expiry.QuadPart);
        SetScanResult(Item, PhReferenceObject(ScanUnknownString));

        UpdateDBHybridAnalysis(
            Item->FileHash->Sha256,
            httpStatus,
            &expiry,
            multiscanResult,
            vxFamily,
            threatScore,
            verdict
            );
    }

CleanupExit:

    PhClearReference(&vxFamily);
    PhClearReference(&verdict);
    if (report)
        HybridAnalysisFreeFileReport(report);
}

VOID ProcessScanItems(
    _In_count_(Count) PSCAN_ITEM* Items,
    _In_ ULONG Count
    )
{
    NTSTATUS status;

    for (ULONG i = 0; i < Count; i++)
    {
        PSCAN_ITEM item = Items[i];

        if (ReadAcquire8(&item->Abort))
            continue;

        PhAcquireQueuedLockShared(&item->FileHash->Lock);
        status = item->FileHash->Status;
        PhReleaseQueuedLockShared(&item->FileHash->Lock);

        if (NT_SUCCESS(status))
        {
            if (item->Type == SCAN_TYPE_VIRUSTOTAL)
                ProcessVirusTotal(item);
            else if (item->Type == SCAN_TYPE_HYBRIDANALYSIS)
                ProcessHybridAnalysis(item);
        }
        else if (status == STATUS_FILE_TOO_LARGE)
        {
            SetScanResult(item, PhReferenceObject(ScanFileTooLarge));
        }

        if (item->Result == ScanScanningString)
        {
            if (item->PreviousResult)
                SetScanResult(item, PhReferenceObject(item->PreviousResult));
            else
                SetScanResult(item, PhReferenceEmptyString());
        }

        if (item->Callback)
        {
            item->Callback(
                item->Type,
                item->FileHash->FileName,
                item->FileHash,
                item->Result,
                item->CallbackContext
                );
        }
    }
}

int _cdecl CompareScanHashPointers(
    _In_opt_ void* Context,
    _In_ const void* Lhs,
    _In_ const void* Rhs
    )
{
    PSCAN_HASH hashLhs = *(PSCAN_HASH*)Lhs;
    PSCAN_HASH hashRhs = *(PSCAN_HASH*)Rhs;

    UNREFERENCED_PARAMETER(Context);

    return uintptrcmp((ULONG_PTR)hashLhs, (ULONG_PTR)hashRhs);
}

typedef enum _SCAN_READ_STATE
{
    ScanReadIdle,
    ScanReadPending,
    ScanReadComplete,
} SCAN_READ_STATE, *PSCAN_READ_STATE;

#define SCAN_READ_BUFFER_SIZE (16 * 1024)

typedef struct _PROCESS_SCAN_HASH_CONTEXT
{
    PSCAN_HASH ScanHash;
    HANDLE FileHandle;
    ULONG ActiveIndex;
    LONG ReadState;
    LARGE_INTEGER ReadOffset;
    IO_STATUS_BLOCK IoStatusBlock;
    BOOLEAN HashFinished;
    BYTE HashBuffer[256 / 8];
    PBYTE ReadBuffer;
} PROCESS_SCAN_HASH_CONTEXT, *PPROCESS_SCAN_HASH_CONTEXT;

VOID CreateProcessScanHashContexts(
    _In_count_(Count) PSCAN_HASH* Hashes,
    _In_ ULONG Count,
    _Outptr_result_buffer_all_(*ContextsCount) PPROCESS_SCAN_HASH_CONTEXT* Contexts,
    _Out_ PULONG ContextsCount
    )
{
    PPROCESS_SCAN_HASH_CONTEXT contexts = NULL;
    ULONG contextsCount = 0;
    PSCAN_HASH lastScanHash = NULL;

    NT_ASSERT(Count > 0);

    //
    // N.B. Scan hash objects are shared between items. Sort the array of
    // pointers to ensure that same items are adjacent, which allows for
    // deduplication based on the pointer. Then return a deduplicated array of
    // contexts.
    //
    qsort_s(Hashes, Count, sizeof(PSCAN_HASH), CompareScanHashPointers, NULL);

    contexts = PhAllocateZero(Count * sizeof(PROCESS_SCAN_HASH_CONTEXT));

    for (ULONG i = 0; i < Count; i++)
    {
        contexts[i].ActiveIndex = ULONG_MAX;

        if (Hashes[i] != lastScanHash)
        {
            contexts[contextsCount].ScanHash = Hashes[i];
            lastScanHash = Hashes[i];
            contextsCount++;
        }
    }

    *Contexts = contexts;
    *ContextsCount = contextsCount;
}

_Function_class_(IO_APC_ROUTINE)
static VOID NTAPI ProcessScanHashApcRoutine(
    _In_ PVOID ApcContext,
    _In_ PIO_STATUS_BLOCK IoStatusBlock,
    _In_ ULONG Reserved
    )
{
    PPROCESS_SCAN_HASH_CONTEXT context = (PPROCESS_SCAN_HASH_CONTEXT)ApcContext;

    WriteRelease(&context->ReadState, ScanReadComplete);
}

VOID ProcessScanHashes(
    _In_count_(Count) PSCAN_HASH* Hashes,
    _In_ ULONG Count
    )
{
    NTSTATUS status;
    PPROCESS_SCAN_HASH_CONTEXT contexts;
    ULONG count;
    BCRYPT_ALG_HANDLE algorithmHandle;
    ULONG activeCount;
    BCRYPT_HASH_HANDLE multiHashHandle = NULL;
    BCRYPT_MULTI_HASH_OPERATION* hashOps = NULL;

    CreateProcessScanHashContexts(Hashes, Count, &contexts, &count);

    if (!NT_SUCCESS(status = BCryptOpenAlgorithmProvider_I(
        &algorithmHandle,
        BCRYPT_SHA256_ALGORITHM,
        NULL,
        BCRYPT_MULTI_FLAG
        )))
    {
        algorithmHandle = NULL;
    }

    activeCount = 0;
    for (ULONG i = 0; i < count; i++)
    {
        PPROCESS_SCAN_HASH_CONTEXT context = &contexts[i];
        HANDLE fileHandle;
        LARGE_INTEGER fileSize;

        //
        // N.B. Hashes are intentionally locked sequentially to avoid duplicate
        // work. They are released when we're finished. This is to avoid doing
        // the same expensive hash calculation multiple times for the same file.
        //
        PhAcquireQueuedLockExclusive(&context->ScanHash->Lock);
        if (context->ScanHash->Status != STATUS_PENDING)
        {
            PhReleaseQueuedLockExclusive(&context->ScanHash->Lock);
            continue;
        }

        if (!NT_SUCCESS(status = PhCreateFileWin32(
            &fileHandle,
            PhGetString(context->ScanHash->FileName),
            FILE_READ_DATA | SYNCHRONIZE,
            FILE_ATTRIBUTE_NORMAL,
            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
            FILE_OPEN,
            FILE_NON_DIRECTORY_FILE | FILE_SEQUENTIAL_ONLY
            )))
        {
            context->ScanHash->Status = status;
            PhReleaseQueuedLockExclusive(&context->ScanHash->Lock);
            continue;
        }

        if (!NT_SUCCESS(status = PhGetFileSize(fileHandle, &fileSize)))
        {
            context->ScanHash->Status = status;
            PhReleaseQueuedLockExclusive(&context->ScanHash->Lock);
            NtClose(fileHandle);
            continue;
        }

        if (fileSize.QuadPart > ScanMaxFileSize)
        {
            context->ScanHash->Status = STATUS_FILE_TOO_LARGE;
            PhReleaseQueuedLockExclusive(&context->ScanHash->Lock);
            NtClose(fileHandle);
            continue;
        }

        //
        // Fast-path through the driver. The kernel driver caches file hashes in
        // kernel purge extended attributes. This means that the hash of the
        // file does not need recalculated every time. A cache-hit here
        // significantly reduces the time taken and required I/O.
        //
        if (KsiLevel() == KphLevelMax)
        {
            KPH_HASH_INFORMATION hashInfo;

            hashInfo.Algorithm = KphHashAlgorithmSha256;

            if (NT_SUCCESS(status = KsiQueryHashInformationFile(fileHandle, &hashInfo, sizeof(hashInfo))))
            {
                context->ScanHash->Sha256 = PhBufferToHexString(hashInfo.Hash, hashInfo.Length);;
                context->ScanHash->Status = STATUS_SUCCESS;
                PhReleaseQueuedLockExclusive(&context->ScanHash->Lock);
                NtClose(fileHandle);
                continue;
            }
        }

        if (!algorithmHandle)
        {
            context->ScanHash->Status = STATUS_UNSUCCESSFUL;
            PhReleaseQueuedLockExclusive(&context->ScanHash->Lock);
            NtClose(fileHandle);
            continue;
        }

        //
        // Keep the hash item locked while we go batch hash.
        //
        context->FileHandle = fileHandle;
        context->ActiveIndex = activeCount;
        context->ReadBuffer = PhAllocate(SCAN_READ_BUFFER_SIZE);
        activeCount++;
    }

    if (activeCount == 0)
        goto CleanupExit;

    if (!NT_SUCCESS(status = BCryptCreateMultiHash_I(
        algorithmHandle,
        &multiHashHandle,
        activeCount,
        NULL,
        0,
        NULL,
        0,
        0
        )))
    {
        goto CleanupExit;
    }

    hashOps = PhAllocate(activeCount * sizeof(BCRYPT_MULTI_HASH_OPERATION));

    for (;;)
    {
        ULONG countOps = 0;
        ULONG finishedCount = 0;
        ULONG pendingCount = 0;

        for (ULONG i = 0; i < count; i++)
        {
            PPROCESS_SCAN_HASH_CONTEXT context = &contexts[i];

            if (context->ActiveIndex == ULONG_MAX || context->HashFinished)
                continue;

            NT_ASSERT(context->FileHandle);

            if (InterlockedCompareExchange(
                &context->ReadState,
                ScanReadPending,
                ScanReadIdle
                ) == ScanReadIdle)
            {
                status = NtReadFile(
                    context->FileHandle,
                    NULL,
                    ProcessScanHashApcRoutine,
                    context,
                    &context->IoStatusBlock,
                    context->ReadBuffer,
                    SCAN_READ_BUFFER_SIZE,
                    &context->ReadOffset,
                    NULL
                    );
                if (!NT_VERIFY(status == STATUS_PENDING))
                {
                    context->IoStatusBlock.Status = status;
                    WriteNoFence(&context->ReadState, ScanReadComplete);
                }
            }
        }

        for (ULONG i = 0; i < count; i++)
        {
            PPROCESS_SCAN_HASH_CONTEXT context = &contexts[i];
            SCAN_READ_STATE readState;

            if (context->ActiveIndex == ULONG_MAX)
                continue;

            NT_ASSERT(context->FileHandle);

            if (context->HashFinished)
            {
                finishedCount++;
                continue;
            }

            readState = ReadAcquire(&context->ReadState);

            if (readState == ScanReadPending)
            {
                pendingCount++;
                continue;
            }

            NT_ASSERT(readState == ScanReadComplete);

            if (!NT_SUCCESS(context->IoStatusBlock.Status) || context->IoStatusBlock.Information == 0)
            {
                context->HashFinished = TRUE;
                finishedCount++;

                hashOps[countOps].iHash = context->ActiveIndex;
                hashOps[countOps].hashOperation = BCRYPT_HASH_OPERATION_FINISH_HASH;
                hashOps[countOps].pbBuffer = context->HashBuffer;
                hashOps[countOps].cbBuffer = sizeof(context->HashBuffer);
                countOps++;
            }
            else
            {
                WriteNoFence(&context->ReadState, ScanReadIdle);
                context->ReadOffset.QuadPart += context->IoStatusBlock.Information;

                hashOps[countOps].iHash = context->ActiveIndex;
                hashOps[countOps].hashOperation = BCRYPT_HASH_OPERATION_HASH_DATA;
                hashOps[countOps].pbBuffer = context->ReadBuffer;
                hashOps[countOps].cbBuffer = (ULONG)context->IoStatusBlock.Information;
                countOps++;
            }
        }

        if (countOps > 0)
        {
            NT_VERIFY(NT_SUCCESS(BCryptProcessMultiOperations_I(
                multiHashHandle,
                BCRYPT_OPERATION_TYPE_HASH,
                hashOps,
                countOps * sizeof(BCRYPT_MULTI_HASH_OPERATION),
                0
                )));
        }

        if (finishedCount >= activeCount)
            break;

        if (pendingCount > 0)
        {
            LARGE_INTEGER timeout;

            //
            // N.B. Alertable wait to drain I/O completion APCs.
            //
            NtDelayExecution(TRUE, PhTimeoutFromMilliseconds(&timeout, 100));
        }
    }

CleanupExit:

    for (ULONG i = count; i > 0; i--)
    {
        PPROCESS_SCAN_HASH_CONTEXT context = &contexts[i - 1];

        if (context->ReadBuffer)
        {
#pragma prefast(suppress: 6001) // ReadBuffer is initialized
            PhFree(context->ReadBuffer);
        }

        if (!context->FileHandle)
            continue;

        if (context->HashFinished)
        {
            context->ScanHash->Sha256 = PhBufferToHexString(context->HashBuffer, sizeof(context->HashBuffer));
            context->ScanHash->Status = STATUS_SUCCESS;
        }
        else if (context->ScanHash->Status == STATUS_PENDING)
        {
            context->ScanHash->Status = STATUS_UNSUCCESSFUL;
        }

#pragma prefast(suppress: 26110) // Lock handling is correct
        PhReleaseQueuedLockExclusive(&context->ScanHash->Lock);
#pragma prefast(suppress: 6001) // FileHandle is initialized
        NtClose(context->FileHandle);
    }

    if (hashOps)
        PhFree(hashOps);

    if (multiHashHandle)
        BCryptDestroyHash_I(multiHashHandle);

    if (algorithmHandle)
        BCryptCloseAlgorithmProvider_I(algorithmHandle, 0);

    PhFree(contexts);
}

VOID ProcessScanItemsList(
    _In_ PSLIST_ENTRY First
    )
{
    ULONG count;
    PSCAN_ITEM* scanItems;
    PSCAN_HASH* scanHashes;

    count = 0;
    for (PSLIST_ENTRY entry = First; entry; entry = entry->Next)
        count++;

    scanItems = PhAllocate(count * sizeof(PSCAN_ITEM));
    scanHashes = PhAllocate(count * sizeof(PSCAN_HASH));
    count = 0;
    for (PSLIST_ENTRY entry = First; entry; entry = entry->Next)
    {
        scanItems[count] = CONTAINING_RECORD(entry, SCAN_ITEM, Entry);
        scanHashes[count] = scanItems[count]->FileHash;
        count++;
    }

    ProcessScanHashes(scanHashes, count);
    ProcessScanItems(scanItems, count);

    for (ULONG i = 0 ; i < count; i++)
        PhDereferenceObject(scanItems[i]);

    PhFree(scanHashes);
    PhFree(scanItems);
}

_Function_class_(USER_THREAD_START_ROUTINE)
NTSTATUS ScanItemWorkerRoutine(
    _In_ PVOID Parameter
    )
{
    PSLIST_ENTRY first;
    KPRIORITY threadPriority;
    IO_PRIORITY_HINT ioPriority;
    ULONG pagePriority = ULONG_MAX;

    PhGetThreadBasePriority(NtCurrentThread(), &threadPriority);
    PhGetThreadIoPriority(NtCurrentThread(), &ioPriority);
    PhGetThreadPagePriority(NtCurrentThread(), &pagePriority);

    PhSetThreadBasePriority(NtCurrentThread(), THREAD_PRIORITY_BELOW_NORMAL);
    PhSetThreadIoPriority(NtCurrentThread(), IoPriorityLow);
    PhSetThreadPagePriority(NtCurrentThread(), MEMORY_PRIORITY_LOW);

    first = RtlInterlockedFlushSList(&ScanItemQueueListHead);
    if (first)
        ProcessScanItemsList(first);

    PhSetThreadPagePriority(NtCurrentThread(), pagePriority);
    PhSetThreadIoPriority(NtCurrentThread(), ioPriority);
    PhSetThreadBasePriority(NtCurrentThread(), threadPriority);

    return STATUS_SUCCESS;
}

_Function_class_(USER_THREAD_START_ROUTINE)
NTSTATUS ScanItemPriorityWorkerRoutine(
    _In_ PVOID Parameter
    )
{
    PSLIST_ENTRY first;

    first = RtlInterlockedFlushSList(&ScanItemPriorityQueueListHead);
    if (first)
        ProcessScanItemsList(first);

    return STATUS_SUCCESS;
}

PSCAN_ITEM CreateAndEnqueueScanItem(
    _In_ PSCAN_CONTEXT Context,
    _In_ SCAN_TYPE Type,
    _In_ ULONG Flags,
    _In_ BOOLEAN PriorityQueue,
    _In_opt_ PPH_STRING PreviousResult,
    _In_opt_ PSCAN_COMPLETE_CALLBACK Callback,
    _In_opt_ PVOID CallbackContext
    )
{
    PSCAN_ITEM item;

    item = PhCreateObject(sizeof(SCAN_ITEM), ScanItemObjectType);
    memset(item, 0, sizeof(SCAN_ITEM));

    PhInitializeQueuedLock(&item->Lock);
    item->Type = Type;
    item->Flags = Flags;
    item->FileHash = PhReferenceObject(Context->FileHash);
    item->Result = PhReferenceObject(ScanScanningString);
    if (PreviousResult)
        item->PreviousResult = PhReferenceObject(PreviousResult);
    item->Callback = Callback;
    item->CallbackContext = CallbackContext;
    item->Expiry.QuadPart = LONG64_MAX;

    PhReferenceObject(item);
    if (PriorityQueue)
    {
        if (!RtlInterlockedPushEntrySList(&ScanItemPriorityQueueListHead, &item->Entry))
            PhQueueItemWorkQueue(&ScanItemWorkPriorityQueue, ScanItemPriorityWorkerRoutine, NULL);
    }
    else
    {
        if (!RtlInterlockedPushEntrySList(&ScanItemQueueListHead, &item->Entry))
            PhQueueItemWorkQueue(&ScanItemWorkQueue, ScanItemWorkerRoutine, NULL);
    }

    return item;
}

NTSTATUS GetScanFileId(
    _In_ PPH_STRING FileName,
    _Out_ PSCAN_FILE_ID FileId
    )
{
    static const PH_STRINGREF volumePrefix = PH_STRINGREF_INIT(L"Volume{");
    NTSTATUS status;
    WCHAR volumePathName[MAX_PATH];
    WCHAR mountPoint[MAX_PATH];
    PH_STRINGREF mountPointRef;
    PH_STRINGREF firstPart;
    PH_STRINGREF secondPart;
    UNICODE_STRING volumeGuidString;
    HANDLE fileHandle;
    FILE_ID_INFORMATION fileIdInfo;
    GUID volumeGuid;

    memset(FileId, 0, sizeof(SCAN_FILE_ID));

    if (NT_SUCCESS(status = PhCreateFileWin32(
        &fileHandle,
        PhGetString(FileName),
        FILE_READ_ATTRIBUTES | SYNCHRONIZE,
        FILE_ATTRIBUTE_NORMAL,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        FILE_OPEN,
        FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT
        )))
    {
        status = PhGetFileId(fileHandle, &fileIdInfo);
        NtClose(fileHandle);
    }

    if (!NT_SUCCESS(status))
        return status;

    if (!GetVolumePathNameW(PhGetString(FileName), volumePathName, RTL_NUMBER_OF(volumePathName)))
        return STATUS_UNSUCCESSFUL;
    if (!GetVolumeNameForVolumeMountPointW(volumePathName, mountPoint, RTL_NUMBER_OF(mountPoint)))
        return STATUS_UNSUCCESSFUL;
    PhInitializeStringRef(&mountPointRef, mountPoint);
    if (!PhSplitStringRefAtString(&mountPointRef, &volumePrefix, FALSE, &firstPart, &secondPart))
        return STATUS_UNSUCCESSFUL;
    if (!PhSplitStringRefAtChar(&secondPart, L'}', &firstPart, &secondPart))
        return STATUS_UNSUCCESSFUL;
    firstPart.Buffer -= 1;
    firstPart.Length += (sizeof(WCHAR) * 2);
    if (!PhStringRefToUnicodeString(&firstPart, &volumeGuidString))
        return STATUS_UNSUCCESSFUL;
    if (!NT_SUCCESS(status = RtlGUIDFromString(&volumeGuidString, &volumeGuid)))
        return status;

    memcpy(&FileId->VolumeGuid, &volumeGuid, sizeof(GUID));
    memcpy(&FileId->FileId, &fileIdInfo.FileId, sizeof(FILE_ID_128));

    return status;
}

_Function_class_(PH_HASHTABLE_EQUAL_FUNCTION)
BOOLEAN NTAPI ScanHashHashtableEqualFunction(
    _In_ PVOID Entry1,
    _In_ PVOID Entry2
    )
{
    PSCAN_HASH hash1 = *((PSCAN_HASH*)Entry1);
    PSCAN_HASH hash2 = *((PSCAN_HASH*)Entry2);

    return (memcmp(&hash1->FileId, &hash2->FileId, sizeof(SCAN_FILE_ID)) == 0);
}

_Function_class_(PH_HASHTABLE_HASH_FUNCTION)
ULONG NTAPI ScanHashHashtableHashFunction(
    _In_ PVOID Entry
    )
{
    PSCAN_HASH hash = *((PSCAN_HASH*)Entry);

    return PhHashBytes((PUCHAR)&hash->FileId, sizeof(SCAN_FILE_ID));
}

PSCAN_HASH GetScanHash(
    _In_ PPH_STRING FileName
    )
{
    PSCAN_HASH scanHash;
    BOOLEAN haveFileId = FALSE;
    SCAN_FILE_ID fileId;

    if (NT_SUCCESS(GetScanFileId(FileName, &fileId)))
        haveFileId = TRUE;

    PhAcquireQueuedLockExclusive(&ScanHashHashtableLock);

    if (haveFileId)
    {
        PSCAN_HASH* entry;
        C_ASSERT(FIELD_OFFSET(SCAN_HASH, FileId) == 0);
        scanHash = (PSCAN_HASH)&fileId;
        entry = PhFindEntryHashtable(ScanHashHashtable, &scanHash);
        if (entry)
        {
            scanHash = *entry;
            PhReferenceObject(scanHash);
            goto CleanupExit;
        }
    }

    scanHash = PhCreateObject(sizeof(SCAN_HASH), ScanHashObjectType);
    memset(scanHash, 0, sizeof(SCAN_HASH));
    PhInitializeQueuedLock(&scanHash->Lock);
    scanHash->FileName = PhReferenceObject(FileName);
    scanHash->Status = STATUS_PENDING;

    if (haveFileId)
    {
        memcpy(&scanHash->FileId, &fileId, sizeof(SCAN_FILE_ID));
        PhReferenceObject(scanHash);
        PhAddEntryHashtable(ScanHashHashtable, &scanHash);
    }

CleanupExit:

    PhReleaseQueuedLockExclusive(&ScanHashHashtableLock);

    return scanHash;
}

VOID ReapScanHashCache(
    VOID
    )
{
    PSCAN_HASH* scanHash;
    ULONG enumerationKey = 0;
    PPH_LIST reapList = PhCreateList(10);

    PhAcquireQueuedLockExclusive(&ScanHashHashtableLock);

    while (PhEnumHashtable(ScanHashHashtable, (PVOID*)&scanHash, &enumerationKey))
    {
        if (PhGetObjectRefCount(*scanHash) == 1)
            PhAddItemList(reapList, *scanHash);
    }

    for (ULONG i = 0; i < reapList->Count; i++)
        PhRemoveEntryHashtable(ScanHashHashtable, &reapList->Items[i]);

    PhReleaseQueuedLockExclusive(&ScanHashHashtableLock);

    for (ULONG i = 0; i < reapList->Count; i++)
        PhDereferenceObject(reapList->Items[i]);

    PhDereferenceObject(reapList);
}

VOID InitializeScanContext(
    _Out_ PSCAN_CONTEXT Context
    )
{
    memset(Context, 0, sizeof(SCAN_CONTEXT));
}

VOID EnqueueScanInternal(
    _In_ PSCAN_CONTEXT Context,
    _In_ SCAN_TYPE Type,
    _In_ PPH_STRING FileName,
    _In_ ULONG Flags,
    _In_ BOOLEAN PriorityQueue,
    _In_opt_ PSCAN_COMPLETE_CALLBACK Callback,
    _In_opt_ PVOID CallbackContext
    )
{
    PPH_STRING previousResult = NULL;
    PSCAN_ITEM item;

    if (!Context->FileHash)
        Context->FileHash = GetScanHash(FileName);

    if (Context->ScanItems[Type])
    {
        WriteRelease8(&Context->ScanItems[Type]->Abort, TRUE);

        if (Context->ScanItems[Type]->Result)
        {
            PhAcquireQueuedLockShared(&Context->ScanItems[Type]->Lock);
            previousResult = PhReferenceObject(Context->ScanItems[Type]->Result);
            PhReleaseQueuedLockShared(&Context->ScanItems[Type]->Lock);
        }
    }

    item = CreateAndEnqueueScanItem(
        Context,
        Type,
        Flags,
        PriorityQueue,
        previousResult,
        Callback,
        CallbackContext
        );

    PhMoveReference(&Context->ScanItems[Type], item);

    PhClearReference(&previousResult);
}

VOID EnqueueScan(
    _In_ PSCAN_CONTEXT Context,
    _In_ SCAN_TYPE Type,
    _In_ PPH_STRING FileName,
    _In_ ULONG Flags,
    _In_opt_ PSCAN_COMPLETE_CALLBACK Callback,
    _In_opt_ PVOID CallbackContext
    )
{
    EnqueueScanInternal(
        Context,
        Type,
        FileName,
        Flags,
        TRUE,
        Callback,
        CallbackContext
        );
}

BOOLEAN ScanHashEqual(
    _In_ PSCAN_HASH Hash1,
    _In_ PSCAN_HASH Hash2
    )
{
    BOOLEAN equal = FALSE;

    PhAcquireQueuedLockShared(&Hash1->Lock);
    PhAcquireQueuedLockShared(&Hash2->Lock);

    if (Hash1->Sha256 && Hash2->Sha256)
        equal = PhEqualString(Hash1->Sha256, Hash2->Sha256, FALSE);

    PhReleaseQueuedLockShared(&Hash2->Lock);
    PhReleaseQueuedLockShared(&Hash1->Lock);

    return equal;
}

VOID EvaluateScanContext(
    _In_ PLARGE_INTEGER SystemTime,
    _Inout_ PSCAN_CONTEXT Context,
    _In_ PPH_STRING FileName,
    _In_ ULONG Flags
    )
{
    for (ULONG i = 0; i < RTL_NUMBER_OF(Context->ScanItems); i++)
    {
        if (!Context->ScanItems[i] || Context->ScanItems[i]->Expiry.QuadPart <= SystemTime->QuadPart)
        {
            EnqueueScanInternal(Context, i, FileName, Flags, FALSE, NULL, NULL);
        }
    }
}

VOID DeleteScanContext(
    _In_ PSCAN_CONTEXT Context
    )
{
    for (ULONG i = 0; i < RTL_NUMBER_OF(Context->ScanItems); i++)
    {
        if (Context->ScanItems[i])
        {
            WriteRelease8(&Context->ScanItems[i]->Abort, TRUE);
            PhDereferenceObject(Context->ScanItems[i]);
        }
    }

    PhClearReference(&Context->FileHash);
}

_Function_class_(PH_TYPE_DELETE_PROCEDURE)
VOID NTAPI ScanItemDeleteProcedure(
    _In_ PVOID Object,
    _In_ ULONG Flags
    )
{
    PSCAN_ITEM item = Object;

    PhClearReference(&item->FileHash);
    PhClearReference(&item->Result);
    PhClearReference(&item->PreviousResult);
}

_Function_class_(PH_TYPE_DELETE_PROCEDURE)
VOID NTAPI ScanHashDeleteProcedure(
    _In_ PVOID Object,
    _In_ ULONG Flags
    )
{
    PSCAN_HASH item = Object;

    PhClearReference(&item->FileName);
    PhClearReference(&item->Sha256);
}

BOOLEAN LoadBCrypt(
    VOID
    )
{
    PVOID baseAddress;

    if (baseAddress = PhLoadLibrary(L"bcrypt.dll"))
    {
        BCryptOpenAlgorithmProvider_I = PhGetProcedureAddress(baseAddress, "BCryptOpenAlgorithmProvider", 0);
        BCryptCloseAlgorithmProvider_I = PhGetProcedureAddress(baseAddress, "BCryptCloseAlgorithmProvider", 0);
        BCryptDestroyHash_I = PhGetProcedureAddress(baseAddress, "BCryptDestroyHash", 0);
        BCryptCreateMultiHash_I = PhGetProcedureAddress(baseAddress, "BCryptCreateMultiHash", 0);
        BCryptProcessMultiOperations_I = PhGetProcedureAddress(baseAddress, "BCryptProcessMultiOperations", 0);
    }

    return (
        BCryptOpenAlgorithmProvider_I &&
        BCryptCloseAlgorithmProvider_I &&
        BCryptDestroyHash_I &&
        BCryptCreateMultiHash_I &&
        BCryptProcessMultiOperations_I
        );
}

BOOLEAN LoadSQLite(
    VOID
    )
{
    PVOID baseAddress;

    if (baseAddress = PhLoadLibrary(L"winsqlite3.dll"))
    {
        sqlite3_open_v2_I = PhGetProcedureAddress(baseAddress, "sqlite3_open_v2", 0);
        sqlite3_close_v2_I = PhGetProcedureAddress(baseAddress, "sqlite3_close_v2", 0);
        sqlite3_exec_I = PhGetProcedureAddress(baseAddress, "sqlite3_exec", 0);
        sqlite3_prepare_v2_I = PhGetProcedureAddress(baseAddress, "sqlite3_prepare_v2", 0);
        sqlite3_finalize_I = PhGetProcedureAddress(baseAddress, "sqlite3_finalize", 0);
        sqlite3_bind_int64_I = PhGetProcedureAddress(baseAddress, "sqlite3_bind_int64", 0);
        sqlite3_bind_text16_I = PhGetProcedureAddress(baseAddress, "sqlite3_bind_text16", 0);
        sqlite3_step_I = PhGetProcedureAddress(baseAddress, "sqlite3_step", 0);
        sqlite3_reset_I = PhGetProcedureAddress(baseAddress, "sqlite3_reset", 0);
        sqlite3_column_int64_I = PhGetProcedureAddress(baseAddress, "sqlite3_column_int64", 0);
        sqlite3_column_text16_I = PhGetProcedureAddress(baseAddress, "sqlite3_column_text16", 0);
    }

    return (
        sqlite3_open_v2_I &&
        sqlite3_close_v2_I &&
        sqlite3_exec_I &&
        sqlite3_prepare_v2_I &&
        sqlite3_finalize_I &&
        sqlite3_bind_int64_I &&
        sqlite3_bind_text16_I &&
        sqlite3_step_I &&
        sqlite3_reset_I &&
        sqlite3_column_int64_I &&
        sqlite3_column_text16_I
        );
}

BOOLEAN InitializeScanning(
    VOID
    )
{
    static const PH_STRINGREF databaseFileName = PH_STRINGREF_INIT(L"scan.db");
    BOOLEAN result;
    PPH_STRING fileName;
    PPH_BYTES fileNameUTF8;
    ULONG version = 0;
    sqlite3_stmt* stmt;

    ScanVirusTotalPAT = PhGetStringSetting(SETTING_NAME_VIRUSTOTAL_DEFAULT_PAT);
    ScanHybridAnalysisPAT = PhGetStringSetting(SETTING_NAME_HYBRIDANAL_DEFAULT_PAT);

    ScanScanningString = PhCreateString(L"Scanning...");
    ScanUnauthorizedString = PhCreateString(L"Unauthorized");
    ScanCleanString = PhCreateString(L"Clean");
    ScanUnknownString = PhCreateString(L"Unknown");
    ScanRateLimitedString = PhCreateString(L"Rate limited...");
    ScanFileTooLarge = PhCreateString(L"File too large");

    result = FALSE;
    if (!!SystemInformer_IsPortableMode())
        fileName = PhGetApplicationDirectoryFileName(&databaseFileName, FALSE);
    else
        fileName = PhGetRoamingAppDataDirectory(&databaseFileName, FALSE);
    fileNameUTF8 = PhConvertStringRefToUtf8(&fileName->sr);

    ScanItemObjectType = PhCreateObjectType(L"ScanItem", 0, ScanItemDeleteProcedure);
    ScanHashObjectType = PhCreateObjectType(L"ScanHash", 0, ScanHashDeleteProcedure);
    PhInitializeWorkQueue(&ScanItemWorkQueue, 0, 3, 500);
    PhInitializeWorkQueue(&ScanItemWorkPriorityQueue, 0, 3, 500);
    RtlInitializeSListHead(&ScanItemQueueListHead);
    RtlInitializeSListHead(&ScanItemPriorityQueueListHead);
    ScanHashHashtable = PhCreateHashtable(
        sizeof(PSCAN_HASH),
        ScanHashHashtableEqualFunction,
        ScanHashHashtableHashFunction,
        100
        );

    if (!LoadSQLite() || !LoadBCrypt())
        goto CleanupExit;

    const char* fn = fileNameUTF8->Buffer;
    int flags = SQLITE_OPEN_CREATE | SQLITE_OPEN_READWRITE | SQLITE_OPEN_NOMUTEX;
    if (sqlite3_open_v2_I(fn, &ScanDB, flags, NULL) != SQLITE_OK)
        goto CleanupExit;

    if (sqlite3_exec_I(ScanDB, ScanDBSQL, NULL, NULL, NULL) != SQLITE_OK)
        goto CleanupExit;

    if (sqlite3_prepare_v2_I(ScanDB, "PRAGMA user_version;", -1, &stmt, NULL) == SQLITE_OK)
    {
        if (sqlite3_step_I(stmt) == SQLITE_ROW)
            version = (ULONG)sqlite3_column_int64_I(stmt, 0);
        sqlite3_finalize_I(stmt);
    }

    sqlite3_exec_I(ScanDB, "BEGIN TRANSACTION;", NULL, NULL, NULL);

    for (ULONG i = 0; i < RTL_NUMBER_OF(ScanDBVersion); i++)
    {
        CHAR pragma[64];

        if ((i + 1) > version)
        {
            if (sqlite3_exec_I(ScanDB, ScanDBVersion[i], NULL, NULL, NULL) != SQLITE_OK)
                goto CleanupExit;

            snprintf(pragma, sizeof(pragma), "PRAGMA user_version = %lu;", i + 1);
            if (sqlite3_exec_I(ScanDB, pragma, NULL, NULL, NULL) != SQLITE_OK)
                goto CleanupExit;
        }
    }

    if (sqlite3_exec_I(ScanDB, "COMMIT;", NULL, NULL, NULL) != SQLITE_OK)
    {
        sqlite3_exec_I(ScanDB, "ROLLBACK;", NULL, NULL, NULL);
        goto CleanupExit;
    }

    result = TRUE;
    for (ULONG i = 0; i < RTL_NUMBER_OF(ScanDBSQLSmts); i++)
    {
        if (sqlite3_prepare_v2_I(
            ScanDB,
            ScanDBSQLSmts[i].Sql,
            -1,
            ScanDBSQLSmts[i].Smt,
            NULL
            ) != SQLITE_OK)
        {
            result = FALSE;
            break;
        }
    }

CleanupExit:

    PhDereferenceObject(fileName);
    PhDereferenceObject(fileNameUTF8);

    return result;
}

VOID CleanupScanning(
    VOID
    )
{
    PhAcquireQueuedLockExclusive(&ScanDBLock);

    for (ULONG i = 0; i < RTL_NUMBER_OF(ScanDBSQLSmts); i++)
    {
        if (*ScanDBSQLSmts[i].Smt)
        {
            sqlite3_finalize_I(*ScanDBSQLSmts[i].Smt);
            *ScanDBSQLSmts[i].Smt = NULL;
        }
    }

    if (ScanDB)
    {
        sqlite3_close_v2_I(ScanDB);
        ScanDB = NULL;
    }

    PhReleaseQueuedLockExclusive(&ScanDBLock);
}
