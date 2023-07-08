/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     jxy-s   2022-2023
 *
 */

#include <phapp.h>
#include <phplug.h>
#include <settings.h>

#include <devprv.h>

#include <SetupAPI.h>
#include <cfgmgr32.h>
#include <wdmguid.h>

#undef DEFINE_GUID
#include <pciprop.h>
#include <ntddstor.h>

static PH_STRINGREF RootInstanceId = PH_STRINGREF_INIT(L"HTREE\\ROOT\\0");
PPH_OBJECT_TYPE PhDeviceTreeType = NULL;
PPH_OBJECT_TYPE PhDeviceItemType = NULL;
PPH_OBJECT_TYPE PhDeviceNotifyType = NULL;
static PPH_OBJECT_TYPE PhpDeviceInfoType = NULL;
static PPH_DEVICE_TREE PhpDeviceTree = NULL;
static PH_FAST_LOCK PhpDeviceTreeLock = PH_FAST_LOCK_INIT;
static HCMNOTIFICATION PhpDeviceNotification = NULL;
static HCMNOTIFICATION PhpDeviceInterfaceNotification = NULL;
static PH_FAST_LOCK PhpDeviceNotifyLock = PH_FAST_LOCK_INIT;
static HANDLE PhpDeviceNotifyEvent = NULL;
static LIST_ENTRY PhpDeviceNotifyList = { 0 };

#if !defined(NTDDI_WIN10_NI) || (NTDDI_VERSION < NTDDI_WIN10_NI)
// Note: This propkey is required for building with 22H1 and older Windows SDK (dmex)
DEFINE_DEVPROPKEY(DEVPKEY_Device_FirmwareVendor, 0x540b947e, 0x8b40, 0x45bc, 0xa8, 0xa2, 0x6a, 0x0b, 0x89, 0x4c, 0xbd, 0xa2, 26);   // DEVPROP_TYPE_STRING
#endif

#define DEVPROP_FILL_FLAG_CLASS_INTERFACE 0x00000001
#define DEVPROP_FILL_FLAG_CLASS_INSTALLER 0x00000002

_Function_class_(PH_DEVICE_PROPERTY_FILL_CALLBACK)
typedef
VOID
NTAPI
PH_DEVICE_PROPERTY_FILL_CALLBACK(
    _In_ HDEVINFO DeviceInfoSet,
    _In_ PSP_DEVINFO_DATA DeviceInfoData,
    _In_ const DEVPROPKEY* PropertyKey,
    _Out_ PPH_DEVICE_PROPERTY Property,
    _In_ ULONG Flags
    );
typedef PH_DEVICE_PROPERTY_FILL_CALLBACK* PPH_DEVICE_PROPERTY_FILL_CALLBACK;

typedef struct _PH_DEVICE_PROPERTY_TABLE_ENTRY
{
    PH_DEVICE_PROPERTY_CLASS PropClass;
    const DEVPROPKEY* PropKey;
    PPH_DEVICE_PROPERTY_FILL_CALLBACK Callback;
    ULONG CallbackFlags;
} PH_DEVICE_PROPERTY_TABLE_ENTRY, *PPH_DEVICE_PROPERTY_TABLE_ENTRY;

BOOLEAN PhpGetDevicePropertyGuid(
    _In_ HDEVINFO DeviceInfoSet,
    _In_ PSP_DEVINFO_DATA DeviceInfoData,
    _In_ const DEVPROPKEY* DeviceProperty,
    _Out_ PGUID Guid
    )
{
    BOOL result;
    DEVPROPTYPE devicePropertyType = DEVPROP_TYPE_EMPTY;
    ULONG requiredLength = sizeof(GUID);

    result = SetupDiGetDevicePropertyW(
        DeviceInfoSet,
        DeviceInfoData,
        DeviceProperty,
        &devicePropertyType,
        (PBYTE)Guid,
        sizeof(GUID),
        &requiredLength,
        0
        );
    if (result && (devicePropertyType == DEVPROP_TYPE_GUID))
    {
        return TRUE;
    }

    RtlZeroMemory(Guid, sizeof(GUID));

    return FALSE;
}

BOOLEAN PhpGetClassPropertyGuid(
    _In_ const GUID* ClassGuid,
    _In_ const DEVPROPKEY* DeviceProperty,
    _In_ ULONG Flags,
    _Out_ PGUID Guid
    )
{
    BOOL result;
    DEVPROPTYPE devicePropertyType = DEVPROP_TYPE_EMPTY;
    ULONG requiredLength = sizeof(GUID);

    result = SetupDiGetClassPropertyW(
        ClassGuid,
        DeviceProperty,
        &devicePropertyType,
        (PBYTE)Guid,
        sizeof(GUID),
        &requiredLength,
        Flags
        );
    if (result && (devicePropertyType == DEVPROP_TYPE_GUID))
    {
        return TRUE;
    }

    RtlZeroMemory(Guid, sizeof(GUID));

    return FALSE;
}

BOOLEAN PhpGetDevicePropertyUInt64(
    _In_ HDEVINFO DeviceInfoSet,
    _In_ PSP_DEVINFO_DATA DeviceInfoData,
    _In_ const DEVPROPKEY* DeviceProperty,
    _Out_ PULONG64 Value
    )
{
    BOOL result;
    DEVPROPTYPE devicePropertyType = DEVPROP_TYPE_EMPTY;
    ULONG requiredLength = sizeof(ULONG64);

    result = SetupDiGetDevicePropertyW(
        DeviceInfoSet,
        DeviceInfoData,
        DeviceProperty,
        &devicePropertyType,
        (PBYTE)Value,
        sizeof(ULONG64),
        &requiredLength,
        0
        );
    if (result && (devicePropertyType == DEVPROP_TYPE_UINT64))
    {
        return TRUE;
    }

    *Value = 0;

    return FALSE;
}

BOOLEAN PhpGetClassPropertyUInt64(
    _In_ const GUID* ClassGuid,
    _In_ const DEVPROPKEY* DeviceProperty,
    _In_ ULONG Flags,
    _Out_ PULONG64 Value
    )
{
    BOOL result;
    DEVPROPTYPE devicePropertyType = DEVPROP_TYPE_EMPTY;
    ULONG requiredLength = sizeof(ULONG64);

    result = SetupDiGetClassPropertyW(
        ClassGuid,
        DeviceProperty,
        &devicePropertyType,
        (PBYTE)Value,
        sizeof(ULONG64),
        &requiredLength,
        Flags
        );
    if (result && (devicePropertyType == DEVPROP_TYPE_UINT64))
    {
        return TRUE;
    }

    *Value = 0;

    return FALSE;
}

BOOLEAN PhpGetDevicePropertyUInt32(
    _In_ HDEVINFO DeviceInfoSet,
    _In_ PSP_DEVINFO_DATA DeviceInfoData,
    _In_ const DEVPROPKEY* DeviceProperty,
    _Out_ PULONG Value
    )
{
    BOOL result;
    DEVPROPTYPE devicePropertyType = DEVPROP_TYPE_EMPTY;
    ULONG requiredLength = sizeof(ULONG);

    result = SetupDiGetDevicePropertyW(
        DeviceInfoSet,
        DeviceInfoData,
        DeviceProperty,
        &devicePropertyType,
        (PBYTE)Value,
        sizeof(ULONG),
        &requiredLength,
        0
        );
    if (result && (devicePropertyType == DEVPROP_TYPE_UINT32))
    {
        return TRUE;
    }

    *Value = 0;

    return FALSE;
}

BOOLEAN PhpGetClassPropertyUInt32(
    _In_ const GUID* ClassGuid,
    _In_ const DEVPROPKEY* DeviceProperty,
    _In_ ULONG Flags,
    _Out_ PULONG Value
    )
{
    BOOL result;
    DEVPROPTYPE devicePropertyType = DEVPROP_TYPE_EMPTY;
    ULONG requiredLength = sizeof(ULONG);

    result = SetupDiGetClassPropertyW(
        ClassGuid,
        DeviceProperty,
        &devicePropertyType,
        (PBYTE)Value,
        sizeof(ULONG),
        &requiredLength,
        Flags
        );
    if (result && (devicePropertyType == DEVPROP_TYPE_UINT32))
    {
        return TRUE;
    }

    *Value = 0;

    return FALSE;
}

BOOLEAN PhpGetDevicePropertyInt32(
    _In_ HDEVINFO DeviceInfoSet,
    _In_ PSP_DEVINFO_DATA DeviceInfoData,
    _In_ const DEVPROPKEY* DeviceProperty,
    _Out_ PLONG Value
    )
{
    BOOL result;
    DEVPROPTYPE devicePropertyType = DEVPROP_TYPE_EMPTY;
    ULONG requiredLength = sizeof(LONG);

    result = SetupDiGetDevicePropertyW(
        DeviceInfoSet,
        DeviceInfoData,
        DeviceProperty,
        &devicePropertyType,
        (PBYTE)Value,
        sizeof(LONG),
        &requiredLength,
        0
        );
    if (result && (devicePropertyType == DEVPROP_TYPE_INT32))
    {
        return TRUE;
    }

    *Value = 0;

    return FALSE;
}

BOOLEAN PhpGetClassPropertyInt32(
    _In_ const GUID* ClassGuid,
    _In_ const DEVPROPKEY* DeviceProperty,
    _In_ ULONG Flags,
    _Out_ PLONG Value
    )
{
    BOOL result;
    DEVPROPTYPE devicePropertyType = DEVPROP_TYPE_EMPTY;
    ULONG requiredLength = sizeof(LONG);

    result = SetupDiGetClassPropertyW(
        ClassGuid,
        DeviceProperty,
        &devicePropertyType,
        (PBYTE)Value,
        sizeof(LONG),
        &requiredLength,
        Flags
        );
    if (result && (devicePropertyType == DEVPROP_TYPE_INT32))
    {
        return TRUE;
    }

    *Value = 0;

    return FALSE;
}

BOOLEAN PhpGetDevicePropertyNTSTATUS(
    _In_ HDEVINFO DeviceInfoSet,
    _In_ PSP_DEVINFO_DATA DeviceInfoData,
    _In_ const DEVPROPKEY* DeviceProperty,
    _Out_ PNTSTATUS Status
    )
{
    BOOL result;
    DEVPROPTYPE devicePropertyType = DEVPROP_TYPE_EMPTY;
    ULONG requiredLength = sizeof(NTSTATUS);

    result = SetupDiGetDevicePropertyW(
        DeviceInfoSet,
        DeviceInfoData,
        DeviceProperty,
        &devicePropertyType,
        (PBYTE)Status,
        sizeof(NTSTATUS),
        &requiredLength,
        0
        );
    if (result && (devicePropertyType == DEVPROP_TYPE_NTSTATUS))
    {
        return TRUE;
    }

    *Status = 0;

    return FALSE;
}

BOOLEAN PhpGetClassPropertyNTSTATUS(
    _In_ const GUID* ClassGuid,
    _In_ const DEVPROPKEY* DeviceProperty,
    _In_ ULONG Flags,
    _Out_ PNTSTATUS Status
    )
{
    BOOL result;
    DEVPROPTYPE devicePropertyType = DEVPROP_TYPE_EMPTY;
    ULONG requiredLength = sizeof(NTSTATUS);

    result = SetupDiGetClassPropertyW(
        ClassGuid,
        DeviceProperty,
        &devicePropertyType,
        (PBYTE)Status,
        sizeof(NTSTATUS),
        &requiredLength,
        Flags
        );
    if (result && (devicePropertyType == DEVPROP_TYPE_NTSTATUS))
    {
        return TRUE;
    }

    *Status = 0;

    return FALSE;
}

BOOLEAN PhpGetDevicePropertyBoolean(
    _In_ HDEVINFO DeviceInfoSet,
    _In_ PSP_DEVINFO_DATA DeviceInfoData,
    _In_ const DEVPROPKEY* DeviceProperty,
    _Out_ PBOOLEAN Boolean
    )
{
    BOOL result;
    DEVPROPTYPE devicePropertyType = DEVPROP_TYPE_EMPTY;
    DEVPROP_BOOLEAN boolean;
    ULONG requiredLength = sizeof(DEVPROP_BOOLEAN);

    result = SetupDiGetDevicePropertyW(
        DeviceInfoSet,
        DeviceInfoData,
        DeviceProperty,
        &devicePropertyType,
        (PBYTE)&boolean,
        sizeof(DEVPROP_BOOLEAN),
        &requiredLength,
        0
        );
    if (result && (devicePropertyType == DEVPROP_TYPE_BOOLEAN))
    {
        *Boolean = !!boolean;

        return TRUE;
    }

    *Boolean = FALSE;

    return FALSE;
}

BOOLEAN PhpGetClassPropertyBoolean(
    _In_ const GUID* ClassGuid,
    _In_ const DEVPROPKEY* DeviceProperty,
    _In_ ULONG Flags,
    _Out_ PBOOLEAN Boolean
    )
{
    BOOL result;
    DEVPROPTYPE devicePropertyType = DEVPROP_TYPE_EMPTY;
    DEVPROP_BOOLEAN boolean;
    ULONG requiredLength = sizeof(DEVPROP_BOOLEAN);

    result = SetupDiGetClassPropertyW(
        ClassGuid,
        DeviceProperty,
        &devicePropertyType,
        (PBYTE)&boolean,
        sizeof(DEVPROP_BOOLEAN),
        &requiredLength,
        Flags
        );
    if (result && (devicePropertyType == DEVPROP_TYPE_BOOLEAN))
    {
        *Boolean = !!boolean;

        return TRUE;
    }

    *Boolean = FALSE;

    return FALSE;
}

