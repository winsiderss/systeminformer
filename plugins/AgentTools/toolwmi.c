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
#include <wbemidl.h>

// Persistence that is not a file. A permanent WMI event subscription is three objects in the
// repository - an __EventFilter, an __EventConsumer and a __FilterToConsumerBinding - and when the
// query matches, the WMI service runs the consumer. A binding is the unit that matters: a filter or
// a consumer alone does nothing.

DEFINE_GUID(CLSID_WbemLocator, 0x4590f811, 0x1d3a, 0x11d0, 0x89, 0x1f, 0x00, 0xaa, 0x00, 0x4b, 0x2e, 0x24);
DEFINE_GUID(IID_IWbemLocator, 0xdc12a687, 0x737f, 0x11cf, 0x88, 0x4d, 0x00, 0xaa, 0x00, 0x4b, 0x2e, 0x24);
DEFINE_GUID(IID_IClientSecurity, 0x0000013D, 0x0000, 0x0000, 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46);

// The repository is answered by a service, and a service can be busy. Every wait here is finite so
// a wedged WMI stops this tool rather than the connection it is answering on.
#define AT_WMI_TIMEOUT 15000
// A namespace with more subscriptions than this is not a subscription list any more.
#define AT_WMI_MAXIMUM_OBJECTS 4096
// Script text is the payload of an ActiveScriptEventConsumer and is written by whoever made it.
#define AT_WMI_MAXIMUM_SCRIPT 4096

typedef struct _AT_WMI_FILTER
{
    PPH_STRING Name;
    PPH_STRING Query;
    PPH_STRING QueryLanguage;
    PPH_STRING EventNamespace;
    PPH_STRING CreatorSid;
    PPH_STRING RelativePath;
    PPH_STRING Path;
    BOOLEAN Bound;
} AT_WMI_FILTER, *PAT_WMI_FILTER;

typedef struct _AT_WMI_CONSUMER
{
    PPH_STRING Name;
    PPH_STRING ClassName;
    PPH_STRING CreatorSid;
    PPH_STRING RelativePath;
    PPH_STRING Path;
    PPH_STRING CommandLine;
    PPH_STRING ExecutablePath;
    PPH_STRING WorkingDirectory;
    PPH_STRING ScriptingEngine;
    PPH_STRING ScriptText;
    PPH_STRING ScriptFileName;
    PPH_STRING LogFileName;
    PPH_STRING LogText;
    BOOLEAN RunInteractively;
    BOOLEAN HasRunInteractively;
    BOOLEAN ScriptTextTruncated;
    BOOLEAN Bound;
} AT_WMI_CONSUMER, *PAT_WMI_CONSUMER;

typedef struct _AT_WMI_CONTEXT
{
    PAT_ROWS Rows;
    PPH_STRING NameContains;
    PPH_STRING Namespace;
} AT_WMI_CONTEXT, *PAT_WMI_CONTEXT;

/**
 * Converts an HRESULT to the nearest status this server reports.
 */
NTSTATUS AtpWmiStatus(
    _In_ HRESULT Result
    )
{
    if (Result == WBEM_E_ACCESS_DENIED)
        return STATUS_ACCESS_DENIED;
    if (Result == WBEM_E_INVALID_NAMESPACE || Result == WBEM_E_INVALID_CLASS)
        return STATUS_OBJECT_NAME_NOT_FOUND;
    if (HRESULT_FACILITY(Result) == FACILITY_WIN32)
        return PhDosErrorToNtStatus(HRESULT_CODE(Result));

    return STATUS_UNSUCCESSFUL;
}

PPH_STRING AtpWmiVariantString(
    _In_ VARIANT* Variant
    )
{
    if (V_VT(Variant) != VT_BSTR || !V_BSTR(Variant))
        return NULL;

    if (SysStringLen(V_BSTR(Variant)) == 0)
        return NULL;

    return PhCreateStringEx(V_BSTR(Variant), SysStringLen(V_BSTR(Variant)) * sizeof(WCHAR));
}

