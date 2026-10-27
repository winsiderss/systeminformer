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
#include <mapldr.h>

#include <fwpmu.h>

#define AT_FWP_DIRECTION_MAP_INBOUND 0x3900
#define AT_FWP_DIRECTION_MAP_OUTBOUND 0x3901
#define AT_FWP_DIRECTION_MAP_FORWARD 0x3902
#define AT_FWP_DIRECTION_MAP_BIDIRECTIONAL 0x3903

static PVOID AtpFwpuclntBaseAddress = NULL;
static ULONG (WINAPI *AtpFwpmEngineOpen)(PCWSTR, ULONG, PSEC_WINNT_AUTH_IDENTITY_W, const FWPM_SESSION0*, HANDLE*) = NULL;
static ULONG (WINAPI *AtpFwpmEngineClose)(HANDLE) = NULL;
static ULONG (WINAPI *AtpFwpmEngineGetOption)(HANDLE, FWPM_ENGINE_OPTION, FWP_VALUE0**) = NULL;
static ULONG (WINAPI *AtpFwpmFreeMemory)(VOID**) = NULL;
static ULONG (WINAPI *AtpFwpmNetEventCreateEnumHandle)(HANDLE, const FWPM_NET_EVENT_ENUM_TEMPLATE0*, HANDLE*) = NULL;
static ULONG (WINAPI *AtpFwpmNetEventDestroyEnumHandle)(HANDLE, HANDLE) = NULL;
static ULONG (WINAPI *AtpFwpmNetEventEnum)(HANDLE, HANDLE, ULONG, FWPM_NET_EVENT***, ULONG*) = NULL;

BOOLEAN AtpInitializeFirewall(
    VOID
    )
{
    static PH_INITONCE initOnce = PH_INITONCE_INIT;

    if (PhBeginInitOnce(&initOnce))
    {
        if (AtpFwpuclntBaseAddress = PhLoadLibrary(L"fwpuclnt.dll"))
        {
            AtpFwpmEngineOpen = PhGetProcedureAddress(AtpFwpuclntBaseAddress, "FwpmEngineOpen0", 0);
            AtpFwpmEngineClose = PhGetProcedureAddress(AtpFwpuclntBaseAddress, "FwpmEngineClose0", 0);
            AtpFwpmEngineGetOption = PhGetProcedureAddress(AtpFwpuclntBaseAddress, "FwpmEngineGetOption0", 0);
            AtpFwpmFreeMemory = PhGetProcedureAddress(AtpFwpuclntBaseAddress, "FwpmFreeMemory0", 0);
            AtpFwpmNetEventCreateEnumHandle = PhGetProcedureAddress(AtpFwpuclntBaseAddress, "FwpmNetEventCreateEnumHandle0", 0);
            AtpFwpmNetEventDestroyEnumHandle = PhGetProcedureAddress(AtpFwpuclntBaseAddress, "FwpmNetEventDestroyEnumHandle0", 0);

            // The newest enumeration the platform offers, and no lower than 3. The events are read
            // as FWPM_NET_EVENT, whose header is FWPM_NET_EVENT_HEADER3; versions 3, 4 and 5 all
            // carry that header, while 2, 1 and 0 carry HEADER2, HEADER1 and HEADER0, which are
            // laid out differently. Falling back to those would read one structure as another.
            AtpFwpmNetEventEnum = PhGetProcedureAddress(AtpFwpuclntBaseAddress, "FwpmNetEventEnum5", 0);
            if (!AtpFwpmNetEventEnum)
                AtpFwpmNetEventEnum = PhGetProcedureAddress(AtpFwpuclntBaseAddress, "FwpmNetEventEnum4", 0);
            if (!AtpFwpmNetEventEnum)
                AtpFwpmNetEventEnum = PhGetProcedureAddress(AtpFwpuclntBaseAddress, "FwpmNetEventEnum3", 0);
        }

        PhEndInitOnce(&initOnce);
    }

    return AtpFwpmEngineOpen && AtpFwpmEngineClose && AtpFwpmFreeMemory &&
        AtpFwpmNetEventCreateEnumHandle && AtpFwpmNetEventDestroyEnumHandle && AtpFwpmNetEventEnum;
}

