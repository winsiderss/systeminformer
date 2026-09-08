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

#include <networktoolsintf.h>

// The two tools that reach off the machine, and the only ones in this server that do. Everything
// else answers from what the system already knows; these send packets to an address the caller
// chose, which is why they sit in their own consent tier and are asked about separately.
//
// ping_host is implemented here rather than borrowed from NetworkTools, whose ping runs on a
// dialog's own thread and reports into its window. whois_lookup goes through the plugin, which
// knows the referral chain.

// The reply buffer has to hold the reply, the payload, an ICMP error message, an IO_STATUS_BLOCK
// and any options, which is what the helper documents and what NetworkTools uses.
#define AT_ICMP_BUFFER_SIZE(EchoReplyLength, BufferLength)     (ULONG)(((EchoReplyLength) + (BufferLength)) + 8 + sizeof(IO_STATUS_BLOCK) + MAX_OPT_SIZE)

#define AT_PING_MAX_COUNT 16
#define AT_PING_MAX_TIMEOUT 10000
#define AT_PING_PAYLOAD_SIZE 32

static PVOID AtpIcmpBaseAddress = NULL;
static HANDLE (WINAPI *AtpIcmpCreateFile)(VOID) = NULL;
static HANDLE (WINAPI *AtpIcmp6CreateFile)(VOID) = NULL;
static BOOL (WINAPI *AtpIcmpCloseHandle)(HANDLE) = NULL;
static ULONG (WINAPI *AtpIcmpSendEcho2Ex)(HANDLE, HANDLE, PVOID, PVOID, IPAddr, IPAddr, LPVOID, WORD, PIP_OPTION_INFORMATION, LPVOID, DWORD, DWORD) = NULL;
static ULONG (WINAPI *AtpIcmp6SendEcho2)(HANDLE, HANDLE, PVOID, PVOID, PSOCKADDR_IN6, PSOCKADDR_IN6, LPVOID, WORD, PIP_OPTION_INFORMATION, LPVOID, DWORD, DWORD) = NULL;

BOOLEAN AtpInitializeIcmp(
    VOID
    )
{
    static PH_INITONCE initOnce = PH_INITONCE_INIT;

    if (PhBeginInitOnce(&initOnce))
    {
        if (AtpIcmpBaseAddress = PhLoadLibrary(L"iphlpapi.dll"))
        {
            AtpIcmpCreateFile = PhGetProcedureAddress(AtpIcmpBaseAddress, "IcmpCreateFile", 0);
            AtpIcmp6CreateFile = PhGetProcedureAddress(AtpIcmpBaseAddress, "Icmp6CreateFile", 0);
            AtpIcmpCloseHandle = PhGetProcedureAddress(AtpIcmpBaseAddress, "IcmpCloseHandle", 0);
            AtpIcmpSendEcho2Ex = PhGetProcedureAddress(AtpIcmpBaseAddress, "IcmpSendEcho2Ex", 0);
            AtpIcmp6SendEcho2 = PhGetProcedureAddress(AtpIcmpBaseAddress, "Icmp6SendEcho2", 0);
        }

        PhEndInitOnce(&initOnce);
    }

    return !!AtpIcmpCreateFile && !!AtpIcmpSendEcho2Ex;
}

