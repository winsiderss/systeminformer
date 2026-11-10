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
#include <hwprv.h>
#include <procprv.h>

// The history of every hardware sensor, appended once per provider run with the system histories.

// Throttle flags are stored as FLOAT values, which hold every integer below 2^24 exactly.
C_ASSERT(PH_HW_THROTTLE_AUTONOMOUS < (1UL << 24));

typedef struct _PHP_HW_HISTORY_ENTRY
{
    ULONG64 ComponentKey;
    ULONG64 SensorKey;
    PH_HW_SENSOR_GROUP Group;
    ULONG UnlistedRuns;
    BOOLEAN Listed;
    PH_HW_COMPONENT_KIND ComponentKind;
    ULONG ComponentIndex;
    ULONG SourceId;
    PH_HW_SENSOR_UNIT Unit;
    PPH_STRING ComponentName;
    PPH_STRING SensorName;
    PH_CIRCULAR_BUFFER_FLOAT Values; // most recent first; NaN without a reading
} PHP_HW_HISTORY_ENTRY, *PPHP_HW_HISTORY_ENTRY;

// One sensor of a run, copied out of the enumeration so the store lock is never held with its locks.
typedef struct _PHP_HW_HISTORY_SAMPLE
{
    ULONG64 ComponentKey;
    ULONG64 SensorKey;
    PH_HW_SENSOR_GROUP Group;
    PH_HW_COMPONENT_KIND ComponentKind;
    ULONG ComponentIndex;
    ULONG SourceId;
    PH_HW_SENSOR_UNIT Unit;
    FLOAT Value;
    PPH_STRING ComponentName; // named pass only
    PPH_STRING SensorName; // named pass only
} PHP_HW_HISTORY_SAMPLE, *PPHP_HW_HISTORY_SAMPLE;

static PH_QUEUED_LOCK PhpHwHistoryLock = PH_QUEUED_LOCK_INIT;
static PPH_HASHTABLE PhpHwHistoryTable = NULL; // PHP_HW_HISTORY_ENTRY, created on the first append
static PH_CIRCULAR_BUFFER_ULONG64 PhpHwHistoryTimes; // system time of each run, most recent first

_Function_class_(PH_HASHTABLE_EQUAL_FUNCTION)
static BOOLEAN NTAPI PhpHwHistoryEqual(
    _In_ PVOID Entry1,
    _In_ PVOID Entry2
    )
{
    PPHP_HW_HISTORY_ENTRY entry1 = Entry1;
    PPHP_HW_HISTORY_ENTRY entry2 = Entry2;

    return
        entry1->ComponentKey == entry2->ComponentKey &&
        entry1->SensorKey == entry2->SensorKey &&
        entry1->Group == entry2->Group;
}

_Function_class_(PH_HASHTABLE_HASH_FUNCTION)
static ULONG NTAPI PhpHwHistoryHash(
    _In_ PVOID Entry
    )
{
    PPHP_HW_HISTORY_ENTRY entry = Entry;

    return PhHashInt64(entry->ComponentKey ^ (entry->SensorKey * 0x9E3779B97F4A7C15ULL) ^ ((ULONG64)entry->Group << 56));
}

_Requires_lock_held_(PhpHwHistoryLock)
static PPHP_HW_HISTORY_ENTRY PhpHwHistoryFind(
    _In_ ULONG64 ComponentKey,
    _In_ PH_HW_SENSOR_GROUP Group,
    _In_ ULONG64 SensorKey
    )
{
    PHP_HW_HISTORY_ENTRY lookup;

    if (!PhpHwHistoryTable)
        return NULL;

    lookup.ComponentKey = ComponentKey;
    lookup.SensorKey = SensorKey;
    lookup.Group = Group;

    return PhFindEntryHashtable(PhpHwHistoryTable, &lookup);
}

