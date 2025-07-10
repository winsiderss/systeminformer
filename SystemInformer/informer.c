/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     jxy-s   2024-2025
 *
 */
#include <phapp.h>
#include <phsettings.h>
#include <phplug.h>
#include <procprv.h>
#include <kphcomms.h>
#include <mapldr.h>

#include <winsqlite/winsqlite3.h>

#include <informer.h>

typedef struct _PH_INFORMER_DB_REAP
{
    ULONG64 ProcessStartKey;
    ULONG64 MaxCount;
    ULONG Seconds;
} PH_INFORMER_DB_REAP, *PPH_INFORMER_DB_REAP;

typedef int SQLITE_APICALL sqlite3_open_v2_fn(
  const char *filename,   /* Database filename (UTF-8) */
  sqlite3 **ppDb,         /* OUT: SQLite db handle */
  int flags,              /* Flags */
  const char *zVfs        /* Name of VFS module to use */
);
typedef int SQLITE_APICALL sqlite3_exec_fn(
  sqlite3*,                                  /* An open database */
  const char *sql,                           /* SQL to be evaluated */
  int (SQLITE_CALLBACK *callback)(void*,int,char**,char**),  /* Callback function */
  void *,                                    /* 1st argument to callback */
  char **errmsg                              /* Error msg written here */
);
typedef int SQLITE_APICALL sqlite3_create_function_v2_fn(
  sqlite3 *db,
  const char *zFunctionName,
  int nArg,
  int eTextRep,
  void *pApp,
  void (SQLITE_CALLBACK *xFunc)(sqlite3_context*,int,sqlite3_value**),
  void (SQLITE_CALLBACK *xStep)(sqlite3_context*,int,sqlite3_value**),
  void (SQLITE_CALLBACK *xFinal)(sqlite3_context*),
  void(SQLITE_CALLBACK *xDestroy)(void*)
);
typedef int SQLITE_APICALL sqlite3_prepare_v2_fn(
  sqlite3 *db,            /* Database handle */
  const char *zSql,       /* SQL statement, UTF-8 encoded */
  int nByte,              /* Maximum length of zSql in bytes. */
  sqlite3_stmt **ppStmt,  /* OUT: Statement handle */
  const char **pzTail     /* OUT: Pointer to unused portion of zSql */
);
typedef int SQLITE_APICALL sqlite3_bind_int64_fn(sqlite3_stmt*, int, sqlite3_int64);
typedef int SQLITE_APICALL sqlite3_step_fn(sqlite3_stmt*);
typedef int SQLITE_APICALL sqlite3_reset_fn(sqlite3_stmt *pStmt);
typedef sqlite3_int64 SQLITE_APICALL sqlite3_value_int64_fn(sqlite3_value*);
typedef sqlite3_int64 SQLITE_APICALL sqlite3_column_int64_fn(sqlite3_stmt*, int iCol);

static PPH_OBJECT_TYPE KphMessageObjectType = NULL;

static PH_CALLBACK_REGISTRATION PhpInformerProcessesUpdatedRegistration;
static PH_CALLBACK_REGISTRATION PhpInformerProcessesRemovedRegistration;

static PH_FREE_LIST PhpInformerReapFreeList;
static PH_QUEUED_LOCK PhpInformerDatabaseLock = PH_QUEUED_LOCK_INIT;
static sqlite3* PhpInformerDB = NULL;
static sqlite3_stmt* PhpInformerDBInsert = NULL;
static sqlite3_stmt* PhpInformerDBQueryCount = NULL;
static sqlite3_stmt* PhpInformerDBReapMax = NULL;
static sqlite3_stmt* PhpInformerDBReapTime = NULL;
static sqlite3_stmt* PhpInformerDBReapProc = NULL;
static sqlite3_stmt* PhpInformerDBQuery = NULL;
static sqlite3_stmt* PhpInformerDBQueryProc = NULL;