// IP_STATUS is not an NTSTATUS and its values are not errors in the usual sense: a reply that timed
// out and one that came back from an unreachable router are both answers about the path.
PCSTR AtpIpStatusString(
    _In_ ULONG Status
    )
{
    switch (Status)
    {
    case IP_SUCCESS:
        return "success";
    case IP_DEST_NET_UNREACHABLE:
        return "destination_network_unreachable";
    case IP_DEST_HOST_UNREACHABLE:
        return "destination_host_unreachable";
    case IP_DEST_PROT_UNREACHABLE:
        return "destination_protocol_unreachable";
    case IP_DEST_PORT_UNREACHABLE:
        return "destination_port_unreachable";
    case IP_NO_RESOURCES:
        return "no_resources";
    case IP_BAD_OPTION:
        return "bad_option";
    case IP_HW_ERROR:
        return "hardware_error";
    case IP_PACKET_TOO_BIG:
        return "packet_too_big";
    case IP_REQ_TIMED_OUT:
        return "timed_out";
    case IP_BAD_ROUTE:
        return "bad_route";
    case IP_TTL_EXPIRED_TRANSIT:
        return "ttl_expired_in_transit";
    case IP_TTL_EXPIRED_REASSEM:
        return "ttl_expired_in_reassembly";
    case IP_PARAM_PROBLEM:
        return "parameter_problem";
    case IP_SOURCE_QUENCH:
        return "source_quench";
    case IP_OPTION_TOO_BIG:
        return "option_too_big";
    case IP_BAD_DESTINATION:
        return "bad_destination";
    case IP_GENERAL_FAILURE:
        return "general_failure";
    }

    return "unknown";
}

_Success_(return)
BOOLEAN AtpParseAddress(
    _In_ PPH_STRING String,
    _Out_ PPH_IP_ADDRESS Address
    )
{
    USHORT port = 0;
    ULONG scopeId = 0;

    memset(Address, 0, sizeof(PH_IP_ADDRESS));

    if (NT_SUCCESS(PhIpv4StringToAddress(String->Buffer, TRUE, &Address->InAddr, &port)))
    {
        Address->Type = PH_NETWORK_TYPE_IPV4;
        return TRUE;
    }

    if (NT_SUCCESS(PhIpv6StringToAddress(String->Buffer, &Address->In6Addr, &scopeId, &port)))
    {
        Address->Type = PH_NETWORK_TYPE_IPV6;
        return TRUE;
    }

    return FALSE;
}

