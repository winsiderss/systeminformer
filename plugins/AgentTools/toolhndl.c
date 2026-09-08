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

// What one handle really refers to, when the handle list can only give a name. ALPC is the transport
// almost every RPC call on Windows rides on, and an ALPC port handle by itself says nothing about who
// is on the other end - the kernel knows, and the System Informer driver is the only way to ask.

PCWSTR AtpAlpcPortTypeString(
    _In_ ULONG State
    )
{
    switch ((State >> 1) & 0x3)
    {
    case 1:
        return L"server_connection";
    case 2:
        return L"client_communication";
    case 3:
        return L"server_communication";
    }

    return L"unconnected";
}

// One of the three ports in a communication triple, or the port the handle itself refers to. The
// owner is the answer people come here for: it is the process on the other end.
PVOID AtpCreateAlpcPortObject(
    _In_ PKPH_ALPC_BASIC_INFORMATION Basic,
    _In_opt_ PUNICODE_STRING Name
    )
{
    static CONST ULONG portFlags[] =
    {
        ALPC_PORFLG_LPC_MODE, ALPC_PORFLG_ALLOW_IMPERSONATION, ALPC_PORFLG_ALLOW_LPC_REQUESTS,
        ALPC_PORFLG_WAITABLE_PORT, ALPC_PORFLG_ALLOW_DUP_OBJECT, ALPC_PORFLG_SYSTEM_PROCESS,
        ALPC_PORFLG_WAKE_POLICY1, ALPC_PORFLG_WAKE_POLICY2, ALPC_PORFLG_WAKE_POLICY3,
        ALPC_PORFLG_DIRECT_MESSAGE, ALPC_PORFLG_ALLOW_MULTIHANDLE_ATTRIBUTE
    };
    static CONST PWSTR portFlagNames[] =
    {
        L"lpc_mode", L"allow_impersonation", L"allow_lpc_requests",
        L"waitable", L"allow_dup_object", L"system_process_only",
        L"wake_policy1", L"wake_policy2", L"wake_policy3",
        L"direct_message", L"allow_multihandle_attribute"
    };
    // The state word carries the port type in bits 1-2; every other bit is a flag.
    static CONST ULONG stateFlags[] =
    {
        0x00001, 0x00008, 0x00010, 0x00020, 0x00040, 0x00080, 0x00100, 0x00200,
        0x00400, 0x00800, 0x01000, 0x02000, 0x04000, 0x08000, 0x10000
    };
    static CONST PWSTR stateFlagNames[] =
    {
        L"initialized", L"connection_pending", L"connection_refused", L"disconnected", L"closed",
        L"no_flush_on_close", L"return_extended_info", L"waitable", L"dynamic_security",
        L"wow64_completion_list", L"lpc", L"lpc_to_lpc", L"has_completion_list",
        L"had_completion_list", L"enable_completion_list"
    };
    PVOID object;

    object = PhCreateJsonObject();

    if (Basic->OwnerProcessId)
    {
        PVOID owner = PhCreateJsonObject();
        PPH_PROCESS_ITEM processItem;

        if (processItem = PhReferenceProcessItem(Basic->OwnerProcessId))
        {
            AtFillProcessIdentity(owner, processItem);
            PhDereferenceObject(processItem);
        }
        else
        {
            PhAddJsonObjectUInt64(owner, "pid", HandleToUlong(Basic->OwnerProcessId));
        }

        PhAddJsonObjectValue(object, "owner", owner);
    }
    else
    {
        AtJsonAddNull(object, "owner");
    }

    if (Name && Name->Length != 0)
    {
        PH_STRINGREF name;

        PhUnicodeStringToStringRef(Name, &name);
        AtJsonAddStringRef(object, "name", &name);
    }
    else
    {
        AtJsonAddNull(object, "name");
    }

    AtJsonAddStringZ(object, "port_type", AtpAlpcPortTypeString(Basic->State));
    AtJsonAddHex(object, "state", Basic->State);
    AtJsonAddFlagStrings(object, "state_flags", Basic->State,
        stateFlags, (CONST PWSTR*)stateFlagNames, RTL_NUMBER_OF(stateFlags));
    AtJsonAddHex(object, "flags", Basic->Flags);
    AtJsonAddFlagStrings(object, "flag_names", Basic->Flags,
        portFlags, (CONST PWSTR*)portFlagNames, RTL_NUMBER_OF(portFlags));
    PhAddJsonObjectInt64(object, "sequence_number", Basic->SequenceNo);
    AtJsonAddPointer(object, "port_context", Basic->PortContext);

    return object;
}

VOID AtpAddAlpcPeer(
    _In_ PVOID Structured,
    _In_ PCSTR Key,
    _In_ PKPH_ALPC_BASIC_INFORMATION Basic,
    _In_opt_ PUNICODE_STRING Name
    )
{
    // A port the kernel left unset is not a peer with no owner, it is not part of this connection.
    if (Basic->OwnerProcessId || (Name && Name->Length != 0))
        PhAddJsonObjectValue(Structured, Key, AtpCreateAlpcPortObject(Basic, Name));
    else
        AtJsonAddNull(Structured, Key);
}

