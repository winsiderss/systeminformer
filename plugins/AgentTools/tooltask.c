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
#include <taskschd.h>

DEFINE_GUID(CLSID_TaskScheduler, 0x0f87369f, 0xa4e5, 0x4cfc, 0xbd, 0x3e, 0x73, 0xe6, 0x15, 0x45, 0x72, 0xdd);
DEFINE_GUID(IID_ITaskService, 0x2FABA4C7, 0x4DA9, 0x4013, 0x96, 0x97, 0x20, 0xCC, 0x3F, 0xD4, 0x0F, 0x85);
DEFINE_GUID(IID_IExecAction, 0x4C3D624D, 0xFD6B, 0x49A3, 0xB9, 0xB7, 0x09, 0xCB, 0x3C, 0xD3, 0xF0, 0x47);
DEFINE_GUID(IID_IComHandlerAction, 0x6D2FD252, 0x75C5, 0x4F66, 0x90, 0xBA, 0x2A, 0x7D, 0x8C, 0xC3, 0x03, 0x9F);

#define AT_TASK_MAX_DEPTH 16

typedef struct _AT_TASK_CONTEXT
{
    PAT_ROWS Rows;
    PPH_STRING NameContains;
    PPH_STRING ActionContains;
    PPH_STRING StateFilter;
    BOOLEAN EnabledOnly;
    BOOLEAN HiddenOnly;
    BOOLEAN ExcludeMicrosoftFolder;
    BOOLEAN IncludeDetails;
    ULONG FolderCount;
    ULONG EnumeratedCount;
    ULONG UnreadableCount;
    ULONG TruncatedCount;
} AT_TASK_CONTEXT, *PAT_TASK_CONTEXT;

NTSTATUS AtpTaskStatus(
    _In_ HRESULT Result
    )
{
    if (HRESULT_FACILITY(Result) == FACILITY_WIN32)
        return PhDosErrorToNtStatus(HRESULT_CODE(Result));

    return STATUS_UNSUCCESSFUL;
}

PPH_STRING AtpTaskStringFromBstr(
    _In_opt_ BSTR String
    )
{
    if (!String || String[0] == UNICODE_NULL)
        return NULL;

    return PhCreateStringEx(String, SysStringLen(String) * sizeof(WCHAR));
}

VOID AtpTaskAddBstr(
    _In_ PVOID Object,
    _In_ PCSTR Key,
    _In_ HRESULT Result,
    _Inout_ BSTR* String
    )
{
    PPH_STRING string = NULL;

    if (HR_SUCCESS(Result))
        string = AtpTaskStringFromBstr(*String);

    AtJsonAddString(Object, Key, string);

    PhClearReference(&string);
    SysFreeString(*String);
    *String = NULL;
}

VOID AtpTaskAddDate(
    _In_ PVOID Object,
    _In_ PCSTR Key,
    _In_ HRESULT Result,
    _In_ DATE Date
    )
{
    SYSTEMTIME localTime;
    SYSTEMTIME utcTime;
    LARGE_INTEGER time;

    if (HR_FAILED(Result) || Date < 1.0 ||
        !VariantTimeToSystemTime(Date, &localTime) ||
        !TzSpecificLocalTimeToSystemTime(NULL, &localTime, &utcTime) ||
        !PhSystemTimeToLargeInteger(&time, &utcTime))
    {
        AtJsonAddNull(Object, Key);
        return;
    }

    AtJsonAddTime(Object, Key, &time);
}

PCWSTR AtpTaskStateString(
    _In_ TASK_STATE State
    )
{
    switch (State)
    {
    case TASK_STATE_DISABLED:
        return L"disabled";
    case TASK_STATE_QUEUED:
        return L"queued";
    case TASK_STATE_READY:
        return L"ready";
    case TASK_STATE_RUNNING:
        return L"running";
    case TASK_STATE_UNKNOWN:
        return L"unknown";
    }

    return NULL;
}

PCWSTR AtpTaskLogonTypeString(
    _In_ TASK_LOGON_TYPE LogonType
    )
{
    switch (LogonType)
    {
    case TASK_LOGON_NONE:
        return L"none";
    case TASK_LOGON_PASSWORD:
        return L"password";
    case TASK_LOGON_S4U:
        return L"s4u";
    case TASK_LOGON_INTERACTIVE_TOKEN:
        return L"interactive_token";
    case TASK_LOGON_GROUP:
        return L"group";
    case TASK_LOGON_SERVICE_ACCOUNT:
        return L"service_account";
    case TASK_LOGON_INTERACTIVE_TOKEN_OR_PASSWORD:
        return L"interactive_token_or_password";
    }

    return NULL;
}

