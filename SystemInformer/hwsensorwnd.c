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

#include <phapp.h>
#include <phplug.h>
#include <phsettings.h>
#include <settings.h>
#include <colmgr.h>
#include <cpysave.h>
#include <emenu.h>
#include <hwprv.h>
#include <procprv.h>
#include <sysinfo.h>
#include <sysinfop.h>

#define WM_PH_HW_SENSORS_UPDATE (WM_APP + 1)
#define PHP_HW_COLLAPSE_PROCESSOR_COUNT 16

typedef enum _PHP_HW_SENSORS_COLUMN
{
    PhpHwSensorsColumnName,
    PhpHwSensorsColumnValue,
    PhpHwSensorsColumnMinimum,
    PhpHwSensorsColumnMaximum,
    PhpHwSensorsColumnHistory,
    PhpHwSensorsColumnMaximumId
} PHP_HW_SENSORS_COLUMN;

typedef enum _PHP_HW_NODE_TYPE
{
    PhpHwNodeComponent,
    PhpHwNodeGroup,
    PhpHwNodeSensor
} PHP_HW_NODE_TYPE;

typedef struct _PHP_HW_NODE
{
    PH_TREENEW_NODE Node;
    PHP_HW_NODE_TYPE Type;
    ULONG64 Key; // order among siblings
    PPH_STRING Name;
    PPH_LIST Children; // component: groups with sensors; group: all sensors ordered by key

    union
    {
        struct
        {
            struct _PHP_HW_NODE *Groups[PhHwGroupMaximum];
            PH_HW_COMPONENT_KIND Kind;
        } Component;
        struct
        {
            struct _PHP_HW_NODE *Group;
            PH_HW_SENSOR_UNIT Unit;
            ULONG Capability; // core sensors: removed when the capability is lost
            BOOLEAN History;
            BOOLEAN ScalesToCpuLimit;
            BOOLEAN Listed; // source and named sensors: removed when no longer listed by the provider
            BOOLEAN Current;
            BOOLEAN RangeValid;
            FLOAT Value;
            FLOAT Minimum;
            FLOAT Maximum;
            ULONG ThrottleFlags;
            PPH_STRING ValueText;
            PPH_STRING MinimumText;
            PPH_STRING MaximumText;
            PH_CIRCULAR_BUFFER_FLOAT HistoryBuffer; // valid when History
            PH_GRAPH_BUFFERS GraphBuffers;
        } Sensor;
    };
} PHP_HW_NODE, *PPHP_HW_NODE;

typedef struct _PHP_HW_SENSORS_CONTEXT
{
    HWND WindowHandle;
    HWND TreeNewHandle;
    HWND SearchWindowHandle;
    ULONG_PTR SearchMatchHandle;
    PCPH_STRINGREF EmptyText;
    PH_LAYOUT_MANAGER LayoutManager;
    PH_CALLBACK_REGISTRATION ProcessesUpdatedRegistration;
    PPH_LIST ComponentList; // all components ordered by key
    PPH_LIST RootList; // components with sensors
    PPH_LIST SensorList; // all sensors, unordered
    ULONG ProcessorCount;
    BOOLEAN StructureChanged;

    HDC GraphContext;
    HBITMAP GraphBitmap;
    HBITMAP GraphOldBitmap;
    PVOID GraphBits;
    LONG GraphWidth;
    LONG GraphHeight;
} PHP_HW_SENSORS_CONTEXT, *PPHP_HW_SENSORS_CONTEXT;

static HWND PhpHwSensorsWindowHandle = NULL;

INT_PTR CALLBACK PhpHwSensorsDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

/**
 * Determines whether the hardware sensors window can show sensors: the provider gate is open or a
 * sensor source reports sensors.
 */
BOOLEAN PhHwSensorsWindowAvailable(
    VOID
    )
{
    return PhHwSensorsEnabled() || PhHwSourceSensorsPresent() || PhHwCounterSensorsPresent();
}

VOID PhShowHardwareSensorsWindow(
    _In_ HWND ParentWindowHandle
    )
{
    if (!PhpHwSensorsWindowHandle)
    {
        PhpHwSensorsWindowHandle = PhCreateDialog(
            NtCurrentImageBase(),
            MAKEINTRESOURCE(IDD_HWSENSORS),
            PhCsForceNoParent ? NULL : ParentWindowHandle,
            PhpHwSensorsDlgProc,
            NULL
            );
        PhRegisterDialog(PhpHwSensorsWindowHandle);
        ShowWindow(PhpHwSensorsWindowHandle, SW_SHOW);
    }

    if (IsMinimized(PhpHwSensorsWindowHandle))
        ShowWindow(PhpHwSensorsWindowHandle, SW_RESTORE);
    else
        SetForegroundWindow(PhpHwSensorsWindowHandle);
}

_Function_class_(PH_CALLBACK_FUNCTION)
static VOID NTAPI PhpHwSensorsProcessesUpdatedHandler(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    PPHP_HW_SENSORS_CONTEXT context = Context;

    if (context)
        PostMessage(context->WindowHandle, WM_PH_HW_SENSORS_UPDATE, 0, 0);
}

static PPHP_HW_NODE PhpHwCreateNode(
    _In_ PHP_HW_NODE_TYPE Type,
    _In_ ULONG64 Key,
    _In_ _Assume_refs_(1) PPH_STRING Name
    )
{
    PPHP_HW_NODE node;

    node = PhAllocateZero(sizeof(PHP_HW_NODE));
    PhInitializeTreeNewNode(&node->Node);
    node->Type = Type;
    node->Key = Key;
    node->Name = Name;

    if (Type != PhpHwNodeSensor)
        node->Children = PhCreateList(8);

    return node;
}