PPH_STRING AtpWmiGetString(
    _In_ IWbemClassObject* Object,
    _In_ PCWSTR Name
    )
{
    VARIANT variant;
    PPH_STRING string;

    VariantInit(&variant);

    if (HR_FAILED(IWbemClassObject_Get(Object, Name, 0, &variant, NULL, NULL)))
        return NULL;

    string = AtpWmiVariantString(&variant);
    VariantClear(&variant);

    return string;
}

BOOLEAN AtpWmiGetBoolean(
    _In_ IWbemClassObject* Object,
    _In_ PCWSTR Name,
    _Out_ PBOOLEAN Value
    )
{
    VARIANT variant;
    BOOLEAN found = FALSE;

    *Value = FALSE;

    VariantInit(&variant);

    if (HR_FAILED(IWbemClassObject_Get(Object, Name, 0, &variant, NULL, NULL)))
        return FALSE;

    if (V_VT(&variant) == VT_BOOL)
    {
        *Value = !!V_BOOL(&variant);
        found = TRUE;
    }

    VariantClear(&variant);

    return found;
}

/**
 * A CreatorSID is held as an array of bytes, not as a string. Whoever registered the subscription
 * is worth having, and lookup_account turns the SID into a name.
 */
PPH_STRING AtpWmiGetSid(
    _In_ IWbemClassObject* Object,
    _In_ PCWSTR Name
    )
{
    VARIANT variant;
    PPH_STRING string = NULL;
    SAFEARRAY* array;
    PVOID data;
    LONG lower;
    LONG upper;

    VariantInit(&variant);

    if (HR_FAILED(IWbemClassObject_Get(Object, Name, 0, &variant, NULL, NULL)))
        return NULL;

    if (V_VT(&variant) != (VT_ARRAY | VT_UI1) || !V_ARRAY(&variant))
    {
        VariantClear(&variant);
        return NULL;
    }

    array = V_ARRAY(&variant);

    if (HR_SUCCESS(SafeArrayGetLBound(array, 1, &lower)) &&
        HR_SUCCESS(SafeArrayGetUBound(array, 1, &upper)) &&
        upper >= lower &&
        HR_SUCCESS(SafeArrayAccessData(array, &data)))
    {
        // The bytes are whatever was written into the repository, so they are validated as a SID
        // before being read as one.
        if ((ULONG)(upper - lower + 1) >= RtlLengthRequiredSid(0) &&
            RtlValidSid(data) &&
            RtlLengthSid(data) <= (ULONG)(upper - lower + 1))
        {
            string = PhSidToStringSid(data);
        }

        SafeArrayUnaccessData(array);
    }

    VariantClear(&variant);

    return string;
}

VOID AtpWmiFreeFilter(
    _In_ PAT_WMI_FILTER Filter
    )
{
    PhClearReference(&Filter->Name);
    PhClearReference(&Filter->Query);
    PhClearReference(&Filter->QueryLanguage);
    PhClearReference(&Filter->EventNamespace);
    PhClearReference(&Filter->CreatorSid);
    PhClearReference(&Filter->RelativePath);
    PhClearReference(&Filter->Path);
    PhFree(Filter);
}

VOID AtpWmiFreeConsumer(
    _In_ PAT_WMI_CONSUMER Consumer
    )
{
    PhClearReference(&Consumer->Name);
    PhClearReference(&Consumer->ClassName);
    PhClearReference(&Consumer->CreatorSid);
    PhClearReference(&Consumer->RelativePath);
    PhClearReference(&Consumer->Path);
    PhClearReference(&Consumer->CommandLine);
    PhClearReference(&Consumer->ExecutablePath);
    PhClearReference(&Consumer->WorkingDirectory);
    PhClearReference(&Consumer->ScriptingEngine);
    PhClearReference(&Consumer->ScriptText);
    PhClearReference(&Consumer->ScriptFileName);
    PhClearReference(&Consumer->LogFileName);
    PhClearReference(&Consumer->LogText);
    PhFree(Consumer);
}