static sqlite3_open_v2_fn* sqlite3_open_v2_I = NULL;
static sqlite3_exec_fn* sqlite3_exec_I = NULL;
static sqlite3_create_function_v2_fn* sqlite3_create_function_v2_I = NULL;
static sqlite3_prepare_v2_fn* sqlite3_prepare_v2_I = NULL;
static sqlite3_bind_int64_fn* sqlite3_bind_int64_I = NULL;
static sqlite3_step_fn* sqlite3_step_I = NULL;
static sqlite3_reset_fn* sqlite3_reset_I = NULL;
static sqlite3_value_int64_fn* sqlite3_value_int64_I = NULL;
static sqlite3_column_int64_fn* sqlite3_column_int64_I = NULL;

PH_CALLBACK_DECLARE(PhInformerCallback);

VOID PhpInformerGetKeys(
    _In_ PKPH_MESSAGE Message,
    _Out_ PULONG64 ActorKey,
    _Out_ PULONG64 TargetKey
    )
{
    *ActorKey = ULONG64_MAX;
    *TargetKey = ULONG64_MAX;

    switch (Message->Header.MessageId)
    {
    case KphMsgProcessCreate:
        *ActorKey = Message->Kernel.ProcessCreate.CreatingProcessStartKey;
        *TargetKey = Message->Kernel.ProcessCreate.CreatingProcessStartKey;
        return;
    case KphMsgProcessExit:
        *ActorKey = Message->Kernel.ProcessExit.ProcessStartKey;
        *TargetKey = Message->Kernel.ProcessExit.ProcessStartKey;
        return;
    case KphMsgThreadCreate:
        *ActorKey = Message->Kernel.ThreadCreate.CreatingProcessStartKey;
        *TargetKey = Message->Kernel.ThreadCreate.TargetProcessStartKey;
        return;
    case KphMsgThreadExecute:
        *ActorKey = Message->Kernel.ThreadExecute.ProcessStartKey;
        *TargetKey = Message->Kernel.ThreadExecute.ProcessStartKey;
        return;
    case KphMsgThreadExit:
        *ActorKey = Message->Kernel.ThreadExit.ProcessStartKey;
        *TargetKey = Message->Kernel.ThreadExit.ProcessStartKey;
        return;
    case KphMsgImageLoad:
        *ActorKey = Message->Kernel.ImageLoad.LoadingProcessStartKey;
        *TargetKey = Message->Kernel.ImageLoad.LoadingProcessStartKey;
        return;
    case KphMsgImageVerify:
        *ActorKey = Message->Kernel.ImageVerify.ProcessStartKey;
        return;
    case KphMsgDebugPrint:
        return;
    default:
        break;
    }

    if (Message->Header.MessageId >= KphMsgHandlePreCreateProcess &&
        Message->Header.MessageId <= KphMsgHandlePostDuplicateDesktop)
    {
        *ActorKey = Message->Kernel.Handle.ContextProcessStartKey;
        return;
    }

    if (Message->Header.MessageId >= KphMsgFilePreCreate &&
        Message->Header.MessageId <= KphMsgFilePostVolumeDismount)
    {
        *ActorKey = Message->Kernel.File.ProcessStartKey;
        return;
    }

    if (Message->Header.MessageId >= KphMsgRegPreDeleteKey &&
        Message->Header.MessageId <= KphMsgRegPostSaveMergedKey)
    {
        *ActorKey = Message->Kernel.Reg.ProcessStartKey;
        return;
    }
}

ULONG64 PhpInformerDatabaseQueryCountUnsafe(
    VOID
    )
{
    ULONG64 count = 0;

    if (sqlite3_step_I(PhpInformerDBQueryCount) == SQLITE_ROW)
        count = sqlite3_column_int64_I(PhpInformerDBQueryCount, 0);

    sqlite3_reset_I(PhpInformerDBQueryCount);

    return count;
}