static VOID PhpHwDestroyNode(
    _In_ _Post_invalid_ PPHP_HW_NODE Node
    )
{
    if (Node->Type == PhpHwNodeSensor)
    {
        PhClearReference(&Node->Sensor.ValueText);
        PhClearReference(&Node->Sensor.MinimumText);
        PhClearReference(&Node->Sensor.MaximumText);

        if (Node->Sensor.History)
            PhDeleteCircularBuffer_FLOAT(&Node->Sensor.HistoryBuffer);

        PhDeleteGraphBuffers(&Node->Sensor.GraphBuffers);
    }

    if (Node->Children)
        PhDereferenceObject(Node->Children);

    PhClearReference(&Node->Name);
    PhFree(Node);
}

// Position receives the node's position, or the insertion point when not found.
static PPHP_HW_NODE PhpHwFindNode(
    _In_ PPH_LIST List,
    _In_ ULONG64 Key,
    _Out_ PULONG Position
    )
{
    ULONG low = 0;
    ULONG high = List->Count;

    while (low < high)
    {
        ULONG middle = low + (high - low) / 2;
        PPHP_HW_NODE node = List->Items[middle];

        if (node->Key == Key)
        {
            *Position = middle;
            return node;
        }

        if (node->Key < Key)
            low = middle + 1;
        else
            high = middle;
    }

    *Position = low;
    return NULL;
}

// Ordered by kind, source registration, then index.
static PPHP_HW_NODE PhpHwGetComponent(
    _In_ PPHP_HW_SENSORS_CONTEXT Context,
    _In_ PCPH_HW_SENSOR_ENTRY Entry
    )
{
    PPHP_HW_NODE component;
    ULONG position;

    component = PhpHwFindNode(Context->ComponentList, Entry->ComponentKey, &position);

    if (component)
        return component;

    component = PhpHwCreateNode(PhpHwNodeComponent, Entry->ComponentKey, PhReferenceObject(Entry->ComponentName));
    component->Component.Kind = Entry->ComponentKind;
    PhInsertItemList(Context->ComponentList, position, component);

    return component;
}

static PPHP_HW_NODE PhpHwGetGroup(
    _In_ PPHP_HW_SENSORS_CONTEXT Context,
    _In_ PPHP_HW_NODE Component,
    _In_ PH_HW_SENSOR_GROUP Group
    )
{
    PPHP_HW_NODE group;

    group = Component->Component.Groups[Group];

    if (group)
        return group;

    group = PhpHwCreateNode(PhpHwNodeGroup, Group, PhCreateString(PhHwGetSensorGroupName(Group)));
    group->Node.Expanded = !(
        PhHwIsSensorGroupPerProcessor(Group) &&
        Component->Component.Kind == PhHwComponentCpu &&
        Context->ProcessorCount > PHP_HW_COLLAPSE_PROCESSOR_COUNT
        );
    Component->Component.Groups[Group] = group;

    return group;
}

static PPHP_HW_NODE PhpHwInsertSensor(
    _In_ PPHP_HW_SENSORS_CONTEXT Context,
    _In_ PPHP_HW_NODE Group,
    _In_ ULONG Position,
    _In_ PCPH_HW_SENSOR_ENTRY Entry
    )
{
    PPHP_HW_NODE sensor;

    sensor = PhpHwCreateNode(PhpHwNodeSensor, Entry->SensorKey, PhReferenceObject(Entry->SensorName));
    sensor->Sensor.Group = Group;
    sensor->Sensor.Unit = Entry->Unit;
    sensor->Sensor.Capability = Entry->Capability;
    sensor->Sensor.History = Entry->History;
    sensor->Sensor.ScalesToCpuLimit = PhHwSensorScalesToCpuLimit(Entry);

    if (sensor->Sensor.History)
        PhInitializeCircularBuffer_FLOAT(&sensor->Sensor.HistoryBuffer, PhStatisticsSampleCount);

    PhInitializeGraphBuffers(&sensor->Sensor.GraphBuffers);

    PhInsertItemList(Group->Children, Position, sensor);
    PhAddItemList(Context->SensorList, sensor);
    Context->StructureChanged = TRUE;

    return sensor;
}

static VOID PhpHwSensorUpdateValue(
    _Inout_ PPHP_HW_NODE Sensor,
    _In_ FLOAT Value
    )
{
    Sensor->Sensor.Current = TRUE;
    Sensor->Sensor.Value = Value;

    if (!Sensor->Sensor.RangeValid)
    {
        Sensor->Sensor.Minimum = Value;
        Sensor->Sensor.Maximum = Value;
        Sensor->Sensor.RangeValid = TRUE;
    }
    else
    {
        if (Value < Sensor->Sensor.Minimum)
            Sensor->Sensor.Minimum = Value;
        if (Value > Sensor->Sensor.Maximum)
            Sensor->Sensor.Maximum = Value;
    }
}

// A source sensor node appears on its first listing, any other sensor node on its first current value.
_Function_class_(PH_HW_SENSOR_ENUM_CALLBACK)
static VOID NTAPI PhpHwCollectSensor(
    _In_ PCPH_HW_SENSOR_ENTRY Entry,
    _In_opt_ PVOID Context
    )
{
    PPHP_HW_SENSORS_CONTEXT context = Context;
    PPHP_HW_NODE component;
    PPHP_HW_NODE group;
    PPHP_HW_NODE sensor;
    ULONG position;

    if (!context || Entry->Group >= PhHwGroupMaximum)
        return;

    if (Entry->Current || Entry->ComponentKind == PhHwComponentSource)
    {
        component = PhpHwGetComponent(context, Entry);
        group = PhpHwGetGroup(context, component, Entry->Group);
    }
    else
    {
        if (!(component = PhpHwFindNode(context->ComponentList, Entry->ComponentKey, &position)))
            return;
        if (!(group = component->Component.Groups[Entry->Group]))
            return;
    }

    sensor = PhpHwFindNode(group->Children, Entry->SensorKey, &position);

    if (!sensor)
    {
        if (!Entry->Current && Entry->ComponentKind != PhHwComponentSource)
            return;

        sensor = PhpHwInsertSensor(context, group, position, Entry);
    }

    sensor->Sensor.Listed = TRUE;

    if (!Entry->Current)
        return;

    if (Entry->Unit == PhHwUnitThrottle)
    {
        sensor->Sensor.Current = TRUE;
        sensor->Sensor.ThrottleFlags = Entry->ThrottleFlags;
    }
    else
    {
        PhpHwSensorUpdateValue(sensor, Entry->Value);
    }
}

