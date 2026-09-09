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
#include <cfgmgr32.h>

// The device tree the application already builds, which is the same set of nodes Device Manager
// shows: what is installed, what driver and service is behind it, and which nodes are reporting a
// problem code. get_device_resources then says what hardware a node was actually given.
//
// Every property here is read through PhGetDeviceProperty, which fills it on first use, so a node
// nobody has asked about is not any more expensive than one that has been.

VOID AtpAddDeviceProperty(
    _In_ PVOID Object,
    _In_ PCSTR Key,
    _In_ PPH_DEVICE_ITEM Item,
    _In_ PH_DEVICE_PROPERTY_CLASS Class
    )
{
    PPH_DEVICE_PROPERTY property;

    property = PhGetDeviceProperty(Item, Class);

    if (!property->Valid)
    {
        AtJsonAddNull(Object, Key);
        return;
    }

    // Every class formats itself into AsString; the typed union is only worth reaching into where
    // the caller needs to compare rather than read.
    AtJsonAddString(Object, Key, property->AsString);
}

// A date property carries a real timestamp under its formatted string; the string is whatever the
// user's locale renders, which is not what a caller wants to compare.
VOID AtpAddDevicePropertyTime(
    _In_ PVOID Object,
    _In_ PCSTR Key,
    _In_ PPH_DEVICE_ITEM Item,
    _In_ PH_DEVICE_PROPERTY_CLASS Class
    )
{
    PPH_DEVICE_PROPERTY property;

    property = PhGetDeviceProperty(Item, Class);

    if (!property->Valid || property->Type != PhDevicePropertyTypeTimeStamp)
    {
        AtJsonAddNull(Object, Key);
        return;
    }

    AtJsonAddTime(Object, Key, &property->TimeStamp);
}

VOID AtpAddDevicePropertyList(
    _In_ PVOID Object,
    _In_ PCSTR Key,
    _In_ PPH_DEVICE_ITEM Item,
    _In_ PH_DEVICE_PROPERTY_CLASS Class
    )
{
    PPH_DEVICE_PROPERTY property;
    PVOID array;
    ULONG i;

    property = PhGetDeviceProperty(Item, Class);

    if (!property->Valid || property->Type != PhDevicePropertyTypeStringList || !property->StringList)
    {
        AtJsonAddNull(Object, Key);
        return;
    }

    array = PhCreateJsonArray();

    for (i = 0; i < property->StringList->Count; i++)
    {
        PPH_STRING string = property->StringList->Items[i];
        PPH_BYTES utf8;

        if (utf8 = PhConvertUtf16ToUtf8Ex(string->Buffer, string->Length))
        {
            PhAddJsonArrayObject(array, PhCreateJsonStringObject(utf8->Buffer));
            PhDereferenceObject(utf8);
        }
    }

    PhAddJsonObjectValue(Object, Key, array);
}

BOOLEAN AtpDeviceMatches(
    _In_ PPH_DEVICE_ITEM Item,
    _In_opt_ PPH_STRING NameContains,
    _In_opt_ PPH_STRING DeviceClass,
    _In_opt_ PPH_STRING Service,
    _In_ BOOLEAN ProblemsOnly
    )
{
    if (ProblemsOnly && !FlagOn(Item->DevNodeStatus, DN_HAS_PROBLEM))
        return FALSE;

    if (DeviceClass)
    {
        PPH_DEVICE_PROPERTY property = PhGetDeviceProperty(Item, PhDevicePropertyClass);

        if (!property->Valid || !property->AsString ||
            !PhEqualString(property->AsString, DeviceClass, TRUE))
        {
            return FALSE;
        }
    }

    if (Service)
    {
        PPH_DEVICE_PROPERTY property = PhGetDeviceProperty(Item, PhDevicePropertyService);

        if (!property->Valid || !property->AsString ||
            !PhEqualString(property->AsString, Service, TRUE))
        {
            return FALSE;
        }
    }

    if (NameContains)
    {
        PPH_DEVICE_PROPERTY name = PhGetDeviceProperty(Item, PhDevicePropertyName);

        if (AtContainsString(name->Valid ? name->AsString : NULL, NameContains))
            return TRUE;

        return AtContainsString(Item->InstanceId, NameContains);
    }

    return TRUE;
}

