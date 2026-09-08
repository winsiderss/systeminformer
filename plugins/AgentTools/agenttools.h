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
#include <secedit.h>

#include <networktoolsintf.h>
#include <onlinechecksintf.h>

#include "../ExtendedTools/extension/plugin.h"
#include <svcsup.h>
#include <lsasup.h>
#include <symprv.h>
#include <strsrch.h>
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
#define AT_CONFIRM_NONE 0
#define AT_CONFIRM_ALWAYS 1
#define AT_CONFIRM_DELEGATE 2
// 2: get_service is_microsoft and verify_file_signature is_microsoft_chained both became
// is_microsoft_signed, the name every other row that carries the field already used.
#define AT_SCHEMA_VERSION 2
#define AT_CONSENT_TIMEOUT_MS (60 * 1000)
#define AT_CONSENT_QUEUE_TIMEOUT_MS (5 * 60 * 1000)
#define AT_ELICITATION_TIMEOUT_MS (10 * 60 * 1000)
#define AT_PENDING_CONSENT_TIMEOUT_MS (5 * 60 * 1000)

extern PPH_PLUGIN PluginInstance;

typedef enum _AT_TIER
{
    AtTierRead,
    AtTierSensitiveRead,
    AtTierWrite,
    // Sends something off this machine: an address, a file hash. The data is small, but where it
    // goes is not the user's machine, so it is asked about by default and never granted implicitly.
    AtTierNetworkEgress
} AT_TIER;

typedef enum _AT_ACTION
{
    AtActionConnect,
    // processes
    AtActionListProcesses,
    AtActionGetProcess,
    AtActionGetProcessHistory,
    AtActionRankProcesses,
    AtActionListRecentEvents,
    AtActionListRecentProcessExits,
    AtActionGetProcessMitigations,
    AtActionGetProcessModules,
    AtActionGetProcessThreads,
    AtActionGetProcessHandles,
    AtActionGetProcessMemoryRegions,
    AtActionGetProcessToken,
    AtActionGetProcessWindows,
    AtActionListWindows,
    AtActionGetWindowInfo,
    AtActionGetDotNetAssemblies,
    AtActionGetProcessNotes,
    AtActionSetProcessComment,
    AtActionReadProcessEnvironment,
    AtActionGetProcessHandlesDetailed,
    AtActionFindHandles,
    AtActionFindModules,
    AtActionGetFileUsers,
    AtActionListObjectDirectory,
    AtActionGetObjectInfo,
    AtActionGetAlpcPortInfo,
    AtActionGetHandleDetails,
    AtActionListNamedPipes,
    AtActionGetSectionMappings,
    AtActionFindObjectHandles,
    AtActionGetThreadStack,
    AtActionGetProcessStacks,
    AtActionGetThreadWaitChain,
    AtActionAnalyzeThreadWait,
    AtActionResolveSymbol,
    AtActionSearchProcessStrings,
    AtActionGetProcessUnloadedModules,
    AtActionGetProcessImageCoherency,
    AtActionGetImagePageModifications,
    AtActionGetProcessJob,
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
    AtActionGetDiskPerformance,
    AtActionGetDiskIdentity,
    AtActionGetDiskHealth,
    AtActionListNetworkAdapters,
    AtActionLookupIpCountry,
    AtActionPingHost,
    AtActionWhoisLookup,
    AtActionListFirewallEvents,
    AtActionListNetworkConnections,
    AtActionCloseNetworkConnection,
    // system
    AtActionGetSystemInfo,
    AtActionGetSystemHistory,
    AtActionGetGpuUsage,
    AtActionListGpuAdapters,
    AtActionGetProcessGpuStats,
    AtActionListDevices,
    AtActionGetDeviceResources,
    AtActionGetProcessIoRates,
    AtActionListKernelDrivers,
    AtActionGetKsiStatus,
    AtActionGetPagefileInfo,
    AtActionListStartupEntries,
    AtActionGetSmbiosInfo,
    AtActionGetUefiVariables,
    AtActionGetTpmInfo,
    AtActionGetSystemEnvironment,
    AtActionGetMemoryDetails,
    AtActionGetSecurityPosture,
    AtActionGetCpuInfo,
    AtActionListLogonSessions,
    AtActionListTerminalSessions,
    AtActionLookupAccount,
    AtActionListScheduledTasks,
    AtActionListWmiSubscriptions,
    AtActionListHiddenProcesses,
    AtActionGetProcessKsiState,
    AtActionGetDriverObject,
    AtActionListDirectory,
    AtActionGetObjectSecurity,
    AtActionListPoolTags,
    AtActionSetProcessAffinity,
    AtActionSetProcessPagePriority,
    AtActionEmptyProcessWorkingSet,
    // files and memory
    AtActionVerifyFileSignature,
    AtActionGetFileHashes,
    AtActionGetImageStrings,
    AtActionGetFileInfo,
    AtActionGetFileScanResultCached,
    AtActionLookupFileHashVirusTotal,
    AtActionLookupFileHashHybridAnalysis,
    AtActionReadRegistryKey,
    AtActionGetImageInfo,
    AtActionReadProcessMemory,
    AtActionSearchProcessMemory,
    AtActionMaximum,
} AT_ACTION;

