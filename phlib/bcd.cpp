/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     dmex    2021-2022
 *
 */

#include <ph.h>
#include <bcd.h>

static PVOID BcdDllBaseAddress = nullptr;
static decltype(&BcdOpenSystemStore) BcdOpenSystemStore_I = nullptr;
static decltype(&BcdCloseStore) BcdCloseStore_I = nullptr;
static decltype(&BcdOpenObject) BcdOpenObject_I = nullptr;
static decltype(&BcdCloseObject) BcdCloseObject_I = nullptr;
static decltype(&BcdGetElementData) BcdGetElementData_I = nullptr;
static decltype(&BcdSetElementData) BcdSetElementData_I = nullptr;
//static decltype(&BcdDeleteElement) BcdDeleteElement_I = nullptr;
static decltype(&BcdEnumerateObjects) BcdEnumerateObjects_I = nullptr;

static BOOLEAN PhpBcdApiInitialized(
    VOID
    )
{
    static PH_INITONCE initOnce = PH_INITONCE_INIT;
    static BOOLEAN bcdInitialized = FALSE;

    if (PhBeginInitOnce(&initOnce))
    {
        if (BcdDllBaseAddress = PhLoadLibrary(L"bcd.dll"))
        {
            BcdOpenSystemStore_I = reinterpret_cast<decltype(&BcdOpenSystemStore)>(PhGetDllBaseProcedureAddress(BcdDllBaseAddress, const_cast<char*>("BcdOpenSystemStore"), 0));
            BcdCloseStore_I = reinterpret_cast<decltype(&BcdCloseStore)>(PhGetDllBaseProcedureAddress(BcdDllBaseAddress, const_cast<char*>("BcdCloseStore"), 0));
            BcdOpenObject_I = reinterpret_cast<decltype(&BcdOpenObject)>(PhGetDllBaseProcedureAddress(BcdDllBaseAddress, const_cast<char*>("BcdOpenObject"), 0));
            BcdCloseObject_I = reinterpret_cast<decltype(&BcdCloseObject)>(PhGetDllBaseProcedureAddress(BcdDllBaseAddress, const_cast<char*>("BcdCloseObject"), 0));
            BcdGetElementData_I = reinterpret_cast<decltype(&BcdGetElementData)>(PhGetDllBaseProcedureAddress(BcdDllBaseAddress, const_cast<char*>("BcdGetElementData"), 0));
            BcdSetElementData_I = reinterpret_cast<decltype(&BcdSetElementData)>(PhGetDllBaseProcedureAddress(BcdDllBaseAddress, const_cast<char*>("BcdSetElementData"), 0));
            //BcdDeleteElement_I = reinterpret_cast<decltype(&BcdDeleteElement)>(PhGetDllBaseProcedureAddress(BcdDllBaseAddress, const_cast<char*>("BcdDeleteElement"), 0));
        }

        if (
            BcdOpenSystemStore_I &&
            BcdCloseStore_I &&
            BcdOpenObject_I &&
            BcdCloseObject_I &&
            BcdGetElementData_I &&
            BcdSetElementData_I
            )
        {
            bcdInitialized = TRUE;
        }

        PhEndInitOnce(&initOnce);
    }

    return bcdInitialized;
}

NTSTATUS PhBcdOpenSystemStore(
    _Out_ PHANDLE StoreHandle
    )
{
    NTSTATUS status;
    HANDLE bcdStoreHandle;

    if (!PhpBcdApiInitialized())
        return STATUS_UNSUCCESSFUL;

    status = BcdOpenSystemStore_I(
        &bcdStoreHandle
        );

    if (NT_SUCCESS(status))
    {
        *StoreHandle = bcdStoreHandle;
    }

    return status;
}

NTSTATUS PhBcdCloseStore(
    _In_ HANDLE StoreHandle
    )
{
    if (!PhpBcdApiInitialized())
        return STATUS_UNSUCCESSFUL;

    return BcdCloseStore_I(StoreHandle);
}

