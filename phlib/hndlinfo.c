/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2010-2015
 *     dmex    2017-2023
 *
 */

#include <ph.h>
#include <hndlinfo.h>
#include <json.h>
#include <kphuser.h>
#include <mapldr.h>
#include <lsasup.h>

#include <devquery.h>
#include <devpkey.h>

#define PH_QUERY_HACK_MAX_THREADS 20

typedef struct _PHP_CALL_WITH_TIMEOUT_THREAD_CONTEXT
{
    SLIST_ENTRY ListEntry;

    PUSER_THREAD_START_ROUTINE Routine;
    PVOID Context;

    HANDLE StartEventHandle;
    HANDLE CompletedEventHandle;
    HANDLE ThreadHandle;
} PHP_CALL_WITH_TIMEOUT_THREAD_CONTEXT, *PPHP_CALL_WITH_TIMEOUT_THREAD_CONTEXT;

typedef enum _PHP_QUERY_OBJECT_WORK
{
    NtQueryObjectWork,
    NtQuerySecurityObjectWork,
    NtSetSecurityObjectWork,
    NtQueryFileInformationWork,
    KphQueryFileInformationWork
} PHP_QUERY_OBJECT_WORK;

typedef struct _PHP_QUERY_OBJECT_COMMON_CONTEXT
{
    PHP_QUERY_OBJECT_WORK Work;
    NTSTATUS Status;

    union
    {
        struct
        {
            HANDLE Handle;
            OBJECT_INFORMATION_CLASS ObjectInformationClass;
            PVOID ObjectInformation;
            ULONG ObjectInformationLength;
            PULONG ReturnLength;
        } NtQueryObject;
        struct
        {
            HANDLE Handle;
            SECURITY_INFORMATION SecurityInformation;
            PSECURITY_DESCRIPTOR SecurityDescriptor;
            ULONG Length;
            PULONG LengthNeeded;
        } NtQuerySecurityObject;
        struct
        {
            HANDLE Handle;
            SECURITY_INFORMATION SecurityInformation;
            PSECURITY_DESCRIPTOR SecurityDescriptor;
        } NtSetSecurityObject;
        struct
        {
            HANDLE Handle;
            FILE_INFORMATION_CLASS FileInformationClass;
            PVOID FileInformation;
            ULONG FileInformationLength;
        } NtQueryFileInformation;
        struct
        {
            HANDLE ProcessHandle;
            HANDLE Handle;
            FILE_INFORMATION_CLASS FileInformationClass;
            PVOID FileInformation;
            ULONG FileInformationLength;
        } KphQueryFileInformation;

    } u;
} PHP_QUERY_OBJECT_COMMON_CONTEXT, *PPHP_QUERY_OBJECT_COMMON_CONTEXT;

PPHP_CALL_WITH_TIMEOUT_THREAD_CONTEXT PhpAcquireCallWithTimeoutThread(
    _In_opt_ PLARGE_INTEGER Timeout
    );

VOID PhpReleaseCallWithTimeoutThread(
    _Inout_ PPHP_CALL_WITH_TIMEOUT_THREAD_CONTEXT ThreadContext
    );

NTSTATUS PhpCallWithTimeout(
    _Inout_ PPHP_CALL_WITH_TIMEOUT_THREAD_CONTEXT ThreadContext,
    _In_ PUSER_THREAD_START_ROUTINE Routine,
    _In_opt_ PVOID Context,
    _In_ PLARGE_INTEGER Timeout
    );

NTSTATUS PhpCallWithTimeoutThreadStart(
    _In_ PVOID Parameter
    );

BOOLEAN PhEnableProcessHandlePnPDeviceNameSupport = FALSE;

static PPH_STRING PhObjectTypeNames[MAX_OBJECT_TYPE_NUMBER] = { 0 };
static PPH_GET_CLIENT_ID_NAME PhHandleGetClientIdName = PhStdGetClientIdName;

static SLIST_HEADER PhpCallWithTimeoutThreadListHead;
static PH_WAKE_EVENT PhpCallWithTimeoutThreadReleaseEvent = PH_WAKE_EVENT_INIT;

PPH_GET_CLIENT_ID_NAME PhSetHandleClientIdFunction(
    _In_ PPH_GET_CLIENT_ID_NAME GetClientIdName
    )
{
    return _InterlockedExchangePointer(
        (PVOID *)&PhHandleGetClientIdName,
        GetClientIdName
        );
}

NTSTATUS PhpGetObjectBasicInformation(
    _In_ HANDLE ProcessHandle,
    _In_ HANDLE Handle,
    _Out_ POBJECT_BASIC_INFORMATION BasicInformation
    )
{
    NTSTATUS status;

    if (KphLevel() >= KphLevelMed)
    {
        status = KphQueryInformationObject(
            ProcessHandle,
            Handle,
            KphObjectBasicInformation,
            BasicInformation,
            sizeof(OBJECT_BASIC_INFORMATION),
            NULL
            );

        if (NT_SUCCESS(status))
        {
            // The object was referenced in KSystemInformer, so we need to subtract 1 from the
            // pointer count.
            BasicInformation->PointerCount -= 1;
        }
    }
    else
    {
        ULONG returnLength;

        status = NtQueryObject(
            Handle,
            ObjectBasicInformation,
            BasicInformation,
            sizeof(OBJECT_BASIC_INFORMATION),
            &returnLength
            );

        if (NT_SUCCESS(status))
        {
            // The object was referenced in NtQueryObject and a handle was opened to the object. We
            // need to subtract 1 from the pointer count, then subtract 1 from both counts.
            BasicInformation->HandleCount -= 1;
            BasicInformation->PointerCount -= 2;
        }
    }

    return status;
}

NTSTATUS PhpGetObjectTypeName(
    _In_opt_ HANDLE ProcessHandle,
    _In_ HANDLE Handle,
    _In_ ULONG ObjectTypeNumber,
    _Out_ PPH_STRING *TypeName
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PPH_STRING typeName = NULL;

    // Enumerate the available object types and pre-cache the object type name. (dmex)
    if (WindowsVersion >= WINDOWS_8_1)
    {
        static PH_INITONCE initOnce = PH_INITONCE_INIT;

        if (PhBeginInitOnce(&initOnce))
        {
            POBJECT_TYPES_INFORMATION objectTypes;
            POBJECT_TYPE_INFORMATION objectType;

            if (NT_SUCCESS(PhEnumObjectTypes(&objectTypes)))
            {
                objectType = PH_FIRST_OBJECT_TYPE(objectTypes);

                for (ULONG i = 0; i < objectTypes->NumberOfTypes; i++)
                {
                    PhMoveReference(
                        &PhObjectTypeNames[objectType->TypeIndex],
                        PhCreateStringFromUnicodeString(&objectType->TypeName)
                        );

                    objectType = PH_NEXT_OBJECT_TYPE(objectType);
                }

                PhFree(objectTypes);
            }

            PhEndInitOnce(&initOnce);
        }
    }

    // If the cache contains the object type name, use it. Otherwise, query the type name. (dmex)

    if (ObjectTypeNumber != ULONG_MAX && ObjectTypeNumber < MAX_OBJECT_TYPE_NUMBER)
        typeName = PhObjectTypeNames[ObjectTypeNumber];

    if (typeName)
    {
        PhReferenceObject(typeName);
    }
    else
    {
        POBJECT_TYPE_INFORMATION buffer;
        ULONG returnLength = 0;
        PPH_STRING oldTypeName;
        KPH_LEVEL level;

        level = KphLevel();

        // Get the needed buffer size.
        if (level >= KphLevelMed)
        {
            status = KphQueryInformationObject(
                ProcessHandle,
                Handle,
                KphObjectTypeInformation,
                NULL,
                0,
                &returnLength
                );
        }
        else
        {
            status = NtQueryObject(
                Handle,
                ObjectTypeInformation,
                NULL,
                0,
                &returnLength
                );
        }

        if (returnLength == 0)
            return STATUS_UNSUCCESSFUL;

        buffer = PhAllocate(returnLength);

        if (level >= KphLevelMed)
        {
            status = KphQueryInformationObject(
                ProcessHandle,
                Handle,
                KphObjectTypeInformation,
                buffer,
                returnLength,
                &returnLength
                );
        }
        else
        {
            status = NtQueryObject(
                Handle,
                ObjectTypeInformation,
                buffer,
                returnLength,
                &returnLength
                );
        }

        if (!NT_SUCCESS(status))
        {
            PhFree(buffer);
            return status;
        }

        // Create a copy of the type name.
        typeName = PhCreateStringFromUnicodeString(&buffer->TypeName);

        if (ObjectTypeNumber != ULONG_MAX && ObjectTypeNumber < MAX_OBJECT_TYPE_NUMBER)
        {
            // Try to store the type name in the cache.
            oldTypeName = _InterlockedCompareExchangePointer(
                &PhObjectTypeNames[ObjectTypeNumber],
                typeName,
                NULL
                );

            // Add a reference if we stored the type name successfully.
            if (!oldTypeName)
                PhReferenceObject(typeName);
        }

        PhFree(buffer);
    }

    // At this point typeName should contain a type name with one additional reference.

    *TypeName = typeName;

    return status;
}

