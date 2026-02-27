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

#include <winsqlite/winsqlite3.h>

typedef struct _SCAN_HASH
{
    NTSTATUS Status;
    PH_QUEUED_LOCK Lock;
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
    PPH_STRING FileName;
    PSCAN_HASH FileHash;
    PPH_STRING Result;
    PPH_STRING PreviousResult;
    PSCAN_COMPLETE_CALLBACK Callback;
    PVOID CallbackContext;
} SCAN_ITEM, *PSCAN_ITEM;

typedef int SQLITE_APICALL sqlite3_open_v2_fn(
  const char *filename,   /* Database filename (UTF-8) */
  sqlite3 **ppDb,         /* OUT: SQLite db handle */
  int flags,              /* Flags */
  const char *zVfs        /* Name of VFS module to use */
);
typedef int SQLITE_APICALL sqlite3_close_v2_fn(sqlite3*);
typedef int SQLITE_APICALL sqlite3_exec_fn(
  sqlite3*,                                  /* An open database */
  const char *sql,                           /* SQL to be evaluated */
  int (SQLITE_CALLBACK *callback)(void*,int,char**,char**),  /* Callback function */
  void *,                                    /* 1st argument to callback */
  char **errmsg                              /* Error msg written here */
);
typedef int SQLITE_APICALL sqlite3_prepare_v2_fn(
  sqlite3 *db,            /* Database handle */
  const char *zSql,       /* SQL statement, UTF-8 encoded */
  int nByte,              /* Maximum length of zSql in bytes. */
  sqlite3_stmt **ppStmt,  /* OUT: Statement handle */
  const char **pzTail     /* OUT: Pointer to unused portion of zSql */
);
typedef int SQLITE_APICALL sqlite3_finalize_fn(sqlite3_stmt *pStmt);
typedef int SQLITE_APICALL sqlite3_bind_int64_fn(sqlite3_stmt*, int, sqlite3_int64);
typedef int SQLITE_APICALL sqlite3_bind_text16_fn(sqlite3_stmt*, int, const void*, int, void(SQLITE_CALLBACK *)(void*));
typedef int SQLITE_APICALL sqlite3_step_fn(sqlite3_stmt*);
typedef int SQLITE_APICALL sqlite3_reset_fn(sqlite3_stmt *pStmt);
typedef sqlite3_int64 SQLITE_APICALL sqlite3_column_int64_fn(sqlite3_stmt*, int iCol);
typedef const void *SQLITE_APICALL sqlite3_column_text16_fn(sqlite3_stmt*, int iCol);

static PPH_OBJECT_TYPE ScanItemObjectType;
static PPH_OBJECT_TYPE ScanHashObjectType;
static PH_WORK_QUEUE ScanItemWorkQueue;
static PH_WORK_QUEUE ScanItemWorkPriorityQueue;
static SLIST_HEADER ScanItemQueueListHead;
static SLIST_HEADER ScanItemPriorityQueueListHead;
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

static sqlite3* ScanDB = NULL;
static PH_QUEUED_LOCK ScanDBLock = PH_QUEUED_LOCK_INIT;
static sqlite3_stmt* ScanDBInsertVirusTotal = NULL;
static sqlite3_stmt* ScanDBQueryVirusTotal = NULL;
static sqlite3_stmt* ScanDBInsertHybridAnalysis = NULL;
static sqlite3_stmt* ScanDBQueryHybridAnalysis = NULL;
static sqlite3_open_v2_fn* sqlite3_open_v2_I = NULL;
static sqlite3_close_v2_fn* sqlite3_close_v2_I = NULL;
static sqlite3_exec_fn* sqlite3_exec_I = NULL;
static sqlite3_prepare_v2_fn* sqlite3_prepare_v2_I = NULL;
static sqlite3_finalize_fn* sqlite3_finalize_I = NULL;
static sqlite3_bind_int64_fn* sqlite3_bind_int64_I = NULL;
static sqlite3_bind_text16_fn* sqlite3_bind_text16_I = NULL;
static sqlite3_step_fn* sqlite3_step_I = NULL;
static sqlite3_reset_fn* sqlite3_reset_I = NULL;
static sqlite3_column_int64_fn* sqlite3_column_int64_I = NULL;
static sqlite3_column_text16_fn* sqlite3_column_text16_I = NULL;

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

    assert(Min < Max);

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