BOOLEAN PhpGetDevicePropertyTimeStamp(
    _In_ HDEVINFO DeviceInfoSet,
    _In_ PSP_DEVINFO_DATA DeviceInfoData,
    _In_ const DEVPROPKEY* DeviceProperty,
    _Out_ PLARGE_INTEGER TimeStamp
    )
{
    BOOL result;
    DEVPROPTYPE devicePropertyType = DEVPROP_TYPE_EMPTY;
    FILETIME fileTime;
    ULONG requiredLength = sizeof(FILETIME);

    result = SetupDiGetDevicePropertyW(
        DeviceInfoSet,
        DeviceInfoData,
        DeviceProperty,
        &devicePropertyType,
        (PBYTE)&fileTime,
        sizeof(FILETIME),
        &requiredLength,
        0
        );
    if (result && (devicePropertyType == DEVPROP_TYPE_FILETIME))
    {
        TimeStamp->HighPart = fileTime.dwHighDateTime;
        TimeStamp->LowPart = fileTime.dwLowDateTime;

        return TRUE;
    }

    TimeStamp->QuadPart = 0;

    return FALSE;
}

BOOLEAN PhpGetClassPropertyTimeStamp(
    _In_ const GUID* ClassGuid,
    _In_ const DEVPROPKEY* DeviceProperty,
    _In_ ULONG Flags,
    _Out_ PLARGE_INTEGER TimeStamp
    )
{
    BOOL result;
    DEVPROPTYPE devicePropertyType = DEVPROP_TYPE_EMPTY;
    FILETIME fileTime;
    ULONG requiredLength = sizeof(FILETIME);

    result = SetupDiGetClassPropertyW(
        ClassGuid,
        DeviceProperty,
        &devicePropertyType,
        (PBYTE)&fileTime,
        sizeof(FILETIME),
        &requiredLength,
        Flags
        );
    if (result && (devicePropertyType == DEVPROP_TYPE_FILETIME))
    {
        TimeStamp->HighPart = fileTime.dwHighDateTime;
        TimeStamp->LowPart = fileTime.dwLowDateTime;

        return TRUE;
    }

    TimeStamp->QuadPart = 0;

    return FALSE;
}

BOOLEAN PhpGetDevicePropertyString(
    _In_ HDEVINFO DeviceInfoSet,
    _In_ PSP_DEVINFO_DATA DeviceInfoData,
    _In_ const DEVPROPKEY* DeviceProperty,
    _Out_ PPH_STRING* String
    )
{
    BOOL result;
    DEVPROPTYPE devicePropertyType = DEVPROP_TYPE_EMPTY;
    ULONG requiredLength = 0;
    PVOID buffer = NULL;

    *String = NULL;

    result = SetupDiGetDevicePropertyW(
        DeviceInfoSet,
        DeviceInfoData,
        DeviceProperty,
        &devicePropertyType,
        NULL,
        0,
        &requiredLength,
        0
        );
    if (result ||
        (requiredLength == 0) ||
        (GetLastError() != ERROR_INSUFFICIENT_BUFFER) ||
        ((devicePropertyType != DEVPROP_TYPE_STRING) &&
         (devicePropertyType != DEVPROP_TYPE_SECURITY_DESCRIPTOR_STRING)))
    {
        goto Exit;
    }

    buffer = PhAllocate(requiredLength);

    result = SetupDiGetDevicePropertyW(
        DeviceInfoSet,
        DeviceInfoData,
        DeviceProperty,
        &devicePropertyType,
        buffer,
        requiredLength,
        &requiredLength,
        0
        );
    if (!result)
    {
        goto Exit;
    }

    *String = PhCreateString(buffer);

Exit:

    if (buffer)
        PhFree(buffer);

    return !!result;
}

BOOLEAN PhpGetClassPropertyString(
    _In_ const GUID* ClassGuid,
    _In_ const DEVPROPKEY* DeviceProperty,
    _In_ ULONG Flags,
    _Out_ PPH_STRING* String
    )
{
    BOOL result;
    DEVPROPTYPE devicePropertyType = DEVPROP_TYPE_EMPTY;
    ULONG requiredLength = 0;
    PVOID buffer = NULL;

    *String = NULL;

    result = SetupDiGetClassPropertyW(
        ClassGuid,
        DeviceProperty,
        &devicePropertyType,
        NULL,
        0,
        &requiredLength,
        Flags
        );
    if (result ||
        (requiredLength == 0) ||
        (GetLastError() != ERROR_INSUFFICIENT_BUFFER) ||
        ((devicePropertyType != DEVPROP_TYPE_STRING) &&
         (devicePropertyType != DEVPROP_TYPE_SECURITY_DESCRIPTOR_STRING)))
    {
        goto Exit;
    }

    buffer = PhAllocate(requiredLength);

    result = SetupDiGetClassPropertyW(
        ClassGuid,
        DeviceProperty,
        &devicePropertyType,
        buffer,
        requiredLength,
        &requiredLength,
        Flags
        );
    if (!result)
    {
        goto Exit;
    }

    *String = PhCreateString(buffer);

Exit:

    if (buffer)
        PhFree(buffer);

    return !!result;
}

BOOLEAN PhpGetDevicePropertyStringList(
    _In_ HDEVINFO DeviceInfoSet,
    _In_ PSP_DEVINFO_DATA DeviceInfoData,
    _In_ const DEVPROPKEY* DeviceProperty,
    _Out_ PPH_LIST* StringList
    )
{
    BOOL result;
    DEVPROPTYPE devicePropertyType = DEVPROP_TYPE_EMPTY;
    ULONG requiredLength = 0;
    PVOID buffer = NULL;
    PPH_LIST stringList;

    *StringList = NULL;

    result = SetupDiGetDevicePropertyW(
        DeviceInfoSet,
        DeviceInfoData,
        DeviceProperty,
        &devicePropertyType,
        NULL,
        0,
        &requiredLength,
        0
        );
    if (result ||
        (requiredLength == 0) ||
        (GetLastError() != ERROR_INSUFFICIENT_BUFFER) ||
        (devicePropertyType != DEVPROP_TYPE_STRING_LIST))
    {
        goto Exit;
    }

    buffer = PhAllocate(requiredLength);

    result = SetupDiGetDevicePropertyW(
        DeviceInfoSet,
        DeviceInfoData,
        DeviceProperty,
        &devicePropertyType,
        buffer,
        requiredLength,
        &requiredLength,
        0
        );
    if (!result)
    {
        goto Exit;
    }

    stringList = PhCreateList(1);

    for (PZZWSTR item = buffer;;)
    {
        UNICODE_STRING string;

        RtlInitUnicodeString(&string, item);

        if (string.Length == 0)
        {
            break;
        }

        PhAddItemList(stringList, PhCreateStringFromUnicodeString(&string));

        item = PTR_ADD_OFFSET(item, string.MaximumLength);
    }

    *StringList = stringList;

Exit:

    if (buffer)
        PhFree(buffer);

    return !!result;
}

BOOLEAN PhpGetClassPropertyStringList(
    _In_ const GUID* ClassGuid,
    _In_ const DEVPROPKEY* DeviceProperty,
    _In_ ULONG Flags,
    _Out_ PPH_LIST* StringList
    )
{
    BOOL result;
    DEVPROPTYPE devicePropertyType = DEVPROP_TYPE_EMPTY;
    ULONG requiredLength = 0;
    PVOID buffer = NULL;
    PPH_LIST stringList;

    *StringList = NULL;

    result = SetupDiGetClassPropertyW(
        ClassGuid,
        DeviceProperty,
        &devicePropertyType,
        NULL,
        0,
        &requiredLength,
        Flags
        );
    if (result ||
        (requiredLength == 0) ||
        (GetLastError() != ERROR_INSUFFICIENT_BUFFER) ||
        (devicePropertyType != DEVPROP_TYPE_STRING_LIST))
    {
        goto Exit;
    }

    buffer = PhAllocate(requiredLength);

    result = SetupDiGetClassPropertyW(
        ClassGuid,
        DeviceProperty,
        &devicePropertyType,
        buffer,
        requiredLength,
        &requiredLength,
        Flags
        );
    if (!result)
    {
        goto Exit;
    }

    stringList = PhCreateList(1);

    for (PZZWSTR item = buffer;;)
    {
        PH_STRINGREF string;

        PhInitializeStringRefLongHint(&string, item);

        if (string.Length == 0)
        {
            break;
        }

        PhAddItemList(stringList, PhCreateString2(&string));

        item = PTR_ADD_OFFSET(item, string.Length + sizeof(UNICODE_NULL));
    }

    *StringList = stringList;

Exit:

    if (buffer)
        PhFree(buffer);

    return !!result;
}

_Function_class_(PH_DEVICE_PROPERTY_FILL_CALLBACK)
VOID NTAPI PhpDevPropFillString(
    _In_ HDEVINFO DeviceInfoSet,
    _In_ PSP_DEVINFO_DATA DeviceInfoData,
    _In_ const DEVPROPKEY* PropertyKey,
    _Out_ PPH_DEVICE_PROPERTY Property,
    _In_ ULONG Flags
    )
{
    Property->Type = PhDevicePropertyTypeString;

    if (!(Flags & (DEVPROP_FILL_FLAG_CLASS_INSTALLER | DEVPROP_FILL_FLAG_CLASS_INTERFACE)))
    {
        Property->Valid = PhpGetDevicePropertyString(
            DeviceInfoSet,
            DeviceInfoData,
            PropertyKey,
            &Property->String
            );
    }

    if (!Property->Valid && (Flags & DEVPROP_FILL_FLAG_CLASS_INTERFACE))
    {
        Property->Valid = PhpGetClassPropertyString(
            &DeviceInfoData->ClassGuid,
            PropertyKey,
            DICLASSPROP_INTERFACE,
            &Property->String
            );
    }

    if (!Property->Valid && (Flags & DEVPROP_FILL_FLAG_CLASS_INSTALLER))
    {
        Property->Valid = PhpGetClassPropertyString(
            &DeviceInfoData->ClassGuid,
            PropertyKey,
            DICLASSPROP_INSTALLER,
            &Property->String
            );
    }

    if (Property->Valid)
    {
        Property->AsString = Property->String;
        PhReferenceObject(Property->AsString);
    }
}

_Function_class_(PH_DEVICE_PROPERTY_FILL_CALLBACK)
VOID NTAPI PhpDevPropFillUInt64(
    _In_ HDEVINFO DeviceInfoSet,
    _In_ PSP_DEVINFO_DATA DeviceInfoData,
    _In_ const DEVPROPKEY* PropertyKey,
    _Out_ PPH_DEVICE_PROPERTY Property,
    _In_ ULONG Flags
    )
{
    Property->Type = PhDevicePropertyTypeUInt64;

    if (!(Flags & (DEVPROP_FILL_FLAG_CLASS_INSTALLER | DEVPROP_FILL_FLAG_CLASS_INTERFACE)))
    {
        Property->Valid = PhpGetDevicePropertyUInt64(
            DeviceInfoSet,
            DeviceInfoData,
            PropertyKey,
            &Property->UInt64
            );
    }

    if (!Property->Valid && (Flags & DEVPROP_FILL_FLAG_CLASS_INTERFACE))
    {
        Property->Valid = PhpGetClassPropertyUInt64(
            &DeviceInfoData->ClassGuid,
            PropertyKey,
            DICLASSPROP_INTERFACE,
            &Property->UInt64
            );
    }

    if (!Property->Valid && (Flags & DEVPROP_FILL_FLAG_CLASS_INSTALLER))
    {
        Property->Valid = PhpGetClassPropertyUInt64(
            &DeviceInfoData->ClassGuid,
            PropertyKey,
            DICLASSPROP_INSTALLER,
            &Property->UInt64
            );
    }

    if (Property->Valid)
    {
        PH_FORMAT format[1];

        PhInitFormatI64U(&format[0], Property->UInt64);

        Property->AsString = PhFormat(format, ARRAYSIZE(format), 1);
    }
}

_Function_class_(PH_DEVICE_PROPERTY_FILL_CALLBACK)
VOID NTAPI PhpDevPropFillUInt64Hex(
    _In_ HDEVINFO DeviceInfoSet,
    _In_ PSP_DEVINFO_DATA DeviceInfoData,
    _In_ const DEVPROPKEY* PropertyKey,
    _Out_ PPH_DEVICE_PROPERTY Property,
    _In_ ULONG Flags
    )
{
    Property->Type = PhDevicePropertyTypeUInt64;

    if (!(Flags & (DEVPROP_FILL_FLAG_CLASS_INSTALLER | DEVPROP_FILL_FLAG_CLASS_INTERFACE)))
    {
        Property->Valid = PhpGetDevicePropertyUInt64(
            DeviceInfoSet,
            DeviceInfoData,
            PropertyKey,
            &Property->UInt64
            );
    }

    if (!Property->Valid && (Flags & DEVPROP_FILL_FLAG_CLASS_INTERFACE))
    {
        Property->Valid = PhpGetClassPropertyUInt64(
            &DeviceInfoData->ClassGuid,
            PropertyKey,
            DICLASSPROP_INTERFACE,
            &Property->UInt64
            );
    }

    if (!Property->Valid && (Flags & DEVPROP_FILL_FLAG_CLASS_INSTALLER))
    {
        Property->Valid = PhpGetClassPropertyUInt64(
            &DeviceInfoData->ClassGuid,
            PropertyKey,
            DICLASSPROP_INSTALLER,
            &Property->UInt64
            );
    }

    if (Property->Valid)
    {
        PH_FORMAT format[2];

        PhInitFormatI64X(&format[1], Property->UInt64);

        Property->AsString = PhFormat(format, ARRAYSIZE(format), 10);
    }
}