VOID AtpWmiFreeList(
    _In_ PPH_LIST List,
    _In_ BOOLEAN Consumers
    )
{
    ULONG i;

    for (i = 0; i < List->Count; i++)
    {
        if (Consumers)
            AtpWmiFreeConsumer(List->Items[i]);
        else
            AtpWmiFreeFilter(List->Items[i]);
    }

    PhDereferenceObject(List);
}

HRESULT AtpWmiConnect(
    _In_ PPH_STRING Namespace,
    _Out_ IWbemServices** Services
    )
{
    static CONST PH_STRINGREF wbemPath = PH_STRINGREF_INIT(L"\\wbem\\wbemprox.dll");
    HRESULT status;
    PPH_STRING systemDirectory;
    PPH_STRING fileName;
    IWbemLocator* locator = NULL;
    IWbemServices* services = NULL;
    IClientSecurity* clientSecurity;
    BSTR resource = NULL;

    *Services = NULL;

    // wbemprox.dll lives in System32\wbem, which is not on the loader's search path, so the class
    // is taken from the file rather than from a bare name.
    if (!(systemDirectory = PhGetSystemDirectory()))
        return E_FAIL;

    fileName = PhConcatStringRef2(&systemDirectory->sr, &wbemPath);
    PhDereferenceObject(systemDirectory);

    status = PhGetClassObject(PhGetString(fileName), &CLSID_WbemLocator, &IID_IWbemLocator, &locator);
    PhDereferenceObject(fileName);

    if (HR_FAILED(status))
        return status;

    resource = SysAllocStringLen(Namespace->Buffer, (UINT)(Namespace->Length / sizeof(WCHAR)));

    status = IWbemLocator_ConnectServer(
        locator,
        resource,
        NULL,
        NULL,
        NULL,
        WBEM_FLAG_CONNECT_USE_MAX_WAIT,
        NULL,
        NULL,
        &services
        );

    if (resource)
        SysFreeString(resource);

    IWbemLocator_Release(locator);

    if (HR_FAILED(status))
        return status;

    // Without a blanket the proxy carries no impersonation level and the service refuses the
    // enumeration; this is what PhCoSetProxyBlanket does for the application.
    if (HR_SUCCESS(IWbemServices_QueryInterface(services, &IID_IClientSecurity, &clientSecurity)))
    {
        status = IClientSecurity_SetBlanket(
            clientSecurity,
            (IUnknown*)services,
            RPC_C_AUTHN_WINNT,
            RPC_C_AUTHZ_NONE,
            NULL,
            RPC_C_AUTHN_LEVEL_PKT_PRIVACY,
            RPC_C_IMP_LEVEL_IMPERSONATE,
            NULL,
            EOAC_NONE
            );
        IClientSecurity_Release(clientSecurity);

        if (HR_FAILED(status))
        {
            IWbemServices_Release(services);
            return status;
        }
    }

    *Services = services;

    return S_OK;
}

HRESULT AtpWmiExecQuery(
    _In_ IWbemServices* Services,
    _In_ PCWSTR Query,
    _Out_ IEnumWbemClassObject** Enumerator
    )
{
    static CONST PH_STRINGREF language = PH_STRINGREF_INIT(L"WQL");
    HRESULT status;
    BSTR languageString;
    BSTR queryString;

    *Enumerator = NULL;

    languageString = SysAllocStringLen(language.Buffer, (UINT)(language.Length / sizeof(WCHAR)));
    queryString = SysAllocString(Query);

    status = IWbemServices_ExecQuery(
        Services,
        languageString,
        queryString,
        WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
        NULL,
        Enumerator
        );

    if (queryString)
        SysFreeString(queryString);
    if (languageString)
        SysFreeString(languageString);

    return status;
}

/**
 * The next object of an enumeration, or NULL when there are no more. A timeout is not an end: the
 * caller is told so it can say the list is short rather than say the namespace is empty.
 */