VOID PhpInformerDatabaseInsert(
    _In_ PKPH_MESSAGE Message
    )
{
    if (PhpInformerDBInsert)
    {
        ULONG64 actorKey;
        ULONG64 targetKey;

        PhpInformerGetKeys(Message, &actorKey, &targetKey);

        PhAcquireQueuedLockExclusive(&PhpInformerDatabaseLock);

        sqlite3_bind_int64_I(PhpInformerDBInsert, 1, Message->Header.TimeStamp.QuadPart);
        sqlite3_bind_int64_I(PhpInformerDBInsert, 2, actorKey);
        sqlite3_bind_int64_I(PhpInformerDBInsert, 3, targetKey);
        sqlite3_bind_int64_I(PhpInformerDBInsert, 4, (LONG64)(LONG_PTR)Message);
        sqlite3_step_I(PhpInformerDBInsert);
        sqlite3_reset_I(PhpInformerDBInsert);

        PhReleaseQueuedLockExclusive(&PhpInformerDatabaseLock);
    }
}

VOID PhpInformerDatabaseReap(
    _In_ PPH_INFORMER_DB_REAP Reap
    )
{
    LARGE_INTEGER systemTime;

    PhAcquireQueuedLockExclusive(&PhpInformerDatabaseLock);

    if (PhpInformerDBReapProc && Reap->ProcessStartKey)
    {
        sqlite3_bind_int64_I(PhpInformerDBReapProc, 1, Reap->ProcessStartKey);
        sqlite3_bind_int64_I(PhpInformerDBReapProc, 2, Reap->ProcessStartKey);
        sqlite3_step_I(PhpInformerDBReapProc);
        sqlite3_reset_I(PhpInformerDBReapProc);
    }

    if (PhpInformerDBReapTime && Reap->Seconds)
    {
        KphMsgQuerySystemTime(&systemTime);
        systemTime.QuadPart -= (LONG64)Reap->Seconds * 10000000;

        sqlite3_bind_int64_I(PhpInformerDBReapTime, 1, systemTime.QuadPart);
        sqlite3_step_I(PhpInformerDBReapTime);
        sqlite3_reset_I(PhpInformerDBReapTime);
    }

    if (PhpInformerDBReapMax && Reap->MaxCount)
    {
        if (PhpInformerDatabaseQueryCountUnsafe() > Reap->MaxCount)
        {
            sqlite3_bind_int64_I(PhpInformerDBReapMax, 1, Reap->MaxCount);
            sqlite3_step_I(PhpInformerDBReapMax);
            sqlite3_reset_I(PhpInformerDBReapMax);
        }
    }

    PhReleaseQueuedLockExclusive(&PhpInformerDatabaseLock);
}

PPH_LIST PhInformerDatabaseQuery(
    _In_ ULONG64 ProcessStartKey,
    _In_opt_ PLARGE_INTEGER TimeStamp
    )
{
    LARGE_INTEGER timeStamp;
    PPH_LIST messages;

    messages = PhCreateList(10);

    if (PhpInformerDBQuery && PhpInformerDBQueryProc)
    {
        if (TimeStamp)
        {
            timeStamp.QuadPart = TimeStamp->QuadPart;
        }
        else
        {
            timeStamp.QuadPart = 0;
        }

        PhAcquireQueuedLockExclusive(&PhpInformerDatabaseLock);

        if (ProcessStartKey)
        {
            sqlite3_bind_int64_I(PhpInformerDBQueryProc, 1, ProcessStartKey);
            sqlite3_bind_int64_I(PhpInformerDBQueryProc, 2, ProcessStartKey);
            sqlite3_bind_int64_I(PhpInformerDBQueryProc, 3, timeStamp.QuadPart);

            while (sqlite3_step_I(PhpInformerDBQueryProc) == SQLITE_ROW)
            {
                PKPH_MESSAGE message;

                message = (PVOID)(LONG_PTR)sqlite3_column_int64_I(PhpInformerDBQueryProc, 0);

                PhAddItemList(messages, PhReferenceObject(message));
            }

            sqlite3_reset_I(PhpInformerDBQueryProc);
        }
        else
        {
            sqlite3_bind_int64_I(PhpInformerDBQuery, 1, timeStamp.QuadPart);

            while (sqlite3_step_I(PhpInformerDBQuery) == SQLITE_ROW)
            {
                PKPH_MESSAGE message;

                message = (PVOID)(LONG_PTR)sqlite3_column_int64_I(PhpInformerDBQuery, 0);

                PhAddItemList(messages, PhReferenceObject(message));
            }

            sqlite3_reset_I(PhpInformerDBQuery);
        }

        PhReleaseQueuedLockExclusive(&PhpInformerDatabaseLock);
    }

    return messages;
}