_Function_class_(PH_DEVICE_PROPERTY_FILL_CALLBACK)
VOID NTAPI PhpDevPropFillUInt32(
    _In_ HDEVINFO DeviceInfoSet,
    _In_ PSP_DEVINFO_DATA DeviceInfoData,
    _In_ const DEVPROPKEY* PropertyKey,
    _Out_ PPH_DEVICE_PROPERTY Property,
    _In_ ULONG Flags
    )
{
    Property->Type = PhDevicePropertyTypeUInt32;

    if (!(Flags & (DEVPROP_FILL_FLAG_CLASS_INSTALLER | DEVPROP_FILL_FLAG_CLASS_INTERFACE)))
    {
        Property->Valid = PhpGetDevicePropertyUInt32(
            DeviceInfoSet,
            DeviceInfoData,
            PropertyKey,
            &Property->UInt32
            );
    }

    if (!Property->Valid && (Flags & DEVPROP_FILL_FLAG_CLASS_INTERFACE))
    {
        Property->Valid = PhpGetClassPropertyUInt32(
            &DeviceInfoData->ClassGuid,
            PropertyKey,
            DICLASSPROP_INTERFACE,
            &Property->UInt32
            );
    }

    if (!Property->Valid && (Flags & DEVPROP_FILL_FLAG_CLASS_INSTALLER))
    {
        Property->Valid = PhpGetClassPropertyUInt32(
            &DeviceInfoData->ClassGuid,
            PropertyKey,
            DICLASSPROP_INSTALLER,
            &Property->UInt32
            );
    }

    if (Property->Valid)
    {
        PH_FORMAT format[1];

        PhInitFormatU(&format[0], Property->UInt32);

        Property->AsString = PhFormat(format, ARRAYSIZE(format), 1);
    }
}

_Function_class_(PH_DEVICE_PROPERTY_FILL_CALLBACK)
VOID NTAPI PhpDevPropFillUInt32Hex(
    _In_ HDEVINFO DeviceInfoSet,
    _In_ PSP_DEVINFO_DATA DeviceInfoData,
    _In_ const DEVPROPKEY* PropertyKey,
    _Out_ PPH_DEVICE_PROPERTY Property,
    _In_ ULONG Flags
    )
{
    Property->Type = PhDevicePropertyTypeUInt32;

    if (!(Flags & (DEVPROP_FILL_FLAG_CLASS_INSTALLER | DEVPROP_FILL_FLAG_CLASS_INTERFACE)))
    {
        Property->Valid = PhpGetDevicePropertyUInt32(
            DeviceInfoSet,
            DeviceInfoData,
            PropertyKey,
            &Property->UInt32
            );
    }

    if (!Property->Valid && (Flags & DEVPROP_FILL_FLAG_CLASS_INTERFACE))
    {
        Property->Valid = PhpGetClassPropertyUInt32(
            &DeviceInfoData->ClassGuid,
            PropertyKey,
            DICLASSPROP_INTERFACE,
            &Property->UInt32
            );
    }

    if (!Property->Valid && (Flags & DEVPROP_FILL_FLAG_CLASS_INSTALLER))
    {
        Property->Valid = PhpGetClassPropertyUInt32(
            &DeviceInfoData->ClassGuid,
            PropertyKey,
            DICLASSPROP_INSTALLER,
            &Property->UInt32
            );
    }

    if (Property->Valid)
    {
        PH_FORMAT format[2];

        PhInitFormatS(&format[0], L"0x");
        PhInitFormatIX(&format[1], Property->UInt32);

        Property->AsString = PhFormat(format, ARRAYSIZE(format), 10);
    }
}

_Function_class_(PH_DEVICE_PROPERTY_FILL_CALLBACK)
VOID NTAPI PhpDevPropFillInt32(
    _In_ HDEVINFO DeviceInfoSet,
    _In_ PSP_DEVINFO_DATA DeviceInfoData,
    _In_ const DEVPROPKEY* PropertyKey,
    _Out_ PPH_DEVICE_PROPERTY Property,
    _In_ ULONG Flags
    )
{
    Property->Type = PhDevicePropertyTypeUInt32;

    if (!(Flags & (DEVPROP_FILL_FLAG_CLASS_INSTALLER | DEVPROP_FILL_FLAG_CLASS_INTERFACE)))
    {
        Property->Valid = PhpGetDevicePropertyInt32(
            DeviceInfoSet,
            DeviceInfoData,
            PropertyKey,
            &Property->Int32
            );
    }

    if (!Property->Valid && (Flags & DEVPROP_FILL_FLAG_CLASS_INTERFACE))
    {
        Property->Valid = PhpGetClassPropertyInt32(
            &DeviceInfoData->ClassGuid,
            PropertyKey,
            DICLASSPROP_INTERFACE,
            &Property->Int32
            );
    }

    if (!Property->Valid && (Flags & DEVPROP_FILL_FLAG_CLASS_INSTALLER))
    {
        Property->Valid = PhpGetClassPropertyInt32(
            &DeviceInfoData->ClassGuid,
            PropertyKey,
            DICLASSPROP_INSTALLER,
            &Property->Int32
            );
    }

    if (Property->Valid)
    {
        PH_FORMAT format[1];

        PhInitFormatD(&format[0], Property->Int32);

        Property->AsString = PhFormat(format, ARRAYSIZE(format), 1);
    }
}

_Function_class_(PH_DEVICE_PROPERTY_FILL_CALLBACK)
VOID NTAPI PhpDevPropFillNTSTATUS(
    _In_ HDEVINFO DeviceInfoSet,
    _In_ PSP_DEVINFO_DATA DeviceInfoData,
    _In_ const DEVPROPKEY* PropertyKey,
    _Out_ PPH_DEVICE_PROPERTY Property,
    _In_ ULONG Flags
    )
{
    Property->Type = PhDevicePropertyTypeNTSTATUS;

    if (!(Flags & (DEVPROP_FILL_FLAG_CLASS_INSTALLER | DEVPROP_FILL_FLAG_CLASS_INTERFACE)))
    {
        Property->Valid = PhpGetDevicePropertyNTSTATUS(
            DeviceInfoSet,
            DeviceInfoData,
            PropertyKey,
            &Property->Status
            );
    }

    if (!Property->Valid && (Flags & DEVPROP_FILL_FLAG_CLASS_INTERFACE))
    {
        Property->Valid = PhpGetClassPropertyNTSTATUS(
            &DeviceInfoData->ClassGuid,
            PropertyKey,
            DICLASSPROP_INTERFACE,
            &Property->Status
            );
    }

    if (!Property->Valid && (Flags & DEVPROP_FILL_FLAG_CLASS_INSTALLER))
    {
        Property->Valid = PhpGetClassPropertyNTSTATUS(
            &DeviceInfoData->ClassGuid,
            PropertyKey,
            DICLASSPROP_INSTALLER,
            &Property->Status
            );
    }

    if (Property->Valid && Property->Status != STATUS_SUCCESS)
    {
        Property->AsString = PhGetStatusMessage(Property->Status, 0);
    }
}

_Function_class_(PH_DEVICE_PROPERTY_FILL_CALLBACK)
VOID NTAPI PhpDevPropFillGuid(
    _In_ HDEVINFO DeviceInfoSet,
    _In_ PSP_DEVINFO_DATA DeviceInfoData,
    _In_ const DEVPROPKEY* PropertyKey,
    _Out_ PPH_DEVICE_PROPERTY Property,
    _In_ ULONG Flags
    )
{
    Property->Type = PhDevicePropertyTypeGUID;

    if (!(Flags & (DEVPROP_FILL_FLAG_CLASS_INSTALLER | DEVPROP_FILL_FLAG_CLASS_INTERFACE)))
    {
        Property->Valid = PhpGetDevicePropertyGuid(
            DeviceInfoSet,
            DeviceInfoData,
            PropertyKey,
            &Property->Guid
            );
    }

    if (!Property->Valid && (Flags & DEVPROP_FILL_FLAG_CLASS_INTERFACE))
    {
        Property->Valid = PhpGetClassPropertyGuid(
            &DeviceInfoData->ClassGuid,
            PropertyKey,
            DICLASSPROP_INTERFACE,
            &Property->Guid
            );
    }

    if (!Property->Valid && (Flags & DEVPROP_FILL_FLAG_CLASS_INSTALLER))
    {
        Property->Valid = PhpGetClassPropertyGuid(
            &DeviceInfoData->ClassGuid,
            PropertyKey,
            DICLASSPROP_INSTALLER,
            &Property->Guid
            );
    }

    if (Property->Valid)
    {
        Property->AsString = PhFormatGuid(&Property->Guid);
    }
}

PPH_STRING PhpDevPropBusTypeGuidToString(
    _In_ PGUID BusTypeGuid
    )
{
    if (IsEqualGUID(BusTypeGuid, &GUID_BUS_TYPE_INTERNAL))
    {
        static PH_STRINGREF string = PH_STRINGREF_INIT(L"GUID_BUS_TYPE_INTERNAL");
        return PhCreateString2(&string);
    }
    else if (IsEqualGUID(BusTypeGuid, &GUID_BUS_TYPE_PCMCIA))
    {
        static PH_STRINGREF string = PH_STRINGREF_INIT(L"GUID_BUS_TYPE_PCMCIA");
        return PhCreateString2(&string);
    }
    else if (IsEqualGUID(BusTypeGuid, &GUID_BUS_TYPE_PCI))
    {
        static PH_STRINGREF string = PH_STRINGREF_INIT(L"GUID_BUS_TYPE_PCI");
        return PhCreateString2(&string);
    }
    else if (IsEqualGUID(BusTypeGuid, &GUID_BUS_TYPE_ISAPNP))
    {
        static PH_STRINGREF string = PH_STRINGREF_INIT(L"GUID_BUS_TYPE_ISAPNP");
        return PhCreateString2(&string);
    }
    else if (IsEqualGUID(BusTypeGuid, &GUID_BUS_TYPE_EISA))
    {
        static PH_STRINGREF string = PH_STRINGREF_INIT(L"GUID_BUS_TYPE_EISA");
        return PhCreateString2(&string);
    }
    else if (IsEqualGUID(BusTypeGuid, &GUID_BUS_TYPE_MCA))
    {
        static PH_STRINGREF string = PH_STRINGREF_INIT(L"GUID_BUS_TYPE_MCA");
        return PhCreateString2(&string);
    }
    else if (IsEqualGUID(BusTypeGuid, &GUID_BUS_TYPE_SERENUM))
    {
        static PH_STRINGREF string = PH_STRINGREF_INIT(L"GUID_BUS_TYPE_SERENUM");
        return PhCreateString2(&string);
    }
    else if (IsEqualGUID(BusTypeGuid, &GUID_BUS_TYPE_USB))
    {
        static PH_STRINGREF string = PH_STRINGREF_INIT(L"GUID_BUS_TYPE_USB");
        return PhCreateString2(&string);
    }
    else if (IsEqualGUID(BusTypeGuid, &GUID_BUS_TYPE_LPTENUM))
    {
        static PH_STRINGREF string = PH_STRINGREF_INIT(L"GUID_BUS_TYPE_LPTENUM");
        return PhCreateString2(&string);
    }
    else if (IsEqualGUID(BusTypeGuid, &GUID_BUS_TYPE_USBPRINT))
    {
        static PH_STRINGREF string = PH_STRINGREF_INIT(L"GUID_BUS_TYPE_USBPRINT");
        return PhCreateString2(&string);
    }
    else if (IsEqualGUID(BusTypeGuid, &GUID_BUS_TYPE_DOT4PRT))
    {
        static PH_STRINGREF string = PH_STRINGREF_INIT(L"GUID_BUS_TYPE_DOT4PRT");
        return PhCreateString2(&string);
    }
    else if (IsEqualGUID(BusTypeGuid, &GUID_BUS_TYPE_1394))
    {
        static PH_STRINGREF string = PH_STRINGREF_INIT(L"GUID_BUS_TYPE_1394");
        return PhCreateString2(&string);
    }
    else if (IsEqualGUID(BusTypeGuid, &GUID_BUS_TYPE_HID))
    {
        static PH_STRINGREF string = PH_STRINGREF_INIT(L"GUID_BUS_TYPE_HID");
        return PhCreateString2(&string);
    }
    else if (IsEqualGUID(BusTypeGuid, &GUID_BUS_TYPE_AVC))
    {
        static PH_STRINGREF string = PH_STRINGREF_INIT(L"GUID_BUS_TYPE_AVC");
        return PhCreateString2(&string);
    }
    else if (IsEqualGUID(BusTypeGuid, &GUID_BUS_TYPE_IRDA))
    {
        static PH_STRINGREF string = PH_STRINGREF_INIT(L"GUID_BUS_TYPE_IRDA");
        return PhCreateString2(&string);
    }
    else if (IsEqualGUID(BusTypeGuid, &GUID_BUS_TYPE_SD))
    {
        static PH_STRINGREF string = PH_STRINGREF_INIT(L"GUID_BUS_TYPE_SD");
        return PhCreateString2(&string);
    }
    else if (IsEqualGUID(BusTypeGuid, &GUID_BUS_TYPE_ACPI))
    {
        static PH_STRINGREF string = PH_STRINGREF_INIT(L"GUID_BUS_TYPE_ACPI");
        return PhCreateString2(&string);
    }
    else if (IsEqualGUID(BusTypeGuid, &GUID_BUS_TYPE_SW_DEVICE))
    {
        static PH_STRINGREF string = PH_STRINGREF_INIT(L"GUID_BUS_TYPE_SW_DEVICE");
        return PhCreateString2(&string);
    }
    else if (IsEqualGUID(BusTypeGuid, &GUID_BUS_TYPE_SCM))
    {
        static PH_STRINGREF string = PH_STRINGREF_INIT(L"GUID_BUS_TYPE_SCM");
        return PhCreateString2(&string);
    }

    return NULL;
}

