/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2009-2016
 *     dmex    2017-2023
 *
 */

#include <ph.h>

static PH_INITONCE PhPredefineKeyInitOnce = PH_INITONCE_INIT;
static UNICODE_STRING PhPredefineKeyNames[PH_KEY_MAXIMUM_PREDEFINE] =
{
    RTL_CONSTANT_STRING(L"\\Registry\\Machine"),
    RTL_CONSTANT_STRING(L"\\Registry\\User"),
    RTL_CONSTANT_STRING(L"\\Registry\\Machine\\Software\\Classes"),
    { 0, 0, NULL }
};
static HANDLE PhPredefineKeyHandles[PH_KEY_MAXIMUM_PREDEFINE] = { 0 };

/**
 * Initializes usage of predefined keys.
 */
VOID PhpInitializePredefineKeys(
    VOID
    )
{
    static UNICODE_STRING currentUserPrefix = RTL_CONSTANT_STRING(L"\\Registry\\User\\");
    NTSTATUS status;
    PH_TOKEN_USER tokenUser;
    UNICODE_STRING stringSid;
    WCHAR stringSidBuffer[SECURITY_MAX_SID_STRING_CHARACTERS];
    PUNICODE_STRING currentUserKeyName;

    // Get the string SID of the current user.

    if (NT_SUCCESS(status = PhGetTokenUser(PhGetOwnTokenAttributes().TokenHandle, &tokenUser)))
    {
        RtlInitEmptyUnicodeString(&stringSid, stringSidBuffer, sizeof(stringSidBuffer));

        if (PhEqualSid(tokenUser.User.Sid, (PSID)&PhSeLocalSystemSid))
        {
            status = RtlInitUnicodeStringEx(&stringSid, L".DEFAULT");
        }
        else
        {
            status = RtlConvertSidToUnicodeString(&stringSid, tokenUser.User.Sid, FALSE);
        }
    }

    // Construct the current user key name.

    if (NT_SUCCESS(status))
    {
        currentUserKeyName = &PhPredefineKeyNames[PH_KEY_CURRENT_USER_NUMBER];
        currentUserKeyName->Length = currentUserPrefix.Length + stringSid.Length;
        currentUserKeyName->MaximumLength = currentUserKeyName->Length + sizeof(UNICODE_NULL);
        currentUserKeyName->Buffer = PhAllocate(currentUserKeyName->MaximumLength);
        memcpy(currentUserKeyName->Buffer, currentUserPrefix.Buffer, currentUserPrefix.Length);
        memcpy(&currentUserKeyName->Buffer[currentUserPrefix.Length / sizeof(WCHAR)], stringSid.Buffer, stringSid.Length);
    }
}

/**
 * Initializes the attributes of a key object for creating/opening.
 *
 * \param RootDirectory A handle to a root key, or one of the predefined keys. See PhCreateKey() for
 * details.
 * \param ObjectName The path to the key.
 * \param Attributes Additional object flags.
 * \param ObjectAttributes The OBJECT_ATTRIBUTES structure to initialize.
 * \param NeedsClose A variable which receives a handle that must be closed when the create/open
 * operation is finished. The variable may be set to NULL if no handle needs to be closed.
 */