VOID AtpGetAlpcPortInfo(
    _In_ PAT_TOOL_CALL Call,
    _Inout_ PAT_TOOL_RESULT Result
    )
{
    NTSTATUS status;
    AT_TARGET target;
    KPH_ALPC_BASIC_INFORMATION basicInfo;
    KPH_ALPC_COMMUNICATION_INFORMATION communicationInfo;
    PKPH_ALPC_COMMUNICATION_NAMES_INFORMATION names = NULL;
    PVOID structured;

    // A plain read is handed no target: mcp.c resolves one only for the tiers that hold it open
    // across a confirmation. The sequence number stays optional, as it is on every other read.
    if (!NT_SUCCESS(AtResolveHandleTarget(Call->Arguments, FALSE, PROCESS_QUERY_LIMITED_INFORMATION, &target, Result)))
        return;

    if (!target.HandleTypeName || !PhEqualString2(target.HandleTypeName, L"ALPC Port", TRUE))
    {
        AtSetToolError(
            Result,
            "identity_mismatch",
            STATUS_OBJECT_TYPE_MISMATCH,
            L"Handle 0x%llx in pid %lu is a %s handle, not an ALPC Port.",
            (ULONG64)(ULONG_PTR)target.HandleValue,
            HandleToUlong(target.ProcessItem->ProcessId),
            PhGetStringOrDefault(target.HandleTypeName, L"(unknown type)")
            );
        AtDeleteTarget(&target);
        return;
    }

    // Nothing here is reachable without the driver: the port objects live in kernel memory and no
    // user-mode call reports them.
    if (KsiLevel() < KphLevelMed)
    {
        AtSetToolError(
            Result,
            "failed",
            STATUS_NOT_SUPPORTED,
            L"ALPC port information comes from the System Informer driver, which is not available to this instance (access level: %s).",
            AtKphLevelString(KsiLevel())
            );
        AtSetToolHint(Result, AT_HINT_NEEDS_DRIVER);
        AtDeleteTarget(&target);
        return;
    }

    status = KphAlpcQueryInformation(
        target.ProcessHandle,
        target.HandleValue,
        KphAlpcBasicInformation,
        &basicInfo,
        sizeof(basicInfo),
        NULL
        );

    if (!NT_SUCCESS(status))
    {
        AtSetToolStatusError(Result, status, L"Querying the ALPC port");
        AtDeleteTarget(&target);
        return;
    }

    memset(&communicationInfo, 0, sizeof(communicationInfo));

    if (!NT_SUCCESS(KphAlpcQueryInformation(
        target.ProcessHandle,
        target.HandleValue,
        KphAlpcCommunicationInformation,
        &communicationInfo,
        sizeof(communicationInfo),
        NULL
        )))
    {
        // An unconnected port has no triple; the basic information is still worth returning.
        memset(&communicationInfo, 0, sizeof(communicationInfo));
    }

    if (!NT_SUCCESS(KphAlpcQueryCommunicationsNamesInfo(
        target.ProcessHandle,
        target.HandleValue,
        &names
        )))
    {
        names = NULL;
    }

    structured = PhCreateJsonObject();
    PhAddJsonObjectUInt64(structured, "pid", HandleToUlong(target.ProcessItem->ProcessId));
    AtJsonAddString(structured, "process_name", target.ProcessItem->ProcessName);
    AtJsonAddPointer(structured, "handle", target.HandleValue);
    AtJsonAddString(structured, "object_name", target.HandleObjectName);

    PhAddJsonObjectValue(structured, "port", AtpCreateAlpcPortObject(&basicInfo, NULL));

    AtpAddAlpcPeer(structured, "connection_port", &communicationInfo.ConnectionPort,
        names ? &names->ConnectionPort : NULL);
    AtpAddAlpcPeer(structured, "server_communication_port", &communicationInfo.ServerCommunicationPort,
        names ? &names->ServerCommunicationPort : NULL);
    AtpAddAlpcPeer(structured, "client_communication_port", &communicationInfo.ClientCommunicationPort,
        names ? &names->ClientCommunicationPort : NULL);

    AtAddSnapshot(structured);
    Result->StructuredContent = structured;

    if (names)
        PhFree(names);

    AtDeleteTarget(&target);
}

VOID AtHandleInvokeTool(
    _In_ PCAT_TOOL Tool,
    _In_ PAT_TOOL_CALL Call,
    _Inout_ PAT_TARGET Target,
    _Inout_ PAT_TOOL_RESULT Result
    )
{
    switch (Tool->Action)
    {
    case AtActionGetAlpcPortInfo:
        AtpGetAlpcPortInfo(Call, Result);
        break;
    default:
        AtSetToolError(Result, "failed", STATUS_NOT_IMPLEMENTED, L"Unhandled tool.");
        break;
    }
}