PCSTR AtpFirewallEventTypeString(
    _In_ ULONG Type
    )
{
    switch (Type)
    {
    case FWPM_NET_EVENT_TYPE_IKEEXT_MM_FAILURE:
        return "ikeext_main_mode_failure";
    case FWPM_NET_EVENT_TYPE_IKEEXT_QM_FAILURE:
        return "ikeext_quick_mode_failure";
    case FWPM_NET_EVENT_TYPE_IKEEXT_EM_FAILURE:
        return "ikeext_extended_mode_failure";
    case FWPM_NET_EVENT_TYPE_CLASSIFY_DROP:
        return "classify_drop";
    case FWPM_NET_EVENT_TYPE_IPSEC_KERNEL_DROP:
        return "ipsec_kernel_drop";
    case FWPM_NET_EVENT_TYPE_IPSEC_DOSP_DROP:
        return "ipsec_dosp_drop";
    case FWPM_NET_EVENT_TYPE_CLASSIFY_ALLOW:
        return "classify_allow";
    case FWPM_NET_EVENT_TYPE_CAPABILITY_DROP:
        return "capability_drop";
    case FWPM_NET_EVENT_TYPE_CAPABILITY_ALLOW:
        return "capability_allow";
    case FWPM_NET_EVENT_TYPE_CLASSIFY_DROP_MAC:
        return "classify_drop_mac";
    case FWPM_NET_EVENT_TYPE_LPM_PACKET_ARRIVAL:
        return "lpm_packet_arrival";
    }

    return "unknown";
}

PCSTR AtpFirewallDirectionString(
    _In_ ULONG Direction
    )
{
    switch (Direction)
    {
    case FWP_DIRECTION_INBOUND:
    case AT_FWP_DIRECTION_MAP_INBOUND:
        return "inbound";
    case FWP_DIRECTION_OUTBOUND:
    case AT_FWP_DIRECTION_MAP_OUTBOUND:
        return "outbound";
    case AT_FWP_DIRECTION_MAP_FORWARD:
        return "forward";
    case AT_FWP_DIRECTION_MAP_BIDIRECTIONAL:
        return "bidirectional";
    }

    return "unknown";
}

BOOLEAN AtpFirewallEventDetail(
    _In_ const FWPM_NET_EVENT* Event,
    _Out_ PULONG Direction,
    _Out_ PULONG64 FilterId,
    _Out_ PUSHORT LayerId,
    _Out_ PBOOLEAN IsLoopback
    )
{
    *Direction = ULONG_MAX;
    *FilterId = 0;
    *LayerId = 0;
    *IsLoopback = FALSE;

    switch ((ULONG)Event->type)
    {
    case FWPM_NET_EVENT_TYPE_CLASSIFY_DROP:
        if (!Event->classifyDrop)
            return FALSE;
        *Direction = Event->classifyDrop->msFwpDirection;
        *FilterId = Event->classifyDrop->filterId;
        *LayerId = Event->classifyDrop->layerId;
        *IsLoopback = !!Event->classifyDrop->isLoopback;
        return TRUE;
    case FWPM_NET_EVENT_TYPE_CLASSIFY_ALLOW:
        if (!Event->classifyAllow)
            return FALSE;
        *Direction = Event->classifyAllow->msFwpDirection;
        *FilterId = Event->classifyAllow->filterId;
        *LayerId = Event->classifyAllow->layerId;
        *IsLoopback = !!Event->classifyAllow->isLoopback;
        return TRUE;
    case FWPM_NET_EVENT_TYPE_CAPABILITY_DROP:
        if (!Event->capabilityDrop)
            return FALSE;
        *FilterId = Event->capabilityDrop->filterId;
        *IsLoopback = !!Event->capabilityDrop->isLoopback;
        return TRUE;
    case FWPM_NET_EVENT_TYPE_CAPABILITY_ALLOW:
        if (!Event->capabilityAllow)
            return FALSE;
        *FilterId = Event->capabilityAllow->filterId;
        *IsLoopback = !!Event->capabilityAllow->isLoopback;
        return TRUE;
    }

    return FALSE;
}