PCWSTR AtpTaskRunLevelString(
    _In_ TASK_RUNLEVEL_TYPE RunLevel
    )
{
    switch (RunLevel)
    {
    case TASK_RUNLEVEL_LUA:
        return L"limited";
    case TASK_RUNLEVEL_HIGHEST:
        return L"highest";
    }

    return NULL;
}

PCWSTR AtpTaskActionTypeString(
    _In_ TASK_ACTION_TYPE Type
    )
{
    switch (Type)
    {
    case TASK_ACTION_EXEC:
        return L"exec";
    case TASK_ACTION_COM_HANDLER:
        return L"com_handler";
    case TASK_ACTION_SEND_EMAIL:
        return L"send_email";
    case TASK_ACTION_SHOW_MESSAGE:
        return L"show_message";
    }

    return NULL;
}

PCWSTR AtpTaskTriggerTypeString(
    _In_ TASK_TRIGGER_TYPE2 Type
    )
{
    switch (Type)
    {
    case TASK_TRIGGER_EVENT:
        return L"event";
    case TASK_TRIGGER_TIME:
        return L"time";
    case TASK_TRIGGER_DAILY:
        return L"daily";
    case TASK_TRIGGER_WEEKLY:
        return L"weekly";
    case TASK_TRIGGER_MONTHLY:
        return L"monthly";
    case TASK_TRIGGER_MONTHLYDOW:
        return L"monthly_day_of_week";
    case TASK_TRIGGER_IDLE:
        return L"idle";
    case TASK_TRIGGER_REGISTRATION:
        return L"registration";
    case TASK_TRIGGER_BOOT:
        return L"boot";
    case TASK_TRIGGER_LOGON:
        return L"logon";
    case TASK_TRIGGER_SESSION_STATE_CHANGE:
        return L"session_state_change";
    case TASK_TRIGGER_CUSTOM_TRIGGER_01:
        return L"custom";
    }

    return NULL;
}

PCWSTR AtpTaskCompatibilityString(
    _In_ TASK_COMPATIBILITY Compatibility
    )
{
    switch (Compatibility)
    {
    case TASK_COMPATIBILITY_AT:
        return L"at";
    case TASK_COMPATIBILITY_V1:
        return L"v1";
    case TASK_COMPATIBILITY_V2:
        return L"v2";
    case TASK_COMPATIBILITY_V2_1:
        return L"v2_1";
    case TASK_COMPATIBILITY_V2_2:
        return L"v2_2";
    case TASK_COMPATIBILITY_V2_3:
        return L"v2_3";
    case TASK_COMPATIBILITY_V2_4:
        return L"v2_4";
    }

    return NULL;
}

PCWSTR AtpTaskInstancesPolicyString(
    _In_ TASK_INSTANCES_POLICY Policy
    )
{
    switch (Policy)
    {
    case TASK_INSTANCES_PARALLEL:
        return L"parallel";
    case TASK_INSTANCES_QUEUE:
        return L"queue";
    case TASK_INSTANCES_IGNORE_NEW:
        return L"ignore_new";
    case TASK_INSTANCES_STOP_EXISTING:
        return L"stop_existing";
    }

    return NULL;
}

VOID AtpTaskAddVariantBoolean(
    _In_ PVOID Object,
    _In_ PCSTR Key,
    _In_ HRESULT Result,
    _In_ VARIANT_BOOL Value
    )
{
    if (HR_SUCCESS(Result))
        PhAddJsonObjectBoolean(Object, Key, Value != VARIANT_FALSE);
    else
        AtJsonAddNull(Object, Key);
}

VOID AtpTaskAddMatchedBstr(
    _In_ PVOID Object,
    _In_ PCSTR Key,
    _In_ HRESULT Result,
    _Inout_ BSTR* String,
    _In_opt_ PPH_STRING Contains,
    _Inout_ PBOOLEAN Matched
    )
{
    PPH_STRING string = NULL;

    if (HR_SUCCESS(Result))
        string = AtpTaskStringFromBstr(*String);

    if (Contains && AtContainsString(string, Contains))
        *Matched = TRUE;

    AtJsonAddString(Object, Key, string);

    PhClearReference(&string);
    SysFreeString(*String);
    *String = NULL;
}

