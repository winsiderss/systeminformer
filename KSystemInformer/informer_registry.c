/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     jxy-s   2023
 *
 */

#include <kph.h>
#include <informer.h>
#include <comms.h>
#include <kphmsgdyn.h>

#include <trace.h>

typedef union _KPH_REG_OPTIONS
{
    UCHAR Flags;
    struct
    {
        UCHAR InPost : 1;
        UCHAR PreEnabled : 1;
        UCHAR PostEnabled : 1;
        UCHAR EnableStackTraces : 1;
        UCHAR EnableValueBuffers : 1;
        UCHAR Spare : 3;
    };
} KPH_REG_OPTIONS, *PKPH_REG_OPTIONS;

typedef struct _KPH_REG_CALL_CONTEXT
{
    ULONG64 PreSequence;
    LARGE_INTEGER PreTimeStamp;
    KPH_REG_OPTIONS Options;
    ULONG_PTR ObjectId;
    PUNICODE_STRING ObjectName;
    PVOID Transaction;
} KPH_REG_CALL_CONTEXT, *PKPH_REG_CALL_CONTEXT;

typedef union _KPH_REG_PRE_INFORMATION
{
    //
    // Invalid for CreateKey and OpenKey. RootObject for LoadKey. Always NULL
    // for SaveMergedKey. Otherwise valid.
    //
    PVOID Object;

    REG_DELETE_KEY_INFORMATION DeleteKey;
    REG_SET_VALUE_KEY_INFORMATION SetValueKey;
    REG_DELETE_VALUE_KEY_INFORMATION DeleteValueKey;
    REG_SET_INFORMATION_KEY_INFORMATION SetInformationKey;
    REG_RENAME_KEY_INFORMATION RenameKey;
    REG_ENUMERATE_KEY_INFORMATION EnumerateKey;
    REG_ENUMERATE_VALUE_KEY_INFORMATION EnumerateValueKey;
    REG_QUERY_KEY_INFORMATION QueryKey;
    REG_QUERY_VALUE_KEY_INFORMATION QueryValueKey;
    REG_QUERY_MULTIPLE_VALUE_KEY_INFORMATION QueryMultipleValueKey;
    REG_KEY_HANDLE_CLOSE_INFORMATION KeyHandleClose;
    REG_CREATE_KEY_INFORMATION_V1 CreateKey;
    REG_OPEN_KEY_INFORMATION_V1 OpenKey;
    REG_FLUSH_KEY_INFORMATION FlushKey;
    REG_LOAD_KEY_INFORMATION_V2 LoadKey;
    REG_UNLOAD_KEY_INFORMATION UnLoadKey;
    REG_QUERY_KEY_SECURITY_INFORMATION QueryKeySecurity;
    REG_SET_KEY_SECURITY_INFORMATION SetKeySecurity;
    REG_RESTORE_KEY_INFORMATION RestoreKey;
    REG_SAVE_KEY_INFORMATION SaveKey;
    REG_REPLACE_KEY_INFORMATION ReplaceKey;
    REG_QUERY_KEY_NAME QueryKeyName;
    REG_SAVE_MERGED_KEY_INFORMATION SaveMergedKey;
} KPH_REG_PRE_INFORMATION, *PKPH_REG_PRE_INFORMATION;

typedef union _KPH_REG_POST_INFORMATION
{
    //
    // Must inspect Common.Status for STATUS_SUCCESS for CreateKey and OpenKey.
    // Always NULL for LoadKey and SaveMergedKey. Unsafe to use with
    // CmCallbackGetKeyObjectIDEx during post KeyHandleClose. Otherwise valid.
    //
    PVOID Object;

    REG_POST_OPERATION_INFORMATION Common;
} KPH_REG_POST_INFORMATION, *PKPH_REG_POST_INFORMATION;

KPH_PROTECTED_DATA_SECTION_RO_PUSH();
static const UNICODE_STRING KphpCmDefaultValueName = RTL_CONSTANT_STRING(L"(Default)");
KPH_PROTECTED_DATA_SECTION_RO_POP();
static PAGED_LOOKASIDE_LIST KphpCmCallContextLookaside = { 0 };

PAGED_FILE();

static volatile ULONG64 KphpCmSequence = 0;
static BOOLEAN KphpCmRegistered = FALSE;
static LARGE_INTEGER KphpCmCookie = { 0 };