NTSTATUS PhBcdOpenObject(
    _In_ HANDLE StoreHandle,
    _In_ PGUID Identifier,
    _Out_ PHANDLE ObjectHandle
    )
{
    NTSTATUS status;
    HANDLE bcdObjectHandle;

    if (!PhpBcdApiInitialized())
        return STATUS_UNSUCCESSFUL;

    status = BcdOpenObject_I(
        StoreHandle,
        Identifier,
        &bcdObjectHandle
        );

    if (NT_SUCCESS(status))
    {
        *ObjectHandle = bcdObjectHandle;
    }

    return status;
}

NTSTATUS PhBcdCloseObject(
    _In_ HANDLE ObjectHandle
    )
{
    if (!PhpBcdApiInitialized())
        return STATUS_UNSUCCESSFUL;

    return BcdCloseObject_I(ObjectHandle);
}

NTSTATUS PhBcdGetElementData(
    _In_ HANDLE ObjectHandle,
    _In_ ULONG ElementType,
    _Out_writes_bytes_opt_(*BufferSize) PVOID Buffer,
    _Inout_ PULONG BufferSize
    )
{
    if (!PhpBcdApiInitialized())
        return STATUS_UNSUCCESSFUL;

    return BcdGetElementData_I(
        ObjectHandle,
        ElementType,
        Buffer,
        BufferSize
        );
}

NTSTATUS PhBcdSetElementData(
    _In_ HANDLE ObjectHandle,
    _In_ ULONG ElementType,
    _In_reads_bytes_opt_(BufferSize) PVOID Buffer,
    _In_ ULONG BufferSize
    )
{
    if (!PhpBcdApiInitialized())
        return STATUS_UNSUCCESSFUL;

    return BcdSetElementData_I(
        ObjectHandle,
        ElementType,
        Buffer,
        BufferSize
        );
}

//NTSTATUS PhBcdDeleteElement(
//    _In_ HANDLE ObjectHandle,
//    _In_ ULONG ElementType
//    )
//{
//    if (!PhpBcdApiInitialized())
//        return STATUS_UNSUCCESSFUL;
//
//    return BcdDeleteElement_I(
//        ObjectHandle,
//        ElementType
//        );
//}

NTSTATUS PhBcdEnumerateObjects(
    _In_ HANDLE StoreHandle,
    _In_ PBCD_OBJECT_DESCRIPTION EnumDescriptor,
    _Out_writes_bytes_opt_(*BufferSize) PVOID Buffer,
    _Inout_ PULONG BufferSize,
    _Out_ PULONG ObjectCount
    )
{
    if (!PhpBcdApiInitialized())
        return STATUS_UNSUCCESSFUL;

    if (!BcdEnumerateObjects_I)
    {
        BcdEnumerateObjects_I = reinterpret_cast<decltype(&BcdEnumerateObjects)>(
            PhGetDllBaseProcedureAddress(BcdDllBaseAddress, const_cast<char*>("BcdEnumerateObjects"), 0));
    }

    if (!BcdEnumerateObjects_I)
        return STATUS_UNSUCCESSFUL;

    return BcdEnumerateObjects_I(
        StoreHandle,
        EnumDescriptor,
        Buffer,
        BufferSize,
        ObjectCount
        );
}

NTSTATUS PhBcdGetElementString(
    _In_ HANDLE ObjectHandle,
    _In_ ULONG ElementType,
    _Out_opt_ PPH_STRING* ElementString
    )
{
    NTSTATUS status;
    PPH_STRING string;
    ULONG stringLength;

    assert(GET_BCDE_DATA_FORMAT(ElementType) == BCD_ELEMENT_DATATYPE_FORMAT_STRING);

    if (!PhpBcdApiInitialized())
        return STATUS_UNSUCCESSFUL;

    stringLength = 0x80;
    string = PhCreateStringEx(nullptr, stringLength);
    status = PhBcdGetElementData(
        ObjectHandle,
        ElementType,
        string->Buffer,
        &stringLength
        );

    if (status == STATUS_BUFFER_TOO_SMALL)
    {
        PhDereferenceObject(string);
        string = PhCreateStringEx(nullptr, stringLength);

        status = PhBcdGetElementData(
            ObjectHandle,
            ElementType,
            string->Buffer,
            &stringLength
            );
    }

    if (NT_SUCCESS(status))
    {
        string->Length = stringLength - sizeof(UNICODE_NULL);

        if (ElementString)
            *ElementString = string;
        else
            PhDereferenceObject(string);

        return STATUS_SUCCESS;
    }

    PhDereferenceObject(string);

    return status;
}