PVOID AtpTaskCreateActionRow(
    _In_ IAction* Action,
    _In_opt_ PPH_STRING Contains,
    _Inout_ PBOOLEAN Matched
    )
{
    PVOID row;
    TASK_ACTION_TYPE type = TASK_ACTION_EXEC;
    IExecAction* execAction;
    IComHandlerAction* comAction;
    BSTR string = NULL;

    row = PhCreateJsonObject();

    if (HR_FAILED(IAction_get_Type(Action, &type)))
        type = TASK_ACTION_EXEC;

    AtJsonAddStringZ(row, "type", AtpTaskActionTypeString(type));
    PhAddJsonObjectUInt64(row, "type_value", type);

    if (HR_SUCCESS(IAction_QueryInterface(Action, &IID_IExecAction, &execAction)))
    {
        // The arguments are as much a part of what an action runs as the image is: a task that
        // hosts its payload in rundll32 or a shell says so nowhere else.
        AtpTaskAddMatchedBstr(row, "path", IExecAction_get_Path(execAction, &string), &string, Contains, Matched);
        AtpTaskAddMatchedBstr(row, "arguments", IExecAction_get_Arguments(execAction, &string), &string, Contains, Matched);
        AtpTaskAddBstr(row, "working_directory", IExecAction_get_WorkingDirectory(execAction, &string), &string);
        AtJsonAddNull(row, "class_id");
        AtJsonAddNull(row, "data");

        IExecAction_Release(execAction);
    }
    else if (HR_SUCCESS(IAction_QueryInterface(Action, &IID_IComHandlerAction, &comAction)))
    {
        AtJsonAddNull(row, "path");
        AtJsonAddNull(row, "arguments");
        AtJsonAddNull(row, "working_directory");
        AtpTaskAddMatchedBstr(row, "class_id", IComHandlerAction_get_ClassId(comAction, &string), &string, Contains, Matched);
        AtpTaskAddMatchedBstr(row, "data", IComHandlerAction_get_Data(comAction, &string), &string, Contains, Matched);

        IComHandlerAction_Release(comAction);
    }
    else
    {
        // A mail or message action, both of which the scheduler stopped running long ago.
        AtJsonAddNull(row, "path");
        AtJsonAddNull(row, "arguments");
        AtJsonAddNull(row, "working_directory");
        AtJsonAddNull(row, "class_id");
        AtJsonAddNull(row, "data");
    }

    return row;
}

PVOID AtpTaskCreateActions(
    _In_ PAT_TASK_CONTEXT Context,
    _In_ IActionCollection* Actions,
    _In_opt_ PPH_STRING ActionContains,
    _Out_ PBOOLEAN Matched
    )
{
    PVOID array;
    LONG count = 0;
    LONG i;

    *Matched = FALSE;

    // An unreadable count is not an empty action list, which would read as a task that runs nothing.
    if (HR_FAILED(IActionCollection_get_Count(Actions, &count)))
    {
        Context->UnreadableCount++;
        return NULL;
    }

    array = PhCreateJsonArray();

    for (i = 1; i <= count; i++)
    {
        IAction* action;

        if (HR_FAILED(IActionCollection_get_Item(Actions, i, &action)))
        {
            Context->UnreadableCount++;
            continue;
        }

        PhAddJsonArrayObject(array, AtpTaskCreateActionRow(action, ActionContains, Matched));

        IAction_Release(action);
    }

    return array;
}