static VOID PhpHwSensorUpdateText(
    _In_ PPHP_HW_NODE Sensor
    )
{
    if (Sensor->Sensor.Unit == PhHwUnitThrottle)
    {
        if (Sensor->Sensor.Current)
            PhMoveReference(&Sensor->Sensor.ValueText, PhHwFormatThrottleFlags(Sensor->Sensor.ThrottleFlags));
        else
            PhClearReference(&Sensor->Sensor.ValueText);

        return;
    }

    if (Sensor->Sensor.Current)
        PhMoveReference(&Sensor->Sensor.ValueText, PhHwFormatSensorValue(Sensor->Sensor.Unit, Sensor->Sensor.Value));
    else
        PhClearReference(&Sensor->Sensor.ValueText);

    // The gear is a mode, not a measurement; it has no range.
    if (Sensor->Sensor.RangeValid && Sensor->Sensor.Unit != PhHwUnitGear)
    {
        PhMoveReference(&Sensor->Sensor.MinimumText, PhHwFormatSensorValue(Sensor->Sensor.Unit, Sensor->Sensor.Minimum));
        PhMoveReference(&Sensor->Sensor.MaximumText, PhHwFormatSensorValue(Sensor->Sensor.Unit, Sensor->Sensor.Maximum));
    }
    else
    {
        PhClearReference(&Sensor->Sensor.MinimumText);
        PhClearReference(&Sensor->Sensor.MaximumText);
    }
}

// A source that unregisters leaves empty groups and components behind.
static VOID PhpHwSensorsRemoveEmptySourceComponents(
    _In_ PPHP_HW_SENSORS_CONTEXT Context
    )
{
    for (ULONG i = 0; i < Context->ComponentList->Count; )
    {
        PPHP_HW_NODE component = Context->ComponentList->Items[i];
        BOOLEAN empty = TRUE;

        if (component->Component.Kind != PhHwComponentSource)
        {
            i++;
            continue;
        }

        for (ULONG group = 0; group < PhHwGroupMaximum; group++)
        {
            if (!component->Component.Groups[group])
                continue;

            if (component->Component.Groups[group]->Children->Count)
            {
                empty = FALSE;
                continue;
            }

            PhpHwDestroyNode(component->Component.Groups[group]);
            component->Component.Groups[group] = NULL;
        }

        if (empty)
        {
            PhRemoveItemList(Context->ComponentList, i);
            PhpHwDestroyNode(component);
            continue;
        }

        i++;
    }
}

static BOOLEAN PhpHwSensorsMatch(
    _In_ PPHP_HW_SENSORS_CONTEXT Context,
    _In_ PPHP_HW_NODE Node
    )
{
    return Node->Name && PhSearchControlMatch(Context->SearchMatchHandle, &Node->Name->sr);
}

// A matching component or group shows all its sensors; a matching sensor shows its ancestors.
static VOID PhpHwSensorsApplySearch(
    _In_ PPHP_HW_SENSORS_CONTEXT Context
    )
{
    for (ULONG i = 0; i < Context->RootList->Count; i++)
    {
        PPHP_HW_NODE component = Context->RootList->Items[i];
        BOOLEAN componentMatch = !Context->SearchMatchHandle || PhpHwSensorsMatch(Context, component);

        component->Node.Visible = componentMatch;

        for (ULONG j = 0; j < component->Children->Count; j++)
        {
            PPHP_HW_NODE group = component->Children->Items[j];
            BOOLEAN groupMatch = componentMatch || PhpHwSensorsMatch(Context, group);

            group->Node.Visible = groupMatch;

            for (ULONG k = 0; k < group->Children->Count; k++)
            {
                PPHP_HW_NODE sensor = group->Children->Items[k];

                sensor->Node.Visible = groupMatch || PhpHwSensorsMatch(Context, sensor);

                if (sensor->Node.Visible)
                {
                    group->Node.Visible = TRUE;
                    component->Node.Visible = TRUE;
                }
            }
        }
    }
}

static VOID PhpHwSensorsUpdateEmptyText(
    _In_ PPHP_HW_SENSORS_CONTEXT Context
    )
{
    static CONST PH_STRINGREF noMatch = PH_STRINGREF_INIT(L"No sensors match the search.");
    static CONST PH_STRINGREF unavailable = PH_STRINGREF_INIT(L"Hardware sensors are unavailable.");
    static CONST PH_STRINGREF unsupported = PH_STRINGREF_INIT(L"No supported hardware sensors were found. Use Save... to create a hardware report and attach it to a new issue to request support.");
    static CONST PH_STRINGREF waiting = PH_STRINGREF_INIT(L"Waiting for sensor readings...");
    PCPH_STRINGREF text;

    if (Context->RootList->Count)
        text = &noMatch;
    else if (!PhHwSensorsEnabled())
        text = &unavailable;
    else if (PhHwSensorsProbed() && !PhHwSensorsAvailable() && !PhHwSourceSensorsPresent())
        text = &unsupported;
    else
        text = &waiting;

    if (text == Context->EmptyText)
        return;

    Context->EmptyText = text;
    TreeNew_SetEmptyText(Context->TreeNewHandle, text, 0);
}

