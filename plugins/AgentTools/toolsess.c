/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     jxy-s   2026
 *
 */

#include "agenttools.h"
#include <ntlsa.h>
#include <mapldr.h>
#include <winsta.h>

// The SDK's ntsecapi.h redefines the LSA structures phnt already declares, so the handful of flag
// values needed here are spelled out the way the application's own user list does it.
#define LOGON_GUEST                 0x00001
#define LOGON_NOENCRYPTION          0x00002
#define LOGON_CACHED_ACCOUNT        0x00004
#define LOGON_EXTRA_SIDS            0x00020
#define LOGON_NTLMV2_ENABLED        0x00100
#define LOGON_PROFILE_PATH_RETURNED 0x00400
#define LOGON_NT_V2                 0x00800
#define LOGON_LM_V2                 0x01000
#define LOGON_NTLM_V2               0x02000
#define LOGON_OPTIMIZED             0x04000
#define LOGON_WINLOGON              0x08000
#define LOGON_PKINIT                0x10000
#define LOGON_NO_OPTIMIZED          0x20000
#define LOGON_NO_ELEVATION          0x40000
#define LOGON_MANAGED_SERVICE       0x80000

// Who is logged on, and how. A process runs as somebody, and the logon session is where that
// somebody came from: typed at the keyboard, arrived over the network, started as a service, or
// came in over RDP. It is the first thing to establish about a machine, because every later
// question - whose process is this, who could have started it - is answered against this list.

static NTSTATUS (NTAPI* AtpLsaFreeReturnBuffer)(
    _In_ PVOID Buffer
    ) = NULL;

static NTSTATUS (NTAPI* AtpLsaEnumerateLogonSessions)(
    _Out_ PULONG LogonSessionCount,
    _Out_ PLUID* LogonSessionList
    ) = NULL;

static NTSTATUS (NTAPI* AtpLsaGetLogonSessionData)(
    _In_ PLUID LogonId,
    _Out_ PSECURITY_LOGON_SESSION_DATA* LogonSessionData
    ) = NULL;

BOOLEAN AtpInitializeLsaRoutines(
    VOID
    )
{
    static PH_INITONCE initOnce = PH_INITONCE_INIT;

    if (PhBeginInitOnce(&initOnce))
    {
        PVOID baseAddress;

        if (baseAddress = PhLoadLibrary(L"secur32.dll"))
        {
            AtpLsaFreeReturnBuffer = PhGetProcedureAddress(baseAddress, "LsaFreeReturnBuffer", 0);
            AtpLsaEnumerateLogonSessions = PhGetProcedureAddress(baseAddress, "LsaEnumerateLogonSessions", 0);
            AtpLsaGetLogonSessionData = PhGetProcedureAddress(baseAddress, "LsaGetLogonSessionData", 0);
        }

        PhEndInitOnce(&initOnce);
    }

    return !!AtpLsaFreeReturnBuffer && !!AtpLsaEnumerateLogonSessions && !!AtpLsaGetLogonSessionData;
}

PCWSTR AtpLogonTypeString(
    _In_ ULONG LogonType
    )
{
    switch (LogonType)
    {
    case Interactive:
        return L"interactive";
    case Network:
        return L"network";
    case Batch:
        return L"batch";
    case Service:
        return L"service";
    case Proxy:
        return L"proxy";
    case Unlock:
        return L"unlock";
    case NetworkCleartext:
        return L"network_cleartext";
    case NewCredentials:
        return L"new_credentials";
    case RemoteInteractive:
        return L"remote_interactive";
    case CachedInteractive:
        return L"cached_interactive";
    case CachedRemoteInteractive:
        return L"cached_remote_interactive";
    case CachedUnlock:
        return L"cached_unlock";
    }

    return NULL;
}