VOID AtpPingHost(
    _In_ PAT_TOOL_CALL Call,
    _Inout_ PAT_TOOL_RESULT Result
    )
{
    PPH_STRING addressString;
    PH_IP_ADDRESS address;
    HANDLE icmpHandle = INVALID_HANDLE_VALUE;
    PVOID replyBuffer = NULL;
    ULONG replyLength;
    UCHAR payload[AT_PING_PAYLOAD_SIZE];
    ULONG64 count = 4;
    ULONG64 timeout = 1000;
    ULONG succeeded = 0;
    ULONG64 totalRtt = 0;
    ULONG minimumRtt = ULONG_MAX;
    ULONG maximumRtt = 0;
    PVOID structured;
    PVOID replies;
    IP_OPTION_INFORMATION options;
    ULONG i;

    if (!AtpInitializeIcmp())
    {
        AtSetToolError(Result, "failed", STATUS_NOT_SUPPORTED, L"The ICMP helper is not available.");
        return;
    }

    if (!(addressString = AtGetArgumentString(Call->Arguments, "address")))
    {
        AtSetToolError(Result, "invalid_arguments", STATUS_INVALID_PARAMETER, L"address is required.");
        return;
    }

    // An address, not a host name: resolving a name is a second thing to go wrong and a second
    // thing to leave the machine, and the caller can resolve it themselves if they want to.
    if (!AtpParseAddress(addressString, &address))
    {
        AtSetToolError(
            Result,
            "invalid_arguments",
            STATUS_INVALID_PARAMETER,
            L"That is not an IPv4 or IPv6 address. This tool does not resolve host names."
            );
        PhDereferenceObject(addressString);
        return;
    }

    if (AtGetArgumentUInt64(Call->Arguments, "count", &count))
        count = min(max(count, 1), AT_PING_MAX_COUNT);

    if (AtGetArgumentUInt64(Call->Arguments, "timeout_ms", &timeout))
        timeout = min(max(timeout, 1), AT_PING_MAX_TIMEOUT);

    memset(payload, 'a', sizeof(payload));

    memset(&options, 0, sizeof(IP_OPTION_INFORMATION));
    options.Ttl = UCHAR_MAX;
    options.Flags = IP_FLAG_DF;

    if (address.Type == PH_NETWORK_TYPE_IPV6)
    {
        if (!AtpIcmp6CreateFile || !AtpIcmp6SendEcho2)
        {
            AtSetToolError(Result, "failed", STATUS_NOT_SUPPORTED, L"The ICMPv6 helper is not available.");
            PhDereferenceObject(addressString);
            return;
        }

        icmpHandle = AtpIcmp6CreateFile();
        replyLength = AT_ICMP_BUFFER_SIZE(sizeof(ICMPV6_ECHO_REPLY), sizeof(payload));
    }
    else
    {
        icmpHandle = AtpIcmpCreateFile();
        replyLength = AT_ICMP_BUFFER_SIZE(sizeof(ICMP_ECHO_REPLY), sizeof(payload));
    }

    if (icmpHandle == INVALID_HANDLE_VALUE)
    {
        AtSetToolError(Result, "failed", STATUS_UNSUCCESSFUL, L"Opening an ICMP handle failed.");
        PhDereferenceObject(addressString);
        return;
    }

    replyBuffer = PhAllocateZero(replyLength);
    replies = PhCreateJsonArray();

    for (i = 0; i < (ULONG)count; i++)
    {
        PVOID entry;
        ULONG replyCount;
        ULONG status;
        ULONG roundTripTime = 0;

        memset(replyBuffer, 0, replyLength);

        if (address.Type == PH_NETWORK_TYPE_IPV6)
        {
            SOCKADDR_IN6 localAddress;
            SOCKADDR_IN6 remoteAddress;
            PICMPV6_ECHO_REPLY reply = replyBuffer;

            memset(&localAddress, 0, sizeof(SOCKADDR_IN6));
            localAddress.sin6_family = AF_INET6;
            // Unspecified source, the IPv6 equivalent of letting the stack choose.
            memset(&localAddress.sin6_addr, 0, sizeof(localAddress.sin6_addr));

            memset(&remoteAddress, 0, sizeof(SOCKADDR_IN6));
            remoteAddress.sin6_family = AF_INET6;
            remoteAddress.sin6_addr = address.In6Addr;

            replyCount = AtpIcmp6SendEcho2(
                icmpHandle,
                NULL,
                NULL,
                NULL,
                &localAddress,
                &remoteAddress,
                payload,
                sizeof(payload),
                &options,
                replyBuffer,
                replyLength,
                (ULONG)timeout
                );

            status = replyCount ? reply->Status : IP_REQ_TIMED_OUT;
            roundTripTime = replyCount ? reply->RoundTripTime : 0;
        }
        else
        {
            PICMP_ECHO_REPLY reply = replyBuffer;

            replyCount = AtpIcmpSendEcho2Ex(
                icmpHandle,
                NULL,
                NULL,
                NULL,
                0, // INADDR_ANY: let the stack pick the source address
                address.InAddr.s_addr,
                payload,
                sizeof(payload),
                &options,
                replyBuffer,
                replyLength,
                (ULONG)timeout
                );

            status = replyCount ? reply->Status : IP_REQ_TIMED_OUT;
            roundTripTime = replyCount ? reply->RoundTripTime : 0;
        }

        entry = PhCreateJsonObject();
        PhAddJsonObjectUInt64(entry, "sequence", i);
        PhAddJsonObject(entry, "status", AtpIpStatusString(status));
        PhAddJsonObjectBoolean(entry, "replied", status == IP_SUCCESS);

        if (status == IP_SUCCESS)
        {
            PhAddJsonObjectUInt64(entry, "round_trip_ms", roundTripTime);

            succeeded++;
            totalRtt += roundTripTime;

            if (roundTripTime < minimumRtt)
                minimumRtt = roundTripTime;
            if (roundTripTime > maximumRtt)
                maximumRtt = roundTripTime;
        }
        else
        {
            AtJsonAddNull(entry, "round_trip_ms");
        }

        PhAddJsonArrayObject(replies, entry);
    }

    if (AtpIcmpCloseHandle)
        AtpIcmpCloseHandle(icmpHandle);

    PhFree(replyBuffer);

    structured = PhCreateJsonObject();
    AtJsonAddString(structured, "address", addressString);
    PhAddJsonObject(structured, "family", address.Type == PH_NETWORK_TYPE_IPV6 ? "ipv6" : "ipv4");
    PhAddJsonObjectUInt64(structured, "sent", (ULONG)count);
    PhAddJsonObjectUInt64(structured, "received", succeeded);
    PhAddJsonObjectUInt64(structured, "lost", (ULONG)count - succeeded);
    PhAddJsonObjectValue(structured, "replies", replies);

    if (succeeded)
    {
        PhAddJsonObjectUInt64(structured, "minimum_round_trip_ms", minimumRtt);
        PhAddJsonObjectUInt64(structured, "maximum_round_trip_ms", maximumRtt);
        PhAddJsonObjectDouble(structured, "average_round_trip_ms", (DOUBLE)totalRtt / succeeded);
    }
    else
    {
        // No reply came back, so there is no round trip to average. Reporting zero would read as
        // an instant reply.
        AtJsonAddNull(structured, "minimum_round_trip_ms");
        AtJsonAddNull(structured, "maximum_round_trip_ms");
        AtJsonAddNull(structured, "average_round_trip_ms");
    }

    AtAddSnapshot(structured);

    Result->StructuredContent = structured;

    PhDereferenceObject(addressString);
}