static VOID PhpHwSensorsRestructure(
    _In_ PPHP_HW_SENSORS_CONTEXT Context
    )
{
    PhpHwSensorsRemoveEmptySourceComponents(Context);
    PhClearList(Context->RootList);

    for (ULONG i = 0; i < Context->ComponentList->Count; i++)
    {
        PPHP_HW_NODE component = Context->ComponentList->Items[i];

        PhClearList(component->Children);

        for (ULONG group = 0; group < PhHwGroupMaximum; group++)
        {
            if (component->Component.Groups[group] && component->Component.Groups[group]->Children->Count)
                PhAddItemList(component->Children, component->Component.Groups[group]);
        }

        if (component->Children->Count)
            PhAddItemList(Context->RootList, component);
    }

    PhpHwSensorsApplySearch(Context);
    TreeNew_NodesStructured(Context->TreeNewHandle);
}

// Sensors appear on their first current value and show an empty value while stale.
static VOID PhpHwSensorsUpdate(
    _In_ PPHP_HW_SENSORS_CONTEXT Context
    )
{
    ULONG capabilities;

    for (ULONG i = 0; i < Context->SensorList->Count; i++)
    {
        PPHP_HW_NODE sensor = Context->SensorList->Items[i];

        sensor->Sensor.Current = FALSE;
        sensor->Sensor.Listed = FALSE;
    }

    PhHwEnumerateSensors(PhpHwCollectSensor, Context);
    capabilities = PhHwGetCapabilities();

    for (ULONG i = 0; i < Context->SensorList->Count; )
    {
        PPHP_HW_NODE sensor = Context->SensorList->Items[i];
        BOOLEAN available;

        if (!sensor->Sensor.Capability)
            available = sensor->Sensor.Listed;
        else
            available = !!(capabilities & sensor->Sensor.Capability);

        if (!available)
        {
            PPH_LIST siblings = sensor->Sensor.Group->Children;
            ULONG position = PhFindItemList(siblings, sensor);

            if (position != ULONG_MAX)
                PhRemoveItemList(siblings, position);

            PhRemoveItemList(Context->SensorList, i);
            PhpHwDestroyNode(sensor);
            Context->StructureChanged = TRUE;
            continue;
        }

        if (sensor->Sensor.History)
        {
            PhAddItemCircularBuffer_FLOAT(&sensor->Sensor.HistoryBuffer, sensor->Sensor.Current ? sensor->Sensor.Value : 0.0f);
            sensor->Sensor.GraphBuffers.Valid = FALSE;
        }

        PhpHwSensorUpdateText(sensor);
        i++;
    }

    if (Context->StructureChanged)
    {
        Context->StructureChanged = FALSE;
        PhpHwSensorsRestructure(Context);
    }
    else
    {
        InvalidateRect(Context->TreeNewHandle, NULL, FALSE);
    }

    PhpHwSensorsUpdateEmptyText(Context);
}

static VOID PhpHwSensorsResetRange(
    _In_ PPHP_HW_SENSORS_CONTEXT Context
    )
{
    for (ULONG i = 0; i < Context->SensorList->Count; i++)
    {
        PPHP_HW_NODE sensor = Context->SensorList->Items[i];

        sensor->Sensor.RangeValid = sensor->Sensor.Current;
        sensor->Sensor.Minimum = sensor->Sensor.Value;
        sensor->Sensor.Maximum = sensor->Sensor.Value;

        PhpHwSensorUpdateText(sensor);
    }

    InvalidateRect(Context->TreeNewHandle, NULL, FALSE);
}

static VOID PhpHwSensorsInvalidateGraphs(
    _In_ PPHP_HW_SENSORS_CONTEXT Context
    )
{
    for (ULONG i = 0; i < Context->SensorList->Count; i++)
    {
        PPHP_HW_NODE sensor = Context->SensorList->Items[i];

        sensor->Sensor.GraphBuffers.Valid = FALSE;
    }
}

static VOID PhpHwSensorsDeleteGraphContext(
    _In_ PPHP_HW_SENSORS_CONTEXT Context
    )
{
    if (Context->GraphContext)
    {
        if (Context->GraphOldBitmap)
        {
            SelectBitmap(Context->GraphContext, Context->GraphOldBitmap);
            Context->GraphOldBitmap = NULL;
        }

        if (Context->GraphBitmap)
        {
            DeleteBitmap(Context->GraphBitmap);
            Context->GraphBitmap = NULL;
        }

        DeleteDC(Context->GraphContext);
        Context->GraphContext = NULL;
    }

    Context->GraphBits = NULL;
    Context->GraphWidth = 0;
    Context->GraphHeight = 0;
}

static VOID PhpHwSensorsNeedGraphContext(
    _In_ PPHP_HW_SENSORS_CONTEXT Context,
    _In_ HDC hdc,
    _In_ LONG Width,
    _In_ LONG Height
    )
{
    if (Context->GraphContext && Context->GraphWidth == Width && Context->GraphHeight == Height)
        return;

    PhpHwSensorsDeleteGraphContext(Context);

    Context->GraphContext = CreateCompatibleDC(hdc);

    if (!Context->GraphContext)
        return;

    Context->GraphBitmap = PhCreateDIBSection(hdc, PHBF_DIB, Width, Height, &Context->GraphBits);

    if (!Context->GraphBitmap)
    {
        DeleteDC(Context->GraphContext);
        Context->GraphContext = NULL;
        Context->GraphBits = NULL;
        return;
    }

    Context->GraphOldBitmap = SelectBitmap(Context->GraphContext, Context->GraphBitmap);
    Context->GraphWidth = Width;
    Context->GraphHeight = Height;
}