NTSTATUS GetFileHash(
    _In_ PPH_STRING FileName,
    _Out_ PPH_STRING* Hash
    )
{
    NTSTATUS status;
    HANDLE fileHandle;
    LARGE_INTEGER fileSize;
    KPH_HASH_INFORMATION hashInfo;
    PH_HASH_CONTEXT hashContext;
    PBYTE buffer = NULL;
    IO_STATUS_BLOCK iosb;
    PPH_STRING hash = NULL;

    *Hash = NULL;

    if (!NT_SUCCESS(status = PhCreateFileWin32(
        &fileHandle,
        PhGetString(FileName),
        FILE_READ_DATA | SYNCHRONIZE,
        FILE_ATTRIBUTE_NORMAL,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        FILE_OPEN,
        FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT | FILE_SEQUENTIAL_ONLY
        )))
        return status;

    if (!NT_SUCCESS(status = PhGetFileSize(fileHandle, &fileSize)))
        goto CleanupExit;

    if (fileSize.QuadPart > ScanMaxFileSize)
    {
        status = STATUS_FILE_TOO_LARGE;
        goto CleanupExit;
    }

    if (KsiLevel() == KphLevelMax)
    {
        hashInfo.Algorithm = KphHashAlgorithmSha256;

        if (NT_SUCCESS(status = KsiQueryHashInformationFile(fileHandle, &hashInfo, sizeof(hashInfo))))
        {
            *Hash = PhBufferToHexString(hashInfo.Hash, hashInfo.Length);
            goto CleanupExit;
        }
    }

    if (!NT_SUCCESS(status = PhInitializeHash(&hashContext, Sha256HashAlgorithm)))
        goto CleanupExit;

    buffer = PhAllocate(PAGE_SIZE * 2);

    while (NT_SUCCESS(NtReadFile(
        fileHandle,
        NULL,
        NULL,
        NULL,
        &iosb,
        buffer,
        PAGE_SIZE * 2,
        NULL,
        NULL
        )))
    {
        ULONG returnLength;

        returnLength = (ULONG)iosb.Information;

        if (returnLength == 0)
            break;

        if (!NT_SUCCESS(status = PhUpdateHash(&hashContext, buffer, returnLength)))
            goto CleanupExit;
    }

    if (!NT_SUCCESS(status = PhFinalHashString(&hashContext, &hash)))
        goto CleanupExit;

    *Hash = hash;
    hash = NULL;

CleanupExit:

    PhClearReference(&hash);

    if (buffer)
        PhFree(buffer);

    NtClose(fileHandle);

    return status;
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

VOID ProcessScanItem(
    _In_ PSCAN_ITEM Item
    )
{
    NTSTATUS status;

    if (ReadAcquire8(&Item->Abort))
        return;

    PhAcquireQueuedLockExclusive(&Item->FileHash->Lock);
    status = Item->FileHash->Status;
    if (status == STATUS_PENDING)
    {
        status = GetFileHash(Item->FileName, &Item->FileHash->Sha256);
        Item->FileHash->Status = status;
    }
    PhReleaseQueuedLockExclusive(&Item->FileHash->Lock);

    if (NT_SUCCESS(status))
    {
        if (Item->Type == SCAN_TYPE_VIRUSTOTAL)
            ProcessVirusTotal(Item);
        else if (Item->Type == SCAN_TYPE_HYBRIDANALYSIS)
            ProcessHybridAnalysis(Item);
    }
    else if (status == STATUS_FILE_TOO_LARGE)
    {
        SetScanResult(Item, PhReferenceObject(ScanFileTooLarge));
    }

    if (Item->Result == ScanScanningString)
    {
        if (Item->PreviousResult)
            SetScanResult(Item, PhReferenceObject(Item->PreviousResult));
        else
            SetScanResult(Item, PhReferenceEmptyString());
    }

    if (Item->Callback)
    {
        Item->Callback(
            Item->Type,
            Item->FileName,
            Item->FileHash,
            Item->Result,
            Item->CallbackContext
            );
    }
}

_Function_class_(USER_THREAD_START_ROUTINE)
NTSTATUS ScanItemWorkerRoutine(
    _In_ PVOID Parameter
    )
{
    PSLIST_ENTRY entry;
    IO_PRIORITY_HINT ioPriority;
    KPRIORITY priority;

    PhGetThreadBasePriority(NtCurrentThread(), &priority);
    PhSetThreadBasePriority(NtCurrentThread(), THREAD_PRIORITY_BELOW_NORMAL);
    PhGetThreadIoPriority(NtCurrentThread(), &ioPriority);
    PhSetThreadIoPriority(NtCurrentThread(), IoPriorityLow);

    entry = RtlInterlockedFlushSList(&ScanItemQueueListHead);

    while (entry)
    {
        PSCAN_ITEM item;

        item = CONTAINING_RECORD(entry, SCAN_ITEM, Entry);
        entry = entry->Next;

        ProcessScanItem(item);
        PhDereferenceObject(item);
    }

    PhSetThreadIoPriority(NtCurrentThread(), ioPriority);
    PhSetThreadBasePriority(NtCurrentThread(), priority);

    return STATUS_SUCCESS;
}

_Function_class_(USER_THREAD_START_ROUTINE)
NTSTATUS ScanItemPriorityWorkerRoutine(
    _In_ PVOID Parameter
    )
{
    PSLIST_ENTRY entry;

    entry = RtlInterlockedFlushSList(&ScanItemPriorityQueueListHead);

    while (entry)
    {
        PSCAN_ITEM item;

        item = CONTAINING_RECORD(entry, SCAN_ITEM, Entry);
        entry = entry->Next;

        ProcessScanItem(item);
        PhDereferenceObject(item);
    }

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
    item->FileName = PhReferenceObject(Context->FileName);
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

VOID InitializeScanContext(
    _Out_ PSCAN_CONTEXT Context
    )
{
    memset(Context, 0, sizeof(SCAN_CONTEXT));
    if (ScanHashObjectType) // checks if scanning is enabled
    {
        Context->FileHash = PhCreateObject(sizeof(SCAN_HASH), ScanHashObjectType);;
        memset(Context->FileHash, 0, sizeof(SCAN_HASH));
        PhInitializeQueuedLock(&Context->FileHash->Lock);
        Context->FileHash->Status = STATUS_PENDING;
    }
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

    if (!Context->FileName)
        Context->FileName = PhReferenceObject(FileName);

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

    PhReleaseQueuedLockShared(&Hash1->Lock);
    PhReleaseQueuedLockShared(&Hash2->Lock);

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
    PhClearReference(&Context->FileName);
}

_Function_class_(PH_TYPE_DELETE_PROCEDURE)
VOID NTAPI ScanItemDeleteProcedure(
    _In_ PVOID Object,
    _In_ ULONG Flags
    )
{
    PSCAN_ITEM item = Object;

    PhClearReference(&item->FileName);
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

    PhClearReference(&item->Sha256);
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

    if (!LoadSQLite())
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