IWbemClassObject* AtpWmiNext(
    _In_ IEnumWbemClassObject* Enumerator,
    _Out_ PBOOLEAN TimedOut
    )
{
    HRESULT status;
    IWbemClassObject* object = NULL;
    ULONG count = 0;

    *TimedOut = FALSE;

    status = IEnumWbemClassObject_Next(Enumerator, AT_WMI_TIMEOUT, 1, &object, &count);

    if (status == WBEM_S_TIMEDOUT)
        *TimedOut = TRUE;

    if (HR_FAILED(status) || count != 1)
        return NULL;

    return object;
}

HRESULT AtpWmiEnumerateFilters(
    _In_ IWbemServices* Services,
    _Inout_ PPH_LIST List,
    _Out_ PBOOLEAN TimedOut
    )
{
    HRESULT status;
    IEnumWbemClassObject* enumerator;
    IWbemClassObject* object;

    status = AtpWmiExecQuery(Services, L"SELECT * FROM __EventFilter", &enumerator);

    if (HR_FAILED(status))
        return status;

    while (List->Count < AT_WMI_MAXIMUM_OBJECTS && (object = AtpWmiNext(enumerator, TimedOut)))
    {
        PAT_WMI_FILTER filter;

        filter = PhAllocateZero(sizeof(AT_WMI_FILTER));
        filter->Name = AtpWmiGetString(object, L"Name");
        filter->Query = AtpWmiGetString(object, L"Query");
        filter->QueryLanguage = AtpWmiGetString(object, L"QueryLanguage");
        filter->EventNamespace = AtpWmiGetString(object, L"EventNamespace");
        filter->CreatorSid = AtpWmiGetSid(object, L"CreatorSID");
        filter->RelativePath = AtpWmiGetString(object, L"__RELPATH");
        filter->Path = AtpWmiGetString(object, L"__PATH");
        PhAddItemList(List, filter);

        IWbemClassObject_Release(object);
    }

    IEnumWbemClassObject_Release(enumerator);

    return S_OK;
}

HRESULT AtpWmiEnumerateConsumers(
    _In_ IWbemServices* Services,
    _Inout_ PPH_LIST List,
    _Out_ PBOOLEAN TimedOut
    )
{
    HRESULT status;
    IEnumWbemClassObject* enumerator;
    IWbemClassObject* object;

    // Every consumer is a subclass of __EventConsumer, so one query over the base class finds the
    // ones this build has never heard of as well as the four that matter.
    status = AtpWmiExecQuery(Services, L"SELECT * FROM __EventConsumer", &enumerator);

    if (HR_FAILED(status))
        return status;

    while (List->Count < AT_WMI_MAXIMUM_OBJECTS && (object = AtpWmiNext(enumerator, TimedOut)))
    {
        PAT_WMI_CONSUMER consumer;

        consumer = PhAllocateZero(sizeof(AT_WMI_CONSUMER));
        consumer->Name = AtpWmiGetString(object, L"Name");
        consumer->ClassName = AtpWmiGetString(object, L"__CLASS");
        consumer->CreatorSid = AtpWmiGetSid(object, L"CreatorSID");
        consumer->RelativePath = AtpWmiGetString(object, L"__RELPATH");
        consumer->Path = AtpWmiGetString(object, L"__PATH");

        // Asking a consumer for a property its class does not have fails, which is how a class this
        // build has never heard of still gets a row with a name and a class on it.
        consumer->CommandLine = AtpWmiGetString(object, L"CommandLineTemplate");
        consumer->ExecutablePath = AtpWmiGetString(object, L"ExecutablePath");
        consumer->WorkingDirectory = AtpWmiGetString(object, L"WorkingDirectory");
        consumer->HasRunInteractively = AtpWmiGetBoolean(object, L"RunInteractively", &consumer->RunInteractively);
        consumer->ScriptingEngine = AtpWmiGetString(object, L"ScriptingEngine");
        consumer->ScriptFileName = AtpWmiGetString(object, L"ScriptFileName");
        consumer->LogFileName = AtpWmiGetString(object, L"Filename");
        consumer->LogText = AtpWmiGetString(object, L"Text");

        if (consumer->ScriptText = AtpWmiGetString(object, L"ScriptText"))
        {
            if (consumer->ScriptText->Length > AT_WMI_MAXIMUM_SCRIPT * sizeof(WCHAR))
            {
                PhMoveReference(
                    &consumer->ScriptText,
                    PhCreateStringEx(consumer->ScriptText->Buffer, AT_WMI_MAXIMUM_SCRIPT * sizeof(WCHAR))
                    );
                consumer->ScriptTextTruncated = TRUE;
            }
        }

        PhAddItemList(List, consumer);

        IWbemClassObject_Release(object);
    }

    IEnumWbemClassObject_Release(enumerator);

    return S_OK;
}

