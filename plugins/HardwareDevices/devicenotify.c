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

#include "devices.h"
#include "deviceprv.h"

#include <devguid.h>

static PH_INITONCE DeviceNotifyInitOnce = PH_INITONCE_INIT;
static LIST_ENTRY DeviceNotifyList = { 0 };
static PH_FAST_LOCK DeviceNotifyLock = PH_FAST_LOCK_INIT;
static HANDLE DeviceNotifyEvent = NULL;
static PH_STRINGREF DeviceString = PH_STRINGREF_INIT(L"Device");

typedef struct _DEVICE_NOTIFY_ENTRY
{
    LIST_ENTRY ListEntry;
    BOOLEAN Removed;
    PPH_STRING InstanceId;
} DEVICE_NOTIFY_ENTRY, *PDEVICE_NOTIFY_ENTRY;

PPH_STRING NotifyDeviceGetString(
    _In_ PDEVICE_NODE Node,
    _In_ DEVNODE_PROP_KEY Property,
    _In_ BOOLEAN TrimDevice
    )
{
    PH_STRINGREF sr;
    PPH_STRING string;

    string = GetDeviceNodeProperty(Node, Property)->AsString;
    if (!string || string->Length == 0)
        return NULL;

    sr = string->sr;

    if (TrimDevice && PhEndsWithStringRef(&sr, &DeviceString, TRUE))
        sr.Length -= DeviceString.Length;

    return PhCreateString2(&sr);
}

VOID NotifyForDevice(
    _In_ BOOLEAN Removed,
    _In_ PDEVICE_NODE Node
    )
{
    PPH_STRING classification;
    PPH_STRING name;
    PPH_STRING title;

    classification = NotifyDeviceGetString(Node, DevKeyClass, TRUE);
    name = NotifyDeviceGetString(Node, DevKeyName, FALSE);

    title = PhFormatString(
        L"%ls Device %ls",
        PhGetStringOrDefault(classification, L"Unclassified"),
        Removed ? L"Removed" : L"Arrived"
        );

    PhShowIconNotification(PhGetString(title), PhGetStringOrDefault(name, L"Unnamed"));

    PhDereferenceObject(title);

    if (name)
        PhDereferenceObject(name);

    if (classification)
        PhDereferenceObject(classification);
}

BOOLEAN ShouldNotifyForDevice(
    _In_ PDEVICE_NODE Node
    )
{
    if (Node->Children->Count > 0)
        return FALSE;

    if (IsEqualGUID(&Node->DeviceInfoData.ClassGuid, &GUID_DEVCLASS_HIDCLASS))
        return FALSE;

    return TRUE;
}

VOID DeviceNotify(
    _In_ PLIST_ENTRY List
    )
{
    DEVICE_TREE_OPTIONS options;
    PDEVICE_TREE deviceTree;

    //
    // Using the block of notification events, snap the current device tree. We do this so:
    // 1. Not every notification triggers a device tree creation
    // 2. We can filter to only leaf nodes for device notifications, the device changes must be  
    //    fully reflected in the tree for this to work - it's a bit of hack but works well enough
    //

    RtlZeroMemory(&options, sizeof(DEVICE_TREE_OPTIONS));
    options.IncludeSoftwareComponents = TRUE;

    deviceTree = CreateDeviceTree(&options);

    while (!IsListEmpty(List))
    {
        PDEVICE_NOTIFY_ENTRY entry = CONTAINING_RECORD(RemoveHeadList(List), DEVICE_NOTIFY_ENTRY, ListEntry);

        for (ULONG i = 0; i < deviceTree->DeviceList->Count; i++)
        {
            PDEVICE_NODE node = deviceTree->DeviceList->Items[i];

            if (!PhEqualString(node->InstanceId, entry->InstanceId, TRUE))
                continue;

            if (ShouldNotifyForDevice(node))
                NotifyForDevice(entry->Removed, node);

            break;
        }

        PhDereferenceObject(entry->InstanceId);
        PhFree(entry);
    }
}

_Function_class_(PUSER_THREAD_START_ROUTINE)
NTSTATUS NTAPI DeviceNotifyWorker(
    _In_ PVOID ThreadParameter
    )
{
    for (;;)
    {
        LIST_ENTRY list;

        // delay to process events in blocks
        PhDelayExecution(1000);

        PhAcquireFastLockExclusive(&DeviceNotifyLock);

        if (IsListEmpty(&DeviceNotifyList))
        {
            NtResetEvent(DeviceNotifyEvent, NULL);
            PhReleaseFastLockExclusive(&DeviceNotifyLock);
            NtWaitForSingleObject(DeviceNotifyEvent, FALSE, NULL);
            continue;
        }

        // drain the list
        list = DeviceNotifyList;
        list.Flink->Blink = &list;
        list.Blink->Flink = &list;
        InitializeListHead(&DeviceNotifyList);

        PhReleaseFastLockExclusive(&DeviceNotifyLock);

        DeviceNotify(&list);
    }

    return STATUS_SUCCESS;
}

VOID HandleDeviceNotify(
    _In_ BOOLEAN Removed,
    _In_z_ PWSTR InstanceId
    )
{
    PDEVICE_NOTIFY_ENTRY entry;

    if (!PhGetIntegerSetting(SETTING_NAME_DEVICE_ENABLE_NOTIFICATIONS))
        return;

    if (PhBeginInitOnce(&DeviceNotifyInitOnce))
    {
        InitializeListHead(&DeviceNotifyList);
        if (!NT_SUCCESS(NtCreateEvent(&DeviceNotifyEvent, EVENT_ALL_ACCESS, NULL, NotificationEvent, FALSE)) ||
            !NT_SUCCESS(PhCreateThread2(DeviceNotifyWorker, NULL)))
        {
            DeviceNotifyEvent = NULL;
        }
        PhEndInitOnce(&DeviceNotifyInitOnce);
    }

    if (!DeviceNotifyEvent)
        return;

    entry = PhAllocate(sizeof(DEVICE_NOTIFY_ENTRY));

    entry->Removed = Removed;
    entry->InstanceId = PhCreateString(InstanceId);

    PhAcquireFastLockExclusive(&DeviceNotifyLock);
    InsertTailList(&DeviceNotifyList, &entry->ListEntry);
    NtSetEvent(DeviceNotifyEvent, NULL);
    PhReleaseFastLockExclusive(&DeviceNotifyLock);
}
