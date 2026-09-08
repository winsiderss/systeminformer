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

#define AT_SERVICE_STOP_WAIT_MS 30000
#define AT_SERVICE_STOP_POLL_MS 250

BOOLEAN AtParseServiceStartType(
    _In_opt_ PPH_STRING String,
    _Out_ PULONG StartType
    )
{
    static CONST struct { PCWSTR Name; ULONG Value; } table[] =
    {
        { L"boot", SERVICE_BOOT_START },
        { L"system", SERVICE_SYSTEM_START },
        { L"auto", SERVICE_AUTO_START },
        { L"demand", SERVICE_DEMAND_START },
        { L"disabled", SERVICE_DISABLED },
    };
    ULONG i;

    if (!String)
        return FALSE;

    for (i = 0; i < RTL_NUMBER_OF(table); i++)
    {
        if (PhEqualStringZ(String->Buffer, table[i].Name, TRUE))
        {
            *StartType = table[i].Value;
            return TRUE;
        }
    }

    return FALSE;
}

PPH_STRING AtFormatServiceConfigParameter(
    _In_opt_ PVOID Arguments,
    _Inout_ PAT_TOOL_RESULT Result
    )
{
    PPH_STRING startTypeString;
    PVOID delayedMember;
    ULONG startType;
    PH_STRING_BUILDER builder;

    startTypeString = AtGetArgumentString(Arguments, "start_type");
    delayedMember = AtJsonGetObjectMember(Arguments, "delayed_auto_start", PH_JSON_OBJECT_TYPE_BOOLEAN);

    if (!startTypeString && !delayedMember)
    {
        AtSetToolError(Result, "invalid_arguments", STATUS_INVALID_PARAMETER, L"At least one of start_type and delayed_auto_start is required.");
        return NULL;
    }

    if (startTypeString && !AtParseServiceStartType(startTypeString, &startType))
    {
        AtSetToolError(Result, "invalid_arguments", STATUS_INVALID_PARAMETER, L"start_type must be one of boot, system, auto, demand, disabled.");
        PhClearReference(&startTypeString);
        return NULL;
    }

    PhInitializeStringBuilder(&builder, 64);

    if (startTypeString)
        PhAppendFormatStringBuilder(&builder, L"start type %s", startTypeString->Buffer);

    if (delayedMember)
    {
        if (startTypeString)
            PhAppendStringBuilder2(&builder, L", ");

        PhAppendStringBuilder2(&builder, AtJsonGetObjectBoolean(Arguments, "delayed_auto_start") ? L"delayed start on" : L"delayed start off");
    }

    PhClearReference(&startTypeString);

    return PhFinalStringBuilderString(&builder);
}

PCWSTR AtpServiceTypeString(
    _In_ ULONG Type
    )
{
    if (Type & SERVICE_KERNEL_DRIVER)
        return L"kernel_driver";
    if (Type & SERVICE_FILE_SYSTEM_DRIVER)
        return L"file_system_driver";
    if (Type & SERVICE_WIN32_OWN_PROCESS)
        return L"own_process";
    if (Type & SERVICE_WIN32_SHARE_PROCESS)
        return L"share_process";
    if (Type & SERVICE_USER_OWN_PROCESS)
        return L"user_own_process";
    if (Type & SERVICE_USER_SHARE_PROCESS)
        return L"user_share_process";

    return NULL;
}

VOID AtpAddControlsAccepted(
    _In_ PVOID Object,
    _In_ ULONG ControlsAccepted
    )
{
    PVOID controls = PhCreateJsonArray();

    if (ControlsAccepted & SERVICE_ACCEPT_STOP)
        PhAddJsonArrayObject(controls, PhCreateJsonStringObject("stop"));
    if (ControlsAccepted & SERVICE_ACCEPT_PAUSE_CONTINUE)
        PhAddJsonArrayObject(controls, PhCreateJsonStringObject("pause_continue"));
    if (ControlsAccepted & SERVICE_ACCEPT_SHUTDOWN)
        PhAddJsonArrayObject(controls, PhCreateJsonStringObject("shutdown"));
    if (ControlsAccepted & SERVICE_ACCEPT_PARAMCHANGE)
        PhAddJsonArrayObject(controls, PhCreateJsonStringObject("param_change"));
    if (ControlsAccepted & SERVICE_ACCEPT_NETBINDCHANGE)
        PhAddJsonArrayObject(controls, PhCreateJsonStringObject("net_bind_change"));
    if (ControlsAccepted & SERVICE_ACCEPT_PRESHUTDOWN)
        PhAddJsonArrayObject(controls, PhCreateJsonStringObject("preshutdown"));

    PhAddJsonObjectValue(Object, "controls_accepted", controls);
}

