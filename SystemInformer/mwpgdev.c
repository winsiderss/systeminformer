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

#include <phapp.h>
#include <devprv.h>
#include <settings.h>

#include <phplug.h>
#include <mainwnd.h>
#include <mainwndp.h>

#include <devguid.h>

static PH_CALLBACK_REGISTRATION PhpDeviceNotifyRegistration = { 0 };
static CONST PH_STRINGREF PhpDeviceString = PH_STRINGREF_INIT(L"Device");

BOOLEAN PhpShouldNotifyForDevice(
    _In_ PPH_DEVICE_ITEM Item,
    _In_ ULONG Type
    )
{
    if (Item->ChildrenCount)
        return FALSE;

    if (IsEqualGUID(&Item->ClassGuid, &GUID_DEVCLASS_HIDCLASS))
        return FALSE;

    if (PhPluginsEnabled && PhMwpPluginNotifyEvent(Type, Item))
        return FALSE;

    return TRUE;
}

PPH_STRING PhpNotifyDeviceGetString(
    _In_ PPH_DEVICE_ITEM Node,
    _In_ PH_DEVICE_PROPERTY_CLASS Class,
    _In_ BOOLEAN TrimDevice
    )
{
    PH_STRINGREF sr;
    PPH_STRING string;

    string = PhGetDeviceProperty(Node, Class)->AsString;
    if (PhIsNullOrEmptyString(string))
        return NULL;

    sr = string->sr;

    if (TrimDevice && PhEndsWithStringRef(&sr, &PhpDeviceString, TRUE))
        sr.Length -= PhpDeviceString.Length;

    return PhCreateString2(&sr);
}

VOID PhpNotifyForDevice(
    _In_ PPH_DEVICE_ITEM Item,
    _In_ ULONG Type
    )
{
    PPH_STRING classification;
    PPH_STRING name;
    PPH_STRING title;

    classification = PhpNotifyDeviceGetString(Item, PhDevicePropertyClass, TRUE);
    if (!classification)
        classification = PhCreateString(L"Unclassified");

    name = PhpNotifyDeviceGetString(Item, PhDevicePropertyName, FALSE);
    if (!name)
        name = PhCreateString(L"Unnamed");

    if (Type == PH_NOTIFY_DEVICE_REMOVED)
    {
        PhLogDeviceEntry(PH_LOG_ENTRY_DEVICE_REMOVED, classification, name);
        title = PhConcatStringRefZ(&classification->sr, L" Device Removed");
    }
    else
    {
        PhLogDeviceEntry(PH_LOG_ENTRY_DEVICE_ARRIVED, classification, name);
        title = PhConcatStringRefZ(&classification->sr, L" Device Arrived");
    }

    if ((PhMwpNotifyIconNotifyMask & Type))
        PhShowIconNotification(PhGetString(title), PhGetString(name));

    PhDereferenceObject(title);
    PhDereferenceObject(name);
    PhDereferenceObject(classification);
}

VOID NTAPI PhpDeviceProviderCallbackHandler(
    _In_ PVOID Parameter,
    _In_ PVOID Context
    )
{
    ULONG type = 0;
    PPH_DEVICE_NOTIFY notify = Parameter;
    PPH_DEVICE_TREE tree;
    PPH_DEVICE_ITEM item;

    if (notify->Action == PhDeviceNotifyInstanceStarted)
        type = PH_NOTIFY_DEVICE_ARRIVED;
    else if (notify->Action == PhDeviceNotifyInstanceRemoved)
        type = PH_NOTIFY_DEVICE_REMOVED;
    else
        return;

    if (tree = PhReferenceDeviceTree())
    {
        item = PhLookupDeviceItem(tree, &notify->DeviceInstance.InstanceId->sr);
        if (item && PhpShouldNotifyForDevice(item, type))
        {
            PhpNotifyForDevice(item, type);
        }

        PhClearReference(&tree);
    }
}

VOID PhMwpInitializeDeviceNotifications(
    VOID
    )
{
    if (!PhGetIntegerSetting(L"EnableDeviceNotifySupport")) // EnableDeviceNotificationSupport
        return;

    if (!PhDeviceProviderInitialization())
        return;

    PhRegisterCallback(
        PhGetGeneralCallback(GeneralCallbackDeviceNotificationEvent),
        PhpDeviceProviderCallbackHandler,
        NULL,
        &PhpDeviceNotifyRegistration
        );
}