/**
 * Whether a binding's reference names this object. The reference is written either as a relative
 * path or as a full one, so both are compared, and a full reference is compared again from the
 * colon that separates the namespace from the object.
 */
BOOLEAN AtpWmiReferenceMatches(
    _In_opt_ PPH_STRING Reference,
    _In_opt_ PPH_STRING RelativePath,
    _In_opt_ PPH_STRING Path
    )
{
    PH_STRINGREF remaining;
    PH_STRINGREF part;

    if (PhIsNullOrEmptyString(Reference))
        return FALSE;

    if (!PhIsNullOrEmptyString(Path) && PhEqualString(Reference, Path, TRUE))
        return TRUE;

    if (PhIsNullOrEmptyString(RelativePath))
        return FALSE;

    if (PhEqualString(Reference, RelativePath, TRUE))
        return TRUE;

    remaining = Reference->sr;

    if (PhSplitStringRefAtLastChar(&remaining, L':', &part, &remaining))
        return PhEqualStringRef(&remaining, &RelativePath->sr, TRUE);

    return FALSE;
}

BOOLEAN AtpWmiMatchesFilter(
    _In_ PAT_WMI_CONTEXT Context,
    _In_opt_ PAT_WMI_FILTER Filter,
    _In_opt_ PAT_WMI_CONSUMER Consumer,
    _In_opt_ PPH_STRING FilterReference,
    _In_opt_ PPH_STRING ConsumerReference
    )
{
    if (!Context->NameContains)
        return TRUE;

    if (Filter && (AtContainsString(Filter->Name, Context->NameContains) ||
        AtContainsString(Filter->Query, Context->NameContains)))
    {
        return TRUE;
    }

    if (Consumer && (AtContainsString(Consumer->Name, Context->NameContains) ||
        AtContainsString(Consumer->ClassName, Context->NameContains) ||
        AtContainsString(Consumer->CommandLine, Context->NameContains) ||
        AtContainsString(Consumer->ExecutablePath, Context->NameContains) ||
        AtContainsString(Consumer->ScriptText, Context->NameContains) ||
        AtContainsString(Consumer->ScriptFileName, Context->NameContains)))
    {
        return TRUE;
    }

    return AtContainsString(FilterReference, Context->NameContains) ||
        AtContainsString(ConsumerReference, Context->NameContains);
}

PVOID AtpWmiFilterObject(
    _In_ PAT_WMI_FILTER Filter
    )
{
    PVOID object;

    object = PhCreateJsonObject();
    AtJsonAddString(object, "name", Filter->Name);
    AtJsonAddString(object, "query", Filter->Query);
    AtJsonAddString(object, "query_language", Filter->QueryLanguage);
    AtJsonAddString(object, "event_namespace", Filter->EventNamespace);
    AtJsonAddString(object, "creator_sid", Filter->CreatorSid);
    AtJsonAddString(object, "path", Filter->RelativePath);

    return object;
}

