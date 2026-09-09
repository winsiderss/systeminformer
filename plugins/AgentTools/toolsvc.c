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

// The service manager keeps a description of its own; this is well past anything a real one
// carries and stops a caller writing a novel into the registry.
#define AT_SERVICE_DESCRIPTION_MAXIMUM 1024
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
    PPH_STRING description;
    PVOID delayedMember;
    ULONG startType;
    PH_STRING_BUILDER builder;

    startTypeString = AtGetArgumentString(Arguments, "start_type");
    delayedMember = AtJsonGetObjectMember(Arguments, "delayed_auto_start", PH_JSON_OBJECT_TYPE_BOOLEAN);
    description = AtGetArgumentString(Arguments, "description");

    if (!startTypeString && !delayedMember && !description)
    {
        AtSetToolError(Result, "invalid_arguments", STATUS_INVALID_PARAMETER,
            L"At least one of start_type, delayed_auto_start and description is required.");
        return NULL;
    }

    if (description && description->Length > AT_SERVICE_DESCRIPTION_MAXIMUM * sizeof(WCHAR))
    {
        AtSetToolError(Result, "invalid_arguments", STATUS_INVALID_PARAMETER,
            L"description must be %lu characters or fewer.", (ULONG)AT_SERVICE_DESCRIPTION_MAXIMUM);
        PhClearReference(&description);
        PhClearReference(&startTypeString);
        return NULL;
    }

    if (startTypeString && !AtParseServiceStartType(startTypeString, &startType))
    {
        AtSetToolError(Result, "invalid_arguments", STATUS_INVALID_PARAMETER, L"start_type must be one of boot, system, auto, demand, disabled.");
        PhClearReference(&description);
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

    // The text is what the user is asked to approve, so the description goes in it whole: a
    // change to what a service says about itself is only reviewable if it can be read.
    if (description)
    {
        if (startTypeString || delayedMember)
            PhAppendStringBuilder2(&builder, L", ");

        PhAppendFormatStringBuilder(&builder, L"description \"%s\"", description->Buffer);
    }

    PhClearReference(&description);
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
    PhAddJsonObjectBoolean(Object, "details_available", TRUE);
}

/**
 * Fills a row from the service control manager's own answer, for a service the provider cache has
 * not seen yet. The SCM carries the identity and the state; everything the cache adds is null.
 */
VOID AtpFillServiceStatusRow(
    _In_ PVOID Object,
    _In_ LPENUM_SERVICE_STATUS_PROCESS Service
    )
{
    ULONG type = Service->ServiceStatusProcess.dwServiceType;
    ULONG processId = Service->ServiceStatusProcess.dwProcessId;

    AtJsonAddStringZ(Object, "name", Service->lpServiceName);
    AtJsonAddStringZ(Object, "display_name", Service->lpDisplayName);
    AtJsonAddStringZ(Object, "type", AtpServiceTypeString(type));
    PhAddJsonObjectBoolean(Object, "is_driver", !!(type & SERVICE_DRIVER));
    AtJsonAddStringRef(Object, "state", PhGetServiceStateString(Service->ServiceStatusProcess.dwCurrentState));
    AtJsonAddNull(Object, "start_type");

    if (processId)
        PhAddJsonObjectUInt64(Object, "pid", processId);
    else
        AtJsonAddNull(Object, "pid");

    AtJsonAddNull(Object, "process_sequence_number");
    AtJsonAddNull(Object, "image_path");
    AtJsonAddNull(Object, "verify_result");
    AtJsonAddNull(Object, "verify_signer");
    AtJsonAddNull(Object, "runs_in_system_process");
    PhAddJsonObjectBoolean(Object, "details_available", FALSE);
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
    BOOLEAN ExcludeMicrosoft;
    BOOLEAN UnsignedOnly;
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

    // Last, and only when asked: each of these verifies the service image on disk, so the cheap
    // filters above have already thrown away everything they can.
    if (Filter->ExcludeMicrosoft && AtIsMicrosoftSigned(ServiceItem->FileName, NULL))
        return FALSE;

    if (Filter->UnsignedOnly && AtVerifyFileName(ServiceItem->FileName, NULL) == VrTrusted)
        return FALSE;

    return TRUE;
}