NTSTATUS PhpGetObjectName(
    _In_ HANDLE ProcessHandle,
    _In_ HANDLE Handle,
    _In_ BOOLEAN WithTimeout,
    _Out_ PPH_STRING *ObjectName
    )
{
    NTSTATUS status;
    POBJECT_NAME_INFORMATION buffer;
    ULONG bufferSize;
    ULONG attempts = 8;

    bufferSize = sizeof(OBJECT_NAME_INFORMATION) + (MAXIMUM_FILENAME_LENGTH * sizeof(WCHAR));
    buffer = PhAllocate(bufferSize);

    // A loop is needed because the I/O subsystem likes to give us the wrong return lengths... (wj32)
    do
    {
        if (KphLevel() >= KphLevelMed)
        {
            status = KphQueryInformationObject(
                ProcessHandle,
                Handle,
                KphObjectNameInformation,
                buffer,
                bufferSize,
                &bufferSize
                );
        }
        else
        {
            if (WithTimeout)
            {
                status = PhCallNtQueryObjectWithTimeout(
                    Handle,
                    ObjectNameInformation,
                    buffer,
                    bufferSize,
                    &bufferSize
                    );
            }
            else
            {
                status = NtQueryObject(
                    Handle,
                    ObjectNameInformation,
                    buffer,
                    bufferSize,
                    &bufferSize
                    );
            }
        }

        if (status == STATUS_BUFFER_OVERFLOW || status == STATUS_INFO_LENGTH_MISMATCH ||
            status == STATUS_BUFFER_TOO_SMALL)
        {
            PhFree(buffer);
            buffer = PhAllocate(bufferSize);
        }
        else
        {
            break;
        }
    } while (--attempts);

    if (NT_SUCCESS(status))
    {
        *ObjectName = PhCreateStringFromUnicodeString(&buffer->Name);
    }

    PhFree(buffer);

    return status;
}

NTSTATUS PhQueryObjectName(
    _In_ HANDLE Handle,
    _Out_ PPH_STRING *ObjectName
    )
{
    if (Handle == NULL || Handle == NtCurrentProcess() || Handle == NtCurrentThread())
        return STATUS_INVALID_HANDLE;

    return PhpGetObjectName(NtCurrentProcess(), Handle, FALSE, ObjectName);
}

NTSTATUS PhQueryObjectBasicInformation(
    _In_ HANDLE Handle,
    _Out_ POBJECT_BASIC_INFORMATION BasicInformation
    )
{
    if (Handle == NULL || Handle == NtCurrentProcess() || Handle == NtCurrentThread())
        return STATUS_INVALID_HANDLE;

    return PhpGetObjectBasicInformation(NtCurrentProcess(), Handle, BasicInformation);
}

NTSTATUS PhpGetEtwObjectName(
    _In_ HANDLE ProcessHandle,
    _In_ HANDLE Handle,
    _Out_ PPH_STRING *ObjectName
    )
{
    NTSTATUS status;
    ETWREG_BASIC_INFORMATION basicInfo;

    status = KphQueryInformationObject(
        ProcessHandle,
        Handle,
        KphObjectEtwRegBasicInformation,
        &basicInfo,
        sizeof(ETWREG_BASIC_INFORMATION),
        NULL
        );

    if (NT_SUCCESS(status))
    {
        *ObjectName = PhFormatGuid(&basicInfo.Guid);
    }

    return status;
}

typedef struct _PH_ETW_TRACEGUID_ENTRY
{
    PPH_STRING Name;
    PGUID Guid;
} PH_ETW_TRACEGUID_ENTRY, *PPH_ETW_TRACEGUID_ENTRY;

VOID PhInitializeEtwTraceGuidCache(
    _Inout_ PPH_ARRAY EtwTraceGuidArrayList
    )
{
    PPH_BYTES guidListString = NULL;
    PPH_STRING guidListFileName;
    PVOID jsonObject;
    ULONG arrayLength;

    if (guidListFileName = PhGetApplicationDirectory())
    {
        PhMoveReference(&guidListFileName, PhConcatStringRefZ(&guidListFileName->sr, L"etwguids.txt"));
        guidListString = PhFileReadAllText(&guidListFileName->sr, TRUE);
        PhDereferenceObject(guidListFileName);
    }

    if (!guidListString)
        return;

    PhInitializeArray(EtwTraceGuidArrayList, sizeof(PH_ETW_TRACEGUID_ENTRY), 2000);

    if (!(jsonObject = PhCreateJsonParserEx(guidListString, FALSE)))
        return;

    if (!(arrayLength = PhGetJsonArrayLength(jsonObject)))
    {
        PhFreeJsonObject(jsonObject);
        return;
    }

    for (ULONG i = 0; i < arrayLength; i++)
    {
        PVOID jsonArrayObject;
        PPH_STRING guidString;
        PPH_STRING guidName;
        GUID guid;
        PH_ETW_TRACEGUID_ENTRY result;

        if (!(jsonArrayObject = PhGetJsonArrayIndexObject(jsonObject, i)))
            continue;

        guidString = PhGetJsonValueAsString(jsonArrayObject, "guid");
        guidName = PhGetJsonValueAsString(jsonArrayObject, "name");
        //guidGroup = PhGetJsonValueAsString(jsonArrayObject, "group");

        if (!NT_SUCCESS(PhStringToGuid(&guidString->sr, &guid)))
        {
            PhDereferenceObject(guidName);
            PhDereferenceObject(guidString);
            continue;
        }

        result.Name = guidName;
        result.Guid = PhAllocateCopy(&guid, sizeof(GUID));

        PhAddItemArray(EtwTraceGuidArrayList, &result);

        PhDereferenceObject(guidString);
    }

    PhFreeJsonObject(jsonObject);
    PhDereferenceObject(guidListString);
}

PPH_STRING PhGetEtwTraceGuidName(
    _In_ PGUID Guid
    )
{
    static PH_INITONCE initOnce = PH_INITONCE_INIT;
    static PH_ARRAY etwTraceGuidArrayList;
    PPH_ETW_TRACEGUID_ENTRY entry;
    SIZE_T i;

    if (WindowsVersion < WINDOWS_8)
        return NULL;

    if (PhBeginInitOnce(&initOnce))
    {
        PhInitializeEtwTraceGuidCache(&etwTraceGuidArrayList);
        PhEndInitOnce(&initOnce);
    }

    for (i = 0; i < etwTraceGuidArrayList.Count; i++)
    {
        entry = PhItemArray(&etwTraceGuidArrayList, i);

        if (IsEqualGUID(entry->Guid, Guid))
        {
            return PhReferenceObject(entry->Name);
        }
    }

    return NULL;
}

PPH_STRING PhGetEtwPublisherName(
    _In_ PGUID Guid
    )
{
    static PH_STRINGREF publishersKeyName = PH_STRINGREF_INIT(L"Software\\Microsoft\\Windows\\CurrentVersion\\WINEVT\\Publishers\\");
    PPH_STRING guidString;
    PPH_STRING keyName;
    HANDLE keyHandle;
    PPH_STRING publisherName = NULL;

    guidString = PhFormatGuid(Guid);

    // We should perform a lookup on the GUID to get the publisher name. (wj32)

    keyName = PhConcatStringRef2(&publishersKeyName, &guidString->sr);

    if (NT_SUCCESS(PhOpenKey(
        &keyHandle,
        KEY_READ,
        PH_KEY_LOCAL_MACHINE,
        &keyName->sr,
        0
        )))
    {
        publisherName = PhQueryRegistryString(keyHandle, NULL);

        if (publisherName && publisherName->Length == 0)
        {
            PhDereferenceObject(publisherName);
            publisherName = NULL;
        }

        NtClose(keyHandle);
    }

    PhDereferenceObject(keyName);

    if (publisherName)
    {
        PhDereferenceObject(guidString);
        return publisherName;
    }
    else
    {
        if (publisherName = PhGetEtwTraceGuidName(Guid))
        {
            PhDereferenceObject(guidString);
            return publisherName;
        }

        return guidString;
    }
}