NTSTATUS PhBcdGetElementUlong64(
    _In_ HANDLE ObjectHandle,
    _In_ ULONG ElementType,
    _Out_ PULONG64 ElementValue
    )
{
    NTSTATUS status;
    ULONG valueLength = sizeof(ULONG64);
    ULONG64 valueBuffer = 0;

    assert(GET_BCDE_DATA_FORMAT(ElementType) == BCD_ELEMENT_DATATYPE_FORMAT_INTEGER);

    if (!PhpBcdApiInitialized())
        return STATUS_UNSUCCESSFUL;

    status = PhBcdGetElementData(
        ObjectHandle,
        ElementType,
        &valueBuffer,
        &valueLength
        );

    if (NT_SUCCESS(status))
    {
        *ElementValue = valueBuffer;
    }

    return status;
}

NTSTATUS PhBcdGetElementBoolean(
    _In_ HANDLE ObjectHandle,
    _In_ ULONG ElementType,
    _Out_ PBOOLEAN ElementValue
    )
{
    NTSTATUS status;
    ULONG valueLength = sizeof(BOOLEAN);
    BOOLEAN valueBuffer = FALSE;

    assert(GET_BCDE_DATA_FORMAT(ElementType) == BCD_ELEMENT_DATATYPE_FORMAT_BOOLEAN);

    if (!PhpBcdApiInitialized())
        return STATUS_UNSUCCESSFUL;

    status = PhBcdGetElementData(
        ObjectHandle,
        ElementType,
        &valueBuffer,
        &valueLength
        );

    if (NT_SUCCESS(status))
    {
        *ElementValue = valueBuffer;
    }

    return status;
}

NTSTATUS PhBcdGetElementData2(
    _In_ ULONG ElementType,
    _Out_writes_bytes_opt_(*BufferSize) PVOID Buffer,
    _Inout_ PULONG BufferSize
    )
{
    NTSTATUS status;
    HANDLE storeHandle = nullptr;
    HANDLE objectHandle = nullptr;

    if (!PhpBcdApiInitialized())
        return STATUS_UNSUCCESSFUL;

    status = BcdOpenSystemStore_I(
        &storeHandle
        );

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    status = BcdOpenObject_I(
        storeHandle,
        &GUID_CURRENT_BOOT_ENTRY,
        &objectHandle
        );

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    status = BcdGetElementData_I(
        objectHandle,
        ElementType,
        Buffer,
        BufferSize
        );

CleanupExit:
    if (objectHandle)
        PhBcdCloseObject(objectHandle);
    if (storeHandle)
        PhBcdCloseStore(storeHandle);

    return status;
}

NTSTATUS PhBcdSetElementData2(
    _In_ ULONG ElementType,
    _In_reads_bytes_opt_(BufferSize) PVOID Buffer,
    _In_ ULONG BufferSize
    )
{
    NTSTATUS status;
    HANDLE storeHandle = nullptr;
    HANDLE objectHandle = nullptr;

    if (!PhpBcdApiInitialized())
        return STATUS_UNSUCCESSFUL;

    status = BcdOpenSystemStore_I(
        &storeHandle
        );

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    status = BcdOpenObject_I(
        storeHandle,
        &GUID_CURRENT_BOOT_ENTRY,
        &objectHandle
        );

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    status = BcdSetElementData_I(
        objectHandle,
        ElementType,
        Buffer,
        BufferSize
        );

CleanupExit:
    if (objectHandle)
        BcdCloseObject_I(objectHandle);
    if (storeHandle)
        BcdCloseStore_I(storeHandle);

    return status;
}