VOID AtpAddFirewallAddress(
    _In_ PVOID Row,
    _In_ PCSTR AddressKey,
    _In_ PCSTR PortKey,
    _In_ ULONG Address,
    _In_reads_bytes_(16) const UCHAR* Address6,
    _In_ USHORT Port,
    _In_ ULONG Version,
    _In_ BOOLEAN HaveAddress,
    _In_ BOOLEAN HavePort
    )
{
    if (HaveAddress)
    {
        WCHAR buffer[65];
        ULONG length = RTL_NUMBER_OF(buffer);
        PPH_STRING string = NULL;

        if (Version == FWP_IP_VERSION_V4)
        {
            IN_ADDR value;

            // The platform reports the v4 address in host order.
            value.s_addr = _byteswap_ulong(Address);

            if (NT_SUCCESS(PhIpv4AddressToString(&value, 0, buffer, &length)))
                string = PhCreateStringEx(buffer, (length - 1) * sizeof(WCHAR));
        }
        else
        {
            IN6_ADDR value;

            memcpy(&value, Address6, sizeof(IN6_ADDR));

            if (NT_SUCCESS(PhIpv6AddressToString(&value, 0, 0, buffer, &length)))
                string = PhCreateStringEx(buffer, (length - 1) * sizeof(WCHAR));
        }

        AtJsonAddString(Row, AddressKey, string);
        PhClearReference(&string);
    }
    else
    {
        AtJsonAddNull(Row, AddressKey);
    }

    if (HavePort)
        PhAddJsonObjectUInt64(Row, PortKey, Port);
    else
        AtJsonAddNull(Row, PortKey);
}