VOID AtpAddLsaString(
    _In_ PVOID Row,
    _In_ PCSTR Key,
    _In_ PLSA_UNICODE_STRING String
    )
{
    PPH_STRING value;

    if (!String->Buffer || String->Length == 0)
    {
        AtJsonAddNull(Row, Key);
        return;
    }

    value = PhCreateStringEx(String->Buffer, String->Length);
    AtJsonAddString(Row, Key, value);
    PhDereferenceObject(value);
}

VOID AtpAddLogonSessionRow(
    _In_ PVOID Row,
    _In_ PSECURITY_LOGON_SESSION_DATA Data
    )
{
    static CONST ULONG userFlags[] =
    {
        LOGON_GUEST, LOGON_NOENCRYPTION, LOGON_CACHED_ACCOUNT, LOGON_EXTRA_SIDS,
        LOGON_NTLMV2_ENABLED, LOGON_PROFILE_PATH_RETURNED, LOGON_NT_V2, LOGON_LM_V2,
        LOGON_NTLM_V2, LOGON_OPTIMIZED, LOGON_WINLOGON, LOGON_PKINIT, LOGON_NO_OPTIMIZED,
        LOGON_NO_ELEVATION, LOGON_MANAGED_SERVICE,
    };
    static CONST PWSTR userFlagNames[] =
    {
        L"guest", L"no_encryption", L"cached_account", L"extra_sids",
        L"ntlmv2_enabled", L"profile_path_returned", L"nt_v2", L"lm_v2",
        L"ntlm_v2", L"optimized", L"winlogon", L"pkinit", L"no_optimized",
        L"no_elevation", L"managed_service",
    };
    PPH_STRING sid = NULL;

    AtJsonAddHex(Row, "logon_id", ((ULONG64)Data->LogonId.HighPart << 32) | Data->LogonId.LowPart);
    AtpAddLsaString(Row, "user_name", &Data->UserName);
    AtpAddLsaString(Row, "logon_domain", &Data->LogonDomain);
    AtpAddLsaString(Row, "authentication_package", &Data->AuthenticationPackage);

    // The structure grows between Windows versions and says how far it goes in its own Size, so
    // every field past the original set is only read once it is known to be there.
    if (RTL_CONTAINS_FIELD(Data, Data->Size, LogonType))
    {
        PhAddJsonObjectUInt64(Row, "logon_type_value", Data->LogonType);
        AtJsonAddStringZ(Row, "logon_type", AtpLogonTypeString(Data->LogonType));
    }
    else
    {
        AtJsonAddNull(Row, "logon_type_value");
        AtJsonAddNull(Row, "logon_type");
    }

    if (RTL_CONTAINS_FIELD(Data, Data->Size, Session))
        PhAddJsonObjectUInt64(Row, "session_id", Data->Session);
    else
        AtJsonAddNull(Row, "session_id");

    if (RTL_CONTAINS_FIELD(Data, Data->Size, Sid) && Data->Sid)
        sid = PhSidToStringSid(Data->Sid);

    AtJsonAddString(Row, "sid", sid);
    PhClearReference(&sid);

    if (RTL_CONTAINS_FIELD(Data, Data->Size, LogonTime))
        AtJsonAddTime(Row, "logon_time", &Data->LogonTime);
    else
        AtJsonAddNull(Row, "logon_time");

    if (RTL_CONTAINS_FIELD(Data, Data->Size, DnsDomainName))
    {
        AtpAddLsaString(Row, "logon_server", &Data->LogonServer);
        AtpAddLsaString(Row, "dns_domain_name", &Data->DnsDomainName);
        AtpAddLsaString(Row, "upn", &Data->Upn);
    }
    else
    {
        AtJsonAddNull(Row, "logon_server");
        AtJsonAddNull(Row, "dns_domain_name");
        AtJsonAddNull(Row, "upn");
    }

    if (RTL_CONTAINS_FIELD(Data, Data->Size, UserFlags))
    {
        AtJsonAddHex(Row, "user_flags_value", Data->UserFlags);
        AtJsonAddFlagStrings(Row, "user_flags", Data->UserFlags, userFlags, (CONST PWSTR*)userFlagNames, RTL_NUMBER_OF(userFlags));
    }
    else
    {
        AtJsonAddNull(Row, "user_flags_value");
        AtJsonAddNull(Row, "user_flags");
    }

    // What the account did before this: the failed attempts since the last success are the part
    // worth reading, because a session that follows a run of them is worth a second look.
    if (RTL_CONTAINS_FIELD(Data, Data->Size, LastLogonInfo))
    {
        PVOID entry = PhCreateJsonObject();

        AtJsonAddTime(entry, "last_successful_logon", &Data->LastLogonInfo.LastSuccessfulLogon);
        AtJsonAddTime(entry, "last_failed_logon", &Data->LastLogonInfo.LastFailedLogon);
        PhAddJsonObjectUInt64(entry, "failed_attempts_since_last_success", Data->LastLogonInfo.FailedAttemptCountSinceLastSuccessfulLogon);
        PhAddJsonObjectValue(Row, "last_logon_info", entry);
    }
    else
    {
        AtJsonAddNull(Row, "last_logon_info");
    }

    if (RTL_CONTAINS_FIELD(Data, Data->Size, LogonScript))
        AtpAddLsaString(Row, "logon_script", &Data->LogonScript);
    else
        AtJsonAddNull(Row, "logon_script");

    if (RTL_CONTAINS_FIELD(Data, Data->Size, HomeDirectory))
        AtpAddLsaString(Row, "home_directory", &Data->HomeDirectory);
    else
        AtJsonAddNull(Row, "home_directory");
}