PVOID AtpWmiConsumerObject(
    _In_ PAT_WMI_CONSUMER Consumer
    )
{
    PVOID object;

    object = PhCreateJsonObject();
    AtJsonAddString(object, "name", Consumer->Name);
    AtJsonAddString(object, "class", Consumer->ClassName);
    AtJsonAddString(object, "creator_sid", Consumer->CreatorSid);
    AtJsonAddString(object, "path", Consumer->RelativePath);
    AtJsonAddString(object, "command_line", Consumer->CommandLine);
    AtJsonAddString(object, "executable_path", Consumer->ExecutablePath);
    AtJsonAddString(object, "working_directory", Consumer->WorkingDirectory);

    if (Consumer->HasRunInteractively)
        PhAddJsonObjectBoolean(object, "run_interactively", Consumer->RunInteractively);
    else
        AtJsonAddNull(object, "run_interactively");

    AtJsonAddString(object, "scripting_engine", Consumer->ScriptingEngine);
    AtJsonAddString(object, "script_file_name", Consumer->ScriptFileName);
    AtJsonAddString(object, "script_text", Consumer->ScriptText);
    PhAddJsonObjectBoolean(object, "script_text_truncated", Consumer->ScriptTextTruncated);
    AtJsonAddString(object, "log_file_name", Consumer->LogFileName);
    AtJsonAddString(object, "log_text", Consumer->LogText);

    return object;
}

VOID AtpWmiAddRow(
    _In_ PAT_WMI_CONTEXT Context,
    _In_ PCSTR Kind,
    _In_opt_ PAT_WMI_FILTER Filter,
    _In_opt_ PAT_WMI_CONSUMER Consumer,
    _In_opt_ PPH_STRING FilterReference,
    _In_opt_ PPH_STRING ConsumerReference,
    _In_ BOOLEAN HasDeliverSynchronously,
    _In_ BOOLEAN DeliverSynchronously
    )
{
    PVOID row;
    PPH_STRING name;

    if (!AtpWmiMatchesFilter(Context, Filter, Consumer, FilterReference, ConsumerReference))
        return;

    if (Consumer && Consumer->Name)
        name = Consumer->Name;
    else if (Filter && Filter->Name)
        name = Filter->Name;
    else
        name = ConsumerReference ? ConsumerReference : FilterReference;

    row = PhCreateJsonObject();
    PhAddJsonObject(row, "kind", Kind);
    AtJsonAddString(row, "namespace", Context->Namespace);
    AtJsonAddString(row, "name", name);

    if (Filter)
        PhAddJsonObjectValue(row, "filter", AtpWmiFilterObject(Filter));
    else
        AtJsonAddNull(row, "filter");

    if (Consumer)
        PhAddJsonObjectValue(row, "consumer", AtpWmiConsumerObject(Consumer));
    else
        AtJsonAddNull(row, "consumer");

    AtJsonAddString(row, "filter_reference", FilterReference);
    AtJsonAddString(row, "consumer_reference", ConsumerReference);

    if (HasDeliverSynchronously)
        PhAddJsonObjectBoolean(row, "deliver_synchronously", DeliverSynchronously);
    else
        AtJsonAddNull(row, "deliver_synchronously");

    AtAddRow(Context->Rows, row);
}