// A session grant covers a class of data rather than one tool, so a scan that reads the same thing
// across every process asks once instead of once per process, and a later tool that reads exactly
// that data joins the grant the user already made. A class is deliberately narrow: it groups tools
// that expose the same data about the same objects, never merely related ones. AtConsentClassNone
// keys the grant by the action itself.
typedef enum _AT_CONSENT_CLASS
{
    AtConsentClassNone,
    AtConsentClassHandleNames,
    AtConsentClassThreadStacks,
    AtConsentClassProcessMemory,
    AtConsentClassMaximum
} AT_CONSENT_CLASS;

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
    AT_CONSENT_CLASS Class;
    AT_TARGET_KIND TargetKind;
    ACCESS_MASK TargetAccess;
    PWSTR ConfirmSetting;
    PWSTR Verb;
    PWSTR Headline;
    PWSTR AuditName;
} AT_ACTION_INFO, *PAT_ACTION_INFO;
typedef CONST AT_ACTION_INFO *PCAT_ACTION_INFO;

extern CONST AT_ACTION_INFO AtActionInfo[AtActionMaximum];

// Checks that AtActionInfo is in the order it is indexed in; see the definition.
VOID AtVerifySchema(
    VOID
    );

typedef struct _AT_TARGET
{
    AT_TARGET_KIND Kind;

    PPH_PROCESS_ITEM ProcessItem;
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

    PPH_NETWORK_ITEM NetworkItem;
    PPH_STRING ConnectionText;

    PPH_STRING Parameter;
    ULONG64 Identity[4];
} AT_TARGET, *PAT_TARGET;

typedef enum _AT_SESSION_POLICY
{
    AtSessionAsk,
    AtSessionAllow,
    AtSessionDelegate,
} AT_SESSION_POLICY;

#define AT_MAX_PENDING_CONSENTS 16
#define AT_MAX_DEFERRED_REQUESTS 512
#define AT_MAX_DEFERRED_BYTES (1024 * 1024)

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

typedef struct _AT_DEFERRED_REQUEST
{
    LIST_ENTRY ListEntry;
    PPH_BYTES IdJson;
    ULONG Length;
    UCHAR Payload[ANYSIZE_ARRAY];
} AT_DEFERRED_REQUEST, *PAT_DEFERRED_REQUEST;

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
    LIST_ENTRY DeferredRequests;
    ULONG DeferredCount;
    SIZE_T DeferredBytes;

    AT_SESSION_POLICY SessionPolicy[AtActionMaximum];
    AT_SESSION_POLICY ClassPolicy[AtConsentClassMaximum];
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

// events.c

VOID AtEventsInitialize(
    VOID
    );

VOID AtEventsUninitialize(
    VOID
    );

// snapshot.c

VOID AtSnapshotInitialize(
    VOID
    );

VOID AtSnapshotUninitialize(
    VOID
    );

ULONG AtGetSnapshotId(
    VOID
    );

// The provider's update interval in milliseconds, from the last provider run.
ULONG AtGetUpdateInterval(
    VOID
    );

VOID AtAddProcessChanges(
    _In_ PVOID Object,
    _In_ ULONG SinceId
    );

VOID AtAddServiceChanges(
    _In_ PVOID Object,
    _In_ ULONG SinceId
    );

// tools.c

// Hints carried on a tool error: what would have to change for the call to work. Only the hints
// that hold are sent; an absent hint means the change would not help.

#define AT_HINT_NEEDS_ELEVATION 0x00000001ul
#define AT_HINT_NEEDS_DRIVER 0x00000002ul
#define AT_HINT_CONSENT_REQUIRED 0x00000004ul
#define AT_HINT_PLUGIN_MISSING 0x00000008ul
#define AT_HINT_RETRYABLE 0x00000010ul