VOID AtpWhoisLookup(
    _In_ PAT_TOOL_CALL Call,
    _Inout_ PAT_TOOL_RESULT Result
    )
{
    PNETWORKTOOLS_INTERFACE pluginInterface;
    PPH_STRING addressString;
    PH_IP_ADDRESS address;
    PPH_STRING response = NULL;
    PVOID structured;

    if (!(pluginInterface = AtGetNetworkToolsInterface()))
    {
        AtSetToolHint(Result, AT_HINT_PLUGIN_MISSING);
        AtSetToolError(
            Result,
            "plugin_missing",
            STATUS_NOT_FOUND,
            L"The NetworkTools plugin is not loaded, so there is nothing to run the query."
            );
        return;
    }

    if (!(addressString = AtGetArgumentString(Call->Arguments, "address")))
    {
        AtSetToolError(Result, "invalid_arguments", STATUS_INVALID_PARAMETER, L"address is required.");
        return;
    }

    // Only an address, for the same reason ping takes only an address, and because a whois query
    // is sent to a third party: what goes out should be exactly what the caller named.
    if (!AtpParseAddress(addressString, &address))
    {
        AtSetToolError(
            Result,
            "invalid_arguments",
            STATUS_INVALID_PARAMETER,
            L"That is not an IPv4 or IPv6 address. This tool looks up address registrations, not domain names."
            );
        PhDereferenceObject(addressString);
        return;
    }

    if (!pluginInterface->QueryWhois(addressString->Buffer, address.Type == PH_NETWORK_TYPE_IPV6, &response))
    {
        AtSetToolError(Result, "failed", STATUS_UNSUCCESSFUL, L"No whois server answered.");
        PhDereferenceObject(addressString);
        return;
    }

    structured = PhCreateJsonObject();
    AtJsonAddString(structured, "address", addressString);
    PhAddJsonObject(structured, "family", address.Type == PH_NETWORK_TYPE_IPV6 ? "ipv6" : "ipv4");
    AtJsonAddString(structured, "response", response);
    AtAddSnapshot(structured);

    Result->StructuredContent = structured;

    PhClearReference(&response);
    PhDereferenceObject(addressString);
}

VOID AtEgressInvokeTool(
    _In_ PCAT_TOOL Tool,
    _In_ PAT_TOOL_CALL Call,
    _Inout_ PAT_TARGET Target,
    _Inout_ PAT_TOOL_RESULT Result
    )
{
    UNREFERENCED_PARAMETER(Target);

    switch (Tool->Action)
    {
    case AtActionPingHost:
        AtpPingHost(Call, Result);
        break;
    case AtActionWhoisLookup:
        AtpWhoisLookup(Call, Result);
        break;
    default:
        AtSetToolError(Result, "failed", STATUS_NOT_IMPLEMENTED, L"This tool is not implemented.");
        break;
    }
}
