/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     dmex    2016-2017
 *
 */

#ifndef DB_H
#define DB_H

extern PH_STRINGREF ProcessObjectDbHash;
extern PH_STRINGREF ServiceObjectDbHash;

typedef struct _PROCESS_DB_OBJECT
{
    PH_STRINGREF FileName;

    INT64 Positives;
    PPH_STRING Hash;
    PPH_STRING Result;
} PROCESS_DB_OBJECT, *PPROCESS_DB_OBJECT;

VOID InitializeProcessDb(
    VOID
    );

VOID CleanupProcessDb(
    VOID
    );

VOID LockProcessDb(
    VOID
    );

VOID UnlockProcessDb(
    VOID
    );

PPROCESS_DB_OBJECT FindProcessDbObject(
    _In_ PPH_STRINGREF Hash
    );

PPROCESS_DB_OBJECT CreateProcessDbObject(
    _In_ PPH_STRING FileName,
    _In_opt_ INT64 Positives,
    _In_opt_ INT64 Total,
    _In_opt_ PPH_STRING Hash
    );

#endif