VOID AtpFillServiceRow(
    _In_ PVOID Object,
    _In_ PPH_SERVICE_ITEM ServiceItem
    )
{
    AtJsonAddString(Object, "name", ServiceItem->Name);
    AtJsonAddString(Object, "display_name", ServiceItem->DisplayName);
    AtJsonAddStringZ(Object, "type", AtpServiceTypeString(ServiceItem->Type));
    PhAddJsonObjectBoolean(Object, "is_driver", !!(ServiceItem->Type & SERVICE_DRIVER));
    AtJsonAddStringRef(Object, "state", PhGetServiceStateString(ServiceItem->State));
    AtJsonAddStringRef(Object, "start_type", PhGetServiceStartTypeString(ServiceItem->StartType));

    if (ServiceItem->ProcessId)
    {
        PPH_PROCESS_ITEM processItem;

        PhAddJsonObjectUInt64(Object, "pid", HandleToUlong(ServiceItem->ProcessId));

        if (processItem = PhReferenceProcessItem(ServiceItem->ProcessId))
        {
            PhAddJsonObjectUInt64(Object, "process_sequence_number", processItem->ProcessSequenceNumber);
            PhDereferenceObject(processItem);
        }
        else
        {
            AtJsonAddNull(Object, "process_sequence_number");
        }
    }
    else
    {
        AtJsonAddNull(Object, "pid");
        AtJsonAddNull(Object, "process_sequence_number");
    }

    AtJsonAddString(Object, "image_path", ServiceItem->FileName);
    AtJsonAddStringZ(Object, "verify_result", AtVerifyResultString(ServiceItem->VerifyResult));
    AtJsonAddString(Object, "verify_signer", ServiceItem->VerifySignerName);
    PhAddJsonObjectBoolean(Object, "runs_in_system_process", !!(ServiceItem->Flags & SERVICE_RUNS_IN_SYSTEM_PROCESS));
}

typedef struct _AT_SERVICE_FILTER
{
    PPH_STRING NameContains;
    BOOLEAN HaveState;
    ULONG State;
    BOOLEAN HaveType;
    BOOLEAN Driver;
    BOOLEAN HavePid;
    HANDLE Pid;
} AT_SERVICE_FILTER, *PAT_SERVICE_FILTER;

BOOLEAN AtpServiceMatchesFilter(
    _In_ PAT_SERVICE_FILTER Filter,
    _In_ PPH_SERVICE_ITEM ServiceItem
    )
{
    if (Filter->NameContains &&
        !AtContainsString(ServiceItem->Name, Filter->NameContains) &&
        !AtContainsString(ServiceItem->DisplayName, Filter->NameContains))
    {
        return FALSE;
    }

    if (Filter->HaveState)
    {
        if (Filter->State == MAXULONG)
        {
            // "pending" covers every transitional state.
            if (ServiceItem->State == SERVICE_RUNNING || ServiceItem->State == SERVICE_STOPPED || ServiceItem->State == SERVICE_PAUSED)
                return FALSE;
        }
        else if (ServiceItem->State != Filter->State)
        {
            return FALSE;
        }
    }

    if (Filter->HaveType && (!!(ServiceItem->Type & SERVICE_DRIVER)) != Filter->Driver)
        return FALSE;

    if (Filter->HavePid && ServiceItem->ProcessId != Filter->Pid)
        return FALSE;

    return TRUE;
}

