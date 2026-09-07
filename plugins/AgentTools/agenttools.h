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

#ifndef _AGENTTOOLS_H
#define _AGENTTOOLS_H

#include <phdk.h>
#include <phappresource.h>
#include <settings.h>
#include <searchbox.h>
#include <json.h>
#include <verify.h>
#include <workqueue.h>
#include <hndlinfo.h>
#include <svcsup.h>
#include <lsasup.h>
#include <symprv.h>
#include <kphuser.h>
#include <simcp.h>

#include "resource.h"

#define PLUGIN_NAME L"AgentTools"

#define SETTING_NAME_ENABLED (PLUGIN_NAME L".Enabled")
#define SETTING_NAME_ALLOW_SANDBOXED_CLIENTS (PLUGIN_NAME L".AllowSandboxedClients")
#define SETTING_NAME_CONFIRM_CONNECTIONS (PLUGIN_NAME L".ConfirmConnections")
#define SETTING_NAME_TOOL_ACCESS(Tool) (PLUGIN_NAME L".Tools." Tool L".Access")
#define SETTING_NAME_TOOL_CONFIRM(Tool) (PLUGIN_NAME L".Tools." Tool L".Confirm")
#define SETTING_NAME_AGENTS_LISTVIEW_COLUMNS (PLUGIN_NAME L".OptionsAgentsListViewColumns")
#define SETTING_NAME_TOOLS_LISTVIEW_COLUMNS (PLUGIN_NAME L".OptionsToolsListViewColumns")

#define AT_ACCESS_DENIED 0
#define AT_ACCESS_ALLOWED 1
#define AT_CONFIRM_NONE 0 // not required: nobody is asked
#define AT_CONFIRM_ALWAYS 1 // System Informer shows its own dialog
#define AT_CONFIRM_DELEGATE 2 // delegated to the client's elicitation UI
#define AT_SCHEMA_VERSION 1 // MCP tool schema version. Bump on a breaking change; additive changes keep it.
#define AT_CONSENT_TIMEOUT_MS (60 * 1000) // from the moment the dialog is on screen
#define AT_CONSENT_QUEUE_TIMEOUT_MS (5 * 60 * 1000) // from submission, while queued behind other dialogs
#define AT_ELICITATION_TIMEOUT_MS (10 * 60 * 1000)
#define AT_PENDING_CONSENT_TIMEOUT_MS (5 * 60 * 1000)

extern PPH_PLUGIN PluginInstance;

typedef enum _AT_TIER
{
    AtTierRead,
    AtTierSensitiveRead,
    AtTierWrite
} AT_TIER;

typedef enum _AT_ACTION
{
    AtActionConnect,
    // processes
    AtActionListProcesses,
    AtActionGetProcess,
    AtActionGetProcessModules,
    AtActionGetProcessThreads,
    AtActionGetProcessHandles,
    AtActionGetProcessMemoryRegions,
    AtActionGetProcessToken,
    AtActionGetProcessWindows,
    AtActionReadProcessEnvironment,
    AtActionGetProcessHandlesDetailed,
    AtActionGetThreadStack,
    AtActionTerminateProcess,
    AtActionSuspendProcess,
    AtActionResumeProcess,
    AtActionSetProcessPriority,
    AtActionSetProcessIoPriority,
    AtActionCreateProcessMinidump,
    AtActionCloseHandle,
    // threads
    AtActionSuspendThread,
    AtActionResumeThread,
    AtActionTerminateThread,
    // services
    AtActionListServices,
    AtActionGetService,
    AtActionStartService,
    AtActionStopService,
    AtActionRestartService,
    AtActionSetServiceConfig,
    // network
    AtActionListNetworkConnections,
    AtActionCloseNetworkConnection,
    // system
    AtActionGetSystemInfo,
    AtActionListKernelDrivers,
    AtActionGetKsiStatus,
    // files and memory
    AtActionVerifyFileSignature,
    AtActionGetImageInfo,
    AtActionReadProcessMemory,
    AtActionSearchProcessMemory,
    // more system inspection
    AtActionGetPagefileInfo,
    AtActionListStartupEntries,
    AtActionMaximum,
} AT_ACTION;

typedef enum _AT_TARGET_KIND
{
    AtTargetNone,
    AtTargetProcess,
    AtTargetThread,
    AtTargetService,
    AtTargetHandle,
    AtTargetConnection,
} AT_TARGET_KIND;