_Function_class_(PH_DEVICE_PROPERTY_FILL_CALLBACK)
VOID NTAPI PhpDevPropFillBusTypeGuid(
    _In_ HDEVINFO DeviceInfoSet,
    _In_ PSP_DEVINFO_DATA DeviceInfoData,
    _In_ const DEVPROPKEY* PropertyKey,
    _Out_ PPH_DEVICE_PROPERTY Property,
    _In_ ULONG Flags
    )
{
    Property->Type = PhDevicePropertyTypeGUID;

    if (!(Flags & (DEVPROP_FILL_FLAG_CLASS_INSTALLER | DEVPROP_FILL_FLAG_CLASS_INTERFACE)))
    {
        Property->Valid = PhpGetDevicePropertyGuid(
            DeviceInfoSet,
            DeviceInfoData,
            PropertyKey,
            &Property->Guid
            );
    }

    if (!Property->Valid && (Flags & DEVPROP_FILL_FLAG_CLASS_INTERFACE))
    {
        Property->Valid = PhpGetClassPropertyGuid(
            &DeviceInfoData->ClassGuid,
            PropertyKey,
            DICLASSPROP_INTERFACE,
            &Property->Guid
            );
    }

    if (!Property->Valid && (Flags & DEVPROP_FILL_FLAG_CLASS_INSTALLER))
    {
        Property->Valid = PhpGetClassPropertyGuid(
            &DeviceInfoData->ClassGuid,
            PropertyKey,
            DICLASSPROP_INSTALLER,
            &Property->Guid
            );
    }

    if (Property->Valid)
    {
        Property->AsString = PhpDevPropBusTypeGuidToString(&Property->Guid);

        if (!Property->AsString)
        {
            Property->AsString = PhFormatGuid(&Property->Guid);
        }
    }
}

PPH_STRING PhpDevPropPciDeviceInterruptSupportToString(
    _In_ ULONG Flags
    )
{
    PH_STRING_BUILDER stringBuilder;
    WCHAR pointer[PH_PTR_STR_LEN_1];

    PhInitializeStringBuilder(&stringBuilder, 10);

    if (BooleanFlagOn(Flags, DevProp_PciDevice_InterruptType_LineBased))
        PhAppendStringBuilder2(&stringBuilder, L"Line based, ");
    if (BooleanFlagOn(Flags, DevProp_PciDevice_InterruptType_Msi))
        PhAppendStringBuilder2(&stringBuilder, L"Msi, ");
    if (BooleanFlagOn(Flags, DevProp_PciDevice_InterruptType_MsiX))
        PhAppendStringBuilder2(&stringBuilder, L"MsiX, ");

    if (PhEndsWithString2(stringBuilder.String, L", ", FALSE))
        PhRemoveEndStringBuilder(&stringBuilder, 2);

    PhPrintPointer(pointer, UlongToPtr(Flags));
    PhAppendFormatStringBuilder(&stringBuilder, L" (%s)", pointer);

    return PhFinalStringBuilderString(&stringBuilder);
}

_Function_class_(PH_DEVICE_PROPERTY_FILL_CALLBACK)
VOID NTAPI PhpDevPropFillPciDeviceInterruptSupport(
    _In_ HDEVINFO DeviceInfoSet,
    _In_ PSP_DEVINFO_DATA DeviceInfoData,
    _In_ const DEVPROPKEY* PropertyKey,
    _Out_ PPH_DEVICE_PROPERTY Property,
    _In_ ULONG Flags
    )
{
    Property->Type = PhDevicePropertyTypeUInt32;

    if (!(Flags & (DEVPROP_FILL_FLAG_CLASS_INSTALLER | DEVPROP_FILL_FLAG_CLASS_INTERFACE)))
    {
        Property->Valid = PhpGetDevicePropertyUInt32(
            DeviceInfoSet,
            DeviceInfoData,
            PropertyKey,
            &Property->UInt32
            );
    }

    if (!Property->Valid && (Flags & DEVPROP_FILL_FLAG_CLASS_INTERFACE))
    {
        Property->Valid = PhpGetClassPropertyUInt32(
            &DeviceInfoData->ClassGuid,
            PropertyKey,
            DICLASSPROP_INTERFACE,
            &Property->UInt32
            );
    }

    if (!Property->Valid && (Flags & DEVPROP_FILL_FLAG_CLASS_INSTALLER))
    {
        Property->Valid = PhpGetClassPropertyUInt32(
            &DeviceInfoData->ClassGuid,
            PropertyKey,
            DICLASSPROP_INSTALLER,
            &Property->UInt32
            );
    }

    if (Property->Valid)
    {
        Property->AsString = PhpDevPropPciDeviceInterruptSupportToString(Property->UInt32);
    }
}

_Function_class_(PH_DEVICE_PROPERTY_FILL_CALLBACK)
VOID NTAPI PhpDevPropFillBoolean(
    _In_ HDEVINFO DeviceInfoSet,
    _In_ PSP_DEVINFO_DATA DeviceInfoData,
    _In_ const DEVPROPKEY* PropertyKey,
    _Out_ PPH_DEVICE_PROPERTY Property,
    _In_ ULONG Flags
    )
{
    Property->Type = PhDevicePropertyTypeBoolean;

    if (!(Flags & (DEVPROP_FILL_FLAG_CLASS_INSTALLER | DEVPROP_FILL_FLAG_CLASS_INTERFACE)))
    {
        Property->Valid = PhpGetDevicePropertyBoolean(
            DeviceInfoSet,
            DeviceInfoData,
            PropertyKey,
            &Property->Boolean
            );
    }

    if (!Property->Valid && (Flags & DEVPROP_FILL_FLAG_CLASS_INTERFACE))
    {
        Property->Valid = PhpGetClassPropertyBoolean(
            &DeviceInfoData->ClassGuid,
            PropertyKey,
            DICLASSPROP_INTERFACE,
            &Property->Boolean
            );
    }

    if (!Property->Valid && (Flags & DEVPROP_FILL_FLAG_CLASS_INSTALLER))
    {
        Property->Valid = PhpGetClassPropertyBoolean(
            &DeviceInfoData->ClassGuid,
            PropertyKey,
            DICLASSPROP_INSTALLER,
            &Property->Boolean
            );
    }

    if (Property->Valid)
    {
        if (Property->Boolean)
            Property->AsString = PhCreateString(L"true");
        else
            Property->AsString = PhCreateString(L"false");
    }
}

_Function_class_(PH_DEVICE_PROPERTY_FILL_CALLBACK)
VOID NTAPI PhpDevPropFillTimeStamp(
    _In_ HDEVINFO DeviceInfoSet,
    _In_ PSP_DEVINFO_DATA DeviceInfoData,
    _In_ const DEVPROPKEY* PropertyKey,
    _Out_ PPH_DEVICE_PROPERTY Property,
    _In_ ULONG Flags
    )
{
    Property->Type = PhDevicePropertyTypeTimeStamp;

    if (!(Flags & (DEVPROP_FILL_FLAG_CLASS_INSTALLER | DEVPROP_FILL_FLAG_CLASS_INTERFACE)))
    {
        Property->Valid = PhpGetDevicePropertyTimeStamp(
            DeviceInfoSet,
            DeviceInfoData,
            PropertyKey,
            &Property->TimeStamp
            );
    }

    if (!Property->Valid && (Flags & DEVPROP_FILL_FLAG_CLASS_INTERFACE))
    {
        Property->Valid = PhpGetClassPropertyTimeStamp(
            &DeviceInfoData->ClassGuid,
            PropertyKey,
            DICLASSPROP_INTERFACE,
            &Property->TimeStamp
            );
    }

    if (!Property->Valid && (Flags & DEVPROP_FILL_FLAG_CLASS_INSTALLER))
    {
        Property->Valid = PhpGetClassPropertyTimeStamp(
            &DeviceInfoData->ClassGuid,
            PropertyKey,
            DICLASSPROP_INSTALLER,
            &Property->TimeStamp
            );
    }

    if (Property->Valid)
    {
        SYSTEMTIME systemTime;

        PhLargeIntegerToLocalSystemTime(&systemTime, &Property->TimeStamp);

        Property->AsString = PhFormatDateTime(&systemTime);
    }
}

_Function_class_(PH_DEVICE_PROPERTY_FILL_CALLBACK)
VOID NTAPI PhpDevPropFillStringList(
    _In_ HDEVINFO DeviceInfoSet,
    _In_ PSP_DEVINFO_DATA DeviceInfoData,
    _In_ const DEVPROPKEY* PropertyKey,
    _Out_ PPH_DEVICE_PROPERTY Property,
    _In_ ULONG Flags
    )
{
    Property->Type = PhDevicePropertyTypeStringList;

    if (!(Flags & (DEVPROP_FILL_FLAG_CLASS_INSTALLER | DEVPROP_FILL_FLAG_CLASS_INTERFACE)))
    {
        Property->Valid = PhpGetDevicePropertyStringList(
            DeviceInfoSet,
            DeviceInfoData,
            PropertyKey,
            &Property->StringList
            );
    }

    if (!Property->Valid && (Flags & DEVPROP_FILL_FLAG_CLASS_INTERFACE))
    {
        Property->Valid = PhpGetClassPropertyStringList(
            &DeviceInfoData->ClassGuid,
            PropertyKey,
            DICLASSPROP_INTERFACE,
            &Property->StringList
            );
    }

    if (!Property->Valid && (Flags & DEVPROP_FILL_FLAG_CLASS_INSTALLER))
    {
        Property->Valid = PhpGetClassPropertyStringList(
            &DeviceInfoData->ClassGuid,
            PropertyKey,
            DICLASSPROP_INSTALLER,
            &Property->StringList
            );
    }

    if (Property->Valid && Property->StringList->Count > 0)
    {
        PH_STRING_BUILDER stringBuilder;
        PPH_STRING string;

        PhInitializeStringBuilder(&stringBuilder, Property->StringList->Count);

        for (ULONG i = 0; i < Property->StringList->Count - 1; i++)
        {
            string = Property->StringList->Items[i];

            PhAppendStringBuilder(&stringBuilder, &string->sr);
            PhAppendStringBuilder2(&stringBuilder, L", ");
        }

        string = Property->StringList->Items[Property->StringList->Count - 1];

        PhAppendStringBuilder(&stringBuilder, &string->sr);

        Property->AsString = PhFinalStringBuilderString(&stringBuilder);
        PhReferenceObject(Property->AsString);

        PhDeleteStringBuilder(&stringBuilder);
    }
}

_Function_class_(PH_DEVICE_PROPERTY_FILL_CALLBACK)
VOID NTAPI PhpDevPropFillStringOrStringList(
    _In_ HDEVINFO DeviceInfoSet,
    _In_ PSP_DEVINFO_DATA DeviceInfoData,
    _In_ const DEVPROPKEY* PropertyKey,
    _Out_ PPH_DEVICE_PROPERTY Property,
    _In_ ULONG Flags
    )
{
    PhpDevPropFillString(
        DeviceInfoSet,
        DeviceInfoData,
        PropertyKey,
        Property,
        Flags
        );
    if (!Property->Valid)
    {
        PhpDevPropFillStringList(
            DeviceInfoSet,
            DeviceInfoData,
            PropertyKey,
            Property,
            Flags
            );
    }
}