PVOID AtpTaskCreateTriggers(
    _In_ PAT_TASK_CONTEXT Context,
    _In_ ITriggerCollection* Triggers
    )
{
    PVOID array;
    LONG count = 0;
    LONG i;

    // As with the actions, an unreadable count would otherwise read as a task nothing starts.
    if (HR_FAILED(ITriggerCollection_get_Count(Triggers, &count)))
    {
        Context->UnreadableCount++;
        return NULL;
    }

    array = PhCreateJsonArray();

    for (i = 1; i <= count; i++)
    {
        ITrigger* trigger;
        PVOID row;
        TASK_TRIGGER_TYPE2 type = TASK_TRIGGER_EVENT;
        VARIANT_BOOL enabled = VARIANT_FALSE;
        BSTR string = NULL;

        if (HR_FAILED(ITriggerCollection_get_Item(Triggers, i, &trigger)))
        {
            Context->UnreadableCount++;
            continue;
        }

        row = PhCreateJsonObject();

        if (HR_FAILED(ITrigger_get_Type(trigger, &type)))
            type = TASK_TRIGGER_EVENT;

        AtJsonAddStringZ(row, "type", AtpTaskTriggerTypeString(type));
        PhAddJsonObjectUInt64(row, "type_value", type);
        AtpTaskAddBstr(row, "id", ITrigger_get_Id(trigger, &string), &string);
        AtpTaskAddVariantBoolean(row, "enabled", ITrigger_get_Enabled(trigger, &enabled), enabled);
        // The boundaries and the limit are the scheduler's own ISO 8601 text, kept as it wrote it.
        AtpTaskAddBstr(row, "start_boundary", ITrigger_get_StartBoundary(trigger, &string), &string);
        AtpTaskAddBstr(row, "end_boundary", ITrigger_get_EndBoundary(trigger, &string), &string);
        AtpTaskAddBstr(row, "execution_time_limit", ITrigger_get_ExecutionTimeLimit(trigger, &string), &string);

        PhAddJsonArrayObject(array, row);

        ITrigger_Release(trigger);
    }

    return array;
}

VOID AtpTaskAddRegistrationInfo(
    _In_ PVOID Row,
    _In_ ITaskDefinition* Definition
    )
{
    IRegistrationInfo* info;
    BSTR string = NULL;

    if (HR_FAILED(ITaskDefinition_get_RegistrationInfo(Definition, &info)))
    {
        AtJsonAddNull(Row, "author");
        AtJsonAddNull(Row, "description");
        AtJsonAddNull(Row, "registration_date");
        AtJsonAddNull(Row, "source");
        AtJsonAddNull(Row, "uri");
        return;
    }

    AtpTaskAddBstr(Row, "author", IRegistrationInfo_get_Author(info, &string), &string);
    AtpTaskAddBstr(Row, "description", IRegistrationInfo_get_Description(info, &string), &string);
    AtpTaskAddBstr(Row, "registration_date", IRegistrationInfo_get_Date(info, &string), &string);
    AtpTaskAddBstr(Row, "source", IRegistrationInfo_get_Source(info, &string), &string);
    AtpTaskAddBstr(Row, "uri", IRegistrationInfo_get_URI(info, &string), &string);

    IRegistrationInfo_Release(info);
}

VOID AtpTaskAddSettingsDetails(
    _In_ PVOID Row,
    _In_ ITaskSettings* Settings
    )
{
    PVOID settings;
    VARIANT_BOOL value = VARIANT_FALSE;
    TASK_INSTANCES_POLICY policy = TASK_INSTANCES_PARALLEL;
    TASK_COMPATIBILITY compatibility = TASK_COMPATIBILITY_V2;
    LONG priority = 0;
    BSTR string = NULL;

    settings = PhCreateJsonObject();

    AtpTaskAddVariantBoolean(settings, "allow_demand_start", ITaskSettings_get_AllowDemandStart(Settings, &value), value);
    AtpTaskAddVariantBoolean(settings, "allow_hard_terminate", ITaskSettings_get_AllowHardTerminate(Settings, &value), value);
    AtpTaskAddVariantBoolean(settings, "start_when_available", ITaskSettings_get_StartWhenAvailable(Settings, &value), value);
    AtpTaskAddVariantBoolean(settings, "run_only_if_idle", ITaskSettings_get_RunOnlyIfIdle(Settings, &value), value);
    AtpTaskAddVariantBoolean(settings, "run_only_if_network_available", ITaskSettings_get_RunOnlyIfNetworkAvailable(Settings, &value), value);
    AtpTaskAddVariantBoolean(settings, "disallow_start_if_on_batteries", ITaskSettings_get_DisallowStartIfOnBatteries(Settings, &value), value);
    AtpTaskAddVariantBoolean(settings, "stop_if_going_on_batteries", ITaskSettings_get_StopIfGoingOnBatteries(Settings, &value), value);
    AtpTaskAddVariantBoolean(settings, "wake_to_run", ITaskSettings_get_WakeToRun(Settings, &value), value);
    AtpTaskAddBstr(settings, "execution_time_limit", ITaskSettings_get_ExecutionTimeLimit(Settings, &string), &string);
    AtpTaskAddBstr(settings, "delete_expired_task_after", ITaskSettings_get_DeleteExpiredTaskAfter(Settings, &string), &string);
    AtpTaskAddBstr(settings, "restart_interval", ITaskSettings_get_RestartInterval(Settings, &string), &string);

    if (HR_SUCCESS(ITaskSettings_get_Priority(Settings, &priority)))
        PhAddJsonObjectUInt64(settings, "priority", priority);
    else
        AtJsonAddNull(settings, "priority");

    if (HR_SUCCESS(ITaskSettings_get_MultipleInstances(Settings, &policy)))
        AtJsonAddStringZ(settings, "multiple_instances", AtpTaskInstancesPolicyString(policy));
    else
        AtJsonAddNull(settings, "multiple_instances");

    if (HR_SUCCESS(ITaskSettings_get_Compatibility(Settings, &compatibility)))
        AtJsonAddStringZ(settings, "compatibility", AtpTaskCompatibilityString(compatibility));
    else
        AtJsonAddNull(settings, "compatibility");

    PhAddJsonObjectValue(Row, "settings", settings);
}