VOID AtpListLogonSessions(
    _In_ PAT_TOOL_CALL Call,
    _Inout_ PAT_TOOL_RESULT Result
    )
{
    NTSTATUS status;
    AT_ROWS rows;
    PLUID logonSessions = NULL;
    ULONG logonSessionCount = 0;
    PPH_STRING userContains;
    PPH_STRING logonTypeFilter;
    ULONG64 sessionFilter = 0;
    BOOLEAN haveSessionFilter;
    ULONG unreadableCount = 0;
    PVOID structured;
    ULONG i;

    if (!AtpInitializeLsaRoutines())
    {
        AtSetToolError(Result, "failed", STATUS_NOINTERFACE, L"The logon session routines could not be located in secur32.dll.");
        return;
    }

    status = AtpLsaEnumerateLogonSessions(&logonSessionCount, &logonSessions);

    if (!NT_SUCCESS(status))
    {
        AtSetToolStatusError(Result, status, L"Enumerating the logon sessions");
        return;
    }

    userContains = AtGetArgumentString(Call->Arguments, "user_contains");
    logonTypeFilter = AtGetArgumentString(Call->Arguments, "logon_type");
    haveSessionFilter = AtGetArgumentUInt64(Call->Arguments, "session_id", &sessionFilter);

    AtInitializeRows(&rows, Call->Arguments);

    for (i = 0; i < logonSessionCount; i++)
    {
        PSECURITY_LOGON_SESSION_DATA data;
        PPH_STRING userName = NULL;
        PVOID row;

        // Two reasons this fails, and both matter. The session can have ended between the
        // enumeration and the query, and - far more often - a caller that is not elevated is
        // refused the sessions belonging to anybody else, including every service and SYSTEM
        // logon. Counted rather than passed over, because the difference between "these are the
        // sessions" and "these are the sessions I was allowed to read" is the whole answer.
        if (!NT_SUCCESS(AtpLsaGetLogonSessionData(&logonSessions[i], &data)))
        {
            unreadableCount++;
            continue;
        }

        if (data->UserName.Buffer && data->UserName.Length)
            userName = PhCreateStringEx(data->UserName.Buffer, data->UserName.Length);

        if (userContains && !AtContainsString(userName, userContains))
            goto NextSession;

        if (logonTypeFilter && RTL_CONTAINS_FIELD(data, data->Size, LogonType))
        {
            PCWSTR name = AtpLogonTypeString(data->LogonType);

            if (!name || !PhEqualString2(logonTypeFilter, name, TRUE))
                goto NextSession;
        }
        else if (logonTypeFilter)
        {
            goto NextSession;
        }

        if (haveSessionFilter &&
            (!RTL_CONTAINS_FIELD(data, data->Size, Session) || data->Session != (ULONG)sessionFilter))
        {
            goto NextSession;
        }

        row = PhCreateJsonObject();
        AtpAddLogonSessionRow(row, data);
        AtAddRow(&rows, row);

NextSession:
        PhClearReference(&userName);
        AtpLsaFreeReturnBuffer(data);
    }

    AtpLsaFreeReturnBuffer(logonSessions);

    structured = PhCreateJsonObject();
    AtAddRows(structured, "sessions", &rows);
    PhAddJsonObjectUInt64(structured, "enumerated_count", logonSessionCount);
    PhAddJsonObjectUInt64(structured, "unreadable_count", unreadableCount);
    PhAddJsonObjectBoolean(structured, "elevated", !!PhGetOwnTokenAttributes().Elevated);
    AtAddSnapshot(structured);

    Result->StructuredContent = structured;

    AtDeleteRows(&rows);
    PhClearReference(&userContains);
    PhClearReference(&logonTypeFilter);
}