NTSTATUS PhBcdSetAdvancedOptionsOneTime(
    _In_ BOOLEAN Enable
    )
{
    NTSTATUS status;
    HANDLE storeHandle = nullptr;
    HANDLE objectHandle = nullptr;

    status = PhBcdOpenSystemStore(&storeHandle);

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    status = PhBcdOpenObject(
        storeHandle,
        &GUID_CURRENT_BOOT_ENTRY,
        &objectHandle
        );

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    status = PhBcdSetElementData(
        objectHandle,
        BcdOSLoaderBoolean_AdvancedOptionsOneTime,
        &Enable,
        sizeof(BOOLEAN)
        );

CleanupExit:
    if (objectHandle)
        PhBcdCloseObject(objectHandle);
    if (storeHandle)
        PhBcdCloseStore(storeHandle);

    return status;
}

NTSTATUS PhBcdSetBootApplicationOneTime(
    _In_ GUID Identifier,
    _In_opt_ BOOLEAN UpdateOneTimeFirmware
    )
{
    NTSTATUS status;
    HANDLE storeHandle = nullptr;
    HANDLE objectHandle = nullptr;
    BCD_ELEMENT_OBJECT_LIST objectElementList[1];

    status = PhBcdOpenSystemStore(&storeHandle);

    if (!NT_SUCCESS(status))
        return status;

    if (UpdateOneTimeFirmware)
    {
        HANDLE objectFirmwareHandle;

        // The user might have a third party boot loader where the Windows NT {bootmgr} 
        // is NOT the default {fwbootmgr} entry. So make the reboot seemless/effortless by
        // synchronizing the {fwbootmgr} one-time option to the Windows NT {bootmgr}.
        // This is a QOL optimization so you don't have to manually select Windows
        // first for it to then launch the boot application we already selected. (dmex)

        if (NT_SUCCESS(PhBcdOpenObject(
            storeHandle,
            &GUID_FIRMWARE_BOOTMGR,
            &objectFirmwareHandle
            )))
        {
            BCD_ELEMENT_OBJECT_LIST objectFirmwareList[32] = { 0 }; // dynamic?
            ULONG objectFirmwareListLength = sizeof(objectFirmwareList);

            if (NT_SUCCESS(PhBcdGetElementData(
                objectFirmwareHandle,
                BcdBootMgrObjectList_DisplayOrder,
                objectFirmwareList,
                &objectFirmwareListLength
                )))
            {
                // Check if the default entry is some third party application.
                if (!IsEqualGUID(GUID_WINDOWS_BOOTMGR, objectFirmwareList->ObjectList[0]))
                {
                    BCD_ELEMENT_OBJECT_LIST firmwareOneTimeBootEntry[1];

                    firmwareOneTimeBootEntry->ObjectList[0] = GUID_WINDOWS_BOOTMGR;

                    // Set the Windows NT {bootmgr} the one-time {fwbootmgr} entry.
                    status = PhBcdSetElementData(
                        objectFirmwareHandle,
                        BcdBootMgrObjectList_BootSequence,
                        firmwareOneTimeBootEntry,
                        sizeof(firmwareOneTimeBootEntry)
                        );
                }
            }

            PhBcdCloseObject(objectFirmwareHandle);
        }
    }

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    status = PhBcdOpenObject(
        storeHandle,
        &GUID_WINDOWS_BOOTMGR,
        &objectHandle
        );

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    objectElementList->ObjectList[0] = Identifier;

    status = PhBcdSetElementData(
        objectHandle,
        BcdBootMgrObjectList_BootSequence,
        objectElementList,
        sizeof(objectElementList)
        );

CleanupExit:
    if (objectHandle)
        PhBcdCloseObject(objectHandle);
    if (storeHandle)
        PhBcdCloseStore(storeHandle);

    return status;
}