static const PH_DEVICE_PROPERTY_TABLE_ENTRY PhpDeviceItemPropertyTable[] =
{
    { PhDevicePropertyName, &DEVPKEY_NAME, PhpDevPropFillString, 0 },
    { PhDevicePropertyManufacturer, &DEVPKEY_Device_Manufacturer, PhpDevPropFillString, 0 },
    { PhDevicePropertyService, &DEVPKEY_Device_Service, PhpDevPropFillString, 0 },
    { PhDevicePropertyClass, &DEVPKEY_Device_Class, PhpDevPropFillString, 0 },
    { PhDevicePropertyEnumeratorName, &DEVPKEY_Device_EnumeratorName, PhpDevPropFillString, 0 },
    { PhDevicePropertyInstallDate, &DEVPKEY_Device_InstallDate, PhpDevPropFillTimeStamp, 0 },

    { PhDevicePropertyFirstInstallDate, &DEVPKEY_Device_FirstInstallDate, PhpDevPropFillTimeStamp, 0 },
    { PhDevicePropertyLastArrivalDate, &DEVPKEY_Device_LastArrivalDate, PhpDevPropFillTimeStamp, 0 },
    { PhDevicePropertyLastRemovalDate, &DEVPKEY_Device_LastRemovalDate, PhpDevPropFillTimeStamp, 0 },
    { PhDevicePropertyDeviceDesc, &DEVPKEY_Device_DeviceDesc, PhpDevPropFillString, 0 },
    { PhDevicePropertyFriendlyName, &DEVPKEY_Device_FriendlyName, PhpDevPropFillString, 0 },
    { PhDevicePropertyInstanceId, &DEVPKEY_Device_InstanceId, PhpDevPropFillString, 0 },
    { PhDevicePropertyParentInstanceId, &DEVPKEY_Device_Parent, PhpDevPropFillString, 0 },
    { PhDevicePropertyPDOName, &DEVPKEY_Device_PDOName, PhpDevPropFillString, 0 },
    { PhDevicePropertyLocationInfo, &DEVPKEY_Device_LocationInfo, PhpDevPropFillString, 0 },
    { PhDevicePropertyClassGuid, &DEVPKEY_Device_ClassGuid, PhpDevPropFillGuid, 0 },
    { PhDevicePropertyDriver, &DEVPKEY_Device_Driver, PhpDevPropFillString, 0 },
    { PhDevicePropertyDriverVersion, &DEVPKEY_Device_DriverVersion, PhpDevPropFillString, 0 },
    { PhDevicePropertyDriverDate, &DEVPKEY_Device_DriverDate, PhpDevPropFillTimeStamp, 0 },
    { PhDevicePropertyFirmwareDate, &DEVPKEY_Device_FirmwareDate, PhpDevPropFillTimeStamp, 0 },
    { PhDevicePropertyFirmwareVersion, &DEVPKEY_Device_FirmwareVersion, PhpDevPropFillString, 0 },
    { PhDevicePropertyFirmwareRevision, &DEVPKEY_Device_FirmwareRevision, PhpDevPropFillString, 0 },
    { PhDevicePropertyHasProblem, &DEVPKEY_Device_HasProblem, PhpDevPropFillBoolean, 0 },
    { PhDevicePropertyProblemCode, &DEVPKEY_Device_ProblemCode, PhpDevPropFillUInt32, 0 },
    { PhDevicePropertyProblemStatus, &DEVPKEY_Device_ProblemStatus, PhpDevPropFillNTSTATUS, 0 },
    { PhDevicePropertyDevNodeStatus, &DEVPKEY_Device_DevNodeStatus, PhpDevPropFillUInt32Hex, 0 },
    { PhDevicePropertyDevCapabilities, &DEVPKEY_Device_Capabilities, PhpDevPropFillUInt32Hex, 0 },
    { PhDevicePropertyUpperFilters, &DEVPKEY_Device_UpperFilters, PhpDevPropFillStringList, 0 },
    { PhDevicePropertyLowerFilters, &DEVPKEY_Device_LowerFilters, PhpDevPropFillStringList, 0 },
    { PhDevicePropertyHardwareIds, &DEVPKEY_Device_HardwareIds, PhpDevPropFillStringList, 0 },
    { PhDevicePropertyCompatibleIds, &DEVPKEY_Device_CompatibleIds, PhpDevPropFillStringList, 0 },
    { PhDevicePropertyConfigFlags, &DEVPKEY_Device_ConfigFlags, PhpDevPropFillUInt32Hex, 0 },
    { PhDevicePropertyUINumber, &DEVPKEY_Device_UINumber, PhpDevPropFillUInt32, 0 },
    { PhDevicePropertyBusTypeGuid, &DEVPKEY_Device_BusTypeGuid, PhpDevPropFillBusTypeGuid, 0 },
    { PhDevicePropertyLegacyBusType, &DEVPKEY_Device_LegacyBusType, PhpDevPropFillUInt32, 0 },
    { PhDevicePropertyBusNumber, &DEVPKEY_Device_BusNumber, PhpDevPropFillUInt32, 0 },
    //{ , &DEVPKEY_Device_Security, NULL, 0 },               // DEVPROP_TYPE_SECURITY_DESCRIPTOR
    { PhDevicePropertySecuritySDS, &DEVPKEY_Device_SecuritySDS, PhpDevPropFillString, 0 },
    { PhDevicePropertyDevType, &DEVPKEY_Device_DevType, PhpDevPropFillUInt32, 0 },
    { PhDevicePropertyExclusive, &DEVPKEY_Device_Exclusive, PhpDevPropFillBoolean, 0 },
    { PhDevicePropertyCharacteristics, &DEVPKEY_Device_Characteristics, PhpDevPropFillUInt32Hex, 0 },
    { PhDevicePropertyAddress, &DEVPKEY_Device_Address, PhpDevPropFillUInt32Hex, 0 },
    //{ , &DEVPKEY_Device_PowerData, NULL, 0 },              // DEVPROP_TYPE_BINARY
    { PhDevicePropertyRemovalPolicy, &DEVPKEY_Device_RemovalPolicy, PhpDevPropFillUInt32, 0 },
    { PhDevicePropertyRemovalPolicyDefault, &DEVPKEY_Device_RemovalPolicyDefault, PhpDevPropFillUInt32, 0 },
    { PhDevicePropertyRemovalPolicyOverride, &DEVPKEY_Device_RemovalPolicyOverride, PhpDevPropFillUInt32, 0 },
    { PhDevicePropertyInstallState, &DEVPKEY_Device_InstallState, PhpDevPropFillUInt32, 0 },
    { PhDevicePropertyLocationPaths, &DEVPKEY_Device_LocationPaths, PhpDevPropFillStringList, 0 },
    { PhDevicePropertyBaseContainerId, &DEVPKEY_Device_BaseContainerId, PhpDevPropFillGuid, 0 },
    { PhDevicePropertyEjectionRelations, &DEVPKEY_Device_EjectionRelations, PhpDevPropFillStringList, 0 },
    { PhDevicePropertyRemovalRelations, &DEVPKEY_Device_RemovalRelations, PhpDevPropFillStringList, 0 },
    { PhDevicePropertyPowerRelations, &DEVPKEY_Device_PowerRelations, PhpDevPropFillStringList, 0 },
    { PhDevicePropertyBusRelations, &DEVPKEY_Device_BusRelations, PhpDevPropFillStringList, 0 },
    { PhDevicePropertyChildren, &DEVPKEY_Device_Children, PhpDevPropFillStringList, 0 },
    { PhDevicePropertySiblings, &DEVPKEY_Device_Siblings, PhpDevPropFillStringList, 0 },
    { PhDevicePropertyTransportRelations, &DEVPKEY_Device_TransportRelations, PhpDevPropFillStringList, 0 },
    { PhDevicePropertyReported, &DEVPKEY_Device_Reported, PhpDevPropFillBoolean, 0 },
    { PhDevicePropertyLegacy, &DEVPKEY_Device_Legacy, PhpDevPropFillBoolean, 0 },
    { PhDevicePropertyContainerId, &DEVPKEY_Device_ContainerId, PhpDevPropFillGuid, 0 },
    { PhDevicePropertyInLocalMachineContainer, &DEVPKEY_Device_InLocalMachineContainer, PhpDevPropFillBoolean, 0 },
    { PhDevicePropertyModel, &DEVPKEY_Device_Model, PhpDevPropFillString, 0 },
    { PhDevicePropertyModelId, &DEVPKEY_Device_ModelId, PhpDevPropFillGuid, 0 },
    { PhDevicePropertyFriendlyNameAttributes, &DEVPKEY_Device_FriendlyNameAttributes, PhpDevPropFillUInt32Hex, 0 },
    { PhDevicePropertyManufacturerAttributes, &DEVPKEY_Device_ManufacturerAttributes, PhpDevPropFillUInt32Hex, 0 },
    { PhDevicePropertyPresenceNotForDevice, &DEVPKEY_Device_PresenceNotForDevice, PhpDevPropFillBoolean, 0 },
    { PhDevicePropertySignalStrength, &DEVPKEY_Device_SignalStrength, PhpDevPropFillInt32, 0 },
    { PhDevicePropertyIsAssociateableByUserAction, &DEVPKEY_Device_IsAssociateableByUserAction, PhpDevPropFillBoolean, 0 },
    { PhDevicePropertyShowInUninstallUI, &DEVPKEY_Device_ShowInUninstallUI, PhpDevPropFillBoolean, 0 },
    { PhDevicePropertyNumaProximityDomain, &DEVPKEY_Device_Numa_Proximity_Domain, PhpDevPropFillUInt32, 0 },
    { PhDevicePropertyDHPRebalancePolicy, &DEVPKEY_Device_DHP_Rebalance_Policy, PhpDevPropFillUInt32, 0 },
    { PhDevicePropertyNumaNode, &DEVPKEY_Device_Numa_Node, PhpDevPropFillUInt32, 0 },
    { PhDevicePropertyBusReportedDeviceDesc, &DEVPKEY_Device_BusReportedDeviceDesc, PhpDevPropFillString, 0 },
    { PhDevicePropertyIsPresent, &DEVPKEY_Device_IsPresent, PhpDevPropFillBoolean, 0 },
    { PhDevicePropertyConfigurationId, &DEVPKEY_Device_ConfigurationId, PhpDevPropFillString, 0 },
    { PhDevicePropertyReportedDeviceIdsHash, &DEVPKEY_Device_ReportedDeviceIdsHash, PhpDevPropFillUInt32, 0 },
    //{ , &DEVPKEY_Device_PhysicalDeviceLocation, NULL, 0 },    // DEVPROP_TYPE_BINARY
    { PhDevicePropertyBiosDeviceName, &DEVPKEY_Device_BiosDeviceName, PhpDevPropFillString, 0 },
    { PhDevicePropertyDriverProblemDesc, &DEVPKEY_Device_DriverProblemDesc, PhpDevPropFillString, 0 },
    { PhDevicePropertyDebuggerSafe, &DEVPKEY_Device_DebuggerSafe, PhpDevPropFillUInt32, 0 },
    { PhDevicePropertyPostInstallInProgress, &DEVPKEY_Device_PostInstallInProgress, PhpDevPropFillBoolean, 0 },
    { PhDevicePropertyStack, &DEVPKEY_Device_Stack, PhpDevPropFillStringList, 0 },
    { PhDevicePropertyExtendedConfigurationIds, &DEVPKEY_Device_ExtendedConfigurationIds, PhpDevPropFillStringList, 0 },
    { PhDevicePropertyIsRebootRequired, &DEVPKEY_Device_IsRebootRequired, PhpDevPropFillBoolean, 0 },
    { PhDevicePropertyDependencyProviders, &DEVPKEY_Device_DependencyProviders, PhpDevPropFillStringList, 0 },
    { PhDevicePropertyDependencyDependents, &DEVPKEY_Device_DependencyDependents, PhpDevPropFillStringList, 0 },
    { PhDevicePropertySoftRestartSupported, &DEVPKEY_Device_SoftRestartSupported, PhpDevPropFillBoolean, 0 },
    { PhDevicePropertyExtendedAddress, &DEVPKEY_Device_ExtendedAddress, PhpDevPropFillUInt64Hex, 0 },
    { PhDevicePropertyAssignedToGuest, &DEVPKEY_Device_AssignedToGuest, PhpDevPropFillBoolean, 0 },
    { PhDevicePropertyCreatorProcessId, &DEVPKEY_Device_CreatorProcessId, PhpDevPropFillUInt32, 0 },
    { PhDevicePropertyFirmwareVendor, &DEVPKEY_Device_FirmwareVendor, PhpDevPropFillString, 0 },
    { PhDevicePropertySessionId, &DEVPKEY_Device_SessionId, PhpDevPropFillUInt32, 0 },
    { PhDevicePropertyDriverDesc, &DEVPKEY_Device_DriverDesc, PhpDevPropFillString, 0 },
    { PhDevicePropertyDriverInfPath, &DEVPKEY_Device_DriverInfPath, PhpDevPropFillString, 0 },
    { PhDevicePropertyDriverInfSection, &DEVPKEY_Device_DriverInfSection, PhpDevPropFillString, 0 },
    { PhDevicePropertyDriverInfSectionExt, &DEVPKEY_Device_DriverInfSectionExt, PhpDevPropFillString, 0 },
    { PhDevicePropertyMatchingDeviceId, &DEVPKEY_Device_MatchingDeviceId, PhpDevPropFillString, 0 },
    { PhDevicePropertyDriverProvider, &DEVPKEY_Device_DriverProvider, PhpDevPropFillString, 0 },
    { PhDevicePropertyDriverPropPageProvider, &DEVPKEY_Device_DriverPropPageProvider, PhpDevPropFillString, 0 },
    { PhDevicePropertyDriverCoInstallers, &DEVPKEY_Device_DriverCoInstallers, PhpDevPropFillStringList, 0 },
    { PhDevicePropertyResourcePickerTags, &DEVPKEY_Device_ResourcePickerTags, PhpDevPropFillString, 0 },
    { PhDevicePropertyResourcePickerExceptions, &DEVPKEY_Device_ResourcePickerExceptions, PhpDevPropFillString, 0 },
    { PhDevicePropertyDriverRank, &DEVPKEY_Device_DriverRank, PhpDevPropFillUInt32, 0 },
    { PhDevicePropertyDriverLogoLevel, &DEVPKEY_Device_DriverLogoLevel, PhpDevPropFillUInt32, 0 },
    { PhDevicePropertyNoConnectSound, &DEVPKEY_Device_NoConnectSound, PhpDevPropFillBoolean, 0 },
    { PhDevicePropertyGenericDriverInstalled, &DEVPKEY_Device_GenericDriverInstalled, PhpDevPropFillBoolean, 0 },
    { PhDevicePropertyAdditionalSoftwareRequested, &DEVPKEY_Device_AdditionalSoftwareRequested, PhpDevPropFillBoolean, 0 },
    { PhDevicePropertySafeRemovalRequired, &DEVPKEY_Device_SafeRemovalRequired, PhpDevPropFillBoolean, 0 },
    { PhDevicePropertySafeRemovalRequiredOverride, &DEVPKEY_Device_SafeRemovalRequiredOverride, PhpDevPropFillBoolean, 0 },

    { PhDevicePropertyPkgModel, &DEVPKEY_DrvPkg_Model, PhpDevPropFillString, 0 },
    { PhDevicePropertyPkgVendorWebSite, &DEVPKEY_DrvPkg_VendorWebSite, PhpDevPropFillString, 0 },
    { PhDevicePropertyPkgDetailedDescription, &DEVPKEY_DrvPkg_DetailedDescription, PhpDevPropFillString, 0 },
    { PhDevicePropertyPkgDocumentationLink, &DEVPKEY_DrvPkg_DocumentationLink, PhpDevPropFillString, 0 },
    { PhDevicePropertyPkgIcon, &DEVPKEY_DrvPkg_Icon, PhpDevPropFillStringList, 0 },
    { PhDevicePropertyPkgBrandingIcon, &DEVPKEY_DrvPkg_BrandingIcon, PhpDevPropFillStringList, 0 },

    { PhDevicePropertyClassUpperFilters, &DEVPKEY_DeviceClass_UpperFilters, PhpDevPropFillStringList, DEVPROP_FILL_FLAG_CLASS_INTERFACE | DEVPROP_FILL_FLAG_CLASS_INSTALLER },
    { PhDevicePropertyClassLowerFilters, &DEVPKEY_DeviceClass_LowerFilters, PhpDevPropFillStringList, DEVPROP_FILL_FLAG_CLASS_INTERFACE | DEVPROP_FILL_FLAG_CLASS_INSTALLER },
    //{ , &DEVPKEY_DeviceClass_Security, NULL, 0 },    // DEVPROP_TYPE_SECURITY_DESCRIPTOR
    { PhDevicePropertyClassSecuritySDS, &DEVPKEY_DeviceClass_SecuritySDS, PhpDevPropFillString, 0 },
    { PhDevicePropertyClassDevType, &DEVPKEY_DeviceClass_DevType, PhpDevPropFillUInt32, DEVPROP_FILL_FLAG_CLASS_INTERFACE | DEVPROP_FILL_FLAG_CLASS_INSTALLER },
    { PhDevicePropertyClassExclusive, &DEVPKEY_DeviceClass_Exclusive, PhpDevPropFillBoolean, DEVPROP_FILL_FLAG_CLASS_INTERFACE | DEVPROP_FILL_FLAG_CLASS_INSTALLER },
    { PhDevicePropertyClassCharacteristics, &DEVPKEY_DeviceClass_Characteristics, PhpDevPropFillUInt32Hex, DEVPROP_FILL_FLAG_CLASS_INTERFACE | DEVPROP_FILL_FLAG_CLASS_INSTALLER },
    { PhDevicePropertyClassName, &DEVPKEY_DeviceClass_Name, PhpDevPropFillString, DEVPROP_FILL_FLAG_CLASS_INTERFACE | DEVPROP_FILL_FLAG_CLASS_INSTALLER },
    { PhDevicePropertyClassClassName, &DEVPKEY_DeviceClass_ClassName, PhpDevPropFillString, DEVPROP_FILL_FLAG_CLASS_INTERFACE | DEVPROP_FILL_FLAG_CLASS_INSTALLER },
    { PhDevicePropertyClassIcon, &DEVPKEY_DeviceClass_Icon, PhpDevPropFillString, DEVPROP_FILL_FLAG_CLASS_INTERFACE | DEVPROP_FILL_FLAG_CLASS_INSTALLER },
    { PhDevicePropertyClassClassInstaller, &DEVPKEY_DeviceClass_ClassInstaller, PhpDevPropFillString, DEVPROP_FILL_FLAG_CLASS_INTERFACE | DEVPROP_FILL_FLAG_CLASS_INSTALLER },
    { PhDevicePropertyClassPropPageProvider, &DEVPKEY_DeviceClass_PropPageProvider, PhpDevPropFillString, DEVPROP_FILL_FLAG_CLASS_INTERFACE | DEVPROP_FILL_FLAG_CLASS_INSTALLER },
    { PhDevicePropertyClassNoInstallClass, &DEVPKEY_DeviceClass_NoInstallClass, PhpDevPropFillBoolean, DEVPROP_FILL_FLAG_CLASS_INTERFACE | DEVPROP_FILL_FLAG_CLASS_INSTALLER },
    { PhDevicePropertyClassNoDisplayClass, &DEVPKEY_DeviceClass_NoDisplayClass, PhpDevPropFillBoolean, DEVPROP_FILL_FLAG_CLASS_INTERFACE | DEVPROP_FILL_FLAG_CLASS_INSTALLER },
    { PhDevicePropertyClassSilentInstall, &DEVPKEY_DeviceClass_SilentInstall, PhpDevPropFillBoolean, DEVPROP_FILL_FLAG_CLASS_INTERFACE | DEVPROP_FILL_FLAG_CLASS_INSTALLER },
    { PhDevicePropertyClassNoUseClass, &DEVPKEY_DeviceClass_NoUseClass, PhpDevPropFillBoolean, DEVPROP_FILL_FLAG_CLASS_INTERFACE | DEVPROP_FILL_FLAG_CLASS_INSTALLER },
    { PhDevicePropertyClassDefaultService, &DEVPKEY_DeviceClass_DefaultService, PhpDevPropFillString, DEVPROP_FILL_FLAG_CLASS_INTERFACE | DEVPROP_FILL_FLAG_CLASS_INSTALLER },
    { PhDevicePropertyClassIconPath, &DEVPKEY_DeviceClass_IconPath, PhpDevPropFillStringList, DEVPROP_FILL_FLAG_CLASS_INTERFACE | DEVPROP_FILL_FLAG_CLASS_INSTALLER },
    { PhDevicePropertyClassDHPRebalanceOptOut, &DEVPKEY_DeviceClass_DHPRebalanceOptOut, PhpDevPropFillBoolean, DEVPROP_FILL_FLAG_CLASS_INTERFACE | DEVPROP_FILL_FLAG_CLASS_INSTALLER },
    { PhDevicePropertyClassClassCoInstallers, &DEVPKEY_DeviceClass_ClassCoInstallers, PhpDevPropFillStringList, DEVPROP_FILL_FLAG_CLASS_INTERFACE | DEVPROP_FILL_FLAG_CLASS_INSTALLER },

    { PhDevicePropertyInterfaceFriendlyName, &DEVPKEY_DeviceInterface_FriendlyName, PhpDevPropFillString, 0 },
    { PhDevicePropertyInterfaceEnabled, &DEVPKEY_DeviceInterface_Enabled, PhpDevPropFillBoolean, 0 },
    { PhDevicePropertyInterfaceClassGuid, &DEVPKEY_DeviceInterface_ClassGuid, PhpDevPropFillGuid, 0 },
    { PhDevicePropertyInterfaceReferenceString, &DEVPKEY_DeviceInterface_ReferenceString, PhpDevPropFillString, 0 },
    { PhDevicePropertyInterfaceRestricted, &DEVPKEY_DeviceInterface_Restricted, PhpDevPropFillBoolean, 0 },
    { PhDevicePropertyInterfaceUnrestrictedAppCapabilities, &DEVPKEY_DeviceInterface_UnrestrictedAppCapabilities, PhpDevPropFillStringList, 0 },
    { PhDevicePropertyInterfaceSchematicName, &DEVPKEY_DeviceInterface_SchematicName, PhpDevPropFillString, 0 },

    { PhDevicePropertyInterfaceClassDefaultInterface, &DEVPKEY_DeviceInterfaceClass_DefaultInterface, PhpDevPropFillString, DEVPROP_FILL_FLAG_CLASS_INTERFACE | DEVPROP_FILL_FLAG_CLASS_INSTALLER },
    { PhDevicePropertyInterfaceClassName, &DEVPKEY_DeviceInterfaceClass_Name, PhpDevPropFillString, DEVPROP_FILL_FLAG_CLASS_INTERFACE | DEVPROP_FILL_FLAG_CLASS_INSTALLER },

    { PhDevicePropertyContainerAddress, &DEVPKEY_DeviceContainer_Address, PhpDevPropFillStringOrStringList, 0 },
    { PhDevicePropertyContainerDiscoveryMethod, &DEVPKEY_DeviceContainer_DiscoveryMethod, PhpDevPropFillStringList, 0 },
    { PhDevicePropertyContainerIsEncrypted, &DEVPKEY_DeviceContainer_IsEncrypted, PhpDevPropFillBoolean, 0 },
    { PhDevicePropertyContainerIsAuthenticated, &DEVPKEY_DeviceContainer_IsAuthenticated, PhpDevPropFillBoolean, 0 },
    { PhDevicePropertyContainerIsConnected, &DEVPKEY_DeviceContainer_IsConnected, PhpDevPropFillBoolean, 0 },
    { PhDevicePropertyContainerIsPaired, &DEVPKEY_DeviceContainer_IsPaired, PhpDevPropFillBoolean, 0 },
    { PhDevicePropertyContainerIcon, &DEVPKEY_DeviceContainer_Icon, PhpDevPropFillString, 0 },
    { PhDevicePropertyContainerVersion, &DEVPKEY_DeviceContainer_Version, PhpDevPropFillString, 0 },
    { PhDevicePropertyContainerLastSeen, &DEVPKEY_DeviceContainer_Last_Seen, PhpDevPropFillTimeStamp, 0 },
    { PhDevicePropertyContainerLastConnected, &DEVPKEY_DeviceContainer_Last_Connected, PhpDevPropFillTimeStamp, 0 },
    { PhDevicePropertyContainerIsShowInDisconnectedState, &DEVPKEY_DeviceContainer_IsShowInDisconnectedState, PhpDevPropFillBoolean, 0 },
    { PhDevicePropertyContainerIsLocalMachine, &DEVPKEY_DeviceContainer_IsLocalMachine, PhpDevPropFillBoolean, 0 },
    { PhDevicePropertyContainerMetadataPath, &DEVPKEY_DeviceContainer_MetadataPath, PhpDevPropFillString, 0 },
    { PhDevicePropertyContainerIsMetadataSearchInProgress, &DEVPKEY_DeviceContainer_IsMetadataSearchInProgress, PhpDevPropFillBoolean, 0 },
    //{ , &DEVPKEY_DeviceContainer_MetadataChecksum, NULL, 0 },            // DEVPROP_TYPE_BINARY
    { PhDevicePropertyContainerIsNotInterestingForDisplay, &DEVPKEY_DeviceContainer_IsNotInterestingForDisplay, PhpDevPropFillBoolean, 0 },
    { PhDevicePropertyContainerLaunchDeviceStageOnDeviceConnect, &DEVPKEY_DeviceContainer_LaunchDeviceStageOnDeviceConnect, PhpDevPropFillBoolean, 0 },
    { PhDevicePropertyContainerLaunchDeviceStageFromExplorer, &DEVPKEY_DeviceContainer_LaunchDeviceStageFromExplorer, PhpDevPropFillBoolean, 0 },
    { PhDevicePropertyContainerBaselineExperienceId, &DEVPKEY_DeviceContainer_BaselineExperienceId, PhpDevPropFillGuid, 0 },
    { PhDevicePropertyContainerIsDeviceUniquelyIdentifiable, &DEVPKEY_DeviceContainer_IsDeviceUniquelyIdentifiable, PhpDevPropFillBoolean, 0 },
    { PhDevicePropertyContainerAssociationArray, &DEVPKEY_DeviceContainer_AssociationArray, PhpDevPropFillStringList, 0 },
    { PhDevicePropertyContainerDeviceDescription1, &DEVPKEY_DeviceContainer_DeviceDescription1, PhpDevPropFillString, 0 },
    { PhDevicePropertyContainerDeviceDescription2, &DEVPKEY_DeviceContainer_DeviceDescription2, PhpDevPropFillString, 0 },
    { PhDevicePropertyContainerHasProblem, &DEVPKEY_DeviceContainer_HasProblem, PhpDevPropFillBoolean, 0 },
    { PhDevicePropertyContainerIsSharedDevice, &DEVPKEY_DeviceContainer_IsSharedDevice, PhpDevPropFillBoolean, 0 },
    { PhDevicePropertyContainerIsNetworkDevice, &DEVPKEY_DeviceContainer_IsNetworkDevice, PhpDevPropFillBoolean, 0 },
    { PhDevicePropertyContainerIsDefaultDevice, &DEVPKEY_DeviceContainer_IsDefaultDevice, PhpDevPropFillBoolean, 0 },
    { PhDevicePropertyContainerMetadataCabinet, &DEVPKEY_DeviceContainer_MetadataCabinet, PhpDevPropFillString, 0 },
    { PhDevicePropertyContainerRequiresPairingElevation, &DEVPKEY_DeviceContainer_RequiresPairingElevation, PhpDevPropFillBoolean, 0 },
    { PhDevicePropertyContainerExperienceId, &DEVPKEY_DeviceContainer_ExperienceId, PhpDevPropFillGuid, 0 },
    { PhDevicePropertyContainerCategory, &DEVPKEY_DeviceContainer_Category, PhpDevPropFillStringList, 0 },
    { PhDevicePropertyContainerCategoryDescSingular, &DEVPKEY_DeviceContainer_Category_Desc_Singular, PhpDevPropFillStringList, 0 },
    { PhDevicePropertyContainerCategoryDescPlural, &DEVPKEY_DeviceContainer_Category_Desc_Plural, PhpDevPropFillStringList, 0 },
    { PhDevicePropertyContainerCategoryIcon, &DEVPKEY_DeviceContainer_Category_Icon, PhpDevPropFillString, 0 },
    { PhDevicePropertyContainerCategoryGroupDesc, &DEVPKEY_DeviceContainer_CategoryGroup_Desc, PhpDevPropFillStringList, 0 },
    { PhDevicePropertyContainerCategoryGroupIcon, &DEVPKEY_DeviceContainer_CategoryGroup_Icon, PhpDevPropFillString, 0 },
    { PhDevicePropertyContainerPrimaryCategory, &DEVPKEY_DeviceContainer_PrimaryCategory, PhpDevPropFillString, 0 },
    { PhDevicePropertyContainerUnpairUninstall, &DEVPKEY_DeviceContainer_UnpairUninstall, PhpDevPropFillBoolean, 0 },
    { PhDevicePropertyContainerRequiresUninstallElevation, &DEVPKEY_DeviceContainer_RequiresUninstallElevation, PhpDevPropFillBoolean, 0 },
    { PhDevicePropertyContainerDeviceFunctionSubRank, &DEVPKEY_DeviceContainer_DeviceFunctionSubRank, PhpDevPropFillUInt32, 0 },
    { PhDevicePropertyContainerAlwaysShowDeviceAsConnected, &DEVPKEY_DeviceContainer_AlwaysShowDeviceAsConnected, PhpDevPropFillBoolean, 0 },
    { PhDevicePropertyContainerConfigFlags, &DEVPKEY_DeviceContainer_ConfigFlags, PhpDevPropFillUInt32Hex, 0 },
    { PhDevicePropertyContainerPrivilegedPackageFamilyNames, &DEVPKEY_DeviceContainer_PrivilegedPackageFamilyNames, PhpDevPropFillStringList, 0 },
    { PhDevicePropertyContainerCustomPrivilegedPackageFamilyNames, &DEVPKEY_DeviceContainer_CustomPrivilegedPackageFamilyNames, PhpDevPropFillStringList, 0 },
    { PhDevicePropertyContainerIsRebootRequired, &DEVPKEY_DeviceContainer_IsRebootRequired, PhpDevPropFillBoolean, 0 },
    { PhDevicePropertyContainerFriendlyName, &DEVPKEY_DeviceContainer_FriendlyName, PhpDevPropFillString, 0 },
    { PhDevicePropertyContainerManufacturer, &DEVPKEY_DeviceContainer_Manufacturer, PhpDevPropFillString, 0 },
    { PhDevicePropertyContainerModelName, &DEVPKEY_DeviceContainer_ModelName, PhpDevPropFillString, 0 },
    { PhDevicePropertyContainerModelNumber, &DEVPKEY_DeviceContainer_ModelNumber, PhpDevPropFillString, 0 },
    { PhDevicePropertyContainerInstallInProgress, &DEVPKEY_DeviceContainer_InstallInProgress, PhpDevPropFillBoolean, 0 },

    { PhDevicePropertyObjectType, &DEVPKEY_DevQuery_ObjectType, PhpDevPropFillUInt32, 0 },

    { PhDevicePropertyPciInterruptSupport, &DEVPKEY_PciDevice_InterruptSupport, PhpDevPropFillPciDeviceInterruptSupport, 0 },
    { PhDevicePropertyPciExpressCapabilityControl, &DEVPKEY_PciRootBus_PCIExpressCapabilityControl, PhpDevPropFillBoolean, 0 },
    { PhDevicePropertyPciNativeExpressControl, &DEVPKEY_PciRootBus_NativePciExpressControl, PhpDevPropFillBoolean, 0 },
    { PhDevicePropertyPciSystemMsiSupport, &DEVPKEY_PciRootBus_SystemMsiSupport, PhpDevPropFillBoolean, 0 },

    { PhDevicePropertyStoragePortable, &DEVPKEY_Storage_Portable, PhpDevPropFillBoolean, 0 },
    { PhDevicePropertyStorageRemovableMedia, &DEVPKEY_Storage_Removable_Media, PhpDevPropFillBoolean, 0 },
    { PhDevicePropertyStorageSystemCritical, &DEVPKEY_Storage_System_Critical, PhpDevPropFillBoolean, 0 },
    { PhDevicePropertyStorageDiskNumber, &DEVPKEY_Storage_Disk_Number, PhpDevPropFillUInt32, 0 },
    { PhDevicePropertyStoragePartitionNumber, &DEVPKEY_Storage_Partition_Number, PhpDevPropFillUInt32, 0 },
};
C_ASSERT(RTL_NUMBER_OF(PhpDeviceItemPropertyTable) == PhMaxDeviceProperty);