PPH_STRING PhGetPnPDeviceName(
    _In_ PPH_STRING ObjectName
    )
{
    static PH_INITONCE initOnce = PH_INITONCE_INIT;
    static HRESULT (WINAPI* DevGetObjects_I)(
        _In_ DEV_OBJECT_TYPE ObjectType,
        _In_ ULONG QueryFlags,
        _In_ ULONG cRequestedProperties,
        _In_reads_opt_(cRequestedProperties) const DEVPROPCOMPKEY *pRequestedProperties,
        _In_ ULONG cFilterExpressionCount,
        _In_reads_opt_(cFilterExpressionCount) const DEVPROP_FILTER_EXPRESSION *pFilter,
        _Out_ PULONG pcObjectCount,
        _Outptr_result_buffer_maybenull_(*pcObjectCount) const DEV_OBJECT **ppObjects) = NULL;
    static HRESULT (WINAPI* DevFreeObjects_I)(
        _In_ ULONG cObjectCount,
        _In_reads_(cObjectCount) const DEV_OBJECT *pObjects) = NULL;

    PPH_STRING objectPnPDeviceName = NULL;
    ULONG deviceCount = 0;
    PDEV_OBJECT deviceObjects = NULL;
    DEVPROPCOMPKEY deviceProperties[2];
    DEVPROP_FILTER_EXPRESSION deviceFilter[1];
    DEVPROPERTY deviceFilterProperty;
    DEVPROPCOMPKEY deviceFilterCompoundProp;

    if (PhBeginInitOnce(&initOnce))
    {
        PVOID cfgmgr32;

        if (cfgmgr32 = PhLoadLibrary(L"cfgmgr32.dll"))
        {
            DevGetObjects_I = PhGetDllBaseProcedureAddress(cfgmgr32, "DevGetObjects", 0);
            DevFreeObjects_I = PhGetDllBaseProcedureAddress(cfgmgr32, "DevFreeObjects", 0);
        }

        PhEndInitOnce(&initOnce);
    }

    if (!(DevGetObjects_I && DevFreeObjects_I))
        return NULL;

    memset(deviceProperties, 0, sizeof(deviceProperties));
    deviceProperties[0].Key = DEVPKEY_NAME;
    deviceProperties[0].Store = DEVPROP_STORE_SYSTEM;
    deviceProperties[1].Key = DEVPKEY_Device_BusReportedDeviceDesc;
    deviceProperties[1].Store = DEVPROP_STORE_SYSTEM;

    memset(&deviceFilterCompoundProp, 0, sizeof(deviceFilterCompoundProp));
    deviceFilterCompoundProp.Key = DEVPKEY_Device_PDOName;
    deviceFilterCompoundProp.Store = DEVPROP_STORE_SYSTEM;
    deviceFilterCompoundProp.LocaleName = NULL;

    memset(&deviceFilterProperty, 0, sizeof(deviceFilterProperty));
    deviceFilterProperty.CompKey = deviceFilterCompoundProp;
    deviceFilterProperty.Type = DEVPROP_TYPE_STRING;
    deviceFilterProperty.BufferSize = (ULONG)ObjectName->Length + sizeof(UNICODE_NULL);
    deviceFilterProperty.Buffer = ObjectName->Buffer;

    memset(deviceFilter, 0, sizeof(deviceFilter));
    deviceFilter[0].Operator = DEVPROP_OPERATOR_EQUALS_IGNORE_CASE;
    deviceFilter[0].Property = deviceFilterProperty;

    if (SUCCEEDED(DevGetObjects_I(
        DevObjectTypeDevice,
        DevQueryFlagNone,
        RTL_NUMBER_OF(deviceProperties),
        deviceProperties,
        RTL_NUMBER_OF(deviceFilter),
        deviceFilter,
        &deviceCount,
        &deviceObjects
        )))
    {
        // Note: We can get a success with 0 results.
        if (deviceCount > 0)
        {
            DEV_OBJECT device = deviceObjects[0];
            PPH_STRING deviceName;
            PPH_STRING deviceDesc;
            PH_STRINGREF displayName;
            PH_STRINGREF displayDesc;
            PH_STRINGREF firstPart;
            PH_STRINGREF secondPart;

            displayName.Length = device.pProperties[0].BufferSize;
            displayName.Buffer = device.pProperties[0].Buffer;
            displayDesc.Length = device.pProperties[1].BufferSize;
            displayDesc.Buffer = device.pProperties[1].Buffer;

            if (PhSplitStringRefAtLastChar(&displayName, L';', &firstPart, &secondPart))
                deviceName = PhCreateString2(&secondPart);
            else
                deviceName = PhCreateString2(&displayName);

            if (PhSplitStringRefAtLastChar(&displayDesc, L';', &firstPart, &secondPart))
                deviceDesc = PhCreateString2(&secondPart);
            else
                deviceDesc = PhCreateString2(&displayDesc);

            if (deviceName->Length >= sizeof(UNICODE_NULL) && deviceName->Buffer[deviceName->Length / sizeof(WCHAR)] == UNICODE_NULL)
                deviceName->Length -= sizeof(UNICODE_NULL); // PhTrimToNullTerminatorString(deviceName);
            if (deviceDesc->Length >= sizeof(UNICODE_NULL) && deviceDesc->Buffer[deviceDesc->Length / sizeof(WCHAR)] == UNICODE_NULL)
                deviceDesc->Length -= sizeof(UNICODE_NULL); // PhTrimToNullTerminatorString(deviceDesc);

            if (!PhIsNullOrEmptyString(deviceDesc))
            {
                PH_FORMAT format[4];

                PhInitFormatSR(&format[0], deviceDesc->sr);
                PhInitFormatS(&format[1], L" (PDO: ");
                PhInitFormatSR(&format[2], ObjectName->sr);
                PhInitFormatC(&format[3], ')');

                PhSetReference(&objectPnPDeviceName, PhFormat(format, 4, 0x50));
            }
            else if (!PhIsNullOrEmptyString(deviceName))
            {
                PH_FORMAT format[4];

                PhInitFormatSR(&format[0], deviceName->sr);
                PhInitFormatS(&format[1], L" (PDO: ");
                PhInitFormatSR(&format[2], ObjectName->sr);
                PhInitFormatC(&format[3], ')');

                PhSetReference(&objectPnPDeviceName, PhFormat(format, 4, 0x50));
            }

            if (deviceDesc)
                PhDereferenceObject(deviceDesc);
            if (deviceName)
                PhDereferenceObject(deviceName);
        }

        DevFreeObjects_I(deviceCount, deviceObjects);
    }

    return objectPnPDeviceName;
}

PPH_STRING PhFormatNativeKeyName(
    _In_ PPH_STRING Name
    )
{
    static PH_STRINGREF hklmPrefix = PH_STRINGREF_INIT(L"\\Registry\\Machine");
    static PH_STRINGREF hkcrPrefix = PH_STRINGREF_INIT(L"\\Registry\\Machine\\Software\\Classes");
    static PH_STRINGREF hkuPrefix = PH_STRINGREF_INIT(L"\\Registry\\User");
    static PPH_STRING hkcuPrefix;
    static PPH_STRING hkcucrPrefix;

    static PH_STRINGREF hklmString = PH_STRINGREF_INIT(L"HKLM");
    static PH_STRINGREF hkcrString = PH_STRINGREF_INIT(L"HKCR");
    static PH_STRINGREF hkuString = PH_STRINGREF_INIT(L"HKU");
    static PH_STRINGREF hkcuString = PH_STRINGREF_INIT(L"HKCU");
    static PH_STRINGREF hkcucrString = PH_STRINGREF_INIT(L"HKCU\\Software\\Classes");

    static PH_INITONCE initOnce = PH_INITONCE_INIT;

    PPH_STRING newName;
    PH_STRINGREF name;

    if (PhBeginInitOnce(&initOnce))
    {
        PH_TOKEN_USER tokenUser;
        PPH_STRING stringSid = NULL;

        if (NT_SUCCESS(PhGetTokenUser(PhGetOwnTokenAttributes().TokenHandle, &tokenUser)))
        {
            stringSid = PhSidToStringSid(tokenUser.User.Sid);
        }

        if (stringSid)
        {
            static PH_STRINGREF registryUserPrefix = PH_STRINGREF_INIT(L"\\Registry\\User\\");
            static PH_STRINGREF classesString = PH_STRINGREF_INIT(L"_Classes");

            hkcuPrefix = PhConcatStringRef2(&registryUserPrefix, &stringSid->sr);
            hkcucrPrefix = PhConcatStringRef2(&hkcuPrefix->sr, &classesString);

            PhDereferenceObject(stringSid);
        }
        else
        {
            hkcuPrefix = PhCreateString(L"..."); // some random string that won't ever get matched
            hkcucrPrefix = PhCreateString(L"...");
        }

        PhEndInitOnce(&initOnce);
    }

    name = Name->sr;

    if (PhStartsWithStringRef(&name, &hkcrPrefix, TRUE))
    {
        PhSkipStringRef(&name, hkcrPrefix.Length);
        newName = PhConcatStringRef2(&hkcrString, &name);
    }
    else if (PhStartsWithStringRef(&name, &hklmPrefix, TRUE))
    {
        PhSkipStringRef(&name, hklmPrefix.Length);
        newName = PhConcatStringRef2(&hklmString, &name);
    }
    else if (PhStartsWithStringRef(&name, &hkcucrPrefix->sr, TRUE))
    {
        PhSkipStringRef(&name, hkcucrPrefix->Length);
        newName = PhConcatStringRef2(&hkcucrString, &name);
    }
    else if (PhStartsWithStringRef(&name, &hkcuPrefix->sr, TRUE))
    {
        PhSkipStringRef(&name, hkcuPrefix->Length);
        newName = PhConcatStringRef2(&hkcuString, &name);
    }
    else if (PhStartsWithStringRef(&name, &hkuPrefix, TRUE))
    {
        PhSkipStringRef(&name, hkuPrefix.Length);
        newName = PhConcatStringRef2(&hkuString, &name);
    }
    else
    {
        PhSetReference(&newName, Name);
    }

    return newName;
}

NTSTATUS PhGetSectionFileName(
    _In_ HANDLE SectionHandle,
    _Out_ PPH_STRING *FileName
    )
{
    NTSTATUS status;
    SIZE_T viewSize;
    PVOID viewBase;

    viewSize = 1;
    viewBase = NULL;

    status = NtMapViewOfSection(
        SectionHandle,
        NtCurrentProcess(),
        &viewBase,
        0,
        0,
        NULL,
        &viewSize,
        ViewUnmap,
        0,
        PAGE_READONLY
        );

    if (!NT_SUCCESS(status))
        return status;

    status = PhGetProcessMappedFileName(NtCurrentProcess(), viewBase, FileName);
    NtUnmapViewOfSection(NtCurrentProcess(), viewBase);

    return status;
}

_Callback_ PPH_STRING PhStdGetClientIdName(
    _In_ PCLIENT_ID ClientId
    )
{
    return PhStdGetClientIdNameEx(ClientId, NULL);
}