typedef struct _AT_ACTION_INFO
{
    AT_ACTION Action;
    AT_TIER Tier;
    AT_TARGET_KIND TargetKind;
    ACCESS_MASK TargetAccess; // process access for process targets, thread access for thread targets, service access for service targets
    PWSTR ConfirmSetting;
    PWSTR Verb;
    PWSTR Headline;
    PWSTR ButtonText;
    PWSTR AuditName;
} AT_ACTION_INFO, *PAT_ACTION_INFO;
typedef CONST AT_ACTION_INFO *PCAT_ACTION_INFO;

extern CONST AT_ACTION_INFO AtActionInfo[AtActionMaximum];

/**
 * The object a tool call acts on, resolved against the provider caches and, where the action needs
 * one, opened with exactly the rights the action needs. The consent dialog, the audit log and the
 * elicitation prompt all describe the target from this structure, and pending client-side consent
 * is bound to Identity so a retry cannot be redirected to a different object.
 */
typedef struct _AT_TARGET
{
    AT_TARGET_KIND Kind;

    PPH_PROCESS_ITEM ProcessItem; // process, thread, handle and connection targets (optional for connections)
    HANDLE ProcessHandle;

    HANDLE ThreadId;
    HANDLE ThreadHandle;

    PPH_SERVICE_ITEM ServiceItem;
    SC_HANDLE ServiceHandle;

    HANDLE HandleValue;
    ULONG HandleTypeIndex;
    ULONG HandleAttributes;
    PVOID HandleObject;
    PPH_STRING HandleTypeName;
    PPH_STRING HandleObjectName;

    PPH_NETWORK_ITEM NetworkItem; // connection targets; a private copy, never the cached item
    PPH_STRING ConnectionText;

    PPH_STRING Parameter; // what the action sets the target to, when it takes a value
    ULONG64 Identity[4];
} AT_TARGET, *PAT_TARGET;

typedef enum _AT_SESSION_POLICY
{
    AtSessionAsk,
    AtSessionAllow,
    AtSessionDelegate,
} AT_SESSION_POLICY;

#define AT_MAX_PENDING_CONSENTS 8

typedef struct _AT_PENDING_CONSENT
{
    BOOLEAN Used;
    AT_ACTION Action;
    ULONG64 Identity[4];
    ULONG64 Nonce;
    LARGE_INTEGER Expiry;
} AT_PENDING_CONSENT, *PAT_PENDING_CONSENT;

typedef enum _AT_APPROVAL
{
    AtApprovalPending,
    AtApprovalAllowed,
    AtApprovalDenied,
} AT_APPROVAL;

C_ASSERT(sizeof(AT_APPROVAL) == sizeof(LONG));

typedef struct _AT_CONNECTION
{
    LIST_ENTRY ListEntry;
    ULONG ConnectionId;
    HANDLE PipeHandle;
    HANDLE ThreadHandle;
    HANDLE ThreadId;
    LONG Closing;
    ULONG CloseDetail;
    BOOLEAN CloseSent;
    BOOLEAN Registered;
    BOOLEAN Authenticated;
    AT_APPROVAL Approval;
    PVOID ApprovalRequest;

    PPH_STRING UserName;
    PWSTR IntegrityString;
    MANDATORY_LEVEL_RID IntegrityRid;
    BOOLEAN IsAppContainer;
    ULONG BrokerProcessId;

    ULONG LauncherProcessId;
    PPH_STRING LauncherImageName;
    BOOLEAN LauncherVerifyChecked;
    VERIFY_RESULT LauncherVerifyResult;
    PPH_STRING LauncherSignerName;
    PPH_STRING ClientName;
    PPH_STRING ClientVersion;

    LARGE_INTEGER ConnectTime;
    ULONG CallCount;

    BOOLEAN Initialized;
    BOOLEAN LegacyElicitation;
    PPH_STRING ProtocolVersion;
    ULONG NextServerRequestId;

    PPH_BYTES InFlightId;
    BOOLEAN InFlightCancelled;

    AT_SESSION_POLICY SessionPolicy[AtActionMaximum];
    AT_PENDING_CONSENT Pending[AT_MAX_PENDING_CONSENTS];

    PH_QUEUED_LOCK Lock;
} AT_CONNECTION, *PAT_CONNECTION;

// server.c

typedef enum _AT_SERVER_STATE
{
    AtServerStopped,
    AtServerRunning,
    AtServerFailedPipeExists,
    AtServerFailed
} AT_SERVER_STATE;

NTSTATUS AtServerStart(
    VOID
    );