VOID AtpListDevices(
    _In_ PAT_TOOL_CALL Call,
    _Inout_ PAT_TOOL_RESULT Result
    )
{
    PPH_DEVICE_TREE tree;
    PPH_STRING nameContains;
    PPH_STRING deviceClass;
    PPH_STRING service;
    BOOLEAN problemsOnly;
    BOOLEAN includeDriver;
    AT_ROWS rows;
    PVOID structured;
    ULONG i;

    nameContains = AtGetArgumentString(Call->Arguments, "name_contains");
    deviceClass = AtGetArgumentString(Call->Arguments, "device_class");
    service = AtGetArgumentString(Call->Arguments, "service");
    problemsOnly = AtJsonGetObjectBoolean(Call->Arguments, "problems_only");
    includeDriver = AtJsonGetObjectBoolean(Call->Arguments, "include_driver");

    if (!(tree = PhReferenceDeviceTree()))
    {
        AtSetToolError(Result, "failed", STATUS_UNSUCCESSFUL, L"The device tree is not available.");
        PhClearReference(&nameContains);
        PhClearReference(&deviceClass);
        PhClearReference(&service);
        return;
    }

    structured = PhCreateJsonObject();
    AtInitializeRows(&rows, Call->Arguments);

    for (i = 0; i < tree->DeviceList->Count; i++)
    {
        PPH_DEVICE_ITEM item = tree->DeviceList->Items[i];
        PVOID row;

        if (!AtpDeviceMatches(item, nameContains, deviceClass, service, problemsOnly))
            continue;

        row = PhCreateJsonObject();
        AtJsonAddString(row, "instance_id", item->InstanceId);
        AtJsonAddString(row, "parent_instance_id", item->ParentInstanceId);
        AtpAddDeviceProperty(row, "name", item, PhDevicePropertyName);
        AtpAddDeviceProperty(row, "description", item, PhDevicePropertyDeviceDesc);
        AtpAddDeviceProperty(row, "manufacturer", item, PhDevicePropertyManufacturer);
        AtpAddDeviceProperty(row, "device_class", item, PhDevicePropertyClass);
        AtpAddDeviceProperty(row, "enumerator", item, PhDevicePropertyEnumeratorName);
        AtpAddDeviceProperty(row, "service", item, PhDevicePropertyService);

        // The devnode status is what says a device is in trouble, not the problem code: a node the
        // property could not be read from is given CM_PROB_PHANTOM, so testing the code alone
        // reports a problem on every device that has merely gone away.
        PhAddJsonObjectBoolean(row, "has_problem", !!FlagOn(item->DevNodeStatus, DN_HAS_PROBLEM));
        PhAddJsonObjectUInt64(row, "problem_code", item->ProblemCode);
        AtJsonAddHex(row, "devnode_status", item->DevNodeStatus);
        PhAddJsonObjectUInt64(row, "children_count", item->ChildrenCount);
        PhAddJsonObjectUInt64(row, "interface_count", item->InterfaceCount);

        // Filter drivers sit above or below the function driver in the stack, which is where
        // something that wants to see every request to a device installs itself.
        PhAddJsonObjectBoolean(row, "has_upper_filters", !!item->HasUpperFilters);
        PhAddJsonObjectBoolean(row, "has_lower_filters", !!item->HasLowerFilters);

        if (includeDriver)
        {
            AtpAddDeviceProperty(row, "driver", item, PhDevicePropertyDriver);
            AtpAddDeviceProperty(row, "driver_version", item, PhDevicePropertyDriverVersion);
            AtpAddDevicePropertyTime(row, "driver_date", item, PhDevicePropertyDriverDate);
            AtpAddDeviceProperty(row, "location_info", item, PhDevicePropertyLocationInfo);
            AtpAddDevicePropertyList(row, "upper_filters", item, PhDevicePropertyUpperFilters);
            AtpAddDevicePropertyList(row, "lower_filters", item, PhDevicePropertyLowerFilters);
        }
        else
        {
            AtJsonAddNull(row, "driver");
            AtJsonAddNull(row, "driver_version");
            AtJsonAddNull(row, "driver_date");
            AtJsonAddNull(row, "location_info");
            AtJsonAddNull(row, "upper_filters");
            AtJsonAddNull(row, "lower_filters");
        }

        AtAddRow(&rows, row);
    }

    AtAddRows(structured, "devices", &rows);
    AtAddSnapshot(structured);

    Result->StructuredContent = structured;

    PhDereferenceObject(tree);
    AtDeleteRows(&rows);
    PhClearReference(&nameContains);
    PhClearReference(&deviceClass);
    PhClearReference(&service);
}