VOID AtpTaskAddPrincipal(
    _In_ PVOID Row,
    _In_ ITaskDefinition* Definition
    )
{
    IPrincipal* principal;
    TASK_LOGON_TYPE logonType = TASK_LOGON_NONE;
    TASK_RUNLEVEL_TYPE runLevel = TASK_RUNLEVEL_LUA;
    BSTR string = NULL;

    if (HR_FAILED(ITaskDefinition_get_Principal(Definition, &principal)))
    {
        AtJsonAddNull(Row, "user_id");
        AtJsonAddNull(Row, "group_id");
        AtJsonAddNull(Row, "logon_type");
        AtJsonAddNull(Row, "run_level");
        return;
    }

    // The user is written as whatever registered the task chose to write: a name, a SID, or one of
    // the built-in aliases. lookup_account turns any of those into the other.
    AtpTaskAddBstr(Row, "user_id", IPrincipal_get_UserId(principal, &string), &string);
    AtpTaskAddBstr(Row, "group_id", IPrincipal_get_GroupId(principal, &string), &string);

    if (HR_SUCCESS(IPrincipal_get_LogonType(principal, &logonType)))
        AtJsonAddStringZ(Row, "logon_type", AtpTaskLogonTypeString(logonType));
    else
        AtJsonAddNull(Row, "logon_type");

    if (HR_SUCCESS(IPrincipal_get_RunLevel(principal, &runLevel)))
        AtJsonAddStringZ(Row, "run_level", AtpTaskRunLevelString(runLevel));
    else
        AtJsonAddNull(Row, "run_level");

    IPrincipal_Release(principal);
}

BOOLEAN AtpTaskAddDefinition(
    _In_ PAT_TASK_CONTEXT Context,
    _In_ PVOID Row,
    _In_ IRegisteredTask* Task,
    _In_ BOOLEAN NameMatched
    )
{
    ITaskDefinition* definition;
    ITaskSettings* settings;
    IActionCollection* actions;
    ITriggerCollection* triggers;
    BOOLEAN actionMatched = FALSE;
    BOOLEAN hidden = FALSE;
    BOOLEAN haveHidden = FALSE;

    if (HR_FAILED(IRegisteredTask_get_Definition(Task, &definition)))
    {
        Context->UnreadableCount++;

        PhAddJsonObjectBoolean(Row, "definition_readable", FALSE);
        AtJsonAddNull(Row, "hidden");
        AtJsonAddNull(Row, "user_id");
        AtJsonAddNull(Row, "group_id");
        AtJsonAddNull(Row, "logon_type");
        AtJsonAddNull(Row, "run_level");
        AtJsonAddNull(Row, "actions");

        // A task whose definition is out of reach cannot be matched on what it runs, and cannot be
        // shown to be hidden, so either filter drops it rather than guessing.
        return NameMatched && !Context->ActionContains && !Context->HiddenOnly;
    }

    PhAddJsonObjectBoolean(Row, "definition_readable", TRUE);

    if (HR_SUCCESS(ITaskDefinition_get_Settings(definition, &settings)))
    {
        VARIANT_BOOL value = VARIANT_FALSE;

        if (HR_SUCCESS(ITaskSettings_get_Hidden(settings, &value)))
        {
            hidden = value != VARIANT_FALSE;
            haveHidden = TRUE;
        }

        if (haveHidden)
            PhAddJsonObjectBoolean(Row, "hidden", hidden);
        else
            AtJsonAddNull(Row, "hidden");

        if (Context->IncludeDetails)
            AtpTaskAddSettingsDetails(Row, settings);

        ITaskSettings_Release(settings);
    }
    else
    {
        AtJsonAddNull(Row, "hidden");
    }

    AtpTaskAddPrincipal(Row, definition);

    if (HR_SUCCESS(ITaskDefinition_get_Actions(definition, &actions)))
    {
        PVOID array = AtpTaskCreateActions(Context, actions, Context->ActionContains, &actionMatched);

        if (array)
            PhAddJsonObjectValue(Row, "actions", array);
        else
            AtJsonAddNull(Row, "actions");

        IActionCollection_Release(actions);
    }
    else
    {
        Context->UnreadableCount++;
        AtJsonAddNull(Row, "actions");
    }

    if (Context->IncludeDetails)
    {
        AtpTaskAddRegistrationInfo(Row, definition);

        if (HR_SUCCESS(ITaskDefinition_get_Triggers(definition, &triggers)))
        {
            PVOID array = AtpTaskCreateTriggers(Context, triggers);

            if (array)
                PhAddJsonObjectValue(Row, "triggers", array);
            else
                AtJsonAddNull(Row, "triggers");

            ITriggerCollection_Release(triggers);
        }
        else
        {
            Context->UnreadableCount++;
            AtJsonAddNull(Row, "triggers");
        }
    }

    ITaskDefinition_Release(definition);

    if (Context->HiddenOnly && !hidden)
        return FALSE;

    if (Context->ActionContains)
        return actionMatched;

    return NameMatched;
}