VOID AtServerStop(
    _In_ SIMCP_CLOSE_REASON Reason
    );

AT_SERVER_STATE AtServerGetState(
    _Out_opt_ PNTSTATUS Status,
    _Out_opt_ PBOOLEAN Elevated
    );

PPH_LIST AtServerSnapshotConnections(
    VOID
    );

VOID AtServerDisconnect(
    _In_ ULONG ConnectionId
    );

FORCEINLINE
BOOLEAN
AtConnectionIsClosing(
    _In_ PAT_CONNECTION Connection
    )
{
    return !!ReadAcquire(&Connection->Closing);
}

NTSTATUS AtConnectionSend(
    _In_ PAT_CONNECTION Connection,
    _In_ USHORT Type,
    _In_reads_bytes_opt_(PayloadLength) PVOID Payload,
    _In_ ULONG PayloadLength
    );

NTSTATUS AtConnectionRead(
    _In_ PAT_CONNECTION Connection,
    _Out_ PSIMCP_HEADER Header,
    _Outptr_result_maybenull_ PVOID* Payload
    );

NTSTATUS AtConnectionPeek(
    _In_ PAT_CONNECTION Connection,
    _Out_ PBOOLEAN MessageAvailable
    );

VOID AtConnectionClose(
    _In_ PAT_CONNECTION Connection,
    _In_ SIMCP_CLOSE_REASON Reason,
    _In_ ULONG Detail
    );

// mcp.c

typedef enum _AT_CONSENT_RESULT
{
    AtConsentAllowed,
    AtConsentDenied,
    AtConsentTimeout,
    AtConsentDeclined,
    AtConsentCancelled,
    AtConsentElicitationRequired,
    AtConsentInputRequired,
    AtConsentFailed
} AT_CONSENT_RESULT;

typedef struct _AT_TOOL_CALL
{
    PAT_CONNECTION Connection;
    PPH_BYTES IdJson;
    BOOLEAN Modern;
    BOOLEAN ClientElicitation;
    PVOID Arguments;
    PPH_STRING RequestState;
    PVOID InputResponses;
} AT_TOOL_CALL, *PAT_TOOL_CALL;

VOID AtMcpHandleMessage(
    _In_ PAT_CONNECTION Connection,
    _In_reads_bytes_(Length) PVOID Payload,
    _In_ ULONG Length
    );

VOID AtMcpDeleteConnectionState(
    _In_ PAT_CONNECTION Connection
    );

AT_CONSENT_RESULT AtMcpElicitConsent(
    _In_ PAT_TOOL_CALL Call,
    _In_ PCAT_ACTION_INFO Action,
    _In_opt_ PAT_TARGET Target
    );

BOOLEAN AtMcpPumpDuringWait(
    _In_ PAT_CONNECTION Connection
    );

BOOLEAN AtJsonGetObjectBoolean(
    _In_opt_ PVOID Object,
    _In_ PCSTR Key
    );

// JSON helpers (mcp.c)

VOID AtJsonAddString(
    _In_ PVOID Object,
    _In_ PCSTR Key,
    _In_opt_ PPH_STRING String
    );

VOID AtJsonAddStringRef(
    _In_ PVOID Object,
    _In_ PCSTR Key,
    _In_opt_ PCPH_STRINGREF String
    );

VOID AtJsonAddNull(
    _In_ PVOID Object,
    _In_ PCSTR Key
    );

VOID AtJsonAddTime(
    _In_ PVOID Object,
    _In_ PCSTR Key,
    _In_ PLARGE_INTEGER Time
    );

PVOID AtJsonGetObjectMember(
    _In_opt_ PVOID Object,
    _In_ PCSTR Key,
    _In_ PH_JSON_OBJECT_TYPE Type
    );

// tools.c

typedef struct _AT_TOOL_RESULT
{
    PVOID StructuredContent;
    PCSTR ErrorCode;
    PPH_STRING ErrorMessage;
    NTSTATUS Status;
} AT_TOOL_RESULT, *PAT_TOOL_RESULT;

typedef struct _AT_TOOL
{
    PCSTR Name;
    PWSTR DisplayName;
    AT_TIER Tier;
    AT_ACTION Action;
    PWSTR AccessSetting;
    PWSTR ConfirmSetting;
    PCSTR Definition;
} AT_TOOL, *PAT_TOOL;
typedef CONST AT_TOOL *PCAT_TOOL;

extern CONST AT_TOOL AtTools[];
extern CONST ULONG AtToolCount;