VOID PhpInformerDatabaseReferenceMessage(
    sqlite3_context* Context,
    INT Argc,
    sqlite3_value** Argv
    )
{
    PKPH_MESSAGE message;

    message = (PVOID)(LONG_PTR)sqlite3_value_int64_I(Argv[0]);

    PhReferenceObject(message);
}

VOID PhpInformerDatabaseDereferenceMessage(
    sqlite3_context* Context,
    INT Argc,
    sqlite3_value** Argv
    )
{
    PKPH_MESSAGE message;

    message = (PVOID)(LONG_PTR)sqlite3_value_int64_I(Argv[0]);

    PhDereferenceObject(message);
}

BOOLEAN PhpInfomerLoadSQLite(
    VOID
    )
{
    PVOID baseAddress;

    if (baseAddress = PhLoadLibrary(L"winsqlite3.dll"))
    {
        sqlite3_open_v2_I = PhGetDllBaseProcedureAddress(baseAddress, "sqlite3_open_v2", 0);
        sqlite3_exec_I = PhGetDllBaseProcedureAddress(baseAddress, "sqlite3_exec", 0);
        sqlite3_create_function_v2_I = PhGetDllBaseProcedureAddress(baseAddress, "sqlite3_create_function_v2", 0);
        sqlite3_prepare_v2_I = PhGetDllBaseProcedureAddress(baseAddress, "sqlite3_prepare_v2", 0);
        sqlite3_bind_int64_I = PhGetDllBaseProcedureAddress(baseAddress, "sqlite3_bind_int64", 0);
        sqlite3_step_I = PhGetDllBaseProcedureAddress(baseAddress, "sqlite3_step", 0);
        sqlite3_reset_I = PhGetDllBaseProcedureAddress(baseAddress, "sqlite3_reset", 0);
        sqlite3_value_int64_I = PhGetDllBaseProcedureAddress(baseAddress, "sqlite3_value_int64", 0);
        sqlite3_column_int64_I = PhGetDllBaseProcedureAddress(baseAddress, "sqlite3_column_int64", 0);
    }

    return (
        sqlite3_open_v2_I &&
        sqlite3_exec_I &&
        sqlite3_create_function_v2_I &&
        sqlite3_prepare_v2_I &&
        sqlite3_bind_int64_I &&
        sqlite3_step_I &&
        sqlite3_reset_I &&
        sqlite3_value_int64_I &&
        sqlite3_column_int64_I
        );
}