VOID AtpTaskAddTask(
    _In_ PAT_TASK_CONTEXT Context,
    _In_ IRegisteredTask* Task,
    _In_opt_ PPH_STRING FolderPath
    )
{
    static CONST PH_STRINGREF microsoftFolder = PH_STRINGREF_INIT(L"\\Microsoft\\");
    PVOID row;
    PPH_STRING name = NULL;
    PPH_STRING path = NULL;
    TASK_STATE state = TASK_STATE_UNKNOWN;
    VARIANT_BOOL enabled = VARIANT_FALSE;
    PCWSTR stateString;
    HRESULT enabledResult;
    HRESULT result;
    LONG value = 0;
    DATE date = 0.0;
    BSTR string = NULL;

    Context->EnumeratedCount++;

    if (HR_SUCCESS(IRegisteredTask_get_Name(Task, &string)))
    {
        name = AtpTaskStringFromBstr(string);
        SysFreeString(string);
    }

    if (HR_SUCCESS(IRegisteredTask_get_Path(Task, &string)))
    {
        path = AtpTaskStringFromBstr(string);
        SysFreeString(string);
    }

    // The folder a task sits in is a name, not a signature: anything able to write to \Microsoft\
    // would be skipped along with the tasks Windows puts there.
    if (Context->ExcludeMicrosoftFolder && path && PhStartsWithStringRef(&path->sr, &microsoftFolder, TRUE))
        goto CleanupExit;

    if (HR_FAILED(IRegisteredTask_get_State(Task, &state)))
        state = TASK_STATE_UNKNOWN;

    stateString = AtpTaskStateString(state);

    if (Context->StateFilter && (!stateString || !PhEqualString2(Context->StateFilter, stateString, TRUE)))
        goto CleanupExit;

    // A task whose enabled state cannot be read is not a disabled task, so the filter keeps it
    // rather than dropping it on a guess; the sibling state field reports "unknown" for the same
    // reason.
    enabledResult = IRegisteredTask_get_Enabled(Task, &enabled);

    if (Context->EnabledOnly && HR_SUCCESS(enabledResult) && enabled == VARIANT_FALSE)
        goto CleanupExit;

    row = PhCreateJsonObject();
    AtJsonAddString(row, "name", name);
    AtJsonAddString(row, "path", path);
    AtJsonAddString(row, "folder", FolderPath);
    AtpTaskAddVariantBoolean(row, "enabled", enabledResult, enabled);
    AtJsonAddStringZ(row, "state", stateString);
    PhAddJsonObjectUInt64(row, "state_value", state);

    result = IRegisteredTask_get_LastRunTime(Task, &date);
    AtpTaskAddDate(row, "last_run_time", result, date);
    result = IRegisteredTask_get_NextRunTime(Task, &date);
    AtpTaskAddDate(row, "next_run_time", result, date);

    // The exit code of the last run, as the scheduler recorded it. It is an HRESULT-shaped value
    // that carries a process exit code for an exec action, so it is signed and reported both ways.
    if (HR_SUCCESS(IRegisteredTask_get_LastTaskResult(Task, &value)))
    {
        PhAddJsonObjectInt64(row, "last_result", value);
        AtJsonAddHex(row, "last_result_hex", (ULONG)value);
    }
    else
    {
        AtJsonAddNull(row, "last_result");
        AtJsonAddNull(row, "last_result_hex");
    }

    if (HR_SUCCESS(IRegisteredTask_get_NumberOfMissedRuns(Task, &value)))
        PhAddJsonObjectInt64(row, "missed_runs", value);
    else
        AtJsonAddNull(row, "missed_runs");

    if (AtpTaskAddDefinition(
        Context,
        row,
        Task,
        !Context->NameContains || AtContainsString(name, Context->NameContains) || AtContainsString(path, Context->NameContains)
        ))
    {
        AtAddRow(Context->Rows, row);
    }
    else
    {
        PhFreeJsonObject(row);
    }

CleanupExit:

    PhClearReference(&name);
    PhClearReference(&path);
}