/**
 * \brief Retrieves the registry operation options for the specified class.
 *
 * \param[in] RegClass The registry operation class.
 *
 * \return Options for the specified class.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
KPH_REG_OPTIONS KphpRegGetOptions(
    _In_ REG_NOTIFY_CLASS RegClass
    )
{
    KPH_REG_OPTIONS options;
    PKPH_PROCESS_CONTEXT process;

    PAGED_CODE_PASSIVE();

    options.Flags = 0;

    if (ExGetPreviousMode() != KernelMode)
    {
        process = KphGetCurrentProcessContext();
    }
    else
    {
        process = KphGetProcessContext(PsGetProcessId(PsInitialSystemProcess));
    }

#define KPH_REG_SETTING2(reg, name)                                           \
    case RegNtPre##reg:                                                       \
    {                                                                         \
        if (KphInformerEnabled(RegPre##name, process))                        \
        {                                                                     \
            options.PreEnabled = TRUE;                                        \
        }                                                                     \
        if (KphInformerEnabled(RegPost##name, process))                       \
        {                                                                     \
            options.PostEnabled = TRUE;                                       \
        }                                                                     \
        break;                                                                \
    }                                                                         \
    case RegNtPost##reg:                                                      \
    {                                                                         \
        options.InPost = TRUE;                                                \
        break;                                                                \
    }
#define KPH_REG_SETTING(name) KPH_REG_SETTING2(name, name)

    switch (RegClass)
    {
        KPH_REG_SETTING(DeleteKey)
        KPH_REG_SETTING(SetValueKey)
        KPH_REG_SETTING(DeleteValueKey)
        KPH_REG_SETTING(SetInformationKey)
        KPH_REG_SETTING(RenameKey)
        KPH_REG_SETTING(EnumerateKey)
        KPH_REG_SETTING(EnumerateValueKey)
        KPH_REG_SETTING(QueryKey)
        KPH_REG_SETTING(QueryValueKey)
        KPH_REG_SETTING(QueryMultipleValueKey)
        KPH_REG_SETTING(KeyHandleClose)
        KPH_REG_SETTING2(CreateKeyEx, CreateKey)
        KPH_REG_SETTING2(OpenKeyEx, OpenKey)
        KPH_REG_SETTING(FlushKey)
        KPH_REG_SETTING(LoadKey)
        KPH_REG_SETTING(UnLoadKey)
        KPH_REG_SETTING(QueryKeySecurity)
        KPH_REG_SETTING(SetKeySecurity)
        KPH_REG_SETTING(RestoreKey)
        KPH_REG_SETTING(SaveKey)
        KPH_REG_SETTING(ReplaceKey)
        KPH_REG_SETTING(QueryKeyName)
        KPH_REG_SETTING(SaveMergedKey)
        case RegNtPreCreateKey:   // XP
        case RegNtPostCreateKey:  // XP
        case RegNtPreOpenKey:     // XP
        case RegNtPostOpenKey:    // XP
        default:
        {
            NT_ASSERT(FALSE);
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          INFORMER,
                          "Unsupported registry operation: %lu",
                          RegClass);
            break;
        }
    }

    if (!options.InPost && (options.PreEnabled || options.PostEnabled))
    {
        options.EnableStackTraces = KphInformerEnabled(EnableStackTraces, process);
        options.EnableValueBuffers = KphInformerEnabled(RegEnableValueBuffers, process);
    }

    if (process)
    {
        KphDereferenceObject(process);
    }

    return options;
}

/**
 * \brief Retrieves the message ID for the specified registry operation class.
 *
 * \param[in] RegClass The registry operation class.
 *
 * \return The message ID for the specified class.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
KPH_MESSAGE_ID KphpRegGetMessageId(
    _In_ REG_NOTIFY_CLASS RegClass
    )
{
    KPH_MESSAGE_ID messageId;

    PAGED_CODE_PASSIVE();

#define KPH_REG_MESSAGE_ID2(reg, name)                                        \
    case RegNtPre##reg:                                                       \
    {                                                                         \
        messageId = KphMsgRegPre##name;                                       \
        break;                                                                \
    }                                                                         \
    case RegNtPost##reg:                                                      \
    {                                                                         \
        messageId = KphMsgRegPost##name;                                      \
        break;                                                                \
    }
#define KPH_REG_MESSAGE_ID(name) KPH_REG_MESSAGE_ID2(name, name)

    switch (RegClass)
    {
        KPH_REG_MESSAGE_ID(DeleteKey)
        KPH_REG_MESSAGE_ID(SetValueKey)
        KPH_REG_MESSAGE_ID(DeleteValueKey)
        KPH_REG_MESSAGE_ID(SetInformationKey)
        KPH_REG_MESSAGE_ID(RenameKey)
        KPH_REG_MESSAGE_ID(EnumerateKey)
        KPH_REG_MESSAGE_ID(EnumerateValueKey)
        KPH_REG_MESSAGE_ID(QueryKey)
        KPH_REG_MESSAGE_ID(QueryValueKey)
        KPH_REG_MESSAGE_ID(QueryMultipleValueKey)
        KPH_REG_MESSAGE_ID(KeyHandleClose)
        KPH_REG_MESSAGE_ID2(CreateKeyEx, CreateKey)
        KPH_REG_MESSAGE_ID2(OpenKeyEx, OpenKey)
        KPH_REG_MESSAGE_ID(FlushKey)
        KPH_REG_MESSAGE_ID(LoadKey)
        KPH_REG_MESSAGE_ID(UnLoadKey)
        KPH_REG_MESSAGE_ID(QueryKeySecurity)
        KPH_REG_MESSAGE_ID(SetKeySecurity)
        KPH_REG_MESSAGE_ID(RestoreKey)
        KPH_REG_MESSAGE_ID(SaveKey)
        KPH_REG_MESSAGE_ID(ReplaceKey)
        KPH_REG_MESSAGE_ID(QueryKeyName)
        KPH_REG_MESSAGE_ID(SaveMergedKey)
        DEFAULT_UNREACHABLE;
    }

    return messageId;
}

/**
 * \brief Populates a registry operation message with common information.
 *
 * \param[in,out] Message The message to populate.
 * \param[in] RegClass The registry operation class.
 * \param[in] PreInfo The pre-operation information.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphpRegFillCommonMessage(
    _Inout_ PKPH_MESSAGE Message,
    _In_ REG_NOTIFY_CLASS RegClass,
    _In_ PKPH_REG_PRE_INFORMATION PreInfo
    )
{
    PAGED_CODE_PASSIVE();

    Message->Kernel.Reg.ClientId.UniqueProcess = PsGetCurrentProcessId();
    Message->Kernel.Reg.ClientId.UniqueThread = PsGetCurrentThreadId();
    Message->Kernel.Reg.PreviousMode = (ExGetPreviousMode() != KernelMode);

#define KPH_REG_COPY_PARAMETER(name, value)                                   \
    Message->Kernel.Reg.Parameters.##name.##value = PreInfo->##name.##value

    switch (RegClass)
    {
        case RegNtPreDeleteKey:
        case RegNtPostDeleteKey:
        {
            break;
        }
        case RegNtPreSetValueKey:
        case RegNtPostSetValueKey:
        {
            KPH_REG_COPY_PARAMETER(SetValueKey, TitleIndex);
            KPH_REG_COPY_PARAMETER(SetValueKey, Type);
            KPH_REG_COPY_PARAMETER(SetValueKey, Data);
            KPH_REG_COPY_PARAMETER(SetValueKey, DataSize);
            break;
        }
        case RegNtPreDeleteValueKey:
        case RegNtPostDeleteValueKey:
        {
            KPH_REG_COPY_PARAMETER(DeleteValueKey, ValueName);
            break;
        }
        case RegNtPreSetInformationKey:
        case RegNtPostSetInformationKey:
        {
            KPH_REG_COPY_PARAMETER(SetInformationKey, KeySetInformationClass);
            KPH_REG_COPY_PARAMETER(SetInformationKey, KeySetInformation);
            KPH_REG_COPY_PARAMETER(SetInformationKey, KeySetInformationLength);
            break;
        }
        case RegNtPreRenameKey:
        case RegNtPostRenameKey:
        {
            KPH_REG_COPY_PARAMETER(RenameKey, NewName);
            break;
        }
        case RegNtPreEnumerateKey:
        case RegNtPostEnumerateKey:
        {
            KPH_REG_COPY_PARAMETER(EnumerateKey, Index);
            KPH_REG_COPY_PARAMETER(EnumerateKey, KeyInformationClass);
            KPH_REG_COPY_PARAMETER(EnumerateKey, KeyInformation);
            KPH_REG_COPY_PARAMETER(EnumerateKey, Length);
            KPH_REG_COPY_PARAMETER(EnumerateKey, ResultLength);
            break;
        }
        case RegNtPreEnumerateValueKey:
        case RegNtPostEnumerateValueKey:
        {
            KPH_REG_COPY_PARAMETER(EnumerateValueKey, Index);
            KPH_REG_COPY_PARAMETER(EnumerateValueKey, KeyValueInformationClass);
            KPH_REG_COPY_PARAMETER(EnumerateValueKey, KeyValueInformation);
            KPH_REG_COPY_PARAMETER(EnumerateValueKey, Length);
            KPH_REG_COPY_PARAMETER(EnumerateValueKey, ResultLength);
            break;
        }
        case RegNtPreQueryKey:
        case RegNtPostQueryKey:
        {
            KPH_REG_COPY_PARAMETER(QueryKey, KeyInformationClass);
            KPH_REG_COPY_PARAMETER(QueryKey, KeyInformation);
            KPH_REG_COPY_PARAMETER(QueryKey, Length);
            KPH_REG_COPY_PARAMETER(QueryKey, ResultLength);
            break;
        }
        case RegNtPreQueryValueKey:
        case RegNtPostQueryValueKey:
        {
            KPH_REG_COPY_PARAMETER(QueryValueKey, ValueName);
            KPH_REG_COPY_PARAMETER(QueryValueKey, KeyValueInformationClass);
            KPH_REG_COPY_PARAMETER(QueryValueKey, KeyValueInformation);
            KPH_REG_COPY_PARAMETER(QueryValueKey, Length);
            KPH_REG_COPY_PARAMETER(QueryValueKey, ResultLength);
            break;
        }
        case RegNtPreQueryMultipleValueKey:
        case RegNtPostQueryMultipleValueKey:
        {
            KPH_REG_COPY_PARAMETER(QueryMultipleValueKey, ValueEntries);
            KPH_REG_COPY_PARAMETER(QueryMultipleValueKey, EntryCount);
            KPH_REG_COPY_PARAMETER(QueryMultipleValueKey, ValueBuffer);
            KPH_REG_COPY_PARAMETER(QueryMultipleValueKey, BufferLength);
            KPH_REG_COPY_PARAMETER(QueryMultipleValueKey, RequiredBufferLength);
            break;
        }
        case RegNtPreKeyHandleClose:
        case RegNtPostKeyHandleClose:
        {
            break;
        }
        case RegNtPreCreateKeyEx:
        case RegNtPostCreateKeyEx:
        {
            KPH_REG_COPY_PARAMETER(CreateKey, CompleteName);
            KPH_REG_COPY_PARAMETER(CreateKey, RootObject);
            KPH_REG_COPY_PARAMETER(CreateKey, ObjectType);
            KPH_REG_COPY_PARAMETER(CreateKey, Options);
            KPH_REG_COPY_PARAMETER(CreateKey, Class);
            KPH_REG_COPY_PARAMETER(CreateKey, SecurityDescriptor);
            KPH_REG_COPY_PARAMETER(CreateKey, SecurityQualityOfService);
            KPH_REG_COPY_PARAMETER(CreateKey, DesiredAccess);
            KPH_REG_COPY_PARAMETER(CreateKey, GrantedAccess);
            KPH_REG_COPY_PARAMETER(CreateKey, Disposition);
            KPH_REG_COPY_PARAMETER(CreateKey, RemainingName);
            KPH_REG_COPY_PARAMETER(CreateKey, Wow64Flags);
            KPH_REG_COPY_PARAMETER(CreateKey, Attributes);
            KPH_REG_COPY_PARAMETER(CreateKey, CheckAccessMode);
            break;
        }
        case RegNtPreOpenKeyEx:
        case RegNtPostOpenKeyEx:
        {
            KPH_REG_COPY_PARAMETER(OpenKey, CompleteName);
            KPH_REG_COPY_PARAMETER(OpenKey, RootObject);
            KPH_REG_COPY_PARAMETER(OpenKey, ObjectType);
            KPH_REG_COPY_PARAMETER(OpenKey, Options);
            KPH_REG_COPY_PARAMETER(OpenKey, Class);
            KPH_REG_COPY_PARAMETER(OpenKey, SecurityDescriptor);
            KPH_REG_COPY_PARAMETER(OpenKey, SecurityQualityOfService);
            KPH_REG_COPY_PARAMETER(OpenKey, DesiredAccess);
            KPH_REG_COPY_PARAMETER(OpenKey, GrantedAccess);
            KPH_REG_COPY_PARAMETER(OpenKey, Disposition);
            KPH_REG_COPY_PARAMETER(OpenKey, RemainingName);
            KPH_REG_COPY_PARAMETER(OpenKey, Wow64Flags);
            KPH_REG_COPY_PARAMETER(OpenKey, Attributes);
            KPH_REG_COPY_PARAMETER(OpenKey, CheckAccessMode);
            break;
        }
        case RegNtPreFlushKey:
        case RegNtPostFlushKey:
        {
            break;
        }
        case RegNtPreLoadKey:
        case RegNtPostLoadKey:
        {
            Message->Kernel.Reg.Parameters.LoadKey.RootObject = PreInfo->LoadKey.Object;
            KPH_REG_COPY_PARAMETER(LoadKey, KeyName);
            KPH_REG_COPY_PARAMETER(LoadKey, SourceFile);
            KPH_REG_COPY_PARAMETER(LoadKey, Flags);
            KPH_REG_COPY_PARAMETER(LoadKey, TrustClassObject);
            KPH_REG_COPY_PARAMETER(LoadKey, UserEvent);
            KPH_REG_COPY_PARAMETER(LoadKey, DesiredAccess);
            KPH_REG_COPY_PARAMETER(LoadKey, RootHandle);
            KPH_REG_COPY_PARAMETER(LoadKey, FileAccessToken);
            break;
        }
        case RegNtPreUnLoadKey:
        case RegNtPostUnLoadKey:
        {
            KPH_REG_COPY_PARAMETER(UnLoadKey, UserEvent);
            break;
        }
        case RegNtPreQueryKeySecurity:
        case RegNtPostQueryKeySecurity:
        {
            KPH_REG_COPY_PARAMETER(QueryKeySecurity, SecurityInformation);
            KPH_REG_COPY_PARAMETER(QueryKeySecurity, SecurityDescriptor);
            KPH_REG_COPY_PARAMETER(QueryKeySecurity, Length);
            break;
        }
        case RegNtPreSetKeySecurity:
        case RegNtPostSetKeySecurity:
        {
            KPH_REG_COPY_PARAMETER(SetKeySecurity, SecurityInformation);
            KPH_REG_COPY_PARAMETER(SetKeySecurity, SecurityDescriptor);
            break;
        }
        case RegNtPreRestoreKey:
        case RegNtPostRestoreKey:
        {
            KPH_REG_COPY_PARAMETER(RestoreKey, FileHandle);
            KPH_REG_COPY_PARAMETER(RestoreKey, Flags);
            break;
        }
        case RegNtPreSaveKey:
        case RegNtPostSaveKey:
        {
            KPH_REG_COPY_PARAMETER(SaveKey, FileHandle);
            KPH_REG_COPY_PARAMETER(SaveKey, Format);
            break;
        }
        case RegNtPreReplaceKey:
        case RegNtPostReplaceKey:
        {
            KPH_REG_COPY_PARAMETER(ReplaceKey, OldFileName);
            KPH_REG_COPY_PARAMETER(ReplaceKey, NewFileName);
            break;
        }
        case RegNtPreQueryKeyName:
        case RegNtPostQueryKeyName:
        {
            KPH_REG_COPY_PARAMETER(QueryKeyName, ObjectNameInfo);
            KPH_REG_COPY_PARAMETER(QueryKeyName, Length);
            KPH_REG_COPY_PARAMETER(QueryKeyName, ReturnLength);
            break;
        }
        case RegNtPreSaveMergedKey:
        case RegNtPostSaveMergedKey:
        {
            KPH_REG_COPY_PARAMETER(SaveMergedKey, FileHandle);
            KPH_REG_COPY_PARAMETER(SaveMergedKey, HighKeyObject);
            KPH_REG_COPY_PARAMETER(SaveMergedKey, LowKeyObject);
            break;
        }
        DEFAULT_UNREACHABLE;
    }
}

/**
 * \brief Copies the object name information into a message. And returns other
 * associated information for the object.
 *
 * \param[in,out] Message The message to populate.
 * \param[in] FieldId The object name field ID to populate.
 * \param[in] InputObject The object to copy the information from.
 * \param[out] Object Receives a copy of the input object pointer.
 * \param[out] ObjectId Receives the object ID for the object.
 * \param[out] Transaction Receives the transaction for the object, if any.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphpRegCopyObjectInfo(
    _Inout_ PKPH_MESSAGE Message,
    _In_ KPH_MESSAGE_FIELD_ID FieldId,
    _In_ PVOID InputObject,
    _Out_opt_ PVOID* Object,
    _Out_opt_ PULONG_PTR ObjectId,
    _Out_opt_ PVOID* Transaction
    )
{
    NTSTATUS status;
    PUNICODE_STRING objectName;
    PUNICODE_STRING* objectNamePointer;

    PAGED_CODE_PASSIVE();

    objectName = NULL;

    if (Object)
    {
        *Object = InputObject;
    }

    if (ObjectId)
    {
        *ObjectId = 0;
    }

    if (Transaction)
    {
        *Transaction = CmGetBoundTransaction(&KphpCmCookie, InputObject);
    }

    if (FieldId != InvalidKphMsgField)
    {
        objectNamePointer = &objectName;
    }
    else
    {
        objectNamePointer = NULL;
    }

    status = CmCallbackGetKeyObjectIDEx(&KphpCmCookie,
                                        InputObject,
                                        ObjectId,
                                        objectNamePointer,
                                        0);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      INFORMER,
                      "CmCallbackGetKeyObjectIDEx failed: %!STATUS!",
                      status);

        if (ObjectId)
        {
            *ObjectId = 0;
        }

        objectName = NULL;
        goto Exit;
    }

    if (FieldId != InvalidKphMsgField)
    {
        status = KphMsgDynAddUnicodeString(Message, FieldId, objectName);
        if (!NT_SUCCESS(status))
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          INFORMER,
                          "KphMsgDynAddUnicodeString failed: %!STATUS!",
                          status);
        }
    }

Exit:

    if (objectName)
    {
        CmCallbackReleaseKeyObjectIDEx(objectName);
    }
}

/**
 * \brief Fills a message with information about the specified registry object.
 *
 * \param[in,out] Message The message to populate.
 * \param[in] Object The object to copy the information from.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphpRegFillObjectInfo(
    _Inout_ PKPH_MESSAGE Message,
    _In_ PVOID Object
    )
{
    PAGED_CODE_PASSIVE();

    KphpRegCopyObjectInfo(Message,
                          KphMsgFieldObjectName,
                          Object,
                          &Message->Kernel.Reg.Object,
                          &Message->Kernel.Reg.ObjectId,
                          &Message->Kernel.Reg.Transaction);
}

/**
 * \brief Creates a registry object name from a root object and relative name.
 *
 * \param[in] RootObject The root registry object.
 * \param[in] RelativeName The relative name from the root object.
 * \param[out] ObjectName Receives a pointer to the allocated object name.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphpRegMakeObjectName(
    _In_ PVOID RootObject,
    _In_ PUNICODE_STRING RelativeName,
    _Outptr_allocatesMem_ PUNICODE_STRING* ObjectName
    )
{
    NTSTATUS status;
    PUNICODE_STRING rootName;
    PUNICODE_STRING objectName;
    BOOLEAN needsSeparator;
    ULONG length;

    PAGED_CODE_PASSIVE();

    *ObjectName = NULL;

    objectName = NULL;

    status = CmCallbackGetKeyObjectIDEx(&KphpCmCookie,
                                        RootObject,
                                        NULL,
                                        &rootName,
                                        0);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      INFORMER,
                      "CmCallbackGetKeyObjectIDEx failed: %!STATUS!",
                      status);

        rootName = NULL;
        goto Exit;
    }

    length = (rootName->Length + RelativeName->Length);

    if ((rootName->Length >= sizeof(WCHAR)) &&
        (rootName->Buffer[(rootName->Length / sizeof(WCHAR)) - 1] != L'\\') &&
        (RelativeName->Length >= sizeof(WCHAR)) &&
        (RelativeName->Buffer[0] != L'\\'))
    {
        needsSeparator = TRUE;
        length += sizeof(WCHAR);
    }
    else
    {
        needsSeparator = FALSE;
    }

    if (length > UNICODE_STRING_MAX_BYTES)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      INFORMER,
                      "Object name too long: %lu",
                      length);

        status = STATUS_INTEGER_OVERFLOW;
        goto Exit;
    }

    objectName = KphAllocatePaged((length + sizeof(UNICODE_STRING)),
                                  KPH_TAG_REG_OBJECT_NAME);
    if (!objectName)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      INFORMER,
                      "Failed to allocate object name.");

        status = STATUS_INSUFFICIENT_RESOURCES;
        goto Exit;
    }

    objectName->Length = 0;
    objectName->MaximumLength = (USHORT)length;
    objectName->Buffer = Add2Ptr(objectName, sizeof(UNICODE_STRING));

    RtlAppendUnicodeStringToString(objectName, rootName);

    if (needsSeparator)
    {
        RtlAppendUnicodeToString(objectName, L"\\");
    }

    RtlAppendUnicodeStringToString(objectName, RelativeName);

    *ObjectName = objectName;
    objectName = NULL;

Exit:

    if (rootName)
    {
        CmCallbackReleaseKeyObjectIDEx(rootName);
    }

    if (objectName)
    {
        KphFree(objectName, KPH_TAG_REG_OBJECT_NAME);
    }

    return status;
}

/**
 * \brief Fills a message with information about a creation or open operation.
 *
 * \param[in,out] Message The message to populate.
 * \param[in] CreateInfo The creation or open information.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphpRegFillCreateKeyObjectInfo(
    _Inout_ PKPH_MESSAGE Message,
    _In_ PREG_CREATE_KEY_INFORMATION_V1 CreateInfo
    )
{
    NTSTATUS status;
    PUNICODE_STRING objectName;

    PAGED_CODE_PASSIVE();

    Message->Kernel.Reg.Transaction = CreateInfo->Transaction;

    if ((CreateInfo->CompleteName->Length >= sizeof(WCHAR)) &&
        (CreateInfo->CompleteName->Buffer[0] == L'\\'))
    {
        objectName = CreateInfo->CompleteName;
    }
    else
    {
        status = KphpRegMakeObjectName(CreateInfo->RootObject,
                                       CreateInfo->CompleteName,
                                       &objectName);
        if (!NT_SUCCESS(status))
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          INFORMER,
                          "KphpRegMakeObjectName failed: %!STATUS!",
                          status);

            goto Exit;
        }
    }

    status = KphMsgDynAddUnicodeString(Message,
                                       KphMsgFieldObjectName,
                                       objectName);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      INFORMER,
                      "KphMsgDynAddUnicodeString failed: %!STATUS!",
                      status);
    }

Exit:

    if (objectName && (objectName != CreateInfo->CompleteName))
    {
        KphFree(objectName, KPH_TAG_REG_OBJECT_NAME);
    }
}

/**
 * \brief Fills a message with information about a load key operation.
 *
 * \param[in,out] Message The message to populate.
 * \param[in] LoadInfo The load key information.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphpRegFillLoadKeyObjectInfo(
    _Inout_ PKPH_MESSAGE Message,
    _In_ PREG_LOAD_KEY_INFORMATION_V2 LoadInfo
    )
{
    NTSTATUS status;
    PUNICODE_STRING objectName;

    PAGED_CODE_PASSIVE();

    if (LoadInfo->Object)
    {
        status = KphpRegMakeObjectName(LoadInfo->Object,
                                       LoadInfo->KeyName,
                                       &objectName);
        if (!NT_SUCCESS(status))
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          INFORMER,
                          "KphpRegMakeObjectName failed: %!STATUS!",
                          status);

            goto Exit;
        }
    }
    else
    {
        objectName = LoadInfo->KeyName;
    }

    status = KphMsgDynAddUnicodeString(Message,
                                       KphMsgFieldObjectName,
                                       objectName);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      INFORMER,
                      "KphMsgDynAddUnicodeString failed: %!STATUS!",
                      status);
    }

Exit:

    if (objectName && (objectName != LoadInfo->KeyName))
    {
        KphFree(objectName, KPH_TAG_REG_OBJECT_NAME);
    }
}

/**
 * \brief Copies a Unicode string into a message
 *
 * \param[in] Message The message to populate.
 * \param[in] FieldId The field ID for the Unicode string.
 * \param[in] String The string to populate into the message.
 * \param[in] Default Default string to use input string length is zero.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphpRegCopyUnicodeStringWithDefault(
    _Inout_ PKPH_MESSAGE Message,
    _In_ KPH_MESSAGE_FIELD_ID FieldId,
    _In_opt_ PUNICODE_STRING String,
    _In_opt_ PCUNICODE_STRING Default
    )
{
    NTSTATUS status;
    UNICODE_STRING string;

    PAGED_CODE_PASSIVE();

    if (!String)
    {
        return;
    }

    __try
    {
        string.Buffer = String->Buffer;
        string.Length = String->Length;
        string.MaximumLength = string.Length;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      INFORMER,
                      "Exception accessing string: %!STATUS!",
                      GetExceptionCode());

        return;
    }

    if (!string.Length)
    {
        if (Default)
        {
            status = KphMsgDynAddUnicodeString(Message, FieldId, Default);
            if (!NT_SUCCESS(status))
            {
                KphTracePrint(TRACE_LEVEL_VERBOSE,
                              INFORMER,
                              "KphMsgDynAddUnicodeString failed: %!STATUS!",
                              status);
            }
        }

        return;
    }

    __try
    {
        status = KphMsgDynAddUnicodeString(Message, FieldId, &string);
        if (!NT_SUCCESS(status))
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          INFORMER,
                          "KphMsgDynAddUnicodeString failed: %!STATUS!",
                          status);
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      INFORMER,
                      "Exception copying string: %!STATUS!",
                      GetExceptionCode());

        //
        // N.B. The KphMsgDynAdd* routines will first reserve a table
        // entry then copy data into the buffer. If we get here it means
        // the control flow was arrested after the reservation. Since we
        // can't guarantee it was all copied successfully clear the last
        // entry that was reserved.
        //
        KphMsgDynClearLast(Message);
    }
}

/**
 * \brief Copies a Unicode string into a message.
 *
 * \param[in,out] Message The message to populate.
 * \param[in] FieldId The field ID for the Unicode string.
 * \param[in] String The string to populate into the message.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphpRegCopyUnicodeString(
    _Inout_ PKPH_MESSAGE Message,
    _In_ KPH_MESSAGE_FIELD_ID FieldId,
    _In_opt_ PUNICODE_STRING String
    )
{
    PAGED_CODE_PASSIVE();

    KphpRegCopyUnicodeStringWithDefault(Message, FieldId, String, NULL);
}

/**
 * \brief Copies a registry value name into a message.
 *
 * \param[in,out] Message The message to populate.
 * \param[in] ValueName The value name to populate into the message.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphpRegCopyValueName(
    _Inout_ PKPH_MESSAGE Message,
    _In_opt_ PUNICODE_STRING ValueName
    )
{
    PAGED_CODE_PASSIVE();

    KphpRegCopyUnicodeStringWithDefault(Message,
                                        KphMsgFieldValueName,
                                        ValueName,
                                        &KphpCmDefaultValueName);
}

/**
 * \brief Copies multiple value names into a message.
 *
 * \param[in,out] Message The message to populate.
 * \param[in] ValueEntries The value entries to populate into the message.
 * \param[in] EntryCount The number of value entries.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphpRegCopyMultipleValueNames(
    _Inout_ PKPH_MESSAGE Message,
    _In_opt_ PKEY_VALUE_ENTRY ValueEntries,
    _In_ ULONG EntryCount
    )
{
    NTSTATUS status;
    ULONG length;
    PUNICODE_STRING valueNames;
    ULONG remaining;

    PAGED_CODE_PASSIVE();

    if (!ValueEntries || !EntryCount)
    {
        return;
    }

    valueNames = NULL;

    //
    // Try to capture the values names as a sequence of null terminated strings.
    //

    length = sizeof(WCHAR);

    __try
    {
        for (ULONG i = 0; i < EntryCount; i++)
        {
            PCUNICODE_STRING entry;
            ULONG valueLength;

            entry = ValueEntries[i].ValueName;
            valueLength = entry->Length;

            if (valueLength)
            {
                valueLength += sizeof(WCHAR);
            }
            else
            {
                valueLength = (KphpCmDefaultValueName.Length + sizeof(WCHAR));
            }

            status = RtlULongAdd(length, valueLength, &length);
            if (!NT_SUCCESS(status))
            {
                KphTracePrint(TRACE_LEVEL_VERBOSE,
                              INFORMER,
                              "RtlULongAdd failed: %!STATUS!",
                              status);

                goto Exit;
            }
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      INFORMER,
                      "Exception accessing value entries: %!STATUS!",
                      GetExceptionCode());

        goto Exit;
    }

    if (length > UNICODE_STRING_MAX_BYTES)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      INFORMER,
                      "Value names too long: %lu",
                      length);

        goto Exit;
    }

    valueNames = KphAllocatePaged((length + sizeof(UNICODE_STRING)),
                                   KPH_TAG_REG_VALUE_NAMES);
    if (!valueNames)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      INFORMER,
                      "Failed to allocate value names.");

        goto Exit;
    }

    valueNames->Length = 0;
    valueNames->MaximumLength = (USHORT)length;
    valueNames->Buffer = Add2Ptr(valueNames, sizeof(UNICODE_STRING));

    __try
    {
        for (ULONG i = 0; i < EntryCount; i++)
        {
            PCUNICODE_STRING entry;

            entry = ValueEntries[i].ValueName;

            if (!entry->Length)
            {
                entry = &KphpCmDefaultValueName;
            }

            status = RtlAppendUnicodeStringToString(valueNames, entry);
            if (!NT_SUCCESS(status))
            {
                KphTracePrint(TRACE_LEVEL_VERBOSE,
                              INFORMER,
                              "RtlAppendUnicodeStringToString failed: %!STATUS!",
                              status);

                goto Exit;
            }

            remaining = (valueNames->MaximumLength - valueNames->Length);

            if (remaining < sizeof(WCHAR))
            {
                KphTracePrint(TRACE_LEVEL_VERBOSE,
                              INFORMER,
                              "Exhausted value names buffer.");

                goto Exit;
            }

            valueNames->Buffer[valueNames->Length / sizeof(WCHAR)] = L'\0';
            valueNames->Length += sizeof(WCHAR);
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      INFORMER,
                      "Exception accessing value entries: %!STATUS!",
                      GetExceptionCode());

        goto Exit;
    }

    remaining = (valueNames->MaximumLength - valueNames->Length);
    if (remaining < sizeof(WCHAR))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      INFORMER,
                      "Exhausted value names buffer.");

        goto Exit;
    }

    valueNames->Buffer[valueNames->Length / sizeof(WCHAR)] = L'\0';
    valueNames->Length += sizeof(WCHAR);

    status = KphMsgDynAddUnicodeString(Message,
                                       KphMsgFieldMultipleValueNames,
                                       valueNames);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      INFORMER,
                      "KphMsgDynAddUnicodeString failed: %!STATUS!",
                      status);
    }

Exit:

    if (valueNames)
    {
        KphFree(valueNames, KPH_TAG_REG_VALUE_NAMES);
    }
}

/**
 * \brief Copies a buffer into a message.
 *
 * \param[in,out] Message The message to populate.
 * \param[in] FieldId The field ID for the buffer.
 * \param[in] Buffer The buffer to copy.
 * \param[in] Length The length of the buffer.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphpRegCopyBuffer(
    _Inout_ PKPH_MESSAGE Message,
    _In_ KPH_MESSAGE_FIELD_ID FieldId,
    _In_opt_ PVOID Buffer,
    _In_ ULONG Length
    )
{
    NTSTATUS status;
    USHORT remaining;
    KPHM_SIZED_BUFFER sizedBuffer;

    PAGED_CODE_PASSIVE();

    if (!Buffer)
    {
        return;
    }

    remaining = KphMsgDynRemaining(Message);
    if (remaining < Length)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      INFORMER,
                      "Buffer too large for message, truncating %lu -> %lu",
                      Length,
                      remaining);

        Length = remaining;
    }

    if (!Length)
    {
        return;
    }

    sizedBuffer.Buffer = Buffer;
    sizedBuffer.Size = (USHORT)Length;

    __try
    {
        status = KphMsgDynAddSizedBuffer(Message, FieldId, &sizedBuffer);
        if (!NT_SUCCESS(status))
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          INFORMER,
                          "KphMsgDynAddSizedBuffer failed: %!STATUS!",
                          status);
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      INFORMER,
                      "Exception copying buffer: %!STATUS!",
                      GetExceptionCode());

        //
        // N.B. The KphMsgDynAdd* routines will first reserve a table
        // entry then copy data into the buffer. If we get here it means
        // the control flow was arrested after the reservation. Since we
        // can't guarantee it was all copied successfully clear the last
        // entry that was reserved.
        //
        KphMsgDynClearLast(Message);
    }
}

/**
 * \brief Copies an object name into a message.
 *
 * \param[in,out] Message The message to populate.
 * \param[in] FieldId The field ID for the object name.
 * \param[in] Object The object to copy the name from.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphpRegCopyObjectName(
    _Inout_ PKPH_MESSAGE Message,
    _In_ KPH_MESSAGE_FIELD_ID FieldId,
    _In_opt_ PVOID Object
    )
{
    NTSTATUS status;
    PUNICODE_STRING objectName;
    PUNICODE_STRING keyName;
    POBJECT_NAME_INFORMATION nameInfo;
    ULONG length;

    PAGED_CODE_PASSIVE();

    if (!Object)
    {
        return;
    }

    keyName = NULL;
    nameInfo = NULL;

    if (ObGetObjectType(Object) == *CmKeyObjectType)
    {
        status = CmCallbackGetKeyObjectIDEx(&KphpCmCookie,
                                            Object,
                                            NULL,
                                            &keyName,
                                            0);
        if (!NT_SUCCESS(status))
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          INFORMER,
                          "CmCallbackGetKeyObjectIDEx failed: %!STATUS!",
                          status);

            keyName = NULL;
            goto Exit;
        }

        objectName = keyName;
    }
    else
    {
        //
        // TODO(jxy-s) improve KphQueryObjectName
        //

        length = ((MAX_PATH * 2) + sizeof(OBJECT_NAME_INFORMATION));

        nameInfo = KphAllocatePaged(length, KPH_TAG_REG_OBJECT_NAME);
        if (!nameInfo)
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          INFORMER,
                          "Failed to allocate object name.");

            goto Exit;
        }

        status = KphQueryNameObject(Object, nameInfo, length, &length);
        if (!NT_SUCCESS(status))
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          INFORMER,
                          "KphQueryNameObject failed: %!STATUS!",
                          status);

            goto Exit;
        }

        objectName = &nameInfo->Name;
    }

    status = KphMsgDynAddUnicodeString(Message, FieldId, objectName);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      INFORMER,
                      "KphMsgDynAddUnicodeString failed: %!STATUS!",
                      status);
    }

Exit:

    if (nameInfo)
    {
        KphFree(nameInfo, KPH_TAG_REG_OBJECT_NAME);
    }

    if (keyName)
    {
        CmCallbackReleaseKeyObjectIDEx(keyName);
    }
}

/**
 * \brief Copies a handle name into a message.
 *
 * \param[in,out] Message The message to populate.
 * \param[in] FieldId The field ID for the handle name.
 * \param[in] Handle The handle to copy the name from.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphpRegCopyHandleName(
    _Inout_ PKPH_MESSAGE Message,
    _In_ KPH_MESSAGE_FIELD_ID FieldId,
    _In_ HANDLE Handle
    )
{
    NTSTATUS status;
    KAPC_STATE apcState;
    PVOID object;

    PAGED_CODE_PASSIVE();

    if (!Handle)
    {
        return;
    }

    if (IsKernelHandle(Handle) && !IsPseudoHandle(Handle))
    {
        KeStackAttachProcess(PsInitialSystemProcess, &apcState);
    }

    NT_ASSERT(Handle);
#pragma prefast(suppress : 6387)
    status = ObReferenceObjectByHandle(Handle,
                                       0,
                                       NULL,
                                       KernelMode,
                                       &object,
                                       NULL);

    if (IsKernelHandle(Handle) && !IsPseudoHandle(Handle))
    {
        KeUnstackDetachProcess(&apcState);
    }

    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      INFORMER,
                      "ObReferenceObjectByHandle failed: %!STATUS!",
                      status);

        return;
    }

    KphpRegCopyObjectName(Message, FieldId, object);

    ObDereferenceObject(object);
}

/**
 * \brief Fills a message with post operation information.
 *
 * \param[in,out] Message The message to populate.
 * \param[in] RegClass The registry operation class.
 * \param[in] PostInfo The post operation information.
 * \param[in] Context The call context.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphpRegFillPostOpMessage(
    _Inout_ PKPH_MESSAGE Message,
    _In_ REG_NOTIFY_CLASS RegClass,
    _In_ PKPH_REG_POST_INFORMATION PostInfo,
    _In_ PKPH_REG_CALL_CONTEXT Context
    )
{
    NTSTATUS status;
    PKPH_REG_PRE_INFORMATION preInfo;

    PAGED_CODE_PASSIVE();

    Message->Kernel.Reg.PostOperation = TRUE;

    Message->Kernel.Reg.Post.PreSequence = Context->PreSequence;
    Message->Kernel.Reg.Post.PreTimeStamp = Context->PreTimeStamp;

    preInfo = PostInfo->Common.PreInformation;
    Message->Kernel.Reg.Post.Status = PostInfo->Common.Status;

    KphpRegFillCommonMessage(Message, RegClass, preInfo);

#define KPH_REG_COPY_OUT_PARAM(n, v)                                          \
    if (preInfo->##n.##v)                                                     \
    {                                                                         \
        __try                                                                 \
        {                                                                     \
            Message->Kernel.Reg.Post.##n.##v = *preInfo->##n.##v;             \
        }                                                                     \
        __except (EXCEPTION_EXECUTE_HANDLER)                                  \
        {                                                                     \
            Message->Kernel.Reg.Post.##n.##v = 0;                             \
        }                                                                     \
    }

    switch (RegClass)
    {
        case RegNtPostSetValueKey:
        {
            KphpRegFillObjectInfo(Message, PostInfo->Object);
            KphpRegCopyValueName(Message, preInfo->SetValueKey.ValueName);
            break;
        }
        case RegNtPostDeleteValueKey:
        {
            KphpRegFillObjectInfo(Message, PostInfo->Object);
            KphpRegCopyValueName(Message, preInfo->DeleteValueKey.ValueName);
            break;
        }
        case RegNtPostEnumerateKey:
        {
            KphpRegFillObjectInfo(Message, PostInfo->Object);

            if (!NT_SUCCESS(Message->Kernel.Reg.Post.Status))
            {
                break;
            }

            KPH_REG_COPY_OUT_PARAM(EnumerateKey, ResultLength);
            KphpRegCopyBuffer(Message,
                              KphMsgFieldInformationBuffer,
                              preInfo->EnumerateKey.KeyInformation,
                              Message->Kernel.Reg.Post.EnumerateKey.ResultLength);
            break;
        }
        case RegNtPostEnumerateValueKey:
        {
            KphpRegFillObjectInfo(Message, PostInfo->Object);

            if (!NT_SUCCESS(PostInfo->Common.Status))
            {
                break;
            }

            KPH_REG_COPY_OUT_PARAM(EnumerateValueKey, ResultLength);
            if (Context->Options.EnableValueBuffers)
            {
                KphpRegCopyBuffer(Message,
                                  KphMsgFieldValueBuffer,
                                  preInfo->EnumerateValueKey.KeyValueInformation,
                                  Message->Kernel.Reg.Post.EnumerateValueKey.ResultLength);
            }
            break;
        }
        case RegNtPostQueryKey:
        {
            KphpRegFillObjectInfo(Message, PostInfo->Object);

            if (!NT_SUCCESS(PostInfo->Common.Status))
            {
                break;
            }

            KPH_REG_COPY_OUT_PARAM(QueryKey, ResultLength);
            KphpRegCopyBuffer(Message,
                              KphMsgFieldInformationBuffer,
                              preInfo->QueryKey.KeyInformation,
                              Message->Kernel.Reg.Post.QueryKey.ResultLength);
            break;
        }
        case RegNtPostQueryValueKey:
        {
            KphpRegFillObjectInfo(Message, PostInfo->Object);
            KphpRegCopyValueName(Message, preInfo->QueryValueKey.ValueName);

            if (!NT_SUCCESS(PostInfo->Common.Status))
            {
                break;
            }

            KPH_REG_COPY_OUT_PARAM(QueryValueKey, ResultLength);
            if (Context->Options.EnableValueBuffers)
            {
                KphpRegCopyBuffer(Message,
                                  KphMsgFieldValueBuffer,
                                  preInfo->QueryValueKey.KeyValueInformation,
                                  Message->Kernel.Reg.Post.QueryValueKey.ResultLength);
            }
            break;
        }
        case RegNtPostQueryMultipleValueKey:
        {
            KphpRegFillObjectInfo(Message, PostInfo->Object);
            KphpRegCopyMultipleValueNames(Message,
                                          preInfo->QueryMultipleValueKey.ValueEntries,
                                          preInfo->QueryMultipleValueKey.EntryCount);
            KPH_REG_COPY_OUT_PARAM(QueryMultipleValueKey, RequiredBufferLength);

            if (!NT_SUCCESS(PostInfo->Common.Status))
            {
                break;
            }

            KPH_REG_COPY_OUT_PARAM(QueryMultipleValueKey, BufferLength);
            if (Context->Options.EnableValueBuffers)
            {
                ULONG entriesLength;

                if (!NT_SUCCESS(RtlULongMult(preInfo->QueryMultipleValueKey.EntryCount,
                                             sizeof(KEY_VALUE_ENTRY),
                                             &entriesLength)))
                {
                    break;
                }

                KphpRegCopyBuffer(Message,
                                  KphMsgFieldMultipleValueEntries,
                                  preInfo->QueryMultipleValueKey.ValueEntries,
                                  entriesLength);

                KphpRegCopyBuffer(Message,
                                  KphMsgFieldValueBuffer,
                                  preInfo->QueryMultipleValueKey.ValueBuffer,
                                  Message->Kernel.Reg.Post.QueryMultipleValueKey.BufferLength);
            }
            break;
        }
        case RegNtPostKeyHandleClose:
        {
            Message->Kernel.Reg.Object = preInfo->Object;
            Message->Kernel.Reg.ObjectId = Context->ObjectId;
            Message->Kernel.Reg.Transaction = Context->Transaction;

            if (Context->ObjectName)
            {
                status = KphMsgDynAddUnicodeString(Message,
                                                   KphMsgFieldObjectName,
                                                   Context->ObjectName);
                if (!NT_SUCCESS(status))
                {
                    KphTracePrint(TRACE_LEVEL_VERBOSE,
                                  INFORMER,
                                  "KphMsgDynAddUnicodeString failed: %!STATUS!",
                                  status);
                }
            }
            break;
        }
        case RegNtPostCreateKeyEx:
        {
            if (PostInfo->Common.Status == STATUS_SUCCESS)
            {
                KphpRegFillObjectInfo(Message, PostInfo->Common.Object);
            }
            else
            {
                KphpRegFillCreateKeyObjectInfo(Message, &preInfo->CreateKey);
            }

            if (NT_SUCCESS(PostInfo->Common.Status))
            {
                KPH_REG_COPY_OUT_PARAM(CreateKey, Disposition);
            }
            break;
        }
        case RegNtPostOpenKeyEx:
        {
            if (PostInfo->Common.Status == STATUS_SUCCESS)
            {
                KphpRegFillObjectInfo(Message, PostInfo->Common.Object);
            }
            else
            {
                KphpRegFillCreateKeyObjectInfo(Message, &preInfo->OpenKey);
            }

            if (NT_SUCCESS(PostInfo->Common.Status))
            {
                KPH_REG_COPY_OUT_PARAM(OpenKey, Disposition);
            }
            break;
        }
        case RegNtPostLoadKey:
        {
            KphpRegFillLoadKeyObjectInfo(Message, &preInfo->LoadKey);

            if (NT_SUCCESS(PostInfo->Common.Status))
            {
                KPH_REG_COPY_OUT_PARAM(LoadKey, RootHandle);
            }
            break;
        }
        case RegNtPostQueryKeySecurity:
        {
            KphpRegFillObjectInfo(Message, PostInfo->Object);

            if (NT_SUCCESS(PostInfo->Common.Status))
            {
                KPH_REG_COPY_OUT_PARAM(QueryKeySecurity, Length);
            }
            break;
        }
        case RegNtPostReplaceKey:
        {
            KphpRegFillObjectInfo(Message, PostInfo->Object);
            KphpRegCopyUnicodeString(Message,
                                     KphMsgFieldFileName,
                                     preInfo->ReplaceKey.NewFileName);
            KphpRegCopyUnicodeString(Message,
                                     KphMsgFieldDestinationFileName,
                                     preInfo->ReplaceKey.OldFileName);
            break;
        }
        case RegNtPostQueryKeyName:
        {
            KphpRegFillObjectInfo(Message, PostInfo->Object);

            if (NT_SUCCESS(PostInfo->Common.Status))
            {
                KPH_REG_COPY_OUT_PARAM(QueryKeyName, ReturnLength);
            }
            break;
        }
        case RegNtPostSaveMergedKey:
        {
            KphpRegFillObjectInfo(Message, preInfo->SaveMergedKey.HighKeyObject);
            KphpRegCopyObjectInfo(Message,
                                  KphMsgFieldOtherObjectName,
                                  preInfo->SaveMergedKey.LowKeyObject,
                                  NULL,
                                  &Message->Kernel.Reg.Post.SaveMergedKey.LowKeyObjectId,
                                  &Message->Kernel.Reg.Post.SaveMergedKey.LowKeyTransaction);
            KphpRegCopyHandleName(Message,
                                  KphMsgFieldFileName,
                                  preInfo->SaveMergedKey.FileHandle);
            break;
        }
        default:
        {
            KphpRegFillObjectInfo(Message, PostInfo->Object);
            break;
        }
    }
}

/**
 * \brief Fills a message with pre operation information.
 *
 * \param[in,out] Message The message to populate.
 * \param[in] RegClass The registry operation class.
 * \param[in] PreInfo The pre operation information.
 * \param[in] Options Registry options to use.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphpRegFillPreOpMessage(
    _Inout_ PKPH_MESSAGE Message,
    _In_ REG_NOTIFY_CLASS RegClass,
    _In_ PKPH_REG_PRE_INFORMATION PreInfo,
    _In_ PKPH_REG_OPTIONS Options
    )
{
    PAGED_CODE_PASSIVE();

    Message->Kernel.Reg.PostOperation = FALSE;

    KphpRegFillCommonMessage(Message, RegClass, PreInfo);

#define KPH_REG_COPY_IN_PARAM(n, v)                                           \
    if (PreInfo->##n.##v)                                                     \
    {                                                                         \
        __try                                                                 \
        {                                                                     \
            Message->Kernel.Reg.Pre.##n.##v = *PreInfo->##n.##v;              \
        }                                                                     \
        __except (EXCEPTION_EXECUTE_HANDLER)                                  \
        {                                                                     \
            Message->Kernel.Reg.Pre.##n.##v = 0;                              \
        }                                                                     \
    }

    switch (RegClass)
    {
        case RegNtPreSetValueKey:
        {
            KphpRegFillObjectInfo(Message, PreInfo->Object);
            KphpRegCopyValueName(Message, PreInfo->SetValueKey.ValueName);

            if (Options->EnableValueBuffers)
            {
                KphpRegCopyBuffer(Message,
                                  KphMsgFieldValueBuffer,
                                  PreInfo->SetValueKey.Data,
                                  PreInfo->SetValueKey.DataSize);
            }
            break;
        }
        case RegNtPreDeleteValueKey:
        {
            KphpRegFillObjectInfo(Message, PreInfo->Object);
            KphpRegCopyValueName(Message, PreInfo->DeleteValueKey.ValueName);
            break;
        }
        case RegNtPreSetInformationKey:
        {
            KphpRegFillObjectInfo(Message, PreInfo->Object);
            KphpRegCopyBuffer(Message,
                              KphMsgFieldInformationBuffer,
                              PreInfo->SetInformationKey.KeySetInformation,
                              PreInfo->SetInformationKey.KeySetInformationLength);
            break;
        }
        case RegNtPreRenameKey:
        {
            KphpRegFillObjectInfo(Message, PreInfo->Object);
            KphpRegCopyUnicodeString(Message,
                                     KphMsgFieldNewName,
                                     PreInfo->RenameKey.NewName);
            break;
        }
        case RegNtPreQueryValueKey:
        {
            KphpRegFillObjectInfo(Message, PreInfo->Object);
            KphpRegCopyValueName(Message, PreInfo->QueryValueKey.ValueName);
            break;
        }
        case RegNtPreQueryMultipleValueKey:
        {
            KphpRegFillObjectInfo(Message, PreInfo->Object);
            KphpRegCopyMultipleValueNames(Message,
                                          PreInfo->QueryMultipleValueKey.ValueEntries,
                                          PreInfo->QueryMultipleValueKey.EntryCount);
            KPH_REG_COPY_IN_PARAM(QueryMultipleValueKey, BufferLength);
            break;
        }
        case RegNtPreCreateKeyEx:
        {
            KphpRegFillCreateKeyObjectInfo(Message, &PreInfo->CreateKey);
            KphpRegCopyUnicodeString(Message,
                                     KphMsgFieldClass,
                                     PreInfo->CreateKey.Class);
            break;
        }
        case RegNtPreOpenKeyEx:
        {
            KphpRegFillCreateKeyObjectInfo(Message, &PreInfo->OpenKey);
            KphpRegCopyUnicodeString(Message,
                                     KphMsgFieldClass,
                                     PreInfo->OpenKey.Class);
            break;
        }
        case RegNtPreLoadKey:
        {
            KphpRegFillLoadKeyObjectInfo(Message, &PreInfo->LoadKey);
            KphpRegCopyUnicodeString(Message,
                                     KphMsgFieldFileName,
                                     PreInfo->LoadKey.SourceFile);
            break;
        }
        case RegNtPreQueryKeySecurity:
        {
            KphpRegFillObjectInfo(Message, PreInfo->Object);
            KPH_REG_COPY_IN_PARAM(QueryKeySecurity, SecurityInformation);
            KPH_REG_COPY_IN_PARAM(QueryKeySecurity, Length);
            break;
        }
        case RegNtPreSetKeySecurity:
        {
            KphpRegFillObjectInfo(Message, PreInfo->Object);
            KPH_REG_COPY_IN_PARAM(SetKeySecurity, SecurityInformation);
            break;
        }
        case RegNtPreRestoreKey:
        {
            KphpRegFillObjectInfo(Message, PreInfo->Object);
            KphpRegCopyHandleName(Message,
                                  KphMsgFieldFileName,
                                  PreInfo->RestoreKey.FileHandle);
            break;
        }
        case RegNtPreSaveKey:
        {
            KphpRegFillObjectInfo(Message, PreInfo->Object);
            KphpRegCopyHandleName(Message,
                                  KphMsgFieldFileName,
                                  PreInfo->SaveKey.FileHandle);
            break;
        }
        case RegNtPreReplaceKey:
        {
            KphpRegFillObjectInfo(Message, PreInfo->Object);
            KphpRegCopyUnicodeString(Message,
                                     KphMsgFieldFileName,
                                     PreInfo->ReplaceKey.NewFileName);
            KphpRegCopyUnicodeString(Message,
                                     KphMsgFieldDestinationFileName,
                                     PreInfo->ReplaceKey.OldFileName);
            break;
        }
        case RegNtPreSaveMergedKey:
        {
            KphpRegFillObjectInfo(Message, PreInfo->SaveMergedKey.HighKeyObject);
            KphpRegCopyObjectInfo(Message,
                                  KphMsgFieldOtherObjectName,
                                  PreInfo->SaveMergedKey.LowKeyObject,
                                  NULL,
                                  &Message->Kernel.Reg.Pre.SaveMergedKey.LowKeyObjectId,
                                  &Message->Kernel.Reg.Pre.SaveMergedKey.LowKeyTransaction);
            KphpRegCopyHandleName(Message,
                                  KphMsgFieldFileName,
                                  PreInfo->SaveMergedKey.FileHandle);
            break;
        }
        default:
        {
            KphpRegFillObjectInfo(Message, PreInfo->Object);
            break;
        }
    }
}

/**
 * \brief Sends a post operation message.
 *
 * \param[in] RegClass The registry operation class.
 * \param[in] PostInfo The post operation information.
 * \param[in] Sequence The registry sequence number.
 * \param[in] Context The call context.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphpRegPostOpSend(
    _In_ REG_NOTIFY_CLASS RegClass,
    _In_ PKPH_REG_POST_INFORMATION PostInfo,
    _In_ ULONG64 Sequence,
    _In_ PKPH_REG_CALL_CONTEXT Context
    )
{
    PKPH_MESSAGE msg;

    PAGED_CODE_PASSIVE();

    msg = KphAllocateMessage();
    if (!msg)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      INFORMER,
                      "KphAllocateMessage failed");

        return;
    }

    KphMsgInit(msg, KphpRegGetMessageId(RegClass));

    msg->Kernel.Reg.Sequence = Sequence;

    KphpRegFillPostOpMessage(msg, RegClass, PostInfo, Context);

    if (Context->Options.EnableStackTraces)
    {
        KphCaptureStackInMessage(msg);
    }

    KphCommsSendMessageAsync(msg);
}

_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphpRegFreeCallContext(
    _In_ PKPH_REG_CALL_CONTEXT Context
    )
{
    PAGED_CODE_PASSIVE();

    if (Context->ObjectName)
    {
        CmCallbackReleaseKeyObjectIDEx(Context->ObjectName);
    }

    KphFreeToPagedLookaside(&KphpCmCallContextLookaside, Context);
}

/**
 * \brief Registry post operation callback.
 *
 * \param[in] RegClass The registry operation class.
 * \param[in] PostInfo The post operation information.
 * \param[in] Sequence The registry sequence number for the post operation.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphpRegPostOp(
    _In_ REG_NOTIFY_CLASS RegClass,
    _In_ PKPH_REG_POST_INFORMATION PostInfo,
    _In_ ULONG64 Sequence
    )
{
    PKPH_REG_CALL_CONTEXT context;

    PAGED_CODE_PASSIVE();

    context = PostInfo->Common.CallContext;

    if (context)
    {
        KphpRegPostOpSend(RegClass, PostInfo, Sequence, context);

        KphpRegFreeCallContext(context);
    }
}

/**
 * \brief Sets the call context for a registry operation.
 *
 * \details This routine sets the call context in the pre operation information
 * in preparation for the post operation to handle it.
 *
 * \param[in] RegClass The registry operation class.
 * \param[in] PreInfo The pre operation information.
 * \param[in] Options Registry options to use.
 * \param[in] Sequence The registry sequence number for the pre operation.
 * \param[in] TimeStamp The time stamp for the pre operation.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphpRegPreOpSetCallContext(
    _In_ REG_NOTIFY_CLASS RegClass,
    _In_ PKPH_REG_PRE_INFORMATION PreInfo,
    _In_ PKPH_REG_OPTIONS Options,
    _In_ ULONG64 Sequence,
    _In_ PLARGE_INTEGER TimeStamp
    )
{
    NTSTATUS status;
    PKPH_REG_CALL_CONTEXT context;

    PAGED_CODE_PASSIVE();

    context = KphAllocateFromPagedLookaside(&KphpCmCallContextLookaside);
    if (!context)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      INFORMER,
                      "KphAllocateFromPagedLookaside failed");

        status = STATUS_INSUFFICIENT_RESOURCES;
        goto Exit;
    }

    context->PreSequence = Sequence;
    context->PreTimeStamp = *TimeStamp;
    context->Options.Flags = Options->Flags;

    if (RegClass == RegNtPreKeyHandleClose)
    {
        PVOID object;

        object = PreInfo->KeyHandleClose.Object;

        context->Transaction = CmGetBoundTransaction(&KphpCmCookie, object);

        status = CmCallbackGetKeyObjectIDEx(&KphpCmCookie,
                                            object,
                                            &context->ObjectId,
                                            &context->ObjectName,
                                            0);
        if (!NT_SUCCESS(status))
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          INFORMER,
                          "CmCallbackGetKeyObjectIDEx failed: %!STATUS!",
                          status);

            context->ObjectId = 0;
            context->ObjectName = NULL;
        }
    }

    status = STATUS_SUCCESS;

#define KPH_REG_SET_CALL_CONTEXT2(reg, name)                                  \
    case RegNtPre##reg:                                                       \
    {                                                                         \
        NT_ASSERT(!PreInfo->##name.CallContext);                              \
        PreInfo->##name.CallContext = context;                                \
        context = NULL;                                                       \
        break;                                                                \
    }
#define KPH_REG_SET_CALL_CONTEXT(name) KPH_REG_SET_CALL_CONTEXT2(name, name)

    switch (RegClass)
    {
        KPH_REG_SET_CALL_CONTEXT(DeleteKey)
        KPH_REG_SET_CALL_CONTEXT(SetValueKey)
        KPH_REG_SET_CALL_CONTEXT(DeleteValueKey)
        KPH_REG_SET_CALL_CONTEXT(SetInformationKey)
        KPH_REG_SET_CALL_CONTEXT(RenameKey)
        KPH_REG_SET_CALL_CONTEXT(EnumerateKey)
        KPH_REG_SET_CALL_CONTEXT(EnumerateValueKey)
        KPH_REG_SET_CALL_CONTEXT(QueryKey)
        KPH_REG_SET_CALL_CONTEXT(QueryValueKey)
        KPH_REG_SET_CALL_CONTEXT(QueryMultipleValueKey)
        KPH_REG_SET_CALL_CONTEXT(KeyHandleClose)
        KPH_REG_SET_CALL_CONTEXT2(CreateKeyEx, CreateKey)
        KPH_REG_SET_CALL_CONTEXT2(OpenKeyEx, OpenKey)
        KPH_REG_SET_CALL_CONTEXT(FlushKey)
        KPH_REG_SET_CALL_CONTEXT(LoadKey)
        KPH_REG_SET_CALL_CONTEXT(UnLoadKey)
        KPH_REG_SET_CALL_CONTEXT(QueryKeySecurity)
        KPH_REG_SET_CALL_CONTEXT(SetKeySecurity)
        KPH_REG_SET_CALL_CONTEXT(RestoreKey)
        KPH_REG_SET_CALL_CONTEXT(SaveKey)
        KPH_REG_SET_CALL_CONTEXT(ReplaceKey)
        KPH_REG_SET_CALL_CONTEXT(QueryKeyName)
        KPH_REG_SET_CALL_CONTEXT(SaveMergedKey)
        DEFAULT_UNREACHABLE;
    }

Exit:

    if (context)
    {
        KphFreeToPagedLookaside(&KphpCmCallContextLookaside, context);
    }

    return status;
}

/**
 * \brief Sends a pre operation message.
 *
 * \param[in] RegClass The registry operation class.
 * \param[in] PreInfo The pre operation information.
 * \param[in] Options Registry options to use.
 * \param[in] Sequence The registry sequence number for the pre operation.
 * \param[out] TimeStamp Receives time stamp for the pre operation.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphpRegPreOpSend(
    _In_ REG_NOTIFY_CLASS RegClass,
    _In_ PKPH_REG_PRE_INFORMATION PreInfo,
    _In_ PKPH_REG_OPTIONS Options,
    _In_ ULONG64 Sequence,
    _Out_ PLARGE_INTEGER TimeStamp
    )
{
    PKPH_MESSAGE msg;

    PAGED_CODE_PASSIVE();

    msg = KphAllocateMessage();
    if (!msg)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      INFORMER,
                      "KphAllocateMessage failed");

        KeQuerySystemTime(TimeStamp);
        return;
    }

    KphMsgInit(msg, KphpRegGetMessageId(RegClass));

    *TimeStamp = msg->Header.TimeStamp;

    msg->Kernel.Reg.Sequence = Sequence;

    KphpRegFillPreOpMessage(msg, RegClass, PreInfo, Options);

    if (Options->EnableStackTraces)
    {
        KphCaptureStackInMessage(msg);
    }

    KphCommsSendMessageAsync(msg);
}

/**
 * \brief Registry pre operation callback.
 *
 * \param[in] RegClass The registry operation class.
 * \param[in] PreInfo The pre operation information.
 * \param[in] Options Registry options to use.
 * \param[in] Sequence The registry sequence number for the pre operation.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphpRegPreOp(
    _In_ REG_NOTIFY_CLASS RegClass,
    _In_ PKPH_REG_PRE_INFORMATION PreInfo,
    _In_ PKPH_REG_OPTIONS Options,
    _In_ ULONG64 Sequence
    )
{
    NTSTATUS status;
    LARGE_INTEGER timeStamp;

    PAGED_CODE_PASSIVE();

    NT_ASSERT(!Options->InPost);

    if (Options->PreEnabled)
    {
        KphpRegPreOpSend(RegClass, PreInfo, Options, Sequence, &timeStamp);
    }
    else
    {
        //
        // Pre operations aren't enabled, but we need the time stamp for the
        // post operation to use.
        //
        KeQuerySystemTime(&timeStamp);
    }

    if (Options->PostEnabled)
    {
        status = KphpRegPreOpSetCallContext(RegClass,
                                            PreInfo,
                                            Options,
                                            Sequence,
                                            &timeStamp);
        if (!NT_SUCCESS(status))
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          INFORMER,
                          "KphpRegPreOpSetCallContext failed: %!STATUS!",
                          status);
        }
    }
}

/**
 * \brief System registry callback, registered with the configuration manager.
 *
 * \param[in] CallbackContext Unused.
 * \param[in] Argument1 The registry operation class.
 * \param[in] Argument2 The registry operation information.
 *
 * \return STATUS_SUCCESS
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_IRQL_requires_same_
_Function_class_(EX_CALLBACK_FUNCTION)
NTSTATUS KphpRegistryCallback(
    _In_ PVOID CallbackContext,
    _In_opt_ PVOID Argument1,
    _In_opt_ PVOID Argument2
    )
{
    REG_NOTIFY_CLASS regClass;
    ULONG64 sequence;
    KPH_REG_OPTIONS options;

    PAGED_CODE_PASSIVE();

    UNREFERENCED_PARAMETER(CallbackContext);

    NT_ASSERT(Argument2);

    regClass = (REG_NOTIFY_CLASS)(ULONG_PTR)Argument1;

    if (regClass == RegNtCallbackObjectContextCleanup)
    {
        return STATUS_SUCCESS;
    }

    sequence = InterlockedIncrementU64(&KphpCmSequence);

    options = KphpRegGetOptions(regClass);

    if (options.InPost)
    {
        KphpRegPostOp(regClass, Argument2, sequence);
    }
    else
    {
        KphpRegPreOp(regClass, Argument2, &options, sequence);
    }

    return STATUS_SUCCESS;
}

/**
 * \brief Starts the registry informer.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphRegistryInformerStart(
    VOID
    )
{
    NTSTATUS status;

    PAGED_CODE_PASSIVE();

    KphInitializePagedLookaside(&KphpCmCallContextLookaside,
                                sizeof(KPH_REG_CALL_CONTEXT),
                                KPH_TAG_REG_CALL_CONTEXT);

    status = CmRegisterCallbackEx(KphpRegistryCallback,
                                  KphAltitude,
                                  KphDriverObject,
                                  NULL,
                                  &KphpCmCookie,
                                  NULL);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      INFORMER,
                      "CmRegisterCallbackEx failed: %!STATUS!",
                      status);

        KphDeletePagedLookaside(&KphpCmCallContextLookaside);

        return status;
    }

    KphpCmRegistered = TRUE;

    return STATUS_SUCCESS;
}

/**
 * \brief Stops the registry informer.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphRegistryInformerStop(
    VOID
    )
{
    NTSTATUS status;

    PAGED_CODE_PASSIVE();

    if (!KphpCmRegistered)
    {
        return;
    }

    status = CmUnRegisterCallback(KphpCmCookie);

    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      INFORMER,
                      "CmUnRegisterCallback failed: %!STATUS!",
                      status);
    }

    KphDeletePagedLookaside(&KphpCmCallContextLookaside);
}