// The terminal services sessions the machine has. Every process belongs to one, and a session is
// where a desktop lives: session 0 holds the services and no desktop at all, and every interactive
// user gets one of their own. A disconnected session is the interesting shape - somebody logged on,
// their programs are still running, and nobody is looking at the screen - and a session whose
// client is a remote address is somebody who arrived over the network.

PCWSTR AtpWinStationStateString(
    _In_ WINSTATIONSTATECLASS State
    )
{
    switch (State)
    {
    case State_Active:
        return L"active";
    case State_Connected:
        return L"connected";
    case State_ConnectQuery:
        return L"connect_query";
    case State_Shadow:
        return L"shadow";
    case State_Disconnected:
        return L"disconnected";
    case State_Idle:
        return L"idle";
    case State_Listen:
        return L"listen";
    case State_Reset:
        return L"reset";
    case State_Down:
        return L"down";
    case State_Init:
        return L"init";
    }

    return NULL;
}

VOID AtpAddWinStationTime(
    _In_ PVOID Row,
    _In_ PCSTR Key,
    _In_ PLARGE_INTEGER Time
    )
{
    // A session that has never connected, or never been disconnected, carries a zero here rather
    // than a time, and 1601 is not an answer to when it happened.
    if (Time->QuadPart == 0)
        AtJsonAddNull(Row, Key);
    else
        AtJsonAddTime(Row, Key, Time);
}