static COLORREF PhpHwSensorGraphColor(
    _In_ PH_HW_SENSOR_UNIT Unit
    )
{
    switch (Unit)
    {
    case PhHwUnitTemperature:
        return PhCsColorTemperature;
    case PhHwUnitClock:
        return PhCsColorClock;
    case PhHwUnitMultiplier:
        return PhCsColorMultiplier;
    case PhHwUnitVoltage:
        return PhCsColorVoltage;
    case PhHwUnitCurrent:
        return PhCsColorCurrent;
    case PhHwUnitFan:
        return PhCsColorFanRpm;
    case PhHwUnitBandwidth:
        return PhCsColorBandwidth;
    case PhHwUnitPercent:
        return PhCsColorPercentage;
    }

    return PhCsColorPowerUsage;
}

// The smallest full scale of a graph scaled to its maximum, so that a near-zero history stays near the bottom.
static FLOAT PhpHwSensorGraphMinimumScale(
    _In_ PH_HW_SENSOR_UNIT Unit
    )
{
    switch (Unit)
    {
    case PhHwUnitClock:
        return 100.0f;
    case PhHwUnitMultiplier:
    case PhHwUnitVoltage:
    case PhHwUnitCurrent:
        return 1.0f;
    }

    return 10.0f;
}

static VOID PhpHwSensorsDrawHistory(
    _In_ PPHP_HW_SENSORS_CONTEXT Context,
    _In_ PPH_TREENEW_CUSTOM_DRAW CustomDraw
    )
{
    PPHP_HW_NODE node = (PPHP_HW_NODE)CustomDraw->Node;
    RECT rect = CustomDraw->CellRect;
    PH_GRAPH_DRAW_INFO drawInfo;
    COLORREF color;

    if (node->Type != PhpHwNodeSensor)
        return;
    if (!node->Sensor.History || !node->Sensor.HistoryBuffer.Count)
        return;
    if (rect.right - rect.left <= 1 || rect.bottom - rect.top <= 1)
        return;

    color = PhpHwSensorGraphColor(node->Sensor.Unit);

    memset(&drawInfo, 0, sizeof(PH_GRAPH_DRAW_INFO));
    drawInfo.Width = rect.right - rect.left - 1; // leave a small gap
    drawInfo.Height = rect.bottom - rect.top - 1; // leave a small gap
    drawInfo.Step = 2;
    drawInfo.BackColor = RGB(0x00, 0x00, 0x00);
    drawInfo.LineColor1 = color;
    drawInfo.LineBackColor1 = PhHalveColorBrightness(color);

    PhGetDrawInfoGraphBuffers(&node->Sensor.GraphBuffers, &drawInfo, node->Sensor.HistoryBuffer.Count);

    if (!node->Sensor.GraphBuffers.Valid)
    {
        FLOAT scale;

        PhCopyCircularBuffer_FLOAT(&node->Sensor.HistoryBuffer, node->Sensor.GraphBuffers.Data1, drawInfo.LineDataCount);

        if (node->Sensor.ScalesToCpuLimit)
        {
            scale = PhHwGetCpuTemperatureLimit();
        }
        else if (node->Sensor.Unit == PhHwUnitTemperature)
        {
            // A source, chipset, zone or DIMM temperature is not bounded by the CPU TjMax.
            scale = max(PhMaxMemorySingles(node->Sensor.GraphBuffers.Data1, drawInfo.LineDataCount), 100.0f);
        }
        else if (node->Sensor.Unit == PhHwUnitPercent)
        {
            scale = 100.0f;
        }
        else
        {
            scale = max(PhMaxMemorySingles(node->Sensor.GraphBuffers.Data1, drawInfo.LineDataCount), PhpHwSensorGraphMinimumScale(node->Sensor.Unit));
        }

        PhDivideSinglesBySingle(node->Sensor.GraphBuffers.Data1, scale, drawInfo.LineDataCount);
        node->Sensor.GraphBuffers.Valid = TRUE;
    }

    PhpHwSensorsNeedGraphContext(Context, CustomDraw->Dc, drawInfo.Width, drawInfo.Height);

    if (Context->GraphBits)
    {
        PhDrawGraphDirect(Context->GraphContext, Context->GraphBits, &drawInfo);
        BitBlt(
            CustomDraw->Dc,
            rect.left,
            rect.top + 1, // + 1 for small gap
            drawInfo.Width,
            drawInfo.Height,
            Context->GraphContext,
            0,
            0,
            SRCCOPY
            );
    }
}

static VOID PhpHwSensorsCopy(
    _In_ PPHP_HW_SENSORS_CONTEXT Context
    )
{
    PPH_STRING text;

    text = PhGetTreeNewText(Context->TreeNewHandle, 0);
    PhSetClipboardString(Context->TreeNewHandle, &text->sr);
    PhDereferenceObject(text);
}