_Function_class_(PH_HW_SENSOR_ENUM_CALLBACK)
static VOID NTAPI PhpHwHistoryCollect(
    _In_ PCPH_HW_SENSOR_ENTRY Entry,
    _In_opt_ PVOID Context
    )
{
    PPH_ARRAY samples = Context;
    PHP_HW_HISTORY_SAMPLE sample;

    if (!samples)
        return;

    sample.ComponentKey = Entry->ComponentKey;
    sample.SensorKey = Entry->SensorKey;
    sample.Group = Entry->Group;
    sample.ComponentKind = Entry->ComponentKind;
    sample.ComponentIndex = Entry->ComponentIndex;
    sample.SourceId = Entry->SourceId;
    sample.Unit = Entry->Unit;

    if (!Entry->Current)
        sample.Value = NAN;
    else if (Entry->Unit == PhHwUnitThrottle)
        sample.Value = (FLOAT)Entry->ThrottleFlags;
    else
        sample.Value = Entry->Value;

    sample.ComponentName = Entry->ComponentName ? PhReferenceObject(Entry->ComponentName) : NULL;
    sample.SensorName = Entry->SensorName ? PhReferenceObject(Entry->SensorName) : NULL;

    PhAddItemArray(samples, &sample);
}

static VOID PhpHwHistoryDeleteSamples(
    _Inout_ PPH_ARRAY Samples
    )
{
    for (SIZE_T i = 0; i < Samples->Count; i++)
    {
        PPHP_HW_HISTORY_SAMPLE sample = PhItemArray(Samples, i);

        PhClearReference(&sample->ComponentName);
        PhClearReference(&sample->SensorName);
    }

    PhDeleteArray(Samples);
}

// Names are read only when sensors are first stored, so a run does not format a name per sensor.
static VOID PhpHwHistoryUpdateNames(
    VOID
    )
{
    PH_ARRAY samples;

    PhInitializeArray(&samples, sizeof(PHP_HW_HISTORY_SAMPLE), 64);
    PhpHwEnumerateSensorsEx(PhpHwHistoryCollect, &samples, 0);

    PhAcquireQueuedLockExclusive(&PhpHwHistoryLock);

    for (SIZE_T i = 0; i < samples.Count; i++)
    {
        PPHP_HW_HISTORY_SAMPLE sample = PhItemArray(&samples, i);
        PPHP_HW_HISTORY_ENTRY entry;

        if (!(entry = PhpHwHistoryFind(sample->ComponentKey, sample->Group, sample->SensorKey)))
            continue;

        if (sample->ComponentName && (!entry->ComponentName || !PhEqualString(entry->ComponentName, sample->ComponentName, FALSE)))
            PhSwapReference(&entry->ComponentName, sample->ComponentName);
        if (sample->SensorName && (!entry->SensorName || !PhEqualString(entry->SensorName, sample->SensorName, FALSE)))
            PhSwapReference(&entry->SensorName, sample->SensorName);
    }

    PhReleaseQueuedLockExclusive(&PhpHwHistoryLock);

    PhpHwHistoryDeleteSamples(&samples);
}

/**
 * Appends the current value of every sensor to its history. Called by the provider with the
 * system histories, so sample i of every sensor is the same run.
 */
