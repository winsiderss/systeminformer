/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2011-2015
 *     dmex    2020-2023
 *
 */

#include "extsrv.h"
#include <hndlinfo.h>

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

typedef struct _TYPE_ENTRY
{
    ULONG Type;
    PWSTR Name;
} TYPE_ENTRY, PTYPE_ENTRY;

typedef struct _SUBTYPE_ENTRY
{
    ULONG Type;
    PCGUID Guid;
    PWSTR Name;
} SUBTYPE_ENTRY, PSUBTYPE_ENTRY;

typedef struct _ETW_PUBLISHER_ENTRY
{
    PPH_STRING PublisherName;
    GUID Guid;
} ETW_PUBLISHER_ENTRY, *PETW_PUBLISHER_ENTRY;

INT_PTR CALLBACK EspServiceTriggerDlgProc(
    _In_ HWND WindowHandle,
    _In_ UINT WindowMessage,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

INT_PTR CALLBACK ValueDlgProc(
    _In_ HWND WindowHandle,
    _In_ UINT WindowMessage,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

DEFINE_GUID(NetworkManagerFirstIpAddressArrivalGuid, 0x4f27f2de, 0x14e2, 0x430b, 0xa5, 0x49, 0x7c, 0xd4, 0x8c, 0xbc, 0x82, 0x45);
DEFINE_GUID(NetworkManagerLastIpAddressRemovalGuid, 0xcc4ba62a, 0x162e, 0x4648, 0x84, 0x7a, 0xb6, 0xbd, 0xf9, 0x93, 0xe3, 0x35);
DEFINE_GUID(DomainJoinGuid, 0x1ce20aba, 0x9851, 0x4421, 0x94, 0x30, 0x1d, 0xde, 0xb7, 0x66, 0xe8, 0x09);
DEFINE_GUID(DomainLeaveGuid, 0xddaf516e, 0x58c2, 0x4866, 0x95, 0x74, 0xc3, 0xb6, 0x15, 0xd4, 0x2e, 0xa1);
DEFINE_GUID(FirewallPortOpenGuid, 0xb7569e07, 0x8421, 0x4ee0, 0xad, 0x10, 0x86, 0x91, 0x5a, 0xfd, 0xad, 0x09);
DEFINE_GUID(FirewallPortCloseGuid, 0xa144ed38, 0x8e12, 0x4de4, 0x9d, 0x96, 0xe6, 0x47, 0x40, 0xb1, 0xa5, 0x24);
DEFINE_GUID(MachinePolicyPresentGuid, 0x659fcae6, 0x5bdb, 0x4da9, 0xb1, 0xff, 0xca, 0x2a, 0x17, 0x8d, 0x46, 0xe0);
DEFINE_GUID(UserPolicyPresentGuid, 0x54fb46c8, 0xf089, 0x464c, 0xb1, 0xfd, 0x59, 0xd1, 0xb6, 0x2c, 0x3b, 0x50);
DEFINE_GUID(RpcInterfaceEventGuid, 0xbc90d167, 0x9470, 0x4139, 0xa9, 0xba, 0xbe, 0x0b, 0xbb, 0xf5, 0xb7, 0x4d);
DEFINE_GUID(NamedPipeEventGuid, 0x1f81d131, 0x3fac, 0x4537, 0x9e, 0x0c, 0x7e, 0x7b, 0x0c, 0x2f, 0x4b, 0x55);
DEFINE_GUID(SubTypeUnknownGuid, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);

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

PES_TRIGGER_CONTEXT EsCreateServiceTriggerContext(
    _In_ PPH_SERVICE_ITEM ServiceItem,
    _In_ HWND WindowHandle,
    _In_ HWND TriggersLv
    )
{
    PES_TRIGGER_CONTEXT context;

    context = PhAllocateZero(sizeof(ES_TRIGGER_CONTEXT));
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
    _In_ PES_TRIGGER_CONTEXT Context
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

BOOLEAN NTAPI EspEtwPublishersEnumerateKeyCallback(
    _In_ HANDLE RootDirectory,
    _In_ PKEY_BASIC_INFORMATION Information,
    _In_ PVOID Context
    )
{
    PH_STRINGREF keyName;
    HANDLE keyHandle;
    GUID guid;
    PPH_STRING publisherName;

    keyName.Buffer = Information->Name;
    keyName.Length = Information->NameLength;

    // Make sure this is a valid publisher key. (wj32)
    if (NT_SUCCESS(PhStringToGuid(&keyName, &guid)))
    {
        if (NT_SUCCESS(PhOpenKey(
            &keyHandle,
            KEY_READ,
            RootDirectory,
            &keyName,
            0
            )))
        {
            publisherName = PhQueryRegistryString(keyHandle, NULL);

            if (publisherName)
            {
                if (publisherName->Length != 0)
                {
                    ETW_PUBLISHER_ENTRY entry;

                    memset(&entry, 0, sizeof(ETW_PUBLISHER_ENTRY));
                    PhMoveReference(&entry.PublisherName, publisherName);
                    memcpy_s(&entry.Guid, sizeof(entry.Guid), &guid, sizeof(GUID));

                    PhAddItemArray(Context, &entry);
                }
                else
                {
                    PhDereferenceObject(publisherName);
                }
            }

            NtClose(keyHandle);
        }
    }

    return TRUE;
}

NTSTATUS EspEnumerateEtwPublishers(
    _Out_ PETW_PUBLISHER_ENTRY *Entries,
    _Out_ PULONG NumberOfEntries
    )
{
    NTSTATUS status;
    HANDLE publishersKeyHandle;
    PH_ARRAY publishersArrayList;

    status = PhOpenKey(
        &publishersKeyHandle,
        KEY_READ,
        PH_KEY_LOCAL_MACHINE,
        &PublishersKeyName,
        0
        );

    if (!NT_SUCCESS(status))
        return status;

    PhInitializeArray(&publishersArrayList, sizeof(ETW_PUBLISHER_ENTRY), 100);
    PhEnumerateKey(
        publishersKeyHandle,
        KeyBasicInformation,
        EspEtwPublishersEnumerateKeyCallback,
        &publishersArrayList
        );
    NtClose(publishersKeyHandle);

    *Entries = PhFinalArrayItems(&publishersArrayList);
    *NumberOfEntries = (ULONG)publishersArrayList.Count;

    return STATUS_SUCCESS;
}

_Success_(return)
BOOLEAN EspLookupEtwPublisherGuid(
    _In_ PPH_STRINGREF PublisherName,
    _Out_ PGUID Guid
    )
{
    BOOLEAN result;
    PETW_PUBLISHER_ENTRY entries;
    ULONG numberOfEntries;
    ULONG i;

    if (!NT_SUCCESS(EspEnumerateEtwPublishers(&entries, &numberOfEntries)))
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

                // Try to lookup the publisher name from the GUID. (wj32)
                publisherName = PhGetEtwPublisherName(Info->Subtype);
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
                    else if (Info->Subtype && SubTypeEntries[i].Guid && IsEqualGUID(Info->Subtype, SubTypeEntries[i].Guid))
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
    _In_ PES_TRIGGER_CONTEXT Context,
    _In_ SC_HANDLE ServiceHandle
    )
{
    PSERVICE_TRIGGER_INFO triggerInfo;
    ULONG i;

    EspClearTriggerInfoList(Context->InfoList);

    if (NT_SUCCESS(PhQueryServiceVariableSize(ServiceHandle, SERVICE_CONFIG_TRIGGER_INFO, &triggerInfo)))
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

_Success_(return)
BOOLEAN EsSaveServiceTriggerInfo(
    _In_ PES_TRIGGER_CONTEXT Context,
    _Out_opt_ PNTSTATUS NtResult
    )
{
    BOOLEAN result = TRUE;
    NTSTATUS status;
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
        triggerInfo.pTriggers = PH_AUTO(PhCreateAlloc(Context->InfoList->Count * sizeof(SERVICE_TRIGGER)));
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
                trigger->pDataItems = PH_AUTO(PhCreateAlloc(info->DataList->Count * sizeof(SERVICE_TRIGGER_SPECIFIC_DATA_ITEM)));

                for (j = 0; j < info->DataList->Count; j++)
                {
                    PSERVICE_TRIGGER_SPECIFIC_DATA_ITEM dataItem = &trigger->pDataItems[j];
                    PES_TRIGGER_DATA data = info->DataList->Items[j];

                    dataItem->dwDataType = data->Type;

                    if (data->Type == SERVICE_TRIGGER_DATA_TYPE_STRING)
                    {
                        dataItem->cbData = (ULONG)data->String->Length + sizeof(UNICODE_NULL); // include null terminator
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

    status = PhOpenService(&serviceHandle, SERVICE_CHANGE_CONFIG, PhGetString(Context->ServiceItem->Name));

    if (NT_SUCCESS(status))
    {
        status = PhChangeServiceConfig2(serviceHandle, SERVICE_CONFIG_TRIGGER_INFO, &triggerInfo);

        if (!NT_SUCCESS(status))
        {
            result = FALSE;
        }

        PhCloseServiceHandle(serviceHandle);
    }
    else
    {
        result = FALSE;

        if (status == STATUS_ACCESS_DENIED && !PhGetOwnTokenAttributes().Elevated)
        {
            // Elevate using phsvc.
            if (PhUiConnectToPhSvc(Context->WindowHandle, FALSE))
            {
                result = TRUE;

                if (!NT_SUCCESS(status = PhSvcCallChangeServiceConfig2(
                    PhGetString(Context->ServiceItem->Name),
                    SERVICE_CONFIG_TRIGGER_INFO,
                    &triggerInfo
                    )))
                {
                    result = FALSE;
                }

                PhUiDisconnectFromPhSvc();
            }
            else
            {
                // User cancelled elevation.
                status = STATUS_CANCELLED;
            }
        }
    }

    PhDeleteAutoPool(&autoPool);

    if (NtResult)
        *NtResult = status;

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
    _In_ PES_TRIGGER_CONTEXT Context,
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

            if (PhDialogBox(
                PluginInstance->DllBase,
                MAKEINTRESOURCE(IDD_SRVTRIGGER),
                Context->WindowHandle,
                EspServiceTriggerDlgProc,
                Context
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

            lvItemIndex = PhFindListViewItemByFlags(Context->TriggersLv, INT_ERROR, LVNI_SELECTED);

            if (lvItemIndex != INT_ERROR && PhGetListViewItemParam(Context->TriggersLv, lvItemIndex, (PVOID *)&info))
            {
                index = PhFindItemList(Context->InfoList, info);

                if (index != ULONG_MAX)
                {
                    Context->EditingInfo = EspCloneTriggerInfo(info);

                    if (PhDialogBox(
                        PluginInstance->DllBase,
                        MAKEINTRESOURCE(IDD_SRVTRIGGER),
                        Context->WindowHandle,
                        EspServiceTriggerDlgProc,
                        Context
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

            lvItemIndex = PhFindListViewItemByFlags(Context->TriggersLv, INT_ERROR, LVNI_SELECTED);

            if (lvItemIndex != INT_ERROR && PhGetListViewItemParam(Context->TriggersLv, lvItemIndex, (PVOID *)&info))
            {
                index = PhFindItemList(Context->InfoList, info);

                if (index != ULONG_MAX)
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

ULONG EspTriggerTypeStringToInteger(
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

VOID EspFixServiceTriggerControls(
    _In_ HWND WindowHandle,
    _In_ PES_TRIGGER_CONTEXT Context
    )
{
    HWND typeComboBox;
    HWND subTypeComboBox;
    ULONG i;
    PPH_STRING selectedTypeString;
    ULONG type;
    PPH_STRING selectedSubTypeString;

    typeComboBox = GetDlgItem(WindowHandle, IDC_TYPE);
    subTypeComboBox = GetDlgItem(WindowHandle, IDC_SUBTYPE);

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

                ComboBox_AddString(subTypeComboBox, L"Custom");

                // Display a list of publishers.
                if (NT_SUCCESS(EspEnumerateEtwPublishers(&entries, &numberOfEntries)))
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
        EnableWindow(GetDlgItem(WindowHandle, IDC_SUBTYPECUSTOM), TRUE);
        PhSetDialogItemText(WindowHandle, IDC_SUBTYPECUSTOM, Context->LastCustomSubType->Buffer);
    }
    else
    {
        if (IsWindowEnabled(GetDlgItem(WindowHandle, IDC_SUBTYPECUSTOM)))
        {
            EnableWindow(GetDlgItem(WindowHandle, IDC_SUBTYPECUSTOM), FALSE);
            PhMoveReference(&Context->LastCustomSubType, PhGetWindowText(GetDlgItem(WindowHandle, IDC_SUBTYPECUSTOM)));
            PhSetDialogItemText(WindowHandle, IDC_SUBTYPECUSTOM, L"");
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

    text = PhCreateStringEx(NULL, String->Length + sizeof(UNICODE_NULL)); // plus one character for an extra null terminator (see below)
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
            text->Buffer[count++] = UNICODE_NULL;
            continue;
        }

        text->Buffer[count++] = String->Buffer[i];
    }

    if (count != 0)
    {
        // Make sure we have an extra null terminator at the end, as required of multistrings.
        if (text->Buffer[count - 1] != UNICODE_NULL)
            text->Buffer[count++] = UNICODE_NULL;
    }

    text->Length = count * sizeof(WCHAR);

    return text;
}

PPH_STRING EspConvertNullsToSpaces(
    _In_ PPH_STRING String
    )
{
    PPH_STRING text;
    SIZE_T j;

    text = PhDuplicateString(String);

    for (j = 0; j < text->Length / sizeof(WCHAR); j++)
    {
        if (text->Buffer[j] == UNICODE_NULL)
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
    _In_ HWND WindowHandle,
    _In_ UINT WindowMessage,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PES_TRIGGER_CONTEXT context;

    if (WindowMessage == WM_INITDIALOG)
    {
        context = (PES_TRIGGER_CONTEXT)lParam;
        PhSetWindowContext(WindowHandle, PH_WINDOW_CONTEXT_DEFAULT, context);
    }
    else
    {
        context = PhGetWindowContext(WindowHandle, PH_WINDOW_CONTEXT_DEFAULT);
    }

    if (!context)
        return FALSE;

    switch (WindowMessage)
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

            typeComboBox = GetDlgItem(WindowHandle, IDC_TYPE);
            actionComboBox = GetDlgItem(WindowHandle, IDC_ACTION);

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

            EspFixServiceTriggerControls(WindowHandle, context);

            if (context->EditingInfo->Type != SERVICE_TRIGGER_TYPE_CUSTOM)
            {
                for (i = 0; i < sizeof(SubTypeEntries) / sizeof(SUBTYPE_ENTRY); i++)
                {
                    if (
                        SubTypeEntries[i].Type == context->EditingInfo->Type &&
                        SubTypeEntries[i].Guid &&
                        context->EditingInfo->Subtype &&
                        IsEqualGUID(SubTypeEntries[i].Guid, context->EditingInfo->Subtype)
                        )
                    {
                        PhSelectComboBoxString(GetDlgItem(WindowHandle, IDC_SUBTYPE), SubTypeEntries[i].Name, FALSE);
                        break;
                    }
                }
            }
            else
            {
                if (context->EditingInfo->Subtype)
                {
                    PPH_STRING publisherName;

                    // Try to select the publisher name in the subtype list. (wj32)
                    publisherName = PhGetEtwPublisherName(context->EditingInfo->Subtype);
                    PhSelectComboBoxString(GetDlgItem(WindowHandle, IDC_SUBTYPE), publisherName->Buffer, FALSE);
                    PhDereferenceObject(publisherName);
                }
            }

            // Call a second time since the state of the custom subtype text box may have changed.
            EspFixServiceTriggerControls(WindowHandle, context);

            lvHandle = GetDlgItem(WindowHandle, IDC_LIST);
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

            EnableWindow(GetDlgItem(WindowHandle, IDC_EDIT), FALSE);
            EnableWindow(GetDlgItem(WindowHandle, IDC_DELETE), FALSE);

            PhInitializeWindowTheme(WindowHandle, !!PhGetIntegerSetting(L"EnableThemeSupport"));
        }
        break;
    case WM_DESTROY:
        {
            PhRemoveWindowContext(WindowHandle, PH_WINDOW_CONTEXT_DEFAULT);
            PhClearReference(&context->LastCustomSubType);
        }
        break;
    case WM_COMMAND:
        {
            switch (GET_WM_COMMAND_ID(wParam, lParam))
            {
            case IDC_TYPE:
                if (GET_WM_COMMAND_CMD(wParam, lParam) == CBN_SELCHANGE)
                {
                    EspFixServiceTriggerControls(WindowHandle, context);
                }
                break;
            case IDC_SUBTYPE:
                if (GET_WM_COMMAND_CMD(wParam, lParam) == CBN_SELCHANGE)
                {
                    EspFixServiceTriggerControls(WindowHandle, context);
                }
                break;
            case IDC_NEW:
                {
                    HWND lvHandle;

                    lvHandle = GetDlgItem(WindowHandle, IDC_LIST);
                    context->EditingValue = PhReferenceEmptyString();

                    if (PhDialogBox(
                        PluginInstance->DllBase,
                        MAKEINTRESOURCE(IDD_VALUE),
                        WindowHandle,
                        ValueDlgProc,
                        context
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

                    lvHandle = GetDlgItem(WindowHandle, IDC_LIST);
                    lvItemIndex = PhFindListViewItemByFlags(lvHandle, INT_ERROR, LVNI_SELECTED);

                    if (
                        lvItemIndex != INT_ERROR && PhGetListViewItemParam(lvHandle, lvItemIndex, (PVOID *)&data) &&
                        data->Type == SERVICE_TRIGGER_DATA_TYPE_STRING // editing binary values is not supported
                        )
                    {
                        index = PhFindItemList(context->EditingInfo->DataList, data);

                        if (index != ULONG_MAX)
                        {
                            context->EditingValue = EspConvertNullsToNewLines(data->String);

                            if (PhDialogBox(
                                PluginInstance->DllBase,
                                MAKEINTRESOURCE(IDD_VALUE),
                                WindowHandle,
                                ValueDlgProc,
                                context
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

                    lvHandle = GetDlgItem(WindowHandle, IDC_LIST);
                    lvItemIndex = PhFindListViewItemByFlags(lvHandle, INT_ERROR, LVNI_SELECTED);

                    if (lvItemIndex != INT_ERROR && PhGetListViewItemParam(lvHandle, lvItemIndex, (PVOID *)&data))
                    {
                        index = PhFindItemList(context->EditingInfo->DataList, data);

                        if (index != ULONG_MAX)
                        {
                            EspDestroyTriggerData(data);
                            PhRemoveItemList(context->EditingInfo->DataList, index);
                            PhRemoveListViewItem(lvHandle, lvItemIndex);
                        }
                    }
                }
                break;
            case IDCANCEL:
                EndDialog(WindowHandle, IDCANCEL);
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

                    typeString = PhaGetDlgItemText(WindowHandle, IDC_TYPE);
                    subTypeString = PhaGetDlgItemText(WindowHandle, IDC_SUBTYPE);
                    customSubTypeString = PhaGetDlgItemText(WindowHandle, IDC_SUBTYPECUSTOM);
                    actionString = PhaGetDlgItemText(WindowHandle, IDC_ACTION);

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
                                PhShowError(WindowHandle, L"%s", L"Unable to find the ETW publisher GUID.");
                                goto DoNotClose;
                            }

                            context->EditingInfo->Subtype = &context->EditingInfo->SubtypeBuffer;
                        }
                    }
                    else
                    {
                        PH_STRINGREF guidString = customSubTypeString->sr;

                        // Trim whitespace.

                        while (guidString.Length != 0 && *guidString.Buffer == L' ')
                        {
                            guidString.Buffer++;
                            guidString.Length -= sizeof(WCHAR);
                        }

                        while (guidString.Length != 0 && guidString.Buffer[guidString.Length / sizeof(WCHAR) - 1] == L' ')
                        {
                            guidString.Length -= sizeof(WCHAR);
                        }

                        if (NT_SUCCESS(PhStringToGuid(&guidString, &context->EditingInfo->SubtypeBuffer)))
                        {
                            context->EditingInfo->Subtype = &context->EditingInfo->SubtypeBuffer;
                        }
                        else
                        {
                            PhShowError(WindowHandle, L"%s", L"The custom subtype is invalid. Please ensure that the string is a valid GUID: \"{x-x-x-x-x}\".");
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
                            WindowHandle,
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

                    EndDialog(WindowHandle, IDOK);

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

            lvHandle = GetDlgItem(WindowHandle, IDC_LIST);

            switch (header->code)
            {
            case LVN_ITEMCHANGED:
                {
                    if (header->hwndFrom == lvHandle)
                    {
                        if (ListView_GetSelectedCount(lvHandle) == 1)
                        {
                            PES_TRIGGER_DATA data = PhGetSelectedListViewItemParam(GetDlgItem(WindowHandle, IDC_LIST));

                            // Editing binary data is not supported.
                            EnableWindow(GetDlgItem(WindowHandle, IDC_EDIT), data && data->Type == SERVICE_TRIGGER_DATA_TYPE_STRING);
                            EnableWindow(GetDlgItem(WindowHandle, IDC_DELETE), TRUE);
                        }
                        else
                        {
                            EnableWindow(GetDlgItem(WindowHandle, IDC_EDIT), FALSE);
                            EnableWindow(GetDlgItem(WindowHandle, IDC_DELETE), FALSE);
                        }
                    }
                }
                break;
            case NM_DBLCLK:
                {
                    if (header->hwndFrom == lvHandle)
                    {
                        SendMessage(WindowHandle, WM_COMMAND, IDC_EDIT, 0);
                    }
                }
                break;
            }
        }
        break;
    }

    return FALSE;
}

INT_PTR CALLBACK ValueDlgProc(
    _In_ HWND WindowHandle,
    _In_ UINT WindowMessage,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PES_TRIGGER_CONTEXT context;

    if (WindowMessage == WM_INITDIALOG)
    {
        context = (PES_TRIGGER_CONTEXT)lParam;
        PhSetWindowContext(WindowHandle, PH_WINDOW_CONTEXT_DEFAULT, context);
    }
    else
    {
        context = PhGetWindowContext(WindowHandle, PH_WINDOW_CONTEXT_DEFAULT);
    }

    if (!context)
        return FALSE;

    switch (WindowMessage)
    {
    case WM_INITDIALOG:
        {
            PhSetDialogItemText(WindowHandle, IDC_VALUES, context->EditingValue->Buffer);
            PhSetDialogFocus(WindowHandle, GetDlgItem(WindowHandle, IDC_VALUES));
            Edit_SetSel(GetDlgItem(WindowHandle, IDC_VALUES), 0, -1);

            PhInitializeWindowTheme(WindowHandle, !!PhGetIntegerSetting(L"EnableThemeSupport"));
        }
        break;
    case WM_DESTROY:
        {
            PhRemoveWindowContext(WindowHandle, PH_WINDOW_CONTEXT_DEFAULT);
        }
        break;
    case WM_COMMAND:
        {
            switch (GET_WM_COMMAND_ID(wParam, lParam))
            {
            case IDCANCEL:
                EndDialog(WindowHandle, IDCANCEL);
                break;
            case IDOK:
                PhMoveReference(&context->EditingValue, PhGetWindowText(GetDlgItem(WindowHandle, IDC_VALUES)));
                EndDialog(WindowHandle, IDOK);
                break;
            }
        }
        break;
    }

    return FALSE;
}