VOID AtpListServices(
    _In_ PAT_TOOL_CALL Call,
    _Inout_ PAT_TOOL_RESULT Result
    )
{
    AT_SERVICE_FILTER filter;
    LPENUM_SERVICE_STATUS_PROCESS services;
    ULONG numberOfServiceItems;
    PPH_STRING state;
    PPH_STRING type;
    ULONG64 pid;
    AT_ROWS rows;
    PVOID structured;
    ULONG i;

    memset(&filter, 0, sizeof(AT_SERVICE_FILTER));

    if (Call->Arguments)
    {
        filter.NameContains = AtGetArgumentString(Call->Arguments, "name_contains");

        if (state = AtGetArgumentString(Call->Arguments, "state"))
        {
            filter.HaveState = TRUE;

            if (PhEqualStringZ(state->Buffer, L"running", TRUE))
                filter.State = SERVICE_RUNNING;
            else if (PhEqualStringZ(state->Buffer, L"stopped", TRUE))
                filter.State = SERVICE_STOPPED;
            else if (PhEqualStringZ(state->Buffer, L"paused", TRUE))
                filter.State = SERVICE_PAUSED;
            else
                filter.State = MAXULONG;

            PhDereferenceObject(state);
        }

        if (type = AtGetArgumentString(Call->Arguments, "type"))
        {
            filter.HaveType = TRUE;
            filter.Driver = PhEqualStringZ(type->Buffer, L"driver", TRUE);
            PhDereferenceObject(type);
        }

        if (AtGetArgumentUInt64(Call->Arguments, "pid", &pid) && pid <= MAXULONG)
        {
            filter.HavePid = TRUE;
            filter.Pid = UlongToHandle((ULONG)pid);
        }
    }

    // The service control manager is the authoritative name list; the provider cache is the value
    // (signature, flags), so each SCM service is enriched from the cached item when present.
    if (!NT_SUCCESS(PhEnumServices(&services, &numberOfServiceItems)))
    {
        AtSetToolError(Result, "failed", STATUS_UNSUCCESSFUL, L"The service control manager could not be enumerated.");
        PhClearReference(&filter.NameContains);
        return;
    }

    structured = PhCreateJsonObject();
    AtInitializeRows(&rows, Call->Arguments);

    for (i = 0; i < numberOfServiceItems; i++)
    {
        PPH_SERVICE_ITEM serviceItem;
        PVOID row;

        if (!(serviceItem = PhReferenceServiceItemZ(services[i].lpServiceName)))
            continue;

        if (!AtpServiceMatchesFilter(&filter, serviceItem))
        {
            PhDereferenceObject(serviceItem);
            continue;
        }

        row = PhCreateJsonObject();
        AtpFillServiceRow(row, serviceItem);
        AtAddRow(&rows, row);

        PhDereferenceObject(serviceItem);
    }

    PhFree(services);

    AtAddRows(structured, "services", &rows);
    AtAddSnapshot(structured);

    Result->StructuredContent = structured;

    PhClearReference(&filter.NameContains);
}