typedef struct _AT_DEVICE_RESOURCES
{
    PVOID Array;
    ULONG Count;
} AT_DEVICE_RESOURCES, *PAT_DEVICE_RESOURCES;

// Each resource kind carries its own descriptor, so the ranges are reported as numbers rather than
// rendered into a string: an agent comparing a device's memory window against an address needs the
// numbers, and the end of a range is not the same as its start.
_Function_class_(PH_DEVICE_ENUM_RESOURCES_CALLBACK)
BOOLEAN NTAPI AtpDeviceResourceCallback(
    _In_ ULONG LogicalConfig,
    _In_ ULONG ResourceId,
    _In_ PVOID Buffer,
    _In_ ULONG Length,
    _In_opt_ PVOID Context
    )
{
    PAT_DEVICE_RESOURCES resources = Context;
    PVOID entry;

    UNREFERENCED_PARAMETER(LogicalConfig);
    UNREFERENCED_PARAMETER(Length);

    if (!resources)
        return FALSE;

    entry = PhCreateJsonObject();

    switch (ResourceId)
    {
    case ResType_Mem:
        {
            PMEM_RESOURCE resource = Buffer;

            PhAddJsonObject(entry, "type", "memory");
            AtJsonAddHex(entry, "start", resource->MEM_Header.MD_Alloc_Base);
            AtJsonAddHex(entry, "end", resource->MEM_Header.MD_Alloc_End);
            PhAddJsonObjectUInt64(entry, "length", resource->MEM_Header.MD_Alloc_End - resource->MEM_Header.MD_Alloc_Base + 1);
        }
        break;
    case ResType_MemLarge:
        {
            PMEM_LARGE_RESOURCE resource = Buffer;

            PhAddJsonObject(entry, "type", "memory_large");
            AtJsonAddHex(entry, "start", resource->MEM_LARGE_Header.MLD_Alloc_Base);
            AtJsonAddHex(entry, "end", resource->MEM_LARGE_Header.MLD_Alloc_End);
            PhAddJsonObjectUInt64(entry, "length", resource->MEM_LARGE_Header.MLD_Alloc_End - resource->MEM_LARGE_Header.MLD_Alloc_Base + 1);
        }
        break;
    case ResType_IO:
        {
            PIO_RESOURCE resource = Buffer;

            PhAddJsonObject(entry, "type", "io_port");
            AtJsonAddHex(entry, "start", resource->IO_Header.IOD_Alloc_Base);
            AtJsonAddHex(entry, "end", resource->IO_Header.IOD_Alloc_End);
            PhAddJsonObjectUInt64(entry, "length", resource->IO_Header.IOD_Alloc_End - resource->IO_Header.IOD_Alloc_Base + 1);
        }
        break;
    case ResType_IRQ:
        {
            PIRQ_RESOURCE resource = Buffer;

            PhAddJsonObject(entry, "type", "irq");
            // Message-signalled interrupts are reported as negative numbers, the same way Device
            // Manager shows them; read unsigned they come out as four billion and change.
            PhAddJsonObjectInt64(entry, "number", (LONG)resource->IRQ_Header.IRQD_Alloc_Num);
            AtJsonAddHex(entry, "affinity", resource->IRQ_Header.IRQD_Affinity);
        }
        break;
    case ResType_DMA:
        {
            PDMA_RESOURCE resource = Buffer;

            PhAddJsonObject(entry, "type", "dma");
            PhAddJsonObjectUInt64(entry, "channel", resource->DMA_Header.DD_Alloc_Chan);
        }
        break;
    case ResType_BusNumber:
        {
            PBUSNUMBER_RESOURCE resource = Buffer;

            PhAddJsonObject(entry, "type", "bus_number");
            PhAddJsonObjectUInt64(entry, "start", resource->BusNumber_Header.BUSD_Alloc_Base);
            PhAddJsonObjectUInt64(entry, "end", resource->BusNumber_Header.BUSD_Alloc_End);
        }
        break;
    case ResType_ClassSpecific:
        {
            PCS_RESOURCE resource = Buffer;
            PPH_STRING guid = PhFormatGuid(&resource->CS_Header.CSD_ClassGuid);

            PhAddJsonObject(entry, "type", "class_specific");
            AtJsonAddString(entry, "class_guid", guid);
            PhClearReference(&guid);
        }
        break;
    case ResType_Connection:
        {
            PCONNECTION_RESOURCE resource = Buffer;

            PhAddJsonObject(entry, "type", "connection");
            PhAddJsonObjectUInt64(entry, "connection_class", resource->Connection_Header.COND_Class);
            PhAddJsonObjectUInt64(entry, "connection_type", resource->Connection_Header.COND_Type);
            PhAddJsonObjectUInt64(entry, "connection_id", resource->Connection_Header.COND_Id.QuadPart);
        }
        break;
    case ResType_DevicePrivate:
    case ResType_Reserved:
        {
            // Driver-internal data with no meaning outside the driver, and on a real machine there
            // is one of these between every pair of actual resources.
            PhFreeJsonObject(entry);
            return FALSE;
        }
    default:
        {
            PhAddJsonObject(entry, "type", "other");
            PhAddJsonObjectUInt64(entry, "resource_id", ResourceId);
        }
        break;
    }

    PhAddJsonArrayObject(resources->Array, entry);
    resources->Count++;

    return FALSE;
}