PPH_STRING PhStdGetClientIdNameEx(
    _In_ PCLIENT_ID ClientId,
    _In_opt_ PPH_STRING ProcessName
    )
{
    PPH_STRING result;
    PPH_STRING processName = NULL;
    PPH_STRING threadName = NULL;
    PH_STRINGREF processNameRef;
    PH_STRINGREF threadNameRef;
    HANDLE processHandle;
    HANDLE threadHandle;
    ULONG isProcessTerminated = FALSE;
    BOOLEAN isThreadTerminated = FALSE;

    if (ProcessName)
    {
        // Use the supplied name
        processNameRef.Length = ProcessName->Length;
        processNameRef.Buffer = ProcessName->Buffer;
    }
    else
    {
        if (ClientId->UniqueProcess == SYSTEM_PROCESS_ID)
        {
            if (processName = PhGetKernelFileName2())
            {
                PhMoveReference(&processName, PhGetBaseName(processName));
                processNameRef.Length = processName->Length;
                processNameRef.Buffer = processName->Buffer;
            }
            else
            {
                PhInitializeStringRef(&processNameRef, L"Unknown process");
            }
        }
        else
        {
            // Determine the name of the process ourselves (diversenok)
            if (NT_SUCCESS(PhGetProcessImageFileNameById(
                ClientId->UniqueProcess,
                NULL,
                &processName
                )))
            {
                processNameRef.Length = processName->Length;
                processNameRef.Buffer = processName->Buffer;
            }
            else
            {
                PhInitializeStringRef(&processNameRef, L"Unknown process");
            }
        }
    }

    // Check if the process is alive, but only if we didn't get its name
    if (!ProcessName && NT_SUCCESS(PhOpenProcess(
        &processHandle,
        SYNCHRONIZE,
        ClientId->UniqueProcess
        )))
    {
        LARGE_INTEGER timeout = { 0 };

        // Waiting with zero timeout checks for termination
        if (NtWaitForSingleObject(processHandle, FALSE, &timeout) == STATUS_WAIT_0)
            isProcessTerminated = TRUE;

        NtClose(processHandle);
    }

    PhInitializeStringRef(&threadNameRef, L"unnamed thread");

    if (ClientId->UniqueThread)
    {
        if (NT_SUCCESS(PhOpenThread(
            &threadHandle,
            THREAD_QUERY_LIMITED_INFORMATION,
            ClientId->UniqueThread
            )))
        {
            // Check if the thread is alive
            PhGetThreadIsTerminated(
                threadHandle,
                &isThreadTerminated
                );

            // Use the name of the thread if available
            if (NT_SUCCESS(PhGetThreadName(threadHandle, &threadName)))
            {
                threadNameRef.Length = threadName->Length;
                threadNameRef.Buffer = threadName->Buffer;
            }

            NtClose(threadHandle);
        }
    }

    // Combine everything

    if (ClientId->UniqueThread)
    {
        PH_FORMAT format[10];

        // L"%s%.*s (%lu): %s%.*s (%lu)"
        PhInitFormatS(&format[0], isProcessTerminated ? L"Terminated " : L"");
        PhInitFormatSR(&format[1], processNameRef);
        PhInitFormatS(&format[2], L" (");
        PhInitFormatU(&format[3], HandleToUlong(ClientId->UniqueProcess));
        PhInitFormatS(&format[4], L"): ");
        PhInitFormatS(&format[5], isThreadTerminated ? L"terminated " : L"");
        PhInitFormatSR(&format[6], threadNameRef);
        PhInitFormatS(&format[7], L" (");
        PhInitFormatU(&format[8], HandleToUlong(ClientId->UniqueThread));
        PhInitFormatC(&format[9], L')');

        result = PhFormat(format, RTL_NUMBER_OF(format), 0x50);
    }
    else
    {
        PH_FORMAT format[5];

        // L"%s%.*s (%lu)"
        PhInitFormatS(&format[0], isProcessTerminated ? L"Terminated " : L"");
        PhInitFormatSR(&format[1], processNameRef);
        PhInitFormatS(&format[2], L" (");
        PhInitFormatU(&format[3], HandleToUlong(ClientId->UniqueProcess));
        PhInitFormatC(&format[4], L')');

        result = PhFormat(format, RTL_NUMBER_OF(format), 0x50);
    }

    //result = PhFormatString(
    //    ClientId->UniqueThread ? L"%s%.*s (%lu): %s%.*s (%lu)" : L"%s%.*s (%lu)",
    //    isProcessTerminated ? L"Terminated " : L"",
    //    processNameRef.Length / sizeof(WCHAR),
    //    processNameRef.Buffer,
    //    HandleToUlong(ClientId->UniqueProcess),
    //    isThreadTerminated ? L"terminated " : L"",
    //    threadNameRef.Length / sizeof(WCHAR),
    //    threadNameRef.Buffer,
    //    HandleToUlong(ClientId->UniqueThread)
    //    );

    if (processName)
        PhDereferenceObject(processName);

    if (threadName)
        PhDereferenceObject(threadName);

    return result;
}