VOID AtpTaskEnumerateFolder(
    _In_ PAT_TASK_CONTEXT Context,
    _In_ ITaskFolder* Folder,
    _In_ ULONG Depth
    )
{
    IRegisteredTaskCollection* tasks;
    ITaskFolderCollection* folders;
    PPH_STRING folderPath = NULL;
    LONG count;
    LONG i;
    BSTR string = NULL;

    Context->FolderCount++;

    if (HR_SUCCESS(ITaskFolder_get_Path(Folder, &string)))
    {
        folderPath = AtpTaskStringFromBstr(string);
        SysFreeString(string);
    }

    // TASK_ENUM_HIDDEN, because a task that asked not to be shown is the one worth seeing.
    if (HR_SUCCESS(ITaskFolder_GetTasks(Folder, TASK_ENUM_HIDDEN, &tasks)))
    {
        count = 0;

        if (HR_SUCCESS(IRegisteredTaskCollection_get_Count(tasks, &count)))
        {
            for (i = 1; i <= count; i++)
            {
                IRegisteredTask* task;
                VARIANT index;

                V_VT(&index) = VT_I4;
                V_I4(&index) = i;

                if (HR_FAILED(IRegisteredTaskCollection_get_Item(tasks, index, &task)))
                {
                    Context->UnreadableCount++;
                    continue;
                }

                AtpTaskAddTask(Context, task, folderPath);

                IRegisteredTask_Release(task);
            }
        }
        else
        {
            Context->UnreadableCount++;
        }

        IRegisteredTaskCollection_Release(tasks);
    }
    else
    {
        Context->UnreadableCount++;
    }

    if (HR_SUCCESS(ITaskFolder_GetFolders(Folder, 0, &folders)))
    {
        count = 0;

        if (HR_SUCCESS(ITaskFolderCollection_get_Count(folders, &count)))
        {
            // Recursion stops at the depth cap, but a folder that still has children there is a
            // subtree that went unwalked rather than one that is empty.
            if (Depth >= AT_TASK_MAX_DEPTH)
            {
                if (count > 0)
                    Context->TruncatedCount++;
            }
            else
            {
                for (i = 1; i <= count; i++)
                {
                    ITaskFolder* folder;
                    VARIANT index;

                    V_VT(&index) = VT_I4;
                    V_I4(&index) = i;

                    if (HR_FAILED(ITaskFolderCollection_get_Item(folders, index, &folder)))
                    {
                        Context->UnreadableCount++;
                        continue;
                    }

                    AtpTaskEnumerateFolder(Context, folder, Depth + 1);

                    ITaskFolder_Release(folder);
                }
            }
        }
        else
        {
            Context->UnreadableCount++;
        }

        ITaskFolderCollection_Release(folders);
    }
    else
    {
        Context->UnreadableCount++;
    }

    PhClearReference(&folderPath);
}