VOID AtpGetService(
    _In_ PAT_TOOL_CALL Call,
    _Inout_ PAT_TOOL_RESULT Result
    )
{
    PPH_STRING name;
    PPH_SERVICE_ITEM serviceItem;
    SC_HANDLE serviceHandle;
    PVOID structured;
    BOOLEAN accessDenied = FALSE;

    if (!(name = AtGetArgumentString(Call->Arguments, "name")) || name->Length == 0)
    {
        AtSetToolError(Result, "invalid_arguments", STATUS_INVALID_PARAMETER, L"name is required and must be the service name.");
        PhClearReference(&name);
        return;
    }

    if (!(serviceItem = PhReferenceServiceItem(&name->sr)))
    {
        AtSetToolError(Result, "not_found", STATUS_NOT_FOUND, L"No service named %s is in the provider cache.", PhGetString(name));
        PhDereferenceObject(name);
        return;
    }

    PhDereferenceObject(name);

    structured = PhCreateJsonObject();
    AtpFillServiceRow(structured, serviceItem);

    if (NT_SUCCESS(PhOpenService(&serviceHandle, SERVICE_QUERY_CONFIG | SERVICE_QUERY_STATUS, PhGetString(serviceItem->Name))))
    {
        LPQUERY_SERVICE_CONFIG config;
        PPH_STRING description;
        SERVICE_STATUS_PROCESS status;
        BOOLEAN delayedAutoStart;

        if (description = PhGetServiceDescription(serviceHandle))
        {
            AtJsonAddString(structured, "description", description);
            PhDereferenceObject(description);
        }
        else
        {
            AtJsonAddNull(structured, "description");
        }

        if (NT_SUCCESS(PhGetServiceConfig(serviceHandle, &config)))
        {
            AtJsonAddStringZ(structured, "binary_path", config->lpBinaryPathName);
            AtJsonAddStringZ(structured, "account", config->lpServiceStartName);
            AtJsonAddStringRef(structured, "error_control", PhGetServiceErrorControlString(config->dwErrorControl));
            AtJsonAddStringZ(structured, "load_order_group", config->lpLoadOrderGroup && config->lpLoadOrderGroup[0] ? config->lpLoadOrderGroup : NULL);

            if (config->dwTagId)
                PhAddJsonObjectUInt64(structured, "tag_id", config->dwTagId);
            else
                AtJsonAddNull(structured, "tag_id");

            {
                PVOID dependencies = PhCreateJsonArray();
                PWSTR dependency = config->lpDependencies;

                // Double-null-terminated list of dependency names.
                if (dependency)
                {
                    while (*dependency)
                    {
                        PH_STRINGREF sr;
                        PPH_BYTES utf8;

                        PhInitializeStringRef(&sr, dependency);

                        if (utf8 = PhConvertUtf16ToUtf8Ex(sr.Buffer, sr.Length))
                        {
                            PhAddJsonArrayObject(dependencies, PhCreateJsonStringObject(utf8->Buffer));
                            PhDereferenceObject(utf8);
                        }

                        dependency += sr.Length / sizeof(WCHAR) + 1;
                    }
                }

                PhAddJsonObjectValue(structured, "dependencies", dependencies);
            }

            PhFree(config);
        }
        else
        {
            accessDenied = TRUE;
            AtJsonAddNull(structured, "binary_path");
            AtJsonAddNull(structured, "account");
            AtJsonAddNull(structured, "error_control");
            AtJsonAddNull(structured, "load_order_group");
            AtJsonAddNull(structured, "tag_id");
            PhAddJsonObjectValue(structured, "dependencies", PhCreateJsonArray());
        }

        if (NT_SUCCESS(PhGetServiceDelayedAutoStart(serviceHandle, &delayedAutoStart)))
            PhAddJsonObjectBoolean(structured, "delayed_auto_start", delayedAutoStart);
        else
            AtJsonAddNull(structured, "delayed_auto_start");

        if (NT_SUCCESS(PhQueryServiceStatus(serviceHandle, &status)))
        {
            AtpAddControlsAccepted(structured, status.dwControlsAccepted);
            PhAddJsonObjectUInt64(structured, "exit_code", status.dwWin32ExitCode);
            PhAddJsonObjectUInt64(structured, "service_specific_exit_code", status.dwServiceSpecificExitCode);
        }
        else
        {
            PhAddJsonObjectValue(structured, "controls_accepted", PhCreateJsonArray());
            PhAddJsonObjectUInt64(structured, "exit_code", serviceItem->Win32ExitCode);
            PhAddJsonObjectUInt64(structured, "service_specific_exit_code", serviceItem->ServiceSpecificExitCode);
        }

        PhCloseServiceHandle(serviceHandle);
    }
    else
    {
        accessDenied = TRUE;
        AtJsonAddNull(structured, "description");
        AtJsonAddNull(structured, "binary_path");
        AtJsonAddNull(structured, "account");
        AtJsonAddNull(structured, "error_control");
        AtJsonAddNull(structured, "load_order_group");
        AtJsonAddNull(structured, "tag_id");
        PhAddJsonObjectValue(structured, "dependencies", PhCreateJsonArray());
        AtJsonAddNull(structured, "delayed_auto_start");
        PhAddJsonObjectValue(structured, "controls_accepted", PhCreateJsonArray());
        PhAddJsonObjectUInt64(structured, "exit_code", 0);
        PhAddJsonObjectUInt64(structured, "service_specific_exit_code", 0);
    }

    PhAddJsonObjectBoolean(structured, "access_denied", accessDenied);
    AtAddSnapshot(structured);

    Result->StructuredContent = structured;

    PhDereferenceObject(serviceItem);
}

NTSTATUS AtpWaitForServiceStop(
    _In_ SC_HANDLE ServiceHandle
    )
{
    ULONG64 startTick = NtGetTickCount64();
    SERVICE_STATUS_PROCESS status;

    while (NT_SUCCESS(PhQueryServiceStatus(ServiceHandle, &status)))
    {
        if (status.dwCurrentState == SERVICE_STOPPED)
            return STATUS_SUCCESS;

        if (NtGetTickCount64() - startTick > AT_SERVICE_STOP_WAIT_MS)
            return STATUS_TIMEOUT;

        PhDelayExecution(AT_SERVICE_STOP_POLL_MS);
    }

    return STATUS_SUCCESS; // could not query; assume the caller's stop succeeded
}