NTSTATUS PhpGetBestObjectName(
    _In_ HANDLE ProcessHandle,
    _In_ HANDLE Handle,
    _In_ PPH_STRING ObjectName,
    _In_ PPH_STRING TypeName,
    _Out_ PPH_STRING *BestObjectName
    )
{
    NTSTATUS status;
    PPH_STRING bestObjectName = NULL;
    PPH_GET_CLIENT_ID_NAME handleGetClientIdName = PhHandleGetClientIdName;

    if (PhEqualString2(TypeName, L"EtwRegistration", TRUE))
    {
        if (KphLevel() >= KphLevelMed)
        {
            ETWREG_BASIC_INFORMATION basicInfo;

            status = KphQueryInformationObject(
                ProcessHandle,
                Handle,
                KphObjectEtwRegBasicInformation,
                &basicInfo,
                sizeof(ETWREG_BASIC_INFORMATION),
                NULL
                );

            if (NT_SUCCESS(status))
            {
                bestObjectName = PhGetEtwPublisherName(&basicInfo.Guid);
            }
        }
    }
    else if (PhEqualString2(TypeName, L"File", TRUE))
    {
        // Convert the file name to a DOS file name.
        bestObjectName = PhResolveDevicePrefix(&ObjectName->sr);

        if (!bestObjectName)
        {
            if (PhEnableProcessHandlePnPDeviceNameSupport)
            {
                if (PhStartsWithString2(ObjectName, L"\\Device\\", TRUE))
                {
                    // The device might be a PDO... Query the PnP manager for the friendly name of the device. (dmex)
                    bestObjectName = PhGetPnPDeviceName(ObjectName);
                }
            }

            if (!bestObjectName)
            {
                // The file doesn't have a DOS filename and doesn't have a PnP friendly name.
                PhSetReference(&bestObjectName, ObjectName);
            }
        }

        if (PhIsNullOrEmptyString(bestObjectName) && (KphLevel() >= KphLevelMed))
        {
            KPH_FILE_OBJECT_DRIVER fileObjectDriver;
            PPH_STRING driverName;

            status = KphQueryInformationObject(
                ProcessHandle,
                Handle,
                KphObjectFileObjectDriver,
                &fileObjectDriver,
                sizeof(KPH_FILE_OBJECT_DRIVER),
                NULL
                );

            if (NT_SUCCESS(status) && fileObjectDriver.DriverHandle)
            {
                if (NT_SUCCESS(PhGetDriverName(fileObjectDriver.DriverHandle, &driverName)))
                {
                    static PH_STRINGREF prefix = PH_STRINGREF_INIT(L"Unnamed file: ");

                    PhMoveReference(&bestObjectName, PhConcatStringRef2(&prefix, &driverName->sr));
                    PhDereferenceObject(driverName);
                }

                NtClose(fileObjectDriver.DriverHandle);
            }
        }
    }
    else if (PhEqualString2(TypeName, L"Job", TRUE))
    {
        HANDLE dupHandle;
        PJOBOBJECT_BASIC_PROCESS_ID_LIST processIdList;

        // Skip when we already have a valid job object name. (dmex)
        if (!PhIsNullOrEmptyString(ObjectName))
            goto CleanupExit;

        status = NtDuplicateObject(
            ProcessHandle,
            Handle,
            NtCurrentProcess(),
            &dupHandle,
            JOB_OBJECT_QUERY,
            0,
            0
            );

        if (!NT_SUCCESS(status))
            goto CleanupExit;

        if (handleGetClientIdName && NT_SUCCESS(PhGetJobProcessIdList(dupHandle, &processIdList)))
        {
            PH_STRING_BUILDER sb;
            ULONG i;
            CLIENT_ID clientId;
            PPH_STRING name;

            PhInitializeStringBuilder(&sb, 40);
            clientId.UniqueThread = NULL;

            for (i = 0; i < processIdList->NumberOfProcessIdsInList; i++)
            {
                clientId.UniqueProcess = (HANDLE)processIdList->ProcessIdList[i];
                name = handleGetClientIdName(&clientId);

                if (name)
                {
                    PhAppendStringBuilder(&sb, &name->sr);
                    PhAppendStringBuilder2(&sb, L"; ");
                    PhDereferenceObject(name);
                }
            }

            PhFree(processIdList);

            if (sb.String->Length != 0)
                PhRemoveEndStringBuilder(&sb, 2);

            if (sb.String->Length == 0)
                PhAppendStringBuilder2(&sb, L"(No processes)");

            bestObjectName = PhFinalStringBuilderString(&sb);
        }

        NtClose(dupHandle);
    }
    else if (PhEqualString2(TypeName, L"Key", TRUE))
    {
        bestObjectName = PhFormatNativeKeyName(ObjectName);
    }
    else if (PhEqualString2(TypeName, L"Process", TRUE))
    {
        CLIENT_ID clientId;

        clientId.UniqueThread = NULL;

        if (KphLevel() >= KphLevelMed)
        {
            PROCESS_BASIC_INFORMATION basicInfo;

            status = KphQueryInformationObject(
                ProcessHandle,
                Handle,
                KphObjectProcessBasicInformation,
                &basicInfo,
                sizeof(PROCESS_BASIC_INFORMATION),
                NULL
                );

            if (!NT_SUCCESS(status))
                goto CleanupExit;

            clientId.UniqueProcess = basicInfo.UniqueProcessId;
        }
        else
        {
            HANDLE dupHandle;
            PROCESS_BASIC_INFORMATION basicInfo;

            status = NtDuplicateObject(
                ProcessHandle,
                Handle,
                NtCurrentProcess(),
                &dupHandle,
                PROCESS_QUERY_LIMITED_INFORMATION,
                0,
                0
                );

            if (!NT_SUCCESS(status))
                goto CleanupExit;

            status = PhGetProcessBasicInformation(dupHandle, &basicInfo);
            NtClose(dupHandle);

            if (!NT_SUCCESS(status))
                goto CleanupExit;

            clientId.UniqueProcess = basicInfo.UniqueProcessId;
        }

        if (handleGetClientIdName)
            bestObjectName = handleGetClientIdName(&clientId);
    }
    else if (PhEqualString2(TypeName, L"Section", TRUE))
    {
        HANDLE dupHandle = NULL;
        PPH_STRING fileName;

        if (!PhIsNullOrEmptyString(ObjectName))
            goto CleanupExit;

        if (KphLevel() >= KphLevelMed)
        {
            ULONG returnLength;
            ULONG bufferSize;
            PUNICODE_STRING buffer;

            bufferSize = 0x100;
            buffer = PhAllocate(bufferSize);

            status = KphQueryInformationObject(
                ProcessHandle,
                Handle,
                KphObjectSectionFileName,
                buffer,
                bufferSize,
                &returnLength
                );
            if (status == STATUS_BUFFER_OVERFLOW && returnLength > 0)
            {
                PhFree(buffer);
                bufferSize = returnLength;
                buffer = PhAllocate(bufferSize);

                status = KphQueryInformationObject(
                    ProcessHandle,
                    Handle,
                    KphObjectSectionFileName,
                    buffer,
                    bufferSize,
                    &returnLength
                    );
            }

            if (NT_SUCCESS(status))
                fileName = PhCreateStringFromUnicodeString(buffer);

            PhFree(buffer);
        }
        else
        {
            status = NtDuplicateObject(
                ProcessHandle,
                Handle,
                NtCurrentProcess(),
                &dupHandle,
                SECTION_QUERY | SECTION_MAP_READ,
                0,
                0
                );

            if (!NT_SUCCESS(status))
                goto CleanupExit;

            status = PhGetSectionFileName(dupHandle, &fileName);
        }

        if (NT_SUCCESS(status))
        {
            bestObjectName = PhResolveDevicePrefix(&fileName->sr);
            PhDereferenceObject(fileName);
        }
        else
        {
            SECTION_BASIC_INFORMATION basicInfo;

            if (KphLevel() >= KphLevelMed)
            {
                status = KphQueryInformationObject(
                    ProcessHandle,
                    Handle,
                    KphObjectSectionBasicInformation,
                    &basicInfo,
                    sizeof(basicInfo),
                    NULL
                    );
            }
            else if (dupHandle)
            {
                status = PhGetSectionBasicInformation(dupHandle, &basicInfo);
            }

            if (NT_SUCCESS(status))
            {
                PH_FORMAT format[4];
                PWSTR sectionType = L"Unknown";

                if (basicInfo.AllocationAttributes & SEC_COMMIT)
                    sectionType = L"Commit";
                else if (basicInfo.AllocationAttributes & SEC_FILE)
                    sectionType = L"File";
                else if (basicInfo.AllocationAttributes & SEC_IMAGE)
                    sectionType = L"Image";
                else if (basicInfo.AllocationAttributes & SEC_RESERVE)
                    sectionType = L"Reserve";

                PhInitFormatS(&format[0], sectionType);
                PhInitFormatS(&format[1], L" (");
                PhInitFormatSize(&format[2], basicInfo.MaximumSize.QuadPart);
                PhInitFormatC(&format[3], ')');
                bestObjectName = PhFormat(format, 4, 20);
            }
        }

        if (dupHandle)
            NtClose(dupHandle);
    }
    else if (PhEqualString2(TypeName, L"Thread", TRUE))
    {
        CLIENT_ID clientId;

        if (KphLevel() >= KphLevelMed)
        {
            THREAD_BASIC_INFORMATION basicInfo;

            status = KphQueryInformationObject(
                ProcessHandle,
                Handle,
                KphObjectThreadBasicInformation,
                &basicInfo,
                sizeof(THREAD_BASIC_INFORMATION),
                NULL
                );

            if (!NT_SUCCESS(status))
                goto CleanupExit;

            clientId = basicInfo.ClientId;
        }
        else
        {
            HANDLE dupHandle;
            THREAD_BASIC_INFORMATION basicInfo;

            status = NtDuplicateObject(
                ProcessHandle,
                Handle,
                NtCurrentProcess(),
                &dupHandle,
                THREAD_QUERY_LIMITED_INFORMATION,
                0,
                0
                );

            if (!NT_SUCCESS(status))
                goto CleanupExit;

            status = PhGetThreadBasicInformation(dupHandle, &basicInfo);
            NtClose(dupHandle);

            if (!NT_SUCCESS(status))
                goto CleanupExit;

            clientId = basicInfo.ClientId;
        }

        if (handleGetClientIdName)
            bestObjectName = handleGetClientIdName(&clientId);
    }
    else if (PhEqualString2(TypeName, L"TmEn", TRUE))
    {
        HANDLE dupHandle;
        ENLISTMENT_BASIC_INFORMATION basicInfo;

        status = NtDuplicateObject(
            ProcessHandle,
            Handle,
            NtCurrentProcess(),
            &dupHandle,
            ENLISTMENT_QUERY_INFORMATION,
            0,
            0
            );

        if (!NT_SUCCESS(status))
            goto CleanupExit;

        status = PhGetEnlistmentBasicInformation(dupHandle, &basicInfo);
        NtClose(dupHandle);

        if (NT_SUCCESS(status))
        {
            bestObjectName = PhFormatGuid(&basicInfo.EnlistmentId);
        }
    }
    else if (PhEqualString2(TypeName, L"TmRm", TRUE))
    {
        HANDLE dupHandle;
        GUID guid;
        PPH_STRING description;

        status = NtDuplicateObject(
            ProcessHandle,
            Handle,
            NtCurrentProcess(),
            &dupHandle,
            RESOURCEMANAGER_QUERY_INFORMATION,
            0,
            0
            );

        if (!NT_SUCCESS(status))
            goto CleanupExit;

        status = PhGetResourceManagerBasicInformation(
            dupHandle,
            &guid,
            &description
            );
        NtClose(dupHandle);

        if (NT_SUCCESS(status))
        {
            if (!PhIsNullOrEmptyString(description))
            {
                bestObjectName = description;
            }
            else
            {
                bestObjectName = PhFormatGuid(&guid);

                if (description)
                    PhDereferenceObject(description);
            }
        }
    }
    else if (PhEqualString2(TypeName, L"TmTm", TRUE))
    {
        HANDLE dupHandle;
        PPH_STRING logFileName = NULL;
        TRANSACTIONMANAGER_BASIC_INFORMATION basicInfo;

        status = NtDuplicateObject(
            ProcessHandle,
            Handle,
            NtCurrentProcess(),
            &dupHandle,
            TRANSACTIONMANAGER_QUERY_INFORMATION,
            0,
            0
            );

        if (!NT_SUCCESS(status))
            goto CleanupExit;

        status = PhGetTransactionManagerLogFileName(
            dupHandle,
            &logFileName
            );

        if (NT_SUCCESS(status) && !PhIsNullOrEmptyString(logFileName))
        {
            bestObjectName = PhGetFileName(logFileName);
            PhDereferenceObject(logFileName);
        }
        else
        {
            if (logFileName)
                PhDereferenceObject(logFileName);

            status = PhGetTransactionManagerBasicInformation(
                dupHandle,
                &basicInfo
                );

            if (NT_SUCCESS(status))
            {
                bestObjectName = PhFormatGuid(&basicInfo.TmIdentity);
            }
        }

        NtClose(dupHandle);
    }
    else if (PhEqualString2(TypeName, L"TmTx", TRUE))
    {
        HANDLE dupHandle;
        PPH_STRING description = NULL;
        TRANSACTION_BASIC_INFORMATION basicInfo;

        status = NtDuplicateObject(
            ProcessHandle,
            Handle,
            NtCurrentProcess(),
            &dupHandle,
            TRANSACTION_QUERY_INFORMATION,
            0,
            0
            );

        if (!NT_SUCCESS(status))
            goto CleanupExit;

        status = PhGetTransactionPropertiesInformation(
            dupHandle,
            NULL,
            NULL,
            &description
            );

        if (NT_SUCCESS(status) && !PhIsNullOrEmptyString(description))
        {
            bestObjectName = description;
        }
        else
        {
            if (description)
                PhDereferenceObject(description);

            status = PhGetTransactionBasicInformation(
                dupHandle,
                &basicInfo
                );

            if (NT_SUCCESS(status))
            {
                bestObjectName = PhFormatGuid(&basicInfo.TransactionId);
            }
        }

        NtClose(dupHandle);
    }
    else if (PhEqualString2(TypeName, L"Token", TRUE))
    {
        HANDLE dupHandle;
        PH_TOKEN_USER tokenUser = { 0 };
        TOKEN_STATISTICS statistics = { 0 };

        status = NtDuplicateObject(
            ProcessHandle,
            Handle,
            NtCurrentProcess(),
            &dupHandle,
            TOKEN_QUERY,
            0,
            0
            );

        if (!NT_SUCCESS(status))
            goto CleanupExit;

        status = PhGetTokenUser(dupHandle, &tokenUser);
        PhGetTokenStatistics(dupHandle, &statistics);

        if (NT_SUCCESS(status))
        {
            PPH_STRING fullName;

            fullName = PhGetSidFullName(tokenUser.User.Sid, TRUE, NULL);

            if (fullName)
            {
                PH_FORMAT format[4];

                PhInitFormatSR(&format[0], fullName->sr);
                PhInitFormatS(&format[1], L": 0x");
                PhInitFormatX(&format[2], statistics.AuthenticationId.LowPart);
                PhInitFormatS(&format[3], statistics.TokenType == TokenPrimary ? L" (Primary)" : L" (Impersonation)");

                bestObjectName = PhFormat(format, 4, fullName->Length + 8 + 16 + 16);
                PhDereferenceObject(fullName);
            }
        }

        NtClose(dupHandle);
    }
    else if (PhEqualString2(TypeName, L"ALPC Port", TRUE))
    {
        PROCESS_BASIC_INFORMATION processInfo;
        KPH_ALPC_COMMUNICATION_INFORMATION commsInfo;
        PKPH_ALPC_COMMUNICATION_NAMES_INFORMATION namesInfo;
        USHORT formatCount = 0;
        PH_FORMAT format[5];
        PPH_STRING name = NULL;
        CLIENT_ID clientId;

        if (KphLevel() < KphLevelMed)
            goto CleanupExit;

        status = PhGetProcessBasicInformation(ProcessHandle, &processInfo);
        if (!NT_SUCCESS(status))
            goto CleanupExit;

        status = KphAlpcQueryInformation(
            ProcessHandle,
            Handle,
            KphAlpcCommunicationInformation,
            &commsInfo,
            sizeof(commsInfo),
            NULL
            );
        if (!NT_SUCCESS(status))
            goto CleanupExit;

        if (!NT_SUCCESS(KphAlpcQueryComminicationsNamesInfo(ProcessHandle, Handle, &namesInfo)))
        {
            namesInfo = NULL;
        }

        if (commsInfo.ClientCommunicationPort.OwnerProcessId == processInfo.UniqueProcessId)
        {
            PhInitFormatS(&format[formatCount], L"Client: ");
            formatCount++;
            if (commsInfo.ServerCommunicationPort.OwnerProcessId)
            {
                PhInitFormatS(&format[formatCount], L"Connection to ");
                formatCount++;
                clientId.UniqueProcess = commsInfo.ServerCommunicationPort.OwnerProcessId;
                clientId.UniqueThread = 0;
                name = PhStdGetClientIdName(&clientId);
            }
            else if (commsInfo.ClientCommunicationPort.ConnectionRefused)
            {
                PhInitFormatS(&format[formatCount], L"Refused ");
                formatCount++;
            }
            else if (commsInfo.ClientCommunicationPort.Closed)
            {
                PhInitFormatS(&format[formatCount], L"Closed ");
                formatCount++;
            }
            else if (commsInfo.ClientCommunicationPort.Disconnected)
            {
                PhInitFormatS(&format[formatCount], L"Disconnected ");
                formatCount++;
            }
            else if (commsInfo.ClientCommunicationPort.ConnectionPending)
            {
                PhInitFormatS(&format[formatCount], L"Pending ");
                formatCount++;
            }
        }
        else if (commsInfo.ServerCommunicationPort.OwnerProcessId == processInfo.UniqueProcessId)
        {
            PhInitFormatS(&format[formatCount], L"Server: ");
            formatCount++;
            if (commsInfo.ClientCommunicationPort.OwnerProcessId)
            {
                PhInitFormatS(&format[formatCount], L" Connection from ");
                formatCount++;
                clientId.UniqueProcess = commsInfo.ClientCommunicationPort.OwnerProcessId;
                clientId.UniqueThread = 0;
                name = PhStdGetClientIdName(&clientId);
            }
            else if (commsInfo.ClientCommunicationPort.ConnectionRefused)
            {
                PhInitFormatS(&format[formatCount], L"Refused ");
                formatCount++;
            }
            else if (commsInfo.ServerCommunicationPort.Closed)
            {
                PhInitFormatS(&format[formatCount], L"Closed ");
                formatCount++;
            }
            else if (commsInfo.ServerCommunicationPort.Disconnected)
            {
                PhInitFormatS(&format[formatCount], L"Disconnected ");
                formatCount++;
            }
            else if (commsInfo.ServerCommunicationPort.ConnectionPending)
            {
                PhInitFormatS(&format[formatCount], L"Pending ");
                formatCount++;
            }
        }
        else if (commsInfo.ConnectionPort.OwnerProcessId == processInfo.UniqueProcessId)
        {
            PhInitFormatS(&format[formatCount], L"Connection: ");
            formatCount++;
        }

        if (name)
        {
            PhInitFormatSR(&format[formatCount], name->sr);
            formatCount++;

            if (namesInfo && namesInfo->ConnectionPort.Length > 0)
            {
                PhInitFormatS(&format[formatCount], L" on ");
                formatCount++;
            }
        }

        if (namesInfo && namesInfo->ConnectionPort.Length > 0)
        {
            PhInitFormatUCS(&format[formatCount], &namesInfo->ConnectionPort);
            formatCount++;
        }

        if (formatCount > 0)
            bestObjectName = PhFormat(format, formatCount, 0);

        if (name)
            PhDereferenceObject(name);

        if (namesInfo)
            PhFree(namesInfo);
    }

CleanupExit:

    if (!bestObjectName)
        PhSetReference(&bestObjectName, ObjectName);

    *BestObjectName = bestObjectName;

    return STATUS_SUCCESS;
}