// Every sensor, regardless of the search and collapsed nodes.
static PPH_STRING PhpHwSensorsGetText(
    _In_ PPHP_HW_SENSORS_CONTEXT Context
    )
{
    PH_STRING_BUILDER builder;

    PhInitializeStringBuilder(&builder, 0x1000);

    for (ULONG i = 0; i < Context->RootList->Count; i++)
    {
        PPHP_HW_NODE component = Context->RootList->Items[i];

        PhAppendFormatStringBuilder(&builder, L"%s\n", PhGetStringOrEmpty(component->Name));

        for (ULONG j = 0; j < component->Children->Count; j++)
        {
            PPHP_HW_NODE group = component->Children->Items[j];

            PhAppendFormatStringBuilder(&builder, L"  %s\n", PhGetStringOrEmpty(group->Name));

            for (ULONG k = 0; k < group->Children->Count; k++)
            {
                PPHP_HW_NODE sensor = group->Children->Items[k];

                PhAppendFormatStringBuilder(
                    &builder,
                    L"    %s: %s (min %s, max %s)\n",
                    PhGetStringOrEmpty(sensor->Name),
                    PhGetStringOrEmpty(sensor->Sensor.ValueText),
                    PhGetStringOrEmpty(sensor->Sensor.MinimumText),
                    PhGetStringOrEmpty(sensor->Sensor.MaximumText)
                    );
            }
        }
    }

    return PhFinalStringBuilderString(&builder);
}

static VOID PhpHwSensorsSave(
    _In_ PPHP_HW_SENSORS_CONTEXT Context
    )
{
    static PH_FILETYPE_FILTER filters[] =
    {
        { L"Text files (*.txt)", L"*.txt" },
        { L"All files (*.*)", L"*.*" }
    };
    PVOID fileDialog;

    fileDialog = PhCreateSaveFileDialog();
    PhSetFileDialogFilter(fileDialog, filters, RTL_NUMBER_OF(filters));
    PhSetFileDialogFileName(fileDialog, L"Hardware.txt");

    if (PhShowFileDialog(Context->WindowHandle, fileDialog))
    {
        NTSTATUS status;
        PPH_STRING fileName;
        PPH_FILE_STREAM fileStream;
        PPH_STRING sensors;
        PPH_STRING report;

        fileName = PH_AUTO(PhGetFileDialogFileName(fileDialog));
        sensors = PhpHwSensorsGetText(Context);
        report = PhHwCreateReport(&sensors->sr);

        if (NT_SUCCESS(status = PhCreateFileStream(
            &fileStream,
            fileName->Buffer,
            FILE_GENERIC_WRITE,
            FILE_SHARE_READ,
            FILE_OVERWRITE_IF,
            0
            )))
        {
            PhWriteStringAsUtf8FileStream(fileStream, (PPH_STRINGREF)&PhUnicodeByteOrderMark);
            PhWriteStringAsUtf8FileStream(fileStream, &report->sr);
            PhDereferenceObject(fileStream);
        }

        if (!NT_SUCCESS(status))
            PhShowStatus(Context->WindowHandle, L"Unable to create the file", status, 0);

        PhDereferenceObject(report);
        PhDereferenceObject(sensors);
    }

    PhFreeFileDialog(fileDialog);
}

_Function_class_(PH_SEARCHCONTROL_CALLBACK)
static VOID NTAPI PhpHwSensorsSearchControlCallback(
    _In_ ULONG_PTR MatchHandle,
    _In_opt_ PVOID Context
    )
{
    PPHP_HW_SENSORS_CONTEXT context = Context;

    if (!context)
        return;

    context->SearchMatchHandle = MatchHandle;
    PhpHwSensorsApplySearch(context);
    TreeNew_NodesStructured(context->TreeNewHandle);
}

static VOID PhpHwSensorsShowContextMenu(
    _In_ PPHP_HW_SENSORS_CONTEXT Context,
    _In_ PPH_TREENEW_CONTEXT_MENU ContextMenu
    )
{
    PPH_EMENU menu;
    PPH_EMENU_ITEM item;

    menu = PhCreateEMenu();
    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, IDC_RESET, L"&Reset min/max", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(menu, PhCreateEMenuSeparator(), ULONG_MAX);
    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, IDC_COPY, L"&Copy", NULL, NULL), ULONG_MAX);
    PhInsertCopyCellEMenuItem(menu, IDC_COPY, Context->TreeNewHandle, ContextMenu->Column);

    item = PhShowEMenu(
        menu,
        Context->WindowHandle,
        PH_EMENU_SHOW_LEFTRIGHT,
        PH_ALIGN_LEFT | PH_ALIGN_TOP,
        ContextMenu->Location.x,
        ContextMenu->Location.y
        );

    if (item && !PhHandleCopyCellEMenuItem(item))
    {
        switch (item->Id)
        {
        case IDC_RESET:
            PhpHwSensorsResetRange(Context);
            break;
        case IDC_COPY:
            PhpHwSensorsCopy(Context);
            break;
        }
    }

    PhDestroyEMenu(menu);
}