VOID PhpInitializeInformerDatabase(
    VOID
    )
{
    if (!PhpInfomerLoadSQLite())
        return;

    if (sqlite3_open_v2_I(
        "informer",
        &PhpInformerDB,
        SQLITE_OPEN_MEMORY | SQLITE_OPEN_READWRITE | SQLITE_OPEN_NOMUTEX,
        NULL
        ) == SQLITE_OK)
    {
        if (sqlite3_exec_I(
            PhpInformerDB,
            "PRAGMA synchronous = OFF;"
            "PRAGMA journal_mode = OFF;"
            "DROP TABLE IF EXISTS messages;"
            "CREATE TABLE messages("
            "id INTEGER PRIMARY KEY AUTOINCREMENT,"
            "time_stamp INTEGER NOT NULL,"
            "actor_key INTEGER NOT NULL,"
            "target_key INTEGER NOT NULL,"
            "message INTEGER NOT NULL"
            ");",
            NULL,
            NULL,
            NULL
            ) == SQLITE_OK)
        {
            sqlite3_create_function_v2_I(
                PhpInformerDB,
                "reference_message",
                1,
                SQLITE_UTF8,
                NULL,
                PhpInformerDatabaseReferenceMessage,
                NULL,
                NULL,
                NULL
                );

            sqlite3_create_function_v2_I(
                PhpInformerDB,
                "dereference_message",
                1,
                SQLITE_UTF8,
                NULL,
                PhpInformerDatabaseDereferenceMessage,
                NULL,
                NULL,
                NULL
                );

            sqlite3_exec_I(
                PhpInformerDB,
                "CREATE TRIGGER before_insert_messages "
                "BEFORE INSERT ON messages "
                "FOR EACH ROW "
                "BEGIN "
                "    SELECT reference_message(NEW.message); "
                "END;",
                NULL,
                NULL,
                NULL
                );

            sqlite3_exec_I(
                PhpInformerDB,
                "CREATE TRIGGER after_delete_messages "
                "AFTER DELETE ON messages "
                "FOR EACH ROW "
                "BEGIN "
                "    SELECT dereference_message(OLD.message); "
                "END;",
                NULL,
                NULL,
                NULL
                );

            sqlite3_prepare_v2_I(
                PhpInformerDB,
                "INSERT INTO messages(time_stamp, actor_key, target_key, message)"
                "VALUES(?, ?, ?, ?);",
                -1,
                &PhpInformerDBInsert,
                NULL
                );

            sqlite3_prepare_v2_I(
                PhpInformerDB,
                "SELECT COUNT(*) FROM messages;",
                -1,
                &PhpInformerDBQueryCount,
                NULL
                );

            sqlite3_prepare_v2_I(
                PhpInformerDB,
                "DELETE FROM messages "
                "WHERE id IN ("
                "    SELECT id FROM messages "
                "    ORDER BY id ASC "
                "    LIMIT (SELECT COUNT(*) - ? FROM messages)"
                ");",
                -1,
                &PhpInformerDBReapMax,
                NULL
                );

            sqlite3_prepare_v2_I(
                PhpInformerDB,
                "DELETE FROM messages "
                "WHERE time_stamp < ?;",
                -1,
                &PhpInformerDBReapTime,
                NULL
                );

            sqlite3_prepare_v2_I(
                PhpInformerDB,
                "DELETE FROM messages "
                "WHERE actor_key = ? OR target_key = ?;",
                -1,
                &PhpInformerDBReapProc,
                NULL
                );

            sqlite3_prepare_v2_I(
                PhpInformerDB,
                "SELECT message FROM messages "
                "WHERE time_stamp >= ? "
                "ORDER BY time_stamp ASC;",
                -1,
                &PhpInformerDBQuery,
                NULL
                );

            sqlite3_prepare_v2_I(
                PhpInformerDB,
                "SELECT message FROM messages "
                "WHERE (actor_key = ? OR target_key = ?) AND time_stamp >= ? "
                "ORDER BY time_stamp ASC;",
                -1,
                &PhpInformerDBQueryProc,
                NULL
                );
        }
    }
}

NTSTATUS PhInformerReply(
    _Inout_ PPH_INFORMER_CONTEXT Context,
    _In_ PKPH_MESSAGE ReplyMessage
    )
{
    NTSTATUS status;

    if (Context->Handled)
        return STATUS_INVALID_PARAMETER_1;

    if (NT_SUCCESS(status = KphCommsReplyMessage(Context->ReplyToken, ReplyMessage)))
        Context->Handled = TRUE;

    return status;
}

BOOLEAN PhInformerDispatch(
    _In_ ULONG_PTR ReplyToken,
    _In_ PCKPH_MESSAGE Message
    )
{
    PKPH_MESSAGE message;
    PH_INFORMER_CONTEXT context;

    message = PhCreateObject(Message->Header.Size, KphMessageObjectType);
    memcpy(message, Message, Message->Header.Size);

    context.Message = message;
    context.ReplyToken = ReplyToken;
    context.Handled = FALSE;

    PhpInformerDatabaseInsert(message);

    PhInvokeCallback(&PhInformerCallback, &context);

    PhDereferenceObject(message);

    return context.Handled;
}