/**
 * Applies the filters that the service control manager's own answer can decide, for a service the
 * provider cache has not seen yet.
 */
BOOLEAN AtpServiceStatusMatchesFilter(
    _In_ PAT_SERVICE_FILTER Filter,
    _In_ LPENUM_SERVICE_STATUS_PROCESS Service
    )
{
    ULONG state = Service->ServiceStatusProcess.dwCurrentState;
    ULONG type = Service->ServiceStatusProcess.dwServiceType;

    if (Filter->NameContains)
    {
        PH_STRINGREF name;
        PH_STRINGREF displayName;

        PhInitializeStringRefLongHint(&name, Service->lpServiceName);

        if (Service->lpDisplayName)
            PhInitializeStringRefLongHint(&displayName, Service->lpDisplayName);
        else
            PhInitializeEmptyStringRef(&displayName);

        if (PhFindStringInStringRef(&name, &Filter->NameContains->sr, TRUE) == SIZE_MAX &&
            PhFindStringInStringRef(&displayName, &Filter->NameContains->sr, TRUE) == SIZE_MAX)
        {
            return FALSE;
        }
    }

    if (Filter->HaveState)
    {
        if (Filter->State == MAXULONG)
        {
            if (state == SERVICE_RUNNING || state == SERVICE_STOPPED || state == SERVICE_PAUSED)
                return FALSE;
        }
        else if (state != Filter->State)
        {
            return FALSE;
        }
    }

    if (Filter->HaveType && (!!(type & SERVICE_DRIVER)) != Filter->Driver)
        return FALSE;

    if (Filter->HavePid && UlongToHandle(Service->ServiceStatusProcess.dwProcessId) != Filter->Pid)
        return FALSE;

    // ExcludeMicrosoft and UnsignedOnly both verify the service image, which is one of the fields
    // only the cache holds, so a service it has not seen yet is kept rather than judged on a guess.
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
    ULONG64 sinceSnapshotId;
    AT_ROWS rows;
    PVOID structured;
    BOOLEAN verifySignatures;
    ULONG i;
    ULONG uncachedCount = 0;
    NTSTATUS status;

    memset(&filter, 0, sizeof(AT_SERVICE_FILTER));
    verifySignatures = AtJsonGetObjectBoolean(Call->Arguments, "verify_signatures");

    if (Call->Arguments)
    {
        filter.NameContains = AtGetArgumentString(Call->Arguments, "name_contains");
        filter.ExcludeMicrosoft = AtJsonGetObjectBoolean(Call->Arguments, "exclude_microsoft");
        filter.UnsignedOnly = AtJsonGetObjectBoolean(Call->Arguments, "unsigned_only");

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
    if (!NT_SUCCESS(status = PhEnumServices(&services, &numberOfServiceItems)))
    {
        AtSetToolStatusError(Result, status, L"Enumerating the service control manager");
        PhClearReference(&filter.NameContains);
        return;
    }

    structured = PhCreateJsonObject();
    AtInitializeRows(&rows, Call->Arguments);

    for (i = 0; i < numberOfServiceItems; i++)
    {
        PPH_SERVICE_ITEM serviceItem;
        PVOID row;

        // A service the cache has not seen yet - typically one created since the last provider
        // tick - is still a service the SCM lists, so it is reported from what the SCM said rather
        // than dropped out of the listing and the counts alike.
        if (!(serviceItem = PhReferenceServiceItemZ(services[i].lpServiceName)))
        {
            if (!AtpServiceStatusMatchesFilter(&filter, &services[i]))
                continue;

            uncachedCount++;

            row = PhCreateJsonObject();
            AtpFillServiceStatusRow(row, &services[i]);

            if (verifySignatures)
                AtJsonAddMicrosoftSigned(row, "is_microsoft_signed", NULL);

            AtAddRow(&rows, row);
            continue;
        }

        if (!AtpServiceMatchesFilter(&filter, serviceItem))
        {
            PhDereferenceObject(serviceItem);
            continue;
        }

        row = PhCreateJsonObject();
        AtpFillServiceRow(row, serviceItem);

        // A verification per service, so only when the caller asked for the field or filtered on it.
        if (verifySignatures)
            AtJsonAddMicrosoftSigned(row, "is_microsoft_signed", serviceItem->FileName);

        AtAddRow(&rows, row);

        PhDereferenceObject(serviceItem);
    }

    PhFree(services);

    AtAddRows(structured, "services", &rows);
    PhAddJsonObjectUInt64(structured, "uncached_count", uncachedCount);

    if (AtGetArgumentUInt64(Call->Arguments, "since_snapshot_id", &sinceSnapshotId))
        AtAddServiceChanges(structured, (ULONG)min(sinceSnapshotId, MAXULONG));

    AtAddSnapshot(structured);

    Result->StructuredContent = structured;

    PhClearReference(&filter.NameContains);
}

PCWSTR AtpServiceTriggerTypeString(
    _In_ ULONG Type
    )
{
    switch (Type)
    {
    case SERVICE_TRIGGER_TYPE_DEVICE_INTERFACE_ARRIVAL:
        return L"device_interface_arrival";
    case SERVICE_TRIGGER_TYPE_IP_ADDRESS_AVAILABILITY:
        return L"ip_address_availability";
    case SERVICE_TRIGGER_TYPE_DOMAIN_JOIN:
        return L"domain_join";
    case SERVICE_TRIGGER_TYPE_FIREWALL_PORT_EVENT:
        return L"firewall_port_event";
    case SERVICE_TRIGGER_TYPE_GROUP_POLICY:
        return L"group_policy";
    case SERVICE_TRIGGER_TYPE_NETWORK_ENDPOINT:
        return L"network_endpoint";
    case SERVICE_TRIGGER_TYPE_CUSTOM_SYSTEM_STATE_CHANGE:
        return L"custom_system_state_change";
    case SERVICE_TRIGGER_TYPE_CUSTOM:
        return L"custom";
    case SERVICE_TRIGGER_TYPE_AGGREGATE:
        return L"aggregate";
    }

    return NULL;
}

PCWSTR AtpServiceSidTypeString(
    _In_ ULONG SidType
    )
{
    switch (SidType)
    {
    case SERVICE_SID_TYPE_NONE:
        return L"none";
    case SERVICE_SID_TYPE_UNRESTRICTED:
        return L"unrestricted";
    case SERVICE_SID_TYPE_RESTRICTED:
        return L"restricted";
    }

    return NULL;
}

PCWSTR AtpServiceActionString(
    _In_ ULONG Action
    )
{
    switch (Action)
    {
    case SC_ACTION_NONE:
        return L"none";
    case SC_ACTION_RESTART:
        return L"restart";
    case SC_ACTION_REBOOT:
        return L"reboot";
    case SC_ACTION_RUN_COMMAND:
        return L"run_command";
    }

    return NULL;
}

VOID AtpAddServiceTriggers(
    _In_ PVOID Object,
    _In_ SC_HANDLE ServiceHandle
    )
{
    PSERVICE_TRIGGER_INFO triggerInfo;
    PVOID array;
    ULONG i;

    if (!NT_SUCCESS(PhQueryServiceVariableSize(ServiceHandle, SERVICE_CONFIG_TRIGGER_INFO, &triggerInfo)))
    {
        AtJsonAddNull(Object, "triggers");
        return;
    }

    array = PhCreateJsonArray();

    for (i = 0; i < triggerInfo->cTriggers; i++)
    {
        PSERVICE_TRIGGER trigger = &triggerInfo->pTriggers[i];
        PVOID entry;

        entry = PhCreateJsonObject();
        AtJsonAddStringZ(entry, "type", AtpServiceTriggerTypeString(trigger->dwTriggerType));
        PhAddJsonObject(entry, "action", trigger->dwAction == SERVICE_TRIGGER_ACTION_SERVICE_START ? "start" : "stop");

        if (trigger->pTriggerSubtype)
        {
            PPH_STRING guid = PhFormatGuid(trigger->pTriggerSubtype);

            AtJsonAddString(entry, "subtype", guid);
            PhClearReference(&guid);
        }
        else
        {
            AtJsonAddNull(entry, "subtype");
        }

        PhAddJsonObjectUInt64(entry, "data_item_count", trigger->cDataItems);
        PhAddJsonArrayObject(array, entry);
    }

    PhAddJsonObjectValue(Object, "triggers", array);
    PhFree(triggerInfo);
}

VOID AtpAddServiceRecovery(
    _In_ PVOID Object,
    _In_ SC_HANDLE ServiceHandle
    )
{
    LPSERVICE_FAILURE_ACTIONS failureActions;
    LPSERVICE_FAILURE_ACTIONS_FLAG failureFlag;
    PVOID entry;
    PVOID array;
    ULONG i;

    if (!NT_SUCCESS(PhQueryServiceVariableSize(ServiceHandle, SERVICE_CONFIG_FAILURE_ACTIONS, &failureActions)))
    {
        AtJsonAddNull(Object, "recovery");
        return;
    }

    entry = PhCreateJsonObject();
    PhAddJsonObjectUInt64(entry, "reset_period_seconds", failureActions->dwResetPeriod);
    AtJsonAddStringZ(entry, "reboot_message", failureActions->lpRebootMsg);
    AtJsonAddStringZ(entry, "command", failureActions->lpCommand);

    array = PhCreateJsonArray();

    for (i = 0; i < failureActions->cActions; i++)
    {
        PVOID action = PhCreateJsonObject();

        AtJsonAddStringZ(action, "action", AtpServiceActionString(failureActions->lpsaActions[i].Type));
        PhAddJsonObjectUInt64(action, "delay_ms", failureActions->lpsaActions[i].Delay);
        PhAddJsonArrayObject(array, action);
    }

    PhAddJsonObjectValue(entry, "actions", array);

    if (NT_SUCCESS(PhQueryServiceVariableSize(ServiceHandle, SERVICE_CONFIG_FAILURE_ACTIONS_FLAG, &failureFlag)))
    {
        PhAddJsonObjectBoolean(entry, "on_non_crash_failures", !!failureFlag->fFailureActionsOnNonCrashFailures);
        PhFree(failureFlag);
    }
    else
    {
        AtJsonAddNull(entry, "on_non_crash_failures");
    }

    PhAddJsonObjectValue(Object, "recovery", entry);
    PhFree(failureActions);
}

VOID AtpAddServiceSecurityConfig(
    _In_ PVOID Object,
    _In_ SC_HANDLE ServiceHandle
    )
{
    LPSERVICE_SID_INFO sidInfo;
    PSERVICE_LAUNCH_PROTECTED_INFO protectedInfo;
    LPSERVICE_PRESHUTDOWN_INFO preshutdownInfo;
    PWSTR privileges;

    if (NT_SUCCESS(PhQueryServiceVariableSize(ServiceHandle, SERVICE_CONFIG_SERVICE_SID_INFO, &sidInfo)))
    {
        AtJsonAddStringZ(Object, "sid_type", AtpServiceSidTypeString(sidInfo->dwServiceSidType));
        PhFree(sidInfo);
    }
    else
    {
        AtJsonAddNull(Object, "sid_type");
    }

    if (NT_SUCCESS(PhQueryServiceVariableSize(ServiceHandle, SERVICE_CONFIG_LAUNCH_PROTECTED, &protectedInfo)))
    {
        PhAddJsonObjectUInt64(Object, "launch_protected", protectedInfo->dwLaunchProtected);
        PhFree(protectedInfo);
    }
    else
    {
        AtJsonAddNull(Object, "launch_protected");
    }

    if (NT_SUCCESS(PhQueryServiceVariableSize(ServiceHandle, SERVICE_CONFIG_PRESHUTDOWN_INFO, &preshutdownInfo)))
    {
        PhAddJsonObjectUInt64(Object, "preshutdown_timeout_ms", preshutdownInfo->dwPreshutdownTimeout);
        PhFree(preshutdownInfo);
    }
    else
    {
        AtJsonAddNull(Object, "preshutdown_timeout_ms");
    }

    // A service that asks for more privileges than it needs is worth noticing.
    if (NT_SUCCESS(PhQueryServiceVariableSize(ServiceHandle, SERVICE_CONFIG_REQUIRED_PRIVILEGES_INFO, &privileges)))
    {
        LPSERVICE_REQUIRED_PRIVILEGES_INFOW info = (LPSERVICE_REQUIRED_PRIVILEGES_INFOW)privileges;
        PVOID array = PhCreateJsonArray();
        PWSTR current = info->pmszRequiredPrivileges;

        // Double-null-terminated list.
        if (current)
        {
            while (*current)
            {
                PH_STRINGREF sr;
                PPH_BYTES utf8;

                PhInitializeStringRef(&sr, current);

                if (utf8 = PhConvertUtf16ToUtf8Ex(sr.Buffer, sr.Length))
                {
                    PhAddJsonArrayObject(array, PhCreateJsonStringObject(utf8->Buffer));
                    PhDereferenceObject(utf8);
                }

                current += sr.Length / sizeof(WCHAR) + 1;
            }
        }

        PhAddJsonObjectValue(Object, "required_privileges", array);
        PhFree(privileges);
    }
    else
    {
        AtJsonAddNull(Object, "required_privileges");
    }
}

VOID AtpAddServiceDependents(
    _In_ PVOID Object,
    _In_ SC_HANDLE ServiceHandle
    )
{
    LPENUM_SERVICE_STATUS dependents;
    ULONG count;
    PVOID array;
    ULONG i;

    if (!NT_SUCCESS(PhEnumDependentServices(ServiceHandle, &dependents, &count)))
    {
        AtJsonAddNull(Object, "dependents");
        return;
    }

    array = PhCreateJsonArray();

    for (i = 0; i < count; i++)
    {
        PVOID entry = PhCreateJsonObject();

        AtJsonAddStringZ(entry, "name", dependents[i].lpServiceName);
        AtJsonAddStringZ(entry, "display_name", dependents[i].lpDisplayName);
        PhAddJsonArrayObject(array, entry);
    }

    PhAddJsonObjectValue(Object, "dependents", array);
    PhFree(dependents);
}

VOID AtpAddServiceKeyModifiedTime(
    _In_ PVOID Object,
    _In_ PPH_STRING ServiceName
    )
{
    static PH_STRINGREF servicesKeyName = PH_STRINGREF_INIT(L"System\\CurrentControlSet\\Services\\");
    HANDLE keyHandle;
    PPH_STRING keyName;
    LARGE_INTEGER lastWriteTime;

    keyName = PhConcatStringRef2(&servicesKeyName, &ServiceName->sr);

    if (NT_SUCCESS(PhOpenKey(&keyHandle, KEY_QUERY_VALUE, PH_KEY_LOCAL_MACHINE, &keyName->sr, 0)))
    {
        if (NT_SUCCESS(PhQueryKeyLastWriteTime(keyHandle, &lastWriteTime)))
            AtJsonAddTime(Object, "key_modified_time", &lastWriteTime);
        else
            AtJsonAddNull(Object, "key_modified_time");

        NtClose(keyHandle);
    }
    else
    {
        AtJsonAddNull(Object, "key_modified_time");
    }

    PhDereferenceObject(keyName);
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
    NTSTATUS openStatus;
    NTSTATUS configStatus;
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
    AtpAddServiceKeyModifiedTime(structured, serviceItem->Name);

    // Named is_microsoft_signed, like every other row that carries it.
    if (serviceItem->FileName)
        AtJsonAddMicrosoftSigned(structured, "is_microsoft_signed", serviceItem->FileName);
    else
        AtJsonAddNull(structured, "is_microsoft_signed");

    openStatus = PhOpenService(
        &serviceHandle,
        SERVICE_QUERY_CONFIG | SERVICE_QUERY_STATUS | SERVICE_ENUMERATE_DEPENDENTS,
        PhGetString(serviceItem->Name)
        );

    if (NT_SUCCESS(openStatus))
    {
        LPQUERY_SERVICE_CONFIG config;
        PPH_STRING description;
        SERVICE_STATUS_PROCESS status;
        BOOLEAN delayedAutoStart;

        AtpAddServiceTriggers(structured, serviceHandle);
        AtpAddServiceRecovery(structured, serviceHandle);
        AtpAddServiceSecurityConfig(structured, serviceHandle);
        AtpAddServiceDependents(structured, serviceHandle);

        if (description = PhGetServiceDescription(serviceHandle))
        {
            AtJsonAddString(structured, "description", description);
            PhDereferenceObject(description);
        }
        else
        {
            AtJsonAddNull(structured, "description");
        }

        if (NT_SUCCESS(configStatus = PhGetServiceConfig(serviceHandle, &config)))
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
            // Only a refusal is a refusal. A service that went away between the listing and this
            // call fails here too, and calling that access_denied sends the caller after a
            // permission it already holds.
            accessDenied = configStatus == STATUS_ACCESS_DENIED;
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
        accessDenied = openStatus == STATUS_ACCESS_DENIED;
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
    NTSTATUS queryStatus;

    while (NT_SUCCESS(queryStatus = PhQueryServiceStatus(ServiceHandle, &status)))
    {
        if (status.dwCurrentState == SERVICE_STOPPED)
            return STATUS_SUCCESS;

        // STATUS_TIMEOUT is a success code; the caller must be able to test this one.
        if (NtGetTickCount64() - startTick > AT_SERVICE_STOP_WAIT_MS)
            return STATUS_IO_TIMEOUT;

        PhDelayExecution(AT_SERVICE_STOP_POLL_MS);
    }

    return queryStatus;
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
    case AtActionPauseService:
    case AtActionContinueService:
        {
            // Most services do not implement pause, and the error for asking is "cannot accept
            // control messages at this time", which reads like a timing problem. What the service
            // accepts is in its own status.
            if (NT_SUCCESS(status = PhQueryServiceStatus(serviceHandle, &serviceStatus)) &&
                !FlagOn(serviceStatus.dwControlsAccepted, SERVICE_ACCEPT_PAUSE_CONTINUE))
            {
                AtSetToolError(Result, "failed", STATUS_NOT_SUPPORTED,
                    L"%s does not accept pause and continue; get_service lists what a service accepts "
                    L"under controls_accepted, and most accept only stop.",
                    PhGetString(serviceItem->Name)
                    );
                return;
            }

            if (NT_SUCCESS(status))
            {
                status = Tool->Action == AtActionPauseService ?
                    PhPauseService(serviceHandle) : PhContinueService(serviceHandle);
            }
        }
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

            if (NT_SUCCESS(configStatus))
            {
                PPH_STRING description = AtGetArgumentString(Call->Arguments, "description");

                if (description)
                {
                    SERVICE_DESCRIPTION descriptionInfo;

                    // An empty description reads as null, so it is ignored rather than applied.
                    descriptionInfo.lpDescription = description->Buffer;
                    configStatus = PhChangeServiceConfig2(serviceHandle, SERVICE_CONFIG_DESCRIPTION, &descriptionInfo);
                    PhDereferenceObject(description);
                }
            }

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
        PPH_STRING description;
        BOOLEAN delayedAutoStart;

        // The description is one of the three things this can set and the steps are applied in
        // order, so a caller that set it needs to see whether it took.
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
    case AtActionPauseService:
    case AtActionContinueService:
    case AtActionSetServiceConfig:
        AtpControlService(Tool, Call, Target, Result);
        break;
    default:
        AtSetToolError(Result, "failed", STATUS_NOT_IMPLEMENTED, L"This tool is not implemented.");
        break;
    }
}