NTSTATUS PhBcdSetFirmwareBootApplicationOneTime(
    _In_ GUID Identifier
    )
{
    NTSTATUS status;
    HANDLE storeHandle = nullptr;
    HANDLE objectHandle = nullptr;
    BCD_ELEMENT_OBJECT_LIST objectElementList[1];

    status = PhBcdOpenSystemStore(&storeHandle);

    if (!NT_SUCCESS(status))
        return status;

    status = PhBcdOpenObject(
        storeHandle,
        &GUID_FIRMWARE_BOOTMGR,
        &objectHandle
        );

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    objectElementList->ObjectList[0] = Identifier;

    status = PhBcdSetElementData(
        objectHandle,
        BcdBootMgrObjectList_BootSequence,
        objectElementList,
        sizeof(objectElementList)
        );

CleanupExit:
    if (objectHandle)
        PhBcdCloseObject(objectHandle);
    if (storeHandle)
        PhBcdCloseStore(storeHandle);

    return status;
}

static VOID PhpBcdEnumerateOsLoaderList(
    _In_ HANDLE StoreHandle,
    _In_ PPH_LIST ObjectList
    )
{
    NTSTATUS status;
    ULONG objectCount = 0;
    ULONG objectSize = 0;
    PBCD_OBJECT object = nullptr;
    BCD_OBJECT_DESCRIPTION description;

    description.Version = BCD_OBJECT_DESCRIPTION_VERSION;
    description.Type = BCD_OBJECT_OSLOADER_TYPE; // restrict enumeration to OSLOADER types

    status = PhBcdEnumerateObjects(
        StoreHandle,
        &description,
        object,
        &objectSize,
        &objectCount
        );

    if (status == STATUS_BUFFER_TOO_SMALL)
    {
        object = static_cast<PBCD_OBJECT>(PhAllocate(objectSize));
        memset(object, 0, objectSize);

        status = PhBcdEnumerateObjects(
            StoreHandle,
            &description,
            object,
            &objectSize,
            &objectCount
            );
    }

    if (!NT_SUCCESS(status) || !object)
    {
        if (object)
            PhFree(object);
        return;
    }

    for (ULONG i = 0; i < objectCount; i++)
    {
        HANDLE objectHandle;
        PPH_STRING objectDescription;

        if (!NT_SUCCESS(PhBcdOpenObject(
            StoreHandle,
            &object[i].Identifer,
            &objectHandle
            )))
        {
            continue;
        }

        if (NT_SUCCESS(PhBcdGetElementString(
            objectHandle,
            BcdLibraryString_Description,
            &objectDescription
            )))
        {
            PPH_BCD_OBJECT_LIST entry;

            entry = static_cast<PPH_BCD_OBJECT_LIST>(PhAllocateZero(sizeof(PH_BCD_OBJECT_LIST)));
            memcpy(&entry->ObjectGuid, &object[i].Identifer, sizeof(GUID));
            entry->ObjectName = objectDescription;

            PhAddItemList(ObjectList, entry);
        }

        PhBcdCloseObject(objectHandle);
    }

    {
        HANDLE objectHandle;
        PPH_STRING objectDescription;

        // Manually add the memory test application since it's a separate guid. (dmex)

        if (NT_SUCCESS(PhBcdOpenObject(StoreHandle, &GUID_WINDOWS_MEMORY_TESTER, &objectHandle)))
        {
            if (NT_SUCCESS(PhBcdGetElementString(objectHandle, BcdLibraryString_Description, &objectDescription)))
            {
                PPH_BCD_OBJECT_LIST entry;

                entry = static_cast<PPH_BCD_OBJECT_LIST>(PhAllocateZero(sizeof(PH_BCD_OBJECT_LIST)));
                memcpy(&entry->ObjectGuid, &GUID_WINDOWS_MEMORY_TESTER, sizeof(GUID));
                entry->ObjectName = objectDescription;

                PhAddItemList(ObjectList, entry);
            }

            PhBcdCloseObject(objectHandle);
        }
    }

    PhFree(object);
}