PCAT_TOOL AtFindTool(
    _In_ PPH_STRING Name
    );

ULONG AtToolDefaultAccess(
    _In_ PCAT_TOOL Tool
    );

ULONG AtToolDefaultConfirm(
    _In_ PCAT_TOOL Tool
    );

VOID AtRegisterToolSettings(
    VOID
    );

BOOLEAN AtIsToolEnabled(
    _In_ PCAT_TOOL Tool
    );

VOID AtEnumTools(
    _In_ PVOID ToolsArray
    );

VOID AtInvokeTool(
    _In_ PCAT_TOOL Tool,
    _In_ PAT_TOOL_CALL Call,
    _Inout_ PAT_TARGET Target,
    _Inout_ PAT_TOOL_RESULT Result
    );

VOID AtSetToolError(
    _Inout_ PAT_TOOL_RESULT Result,
    _In_ PCSTR ErrorCode,
    _In_ NTSTATUS Status,
    _In_ PCWSTR Format,
    ...
    );

VOID AtSetToolStatusError(
    _Inout_ PAT_TOOL_RESULT Result,
    _In_ NTSTATUS Status,
    _In_ PCWSTR Operation
    );

VOID AtDeleteToolResult(
    _Inout_ PAT_TOOL_RESULT Result
    );

// Shared helpers for tool implementations (tools.c)

VOID AtAddSnapshot(
    _In_ PVOID Object
    );

BOOLEAN AtGetArgumentUInt64(
    _In_opt_ PVOID Arguments,
    _In_ PCSTR Key,
    _Out_ PULONG64 Value
    );

BOOLEAN AtGetArgumentPointer(
    _In_opt_ PVOID Arguments,
    _In_ PCSTR Key,
    _Out_ PULONG64 Value
    );

PPH_STRING AtGetArgumentString(
    _In_opt_ PVOID Arguments,
    _In_ PCSTR Key
    );

VOID AtJsonAddStringZ(
    _In_ PVOID Object,
    _In_ PCSTR Key,
    _In_opt_ PCWSTR String
    );

VOID AtJsonAddPointer(
    _In_ PVOID Object,
    _In_ PCSTR Key,
    _In_opt_ PVOID Pointer
    );

VOID AtJsonAddWin32FileName(
    _In_ PVOID Object,
    _In_ PCSTR Key,
    _In_opt_ PPH_STRING NativeFileName
    );

VOID AtJsonAddDuration(
    _In_ PVOID Object,
    _In_ PCSTR Key,
    _In_ ULONG64 Duration100ns
    );

BOOLEAN AtContainsString(
    _In_opt_ PPH_STRING String,
    _In_opt_ PPH_STRING Needle
    );

PCWSTR AtVerifyResultString(
    _In_ VERIFY_RESULT Result
    );

PCWSTR AtIoPriorityString(
    _In_ IO_PRIORITY_HINT IoPriority
    );

PCWSTR AtPriorityClassString(
    _In_ ULONG PriorityClass
    );

VOID AtFillProcessIdentity(
    _In_ PVOID Object,
    _In_ PPH_PROCESS_ITEM ProcessItem
    );

BOOLEAN AtParsePriorityClass(
    _In_opt_ PPH_STRING String,
    _Out_ PULONG PriorityClass
    );

BOOLEAN AtParseIoPriority(
    _In_opt_ PPH_STRING String,
    _Out_ IO_PRIORITY_HINT* IoPriority
    );

BOOLEAN AtParseServiceStartType(
    _In_opt_ PPH_STRING String,
    _Out_ PULONG StartType
    );

PPH_STRING AtFormatServiceConfigParameter(
    _In_opt_ PVOID Arguments,
    _Inout_ PAT_TOOL_RESULT Result
    );

// Tool implementations by area

VOID AtProcessInvokeTool(
    _In_ PCAT_TOOL Tool,
    _In_ PAT_TOOL_CALL Call,
    _Inout_ PAT_TARGET Target,
    _Inout_ PAT_TOOL_RESULT Result
    );

VOID AtThreadInvokeTool(
    _In_ PCAT_TOOL Tool,
    _In_ PAT_TOOL_CALL Call,
    _Inout_ PAT_TARGET Target,
    _Inout_ PAT_TOOL_RESULT Result
    );

VOID AtMemoryInvokeTool(
    _In_ PCAT_TOOL Tool,
    _In_ PAT_TOOL_CALL Call,
    _Inout_ PAT_TARGET Target,
    _Inout_ PAT_TOOL_RESULT Result
    );