VOID AtpAddFirewallEvent(
    _In_ PAT_ROWS Rows,
    _In_ const FWPM_NET_EVENT* Event,
    _In_opt_ PPH_STRING PathContains,
    _In_ BOOLEAN DropsOnly
    )
{
    const FWPM_NET_EVENT_HEADER3* header = &Event->header;
    PVOID row;
    ULONG direction;
    ULONG64 filterId;
    USHORT layerId;
    BOOLEAN isLoopback;
    BOOLEAN haveDetail;
    PPH_STRING applicationPath = NULL;
    LARGE_INTEGER timeStamp;
    PCSTR typeName;

    typeName = AtpFirewallEventTypeString(Event->type);

    if (DropsOnly && !PhEqualBytesZ(typeName, "classify_drop", TRUE) &&
        !PhEqualBytesZ(typeName, "capability_drop", TRUE) &&
        !PhEqualBytesZ(typeName, "ipsec_kernel_drop", TRUE) &&
        !PhEqualBytesZ(typeName, "ipsec_dosp_drop", TRUE) &&
        !PhEqualBytesZ(typeName, "classify_drop_mac", TRUE))
    {
        return;
    }

    // The application is a device path in a counted buffer whose size may or may not include the
    // terminator, and is not guaranteed to be a whole number of characters.
    if (FlagOn(header->flags, FWPM_NET_EVENT_FLAG_APP_ID_SET) && header->appId.data && header->appId.size)
    {
        SIZE_T dataLength = header->appId.size & ~(sizeof(WCHAR) - 1);

        if (dataLength >= sizeof(WCHAR) &&
            *(PWCHAR)PTR_ADD_OFFSET(header->appId.data, dataLength - sizeof(WCHAR)) == UNICODE_NULL)
        {
            dataLength -= sizeof(WCHAR);
        }

        if (dataLength)
            applicationPath = PhCreateStringEx((PWCHAR)header->appId.data, dataLength);
    }

    if (PathContains && !AtContainsString(applicationPath, PathContains))
    {
        PhClearReference(&applicationPath);
        return;
    }

    haveDetail = AtpFirewallEventDetail(Event, &direction, &filterId, &layerId, &isLoopback);

    row = PhCreateJsonObject();

    timeStamp.HighPart = header->timeStamp.dwHighDateTime;
    timeStamp.LowPart = header->timeStamp.dwLowDateTime;
    AtJsonAddTime(row, "time", &timeStamp);
    PhAddJsonObject(row, "type", typeName);

    AtJsonAddWin32FileName(row, "application", applicationPath);

    if (FlagOn(header->flags, FWPM_NET_EVENT_FLAG_IP_PROTOCOL_SET))
        PhAddJsonObjectUInt64(row, "ip_protocol", header->ipProtocol);
    else
        AtJsonAddNull(row, "ip_protocol");

    {
        BOOLEAN version4 = !FlagOn(header->flags, FWPM_NET_EVENT_FLAG_IP_VERSION_SET) ||
            header->ipVersion == FWP_IP_VERSION_V4;

        AtpAddFirewallAddress(
            row,
            "local_address",
            "local_port",
            header->localAddrV4,
            header->localAddrV6.byteArray16,
            header->localPort,
            version4 ? FWP_IP_VERSION_V4 : FWP_IP_VERSION_V6,
            FlagOn(header->flags, FWPM_NET_EVENT_FLAG_IP_VERSION_SET) && FlagOn(header->flags, FWPM_NET_EVENT_FLAG_LOCAL_ADDR_SET),
            !!FlagOn(header->flags, FWPM_NET_EVENT_FLAG_LOCAL_PORT_SET)
            );

        AtpAddFirewallAddress(
            row,
            "remote_address",
            "remote_port",
            header->remoteAddrV4,
            header->remoteAddrV6.byteArray16,
            header->remotePort,
            version4 ? FWP_IP_VERSION_V4 : FWP_IP_VERSION_V6,
            FlagOn(header->flags, FWPM_NET_EVENT_FLAG_IP_VERSION_SET) && FlagOn(header->flags, FWPM_NET_EVENT_FLAG_REMOTE_ADDR_SET),
            !!FlagOn(header->flags, FWPM_NET_EVENT_FLAG_REMOTE_PORT_SET)
            );
    }

    if (FlagOn(header->flags, FWPM_NET_EVENT_FLAG_USER_ID_SET) && header->userId)
    {
        PPH_STRING sid = PhSidToStringSid(header->userId);

        AtJsonAddString(row, "user_sid", sid);
        PhClearReference(&sid);
    }
    else
    {
        AtJsonAddNull(row, "user_sid");
    }

    if (haveDetail)
    {
        PhAddJsonObject(row, "direction", AtpFirewallDirectionString(direction));
        PhAddJsonObjectUInt64(row, "filter_id", filterId);
        PhAddJsonObjectUInt64(row, "layer_id", layerId);
        PhAddJsonObjectBoolean(row, "is_loopback", isLoopback);
    }
    else
    {
        // An IPsec negotiation failure is not about a packet, so it has no direction or filter.
        AtJsonAddNull(row, "direction");
        AtJsonAddNull(row, "filter_id");
        AtJsonAddNull(row, "layer_id");
        AtJsonAddNull(row, "is_loopback");
    }

    AtAddRow(Rows, row);

    PhClearReference(&applicationPath);
}