#if DEBUG
static BOOLEAN PhpBreakOnDeviceProperty = FALSE;
static PH_DEVICE_PROPERTY_CLASS PhpBreakOnDevicePropertyClass = 0;
#endif

PPH_DEVICE_PROPERTY PhGetDeviceProperty(
    _In_ PPH_DEVICE_ITEM Item,
    _In_ PH_DEVICE_PROPERTY_CLASS Class
    )
{
    PPH_DEVICE_PROPERTY prop;

    prop = &Item->Properties[Class];
    if (!prop->Initialized)
    {
        const PH_DEVICE_PROPERTY_TABLE_ENTRY* entry;

        entry = &PhpDeviceItemPropertyTable[Class];

#if DEBUG
        if (PhpBreakOnDeviceProperty && (PhpBreakOnDevicePropertyClass == Class))
            __debugbreak();
#endif

        entry->Callback(
            Item->DeviceInfo->Handle,
            &Item->DeviceInfoData,
            entry->PropKey,
            prop,
            entry->CallbackFlags
            );

        prop->Initialized = TRUE;
    }

    return prop;
}

HICON PhGetDeviceIcon(
    _In_ PPH_DEVICE_ITEM Item,
    _In_ PPH_INTEGER_PAIR IconSize
    )
{
    HICON iconHandle;

    if (!SetupDiLoadDeviceIcon(
        Item->DeviceInfo->Handle,
        &Item->DeviceInfoData,
        IconSize->X,
        IconSize->Y,
        0,
        &iconHandle
        ))
    {
        iconHandle = NULL;
    }

    return iconHandle;
}