VOID AtpGetDeviceResources(
    _In_ PAT_TOOL_CALL Call,
    _Inout_ PAT_TOOL_RESULT Result
    )
{
    PPH_DEVICE_TREE tree;
    PPH_DEVICE_ITEM item;
    PPH_STRING instanceId;
    AT_DEVICE_RESOURCES resources;
    PVOID structured;

    if (!(instanceId = AtGetArgumentString(Call->Arguments, "instance_id")))
    {
        AtSetToolError(Result, "invalid_arguments", STATUS_INVALID_PARAMETER, L"instance_id is required.");
        return;
    }

    if (!(tree = PhReferenceDeviceTree()))
    {
        AtSetToolError(Result, "failed", STATUS_UNSUCCESSFUL, L"The device tree is not available.");
        PhDereferenceObject(instanceId);
        return;
    }

    if (!(item = PhLookupDeviceItem(tree, &instanceId->sr)))
    {
        AtSetToolError(Result, "not_found", STATUS_NOT_FOUND, L"No device has that instance id.");
        PhDereferenceObject(tree);
        PhDereferenceObject(instanceId);
        return;
    }

    structured = PhCreateJsonObject();
    AtJsonAddString(structured, "instance_id", item->InstanceId);
    AtpAddDeviceProperty(structured, "name", item, PhDevicePropertyName);
    PhAddJsonObjectUInt64(structured, "problem_code", item->ProblemCode);

    resources.Array = PhCreateJsonArray();
    resources.Count = 0;

    // The allocated configuration: what the device was actually given, not what it could accept.
    PhEnumDeviceResources(item, ALLOC_LOG_CONF, AtpDeviceResourceCallback, &resources);

    PhAddJsonObjectValue(structured, "resources", resources.Array);
    PhAddJsonObjectUInt64(structured, "count", resources.Count);
    AtAddSnapshot(structured);

    Result->StructuredContent = structured;

    PhDereferenceObject(tree);
    PhDereferenceObject(instanceId);
}

/**
 * Turning a device off, and back on. The whole of it is two configuration manager calls; the care
 * is in what is said about them.
 */