VOID AtpAddTerminalSessionDetails(
    _In_ PVOID Row,
    _In_ ULONG SessionId
    )
{
    WINSTATIONINFORMATION information;
    ULONG returnLength;

    memset(&information, 0, sizeof(WINSTATIONINFORMATION));

    if (!WinStationQueryInformationW(
        WINSTATION_CURRENT_SERVER,
        SessionId,
        WinStationInformation,
        &information,
        sizeof(WINSTATIONINFORMATION),
        &returnLength
        ))
    {
        AtJsonAddNull(Row, "user_name");
        AtJsonAddNull(Row, "domain");
        AtJsonAddNull(Row, "logon_time");
        AtJsonAddNull(Row, "connect_time");
        AtJsonAddNull(Row, "disconnect_time");
        AtJsonAddNull(Row, "last_input_time");
        AtJsonAddNull(Row, "idle_seconds");
        AtJsonAddNull(Row, "bytes_sent");
        AtJsonAddNull(Row, "bytes_received");
        return;
    }

    AtJsonAddStringZ(Row, "user_name", information.UserName[0] ? information.UserName : NULL);
    AtJsonAddStringZ(Row, "domain", information.Domain[0] ? information.Domain : NULL);
    AtpAddWinStationTime(Row, "logon_time", &information.LogonTime);
    AtpAddWinStationTime(Row, "connect_time", &information.ConnectTime);
    AtpAddWinStationTime(Row, "disconnect_time", &information.DisconnectTime);
    AtpAddWinStationTime(Row, "last_input_time", &information.LastInputTime);

    // How long since anybody touched it, worked out against the session's own clock rather than
    // this process's, because the two are the same machine but the API hands both back.
    if (information.LastInputTime.QuadPart != 0 && information.CurrentTime.QuadPart != 0)
        AtJsonAddDuration(Row, "idle_seconds", information.CurrentTime.QuadPart - information.LastInputTime.QuadPart);
    else
        AtJsonAddNull(Row, "idle_seconds");

    PhAddJsonObjectUInt64(Row, "bytes_sent", information.Status.Output.Bytes);
    PhAddJsonObjectUInt64(Row, "bytes_received", information.Status.Input.Bytes);
}

VOID AtpListTerminalSessions(
    _In_ PAT_TOOL_CALL Call,
    _Inout_ PAT_TOOL_RESULT Result
    )
{
    PSESSIONIDW sessions = NULL;
    ULONG sessionCount = 0;
    AT_ROWS rows;
    PPH_STRING stateFilter;
    PVOID structured;
    ULONG i;

    if (!WinStationEnumerateW(WINSTATION_CURRENT_SERVER, &sessions, &sessionCount))
    {
        AtSetToolStatusError(Result, PhGetLastWin32ErrorAsNtStatus(), L"Enumerating the sessions");
        return;
    }

    stateFilter = AtGetArgumentString(Call->Arguments, "state");

    AtInitializeRows(&rows, Call->Arguments);

    for (i = 0; i < sessionCount; i++)
    {
        PCWSTR state = AtpWinStationStateString(sessions[i].State);
        PVOID row;

        if (stateFilter && (!state || !PhEqualString2(stateFilter, state, TRUE)))
            continue;

        row = PhCreateJsonObject();
        PhAddJsonObjectUInt64(row, "session_id", sessions[i].SessionId);
        AtJsonAddStringZ(row, "name", sessions[i].WinStationName[0] ? sessions[i].WinStationName : NULL);
        AtJsonAddStringZ(row, "state", state);
        PhAddJsonObjectUInt64(row, "state_value", sessions[i].State);
        AtpAddTerminalSessionDetails(row, sessions[i].SessionId);
        AtAddRow(&rows, row);
    }

    WinStationFreeMemory(sessions);

    structured = PhCreateJsonObject();
    AtAddRows(structured, "sessions", &rows);
    PhAddJsonObjectUInt64(structured, "current_session_id", NtCurrentPeb()->SessionId);
    AtAddSnapshot(structured);

    Result->StructuredContent = structured;

    AtDeleteRows(&rows);
    PhClearReference(&stateFilter);
}

VOID AtSessionInvokeTool(
    _In_ PCAT_TOOL Tool,
    _In_ PAT_TOOL_CALL Call,
    _Inout_ PAT_TARGET Target,
    _Inout_ PAT_TOOL_RESULT Result
    )
{
    UNREFERENCED_PARAMETER(Target);

    switch (Tool->Action)
    {
    case AtActionListLogonSessions:
        AtpListLogonSessions(Call, Result);
        break;
    case AtActionListTerminalSessions:
        AtpListTerminalSessions(Call, Result);
        break;
    default:
        AtSetToolError(Result, "failed", STATUS_NOT_IMPLEMENTED, L"This tool is not implemented.");
        break;
    }
}