PPH_DEVICE_TREE PhReferenceDeviceTree(
    VOID
    )
{
    PPH_DEVICE_TREE deviceTree;

    PhAcquireFastLockShared(&PhpDeviceTreeLock);
    PhSetReference(&deviceTree, PhpDeviceTree);
    PhReleaseFastLockShared(&PhpDeviceTreeLock);

    return deviceTree;
}

ULONG PhpGenerateInstanceIdHash(
    _In_ PPH_STRINGREF InstanceId
    )
{
    return PhHashStringRefEx(InstanceId, TRUE, PH_STRING_HASH_X65599);
}

static int __cdecl PhpDeviceItemSortFunction(
    const void* Left,
    const void* Right
    )
{
    PPH_DEVICE_ITEM lhsItem;
    PPH_DEVICE_ITEM rhsItem;

    lhsItem = *(PPH_DEVICE_ITEM*)Left;
    rhsItem = *(PPH_DEVICE_ITEM*)Right;

    if (lhsItem->InstanceIdHash < rhsItem->InstanceIdHash)
        return -1;
    else if (lhsItem->InstanceIdHash > rhsItem->InstanceIdHash)
        return 1;
    else
        return 0;
}

static int __cdecl PhpDeviceItemSearchFunction(
    const void* Hash,
    const void* Item 
    )
{
    PPH_DEVICE_ITEM item;

    item = *(PPH_DEVICE_ITEM*)Item;

    if (PtrToUlong(Hash) < item->InstanceIdHash)
        return -1;
    else if (PtrToUlong(Hash) > item->InstanceIdHash)
        return 1;
    else
        return 0;
}

_Success_(return != NULL)
_Must_inspect_result_
PPH_DEVICE_ITEM PhLookupDeviceItem(
    _In_ PPH_DEVICE_TREE Tree,
    _In_ PPH_STRINGREF InstanceId
    )
{
    ULONG hash;
    PPH_DEVICE_ITEM* deviceItem;

    hash = PhpGenerateInstanceIdHash(InstanceId);

    deviceItem = bsearch(
        UlongToPtr(hash),
        Tree->DeviceList->Items,
        Tree->DeviceList->Count,
        sizeof(PVOID),
        PhpDeviceItemSearchFunction
        );

    return deviceItem ? *deviceItem : NULL;
}

_Success_(return != NULL)
_Must_inspect_result_
PPH_DEVICE_ITEM PhReferenceDeviceItem(
    _In_ PPH_DEVICE_TREE Tree,
    _In_ PPH_STRINGREF InstanceId
    )
{
    PPH_DEVICE_ITEM deviceItem;

    PhSetReference(&deviceItem, PhLookupDeviceItem(Tree, InstanceId));

    return deviceItem;
}

_Success_(return != NULL)
_Must_inspect_result_
PPH_DEVICE_ITEM PhReferenceDeviceItem2(
    _In_ PPH_STRINGREF InstanceId
    )
{
    PPH_DEVICE_TREE deviceTree;
    PPH_DEVICE_ITEM deviceItem;

    deviceTree = PhReferenceDeviceTree();
    PhSetReference(&deviceItem, PhLookupDeviceItem(deviceTree, InstanceId));
    PhDereferenceObject(deviceTree);

    return deviceItem;
}

VOID PhpDeviceInfoDeleteProcedure(
    _In_ PVOID Object,
    _In_ ULONG Flags
    )
{
    PPH_DEVINFO info = Object;

    if (info->Handle != INVALID_HANDLE_VALUE)
    {
        SetupDiDestroyDeviceInfoList(info->Handle);
        info->Handle = INVALID_HANDLE_VALUE;
    }
}

VOID PhpDeviceItemDeleteProcedure(
    _In_ PVOID Object,
    _In_ ULONG Flags
    )
{
    PPH_DEVICE_ITEM item = Object;

    PhDereferenceObject(item->Children);

    for (ULONG i = 0; i < ARRAYSIZE(item->Properties); i++)
    {
        PPH_DEVICE_PROPERTY prop;

        prop = &item->Properties[i];

        if (prop->Valid)
        {
            if (prop->Type == PhDevicePropertyTypeString)
            {
                PhDereferenceObject(prop->String);
            }
            else if (prop->Type == PhDevicePropertyTypeStringList)
            {
                for (ULONG j = 0; j < prop->StringList->Count; j++)
                {
                    PhDereferenceObject(prop->StringList->Items[j]);
                }

                PhDereferenceObject(prop->StringList);
            }
        }

        PhClearReference(&prop->AsString);
    }

    PhClearReference(&item->InstanceId);
    PhClearReference(&item->ParentInstanceId);
}

VOID PhpDeviceTreeDeleteProcedure(
    _In_ PVOID Object,
    _In_ ULONG Flags
    )
{
    PPH_DEVICE_TREE tree = Object;

    for (ULONG i = 0; i < tree->DeviceList->Count; i++)
    {
        PPH_DEVICE_ITEM item;

        item = tree->DeviceList->Items[i];

        PhDereferenceObject(item);
    }

    PhDereferenceObject(tree->DeviceList);
    PhDereferenceObject(tree->DeviceInfo);
}

VOID PhpDeviceNotifyDeleteProcedure(
    _In_ PVOID Object,
    _In_ ULONG Flags
    )
{
    PPH_DEVICE_NOTIFY notify = Object;

    if (notify->Action == PhDeviceNotifyInstanceEnumerated ||
        notify->Action == PhDeviceNotifyInstanceStarted ||
        notify->Action == PhDeviceNotifyInstanceRemoved)
    {
        PhDereferenceObject(notify->DeviceInstance.InstanceId);
    }
}