VOID AtpListFirewallEvents(
    _In_ PAT_TOOL_CALL Call,
    _Inout_ PAT_TOOL_RESULT Result
    )
{
    HANDLE engineHandle = NULL;
    HANDLE enumHandle = NULL;
    FWPM_NET_EVENT_ENUM_TEMPLATE enumTemplate;
    FWP_VALUE* collecting = NULL;
    PPH_STRING pathContains;
    BOOLEAN dropsOnly;
    BOOLEAN collectionEnabled = FALSE;
    AT_ROWS rows;
    PVOID structured;
    PVOID collector;
    ULONG status;
    BOOLEAN enumComplete = TRUE;

    if (!AtpInitializeFirewall())
    {
        AtSetToolError(Result, "failed", STATUS_NOT_SUPPORTED, L"The filtering platform library is not available.");
        return;
    }

    status = AtpFwpmEngineOpen(NULL, RPC_C_AUTHN_DEFAULT, NULL, NULL, &engineHandle);

    if (status != ERROR_SUCCESS)
    {
        AtSetToolStatusError(Result, PhDosErrorToNtStatus(status), L"Opening the filtering engine");
        return;
    }

    pathContains = AtGetArgumentString(Call->Arguments, "application_contains");
    dropsOnly = AtJsonGetObjectBoolean(Call->Arguments, "drops_only");

    // This does not turn collection on: the switch is machine-wide. An empty list from a machine
    // that is not collecting must not read as no firewall activity.
    if (AtpFwpmEngineGetOption &&
        AtpFwpmEngineGetOption(engineHandle, FWPM_ENGINE_COLLECT_NET_EVENTS, &collecting) == ERROR_SUCCESS &&
        collecting)
    {
        collectionEnabled = collecting->type == FWP_UINT32 && !!collecting->uint32;
        AtpFwpmFreeMemory((VOID**)&collecting);
    }

    memset(&enumTemplate, 0, sizeof(FWPM_NET_EVENT_ENUM_TEMPLATE));
    status = AtpFwpmNetEventCreateEnumHandle(engineHandle, &enumTemplate, &enumHandle);

    if (status != ERROR_SUCCESS)
    {
        AtSetToolStatusError(Result, PhDosErrorToNtStatus(status), L"Enumerating the firewall events");
        AtpFwpmEngineClose(engineHandle);
        PhClearReference(&pathContains);
        return;
    }

    structured = PhCreateJsonObject();
    AtInitializeRows(&rows, Call->Arguments);

    while (TRUE)
    {
        FWPM_NET_EVENT** entries;
        ULONG count = 0;
        ULONG i;

        status = AtpFwpmNetEventEnum(engineHandle, enumHandle, ULONG_MAX, &entries, &count);

        if (status != ERROR_SUCCESS)
        {
            enumComplete = FALSE;
            break;
        }

        if (count == 0)
        {
            AtpFwpmFreeMemory((VOID**)&entries);
            break;
        }

        for (i = 0; i < count; i++)
            AtpAddFirewallEvent(&rows, entries[i], pathContains, dropsOnly);

        AtpFwpmFreeMemory((VOID**)&entries);
    }

    AtpFwpmNetEventDestroyEnumHandle(engineHandle, enumHandle);
    AtpFwpmEngineClose(engineHandle);

    // Nothing was read at all, so there is no partial answer worth returning.
    if (!enumComplete && rows.TotalCount == 0)
    {
        AtSetToolStatusError(Result, PhDosErrorToNtStatus(status), L"Enumerating the firewall events");
        PhFreeJsonObject(structured);
        AtDeleteRows(&rows);
        PhClearReference(&pathContains);
        return;
    }

    AtAddRows(structured, "events", &rows);

    collector = PhCreateJsonObject();
    PhAddJsonObjectBoolean(collector, "collection_enabled", collectionEnabled);
    PhAddJsonObjectBoolean(collector, "enumeration_complete", enumComplete);
    PhAddJsonObjectValue(structured, "collector", collector);

    AtAddSnapshot(structured);

    Result->StructuredContent = structured;

    AtDeleteRows(&rows);
    PhClearReference(&pathContains);
}

VOID AtFirewallInvokeTool(
    _In_ PCAT_TOOL Tool,
    _In_ PAT_TOOL_CALL Call,
    _Inout_ PAT_TARGET Target,
    _Inout_ PAT_TOOL_RESULT Result
    )
{
    UNREFERENCED_PARAMETER(Target);

    switch (Tool->Action)
    {
    case AtActionListFirewallEvents:
        AtpListFirewallEvents(Call, Result);
        break;
    default:
        AtSetToolError(Result, "failed", STATUS_NOT_IMPLEMENTED, L"This tool is not implemented.");
        break;
    }
}