_Function_class_(PH_TREENEW_CALLBACK)
static BOOLEAN NTAPI PhpHwSensorsTreeNewCallback(
    _In_ HWND WindowHandle,
    _In_ PH_TREENEW_MESSAGE Message,
    _In_ PVOID Parameter1,
    _In_ PVOID Parameter2,
    _In_ PVOID Context
    )
{
    PPHP_HW_SENSORS_CONTEXT context = Context;
    PPHP_HW_NODE node;

    switch (Message)
    {
    case TreeNewGetChildren:
        {
            PPH_TREENEW_GET_CHILDREN getChildren = Parameter1;
            node = (PPHP_HW_NODE)getChildren->Node;

            if (!node)
            {
                getChildren->Children = (PPH_TREENEW_NODE *)context->RootList->Items;
                getChildren->NumberOfChildren = context->RootList->Count;
            }
            else if (node->Children)
            {
                getChildren->Children = (PPH_TREENEW_NODE *)node->Children->Items;
                getChildren->NumberOfChildren = node->Children->Count;
            }
        }
        return TRUE;
    case TreeNewIsLeaf:
        {
            PPH_TREENEW_IS_LEAF isLeaf = Parameter1;
            node = (PPHP_HW_NODE)isLeaf->Node;

            isLeaf->IsLeaf = node->Type == PhpHwNodeSensor;
        }
        return TRUE;
    case TreeNewGetCellText:
        {
            PPH_TREENEW_GET_CELL_TEXT getCellText = Parameter1;
            node = (PPHP_HW_NODE)getCellText->Node;

            switch (getCellText->Id)
            {
            case PhpHwSensorsColumnName:
                getCellText->Text = PhGetStringRef(node->Name);
                break;
            case PhpHwSensorsColumnValue:
                if (node->Type == PhpHwNodeSensor)
                    getCellText->Text = PhGetStringRef(node->Sensor.ValueText);
                break;
            case PhpHwSensorsColumnMinimum:
                if (node->Type == PhpHwNodeSensor)
                    getCellText->Text = PhGetStringRef(node->Sensor.MinimumText);
                break;
            case PhpHwSensorsColumnMaximum:
                if (node->Type == PhpHwNodeSensor)
                    getCellText->Text = PhGetStringRef(node->Sensor.MaximumText);
                break;
            default:
                return FALSE;
            }
        }
        return TRUE;
    case TreeNewGetNodeColor:
        {
            PPH_TREENEW_GET_NODE_COLOR getNodeColor = Parameter1;

            getNodeColor->Flags = TN_CACHE | TN_AUTO_FORECOLOR;
        }
        return TRUE;
    case TreeNewCustomDraw:
        {
            PPH_TREENEW_CUSTOM_DRAW customDraw = Parameter1;

            if (customDraw->Column->Id == PhpHwSensorsColumnHistory)
                PhpHwSensorsDrawHistory(context, customDraw);
        }
        return TRUE;
    case TreeNewColumnResized:
        {
            PPH_TREENEW_COLUMN column = Parameter1;

            if (column->Id == PhpHwSensorsColumnHistory)
                PhpHwSensorsInvalidateGraphs(context);
        }
        return TRUE;
    case TreeNewKeyDown:
        {
            PPH_TREENEW_KEY_EVENT keyEvent = Parameter1;

            switch (keyEvent->VirtualKey)
            {
            case 'C':
                if (GetKeyState(VK_CONTROL) < 0)
                    PhpHwSensorsCopy(context);
                break;
            case 'A':
                if (GetKeyState(VK_CONTROL) < 0)
                    TreeNew_SelectRange(WindowHandle, 0, -1);
                break;
            }
        }
        return TRUE;
    case TreeNewContextMenu:
        {
            PhpHwSensorsShowContextMenu(context, Parameter1);
        }
        return TRUE;
    case TreeNewHeaderRightClick:
        {
            PH_TN_COLUMN_MENU_DATA data;

            data.TreeNewHandle = WindowHandle;
            data.MouseEvent = Parameter1;
            data.DefaultSortColumn = 0;
            data.DefaultSortOrder = NoSortOrder;
            PhInitializeTreeNewColumnMenu(&data);

            data.Selection = PhShowEMenu(data.Menu, WindowHandle, PH_EMENU_SHOW_LEFTRIGHT,
                PH_ALIGN_LEFT | PH_ALIGN_TOP, data.MouseEvent->ScreenLocation.x, data.MouseEvent->ScreenLocation.y);
            PhHandleTreeNewColumnMenu(&data);
            PhDeleteTreeNewColumnMenu(&data);
        }
        return TRUE;
    }

    return FALSE;
}

static VOID PhpHwSensorsInitializeTree(
    _In_ PPHP_HW_SENSORS_CONTEXT Context
    )
{
    PPH_STRING settings;

    PhSetControlTheme(Context->TreeNewHandle, L"explorer");

    TreeNew_SetCallback(Context->TreeNewHandle, PhpHwSensorsTreeNewCallback, Context);
    TreeNew_SetExtendedFlags(Context->TreeNewHandle, TN_FLAG_ITEM_DRAG_SELECT, TN_FLAG_ITEM_DRAG_SELECT);

    TreeNew_SetRedraw(Context->TreeNewHandle, FALSE);

    PhAddTreeNewColumn(Context->TreeNewHandle, PhpHwSensorsColumnName, TRUE, L"Sensor", 280, PH_ALIGN_LEFT, 0, 0);
    PhAddTreeNewColumn(Context->TreeNewHandle, PhpHwSensorsColumnValue, TRUE, L"Value", 90, PH_ALIGN_RIGHT, 1, DT_RIGHT);
    PhAddTreeNewColumn(Context->TreeNewHandle, PhpHwSensorsColumnMinimum, TRUE, L"Min", 90, PH_ALIGN_RIGHT, 2, DT_RIGHT);
    PhAddTreeNewColumn(Context->TreeNewHandle, PhpHwSensorsColumnMaximum, TRUE, L"Max", 90, PH_ALIGN_RIGHT, 3, DT_RIGHT);
    PhAddTreeNewColumnEx2(Context->TreeNewHandle, PhpHwSensorsColumnHistory, TRUE, L"History", 140, PH_ALIGN_LEFT, 4, 0, TN_COLUMN_FLAG_CUSTOMDRAW);

    TreeNew_SetRedraw(Context->TreeNewHandle, TRUE);

    settings = PhGetStringSetting(SETTING_HW_SENSORS_TREE_LIST_COLUMNS);
    PhCmLoadSettings(Context->TreeNewHandle, &settings->sr);
    PhDereferenceObject(settings);
}