VOID AtpControlService(
    _In_ PCAT_TOOL Tool,
    _In_ PAT_TOOL_CALL Call,
    _In_ PAT_TARGET Target,
    _Inout_ PAT_TOOL_RESULT Result
    )
{
    NTSTATUS status;
    SC_HANDLE serviceHandle = Target->ServiceHandle;
    PPH_SERVICE_ITEM serviceItem = Target->ServiceItem;
    SERVICE_STATUS_PROCESS serviceStatus;
    PVOID structured;

    switch (Tool->Action)
    {
    case AtActionStartService:
        status = PhStartService(serviceHandle, 0, NULL);
        break;
    case AtActionStopService:
        status = PhStopService(serviceHandle);
        break;
    case AtActionRestartService:
        status = PhStopService(serviceHandle);

        if (NT_SUCCESS(status))
        {
            status = AtpWaitForServiceStop(serviceHandle);

            if (NT_SUCCESS(status))
                status = PhStartService(serviceHandle, 0, NULL);
        }
        break;
    case AtActionSetServiceConfig:
        {
            PPH_STRING startTypeString;
            PVOID delayedMember;
            NTSTATUS configStatus = STATUS_SUCCESS;

            startTypeString = AtGetArgumentString(Call->Arguments, "start_type");
            delayedMember = AtJsonGetObjectMember(Call->Arguments, "delayed_auto_start", PH_JSON_OBJECT_TYPE_BOOLEAN);

            if (startTypeString)
            {
                ULONG startType;

                NT_VERIFY(AtParseServiceStartType(startTypeString, &startType));
                configStatus = PhChangeServiceConfig(
                    serviceHandle, SERVICE_NO_CHANGE, startType, SERVICE_NO_CHANGE,
                    NULL, NULL, NULL, NULL, NULL, NULL, NULL
                    );
                PhDereferenceObject(startTypeString);
            }

            if (NT_SUCCESS(configStatus) && delayedMember)
                configStatus = PhSetServiceDelayedAutoStart(serviceHandle, AtJsonGetObjectBoolean(Call->Arguments, "delayed_auto_start"));

            status = configStatus;
        }
        break;
    default:
        status = STATUS_NOT_IMPLEMENTED;
        break;
    }

    if (!NT_SUCCESS(status))
    {
        AtSetToolStatusError(Result, status, L"The operation");
        return;
    }

    structured = PhCreateJsonObject();
    AtJsonAddString(structured, "name", serviceItem->Name);
    AtJsonAddString(structured, "display_name", serviceItem->DisplayName);
    PhAddJsonObject(structured, "action", Tool->Name);

    if (Tool->Action == AtActionSetServiceConfig)
    {
        LPQUERY_SERVICE_CONFIG config;
        BOOLEAN delayedAutoStart;

        if (NT_SUCCESS(PhGetServiceConfig(serviceHandle, &config)))
        {
            AtJsonAddStringRef(structured, "start_type", PhGetServiceStartTypeString(config->dwStartType));
            PhFree(config);
        }
        else
        {
            AtJsonAddNull(structured, "start_type");
        }

        if (NT_SUCCESS(PhGetServiceDelayedAutoStart(serviceHandle, &delayedAutoStart)))
            PhAddJsonObjectBoolean(structured, "delayed_auto_start", delayedAutoStart);
        else
            AtJsonAddNull(structured, "delayed_auto_start");
    }
    else
    {
        if (NT_SUCCESS(PhQueryServiceStatus(serviceHandle, &serviceStatus)))
        {
            AtJsonAddStringRef(structured, "state", PhGetServiceStateString(serviceStatus.dwCurrentState));

            if (serviceStatus.dwProcessId)
                PhAddJsonObjectUInt64(structured, "pid", serviceStatus.dwProcessId);
            else
                AtJsonAddNull(structured, "pid");
        }
        else
        {
            AtJsonAddNull(structured, "state");
            AtJsonAddNull(structured, "pid");
        }
    }

    AtAddSnapshot(structured);

    Result->StructuredContent = structured;
}

VOID AtServiceInvokeTool(
    _In_ PCAT_TOOL Tool,
    _In_ PAT_TOOL_CALL Call,
    _Inout_ PAT_TARGET Target,
    _Inout_ PAT_TOOL_RESULT Result
    )
{
    switch (Tool->Action)
    {
    case AtActionListServices:
        AtpListServices(Call, Result);
        break;
    case AtActionGetService:
        AtpGetService(Call, Result);
        break;
    case AtActionStartService:
    case AtActionStopService:
    case AtActionRestartService:
    case AtActionSetServiceConfig:
        AtpControlService(Tool, Call, Target, Result);
        break;
    default:
        AtSetToolError(Result, "failed", STATUS_NOT_IMPLEMENTED, L"This tool is not implemented.");
        break;
    }
}