VOID AtpSetDeviceEnabled(
    _In_ PAT_TOOL_CALL Call,
    _In_ PAT_TARGET Target,
    _Inout_ PAT_TOOL_RESULT Result
    )
{
    CONFIGRET result;
    DEVINST deviceInstance;
    PPH_STRING instanceId;
    PVOID structured;
    ULONG status = 0;
    ULONG problem = 0;
    BOOLEAN enable;

    // The device comes from the resolved target rather than from the arguments again: the user was
    // asked about that device, and re-reading the argument here would act on whatever it says now.
    instanceId = PhReferenceObject(Target->DeviceInstanceId);
    enable = AtJsonGetObjectBoolean(Call->Arguments, "enabled");

    // PHANTOM finds a node that is not started, which is exactly the state a disabled device is
    // in - without it a device could be disabled and then not found to enable again.
    result = CM_Locate_DevNode(&deviceInstance, PhGetString(instanceId), CM_LOCATE_DEVNODE_PHANTOM);

    if (result != CR_SUCCESS)
    {
        AtSetToolError(Result, "not_found", PhDosErrorToNtStatus(CM_MapCrToWin32Err(result, ERROR_INVALID_HANDLE_STATE)),
            L"No device with that instance id; list_devices reports the ones there are.");
        PhClearReference(&instanceId);
        return;
    }

    if (enable)
        result = CM_Enable_DevInst(deviceInstance, 0);
    else
        result = CM_Disable_DevInst(deviceInstance, 0);

    if (result != CR_SUCCESS)
    {
        // Most of what the configuration manager refuses with has no Win32 equivalent, so mapping
        // it alone reports the fallback - "the handle is in an invalid state" - whatever went
        // wrong. The two that actually happen say opposite things about whether to try again, so
        // they are named rather than left to the caller to look up.
        AtSetToolError(
            Result,
            "failed",
            PhDosErrorToNtStatus(CM_MapCrToWin32Err(result, ERROR_INVALID_HANDLE_STATE)),
            L"%s failed (CONFIGRET %lu).%s",
            enable ? L"Enabling the device" : L"Disabling the device",
            (ULONG)result,
            result == CR_REMOVE_VETOED ?
                L" A driver in the device's stack refused to stop it, which usually means something "
                L"is using it. This disable is not asked to persist, so it has to stop the device "
                L"now and cannot defer to a restart the way Device Manager does; close whatever is "
                L"using the device and try again." :
            result == CR_NOT_DISABLEABLE ?
                L" The device reports that it cannot be disabled at all, so trying again will not "
                L"help - a device the system is running on says this." :
                L" list_devices shows what the node is and whether it has a parent worth asking "
                L"about instead."
            );
        PhClearReference(&instanceId);
        return;
    }

    structured = PhCreateJsonObject();
    AtJsonAddString(structured, "instance_id", instanceId);
    PhAddJsonObject(structured, "action", "set_device_enabled");
    PhAddJsonObjectBoolean(structured, "enabled", enable);

    // Read back from the device node rather than assumed from the call: a device can refuse to
    // stop because something is using it, and the problem code is where that shows.
    if (CM_Get_DevNode_Status(&status, &problem, deviceInstance, 0) == CR_SUCCESS)
    {
        PhAddJsonObjectBoolean(structured, "has_problem", !!(status & DN_HAS_PROBLEM));
        PhAddJsonObjectUInt64(structured, "problem_code", problem);
        PhAddJsonObjectBoolean(structured, "is_disabled", problem == CM_PROB_DISABLED);
    }
    else
    {
        AtJsonAddNull(structured, "has_problem");
        AtJsonAddNull(structured, "problem_code");
        AtJsonAddNull(structured, "is_disabled");
    }

    AtAddSnapshot(structured);
    Result->StructuredContent = structured;

    PhClearReference(&instanceId);
}

VOID AtDeviceInvokeTool(
    _In_ PCAT_TOOL Tool,
    _In_ PAT_TOOL_CALL Call,
    _Inout_ PAT_TARGET Target,
    _Inout_ PAT_TOOL_RESULT Result
    )
{
    UNREFERENCED_PARAMETER(Target);

    switch (Tool->Action)
    {
    case AtActionListDevices:
        AtpListDevices(Call, Result);
        break;
    case AtActionGetDeviceResources:
        AtpGetDeviceResources(Call, Result);
        break;
    case AtActionSetDeviceEnabled:
        AtpSetDeviceEnabled(Call, Target, Result);
        break;
    default:
        AtSetToolError(Result, "failed", STATUS_NOT_IMPLEMENTED, L"This tool is not implemented.");
        break;
    }
}
