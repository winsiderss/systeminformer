/*
 * Process Hacker Extended Services -
 *   trigger editor
 *
 * Copyright (C) 2011-2015 wj32
 *
 * This file is part of Process Hacker.
 *
 * Process Hacker is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Process Hacker is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Process Hacker.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <phdk.h>
#include <windowsx.h>
#include "extsrv.h"
#include "resource.h"

typedef struct _ES_TRIGGER_DATA
{
    ULONG Type;
    union
    {
        PPH_STRING String;
        struct
        {
            PVOID Binary;
            ULONG BinaryLength;
        };
        UCHAR Byte;
        ULONG64 UInt64;
    };
} ES_TRIGGER_DATA, *PES_TRIGGER_DATA;

typedef struct _ES_TRIGGER_INFO
{
    ULONG Type;
    PGUID Subtype;
    ULONG Action;
    PPH_LIST DataList;
    GUID SubtypeBuffer;
} ES_TRIGGER_INFO, *PES_TRIGGER_INFO;

typedef struct _ES_TRIGGER_CONTEXT
{
    PPH_SERVICE_ITEM ServiceItem;
    HWND WindowHandle;
    HWND TriggersLv;
    BOOLEAN Dirty;
    ULONG InitialNumberOfTriggers;
    PPH_LIST InfoList;

    // Trigger dialog box
    PES_TRIGGER_INFO EditingInfo;
    ULONG LastSelectedType;
    PPH_STRING LastCustomSubType;

    // Value dialog box
    PPH_STRING EditingValue;
} ES_TRIGGER_CONTEXT, *PES_TRIGGER_CONTEXT;

typedef struct _TYPE_ENTRY
{
    ULONG Type;
    PWSTR Name;
} TYPE_ENTRY, PTYPE_ENTRY;

typedef struct _SUBTYPE_ENTRY
{
    ULONG Type;
    PGUID Guid;
    PWSTR Name;
} SUBTYPE_ENTRY, PSUBTYPE_ENTRY;

typedef struct _ETW_PUBLISHER_ENTRY
{
    PPH_STRING PublisherName;
    GUID Guid;
} ETW_PUBLISHER_ENTRY, *PETW_PUBLISHER_ENTRY;

INT_PTR CALLBACK EspServiceTriggerDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

INT_PTR CALLBACK ValueDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

static GUID NetworkManagerFirstIpAddressArrivalGuid = { 0x4f27f2de, 0x14e2, 0x430b, { 0xa5, 0x49, 0x7c, 0xd4, 0x8c, 0xbc, 0x82, 0x45 } };
static GUID NetworkManagerLastIpAddressRemovalGuid = { 0xcc4ba62a, 0x162e, 0x4648, { 0x84, 0x7a, 0xb6, 0xbd, 0xf9, 0x93, 0xe3, 0x35 } };
static GUID DomainJoinGuid = { 0x1ce20aba, 0x9851, 0x4421, { 0x94, 0x30, 0x1d, 0xde, 0xb7, 0x66, 0xe8, 0x09 } };
static GUID DomainLeaveGuid = { 0xddaf516e, 0x58c2, 0x4866, { 0x95, 0x74, 0xc3, 0xb6, 0x15, 0xd4, 0x2e, 0xa1 } };
static GUID FirewallPortOpenGuid = { 0xb7569e07, 0x8421, 0x4ee0, { 0xad, 0x10, 0x86, 0x91, 0x5a, 0xfd, 0xad, 0x09 } };
static GUID FirewallPortCloseGuid = { 0xa144ed38, 0x8e12, 0x4de4, { 0x9d, 0x96, 0xe6, 0x47, 0x40, 0xb1, 0xa5, 0x24 } };
static GUID MachinePolicyPresentGuid = { 0x659fcae6, 0x5bdb, 0x4da9, { 0xb1, 0xff, 0xca, 0x2a, 0x17, 0x8d, 0x46, 0xe0 } };
static GUID UserPolicyPresentGuid = { 0x54fb46c8, 0xf089, 0x464c, { 0xb1, 0xfd, 0x59, 0xd1, 0xb6, 0x2c, 0x3b, 0x50 } };
static GUID RpcInterfaceEventGuid = { 0xbc90d167, 0x9470, 0x4139, { 0xa9, 0xba, 0xbe, 0x0b, 0xbb, 0xf5, 0xb7, 0x4d } };
static GUID NamedPipeEventGuid = { 0x1f81d131, 0x3fac, 0x4537, { 0x9e, 0x0c, 0x7e, 0x7b, 0x0c, 0x2f, 0x4b, 0x55 } };
static GUID SubTypeUnknownGuid; // dummy

static TYPE_ENTRY TypeEntries[] =
{
    { SERVICE_TRIGGER_TYPE_DEVICE_INTERFACE_ARRIVAL, L"Device interface arrival" },
    { SERVICE_TRIGGER_TYPE_IP_ADDRESS_AVAILABILITY, L"IP address availability" },
    { SERVICE_TRIGGER_TYPE_DOMAIN_JOIN, L"Domain join" },
    { SERVICE_TRIGGER_TYPE_FIREWALL_PORT_EVENT, L"Firewall port event" },
    { SERVICE_TRIGGER_TYPE_GROUP_POLICY, L"Group policy" },
    { SERVICE_TRIGGER_TYPE_NETWORK_ENDPOINT, L"Network endpoint" },
    { SERVICE_TRIGGER_TYPE_CUSTOM_SYSTEM_STATE_CHANGE, L"Custom system state change" },
    { SERVICE_TRIGGER_TYPE_CUSTOM, L"Custom" }
};

static SUBTYPE_ENTRY SubTypeEntries[] =
{
    { SERVICE_TRIGGER_TYPE_IP_ADDRESS_AVAILABILITY, NULL, L"IP address" },
    { SERVICE_TRIGGER_TYPE_IP_ADDRESS_AVAILABILITY, &NetworkManagerFirstIpAddressArrivalGuid, L"IP address: First arrival" },
    { SERVICE_TRIGGER_TYPE_IP_ADDRESS_AVAILABILITY, &NetworkManagerLastIpAddressRemovalGuid, L"IP address: Last removal" },
    { SERVICE_TRIGGER_TYPE_IP_ADDRESS_AVAILABILITY, &SubTypeUnknownGuid, L"IP address: Unknown" },
    { SERVICE_TRIGGER_TYPE_DOMAIN_JOIN, NULL, L"Domain" },
    { SERVICE_TRIGGER_TYPE_DOMAIN_JOIN, &DomainJoinGuid, L"Domain: Join" },
    { SERVICE_TRIGGER_TYPE_DOMAIN_JOIN, &DomainLeaveGuid, L"Domain: Leave" },
    { SERVICE_TRIGGER_TYPE_DOMAIN_JOIN, &SubTypeUnknownGuid, L"Domain: Unknown" },
    { SERVICE_TRIGGER_TYPE_FIREWALL_PORT_EVENT, NULL, L"Firewall port" },
    { SERVICE_TRIGGER_TYPE_FIREWALL_PORT_EVENT, &FirewallPortOpenGuid, L"Firewall port: Open" },
    { SERVICE_TRIGGER_TYPE_FIREWALL_PORT_EVENT, &FirewallPortCloseGuid, L"Firewall port: Close" },
    { SERVICE_TRIGGER_TYPE_FIREWALL_PORT_EVENT, &SubTypeUnknownGuid, L"Firewall port: Unknown" },
    { SERVICE_TRIGGER_TYPE_GROUP_POLICY, NULL, L"Group policy change" },
    { SERVICE_TRIGGER_TYPE_GROUP_POLICY, &MachinePolicyPresentGuid, L"Group policy change: Machine" },
    { SERVICE_TRIGGER_TYPE_GROUP_POLICY, &UserPolicyPresentGuid, L"Group policy change: User" },
    { SERVICE_TRIGGER_TYPE_GROUP_POLICY, &SubTypeUnknownGuid, L"Group policy change: Unknown" },
    { SERVICE_TRIGGER_TYPE_NETWORK_ENDPOINT, NULL, L"Network endpoint" },
    { SERVICE_TRIGGER_TYPE_NETWORK_ENDPOINT, &RpcInterfaceEventGuid, L"Network endpoint: RPC interface" },
    { SERVICE_TRIGGER_TYPE_NETWORK_ENDPOINT, &NamedPipeEventGuid, L"Network endpoint: Named pipe" },
    { SERVICE_TRIGGER_TYPE_NETWORK_ENDPOINT, &SubTypeUnknownGuid, L"Network endpoint: Unknown" }
};

static PH_STRINGREF PublishersKeyName = PH_STRINGREF_INIT(L"Software\\Microsoft\\Windows\\CurrentVersion\\WINEVT\\Publishers\\");

PES_TRIGGER_DATA EspCreateTriggerData(
    _In_opt_ PSERVICE_TRIGGER_SPECIFIC_DATA_ITEM DataItem
    )
{
    PES_TRIGGER_DATA data;

    data = PhAllocate(sizeof(ES_TRIGGER_DATA));
    memset(data, 0, sizeof(ES_TRIGGER_DATA));

    if (DataItem)
    {
        data->Type = DataItem->dwDataType;

        if (data->Type == SERVICE_TRIGGER_DATA_TYPE_STRING)
        {
            if (DataItem->pData && DataItem->cbData >= 2)
                data->String = PhCreateStringEx((PWSTR)DataItem->pData, DataItem->cbData - 2); // exclude final null terminator
            else
                data->String = PhReferenceEmptyString();
        }
        else if (data->Type == SERVICE_TRIGGER_DATA_TYPE_BINARY)
        {
            data->BinaryLength = DataItem->cbData;
            data->Binary = PhAllocateCopy(DataItem->pData, DataItem->cbData);
        }
        else if (data->Type == SERVICE_TRIGGER_DATA_TYPE_LEVEL)
        {
            if (DataItem->cbData == sizeof(UCHAR))
                data->Byte = *(PUCHAR)DataItem->pData;
        }
        else if (data->Type == SERVICE_TRIGGER_DATA_TYPE_KEYWORD_ANY || data->Type == SERVICE_TRIGGER_DATA_TYPE_KEYWORD_ALL)
        {
            if (DataItem->cbData == sizeof(ULONG64))
                data->UInt64 = *(PULONG64)DataItem->pData;
        }
    }

    return data;
}

PES_TRIGGER_DATA EspCloneTriggerData(
    _In_ PES_TRIGGER_DATA Data
    )
{
    PES_TRIGGER_DATA newData;

    newData = PhAllocateCopy(Data, sizeof(ES_TRIGGER_DATA));

    if (newData->Type == SERVICE_TRIGGER_DATA_TYPE_STRING)
    {
        if (newData->String)
            newData->String = PhDuplicateString(newData->String);
    }
    else if (newData->Type == SERVICE_TRIGGER_DATA_TYPE_BINARY)
    {
        if (newData->Binary)
            newData->Binary = PhAllocateCopy(newData->Binary, newData->BinaryLength);
    }

    return newData;
}

VOID EspDestroyTriggerData(
    _In_ PES_TRIGGER_DATA Data
    )
{
    if (Data->Type == SERVICE_TRIGGER_DATA_TYPE_STRING)
    {
        if (Data->String)
            PhDereferenceObject(Data->String);
    }
    else if (Data->Type == SERVICE_TRIGGER_DATA_TYPE_BINARY)
    {
        if (Data->Binary)
            PhFree(Data->Binary);
    }

    PhFree(Data);
}

PES_TRIGGER_INFO EspCreateTriggerInfo(
    _In_opt_ PSERVICE_TRIGGER Trigger
    )
{
    PES_TRIGGER_INFO info;

    info = PhAllocate(sizeof(ES_TRIGGER_INFO));
    memset(info, 0, sizeof(ES_TRIGGER_INFO));

    if (Trigger)
    {
        info->Type = Trigger->dwTriggerType;

        if (Trigger->pTriggerSubtype)
        {
            info->SubtypeBuffer = *Trigger->pTriggerSubtype;
            info->Subtype = &info->SubtypeBuffer;
        }

        info->Action = Trigger->dwAction;

        if (
            info->Type == SERVICE_TRIGGER_TYPE_CUSTOM ||
            info->Type == SERVICE_TRIGGER_TYPE_DEVICE_INTERFACE_ARRIVAL ||
            info->Type == SERVICE_TRIGGER_TYPE_FIREWALL_PORT_EVENT ||
            info->Type == SERVICE_TRIGGER_TYPE_NETWORK_ENDPOINT
            )
        {
            ULONG i;

            info->DataList = PhCreateList(Trigger->cDataItems);

            for (i = 0; i < Trigger->cDataItems; i++)
            {
                PhAddItemList(info->DataList, EspCreateTriggerData(&Trigger->pDataItems[i]));
            }
        }
    }

    return info;
}

PES_TRIGGER_INFO EspCloneTriggerInfo(
    _In_ PES_TRIGGER_INFO Info
    )
{
    PES_TRIGGER_INFO newInfo;

    newInfo = PhAllocateCopy(Info, sizeof(ES_TRIGGER_INFO));

    if (newInfo->Subtype == &Info->SubtypeBuffer)
        newInfo->Subtype = &newInfo->SubtypeBuffer;

    if (newInfo->DataList)
    {
        ULONG i;

        newInfo->DataList = PhCreateList(Info->DataList->AllocatedCount);
        newInfo->DataList->Count = Info->DataList->Count;

        for (i = 0; i < Info->DataList->Count; i++)
            newInfo->DataList->Items[i] = EspCloneTriggerData(Info->DataList->Items[i]);
    }

    return newInfo;
}

VOID EspDestroyTriggerInfo(
    _In_ PES_TRIGGER_INFO Info
    )
{
    if (Info->DataList)
    {
        ULONG i;

        for (i = 0; i < Info->DataList->Count; i++)
        {
            EspDestroyTriggerData(Info->DataList->Items[i]);
        }

        PhDereferenceObject(Info->DataList);
    }

    PhFree(Info);
}

VOID EspClearTriggerInfoList(
    _In_ PPH_LIST List
    )
{
    ULONG i;

    for (i = 0; i < List->Count; i++)
    {
        EspDestroyTriggerInfo(List->Items[i]);
    }

    PhClearList(List);
}

struct _ES_TRIGGER_CONTEXT *EsCreateServiceTriggerContext(
    _In_ PPH_SERVICE_ITEM ServiceItem,
    _In_ HWND WindowHandle,
    _In_ HWND TriggersLv
    )
{
    PES_TRIGGER_CONTEXT context;

    context = PhAllocate(sizeof(ES_TRIGGER_CONTEXT));
    memset(context, 0, sizeof(ES_TRIGGER_CONTEXT));
    context->ServiceItem = ServiceItem;
    context->WindowHandle = WindowHandle;
    context->TriggersLv = TriggersLv;
    context->InfoList = PhCreateList(4);

    PhSetListViewStyle(TriggersLv, FALSE, TRUE);
    PhSetControlTheme(TriggersLv, L"explorer");
    PhAddListViewColumn(TriggersLv, 0, 0, 0, LVCFMT_LEFT, 300, L"Trigger");
    PhAddListViewColumn(TriggersLv, 1, 1, 1, LVCFMT_LEFT, 60, L"Action");
    PhSetExtendedListView(TriggersLv);

    EnableWindow(GetDlgItem(WindowHandle, IDC_EDIT), FALSE);
    EnableWindow(GetDlgItem(WindowHandle, IDC_DELETE), FALSE);

    return context;
}

VOID EsDestroyServiceTriggerContext(
    _In_ struct _ES_TRIGGER_CONTEXT *Context
    )
{
    ULONG i;

    for (i = 0; i < Context->InfoList->Count; i++)
    {
        EspDestroyTriggerInfo(Context->InfoList->Items[i]);
    }

    PhDereferenceObject(Context->InfoList);
    PhFree(Context);
}

PPH_STRING EspLookupEtwPublisherName(
    _In_ PGUID Guid
    )
{
    PPH_STRING guidString;
    PPH_STRING keyName;
    HANDLE keyHandle;
    PPH_STRING publisherName = NULL;

    // Copied from ProcessHacker\hndlinfo.c.

    guidString = PhFormatGuid(Guid);

    keyName = PhConcatStringRef2(&PublishersKeyName, &guidString->sr);

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
        return guidString;
    }
}

BOOLEAN EspEnumerateEtwPublishers(
    _Out_ PETW_PUBLISHER_ENTRY *Entries,
    _Out_ PULONG NumberOfEntries
    )
{
    NTSTATUS status;
    HANDLE publishersKeyHandle;
    ULONG index;
    PKEY_BASIC_INFORMATION buffer;
    ULONG bufferSize;
    PETW_PUBLISHER_ENTRY entries;
    ULONG numberOfEntries;
    ULONG allocatedEntries;

    if (!NT_SUCCESS(PhOpenKey(
        &publishersKeyHandle,
        KEY_READ,
        PH_KEY_LOCAL_MACHINE,
        &PublishersKeyName,
        0
        )))
    {
        return FALSE;
    }

    numberOfEntries = 0;
    allocatedEntries = 256;
    entries = PhAllocate(allocatedEntries * sizeof(ETW_PUBLISHER_ENTRY));

    index = 0;
    bufferSize = 0x100;
    buffer = PhAllocate(0x100);

    while (TRUE)
    {
        status = NtEnumerateKey(
            publishersKeyHandle,
            index,
            KeyBasicInformation,
            buffer,
            bufferSize,
            &bufferSize
            );

        if (NT_SUCCESS(status))
        {
            UNICODE_STRING nameUs;
            PH_STRINGREF name;
            HANDLE keyHandle;
            GUID guid;
            PPH_STRING publisherName;

            nameUs.Buffer = buffer->Name;
            nameUs.Length = (USHORT)buffer->NameLength;
            name.Buffer = buffer->Name;
            name.Length = buffer->NameLength;

            // Make sure this is a valid publisher key.
            if (NT_SUCCESS(RtlGUIDFromString(&nameUs, &guid)))
            {
                if (NT_SUCCESS(PhOpenKey(
                    &keyHandle,
                    KEY_READ,
                    publishersKeyHandle,
                    &name,
                    0
                    )))
                {
                    publisherName = PhQueryRegistryString(keyHandle, NULL);

                    if (publisherName)
                    {
                        if (publisherName->Length != 0)
                        {
                            PETW_PUBLISHER_ENTRY entry;

                            if (numberOfEntries == allocatedEntries)
                            {
                                allocatedEntries *= 2;
                                entries = PhReAllocate(entries, allocatedEntries * sizeof(ETW_PUBLISHER_ENTRY));
                            }

                            entry = &entries[numberOfEntries++];
                            entry->PublisherName = publisherName;
                            entry->Guid = guid;
                        }
                        else
                        {
                            PhDereferenceObject(publisherName);
                        }
                    }

                    NtClose(keyHandle);
                }
            }

            index++;
        }
        else if (status == STATUS_NO_MORE_ENTRIES)
        {
            break;
        }
        else if (status == STATUS_BUFFER_OVERFLOW || status == STATUS_BUFFER_TOO_SMALL)
        {
            PhFree(buffer);
            buffer = PhAllocate(bufferSize);
        }
        else
        {
            break;
        }
    }

    PhFree(buffer);
    NtClose(publishersKeyHandle);

    *Entries = entries;
    *NumberOfEntries = numberOfEntries;

    return TRUE;
}

BOOLEAN EspLookupEtwPublisherGuid(
    _In_ PPH_STRINGREF PublisherName,
    _Out_ PGUID Guid
    )
{
    BOOLEAN result;
    PETW_PUBLISHER_ENTRY entries;
    ULONG numberOfEntries;
    ULONG i;

    if (!EspEnumerateEtwPublishers(&entries, &numberOfEntries))
        return FALSE;

    result = FALSE;

    for (i = 0; i < numberOfEntries; i++)
    {
        if (!result && PhEqualStringRef(&entries[i].PublisherName->sr, PublisherName, TRUE))
        {
            *Guid = entries[i].Guid;
            result = TRUE;
        }

        PhDereferenceObject(entries[i].PublisherName);
    }

    PhFree(entries);

    return result;
}

VOID EspFormatTriggerInfo(
    _In_ PES_TRIGGER_INFO Info,
    _Out_ PWSTR *TriggerString,
    _Out_ PWSTR *ActionString,
    _Out_ PPH_STRING *StringUsed
    )
{
    PPH_STRING stringUsed = NULL;
    PWSTR triggerString = NULL;
    PWSTR actionString;
    ULONG i;
    BOOLEAN typeFound;
    BOOLEAN subTypeFound;

    switch (Info->Type)
    {
    case SERVICE_TRIGGER_TYPE_DEVICE_INTERFACE_ARRIVAL:
        {
            PPH_STRING guidString;

            if (!Info->Subtype)
            {
                triggerString = L"Device interface arrival";
            }
            else
            {
                guidString = PhFormatGuid(Info->Subtype);
                stringUsed = PhConcatStrings2(L"Device interface arrival: ", guidString->Buffer);
                triggerString = stringUsed->Buffer;
            }
        }
        break;
    case SERVICE_TRIGGER_TYPE_CUSTOM_SYSTEM_STATE_CHANGE:
        {
            PPH_STRING guidString;

            if (!Info->Subtype)
            {
                triggerString = L"Custom system state change";
            }
            else
            {
                guidString = PhFormatGuid(Info->Subtype);
                stringUsed = PhConcatStrings2(L"Custom system state change: ", guidString->Buffer);
                triggerString = stringUsed->Buffer;
            }
        }
        break;
    case SERVICE_TRIGGER_TYPE_CUSTOM:
        {
            if (Info->Subtype)
            {
                PPH_STRING publisherName;

                // Try to lookup the publisher name from the GUID.
                publisherName = EspLookupEtwPublisherName(Info->Subtype);
                stringUsed = PhConcatStrings2(L"Custom: ", publisherName->Buffer);
                PhDereferenceObject(publisherName);
                triggerString = stringUsed->Buffer;
            }
            else
            {
                triggerString = L"Custom";
            }
        }
        break;
    default:
        {
            typeFound = FALSE;
            subTypeFound = FALSE;

            for (i = 0; i < sizeof(SubTypeEntries) / sizeof(SUBTYPE_ENTRY); i++)
            {
                if (SubTypeEntries[i].Type == Info->Type)
                {
                    typeFound = TRUE;

                    if (!Info->Subtype && !SubTypeEntries[i].Guid)
                    {
                        subTypeFound = TRUE;
                        triggerString = SubTypeEntries[i].Name;
                        break;
                    }
                    else if (Info->Subtype && SubTypeEntries[i].Guid && memcmp(Info->Subtype, SubTypeEntries[i].Guid, sizeof(GUID)) == 0)
                    {
                        subTypeFound = TRUE;
                        triggerString = SubTypeEntries[i].Name;
                        break;
                    }
                    else if (!subTypeFound && SubTypeEntries[i].Guid == &SubTypeUnknownGuid)
                    {
                        triggerString = SubTypeEntries[i].Name;
                        break;
                    }
                }
            }

            if (!typeFound)
            {
                triggerString = L"Unknown";
            }
        }
        break;
    }

    switch (Info->Action)
    {
    case SERVICE_TRIGGER_ACTION_SERVICE_START:
        actionString = L"Start";
        break;
    case SERVICE_TRIGGER_ACTION_SERVICE_STOP:
        actionString = L"Stop";
        break;
    default:
        actionString = L"Unknown";
        break;
    }

    *TriggerString = triggerString;
    *ActionString = actionString;
    *StringUsed = stringUsed;
}

VOID EsLoadServiceTriggerInfo(
    _In_ struct _ES_TRIGGER_CONTEXT *Context,
    _In_ SC_HANDLE ServiceHandle
    )
{
    PSERVICE_TRIGGER_INFO triggerInfo;
    ULONG i;

    EspClearTriggerInfoList(Context->InfoList);

    if (triggerInfo = PhQueryServiceVariableSize(ServiceHandle, SERVICE_CONFIG_TRIGGER_INFO))
    {
        for (i = 0; i < triggerInfo->cTriggers; i++)
        {
            PSERVICE_TRIGGER trigger = &triggerInfo->pTriggers[i];
            PES_TRIGGER_INFO info;
            PWSTR triggerString;
            PWSTR actionString;
            PPH_STRING stringUsed;
            INT lvItemIndex;

            info = EspCreateTriggerInfo(trigger);
            PhAddItemList(Context->InfoList, info);

            EspFormatTriggerInfo(info, &triggerString, &actionString, &stringUsed);

            lvItemIndex = PhAddListViewItem(Context->TriggersLv, MAXINT, triggerString, info);
            PhSetListViewSubItem(Context->TriggersLv, lvItemIndex, 1, actionString);

            if (stringUsed)
                PhDereferenceObject(stringUsed);
        }

        Context->InitialNumberOfTriggers = triggerInfo->cTriggers;

        ExtendedListView_SortItems(Context->TriggersLv);

        PhFree(triggerInfo);
    }
}

BOOLEAN EsSaveServiceTriggerInfo(
    _In_ struct _ES_TRIGGER_CONTEXT *Context,
    _Out_ PULONG Win32Result
    )
{
    BOOLEAN result = TRUE;
    PH_AUTO_POOL autoPool;
    SC_HANDLE serviceHandle;
    SERVICE_TRIGGER_INFO triggerInfo;
    ULONG i;
    ULONG j;

    if (!Context->Dirty)
        return TRUE;

    // Do not try to change trigger information if we didn't have any triggers before and we don't
    // have any now. ChangeServiceConfig2 returns an error in this situation.
    if (Context->InitialNumberOfTriggers == 0 && Context->InfoList->Count == 0)
        return TRUE;

    PhInitializeAutoPool(&autoPool);

    memset(&triggerInfo, 0, sizeof(SERVICE_TRIGGER_INFO));
    triggerInfo.cTriggers = Context->InfoList->Count;

    // pTriggers needs to be NULL when there are no triggers.
    if (Context->InfoList->Count != 0)
    {
        triggerInfo.pTriggers = PhAutoDereferenceObject(PhCreateAlloc(Context->InfoList->Count * sizeof(SERVICE_TRIGGER)));
        memset(triggerInfo.pTriggers, 0, Context->InfoList->Count * sizeof(SERVICE_TRIGGER));

        for (i = 0; i < Context->InfoList->Count; i++)
        {
            PSERVICE_TRIGGER trigger = &triggerInfo.pTriggers[i];
            PES_TRIGGER_INFO info = Context->InfoList->Items[i];

            trigger->dwTriggerType = info->Type;
            trigger->dwAction = info->Action;
            trigger->pTriggerSubtype = info->Subtype;

            if (info->DataList && info->DataList->Count != 0)
            {
                trigger->cDataItems = info->DataList->Count;
                trigger->pDataItems = PhAutoDereferenceObject(PhCreateAlloc(info->DataList->Count * sizeof(SERVICE_TRIGGER_SPECIFIC_DATA_ITEM)));

                for (j = 0; j < info->DataList->Count; j++)
                {
                    PSERVICE_TRIGGER_SPECIFIC_DATA_ITEM dataItem = &trigger->pDataItems[j];
                    PES_TRIGGER_DATA data = info->DataList->Items[j];

                    dataItem->dwDataType = data->Type;

                    if (data->Type == SERVICE_TRIGGER_DATA_TYPE_STRING)
                    {
                        dataItem->cbData = (ULONG)data->String->Length + 2; // include null terminator
                        dataItem->pData = (PBYTE)data->String->Buffer;
                    }
                    else if (data->Type == SERVICE_TRIGGER_DATA_TYPE_BINARY)
                    {
                        dataItem->cbData = data->BinaryLength;
                        dataItem->pData = data->Binary;
                    }
                    else if (data->Type == SERVICE_TRIGGER_DATA_TYPE_LEVEL)
                    {
                        dataItem->cbData = sizeof(UCHAR);
                        dataItem->pData = (PBYTE)&data->Byte;
                    }
                    else if (data->Type == SERVICE_TRIGGER_DATA_TYPE_KEYWORD_ANY || data->Type == SERVICE_TRIGGER_DATA_TYPE_KEYWORD_ALL)
                    {
                        dataItem->cbData = sizeof(ULONG64);
                        dataItem->pData = (PBYTE)&data->UInt64;
                    }
                }
            }
        }
    }

    if (serviceHandle = PhOpenService(Context->ServiceItem->Name->Buffer, SERVICE_CHANGE_CONFIG))
    {
        if (!ChangeServiceConfig2(serviceHandle, SERVICE_CONFIG_TRIGGER_INFO, &triggerInfo))
        {
            result = FALSE;
            *Win32Result = GetLastError();
        }

        CloseServiceHandle(serviceHandle);
    }
    else
    {
        result = FALSE;
        *Win32Result = GetLastError();

        if (*Win32Result == ERROR_ACCESS_DENIED && !PhElevated)
        {
            // Elevate using phsvc.
            if (PhUiConnectToPhSvc(Context->WindowHandle, FALSE))
            {
                NTSTATUS status;

                result = TRUE;

                if (!NT_SUCCESS(status = PhSvcCallChangeServiceConfig2(Context->ServiceItem->Name->Buffer,
                    SERVICE_CONFIG_TRIGGER_INFO, &triggerInfo)))
                {
                    result = FALSE;
                    *Win32Result = PhNtStatusToDosError(status);
                }

                PhUiDisconnectFromPhSvc();
            }
            else
            {
                // User cancelled elevation.
                *Win32Result = ERROR_CANCELLED;
            }
        }
    }

    PhDeleteAutoPool(&autoPool);

    return result;
}

LOGICAL EspSetListViewItemParam(
    _In_ HWND ListViewHandle,
    _In_ INT Index,
    _In_ PVOID Param
    )
{
    LVITEM item;

    item.mask = LVIF_PARAM;
    item.iItem = Index;
    item.iSubItem = 0;
    item.lParam = (LPARAM)Param;

    return ListView_SetItem(ListViewHandle, &item);
}

VOID EsHandleEventServiceTrigger(
    _In_ struct _ES_TRIGGER_CONTEXT *Context,
    _In_ ULONG Event
    )
{
    switch (Event)
    {
    case ES_TRIGGER_EVENT_NEW:
        {
            Context->EditingInfo = EspCreateTriggerInfo(NULL);
            Context->EditingInfo->Type = SERVICE_TRIGGER_TYPE_IP_ADDRESS_AVAILABILITY;
            Context->EditingInfo->SubtypeBuffer = NetworkManagerFirstIpAddressArrivalGuid;
            Context->EditingInfo->Subtype = &Context->EditingInfo->SubtypeBuffer;
            Context->EditingInfo->Action = SERVICE_TRIGGER_ACTION_SERVICE_START;

            if (DialogBoxParam(
                PluginInstance->DllBase,
                MAKEINTRESOURCE(IDD_SRVTRIGGER),
                Context->WindowHandle,
                EspServiceTriggerDlgProc,
                (LPARAM)Context
                ) == IDOK)
            {
                PWSTR triggerString;
                PWSTR actionString;
                PPH_STRING stringUsed;
                INT lvItemIndex;

                Context->Dirty = TRUE;
                PhAddItemList(Context->InfoList, Context->EditingInfo);

                EspFormatTriggerInfo(Context->EditingInfo, &triggerString, &actionString, &stringUsed);

                lvItemIndex = PhAddListViewItem(Context->TriggersLv, MAXINT, triggerString, Context->EditingInfo);
                PhSetListViewSubItem(Context->TriggersLv, lvItemIndex, 1, actionString);

                if (stringUsed)
                    PhDereferenceObject(stringUsed);
            }
            else
            {
                EspDestroyTriggerInfo(Context->EditingInfo);
            }

            Context->EditingInfo = NULL;
        }
        break;
    case ES_TRIGGER_EVENT_EDIT:
        {
            INT lvItemIndex;
            PES_TRIGGER_INFO info;
            ULONG index;

            lvItemIndex = ListView_GetNextItem(Context->TriggersLv, -1, LVNI_SELECTED);

            if (lvItemIndex != -1 && PhGetListViewItemParam(Context->TriggersLv, lvItemIndex, (PVOID *)&info))
            {
                index = PhFindItemList(Context->InfoList, info);

                if (index != -1)
                {
                    Context->EditingInfo = EspCloneTriggerInfo(info);

                    if (DialogBoxParam(
                        PluginInstance->DllBase,
                        MAKEINTRESOURCE(IDD_SRVTRIGGER),
                        Context->WindowHandle,
                        EspServiceTriggerDlgProc,
                        (LPARAM)Context
                        ) == IDOK)
                    {
                        PWSTR triggerString;
                        PWSTR actionString;
                        PPH_STRING stringUsed;

                        Context->Dirty = TRUE;
                        EspDestroyTriggerInfo(Context->InfoList->Items[index]);
                        Context->InfoList->Items[index] = Context->EditingInfo;

                        EspFormatTriggerInfo(Context->EditingInfo, &triggerString, &actionString, &stringUsed);
                        PhSetListViewSubItem(Context->TriggersLv, lvItemIndex, 0, triggerString);
                        PhSetListViewSubItem(Context->TriggersLv, lvItemIndex, 1, actionString);

                        if (stringUsed)
                            PhDereferenceObject(stringUsed);

                        EspSetListViewItemParam(Context->TriggersLv, lvItemIndex, Context->EditingInfo);
                    }
                    else
                    {
                        EspDestroyTriggerInfo(Context->EditingInfo);
                    }

                    Context->EditingInfo = NULL;
                }
            }
        }
        break;
    case ES_TRIGGER_EVENT_DELETE:
        {
            INT lvItemIndex;
            PES_TRIGGER_INFO info;
            ULONG index;

            lvItemIndex = ListView_GetNextItem(Context->TriggersLv, -1, LVNI_SELECTED);

            if (lvItemIndex != -1 && PhGetListViewItemParam(Context->TriggersLv, lvItemIndex, (PVOID *)&info))
            {
                index = PhFindItemList(Context->InfoList, info);

                if (index != -1)
                {
                    EspDestroyTriggerInfo(info);
                    PhRemoveItemList(Context->InfoList, index);
                    PhRemoveListViewItem(Context->TriggersLv, lvItemIndex);
                }
            }

            Context->Dirty = TRUE;
        }
        break;
    case ES_TRIGGER_EVENT_SELECTIONCHANGED:
        {
            ULONG selectedCount;

            selectedCount = ListView_GetSelectedCount(Context->TriggersLv);

            EnableWindow(GetDlgItem(Context->WindowHandle, IDC_EDIT), selectedCount == 1);
            EnableWindow(GetDlgItem(Context->WindowHandle, IDC_DELETE), selectedCount == 1);
        }
        break;
    }
}

static ULONG EspTriggerTypeStringToInteger(
    _In_ PWSTR String
    )
{
    ULONG i;

    for (i = 0; i < sizeof(TypeEntries) / sizeof(TYPE_ENTRY); i++)
    {
        if (PhEqualStringZ(TypeEntries[i].Name, String, FALSE))
            return TypeEntries[i].Type;
    }

    return 0;
}

static int __cdecl EtwPublisherByNameCompareFunction(
    _In_ const void *elem1,
    _In_ const void *elem2
    )
{
    PETW_PUBLISHER_ENTRY entry1 = (PETW_PUBLISHER_ENTRY)elem1;
    PETW_PUBLISHER_ENTRY entry2 = (PETW_PUBLISHER_ENTRY)elem2;

    return PhCompareString(entry1->PublisherName, entry2->PublisherName, TRUE);
}

static VOID EspFixServiceTriggerControls(
    _In_ HWND hwndDlg,
    _In_ PES_TRIGGER_CONTEXT Context
    )
{
    HWND typeComboBox;
    HWND subTypeComboBox;
    ULONG i;
    PPH_STRING selectedTypeString;
    ULONG type;
    PPH_STRING selectedSubTypeString;

    typeComboBox = GetDlgItem(hwndDlg, IDC_TYPE);
    subTypeComboBox = GetDlgItem(hwndDlg, IDC_SUBTYPE);

    selectedTypeString = PhGetWindowText(typeComboBox);
    type = EspTriggerTypeStringToInteger(selectedTypeString->Buffer);
    PhDereferenceObject(selectedTypeString);

    if (Context->LastSelectedType != type)
    {
        // Change the contents of the subtype combo box based on the type.

        ComboBox_ResetContent(subTypeComboBox);

        switch (type)
        {
        case SERVICE_TRIGGER_TYPE_DEVICE_INTERFACE_ARRIVAL:
            {
                ComboBox_AddString(subTypeComboBox, L"Custom");
            }
            break;
        case SERVICE_TRIGGER_TYPE_CUSTOM_SYSTEM_STATE_CHANGE:
            {
                ComboBox_AddString(subTypeComboBox, L"Custom");
            }
            break;
        case SERVICE_TRIGGER_TYPE_CUSTOM:
            {
                PETW_PUBLISHER_ENTRY entries;
                ULONG numberOfEntries;
                ULONG i;

                ComboBox_AddString(subTypeComboBox, L"Custom");

                // Display a list of publishers.
                if (EspEnumerateEtwPublishers(&entries, &numberOfEntries))
                {
                    // Sort the list by name.
                    qsort(entries, numberOfEntries, sizeof(ETW_PUBLISHER_ENTRY), EtwPublisherByNameCompareFunction);

                    for (i = 0; i < numberOfEntries; i++)
                    {
                        ComboBox_AddString(subTypeComboBox, entries[i].PublisherName->Buffer);
                        PhDereferenceObject(entries[i].PublisherName);
                    }

                    PhFree(entries);
                }
            }
            break;
        default:
            for (i = 0; i < sizeof(SubTypeEntries) / sizeof(SUBTYPE_ENTRY); i++)
            {
                if (SubTypeEntries[i].Type == type && SubTypeEntries[i].Guid && SubTypeEntries[i].Guid != &SubTypeUnknownGuid)
                {
                    ComboBox_AddString(subTypeComboBox, SubTypeEntries[i].Name);
                }
            }
            break;
        }

        ComboBox_SetCurSel(subTypeComboBox, 0);

        Context->LastSelectedType = type;
    }

    selectedSubTypeString = PhGetWindowText(subTypeComboBox);

    if (PhEqualString2(selectedSubTypeString, L"Custom", FALSE))
    {
        EnableWindow(GetDlgItem(hwndDlg, IDC_SUBTYPECUSTOM), TRUE);
        SetDlgItemText(hwndDlg, IDC_SUBTYPECUSTOM, Context->LastCustomSubType->Buffer);
    }
    else
    {
        if (IsWindowEnabled(GetDlgItem(hwndDlg, IDC_SUBTYPECUSTOM)))
        {
            EnableWindow(GetDlgItem(hwndDlg, IDC_SUBTYPECUSTOM), FALSE);
            PhMoveReference(&Context->LastCustomSubType, PhGetWindowText(GetDlgItem(hwndDlg, IDC_SUBTYPECUSTOM)));
            SetDlgItemText(hwndDlg, IDC_SUBTYPECUSTOM, L"");
        }
    }

    PhDereferenceObject(selectedSubTypeString);
}

PPH_STRING EspConvertNullsToNewLines(
    _In_ PPH_STRING String
    )
{
    PH_STRING_BUILDER sb;
    ULONG i;

    PhInitializeStringBuilder(&sb, String->Length);

    for (i = 0; i < (ULONG)String->Length / 2; i++)
    {
        if (String->Buffer[i] == 0)
        {
            PhAppendStringBuilderEx(&sb, L"\r\n", 4);
            continue;
        }

        PhAppendCharStringBuilder(&sb, String->Buffer[i]);
    }

    return PhFinalStringBuilderString(&sb);
}

PPH_STRING EspConvertNewLinesToNulls(
    _In_ PPH_STRING String
    )
{
    PPH_STRING text;
    SIZE_T count;
    SIZE_T i;

    text = PhCreateStringEx(NULL, String->Length + 2); // plus one character for an extra null terminator (see below)
    text->Length = 0;
    count = 0;

    for (i = 0; i < String->Length / 2; i++)
    {
        // Lines are terminated by "\r\n".
        if (String->Buffer[i] == '\r')
        {
            continue;
        }

        if (String->Buffer[i] == '\n')
        {
            text->Buffer[count++] = 0;
            continue;
        }

        text->Buffer[count++] = String->Buffer[i];
    }

    if (count != 0)
    {
        // Make sure we have an extra null terminator at the end, as required of multistrings.
        if (text->Buffer[count - 1] != 0)
            text->Buffer[count++] = 0;
    }

    text->Length = count * 2;

    return text;
}

PPH_STRING EspConvertNullsToSpaces(
    _In_ PPH_STRING String
    )
{
    PPH_STRING text;
    SIZE_T j;

    text = PhDuplicateString(String);

    for (j = 0; j < text->Length / 2; j++)
    {
        if (text->Buffer[j] == 0)
            text->Buffer[j] = ' ';
    }

    return text;
}

VOID EspFormatTriggerData(
    _In_ PES_TRIGGER_DATA Data,
    _Out_ PPH_STRING *Text
    )
{
    if (Data->Type == SERVICE_TRIGGER_DATA_TYPE_STRING)
    {
        // This check works for both normal strings and multistrings.
        if (Data->String->Length != 0)
        {
            // Prepare the text for display by replacing null characters with spaces.
            *Text = EspConvertNullsToSpaces(Data->String);
        }
        else
        {
            *Text = PhCreateString(L"(empty string)");
        }
    }
    else if (Data->Type == SERVICE_TRIGGER_DATA_TYPE_BINARY)
    {
        *Text = PhCreateString(L"(binary data)");
    }
    else if (Data->Type == SERVICE_TRIGGER_DATA_TYPE_LEVEL)
    {
        *Text = PhFormatString(L"(level) %u", Data->Byte);
    }
    else if (Data->Type == SERVICE_TRIGGER_DATA_TYPE_KEYWORD_ANY)
    {
        *Text = PhFormatString(L"(keyword any) 0x%I64x", Data->UInt64);
    }
    else if (Data->Type == SERVICE_TRIGGER_DATA_TYPE_KEYWORD_ALL)
    {
        *Text = PhFormatString(L"(keyword all) 0x%I64x", Data->UInt64);
    }
    else
    {
        *Text = PhCreateString(L"(unknown type)");
    }
}

INT_PTR CALLBACK EspServiceTriggerDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PES_TRIGGER_CONTEXT context;

    if (uMsg == WM_INITDIALOG)
    {
        context = (PES_TRIGGER_CONTEXT)lParam;
        SetProp(hwndDlg, L"Context", (HANDLE)context);
    }
    else
    {
        context = (PES_TRIGGER_CONTEXT)GetProp(hwndDlg, L"Context");

        if (uMsg == WM_DESTROY)
            RemoveProp(hwndDlg, L"Context");
    }

    if (!context)
        return FALSE;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            HWND typeComboBox;
            HWND actionComboBox;
            HWND lvHandle;
            ULONG i;

            context->LastSelectedType = 0;

            if (context->EditingInfo->Subtype)
                context->LastCustomSubType = PhFormatGuid(context->EditingInfo->Subtype);
            else
                context->LastCustomSubType = PhReferenceEmptyString();

            typeComboBox = GetDlgItem(hwndDlg, IDC_TYPE);
            actionComboBox = GetDlgItem(hwndDlg, IDC_ACTION);

            for (i = 0; i < sizeof(TypeEntries) / sizeof(TYPE_ENTRY); i++)
            {
                ComboBox_AddString(typeComboBox, TypeEntries[i].Name);

                if (TypeEntries[i].Type == context->EditingInfo->Type)
                {
                    PhSelectComboBoxString(typeComboBox, TypeEntries[i].Name, FALSE);
                }
            }

            ComboBox_AddString(actionComboBox, L"Start");
            ComboBox_AddString(actionComboBox, L"Stop");
            ComboBox_SetCurSel(actionComboBox, context->EditingInfo->Action == SERVICE_TRIGGER_ACTION_SERVICE_START ? 0 : 1);

            EspFixServiceTriggerControls(hwndDlg, context);

            if (context->EditingInfo->Type != SERVICE_TRIGGER_TYPE_CUSTOM)
            {
                for (i = 0; i < sizeof(SubTypeEntries) / sizeof(SUBTYPE_ENTRY); i++)
                {
                    if (
                        SubTypeEntries[i].Type == context->EditingInfo->Type &&
                        SubTypeEntries[i].Guid &&
                        context->EditingInfo->Subtype &&
                        memcmp(SubTypeEntries[i].Guid, context->EditingInfo->Subtype, sizeof(GUID)) == 0
                        )
                    {
                        PhSelectComboBoxString(GetDlgItem(hwndDlg, IDC_SUBTYPE), SubTypeEntries[i].Name, FALSE);
                        break;
                    }
                }
            }
            else
            {
                if (context->EditingInfo->Subtype)
                {
                    PPH_STRING publisherName;

                    // Try to select the publisher name in the subtype list.
                    publisherName = EspLookupEtwPublisherName(context->EditingInfo->Subtype);
                    PhSelectComboBoxString(GetDlgItem(hwndDlg, IDC_SUBTYPE), publisherName->Buffer, FALSE);
                    PhDereferenceObject(publisherName);
                }
            }

            // Call a second time since the state of the custom subtype text box may have changed.
            EspFixServiceTriggerControls(hwndDlg, context);

            lvHandle = GetDlgItem(hwndDlg, IDC_LIST);
            PhSetListViewStyle(lvHandle, FALSE, TRUE);
            PhSetControlTheme(lvHandle, L"explorer");
            PhAddListViewColumn(lvHandle, 0, 0, 0, LVCFMT_LEFT, 280, L"Data");

            if (context->EditingInfo->DataList)
            {
                for (i = 0; i < context->EditingInfo->DataList->Count; i++)
                {
                    PES_TRIGGER_DATA data;
                    PPH_STRING text;
                    INT lvItemIndex;

                    data = context->EditingInfo->DataList->Items[i];

                    EspFormatTriggerData(data, &text);
                    lvItemIndex = PhAddListViewItem(lvHandle, MAXINT, text->Buffer, data);
                    PhDereferenceObject(text);
                }
            }

            EnableWindow(GetDlgItem(hwndDlg, IDC_EDIT), FALSE);
            EnableWindow(GetDlgItem(hwndDlg, IDC_DELETE), FALSE);
        }
        break;
    case WM_DESTROY:
        {
            PhClearReference(&context->LastCustomSubType);
        }
        break;
    case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
            case IDC_TYPE:
                if (HIWORD(wParam) == CBN_SELCHANGE)
                {
                    EspFixServiceTriggerControls(hwndDlg, context);
                }
                break;
            case IDC_SUBTYPE:
                if (HIWORD(wParam) == CBN_SELCHANGE)
                {
                    EspFixServiceTriggerControls(hwndDlg, context);
                }
                break;
            case IDC_NEW:
                {
                    HWND lvHandle;

                    lvHandle = GetDlgItem(hwndDlg, IDC_LIST);
                    context->EditingValue = PhReferenceEmptyString();

                    if (DialogBoxParam(
                        PluginInstance->DllBase,
                        MAKEINTRESOURCE(IDD_VALUE),
                        hwndDlg,
                        ValueDlgProc,
                        (LPARAM)context
                        ) == IDOK)
                    {
                        PES_TRIGGER_DATA data;
                        PPH_STRING text;
                        INT lvItemIndex;

                        data = EspCreateTriggerData(NULL);
                        data->Type = SERVICE_TRIGGER_DATA_TYPE_STRING;
                        data->String = EspConvertNewLinesToNulls(context->EditingValue);

                        if (!context->EditingInfo->DataList)
                            context->EditingInfo->DataList = PhCreateList(4);

                        PhAddItemList(context->EditingInfo->DataList, data);

                        EspFormatTriggerData(data, &text);
                        lvItemIndex = PhAddListViewItem(lvHandle, MAXINT, text->Buffer, data);
                        PhDereferenceObject(text);
                    }

                    PhClearReference(&context->EditingValue);
                }
                break;
            case IDC_EDIT:
                {
                    HWND lvHandle;
                    INT lvItemIndex;
                    PES_TRIGGER_DATA data;
                    ULONG index;

                    lvHandle = GetDlgItem(hwndDlg, IDC_LIST);
                    lvItemIndex = ListView_GetNextItem(lvHandle, -1, LVNI_SELECTED);

                    if (
                        lvItemIndex != -1 && PhGetListViewItemParam(lvHandle, lvItemIndex, (PVOID *)&data) &&
                        data->Type == SERVICE_TRIGGER_DATA_TYPE_STRING // editing binary values is not supported
                        )
                    {
                        index = PhFindItemList(context->EditingInfo->DataList, data);

                        if (index != -1)
                        {
                            context->EditingValue = EspConvertNullsToNewLines(data->String);

                            if (DialogBoxParam(
                                PluginInstance->DllBase,
                                MAKEINTRESOURCE(IDD_VALUE),
                                hwndDlg,
                                ValueDlgProc,
                                (LPARAM)context
                                ) == IDOK)
                            {
                                PPH_STRING text;

                                PhMoveReference(&data->String, EspConvertNewLinesToNulls(context->EditingValue));

                                EspFormatTriggerData(data, &text);
                                PhSetListViewSubItem(lvHandle, lvItemIndex, 0, text->Buffer);
                                PhDereferenceObject(text);
                            }

                            PhClearReference(&context->EditingValue);
                        }
                    }
                }
                break;
            case IDC_DELETE:
                {
                    HWND lvHandle;
                    INT lvItemIndex;
                    PES_TRIGGER_DATA data;
                    ULONG index;

                    lvHandle = GetDlgItem(hwndDlg, IDC_LIST);
                    lvItemIndex = ListView_GetNextItem(lvHandle, -1, LVNI_SELECTED);

                    if (lvItemIndex != -1 && PhGetListViewItemParam(lvHandle, lvItemIndex, (PVOID *)&data))
                    {
                        index = PhFindItemList(context->EditingInfo->DataList, data);

                        if (index != -1)
                        {
                            EspDestroyTriggerData(data);
                            PhRemoveItemList(context->EditingInfo->DataList, index);
                            PhRemoveListViewItem(lvHandle, lvItemIndex);
                        }
                    }
                }
                break;
            case IDCANCEL:
                EndDialog(hwndDlg, IDCANCEL);
                break;
            case IDOK:
                {
                    PH_AUTO_POOL autoPool;
                    PPH_STRING typeString;
                    PPH_STRING subTypeString;
                    PPH_STRING customSubTypeString;
                    PPH_STRING actionString;
                    ULONG i;

                    PhInitializeAutoPool(&autoPool);

                    typeString = PhaGetDlgItemText(hwndDlg, IDC_TYPE);
                    subTypeString = PhaGetDlgItemText(hwndDlg, IDC_SUBTYPE);
                    customSubTypeString = PhaGetDlgItemText(hwndDlg, IDC_SUBTYPECUSTOM);
                    actionString = PhaGetDlgItemText(hwndDlg, IDC_ACTION);

                    for (i = 0; i < sizeof(TypeEntries) / sizeof(TYPE_ENTRY); i++)
                    {
                        if (PhEqualStringZ(TypeEntries[i].Name, typeString->Buffer, FALSE))
                        {
                            context->EditingInfo->Type = TypeEntries[i].Type;
                            break;
                        }
                    }

                    if (!PhEqualString2(subTypeString, L"Custom", FALSE))
                    {
                        if (context->EditingInfo->Type != SERVICE_TRIGGER_TYPE_CUSTOM)
                        {
                            for (i = 0; i < sizeof(SubTypeEntries) / sizeof(SUBTYPE_ENTRY); i++)
                            {
                                if (
                                    SubTypeEntries[i].Type == context->EditingInfo->Type &&
                                    PhEqualString2(subTypeString, SubTypeEntries[i].Name, FALSE)
                                    )
                                {
                                    context->EditingInfo->SubtypeBuffer = *SubTypeEntries[i].Guid;
                                    context->EditingInfo->Subtype = &context->EditingInfo->SubtypeBuffer;
                                    break;
                                }
                            }
                        }
                        else
                        {
                            if (!EspLookupEtwPublisherGuid(&subTypeString->sr, &context->EditingInfo->SubtypeBuffer))
                            {
                                PhShowError(hwndDlg, L"Unable to find the ETW publisher GUID.");
                                goto DoNotClose;
                            }

                            context->EditingInfo->Subtype = &context->EditingInfo->SubtypeBuffer;
                        }
                    }
                    else
                    {
                        UNICODE_STRING guidString;

                        PhStringRefToUnicodeString(&customSubTypeString->sr, &guidString);

                        // Trim whitespace.

                        while (guidString.Length != 0 && *guidString.Buffer == ' ')
                        {
                            guidString.Buffer++;
                            guidString.Length -= 2;
                        }

                        while (guidString.Length != 0 && guidString.Buffer[guidString.Length / 2 - 1] == ' ')
                        {
                            guidString.Length -= 2;
                        }

                        if (NT_SUCCESS(RtlGUIDFromString(&guidString, &context->EditingInfo->SubtypeBuffer)))
                        {
                            context->EditingInfo->Subtype = &context->EditingInfo->SubtypeBuffer;
                        }
                        else
                        {
                            PhShowError(hwndDlg, L"The custom subtype is invalid. Please ensure that the string is a valid GUID: \"{x-x-x-x-x}\".");
                            goto DoNotClose;
                        }
                    }

                    if (PhEqualString2(actionString, L"Start", FALSE))
                        context->EditingInfo->Action = SERVICE_TRIGGER_ACTION_SERVICE_START;
                    else
                        context->EditingInfo->Action = SERVICE_TRIGGER_ACTION_SERVICE_STOP;

                    if (
                        context->EditingInfo->DataList &&
                        context->EditingInfo->DataList->Count != 0 &&
                        context->EditingInfo->Type != SERVICE_TRIGGER_TYPE_DEVICE_INTERFACE_ARRIVAL &&
                        context->EditingInfo->Type != SERVICE_TRIGGER_TYPE_FIREWALL_PORT_EVENT &&
                        context->EditingInfo->Type != SERVICE_TRIGGER_TYPE_NETWORK_ENDPOINT &&
                        context->EditingInfo->Type != SERVICE_TRIGGER_TYPE_CUSTOM
                        )
                    {
                        // This trigger has data items, but the trigger type doesn't allow them.
                        if (PhShowMessage(
                            hwndDlg,
                            MB_OKCANCEL | MB_ICONWARNING,
                            L"The trigger type \"%s\" does not allow data items to be configured. "
                            L"If you continue, they will be removed.",
                            typeString->Buffer
                            ) != IDOK)
                        {
                            goto DoNotClose;
                        }

                        for (i = 0; i < context->EditingInfo->DataList->Count; i++)
                        {
                            EspDestroyTriggerData(context->EditingInfo->DataList->Items[i]);
                        }

                        PhClearReference(&context->EditingInfo->DataList);
                    }

                    EndDialog(hwndDlg, IDOK);

DoNotClose:
                    PhDeleteAutoPool(&autoPool);
                }
                break;
            }
        }
        break;
    case WM_NOTIFY:
        {
            LPNMHDR header = (LPNMHDR)lParam;
            HWND lvHandle;

            lvHandle = GetDlgItem(hwndDlg, IDC_LIST);

            switch (header->code)
            {
            case LVN_ITEMCHANGED:
                {
                    if (header->hwndFrom == lvHandle)
                    {
                        if (ListView_GetSelectedCount(lvHandle) == 1)
                        {
                            PES_TRIGGER_DATA data = PhGetSelectedListViewItemParam(GetDlgItem(hwndDlg, IDC_LIST));

                            // Editing binary data is not supported.
                            EnableWindow(GetDlgItem(hwndDlg, IDC_EDIT), data && data->Type == SERVICE_TRIGGER_DATA_TYPE_STRING);
                            EnableWindow(GetDlgItem(hwndDlg, IDC_DELETE), TRUE);
                        }
                        else
                        {
                            EnableWindow(GetDlgItem(hwndDlg, IDC_EDIT), FALSE);
                            EnableWindow(GetDlgItem(hwndDlg, IDC_DELETE), FALSE);
                        }
                    }
                }
                break;
            case NM_DBLCLK:
                {
                    if (header->hwndFrom == lvHandle)
                    {
                        SendMessage(hwndDlg, WM_COMMAND, IDC_EDIT, 0);
                    }
                }
                break;
            }
        }
        break;
    }

    return FALSE;
}

static INT_PTR CALLBACK ValueDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PES_TRIGGER_CONTEXT context;

    if (uMsg == WM_INITDIALOG)
    {
        context = (PES_TRIGGER_CONTEXT)lParam;
        SetProp(hwndDlg, L"Context", (HANDLE)context);
    }
    else
    {
        context = (PES_TRIGGER_CONTEXT)GetProp(hwndDlg, L"Context");

        if (uMsg == WM_DESTROY)
            RemoveProp(hwndDlg, L"Context");
    }

    if (!context)
        return FALSE;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            SetDlgItemText(hwndDlg, IDC_VALUES, context->EditingValue->Buffer);
            SendMessage(hwndDlg, WM_NEXTDLGCTL, (WPARAM)GetDlgItem(hwndDlg, IDC_VALUES), TRUE);
            Edit_SetSel(GetDlgItem(hwndDlg, IDC_VALUES), 0, -1);
        }
        break;
    case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
            case IDCANCEL:
                EndDialog(hwndDlg, IDCANCEL);
                break;
            case IDOK:
                PhMoveReference(&context->EditingValue, PhGetWindowText(GetDlgItem(hwndDlg, IDC_VALUES)));
                EndDialog(hwndDlg, IDOK);
                break;
            }
        }
        break;
    }

    return FALSE;
}