/**
 * Gets information for a handle.
 *
 * \param ProcessHandle A handle to the process in which the handle resides.
 * \param Handle The handle value.
 * \param ObjectTypeNumber The object type number of the handle. You can specify -1 for this
 * parameter if the object type number is not known.
 * \param BasicInformation A variable which receives basic information about the object.
 * \param TypeName A variable which receives the object type name.
 * \param ObjectName A variable which receives the object name.
 * \param BestObjectName A variable which receives the formatted object name.
 *
 * \retval STATUS_INVALID_HANDLE The handle specified in \c ProcessHandle or \c Handle is invalid.
 * \retval STATUS_INVALID_PARAMETER_3 The value specified in \c ObjectTypeNumber is invalid.
 */
NTSTATUS PhGetHandleInformation(
    _In_ HANDLE ProcessHandle,
    _In_ HANDLE Handle,
    _In_ ULONG ObjectTypeNumber,
    _Out_opt_ POBJECT_BASIC_INFORMATION BasicInformation,
    _Out_opt_ PPH_STRING *TypeName,
    _Out_opt_ PPH_STRING *ObjectName,
    _Out_opt_ PPH_STRING *BestObjectName
    )
{
    NTSTATUS status;
    NTSTATUS subStatus;

    status = PhGetHandleInformationEx(
        ProcessHandle,
        Handle,
        ObjectTypeNumber,
        0,
        &subStatus,
        BasicInformation,
        TypeName,
        ObjectName,
        BestObjectName,
        NULL
        );

    if (!NT_SUCCESS(status))
        return status;

    // Fail if any component failed, for compatibility reasons.
    if (!NT_SUCCESS(subStatus))
    {
        if (TypeName)
            PhClearReference(TypeName);
        if (ObjectName)
            PhClearReference(ObjectName);
        if (BestObjectName)
            PhClearReference(BestObjectName);

        return subStatus;
    }

    return status;
}

/**
 * Gets information for a handle.
 *
 * \param ProcessHandle A handle to the process in which the handle resides.
 * \param Handle The handle value.
 * \param ObjectTypeNumber The object type number of the handle. You can specify -1 for this
 * parameter if the object type number is not known.
 * \param Flags Reserved.
 * \param SubStatus A variable which receives the NTSTATUS value of the last component that fails.
 * If all operations succeed, the value will be STATUS_SUCCESS. If the function returns an error
 * status, this variable is not set.
 * \param BasicInformation A variable which receives basic information about the object.
 * \param TypeName A variable which receives the object type name.
 * \param ObjectName A variable which receives the object name.
 * \param BestObjectName A variable which receives the formatted object name.
 * \param ExtraInformation Reserved.
 *
 * \retval STATUS_INVALID_HANDLE The handle specified in \c ProcessHandle or \c Handle is invalid.
 * \retval STATUS_INVALID_PARAMETER_3 The value specified in \c ObjectTypeNumber is invalid.
 *
 * \remarks If \a BasicInformation or \a TypeName are specified, the function will fail if either
 * cannot be queried. \a ObjectName, \a BestObjectName and \a ExtraInformation will be NULL if they
 * cannot be queried.
 */
NTSTATUS PhGetHandleInformationEx(
    _In_ HANDLE ProcessHandle,
    _In_ HANDLE Handle,
    _In_ ULONG ObjectTypeNumber,
    _Reserved_ ULONG Flags,
    _Out_opt_ PNTSTATUS SubStatus,
    _Out_opt_ POBJECT_BASIC_INFORMATION BasicInformation,
    _Out_opt_ PPH_STRING *TypeName,
    _Out_opt_ PPH_STRING *ObjectName,
    _Out_opt_ PPH_STRING *BestObjectName,
    _Reserved_ PVOID *ExtraInformation
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    NTSTATUS subStatus = STATUS_SUCCESS;
    HANDLE objectHandle = NULL;
    PPH_STRING typeName = NULL;
    PPH_STRING objectName = NULL;
    PPH_STRING bestObjectName = NULL;
    BOOLEAN useKph;

    if (ProcessHandle == NULL || Handle == NULL || Handle == NtCurrentProcess() || Handle == NtCurrentThread())
        return STATUS_INVALID_HANDLE;
    if (ObjectTypeNumber != ULONG_MAX && ObjectTypeNumber >= MAX_OBJECT_TYPE_NUMBER)
        return STATUS_INVALID_PARAMETER_3;

    useKph = (KphLevel() >= KphLevelMed);

    // Duplicate the handle if we're not using KPH.
    if (!useKph)
    {
        // However, we obviously don't need to duplicate it
        // if the handle is in the current process.
        if (ProcessHandle != NtCurrentProcess())
        {
            status = NtDuplicateObject(
                ProcessHandle,
                Handle,
                NtCurrentProcess(),
                &objectHandle,
                0,
                0,
                0
                );

            if (!NT_SUCCESS(status))
                return status;
        }
        else
        {
            objectHandle = Handle;
        }
    }
    else
    {
        objectHandle = Handle;
    }

    // Get basic information.
    if (BasicInformation)
    {
        status = PhpGetObjectBasicInformation(
            ProcessHandle,
            objectHandle,
            BasicInformation
            );

        if (!NT_SUCCESS(status))
            goto CleanupExit;
    }

    // Exit early if we don't need to get any other information.
    if (!TypeName && !ObjectName && !BestObjectName)
        goto CleanupExit;

    // Get the type name.
    status = PhpGetObjectTypeName(
        ProcessHandle,
        objectHandle,
        ObjectTypeNumber,
        &typeName
        );

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    // Exit early if we don't need to get the object name.
    if (!ObjectName && !BestObjectName)
        goto CleanupExit;

    // Get the object name.
    // If we're dealing with a file handle we must take special precautions so we don't hang.
    if (PhEqualString2(typeName, L"File", TRUE) && !useKph)
    {
        status = PhpGetObjectName(
            ProcessHandle,
            objectHandle,
            TRUE,
            &objectName
            );
    }
    else if (PhEqualString2(typeName, L"EtwRegistration", TRUE) && useKph)
    {
        status = PhpGetEtwObjectName(
            ProcessHandle,
            Handle,
            &objectName
            );
    }
    else
    {
        // Query the object normally.
        status = PhpGetObjectName(
            ProcessHandle,
            objectHandle,
            FALSE,
            &objectName
            );
    }

    if (!NT_SUCCESS(status))
    {
        if (PhEqualString2(typeName, L"File", TRUE) && useKph)
        {
            // PhpGetBestObjectName can provide us with a name.
            objectName = PhReferenceEmptyString();
            status = STATUS_SUCCESS;
        }
        else
        {
            subStatus = status;
            status = STATUS_SUCCESS;
            goto CleanupExit;
        }
    }

    // Exit early if we don't need to get the best object name.
    if (!BestObjectName)
        goto CleanupExit;

    status = PhpGetBestObjectName(
        ProcessHandle,
        Handle,
        objectName,
        typeName,
        &bestObjectName
        );

    if (!NT_SUCCESS(status))
    {
        subStatus = status;
        status = STATUS_SUCCESS;
        goto CleanupExit;
    }

CleanupExit:

    if (NT_SUCCESS(status))
    {
        if (SubStatus)
            *SubStatus = subStatus;
        if (TypeName)
            PhSetReference(TypeName, typeName);
        if (ObjectName)
            PhSetReference(ObjectName, objectName);
        if (BestObjectName)
            PhSetReference(BestObjectName, bestObjectName);
    }

    if (!useKph && objectHandle && ProcessHandle != NtCurrentProcess())
        NtClose(objectHandle);

    PhClearReference(&typeName);
    PhClearReference(&objectName);
    PhClearReference(&bestObjectName);

    return status;
}