_Function_class_(USER_THREAD_START_ROUTINE)
NTSTATUS NTAPI PhpInformerReapWorkItemRoutine(
    _In_ PVOID Parameter
    )
{
    PPH_INFORMER_DB_REAP reap = Parameter;

    PhpInformerDatabaseReap(reap);

    PhFreeToFreeList(&PhpInformerReapFreeList, reap);

    return STATUS_SUCCESS;
}

_Function_class_(PH_CALLBACK_FUNCTION)
VOID NTAPI PhpInformerProcessUpdatedHandler(
    _In_ PVOID Parameter,
    _In_ PVOID Context
    )
{
    ULONG runCount = PtrToUlong(Parameter);
    PPH_INFORMER_DB_REAP reap;

    reap = PhAllocateFromFreeList(&PhpInformerReapFreeList);

    //
    // We cache the process monitor data for a lookback period so when a user
    // desires to inspect real time activity for a process we can provide a
    // reasonable amount of historical data. Here, reap the information that has
    // expired from the configured lookback period. We also configure a cache
    // limit which enforces a maximum on the total number of retained messages.
    //

    reap->ProcessStartKey = 0;
    reap->MaxCount = PhProcessMonitorCacheLimit;

    if (PhProcessMonitorLookback && runCount % PhProcessMonitorLookback == 0)
        reap->Seconds = PhProcessMonitorLookback;

    if (!NT_SUCCESS(PhQueueUserWorkItem(PhpInformerReapWorkItemRoutine, reap)))
        PhFreeToFreeList(&PhpInformerReapFreeList, reap);
}

_Function_class_(PH_CALLBACK_FUNCTION)
VOID NTAPI PhpInformerProcessRemovedHandler(
    _In_ PVOID Parameter,
    _In_ PVOID Context
    )
{
    PPH_PROCESS_ITEM processItem = (PPH_PROCESS_ITEM)Parameter;
    PPH_INFORMER_DB_REAP reap;

    if (processItem->ProcessStartKey)
    {
        //
        // This is opportunistic since the provider does not track short-lived
        // processes. There is also no guarantee that more information about
        // this process will be added to the database later by the asynchronous
        // nature of processing informer data and the fact that the operating
        // system generates activity for a process after it has "exited". That
        // said, we know at this point that the user can no longer inspect the
        // process to view activity associated with it so we might as well ask
        // the database to reap what it can. Anything missed will be reaped by
        // the periodic lookback reaper.
        //

        reap = PhAllocateFromFreeList(&PhpInformerReapFreeList);

        reap->ProcessStartKey = processItem->ProcessStartKey;
        reap->MaxCount = 0;
        reap->Seconds = 0;

        if (!NT_SUCCESS(PhQueueUserWorkItem(PhpInformerReapWorkItemRoutine, reap)))
            PhFreeToFreeList(&PhpInformerReapFreeList, reap);
    }
}

VOID PhInformerInitialize(
    VOID
    )
{
    PH_OBJECT_TYPE_PARAMETERS parameters;

    parameters.FreeListSize = 1024;
    parameters.FreeListCount = 65536;

    KphMessageObjectType = PhCreateObjectTypeEx(
        L"KsiMessage",
        PH_OBJECT_TYPE_TRY_USE_FREE_LIST,
        NULL,
        &parameters
        );

    PhInitializeFreeList(&PhpInformerReapFreeList, sizeof(PH_INFORMER_DB_REAP), 10);

    if (PhEnableProcessMonitor)
    {
        PhpInitializeInformerDatabase();

        PhRegisterCallback(
            PhGetGeneralCallback(GeneralCallbackProcessProviderUpdatedEvent),
            PhpInformerProcessUpdatedHandler,
            NULL,
            &PhpInformerProcessesUpdatedRegistration
            );

        PhRegisterCallback(
            PhGetGeneralCallback(GeneralCallbackProcessProviderRemovedEvent),
            PhpInformerProcessRemovedHandler,
            NULL,
            &PhpInformerProcessesRemovedRegistration
            );
    }
}