NTSTATUS PhpInitializeKeyObjectAttributes(
    _In_opt_ HANDLE RootDirectory,
    _In_ PUNICODE_STRING ObjectName,
    _In_ ULONG Attributes,
    _Out_ POBJECT_ATTRIBUTES ObjectAttributes,
    _Out_ PHANDLE NeedsClose
    )
{
    NTSTATUS status;
    ULONG predefineIndex;
    HANDLE predefineHandle;
    OBJECT_ATTRIBUTES predefineObjectAttributes;

    InitializeObjectAttributes(
        ObjectAttributes,
        ObjectName,
        Attributes | OBJ_CASE_INSENSITIVE,
        RootDirectory,
        NULL
        );

    *NeedsClose = NULL;

    if (RootDirectory && PH_KEY_IS_PREDEFINED(RootDirectory))
    {
        predefineIndex = PH_KEY_PREDEFINE_TO_NUMBER(RootDirectory);

        if (predefineIndex < PH_KEY_MAXIMUM_PREDEFINE)
        {
            if (PhBeginInitOnce(&PhPredefineKeyInitOnce))
            {
                PhpInitializePredefineKeys();
                PhEndInitOnce(&PhPredefineKeyInitOnce);
            }

            predefineHandle = PhPredefineKeyHandles[predefineIndex];

            if (!predefineHandle)
            {
                // The predefined key has not been opened yet. Do so now.

                if (!PhPredefineKeyNames[predefineIndex].Buffer) // we may have failed in getting the current user key name
                    return STATUS_UNSUCCESSFUL;

                InitializeObjectAttributes(
                    &predefineObjectAttributes,
                    &PhPredefineKeyNames[predefineIndex],
                    OBJ_CASE_INSENSITIVE,
                    NULL,
                    NULL
                    );

                status = NtOpenKey(
                    &predefineHandle,
                    KEY_READ,
                    &predefineObjectAttributes
                    );

                if (!NT_SUCCESS(status))
                    return status;

                if (_InterlockedCompareExchangePointer(
                    &PhPredefineKeyHandles[predefineIndex],
                    predefineHandle,
                    NULL
                    ) != NULL)
                {
                    // Someone else already opened the key and cached it. Indicate that the caller
                    // needs to close the handle later, since it isn't shared.
                    *NeedsClose = predefineHandle;
                }
            }

            ObjectAttributes->RootDirectory = predefineHandle;
        }
    }

    return STATUS_SUCCESS;
}

/**
 * Creates or opens a registry key.
 *
 * \param KeyHandle A variable which receives a handle to the key.
 * \param DesiredAccess The desired access to the key.
 * \param RootDirectory A handle to a root key, or one of the following predefined keys:
 * \li \c PH_KEY_LOCAL_MACHINE Represents \\Registry\\Machine.
 * \li \c PH_KEY_USERS Represents \\Registry\\User.
 * \li \c PH_KEY_CLASSES_ROOT Represents \\Registry\\Machine\\Software\\Classes.
 * \li \c PH_KEY_CURRENT_USER Represents \\Registry\\User\\[SID of current user].
 * \param ObjectName The path to the key.
 * \param Attributes Additional object flags.
 * \param CreateOptions The options to apply when creating or opening the key.
 * \param Disposition A variable which receives a value indicating whether a new key was created or
 * an existing key was opened:
 * \li \c REG_CREATED_NEW_KEY A new key was created.
 * \li \c REG_OPENED_EXISTING_KEY An existing key was opened.
 */
NTSTATUS PhCreateKey(
    _Out_ PHANDLE KeyHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_opt_ HANDLE RootDirectory,
    _In_ PPH_STRINGREF ObjectName,
    _In_ ULONG Attributes,
    _In_ ULONG CreateOptions,
    _Out_opt_ PULONG Disposition
    )
{
    NTSTATUS status;
    UNICODE_STRING objectName;
    OBJECT_ATTRIBUTES objectAttributes;
    HANDLE needsClose;

    if (!PhStringRefToUnicodeString(ObjectName, &objectName))
        return STATUS_NAME_TOO_LONG;

    if (!NT_SUCCESS(status = PhpInitializeKeyObjectAttributes(
        RootDirectory,
        &objectName,
        Attributes,
        &objectAttributes,
        &needsClose
        )))
    {
        return status;
    }

    status = NtCreateKey(
        KeyHandle,
        DesiredAccess,
        &objectAttributes,
        0,
        NULL,
        CreateOptions,
        Disposition
        );

    if (needsClose)
        NtClose(needsClose);

    return status;
}

/**
 * Opens a registry key.
 *
 * \param KeyHandle A variable which receives a handle to the key.
 * \param DesiredAccess The desired access to the key.
 * \param RootDirectory A handle to a root key, or one of the predefined keys. See PhCreateKey() for
 * details.
 * \param ObjectName The path to the key.
 * \param Attributes Additional object flags.
 */