HRESULT AtpWmiEnumerateBindings(
    _In_ PAT_WMI_CONTEXT Context,
    _In_ IWbemServices* Services,
    _In_ PPH_LIST Filters,
    _In_ PPH_LIST Consumers,
    _Out_ PULONG Count,
    _Out_ PBOOLEAN TimedOut
    )
{
    HRESULT status;
    IEnumWbemClassObject* enumerator;
    IWbemClassObject* object;

    *Count = 0;

    status = AtpWmiExecQuery(Services, L"SELECT * FROM __FilterToConsumerBinding", &enumerator);

    if (HR_FAILED(status))
        return status;

    while (*Count < AT_WMI_MAXIMUM_OBJECTS && (object = AtpWmiNext(enumerator, TimedOut)))
    {
        PAT_WMI_FILTER filter = NULL;
        PAT_WMI_CONSUMER consumer = NULL;
        PPH_STRING filterReference;
        PPH_STRING consumerReference;
        BOOLEAN deliverSynchronously;
        BOOLEAN hasDeliverSynchronously;
        ULONG i;

        (*Count)++;

        filterReference = AtpWmiGetString(object, L"Filter");
        consumerReference = AtpWmiGetString(object, L"Consumer");
        hasDeliverSynchronously = AtpWmiGetBoolean(object, L"DeliverSynchronously", &deliverSynchronously);

        for (i = 0; i < Filters->Count; i++)
        {
            PAT_WMI_FILTER entry = Filters->Items[i];

            if (AtpWmiReferenceMatches(filterReference, entry->RelativePath, entry->Path))
            {
                entry->Bound = TRUE;
                filter = entry;
                break;
            }
        }

        for (i = 0; i < Consumers->Count; i++)
        {
            PAT_WMI_CONSUMER entry = Consumers->Items[i];

            if (AtpWmiReferenceMatches(consumerReference, entry->RelativePath, entry->Path))
            {
                entry->Bound = TRUE;
                consumer = entry;
                break;
            }
        }

        AtpWmiAddRow(
            Context,
            "binding",
            filter,
            consumer,
            filterReference,
            consumerReference,
            hasDeliverSynchronously,
            deliverSynchronously
            );

        PhClearReference(&filterReference);
        PhClearReference(&consumerReference);

        IWbemClassObject_Release(object);
    }

    IEnumWbemClassObject_Release(enumerator);

    return S_OK;
}

/**
 * One namespace: its filters, its consumers, and the bindings that tie them together. The entry
 * added to Namespaces says whether the namespace could be read at all, because an empty list from a
 * namespace nobody may enumerate reads as "there is no persistence here".
 */
VOID AtpWmiReadNamespace(
    _In_ PAT_WMI_CONTEXT Context,
    _In_ PPH_STRING Namespace,
    _In_ PVOID Namespaces
    )
{
    HRESULT status;
    PVOID entry;
    IWbemServices* services = NULL;
    PPH_LIST filters = NULL;
    PPH_LIST consumers = NULL;
    ULONG bindingCount = 0;
    BOOLEAN timedOut = FALSE;
    ULONG i;

    entry = PhCreateJsonObject();
    AtJsonAddString(entry, "namespace", Namespace);
    PhSetReference(&Context->Namespace, Namespace);

    status = AtpWmiConnect(Namespace, &services);

    if (HR_FAILED(status))
    {
        PhAddJsonObjectBoolean(entry, "readable", FALSE);
        AtJsonAddNull(entry, "filter_count");
        AtJsonAddNull(entry, "consumer_count");
        AtJsonAddNull(entry, "binding_count");
        PhAddJsonObjectBoolean(entry, "timed_out", FALSE);
        PhAddJsonObject(entry, "error", status == WBEM_E_ACCESS_DENIED ? "access_denied" :
            (status == WBEM_E_INVALID_NAMESPACE ? "not_found" : "failed"));
        AtJsonAddHex(entry, "error_code", (ULONG)status);
        PhAddJsonArrayObject(Namespaces, entry);
        return;
    }

    filters = PhCreateList(8);
    consumers = PhCreateList(8);

    if (HR_FAILED(status = AtpWmiEnumerateFilters(services, filters, &timedOut)))
        goto CleanupExit;

    if (HR_FAILED(status = AtpWmiEnumerateConsumers(services, consumers, &timedOut)))
        goto CleanupExit;

    if (HR_FAILED(status = AtpWmiEnumerateBindings(Context, services, filters, consumers, &bindingCount, &timedOut)))
        goto CleanupExit;

    // A filter with no binding runs nothing, and a consumer with no binding runs nothing - but a
    // half-built or half-removed subscription is worth seeing, so neither is dropped.
    for (i = 0; i < filters->Count; i++)
    {
        PAT_WMI_FILTER filter = filters->Items[i];

        if (!filter->Bound)
            AtpWmiAddRow(Context, "unbound_filter", filter, NULL, NULL, NULL, FALSE, FALSE);
    }

    for (i = 0; i < consumers->Count; i++)
    {
        PAT_WMI_CONSUMER consumer = consumers->Items[i];

        if (!consumer->Bound)
            AtpWmiAddRow(Context, "unbound_consumer", NULL, consumer, NULL, NULL, FALSE, FALSE);
    }

CleanupExit:
    if (HR_FAILED(status))
    {
        PhAddJsonObjectBoolean(entry, "readable", FALSE);
        AtJsonAddNull(entry, "filter_count");
        AtJsonAddNull(entry, "consumer_count");
        AtJsonAddNull(entry, "binding_count");
        PhAddJsonObjectBoolean(entry, "timed_out", timedOut);
        PhAddJsonObject(entry, "error", status == WBEM_E_ACCESS_DENIED ? "access_denied" : "failed");
        AtJsonAddHex(entry, "error_code", (ULONG)status);
    }
    else
    {
        PhAddJsonObjectBoolean(entry, "readable", TRUE);
        PhAddJsonObjectUInt64(entry, "filter_count", filters->Count);
        PhAddJsonObjectUInt64(entry, "consumer_count", consumers->Count);
        PhAddJsonObjectUInt64(entry, "binding_count", bindingCount);
        PhAddJsonObjectBoolean(entry, "timed_out", timedOut);
        AtJsonAddNull(entry, "error");
        AtJsonAddNull(entry, "error_code");
    }

    PhAddJsonArrayObject(Namespaces, entry);

    if (filters)
        AtpWmiFreeList(filters, FALSE);
    if (consumers)
        AtpWmiFreeList(consumers, TRUE);
    if (services)
        IWbemServices_Release(services);
}