typedef struct _AT_TOOL_RESULT
{
    PVOID StructuredContent;
    PCSTR ErrorCode;
    PPH_STRING ErrorMessage;
    NTSTATUS Status;
    ULONG Hints;
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

VOID AtSetToolHint(
    _Inout_ PAT_TOOL_RESULT Result,
    _In_ ULONG Hints
    );

VOID AtAddErrorHints(
    _In_ PVOID Error,
    _In_ PAT_TOOL_RESULT Result
    );

VOID AtDeleteToolResult(
    _Inout_ PAT_TOOL_RESULT Result
    );

// Shared helpers for tool implementations (tools.c)

// Paging and sorting for the list tools. Rows are collected in enumeration order, optionally
// sorted by one of their own fields, then the [offset, offset + limit) window is emitted.

#define AT_ROWS_DEFAULT_LIMIT 200
#define AT_ROWS_MAXIMUM_LIMIT 10000

typedef struct _AT_ROWS
{
    PPH_LIST Rows;
    PPH_STRING SortBy;
    BOOLEAN Descending;
    ULONG Limit;
    ULONG Offset;
    ULONG TotalCount;
} AT_ROWS, *PAT_ROWS;

// pids[] batching on the per-process reads: one call for a triage pass over many processes
// instead of one round trip each. A pid that cannot be answered becomes an error entry rather
// than failing the whole call.

#define AT_MAX_BATCH_PIDS 64

typedef struct _AT_BATCH
{
    PVOID Pids;
    ULONG Count;
    BOOLEAN Summary;
} AT_BATCH, *PAT_BATCH;

_Success_(return)
BOOLEAN AtInitializeBatch(
    _Out_ PAT_BATCH Batch,
    _In_opt_ PVOID Arguments,
    _Inout_ PAT_TOOL_RESULT Result
    );

PPH_PROCESS_ITEM AtBatchReferenceProcessItem(
    _In_ PAT_BATCH Batch,
    _In_ ULONG Index,
    _Out_ PULONG ProcessId
    );

PPH_SYMBOL_PROVIDER AtCreateSymbolProvider(
    _In_ HANDLE ProcessId
    );

// Finds one module of a process: by an address inside it, by name, or the process's own image when
// neither is given. Returns FALSE when there is no such module.
_Success_(return)
BOOLEAN AtFindProcessModule(
    _In_ HANDLE ProcessId,
    _In_opt_ HANDLE ProcessHandle,
    _In_opt_ PVOID Address,
    _In_opt_ PPH_STRING Name,
    _Out_ PVOID *BaseAddress,
    _Out_ PSIZE_T Size,
    _Out_ PPH_STRING *FileName
    );

PCWSTR AtStringEncodingString(
    _In_ PH_STRING_SEARCH_ENCODING Encoding
    );

_Success_(return)
BOOLEAN AtGetArgumentEncoding(
    _In_opt_ PVOID Arguments,
    _Out_ PPH_STRING_SEARCH_ENCODING Encoding,
    _Out_ PBOOLEAN HaveEncoding,
    _Inout_ PAT_TOOL_RESULT Result
    );

PVOID AtCreateBatchError(
    _In_ ULONG ProcessId,
    _In_ PCSTR ErrorCode,
    _In_ PCWSTR Message
    );

PVOID AtCreateBatchResult(
    _In_ PVOID Results
    );

VOID AtInitializeRows(
    _Out_ PAT_ROWS Rows,
    _In_opt_ PVOID Arguments
    );

VOID AtAddRow(
    _Inout_ PAT_ROWS Rows,
    _In_opt_ PVOID Row
    );

VOID AtAddRows(
    _In_ PVOID Object,
    _In_ PCSTR Key,
    _Inout_ PAT_ROWS Rows
    );

VOID AtDeleteRows(
    _Inout_ PAT_ROWS Rows
    );

VOID AtAddSnapshot(
    _In_ PVOID Object
    );

// The ExtendedTools plugin interface, or NULL when that plugin is absent or too old. It publishes
// the GPU and per-process I/O counters that System Informer itself does not collect.
PEXTENDEDTOOLS_INTERFACE AtGetExtendedToolsInterface(
    VOID
    );

VOID AtAddRate(
    _In_ PVOID Object,
    _In_ PCSTR Key,
    _In_ ULONG64 Delta,
    _In_ ULONG IntervalMs
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

VOID AtJsonAddHex(
    _In_ PVOID Object,
    _In_ PCSTR Key,
    _In_ ULONG64 Value
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

_Success_(return)
BOOLEAN AtParseTime(
    _In_opt_ PPH_STRING String,
    _Out_ PLARGE_INTEGER Time
    );

VOID AtParseRegistryPath(
    _In_ PPH_STRING Path,
    _Out_ PHANDLE Root,
    _Out_ PPH_STRING* SubKey,
    _Out_ PCWSTR* NativeRoot
    );

BOOLEAN AtContainsString(
    _In_opt_ PPH_STRING String,
    _In_opt_ PPH_STRING Needle
    );

PCWSTR AtModuleTypeString(
    _In_ ULONG Type
    );

PCWSTR AtVerifyResultString(
    _In_ VERIFY_RESULT Result
    );

PCWSTR AtKphLevelString(
    _In_ KPH_LEVEL Level
    );

PCWSTR AtPagePriorityString(
    _In_ ULONG PagePriority
    );

PCWSTR AtIoPriorityString(
    _In_ IO_PRIORITY_HINT IoPriority
    );

PCWSTR AtPriorityClassString(
    _In_ ULONG PriorityClass
    );

PCWSTR AtMachineString(
    _In_ USHORT Machine
    );

PCWSTR AtSubsystemString(
    _In_ USHORT Subsystem
    );

VOID AtJsonAddFlagStrings(
    _In_ PVOID Object,
    _In_ PCSTR Key,
    _In_ ULONG Value,
    _In_reads_(Count) CONST ULONG* Flags,
    _In_reads_(Count) CONST PWSTR* Names,
    _In_ ULONG Count
    );

BOOLEAN AtIsMicrosoftSigned(
    _In_opt_ PPH_STRING FileName
    );

VERIFY_RESULT AtVerifyFileName(
    _In_opt_ PPH_STRING FileName,
    _Out_opt_ PPH_STRING *Signer
    );

VOID AtAddSectionInfo(
    _In_ PVOID Structured,
    _In_ PSECTION_BASIC_INFORMATION Basic,
    _In_opt_ PSECTION_IMAGE_INFORMATION Image,
    _In_opt_ PPH_STRING FileName
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

VOID AtEventInvokeTool(
    _In_ PCAT_TOOL Tool,
    _In_ PAT_TOOL_CALL Call,
    _Inout_ PAT_TARGET Target,
    _Inout_ PAT_TOOL_RESULT Result
    );

VOID AtIoInvokeTool(
    _In_ PCAT_TOOL Tool,
    _In_ PAT_TOOL_CALL Call,
    _Inout_ PAT_TARGET Target,
    _Inout_ PAT_TOOL_RESULT Result
    );

VOID AtDotNetInvokeTool(
    _In_ PCAT_TOOL Tool,
    _In_ PAT_TOOL_CALL Call,
    _Inout_ PAT_TARGET Target,
    _Inout_ PAT_TOOL_RESULT Result
    );

VOID AtNoteInvokeTool(
    _In_ PCAT_TOOL Tool,
    _In_ PAT_TOOL_CALL Call,
    _Inout_ PAT_TARGET Target,
    _Inout_ PAT_TOOL_RESULT Result
    );

VOID AtWindowInvokeTool(
    _In_ PCAT_TOOL Tool,
    _In_ PAT_TOOL_CALL Call,
    _Inout_ PAT_TARGET Target,
    _Inout_ PAT_TOOL_RESULT Result
    );

VOID AtDiskInvokeTool(
    _In_ PCAT_TOOL Tool,
    _In_ PAT_TOOL_CALL Call,
    _Inout_ PAT_TARGET Target,
    _Inout_ PAT_TOOL_RESULT Result
    );

VOID AtAdapterInvokeTool(
    _In_ PCAT_TOOL Tool,
    _In_ PAT_TOOL_CALL Call,
    _Inout_ PAT_TARGET Target,
    _Inout_ PAT_TOOL_RESULT Result
    );

VOID AtDeviceInvokeTool(
    _In_ PCAT_TOOL Tool,
    _In_ PAT_TOOL_CALL Call,
    _Inout_ PAT_TARGET Target,
    _Inout_ PAT_TOOL_RESULT Result
    );

VOID AtGpuInvokeTool(
    _In_ PCAT_TOOL Tool,
    _In_ PAT_TOOL_CALL Call,
    _Inout_ PAT_TARGET Target,
    _Inout_ PAT_TOOL_RESULT Result
    );

VOID AtHistoryInvokeTool(
    _In_ PCAT_TOOL Tool,
    _In_ PAT_TOOL_CALL Call,
    _Inout_ PAT_TARGET Target,
    _Inout_ PAT_TOOL_RESULT Result
    );

VOID AtMitigationInvokeTool(
    _In_ PCAT_TOOL Tool,
    _In_ PAT_TOOL_CALL Call,
    _Inout_ PAT_TARGET Target,
    _Inout_ PAT_TOOL_RESULT Result
    );

VOID AtModuleInvokeTool(
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

VOID AtSessionInvokeTool(
    _In_ PCAT_TOOL Tool,
    _In_ PAT_TOOL_CALL Call,
    _Inout_ PAT_TARGET Target,
    _Inout_ PAT_TOOL_RESULT Result
    );

VOID AtTaskInvokeTool(
    _In_ PCAT_TOOL Tool,
    _In_ PAT_TOOL_CALL Call,
    _Inout_ PAT_TARGET Target,
    _Inout_ PAT_TOOL_RESULT Result
    );

VOID AtWmiInvokeTool(
    _In_ PCAT_TOOL Tool,
    _In_ PAT_TOOL_CALL Call,
    _Inout_ PAT_TARGET Target,
    _Inout_ PAT_TOOL_RESULT Result
    );

VOID AtSecurityInvokeTool(
    _In_ PCAT_TOOL Tool,
    _In_ PAT_TOOL_CALL Call,
    _Inout_ PAT_TARGET Target,
    _Inout_ PAT_TOOL_RESULT Result
    );

VOID AtWaitInvokeTool(
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

VOID AtHandleInvokeTool(
    _In_ PCAT_TOOL Tool,
    _In_ PAT_TOOL_CALL Call,
    _Inout_ PAT_TARGET Target,
    _Inout_ PAT_TOOL_RESULT Result
    );

VOID AtFindInvokeTool(
    _In_ PCAT_TOOL Tool,
    _In_ PAT_TOOL_CALL Call,
    _Inout_ PAT_TARGET Target,
    _Inout_ PAT_TOOL_RESULT Result
    );

VOID AtFirewallInvokeTool(
    _In_ PCAT_TOOL Tool,
    _In_ PAT_TOOL_CALL Call,
    _Inout_ PAT_TARGET Target,
    _Inout_ PAT_TOOL_RESULT Result
    );

VOID AtEgressInvokeTool(
    _In_ PCAT_TOOL Tool,
    _In_ PAT_TOOL_CALL Call,
    _Inout_ PAT_TARGET Target,
    _Inout_ PAT_TOOL_RESULT Result
    );

// The NetworkTools plugin interface, or NULL when that plugin is absent or too old (toolnet.c).
PNETWORKTOOLS_INTERFACE AtGetNetworkToolsInterface(
    VOID
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

ULONG AtpParseImageSections(
    _In_opt_ PVOID Sections,
    _Out_ PPH_STRING* Invalid
    );

PPH_STRING AtHashFileSha256(
    _In_ PPH_STRING FileName
    );

PONLINECHECKS_INTERFACE AtGetOnlineChecksInterface(
    VOID
    );

VOID AtpReadRegistryKey(
    _In_ PAT_TOOL_CALL Call,
    _Inout_ PAT_TOOL_RESULT Result
    );

VOID AtpLookupFileHashVirusTotal(
    _In_ PAT_TOOL_CALL Call,
    _Inout_ PAT_TOOL_RESULT Result
    );

VOID AtpLookupFileHashHybridAnalysis(
    _In_ PAT_TOOL_CALL Call,
    _Inout_ PAT_TOOL_RESULT Result
    );

VOID AtpGetFileScanResultCached(
    _In_ PAT_TOOL_CALL Call,
    _Inout_ PAT_TOOL_RESULT Result
    );

VOID AtpGetFileInfo(
    _In_ PAT_TOOL_CALL Call,
    _Inout_ PAT_TOOL_RESULT Result
    );

VOID AtpListDirectory(
    _In_ PAT_TOOL_CALL Call,
    _Inout_ PAT_TOOL_RESULT Result
    );

VOID AtpGetImageStrings(
    _In_ PAT_TOOL_CALL Call,
    _Inout_ PAT_TOOL_RESULT Result
    );

PPH_STRING AtGetImageImphash(
    _In_ PVOID MappedImage
    );

VOID AtAddImageSections(
    _In_ PVOID Structured,
    _In_ PVOID MappedImage,
    _In_ PPH_STRING FileName,
    _In_ ULONG Sections,
    _In_ PAT_TOOL_CALL Call
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

NTSTATUS AtResolveHandleTarget(
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

PPH_STRING AtFormatThreadCreateTime(
    _In_ HANDLE ThreadHandle
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

// What a session grant for this action covers, in the user's words; NULL when the grant is the
// action itself.
PCWSTR AtConsentClassDescription(
    _In_ AT_CONSENT_CLASS Class
    );

// Drops every session grant this connection holds, without disconnecting it.
VOID AtConsentRevokeGrants(
    _In_ ULONG ConnectionId
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