VOID AtpListScheduledTasks(
    _In_ PAT_TOOL_CALL Call,
    _Inout_ PAT_TOOL_RESULT Result
    )
{
    static CONST PH_STRINGREF rootFolder = PH_STRINGREF_INIT(L"\\");
    AT_TASK_CONTEXT context;
    AT_ROWS rows;
    HRESULT result;
    ITaskService* taskService = NULL;
    ITaskFolder* taskFolder = NULL;
    PPH_STRING folderName;
    BSTR folderString = NULL;
    VARIANT empty = { VT_EMPTY };
    PVOID structured;

    memset(&context, 0, sizeof(AT_TASK_CONTEXT));

    // PhCreateThreadEx initializes an apartment for every thread it makes, so the connection thread
    // this runs on already has one.
    result = PhGetClassObject(
        L"taskschd.dll",
        &CLSID_TaskScheduler,
        &IID_ITaskService,
        &taskService
        );

    if (HR_FAILED(result))
    {
        AtSetToolError(Result, "failed", AtpTaskStatus(result),
            L"The task scheduler could not be reached (0x%08x).", result);
        goto CleanupExit;
    }

    result = ITaskService_Connect(taskService, empty, empty, empty, empty);

    if (HR_FAILED(result))
    {
        AtSetToolError(Result, "failed", AtpTaskStatus(result),
            L"Connecting to the task scheduler service failed (0x%08x).", result);
        goto CleanupExit;
    }

    folderName = AtGetArgumentString(Call->Arguments, "folder");
    folderString = folderName
        ? SysAllocStringLen(folderName->Buffer, (UINT)folderName->Length / sizeof(WCHAR))
        : SysAllocStringLen(rootFolder.Buffer, (UINT)rootFolder.Length / sizeof(WCHAR));

    result = ITaskService_GetFolder(taskService, folderString, &taskFolder);

    if (HR_FAILED(result))
    {
        AtSetToolError(Result, "not_found", AtpTaskStatus(result),
            L"No task folder named %s could be opened (0x%08x).",
            folderName ? PhGetString(folderName) : rootFolder.Buffer, result);
        PhClearReference(&folderName);
        goto CleanupExit;
    }

    PhClearReference(&folderName);

    AtInitializeRows(&rows, Call->Arguments);

    context.Rows = &rows;
    context.NameContains = AtGetArgumentString(Call->Arguments, "name_contains");
    context.ActionContains = AtGetArgumentString(Call->Arguments, "action_contains");
    context.StateFilter = AtGetArgumentString(Call->Arguments, "state");
    context.EnabledOnly = AtJsonGetObjectBoolean(Call->Arguments, "enabled_only");
    context.HiddenOnly = AtJsonGetObjectBoolean(Call->Arguments, "hidden_only");
    context.ExcludeMicrosoftFolder = AtJsonGetObjectBoolean(Call->Arguments, "exclude_microsoft_folder");
    context.IncludeDetails = AtJsonGetObjectBoolean(Call->Arguments, "include_details");

    AtpTaskEnumerateFolder(&context, taskFolder, 0);

    structured = PhCreateJsonObject();
    AtAddRows(structured, "tasks", &rows);
    PhAddJsonObjectUInt64(structured, "folder_count", context.FolderCount);
    PhAddJsonObjectUInt64(structured, "enumerated_count", context.EnumeratedCount);
    PhAddJsonObjectUInt64(structured, "unreadable_count", context.UnreadableCount);
    PhAddJsonObjectUInt64(structured, "unwalked_folder_count", context.TruncatedCount);
    AtAddSnapshot(structured);

    Result->StructuredContent = structured;

    AtDeleteRows(&rows);

    PhClearReference(&context.NameContains);
    PhClearReference(&context.ActionContains);
    PhClearReference(&context.StateFilter);

CleanupExit:

    if (folderString)
        SysFreeString(folderString);

    if (taskFolder)
        ITaskFolder_Release(taskFolder);

    if (taskService)
        ITaskService_Release(taskService);
}

VOID AtTaskInvokeTool(
    _In_ PCAT_TOOL Tool,
    _In_ PAT_TOOL_CALL Call,
    _Inout_ PAT_TARGET Target,
    _Inout_ PAT_TOOL_RESULT Result
    )
{
    UNREFERENCED_PARAMETER(Target);

    switch (Tool->Action)
    {
    case AtActionListScheduledTasks:
        AtpListScheduledTasks(Call, Result);
        break;
    default:
        AtSetToolError(Result, "failed", STATUS_NOT_IMPLEMENTED, L"This tool is not implemented.");
        break;
    }
}