PPH_DEVICE_ITEM NTAPI PhpAddDeviceItem(
    _In_ PPH_DEVICE_TREE Tree,
    _In_ PSP_DEVINFO_DATA DeviceInfoData
    )
{
    PPH_DEVICE_ITEM item;

    item = PhCreateObjectZero(sizeof(PH_DEVICE_ITEM), PhDeviceItemType);

    item->DeviceInfo = PhReferenceObject(Tree->DeviceInfo);
    RtlCopyMemory(&item->DeviceInfoData, DeviceInfoData, sizeof(SP_DEVINFO_DATA));
    RtlCopyMemory(&item->ClassGuid, &DeviceInfoData->ClassGuid, sizeof(GUID));

    item->Children = PhCreateList(1);

    //
    // Only get the minimum here, the rest will be retrieved later if necessary.
    // For convenience later, other frequently referenced items are gotten here too.
    //

    item->InstanceId = PhGetDeviceProperty(item, PhDevicePropertyInstanceId)->AsString;
    if (item->InstanceId)
    {
        item->InstanceIdHash = PhpGenerateInstanceIdHash(&item->InstanceId->sr);

        //
        // If this is the root enumerator override some properties.
        //
        if (PhEqualStringRef(&item->InstanceId->sr, &RootInstanceId, TRUE))
        {
            PPH_DEVICE_PROPERTY prop;

            prop = &item->Properties[PhDevicePropertyName];
            assert(!prop->Initialized);

            prop->AsString = PhGetActiveComputerName();
            prop->Initialized = TRUE;
        }

        PhReferenceObject(item->InstanceId);
    }

    item->ParentInstanceId = PhGetDeviceProperty(item, PhDevicePropertyParentInstanceId)->AsString;
    if (item->ParentInstanceId)
        PhReferenceObject(item->ParentInstanceId);

    PhGetDeviceProperty(item, PhDevicePropertyProblemCode);
    if (item->Properties[PhDevicePropertyProblemCode].Valid)
        item->ProblemCode = item->Properties[PhDevicePropertyProblemCode].UInt32;
    else
        item->ProblemCode = CM_PROB_PHANTOM;

    PhGetDeviceProperty(item, PhDevicePropertyDevNodeStatus);
    if (item->Properties[PhDevicePropertyDevNodeStatus].Valid)
        item->DevNodeStatus = item->Properties[PhDevicePropertyDevNodeStatus].UInt32;
    else
        item->DevNodeStatus = 0;

    PhGetDeviceProperty(item, PhDevicePropertyDevCapabilities);
    if (item->Properties[PhDevicePropertyDevCapabilities].Valid)
        item->Capabilities = item->Properties[PhDevicePropertyDevCapabilities].UInt32;
    else
        item->Capabilities = 0;

    PhGetDeviceProperty(item, PhDevicePropertyUpperFilters);
    PhGetDeviceProperty(item, PhDevicePropertyClassUpperFilters);
    if ((item->Properties[PhDevicePropertyUpperFilters].Valid &&
         (item->Properties[PhDevicePropertyUpperFilters].StringList->Count > 0)) ||
        (item->Properties[PhDevicePropertyClassUpperFilters].Valid &&
         (item->Properties[PhDevicePropertyClassUpperFilters].StringList->Count > 0)))
    {
        item->HasUpperFilters = TRUE;
    }

    PhGetDeviceProperty(item, PhDevicePropertyLowerFilters);
    PhGetDeviceProperty(item, PhDevicePropertyClassLowerFilters);
    if ((item->Properties[PhDevicePropertyLowerFilters].Valid &&
         (item->Properties[PhDevicePropertyLowerFilters].StringList->Count > 0)) ||
        (item->Properties[PhDevicePropertyClassLowerFilters].Valid &&
         (item->Properties[PhDevicePropertyClassLowerFilters].StringList->Count > 0)))
    {
        item->HasLowerFilters = TRUE;
    }

    PhAddItemList(Tree->DeviceList, item);

    return item;
}

PPH_DEVICE_TREE PhpCreateDeviceTree(
    VOID
    )
{
    PPH_DEVICE_TREE tree;
    PPH_DEVICE_ITEM root;

    tree = PhCreateObjectZero(sizeof(PH_DEVICE_TREE), PhDeviceTreeType);

    tree->DeviceList = PhCreateList(40);
    tree->DeviceInfo = PhCreateObject(sizeof(PH_DEVINFO), PhpDeviceInfoType);

    tree->DeviceInfo->Handle = SetupDiGetClassDevsW(
        NULL,
        NULL,
        NULL,
        DIGCF_ALLCLASSES
        );
    if (tree->DeviceInfo->Handle == INVALID_HANDLE_VALUE)
    {
        return tree;
    }

    for (ULONG i = 0;; i++)
    {
        SP_DEVINFO_DATA deviceInfoData;

        memset(&deviceInfoData, 0, sizeof(SP_DEVINFO_DATA));
        deviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);

        if (!SetupDiEnumDeviceInfo(tree->DeviceInfo->Handle, i, &deviceInfoData))
            break;

        PhpAddDeviceItem(tree, &deviceInfoData);
    }

    // Link the device relations.
    root = NULL;

    for (ULONG i = 0; i < tree->DeviceList->Count; i++)
    {
        BOOLEAN found;
        PPH_DEVICE_ITEM item;
        PPH_DEVICE_ITEM other;

        found = FALSE;

        item = tree->DeviceList->Items[i];

        for (ULONG j = 0; j < tree->DeviceList->Count; j++)
        {
            other = tree->DeviceList->Items[j];

            if (item == other)
            {
                continue;
            }

            if (!other->InstanceId || !item->ParentInstanceId)
            {
                continue;
            }

            if (!PhEqualString(other->InstanceId, item->ParentInstanceId, TRUE))
            {
                continue;
            }

            found = TRUE;

            item->Parent = other;

            if (!other->Child)
            {
                other->Child = item;
                break;
            }

            other = other->Child;
            while (other->Sibling)
            {
                other = other->Sibling;
            }

            other->Sibling = item;
            break;
        }

        if (found)
        {
            continue;
        }

        other = root;
        if (!other)
        {
            root = item;
            continue;
        }

        while (other->Sibling)
        {
            other = other->Sibling;
        }

        other->Sibling = item;
    }

    for (ULONG i = 0; i < tree->DeviceList->Count; i++)
    {
        PPH_DEVICE_ITEM item;
        PPH_DEVICE_ITEM child;

        item = tree->DeviceList->Items[i];

        child = item->Child;
        while (child)
        {
            PhAddItemList(item->Children, child);
            child = child->Sibling;
        }
    }

    assert(root && !root->Sibling);
    tree->Root = root;

    // sort the list for faster lookups later
    qsort(
        tree->DeviceList->Items,
        tree->DeviceList->Count,
        sizeof(PVOID),
        PhpDeviceItemSortFunction
        );

    return tree;
}

VOID PhpDeviceNotify(
    _In_ PLIST_ENTRY List
    )
{
    PPH_DEVICE_TREE newTree;
    PPH_DEVICE_TREE oldTree;

    // We process device notifications in blocks so that bursts of device changes
    // don't each trigger a new tree each time.

    newTree = PhpCreateDeviceTree();
    PhAcquireFastLockExclusive(&PhpDeviceTreeLock);
    oldTree = PhpDeviceTree;
    PhpDeviceTree = newTree;
    PhReleaseFastLockExclusive(&PhpDeviceTreeLock);

    while (!IsListEmpty(List))
    {
        PPH_DEVICE_NOTIFY entry;

        entry = CONTAINING_RECORD(RemoveHeadList(List), PH_DEVICE_NOTIFY, ListEntry);

        PhInvokeCallback(PhGetGeneralCallback(GeneralCallbackDeviceNotificationEvent), entry);

        PhDereferenceObject(entry);
    }

    PhDereferenceObject(oldTree);
}

_Function_class_(PUSER_THREAD_START_ROUTINE)
NTSTATUS NTAPI PhpDeviceNotifyWorker(
    _In_ PVOID ThreadParameter
    )
{
    PhSetThreadName(NtCurrentThread(), L"DeviceNotifyWorker");

    for (;;)
    {
        LIST_ENTRY list;

        // delay to process events in blocks
        PhDelayExecution(1000);

        PhAcquireFastLockExclusive(&PhpDeviceNotifyLock);

        if (IsListEmpty(&PhpDeviceNotifyList))
        {
            NtResetEvent(PhpDeviceNotifyEvent, NULL);
            PhReleaseFastLockExclusive(&PhpDeviceNotifyLock);
            NtWaitForSingleObject(PhpDeviceNotifyEvent, FALSE, NULL);
            continue;
        }

        // drain the list
        list = PhpDeviceNotifyList;
        list.Flink->Blink = &list;
        list.Blink->Flink = &list;
        InitializeListHead(&PhpDeviceNotifyList);

        PhReleaseFastLockExclusive(&PhpDeviceNotifyLock);

        PhpDeviceNotify(&list);
    }

    return STATUS_SUCCESS;
}

_Function_class_(PCM_NOTIFY_CALLBACK)
ULONG CALLBACK PhpCmNotifyCallback(
    _In_ HCMNOTIFICATION hNotify,
    _In_opt_ PVOID Context,
    _In_ CM_NOTIFY_ACTION Action,
    _In_reads_bytes_(EventDataSize) PCM_NOTIFY_EVENT_DATA EventData,
    _In_ ULONG EventDataSize
    )
{
    PPH_DEVICE_NOTIFY entry = PhCreateObjectZero(sizeof(PH_DEVICE_NOTIFY), PhDeviceNotifyType);

    switch (Action)
    {
    case CM_NOTIFY_ACTION_DEVICEINTERFACEARRIVAL:
        {
            entry->Action = PhDeviceNotifyInterfaceArrival;
            RtlCopyMemory(&entry->DeviceInterface.ClassGuid, &EventData->u.DeviceInterface.ClassGuid, sizeof(GUID));
        }
        break;
    case CM_NOTIFY_ACTION_DEVICEINTERFACEREMOVAL:
        {
            entry->Action = PhDeviceNotifyInterfaceRemoval;
            RtlCopyMemory(&entry->DeviceInterface.ClassGuid, &EventData->u.DeviceInterface.ClassGuid, sizeof(GUID));
        }
        break;
    case CM_NOTIFY_ACTION_DEVICEINSTANCEENUMERATED:
        {
            entry->Action = PhDeviceNotifyInstanceEnumerated;
            entry->DeviceInstance.InstanceId = PhCreateString(EventData->u.DeviceInstance.InstanceId);
        }
        break;
    case CM_NOTIFY_ACTION_DEVICEINSTANCESTARTED:
        {
            entry->Action = PhDeviceNotifyInstanceStarted;
            entry->DeviceInstance.InstanceId = PhCreateString(EventData->u.DeviceInstance.InstanceId);
        }
        break;
    case CM_NOTIFY_ACTION_DEVICEINSTANCEREMOVED:
        {
            entry->Action = PhDeviceNotifyInstanceRemoved;
            entry->DeviceInstance.InstanceId = PhCreateString(EventData->u.DeviceInstance.InstanceId);
        }
        break;
    default:
        return ERROR_SUCCESS;
    }

    PhAcquireFastLockExclusive(&PhpDeviceNotifyLock);
    InsertTailList(&PhpDeviceNotifyList, &entry->ListEntry);
    NtSetEvent(PhpDeviceNotifyEvent, NULL);
    PhReleaseFastLockExclusive(&PhpDeviceNotifyLock);

    return ERROR_SUCCESS;
}

BOOLEAN PhDeviceProviderInitialization(
    VOID
    )
{
    NTSTATUS status;
    CM_NOTIFY_FILTER cmFilter;

    if (WindowsVersion < WINDOWS_10 || !PhGetIntegerSetting(L"EnableDeviceSupport"))
        return TRUE;

    PhDeviceItemType = PhCreateObjectType(L"DeviceItem", 0, PhpDeviceItemDeleteProcedure);
    PhDeviceTreeType = PhCreateObjectType(L"DeviceTree", 0, PhpDeviceTreeDeleteProcedure);
    PhDeviceNotifyType = PhCreateObjectType(L"DeviceNotify", 0, PhpDeviceNotifyDeleteProcedure);
    PhpDeviceInfoType = PhCreateObjectType(L"DeviceInfo", 0, PhpDeviceInfoDeleteProcedure);

    PhpDeviceTree = PhpCreateDeviceTree();

    InitializeListHead(&PhpDeviceNotifyList);
    if (!NT_SUCCESS(status = NtCreateEvent(&PhpDeviceNotifyEvent, EVENT_ALL_ACCESS, NULL, NotificationEvent, FALSE)))
        return FALSE;
    if (!NT_SUCCESS(status = PhCreateThread2(PhpDeviceNotifyWorker, NULL)))
        return FALSE;

    RtlZeroMemory(&cmFilter, sizeof(CM_NOTIFY_FILTER));
    cmFilter.cbSize = sizeof(CM_NOTIFY_FILTER);

    cmFilter.FilterType = CM_NOTIFY_FILTER_TYPE_DEVICEINSTANCE;
    cmFilter.Flags = CM_NOTIFY_FILTER_FLAG_ALL_DEVICE_INSTANCES;
    if (CM_Register_Notification(
        &cmFilter,
        NULL,
        PhpCmNotifyCallback,
        &PhpDeviceNotification
        ) != CR_SUCCESS)
    {
        return FALSE;
    }

    cmFilter.FilterType = CM_NOTIFY_FILTER_TYPE_DEVICEINTERFACE;
    cmFilter.Flags = CM_NOTIFY_FILTER_FLAG_ALL_INTERFACE_CLASSES;
    if (CM_Register_Notification(
        &cmFilter,
        NULL,
        PhpCmNotifyCallback,
        &PhpDeviceInterfaceNotification
        ) != CR_SUCCESS)
    {
        return FALSE;
    }

    return TRUE;
}