VOID AtpListWmiSubscriptions(
    _In_ PAT_TOOL_CALL Call,
    _Inout_ PAT_TOOL_RESULT Result
    )
{
    static CONST PWSTR defaultNamespaces[] =
    {
        L"root\\subscription",
        L"root\\default",
        L"root\\cimv2"
    };
    AT_WMI_CONTEXT context;
    AT_ROWS rows;
    PVOID structured;
    PVOID namespaces;
    PPH_STRING requested;
    ULONG i;

    memset(&context, 0, sizeof(AT_WMI_CONTEXT));
    context.Rows = &rows;
    context.NameContains = AtGetArgumentString(Call->Arguments, "name_contains");

    requested = AtGetArgumentString(Call->Arguments, "namespace");

    structured = PhCreateJsonObject();
    namespaces = PhCreateJsonArray();
    AtInitializeRows(&rows, Call->Arguments);

    if (requested)
    {
        AtpWmiReadNamespace(&context, requested, namespaces);
    }
    else
    {
        for (i = 0; i < RTL_NUMBER_OF(defaultNamespaces); i++)
        {
            PPH_STRING name;

            name = PhCreateString(defaultNamespaces[i]);
            AtpWmiReadNamespace(&context, name, namespaces);
            PhDereferenceObject(name);
        }
    }

    AtAddRows(structured, "subscriptions", &rows);
    PhAddJsonObjectValue(structured, "namespaces", namespaces);
    AtAddSnapshot(structured);

    Result->StructuredContent = structured;

    PhClearReference(&context.Namespace);
    PhClearReference(&requested);
    PhClearReference(&context.NameContains);
}

VOID AtWmiInvokeTool(
    _In_ PCAT_TOOL Tool,
    _In_ PAT_TOOL_CALL Call,
    _Inout_ PAT_TARGET Target,
    _Inout_ PAT_TOOL_RESULT Result
    )
{
    UNREFERENCED_PARAMETER(Target);

    switch (Tool->Action)
    {
    case AtActionListWmiSubscriptions:
        AtpListWmiSubscriptions(Call, Result);
        break;
    default:
        AtSetToolError(Result, "failed", STATUS_NOT_IMPLEMENTED, L"This tool is not implemented.");
        break;
    }
}