VOID PhpHwHistoryAppend(
    VOID
    )
{
    PH_ARRAY samples;
    PH_ARRAY expired;
    LARGE_INTEGER time;
    PPHP_HW_HISTORY_ENTRY entry;
    ULONG enumerationKey;
    BOOLEAN added = FALSE;

    PhInitializeArray(&samples, sizeof(PHP_HW_HISTORY_SAMPLE), 64);
    PhpHwEnumerateSensorsEx(PhpHwHistoryCollect, &samples, PHP_HW_ENUM_NO_NAMES);

    // Nothing is allocated until the first sensor; the provider thread is the only writer.
    if (!PhpHwHistoryTable && samples.Count == 0)
    {
        PhDeleteArray(&samples);
        return;
    }

    // The time the system histories just recorded for this run.
    if (!PhGetStatisticsTime(NULL, 0, &time))
        PhQuerySystemTime(&time);

    PhInitializeArray(&expired, sizeof(PHP_HW_HISTORY_ENTRY), 4);

    PhAcquireQueuedLockExclusive(&PhpHwHistoryLock);

    if (!PhpHwHistoryTable)
    {
        PhpHwHistoryTable = PhCreateHashtable(sizeof(PHP_HW_HISTORY_ENTRY), PhpHwHistoryEqual, PhpHwHistoryHash, 64);
        PhInitializeCircularBuffer_ULONG64(&PhpHwHistoryTimes, PhStatisticsSampleCount);
    }

    // Inserts first: an insert can move the entries, so no entry pointer is held across it.
    for (SIZE_T i = 0; i < samples.Count; i++)
    {
        PPHP_HW_HISTORY_SAMPLE sample = PhItemArray(&samples, i);

        if (!PhpHwHistoryFind(sample->ComponentKey, sample->Group, sample->SensorKey))
        {
            PHP_HW_HISTORY_ENTRY newEntry;

            memset(&newEntry, 0, sizeof(PHP_HW_HISTORY_ENTRY));
            newEntry.ComponentKey = sample->ComponentKey;
            newEntry.SensorKey = sample->SensorKey;
            newEntry.Group = sample->Group;
            PhInitializeCircularBuffer_FLOAT(&newEntry.Values, PhStatisticsSampleCount);

            PhAddEntryHashtable(PhpHwHistoryTable, &newEntry);
            added = TRUE;
        }
    }

    PhAddItemCircularBuffer_ULONG64(&PhpHwHistoryTimes, (ULONG64)time.QuadPart);

    enumerationKey = 0;

    while (PhEnumHashtable(PhpHwHistoryTable, &entry, &enumerationKey))
        entry->Listed = FALSE;

    for (SIZE_T i = 0; i < samples.Count; i++)
    {
        PPHP_HW_HISTORY_SAMPLE sample = PhItemArray(&samples, i);

        entry = PhpHwHistoryFind(sample->ComponentKey, sample->Group, sample->SensorKey);
        assert(entry);

        entry->Listed = TRUE;
        entry->UnlistedRuns = 0;
        entry->ComponentKind = sample->ComponentKind;
        entry->ComponentIndex = sample->ComponentIndex;
        entry->SourceId = sample->SourceId;
        entry->Unit = sample->Unit;
        PhAddItemCircularBuffer_FLOAT(&entry->Values, sample->Value);
    }

    // A sensor no longer listed records no reading, and is dropped once its history holds nothing else.
    enumerationKey = 0;

    while (PhEnumHashtable(PhpHwHistoryTable, &entry, &enumerationKey))
    {
        if (entry->Listed)
            continue;

        PhAddItemCircularBuffer_FLOAT(&entry->Values, NAN);

        // Copied, because the table may not be modified while it is enumerated.
        if (++entry->UnlistedRuns >= entry->Values.Size)
            PhAddItemArray(&expired, entry);
    }

    for (SIZE_T i = 0; i < expired.Count; i++)
    {
        PPHP_HW_HISTORY_ENTRY removed = PhItemArray(&expired, i);

        PhRemoveEntryHashtable(PhpHwHistoryTable, removed);
        PhDeleteCircularBuffer_FLOAT(&removed->Values);
        PhClearReference(&removed->ComponentName);
        PhClearReference(&removed->SensorName);
    }

    PhReleaseQueuedLockExclusive(&PhpHwHistoryLock);

    PhDeleteArray(&expired);
    PhpHwHistoryDeleteSamples(&samples);

    if (added)
        PhpHwHistoryUpdateNames();
}

/**
 * Copies the history of a sensor.
 *
 * \param ComponentKey The sensor's component key, from PH_HW_SENSOR_ENTRY.
 * \param Group The sensor's group.
 * \param SensorKey The sensor's key.
 * \param MaximumCount The number of elements in Values and Times.
 * \param Values Receives the values, most recent first; NaN where there was no reading.
 * \param Times Receives the system time of each value.
 * \param Count Receives the number of values copied.
 * \param Info Receives what the sensor is; the names are referenced.
 * \return TRUE when the sensor has history, otherwise FALSE.
 */