NTSTATUS PhOpenKey(
    _Out_ PHANDLE KeyHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_opt_ HANDLE RootDirectory,
    _In_ PPH_STRINGREF ObjectName,
    _In_ ULONG Attributes
    )
{
    NTSTATUS status;
    UNICODE_STRING objectName;
    OBJECT_ATTRIBUTES objectAttributes;
    HANDLE needsClose;

    if (!PhStringRefToUnicodeString(ObjectName, &objectName))
        return STATUS_NAME_TOO_LONG;

    if (!NT_SUCCESS(status = PhpInitializeKeyObjectAttributes(
        RootDirectory,
        &objectName,
        Attributes,
        &objectAttributes,
        &needsClose
        )))
    {
        return status;
    }

    status = NtOpenKey(
        KeyHandle,
        DesiredAccess,
        &objectAttributes
        );

    if (needsClose)
        NtClose(needsClose);

    return status;
}

// rev from RegLoadAppKey
/**
 * Loads the specified registry hive file into a private application hive.
 *
 * \param KeyHandle A variable which receives a handle to the key.
 * \param FileName A string containing a file name.
 * \param DesiredAccess The desired access to the key.
 * \param Flags Optional flags for loading the hive.
 */
NTSTATUS PhLoadAppKey(
    _Out_ PHANDLE KeyHandle,
    _In_ PPH_STRINGREF FileName,
    _In_ ACCESS_MASK DesiredAccess,
    _In_opt_ ULONG Flags
    )
{
    NTSTATUS status;
    GUID guid;
    UNICODE_STRING fileName;
    UNICODE_STRING objectName;
    OBJECT_ATTRIBUTES targetAttributes;
    OBJECT_ATTRIBUTES sourceAttributes;

    if (!PhStringRefToUnicodeString(FileName, &fileName))
        return STATUS_NAME_TOO_LONG;

    PhGenerateGuid(&guid);

#if (PHNT_USE_NATIVE_APPEND)
    UNICODE_STRING guidStringUs;
    WCHAR objectNameBuffer[MAXIMUM_FILENAME_LENGTH];

    RtlInitEmptyUnicodeString(&objectName, objectNameBuffer, sizeof(objectNameBuffer));

    if (!NT_SUCCESS(status = RtlStringFromGUID(&guid, &guidStringUs)))
        return status;

    if (!NT_SUCCESS(status = RtlAppendUnicodeToString(&objectName, L"\\REGISTRY\\A\\")))
        goto CleanupExit;

    if (!NT_SUCCESS(status = RtlAppendUnicodeStringToString(&objectName, &guidStringUs)))
        goto CleanupExit;
#else
    static PH_STRINGREF namespaceString = PH_STRINGREF_INIT(L"\\REGISTRY\\A\\");
    PPH_STRING guidString;

    if (!(guidString = PhFormatGuid(&guid)))
        return STATUS_UNSUCCESSFUL;

    PhMoveReference(&guidString, PhConcatStringRef2(&namespaceString, &guidString->sr));

    if (!PhStringRefToUnicodeString(&guidString->sr, &objectName))
    {
        PhDereferenceObject(guidString);
        return STATUS_NAME_TOO_LONG;
    }
#endif

    InitializeObjectAttributes(
        &targetAttributes,
        &objectName,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    InitializeObjectAttributes(
        &sourceAttributes,
        &fileName,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    status = NtLoadKeyEx(
        &targetAttributes,
        &sourceAttributes,
        REG_APP_HIVE | Flags,
        NULL,
        NULL,
        DesiredAccess,
        KeyHandle,
        NULL
        );

#if (PHNT_USE_NATIVE_APPEND)
    RtlFreeUnicodeString(&guidStringUs);
#else
    PhDereferenceObject(guidString);
#endif

    return status;
}

/**
 * Gets information about a registry key.
 *
 * \param KeyHandle A handle to the key.
 * \param KeyInformationClass The information class to query.
 * \param Buffer A variable which receives a pointer to a buffer containing information about the
 * registry key. You must free the buffer with PhFree() when you no longer need it.
 */
NTSTATUS PhQueryKey(
    _In_ HANDLE KeyHandle,
    _In_ KEY_INFORMATION_CLASS KeyInformationClass,
    _Out_ PVOID *Buffer
    )
{
    NTSTATUS status;
    PVOID buffer;
    ULONG bufferSize;
    ULONG attempts = 16;

    bufferSize = 0x100;
    buffer = PhAllocate(bufferSize);

    do
    {
        status = NtQueryKey(
            KeyHandle,
            KeyInformationClass,
            buffer,
            bufferSize,
            &bufferSize
            );

        if (NT_SUCCESS(status))
            break;

        if (status == STATUS_BUFFER_OVERFLOW)
        {
            PhFree(buffer);
            buffer = PhAllocate(bufferSize);
        }
        else
        {
            PhFree(buffer);
            return status;
        }
    } while (--attempts);

    *Buffer = buffer;

    return status;
}

/**
 * Gets the information for a registry key.
 *
 * \param KeyHandle A handle to the key.
 * \param Information The registry key information, including information
 * about its subkeys and the maximum length for their names and value entries.
 */
NTSTATUS PhQueryKeyInformation(
    _In_ HANDLE KeyHandle,
    _Out_opt_ PKEY_FULL_INFORMATION Information
    )
{
    NTSTATUS status;
    ULONG bufferLength = 0;

    status = NtQueryKey(
        KeyHandle,
        KeyFullInformation,
        Information,
        sizeof(KEY_FULL_INFORMATION),
        &bufferLength
        );

    return status;
}

/**
 * Gets the last write time for a registry key without allocating memory. (dmex)
 * The order of information classes is based on performance.
 *
 * \param KeyHandle A handle to the key.
 * \param LastWriteTime The last write time of the key.
 */
NTSTATUS PhQueryKeyLastWriteTime(
    _In_ HANDLE KeyHandle,
    _Out_ PLARGE_INTEGER LastWriteTime
    )
{
    NTSTATUS status;
    KEY_FULL_INFORMATION fullInfo = { 0 };
    ULONG bufferLength = 0;

    status = NtQueryKey(
        KeyHandle,
        KeyFullInformation,
        &fullInfo,
        sizeof(KEY_FULL_INFORMATION),
        &bufferLength
        );

    if (NT_SUCCESS(status) || status == STATUS_BUFFER_OVERFLOW && fullInfo.LastWriteTime.QuadPart != 0)
    {
        *LastWriteTime = fullInfo.LastWriteTime;
        return STATUS_SUCCESS;
    }
    else
    {
        KEY_BASIC_INFORMATION basicInfo = { 0 };

        status = NtQueryKey(
            KeyHandle,
            KeyBasicInformation,
            &basicInfo,
            UFIELD_OFFSET(KEY_BASIC_INFORMATION, Name),
            &bufferLength
            );

        if (status == STATUS_BUFFER_OVERFLOW && basicInfo.LastWriteTime.QuadPart != 0)
        {
            *LastWriteTime = basicInfo.LastWriteTime;
            return STATUS_SUCCESS;
        }
        else
        {
            PKEY_BASIC_INFORMATION buffer;

            status = PhQueryKey(
                KeyHandle,
                KeyBasicInformation,
                &buffer
                );

            if (NT_SUCCESS(status))
            {
                memcpy(LastWriteTime, &buffer->LastWriteTime, sizeof(LARGE_INTEGER));
                PhFree(buffer);
            }
        }
    }

    return status;
}

/**
 * Gets a registry value of any type.
 *
 * \param KeyHandle A handle to the key.
 * \param ValueName The name of the value.
 * \param KeyValueInformationClass The information class to query.
 * \param Buffer A variable which receives a pointer to a buffer containing information about the
 * registry value. You must free the buffer with PhFree() when you no longer need it.
 */
NTSTATUS PhQueryValueKey(
    _In_ HANDLE KeyHandle,
    _In_opt_ PPH_STRINGREF ValueName,
    _In_ KEY_VALUE_INFORMATION_CLASS KeyValueInformationClass,
    _Out_ PVOID *Buffer
    )
{
    NTSTATUS status;
    UNICODE_STRING valueName;
    PVOID buffer;
    ULONG bufferSize;
    ULONG attempts = 16;

    if (ValueName && ValueName->Length)
    {
        if (!PhStringRefToUnicodeString(ValueName, &valueName))
            return STATUS_NAME_TOO_LONG;
    }
    else
    {
        RtlInitEmptyUnicodeString(&valueName, NULL, 0);
    }

    bufferSize = 0x100;
    buffer = PhAllocate(bufferSize);

    do
    {
        status = NtQueryValueKey(
            KeyHandle,
            &valueName,
            KeyValueInformationClass,
            buffer,
            bufferSize,
            &bufferSize
            );

        if (NT_SUCCESS(status))
            break;

        if (status == STATUS_BUFFER_OVERFLOW)
        {
            PhFree(buffer);
            buffer = PhAllocate(bufferSize);
        }
        else
        {
            PhFree(buffer);
            return status;
        }
    } while (--attempts);

    *Buffer = buffer;

    return status;
}

NTSTATUS PhSetValueKey(
    _In_ HANDLE KeyHandle,
    _In_opt_ PPH_STRINGREF ValueName,
    _In_ ULONG ValueType,
    _In_ PVOID Buffer,
    _In_ ULONG BufferLength
    )
{
    NTSTATUS status;
    UNICODE_STRING valueName;

    if (ValueName)
    {
        if (!PhStringRefToUnicodeString(ValueName, &valueName))
            return STATUS_NAME_TOO_LONG;
    }
    else
    {
        RtlInitEmptyUnicodeString(&valueName, NULL, 0);
    }

    status = NtSetValueKey(
        KeyHandle,
        &valueName,
        0,
        ValueType,
        Buffer,
        BufferLength
        );

    return status;
}

NTSTATUS PhDeleteValueKey(
    _In_ HANDLE KeyHandle,
    _In_opt_ PPH_STRINGREF ValueName
    )
{
    UNICODE_STRING valueName;

    if (ValueName)
    {
        if (!PhStringRefToUnicodeString(ValueName, &valueName))
            return STATUS_NAME_TOO_LONG;
    }
    else
    {
        RtlInitEmptyUnicodeString(&valueName, NULL, 0);
    }

    return NtDeleteValueKey(KeyHandle, &valueName);
}

NTSTATUS PhEnumerateKey(
    _In_ HANDLE KeyHandle,
    _In_ KEY_INFORMATION_CLASS InformationClass,
    _In_ PPH_ENUM_KEY_CALLBACK Callback,
    _In_opt_ PVOID Context
    )
{
    NTSTATUS status;
    PVOID buffer;
    ULONG bufferSize;
    ULONG index = 0;

    bufferSize = 0x100;
    buffer = PhAllocate(bufferSize);

    while (TRUE)
    {
        status = NtEnumerateKey(
            KeyHandle,
            index,
            InformationClass,
            buffer,
            bufferSize,
            &bufferSize
            );

        if (status == STATUS_NO_MORE_ENTRIES)
        {
            status = STATUS_SUCCESS;
            break;
        }

        if (status == STATUS_BUFFER_OVERFLOW || status == STATUS_BUFFER_TOO_SMALL)
        {
            PhFree(buffer);
            buffer = PhAllocate(bufferSize);

            status = NtEnumerateKey(
                KeyHandle,
                index,
                InformationClass,
                buffer,
                bufferSize,
                &bufferSize
                );
        }

        if (!NT_SUCCESS(status))
            break;

        if (!Callback(KeyHandle, buffer, Context))
            break;

        index++;
    }

    PhFree(buffer);

    return status;
}

NTSTATUS PhEnumerateValueKey(
    _In_ HANDLE KeyHandle,
    _In_ KEY_VALUE_INFORMATION_CLASS InformationClass,
    _In_ PPH_ENUM_KEY_CALLBACK Callback,
    _In_opt_ PVOID Context
    )
{
    NTSTATUS status;
    PVOID buffer;
    ULONG bufferSize;
    ULONG returnLength;
    ULONG index = 0;

    bufferSize = 0x500;
    buffer = PhAllocate(bufferSize);

    while (TRUE)
    {
        status = NtEnumerateValueKey(
            KeyHandle,
            index,
            InformationClass,
            buffer,
            bufferSize,
            &returnLength
            );

        if (status == STATUS_NO_MORE_ENTRIES)
        {
            status = STATUS_SUCCESS;
            break;
        }

        if (status == STATUS_BUFFER_OVERFLOW || status == STATUS_BUFFER_TOO_SMALL)
        {
            bufferSize = returnLength;
            PhFree(buffer);
            buffer = PhAllocate(bufferSize);

            status = NtEnumerateValueKey(
                KeyHandle,
                index,
                InformationClass,
                buffer,
                bufferSize,
                &returnLength
                );
        }

        if (!NT_SUCCESS(status))
            break;

        if (!Callback(KeyHandle, buffer, Context))
            break;

        index++;
    }

    PhFree(buffer);

    return status;
}