static VOID PhpHwSensorsDeleteTree(
    _In_ PPHP_HW_SENSORS_CONTEXT Context
    )
{
    PPH_STRING settings;

    settings = PhCmSaveSettings(Context->TreeNewHandle);
    PhSetStringSetting2(SETTING_HW_SENSORS_TREE_LIST_COLUMNS, &settings->sr);
    PhDereferenceObject(settings);

    for (ULONG i = 0; i < Context->SensorList->Count; i++)
        PhpHwDestroyNode(Context->SensorList->Items[i]);

    for (ULONG i = 0; i < Context->ComponentList->Count; i++)
    {
        PPHP_HW_NODE component = Context->ComponentList->Items[i];

        for (ULONG group = 0; group < PhHwGroupMaximum; group++)
        {
            if (component->Component.Groups[group])
                PhpHwDestroyNode(component->Component.Groups[group]);
        }

        PhpHwDestroyNode(component);
    }

    PhDereferenceObject(Context->SensorList);
    PhDereferenceObject(Context->RootList);
    PhDereferenceObject(Context->ComponentList);

    PhpHwSensorsDeleteGraphContext(Context);
}

INT_PTR CALLBACK PhpHwSensorsDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PPHP_HW_SENSORS_CONTEXT context;

    if (uMsg == WM_INITDIALOG)
    {
        context = PhAllocateZero(sizeof(PHP_HW_SENSORS_CONTEXT));
        context->WindowHandle = hwndDlg;

        PhSetWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT, context);
    }
    else
    {
        context = PhGetWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT);
    }

    if (!context)
        return FALSE;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            context->TreeNewHandle = GetDlgItem(hwndDlg, IDC_TREELIST);
            context->SearchWindowHandle = GetDlgItem(hwndDlg, IDC_FILTER);
            context->ComponentList = PhCreateList(2);
            context->RootList = PhCreateList(2);
            context->SensorList = PhCreateList(64);
            context->ProcessorCount = PhSystemProcessorInformation.NumberOfProcessors;

            PhSetApplicationWindowIcon(hwndDlg);

            PhCreateSearchControl2(
                hwndDlg,
                context->SearchWindowHandle,
                L"Search Hardware Sensors",
                SETTING_SEARCH_HW_SENSORS_REGEX,
                SETTING_SEARCH_HW_SENSORS_CASE_SENSITIVE,
                PhpHwSensorsSearchControlCallback,
                context
                );

            PhpHwSensorsInitializeTree(context);

            PhInitializeLayoutManager(&context->LayoutManager, hwndDlg);
            PhAddLayoutItem(&context->LayoutManager, context->SearchWindowHandle, NULL, PH_ANCHOR_LEFT | PH_ANCHOR_TOP | PH_ANCHOR_RIGHT);
            PhAddLayoutItem(&context->LayoutManager, context->TreeNewHandle, NULL, PH_ANCHOR_ALL);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDC_RESET), NULL, PH_ANCHOR_LEFT | PH_ANCHOR_BOTTOM);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDC_SAVE), NULL, PH_ANCHOR_LEFT | PH_ANCHOR_BOTTOM);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDOK), NULL, PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM);

            PhpHwSensorsUpdate(context);

            PhRegisterCallback(
                PhGetGeneralCallback(GeneralCallbackProcessProviderUpdatedEvent),
                PhpHwSensorsProcessesUpdatedHandler,
                context,
                &context->ProcessesUpdatedRegistration
                );

            if (PhValidWindowPlacementFromSetting(SETTING_HW_SENSORS_WINDOW_POSITION))
                PhLoadWindowPlacementFromSetting(SETTING_HW_SENSORS_WINDOW_POSITION, SETTING_HW_SENSORS_WINDOW_SIZE, hwndDlg);
            else
                PhCenterWindow(hwndDlg, GetParent(hwndDlg));

            PhSetDialogFocus(hwndDlg, context->TreeNewHandle);

            PhInitializeWindowTheme(hwndDlg, PhEnableThemeSupport);
        }
        break;
    case WM_DESTROY:
        {
            PhUnregisterCallback(
                PhGetGeneralCallback(GeneralCallbackProcessProviderUpdatedEvent),
                &context->ProcessesUpdatedRegistration
                );

            PhSaveWindowPlacementToSetting(SETTING_HW_SENSORS_WINDOW_POSITION, SETTING_HW_SENSORS_WINDOW_SIZE, hwndDlg);

            PhUnregisterDialog(PhpHwSensorsWindowHandle);
            PhpHwSensorsWindowHandle = NULL;

            PhRemoveWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT);

            PhpHwSensorsDeleteTree(context);
            PhDeleteLayoutManager(&context->LayoutManager);
            PhFree(context);
        }
        break;
    case WM_COMMAND:
        {
            switch (GET_WM_COMMAND_ID(wParam, lParam))
            {
            case IDCANCEL:
            case IDOK:
                DestroyWindow(hwndDlg);
                break;
            case IDC_RESET:
                PhpHwSensorsResetRange(context);
                break;
            case IDC_SAVE:
                PhpHwSensorsSave(context);
                break;
            }
        }
        break;
    case WM_PH_HW_SENSORS_UPDATE:
        {
            PhpHwSensorsUpdate(context);
        }
        break;
    case WM_DPICHANGED:
        {
            PhLayoutManagerUpdate(&context->LayoutManager, LOWORD(wParam));
            PhLayoutManagerLayout(&context->LayoutManager);
        }
        break;
    case WM_SIZE:
        {
            PhLayoutManagerLayout(&context->LayoutManager);
        }
        break;
    case WM_CTLCOLORBTN:
        return HANDLE_WM_CTLCOLORBTN(hwndDlg, wParam, lParam, PhWindowThemeControlColor);
    case WM_CTLCOLORDLG:
        return HANDLE_WM_CTLCOLORDLG(hwndDlg, wParam, lParam, PhWindowThemeControlColor);
    case WM_CTLCOLORSTATIC:
        return HANDLE_WM_CTLCOLORSTATIC(hwndDlg, wParam, lParam, PhWindowThemeControlColor);
    }

    return FALSE;
}