_Success_(return)
BOOLEAN NTAPI PhHwCopySensorHistory(
    _In_ ULONG64 ComponentKey,
    _In_ PH_HW_SENSOR_GROUP Group,
    _In_ ULONG64 SensorKey,
    _In_ ULONG MaximumCount,
    _Out_writes_to_(MaximumCount, *Count) PFLOAT Values,
    _Out_writes_to_opt_(MaximumCount, *Count) PLARGE_INTEGER Times,
    _Out_ PULONG Count,
    _Out_opt_ PPH_HW_SENSOR_HISTORY_INFO Info
    )
{
    PPHP_HW_HISTORY_ENTRY entry;
    ULONG count;

    PhAcquireQueuedLockShared(&PhpHwHistoryLock);

    if (!(entry = PhpHwHistoryFind(ComponentKey, Group, SensorKey)))
    {
        PhReleaseQueuedLockShared(&PhpHwHistoryLock);
        return FALSE;
    }

    count = min(MaximumCount, entry->Values.Count);

    for (ULONG i = 0; i < count; i++)
    {
        Values[i] = PhGetItemCircularBuffer_FLOAT(&entry->Values, i);

        // Every run appends one time and one value to every stored sensor, so the indexes match.
        if (Times)
            Times[i].QuadPart = (LONGLONG)PhGetItemCircularBuffer_ULONG64(&PhpHwHistoryTimes, i);
    }

    if (Info)
    {
        Info->ComponentKind = entry->ComponentKind;
        Info->ComponentIndex = entry->ComponentIndex;
        Info->SourceId = entry->SourceId;
        Info->Unit = entry->Unit;
        Info->ComponentName = entry->ComponentName ? PhReferenceObject(entry->ComponentName) : NULL;
        Info->SensorName = entry->SensorName ? PhReferenceObject(entry->SensorName) : NULL;
        Info->Listed = entry->Listed;
    }

    PhReleaseQueuedLockShared(&PhpHwHistoryLock);

    *Count = count;

    return TRUE;
}

/**
 * Enumerates the sensors that have history, listed or not.
 *
 * \param Callback Called once per sensor, under the history lock (shared).
 * \param Context Passed to the callback.
 */
VOID NTAPI PhHwEnumerateSensorHistories(
    _In_ PPH_HW_SENSOR_HISTORY_ENUM_CALLBACK Callback,
    _In_opt_ PVOID Context
    )
{
    PPHP_HW_HISTORY_ENTRY entry;
    ULONG enumerationKey = 0;

    PhAcquireQueuedLockShared(&PhpHwHistoryLock);

    if (PhpHwHistoryTable)
    {
        while (PhEnumHashtable(PhpHwHistoryTable, &entry, &enumerationKey))
            Callback(entry->ComponentKey, entry->Group, entry->SensorKey, entry->Listed, Context);
    }

    PhReleaseQueuedLockShared(&PhpHwHistoryLock);
}

/**
 * Gets the number of samples each sensor's history holds.
 *
 * \return The size of the history buffers, or the size they will have once the first sensor is stored.
 */
ULONG NTAPI PhHwGetSensorHistorySlots(
    VOID
    )
{
    ULONG slots;

    PhAcquireQueuedLockShared(&PhpHwHistoryLock);

    if (PhpHwHistoryTable)
        slots = PhpHwHistoryTimes.Size;
    else
        slots = PhRoundUpToPowerOfTwo(PhStatisticsSampleCount);

    PhReleaseQueuedLockShared(&PhpHwHistoryLock);

    return slots;
}

/**
 * Appends the size of the history to the hardware report.
 */
VOID PhpHwHistoryWriteDiagnostics(
    _Inout_ PPH_STRING_BUILDER Builder
    )
{
    PPHP_HW_HISTORY_ENTRY entry;
    ULONG enumerationKey = 0;
    ULONG sensors = 0;
    ULONG slots = 0;
    ULONG64 bytes = 0;

    PhAcquireQueuedLockShared(&PhpHwHistoryLock);

    if (PhpHwHistoryTable)
    {
        slots = PhpHwHistoryTimes.Size;
        bytes = (ULONG64)PhpHwHistoryTimes.Size * sizeof(ULONG64) + (ULONG64)PhpHwHistoryTable->AllocatedEntries * sizeof(PHP_HW_HISTORY_ENTRY);

        while (PhEnumHashtable(PhpHwHistoryTable, &entry, &enumerationKey))
        {
            sensors++;
            bytes += (ULONG64)entry->Values.Size * sizeof(FLOAT);
        }
    }

    PhReleaseQueuedLockShared(&PhpHwHistoryLock);

    PhAppendFormatStringBuilder(Builder, L"History: %lu sensors, %lu slots, %I64u bytes\n", sensors, slots, bytes);
}