static VOID PhpBcdEnumerateBootMgrList(
    _In_ HANDLE StoreHandle,
    _In_ PGUID Identifier,
    _In_ ULONG ElementType,
    _In_ PPH_LIST ObjectList
    )
{
    NTSTATUS status;
    HANDLE objectHandle = nullptr;
    BCD_ELEMENT_OBJECT_LIST objectElementList[32] = { }; // dynamic?
    ULONG objectListLength = sizeof(objectElementList);

    status = PhBcdOpenObject(
        StoreHandle,
        Identifier,
        &objectHandle
        );

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    status = PhBcdGetElementData(
        objectHandle,
        ElementType,
        objectElementList,
        &objectListLength
        );

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    for (ULONG i = 0; i < objectListLength / sizeof(BCD_ELEMENT_OBJECT_LIST); i++)
    {
        HANDLE objectEntryHandle;
        PPH_STRING objectEntryDescription;

        if (!NT_SUCCESS(PhBcdOpenObject(
            StoreHandle,
            &objectElementList->ObjectList[i],
            &objectEntryHandle
            )))
        {
            continue;
        }

        if (NT_SUCCESS(PhBcdGetElementString(
            objectEntryHandle,
            BcdLibraryString_Description,
            &objectEntryDescription
            )))
        {
            PPH_BCD_OBJECT_LIST entry;

            entry = static_cast<PPH_BCD_OBJECT_LIST>(PhAllocateZero(sizeof(PH_BCD_OBJECT_LIST)));
            memcpy(&entry->ObjectGuid, &objectElementList->ObjectList[i], sizeof(GUID));
            entry->ObjectName = objectEntryDescription;

            PhAddItemList(ObjectList, entry);
        }

        PhBcdCloseObject(objectEntryHandle);
    }

CleanupExit:
    if (objectHandle)
        PhBcdCloseObject(objectHandle);
}

PPH_LIST PhBcdQueryBootApplicationList(
    _In_ BOOLEAN EnumerateAllObjects
    )
{
    NTSTATUS status;
    HANDLE storeHandle;
    PPH_LIST objectApplicationList;

    status = PhBcdOpenSystemStore(&storeHandle);

    if (!NT_SUCCESS(status))
        return nullptr;

    objectApplicationList = PhCreateList(5);

    if (EnumerateAllObjects)
    {
        PhpBcdEnumerateOsLoaderList(
            storeHandle,
            objectApplicationList
            );
    }
    else
    {
        PhpBcdEnumerateBootMgrList(
            storeHandle,
            &GUID_WINDOWS_BOOTMGR,
            BcdBootMgrObjectList_DisplayOrder,
            objectApplicationList
            );

        PhpBcdEnumerateBootMgrList(
            storeHandle,
            &GUID_WINDOWS_BOOTMGR,
            BcdBootMgrObjectList_ToolsDisplayOrder,
            objectApplicationList
            );
    }

    PhBcdCloseStore(storeHandle);

    return objectApplicationList;
}

PPH_LIST PhBcdQueryFirmwareBootApplicationList(
    VOID
    )
{
    NTSTATUS status;
    HANDLE storeHandle;
    PPH_LIST objectApplicationList;

    status = PhBcdOpenSystemStore(&storeHandle);

    if (!NT_SUCCESS(status))
        return nullptr;

    objectApplicationList = PhCreateList(5);

    PhpBcdEnumerateBootMgrList(
        storeHandle,
        &GUID_FIRMWARE_BOOTMGR,
        BcdBootMgrObjectList_DisplayOrder,
        objectApplicationList
        );
    PhpBcdEnumerateBootMgrList(
        storeHandle,
        &GUID_FIRMWARE_BOOTMGR,
        BcdBootMgrObjectList_ToolsDisplayOrder,
        objectApplicationList
        );

    PhBcdCloseStore(storeHandle);

    return objectApplicationList;
}

VOID PhBcdDestroyBootApplicationList(
    _In_ PPH_LIST ObjectApplicationList
    )
{
    for (ULONG i = 0; i < ObjectApplicationList->Count; i++)
    {
        PPH_BCD_OBJECT_LIST entry = static_cast<PPH_BCD_OBJECT_LIST>(ObjectApplicationList->Items[i]);

        PhDereferenceObject(entry->ObjectName);
        PhFree(entry);
    }

    PhDereferenceObject(ObjectApplicationList);
}