VOID AtServiceInvokeTool(
    _In_ PCAT_TOOL Tool,
    _In_ PAT_TOOL_CALL Call,
    _Inout_ PAT_TARGET Target,
    _Inout_ PAT_TOOL_RESULT Result
    );

VOID AtNetworkInvokeTool(
    _In_ PCAT_TOOL Tool,
    _In_ PAT_TOOL_CALL Call,
    _Inout_ PAT_TARGET Target,
    _Inout_ PAT_TOOL_RESULT Result
    );

VOID AtSystemInvokeTool(
    _In_ PCAT_TOOL Tool,
    _In_ PAT_TOOL_CALL Call,
    _Inout_ PAT_TARGET Target,
    _Inout_ PAT_TOOL_RESULT Result
    );

VOID AtPeInvokeTool(
    _In_ PCAT_TOOL Tool,
    _In_ PAT_TOOL_CALL Call,
    _Inout_ PAT_TARGET Target,
    _Inout_ PAT_TOOL_RESULT Result
    );

VOID AtListStartupEntries(
    _In_ PAT_TOOL_CALL Call,
    _Inout_ PAT_TOOL_RESULT Result
    );

// target.c

NTSTATUS AtResolveTarget(
    _In_ PCAT_TOOL Tool,
    _In_opt_ PVOID Arguments,
    _Out_ PAT_TARGET Target,
    _Inout_ PAT_TOOL_RESULT Result
    );

NTSTATUS AtResolveProcessTarget(
    _In_opt_ PVOID Arguments,
    _In_ BOOLEAN RequireSequenceNumber,
    _In_ ACCESS_MASK ProcessAccess,
    _Out_ PAT_TARGET Target,
    _Inout_ PAT_TOOL_RESULT Result
    );

VOID AtDeleteTarget(
    _Inout_ PAT_TARGET Target
    );

VOID AtSetTargetParameter(
    _Inout_ PAT_TARGET Target,
    _In_ PCWSTR Parameter
    );

PPH_STRING AtFormatTargetHeadline(
    _In_ PAT_TARGET Target
    );

PPH_STRING AtFormatTargetDescription(
    _In_ PAT_TARGET Target
    );

PPH_STRING AtFormatTargetAudit(
    _In_ PAT_TARGET Target
    );

PPH_STRING AtFormatNetworkEndpoint(
    _In_ PPH_IP_ENDPOINT Endpoint,
    _In_ ULONG ProtocolType,
    _In_ ULONG ScopeId,
    _In_ BOOLEAN IncludePort
    );

PCWSTR AtProtocolTypeString(
    _In_ ULONG ProtocolType
    );

BOOLEAN AtParseProtocolType(
    _In_opt_ PPH_STRING String,
    _Out_ PULONG ProtocolType
    );

NTSTATUS AtFindNetworkConnection(
    _In_opt_ PVOID Arguments,
    _In_opt_ PPH_PROCESS_ITEM ProcessItem,
    _Out_ PPH_NETWORK_ITEM* NetworkItem,
    _Inout_ PAT_TOOL_RESULT Result
    );

// consent.c

VOID AtConsentInitialize(
    VOID
    );

VOID AtConsentUninitialize(
    VOID
    );

VOID AtConsentRequestConnection(
    _In_ PAT_CONNECTION Connection
    );

AT_CONSENT_RESULT AtConsentWaitForConnection(
    _In_ PAT_CONNECTION Connection
    );

VOID AtConsentReleaseConnection(
    _In_ PAT_CONNECTION Connection
    );

AT_CONSENT_RESULT AtConsentGate(
    _In_ PAT_TOOL_CALL Call,
    _In_ PCAT_ACTION_INFO Action,
    _In_opt_ PAT_TARGET Target
    );

PPH_STRING AtFormatCallerDescription(
    _In_ PAT_CONNECTION Connection
    );

VOID AtAudit(
    _In_ PAT_CONNECTION Connection,
    _In_ PCAT_ACTION_INFO Action,
    _In_opt_ PAT_TARGET Target,
    _In_ PCWSTR Outcome
    );

// options.c

INT_PTR CALLBACK AtOptionsDlgProc(
    _In_ HWND WindowHandle,
    _In_ UINT WindowMessage,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

INT_PTR CALLBACK AtAgentsDlgProc(
    _In_ HWND WindowHandle,
    _In_ UINT WindowMessage,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

#endif