NTSTATUS PhEnumObjectTypes(
    _Out_ POBJECT_TYPES_INFORMATION *ObjectTypes
    )
{
    NTSTATUS status;
    PVOID buffer;
    ULONG bufferSize;
    ULONG returnLength;

    bufferSize = 0x1000;
    buffer = PhAllocate(bufferSize);

    while ((status = NtQueryObject(
        NULL,
        ObjectTypesInformation,
        buffer,
        bufferSize,
        &returnLength
        )) == STATUS_INFO_LENGTH_MISMATCH)
    {
        PhFree(buffer);
        bufferSize *= 2;

        // Fail if we're resizing the buffer to something very large.
        if (bufferSize > PH_LARGE_BUFFER_SIZE)
            return STATUS_INSUFFICIENT_RESOURCES;

        buffer = PhAllocate(bufferSize);
    }

    if (!NT_SUCCESS(status))
    {
        PhFree(buffer);
        return status;
    }

    *ObjectTypes = (POBJECT_TYPES_INFORMATION)buffer;

    return status;
}

NTSTATUS PhGetObjectTypeMask(
    _In_ PPH_STRINGREF TypeName,
    _Out_ PGENERIC_MAPPING GenericMapping
    )
{
    NTSTATUS status = STATUS_NOT_FOUND;
    POBJECT_TYPES_INFORMATION objectTypes;
    POBJECT_TYPE_INFORMATION objectType;

    if (NT_SUCCESS(PhEnumObjectTypes(&objectTypes)))
    {
        objectType = PH_FIRST_OBJECT_TYPE(objectTypes);

        for (ULONG i = 0; i < objectTypes->NumberOfTypes; i++)
        {
            PH_STRINGREF typeName;

            PhUnicodeStringToStringRef(&objectType->TypeName, &typeName);

            if (PhEqualStringRef(&typeName, TypeName, TRUE))
            {
                *GenericMapping = objectType->GenericMapping;
                status = STATUS_SUCCESS;
                break;
            }

            objectType = PH_NEXT_OBJECT_TYPE(objectType);
        }

        PhFree(objectTypes);
    }

    return status;
}

ULONG PhGetObjectTypeNumber(
    _In_ PPH_STRINGREF TypeName
    )
{
    POBJECT_TYPES_INFORMATION objectTypes;
    POBJECT_TYPE_INFORMATION objectType;
    ULONG objectIndex = ULONG_MAX;
    ULONG i;

    if (NT_SUCCESS(PhEnumObjectTypes(&objectTypes)))
    {
        objectType = PH_FIRST_OBJECT_TYPE(objectTypes);

        for (i = 0; i < objectTypes->NumberOfTypes; i++)
        {
            PH_STRINGREF typeNameSr;

            PhUnicodeStringToStringRef(&objectType->TypeName, &typeNameSr);

            if (PhEqualStringRef(&typeNameSr, TypeName, TRUE))
            {
                if (WindowsVersion >= WINDOWS_8_1)
                {
                    objectIndex = objectType->TypeIndex;
                    break;
                }
                else
                {
                    objectIndex = i + 2;
                    break;
                }
            }

            objectType = PH_NEXT_OBJECT_TYPE(objectType);
        }

        PhFree(objectTypes);
    }

    return objectIndex;
}

PPH_STRING PhGetObjectTypeName(
    _In_ ULONG TypeIndex
    )
{
    static PH_INITONCE initOnce = PH_INITONCE_INIT;
    static POBJECT_TYPES_INFORMATION objectTypes = NULL;
    POBJECT_TYPE_INFORMATION objectType;
    PPH_STRING objectTypeName = NULL;
    ULONG i;

    if (PhBeginInitOnce(&initOnce))
    {
        PhEnumObjectTypes(&objectTypes);
        PhEndInitOnce(&initOnce);
    }

    if (objectTypes)
    {
        objectType = PH_FIRST_OBJECT_TYPE(objectTypes);

        for (i = 0; i < objectTypes->NumberOfTypes; i++)
        {
            if (WindowsVersion >= WINDOWS_8_1)
            {
                if (TypeIndex == objectType->TypeIndex)
                {
                    objectTypeName = PhCreateStringFromUnicodeString(&objectType->TypeName);
                    break;
                }
            }
            else
            {
                if (TypeIndex == (i + 2))
                {
                    objectTypeName = PhCreateStringFromUnicodeString(&objectType->TypeName);
                    break;
                }
            }

            objectType = PH_NEXT_OBJECT_TYPE(objectType);
        }
    }

    return objectTypeName;
}

PPHP_CALL_WITH_TIMEOUT_THREAD_CONTEXT PhpAcquireCallWithTimeoutThread(
    _In_opt_ PLARGE_INTEGER Timeout
    )
{
    static PH_INITONCE initOnce = PH_INITONCE_INIT;
    PPHP_CALL_WITH_TIMEOUT_THREAD_CONTEXT threadContext;
    PSLIST_ENTRY listEntry;
    PH_QUEUED_WAIT_BLOCK waitBlock;

    if (PhBeginInitOnce(&initOnce))
    {
        ULONG i;

        for (i = 0; i < PH_QUERY_HACK_MAX_THREADS; i++)
        {
            threadContext = PhAllocate(sizeof(PHP_CALL_WITH_TIMEOUT_THREAD_CONTEXT));
            memset(threadContext, 0, sizeof(PHP_CALL_WITH_TIMEOUT_THREAD_CONTEXT));
            RtlInterlockedPushEntrySList(&PhpCallWithTimeoutThreadListHead, &threadContext->ListEntry);
        }

        PhEndInitOnce(&initOnce);
    }

    while (TRUE)
    {
        if (listEntry = RtlInterlockedPopEntrySList(&PhpCallWithTimeoutThreadListHead))
            break;

        if (!Timeout || Timeout->QuadPart != 0)
        {
            PhQueueWakeEvent(&PhpCallWithTimeoutThreadReleaseEvent, &waitBlock);

            if (listEntry = RtlInterlockedPopEntrySList(&PhpCallWithTimeoutThreadListHead))
            {
                // A new entry has just become available; cancel the wait.
                PhSetWakeEvent(&PhpCallWithTimeoutThreadReleaseEvent, &waitBlock);
                break;
            }
            else
            {
                PhWaitForWakeEvent(&PhpCallWithTimeoutThreadReleaseEvent, &waitBlock, FALSE, Timeout);
                // TODO: Recompute the timeout value.
            }
        }
        else
        {
            return NULL;
        }
    }

    return CONTAINING_RECORD(listEntry, PHP_CALL_WITH_TIMEOUT_THREAD_CONTEXT, ListEntry);
}

VOID PhpReleaseCallWithTimeoutThread(
    _Inout_ PPHP_CALL_WITH_TIMEOUT_THREAD_CONTEXT ThreadContext
    )
{
    RtlInterlockedPushEntrySList(&PhpCallWithTimeoutThreadListHead, &ThreadContext->ListEntry);
    PhSetWakeEvent(&PhpCallWithTimeoutThreadReleaseEvent, NULL);
}

NTSTATUS PhpCallWithTimeout(
    _Inout_ PPHP_CALL_WITH_TIMEOUT_THREAD_CONTEXT ThreadContext,
    _In_ PUSER_THREAD_START_ROUTINE Routine,
    _In_opt_ PVOID Context,
    _In_ PLARGE_INTEGER Timeout
    )
{
    NTSTATUS status;

    // Create objects if necessary.

    if (!ThreadContext->StartEventHandle)
    {
        if (!NT_SUCCESS(status = NtCreateEvent(&ThreadContext->StartEventHandle, EVENT_ALL_ACCESS, NULL, SynchronizationEvent, FALSE)))
            return status;
    }

    if (!ThreadContext->CompletedEventHandle)
    {
        if (!NT_SUCCESS(status = NtCreateEvent(&ThreadContext->CompletedEventHandle, EVENT_ALL_ACCESS, NULL, SynchronizationEvent, FALSE)))
            return status;
    }

    // Create a query thread if we don't have one.
    if (!ThreadContext->ThreadHandle)
    {
        CLIENT_ID clientId;

        NtClearEvent(ThreadContext->StartEventHandle);
        NtClearEvent(ThreadContext->CompletedEventHandle);

        if (!NT_SUCCESS(status = PhCreateUserThread(
            NtCurrentProcess(),
            NULL,
            0,
            0,
            UInt32x32To64(32, 1024),
            0,
            PhpCallWithTimeoutThreadStart,
            ThreadContext,
            &ThreadContext->ThreadHandle,
            &clientId
            )))
        {
            return status;
        }

        // Wait for the thread to initialize.
        NtWaitForSingleObject(ThreadContext->CompletedEventHandle, FALSE, NULL);
    }

    ThreadContext->Routine = Routine;
    ThreadContext->Context = Context;

    NtSetEvent(ThreadContext->StartEventHandle, NULL);
    status = NtWaitForSingleObject(ThreadContext->CompletedEventHandle, FALSE, Timeout);

    ThreadContext->Routine = NULL;
    MemoryBarrier();
    ThreadContext->Context = NULL;

    if (status != STATUS_WAIT_0)
    {
        // The operation timed out, or there was an error. Kill the thread. On Vista and above, the
        // thread stack is freed automatically.
        NtTerminateThread(ThreadContext->ThreadHandle, STATUS_UNSUCCESSFUL);
        status = NtWaitForSingleObject(ThreadContext->ThreadHandle, FALSE, NULL);
        NtClose(ThreadContext->ThreadHandle);
        ThreadContext->ThreadHandle = NULL;

        status = STATUS_UNSUCCESSFUL;
    }

    return status;
}

NTSTATUS PhpCallWithTimeoutThreadStart(
    _In_ PVOID Parameter
    )
{
    PPHP_CALL_WITH_TIMEOUT_THREAD_CONTEXT threadContext = Parameter;

    NtSetEvent(threadContext->CompletedEventHandle, NULL);

    while (TRUE)
    {
        if (NtWaitForSingleObject(threadContext->StartEventHandle, FALSE, NULL) != STATUS_WAIT_0)
            continue;

        if (threadContext->Routine)
            threadContext->Routine(threadContext->Context);

        NtSetEvent(threadContext->CompletedEventHandle, NULL);
    }

    return STATUS_SUCCESS;
}

NTSTATUS PhCallWithTimeout(
    _In_ PUSER_THREAD_START_ROUTINE Routine,
    _In_opt_ PVOID Context,
    _In_opt_ PLARGE_INTEGER AcquireTimeout,
    _In_ PLARGE_INTEGER CallTimeout
    )
{
    NTSTATUS status;
    PPHP_CALL_WITH_TIMEOUT_THREAD_CONTEXT threadContext;

    if (threadContext = PhpAcquireCallWithTimeoutThread(AcquireTimeout))
    {
        status = PhpCallWithTimeout(threadContext, Routine, Context, CallTimeout);
        PhpReleaseCallWithTimeoutThread(threadContext);
    }
    else
    {
        status = STATUS_UNSUCCESSFUL;
    }

    return status;
}

NTSTATUS PhpCommonQueryObjectRoutine(
    _In_ PVOID Parameter
    )
{
    PPHP_QUERY_OBJECT_COMMON_CONTEXT context = Parameter;
    IO_STATUS_BLOCK isb;

    switch (context->Work)
    {
    case NtQueryObjectWork:
        context->Status = NtQueryObject(
            context->u.NtQueryObject.Handle,
            context->u.NtQueryObject.ObjectInformationClass,
            context->u.NtQueryObject.ObjectInformation,
            context->u.NtQueryObject.ObjectInformationLength,
            context->u.NtQueryObject.ReturnLength
            );
        break;
    case NtQuerySecurityObjectWork:
        context->Status = NtQuerySecurityObject(
            context->u.NtQuerySecurityObject.Handle,
            context->u.NtQuerySecurityObject.SecurityInformation,
            context->u.NtQuerySecurityObject.SecurityDescriptor,
            context->u.NtQuerySecurityObject.Length,
            context->u.NtQuerySecurityObject.LengthNeeded
            );
        break;
    case NtSetSecurityObjectWork:
        context->Status = NtSetSecurityObject(
            context->u.NtSetSecurityObject.Handle,
            context->u.NtSetSecurityObject.SecurityInformation,
            context->u.NtSetSecurityObject.SecurityDescriptor
            );
        break;
    case NtQueryFileInformationWork:
        {
            context->Status = NtQueryInformationFile(
                context->u.NtQueryFileInformation.Handle,
                &isb,
                context->u.NtQueryFileInformation.FileInformation,
                context->u.NtQueryFileInformation.FileInformationLength,
                context->u.NtQueryFileInformation.FileInformationClass
                );
        }
        break;
    case KphQueryFileInformationWork:
        {
            context->Status = KphQueryInformationFile(
                context->u.KphQueryFileInformation.ProcessHandle,
                context->u.KphQueryFileInformation.Handle,
                context->u.KphQueryFileInformation.FileInformationClass,
                context->u.KphQueryFileInformation.FileInformation,
                context->u.KphQueryFileInformation.FileInformationLength,
                &isb
                );
        }
        break;
    default:
        context->Status = STATUS_INVALID_PARAMETER;
        break;
    }

    return STATUS_SUCCESS;
}

NTSTATUS PhpCommonQueryObjectWithTimeout(
    _In_ PPHP_QUERY_OBJECT_COMMON_CONTEXT Context
    )
{
    NTSTATUS status;
    LARGE_INTEGER timeout;

    timeout.QuadPart = -(LONGLONG)UInt32x32To64(1, PH_TIMEOUT_SEC);
    status = PhCallWithTimeout(PhpCommonQueryObjectRoutine, Context, NULL, &timeout);

    if (NT_SUCCESS(status))
        status = Context->Status;

    PhFree(Context);

    return status;
}

NTSTATUS PhCallNtQueryObjectWithTimeout(
    _In_ HANDLE Handle,
    _In_ OBJECT_INFORMATION_CLASS ObjectInformationClass,
    _Out_writes_bytes_opt_(ObjectInformationLength) PVOID ObjectInformation,
    _In_ ULONG ObjectInformationLength,
    _Out_opt_ PULONG ReturnLength
    )
{
    PPHP_QUERY_OBJECT_COMMON_CONTEXT context;

    context = PhAllocate(sizeof(PHP_QUERY_OBJECT_COMMON_CONTEXT));
    context->Work = NtQueryObjectWork;
    context->Status = STATUS_UNSUCCESSFUL;
    context->u.NtQueryObject.Handle = Handle;
    context->u.NtQueryObject.ObjectInformationClass = ObjectInformationClass;
    context->u.NtQueryObject.ObjectInformation = ObjectInformation;
    context->u.NtQueryObject.ObjectInformationLength = ObjectInformationLength;
    context->u.NtQueryObject.ReturnLength = ReturnLength;

    return PhpCommonQueryObjectWithTimeout(context);
}

NTSTATUS PhCallNtQuerySecurityObjectWithTimeout(
    _In_ HANDLE Handle,
    _In_ SECURITY_INFORMATION SecurityInformation,
    _Out_writes_bytes_opt_(Length) PSECURITY_DESCRIPTOR SecurityDescriptor,
    _In_ ULONG Length,
    _Out_ PULONG LengthNeeded
    )
{
    PPHP_QUERY_OBJECT_COMMON_CONTEXT context;

    context = PhAllocate(sizeof(PHP_QUERY_OBJECT_COMMON_CONTEXT));
    context->Work = NtQuerySecurityObjectWork;
    context->Status = STATUS_UNSUCCESSFUL;
    context->u.NtQuerySecurityObject.Handle = Handle;
    context->u.NtQuerySecurityObject.SecurityInformation = SecurityInformation;
    context->u.NtQuerySecurityObject.SecurityDescriptor = SecurityDescriptor;
    context->u.NtQuerySecurityObject.Length = Length;
    context->u.NtQuerySecurityObject.LengthNeeded = LengthNeeded;

    return PhpCommonQueryObjectWithTimeout(context);
}

NTSTATUS PhCallNtSetSecurityObjectWithTimeout(
    _In_ HANDLE Handle,
    _In_ SECURITY_INFORMATION SecurityInformation,
    _In_ PSECURITY_DESCRIPTOR SecurityDescriptor
    )
{
    PPHP_QUERY_OBJECT_COMMON_CONTEXT context;

    context = PhAllocate(sizeof(PHP_QUERY_OBJECT_COMMON_CONTEXT));
    context->Work = NtSetSecurityObjectWork;
    context->Status = STATUS_UNSUCCESSFUL;
    context->u.NtSetSecurityObject.Handle = Handle;
    context->u.NtSetSecurityObject.SecurityInformation = SecurityInformation;
    context->u.NtSetSecurityObject.SecurityDescriptor = SecurityDescriptor;

    return PhpCommonQueryObjectWithTimeout(context);
}

NTSTATUS PhCallNtQueryFileInformationWithTimeout(
    _In_ HANDLE Handle,
    _In_ FILE_INFORMATION_CLASS FileInformationClass,
    _Out_writes_bytes_opt_(FileInformationLength) PVOID FileInformation,
    _In_ ULONG FileInformationLength
    )
{
    PPHP_QUERY_OBJECT_COMMON_CONTEXT context;

    context = PhAllocate(sizeof(PHP_QUERY_OBJECT_COMMON_CONTEXT));
    context->Work = NtQueryFileInformationWork;
    context->Status = STATUS_UNSUCCESSFUL;
    context->u.NtQueryFileInformation.Handle = Handle;
    context->u.NtQueryFileInformation.FileInformationClass = FileInformationClass;
    context->u.NtQueryFileInformation.FileInformation = FileInformation;
    context->u.NtQueryFileInformation.FileInformationLength = FileInformationLength;

    return PhpCommonQueryObjectWithTimeout(context);
}

NTSTATUS PhCallKphQueryFileInformationWithTimeout(
    _In_ HANDLE ProcessHandle,
    _In_ HANDLE Handle,
    _In_ FILE_INFORMATION_CLASS FileInformationClass,
    _Out_writes_bytes_opt_(FileInformationLength) PVOID FileInformation,
    _In_ ULONG FileInformationLength
    )
{
    PPHP_QUERY_OBJECT_COMMON_CONTEXT context;

    context = PhAllocate(sizeof(PHP_QUERY_OBJECT_COMMON_CONTEXT));
    context->Work = KphQueryFileInformationWork;
    context->Status = STATUS_UNSUCCESSFUL;
    context->u.KphQueryFileInformation.ProcessHandle = ProcessHandle;
    context->u.KphQueryFileInformation.Handle = Handle;
    context->u.KphQueryFileInformation.FileInformationClass = FileInformationClass;
    context->u.KphQueryFileInformation.FileInformation = FileInformation;
    context->u.KphQueryFileInformation.FileInformationLength = FileInformationLength;

    return PhpCommonQueryObjectWithTimeout(context);
}

