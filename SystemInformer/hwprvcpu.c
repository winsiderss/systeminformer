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

#include <hwprvp.h>
#include <kphhw.h>
#include <cpusensor.h>
#include <sistatus.h>
#include <trace.h>

// Collector cpu: the processors' model-specific registers through KphHwCpuRead, one plan of registers per processor
// class, read once per update in calls of at most PHP_HW_CPU_BATCH items.

#define PHP_HW_CPU_BATCH 512 // items a call; halved for the session at the script's limits (D8)
#define PHP_HW_CPU_STRIKE_LIMIT 3 // failures of a register before it leaves the plan for the session

typedef enum _PHP_HW_CPU_CLASS
{
    PhpHwCpuNone,
    PhpHwCpuIntel,
    PhpHwCpuAmdZen, // AuthenticAMD or HygonGenuine, family 17h and later
    PhpHwCpuAmdLegacy, // AuthenticAMD, families 10h to 16h
    PhpHwCpuCentaur // CentaurHauls or "  Shanghai  "
} PHP_HW_CPU_CLASS;

typedef enum _PHP_HW_REG
{
    PhpHwRegAperf,
    PhpHwRegMperf,
    PhpHwRegMaximum
} PHP_HW_REG;
C_ASSERT(PhpHwRegMaximum <= 32);

typedef enum _PHP_HW_SCOPE
{
    PhpHwScopeThread,
    PhpHwScopeCore, // the first processor of each core
    PhpHwScopePackage // the first processor of each package
} PHP_HW_SCOPE;

typedef struct _PHP_HW_REGISTER
{
    ULONG Register; // 0 when not in the plan
    PHP_HW_SCOPE Scope;
    BOOLEAN Disabled;
    UCHAR Strikes;
} PHP_HW_REGISTER, *PPHP_HW_REGISTER;

typedef struct _PHP_HW_SAMPLE
{
    ULONG Valid;
    ULONG LastValid;
    ULONG64 Value[PhpHwRegMaximum];
    ULONG64 LastValue[PhpHwRegMaximum];
} PHP_HW_SAMPLE, *PPHP_HW_SAMPLE;

typedef struct _PHP_HW_CPU_TAG
{
    PHP_HW_REG Reg;
    ULONG Processor; // the item's processor; its core and package come from the sensor
} PHP_HW_CPU_TAG, *PPHP_HW_CPU_TAG;

static PHP_HW_CPU_CLASS PhpHwCpuClass = PhpHwCpuNone;
static PH_CPU_IDENTITY PhpHwCpuIdentity;
static ULONG PhpHwCpuSignature = 0;
static PHP_HW_REGISTER PhpHwCpuRegisters[PhpHwRegMaximum];
static ULONG PhpHwCpuBatch = PHP_HW_CPU_BATCH;
static ULONG PhpHwCpuCapacity = 0; // items of the whole plan
static PKPH_HW_CPU_ITEM PhpHwCpuItems = NULL;
static PKPH_HW_VALUE64 PhpHwCpuValues = NULL;
static PPHP_HW_CPU_TAG PhpHwCpuTags = NULL;
static PPHP_HW_SAMPLE PhpHwCpuSamples = NULL; // per processor
static PBOOLEAN PhpHwCpuFirstOfCore = NULL; // per processor
static PBOOLEAN PhpHwCpuFirstOfPackage = NULL; // per processor
static ULONG PhpHwCpuItemCount = 0; // items of the last pass
static ULONG64 PhpHwCpuSampleTime = 0;
static ULONG64 PhpHwCpuLastSampleTime = 0;

static VOID PhpHwCpuSetRegister(
    _In_ PHP_HW_REG Reg,
    _In_ ULONG Register,
    _In_ PHP_HW_SCOPE Scope
    )
{
    PhpHwCpuRegisters[Reg].Register = Register;
    PhpHwCpuRegisters[Reg].Scope = Scope;
    PhpHwCpuRegisters[Reg].Disabled = FALSE;
    PhpHwCpuRegisters[Reg].Strikes = 0;
}

static BOOLEAN PhpHwCpuRegisterActive(
    _In_ PHP_HW_REG Reg
    )
{
    return PhpHwCpuRegisters[Reg].Register && !PhpHwCpuRegisters[Reg].Disabled;
}

/**
 * Builds the plan of registers of the processor class.
 */
static VOID PhpHwCpuInitializePlan(
    VOID
    )
{
    memset(PhpHwCpuRegisters, 0, sizeof(PhpHwCpuRegisters));

    if (PhpHwCpuClass == PhpHwCpuAmdZen)
    {
        PhpHwCpuSetRegister(PhpHwRegAperf, PH_AMD_MSR_APERF_READONLY, PhpHwScopeThread);
        PhpHwCpuSetRegister(PhpHwRegMperf, PH_AMD_MSR_MPERF_READONLY, PhpHwScopeThread);
    }
}

/**
 * Determines whether a register is read on a processor: every processor for thread scope, the first of its core
 * or package otherwise.
 */
static BOOLEAN PhpHwCpuRegisterOnProcessor(
    _In_ PHP_HW_REG Reg,
    _In_ ULONG Processor
    )
{
    switch (PhpHwCpuRegisters[Reg].Scope)
    {
    case PhpHwScopeCore:
        return PhpHwCpuFirstOfCore[Processor];
    case PhpHwScopePackage:
        return PhpHwCpuFirstOfPackage[Processor];
    }

    return TRUE;
}

/**
 * Determines the processor class from the identity the driver gave.
 */
static PHP_HW_CPU_CLASS PhpHwCpuClassify(
    _In_ PCPH_CPU_IDENTITY Identity
    )
{
    if (PhCpuIdentityIsIntel(Identity))
        return PhpHwCpuIntel;
    if (PhCpuIdentityIsAmd(Identity) && Identity->DisplayFamily >= 0x17)
        return PhpHwCpuAmdZen;
    if (PhCpuIdentityIsAmd(Identity) && Identity->DisplayFamily >= 0x10)
        return PhpHwCpuAmdLegacy;
    if (PhCpuIdentityIsCentaur(Identity))
        return PhpHwCpuCentaur;

    return PhpHwCpuNone;
}

/**
 * Applies the session failure policy to an item: a register the script does not permit, or one that faults, leaves
 * the plan at once; an inactive processor, contention and "nothing this time" are skipped; anything else strikes.
 *
 * \return TRUE when the value is usable.
 */
static BOOLEAN PhpHwCpuApplyStatus(
    _In_ PHP_HW_REG Reg,
    _In_ NTSTATUS Status
    )
{
    PPHP_HW_REGISTER reg = &PhpHwCpuRegisters[Reg];

    if (NT_SUCCESS(Status))
    {
        reg->Strikes = 0;
        return TRUE;
    }

    switch (Status)
    {
    case STATUS_INVALID_PARAMETER: // an inactive processor
    case STATUS_RETRY:
    case STATUS_DEVICE_NOT_READY:
        break;
    case STATUS_NOT_FOUND: // not permitted
    case STATUS_NOT_SUPPORTED: // a faulting register
        reg->Disabled = TRUE;
        PhTraceInfo("Hardware sensor register 0x%lx unavailable: 0x%lx", reg->Register, Status);
        break;
    default:
        if (++reg->Strikes >= PHP_HW_CPU_STRIKE_LIMIT)
        {
            reg->Disabled = TRUE;
            PhTraceInfo("Hardware sensor register 0x%lx unavailable: 0x%lx", reg->Register, Status);
        }
        break;
    }

    return FALSE;
}

/**
 * Identifies the processor of the first package and builds the plan. Called once a session.
 *
 * \return STATUS_SUCCESS, STATUS_NOT_SUPPORTED for a machine or processor the collector does not serve, or the
 * status of the identify call.
 */
NTSTATUS PhpHwCpuProbe(
    VOID
    )
{
    PPH_HW_CPU_SENSOR first;
    KPH_HW_CPU_IDENTITY identity;
    NTSTATUS status;
    ULONG lastCore = ULONG_MAX;
    ULONG lastPackage = ULONG_MAX;

    if (PhpHwQueryMachine() == IMAGE_FILE_MACHINE_ARM64)
        return STATUS_NOT_SUPPORTED;
    if (!(first = PhHwGetCpuSensor(0)))
        return STATUS_NOT_SUPPORTED;

    status = KphHwCpuIdentify(first->ProcessorNumber.Group, first->ProcessorNumber.Number, &identity);

    if (!NT_SUCCESS(status))
        return status;

    memset(&PhpHwCpuIdentity, 0, sizeof(PH_CPU_IDENTITY));
    memcpy(PhpHwCpuIdentity.Vendor, identity.Vendor, sizeof(identity.Vendor));
    PhpHwCpuIdentity.DisplayFamily = (USHORT)identity.Family;
    PhpHwCpuIdentity.DisplayModel = (UCHAR)identity.Model;
    PhpHwCpuIdentity.Stepping = (UCHAR)identity.Stepping;
    PhpHwCpuIdentity.Hybrid = !!identity.Hybrid;
    PhpHwCpuSignature = identity.Signature;

    if ((PhpHwCpuClass = PhpHwCpuClassify(&PhpHwCpuIdentity)) == PhpHwCpuNone)
        return STATUS_NOT_SUPPORTED;

    PhpHwCpuInitializePlan();

    PhpHwCpuSamples = PhAllocateZero(sizeof(PHP_HW_SAMPLE) * PhpHwProcessorCount);
    PhpHwCpuFirstOfCore = PhAllocateZero(sizeof(BOOLEAN) * PhpHwProcessorCount);
    PhpHwCpuFirstOfPackage = PhAllocateZero(sizeof(BOOLEAN) * PhpHwProcessorCount);

    // processors of a core or a package are listed together
    for (ULONG i = 0; i < PhpHwProcessorCount; i++)
    {
        PhpHwCpuFirstOfCore[i] = PhpHwCpuSensors[i].CoreIndex != lastCore;
        PhpHwCpuFirstOfPackage[i] = PhpHwCpuSensors[i].PackageIndex != lastPackage;
        lastCore = PhpHwCpuSensors[i].CoreIndex;
        lastPackage = PhpHwCpuSensors[i].PackageIndex;
    }

    PhpHwCpuCapacity = 0;

    for (ULONG i = 0; i < PhpHwProcessorCount; i++)
    {
        for (ULONG reg = 0; reg < PhpHwRegMaximum; reg++)
        {
            if (PhpHwCpuRegisters[reg].Register && PhpHwCpuRegisterOnProcessor(reg, i))
                PhpHwCpuCapacity++;
        }
    }

    PhpHwCpuItems = PhAllocate(sizeof(KPH_HW_CPU_ITEM) * max(PhpHwCpuCapacity, 1));
    PhpHwCpuValues = PhAllocate(sizeof(KPH_HW_VALUE64) * max(PhpHwCpuCapacity, 1));
    PhpHwCpuTags = PhAllocate(sizeof(PHP_HW_CPU_TAG) * max(PhpHwCpuCapacity, 1));
    PhpHwCpuBatch = PHP_HW_CPU_BATCH;
    PhpHwCpuSampleTime = 0;
    PhpHwCpuLastSampleTime = 0;

    return STATUS_SUCCESS;
}

/**
 * Reads the items from Start in calls of at most PhpHwCpuBatch items, cut between processors. A call that meets the
 * script's limits halves the cut for the session and the rest is read again, once.
 *
 * \return The status of the last call, STATUS_RETRY when every call was told to retry.
 */
static NTSTATUS PhpHwCpuReadItems(
    VOID
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    BOOLEAN retried = FALSE;
    BOOLEAN answered = FALSE;
    ULONG start = 0;

    while (start < PhpHwCpuItemCount)
    {
        ULONG end = start;

        // whole processors up to the cut; at least one processor
        while (end < PhpHwCpuItemCount)
        {
            ULONG next = end;
            ULONG processor = PhpHwCpuTags[end].Processor;

            while (next < PhpHwCpuItemCount && PhpHwCpuTags[next].Processor == processor)
                next++;

            if (end > start && next - start > PhpHwCpuBatch)
                break;

            end = next;
        }

        status = KphHwCpuRead(&PhpHwCpuItems[start], end - start, &PhpHwCpuValues[start]);

        if ((status == STATUS_SI_SCRIPT_LIMIT || status == STATUS_INSUFFICIENT_RESOURCES) && !retried)
        {
            ULONG processorItems = 0;

            for (ULONG i = start; i < PhpHwCpuItemCount && PhpHwCpuTags[i].Processor == PhpHwCpuTags[start].Processor; i++)
                processorItems++;

            if (PhpHwCpuBatch / 2 >= processorItems)
            {
                PhpHwCpuBatch /= 2;
                retried = TRUE;
                PhTraceInfo("Hardware sensors: processor reads cut to %lu items", PhpHwCpuBatch);
                continue;
            }
        }

        if (!NT_SUCCESS(status))
        {
            for (ULONG i = start; i < end; i++)
                PhpHwCpuValues[i].Status = status;
        }
        else
        {
            answered = TRUE;
        }

        start = end;
    }

    if (!answered && status == STATUS_RETRY)
        return STATUS_RETRY;

    return answered ? STATUS_SUCCESS : status;
}

/**
 * Decodes the per-processor values of the pass and publishes them: the values first, then their bits.
 */
static VOID PhpHwCpuPublish(
    VOID
    )
{
    FLOAT seconds = 0.0f;

    if (PhpHwCpuLastSampleTime && PhpHwCpuSampleTime > PhpHwCpuLastSampleTime)
        seconds = (FLOAT)(PhpHwCpuSampleTime - PhpHwCpuLastSampleTime) / (FLOAT)PHP_HW_SECOND;

    for (ULONG i = 0; i < PhpHwProcessorCount; i++)
    {
        PPH_HW_CPU_SENSOR sensor = &PhpHwCpuSensors[i];
        PPHP_HW_SAMPLE sample = &PhpHwCpuSamples[i];
        ULONG bit = 1UL << PhpHwRegAperf;
        ULONG valid = 0;

        if (seconds > 0.0f && (sample->Valid & sample->LastValid & bit))
        {
            sensor->EffectiveMhz = PhEffectiveMhzFromAperf(sample->Value[PhpHwRegAperf] - sample->LastValue[PhpHwRegAperf], seconds);
            valid |= PH_HW_CAP_EFFECTIVE_CLOCK;
        }

        WriteULongRelease(&sensor->Valid, valid);
    }
}

/**
 * Reads every active register once and publishes what it decodes.
 *
 * \return STATUS_SUCCESS, STATUS_RETRY, or the status of a call when no call was answered.
 */
NTSTATUS PhpHwCpuPass(
    VOID
    )
{
    NTSTATUS status;

    PhpHwCpuItemCount = 0;

    for (ULONG i = 0; i < PhpHwProcessorCount; i++)
    {
        PPHP_HW_SAMPLE sample = &PhpHwCpuSamples[i];

        sample->LastValid = sample->Valid;
        memcpy(sample->LastValue, sample->Value, sizeof(sample->Value));
        sample->Valid = 0;

        for (ULONG reg = 0; reg < PhpHwRegMaximum; reg++)
        {
            if (!PhpHwCpuRegisterActive(reg) || !PhpHwCpuRegisterOnProcessor(reg, i) || PhpHwCpuItemCount >= PhpHwCpuCapacity)
                continue;

            PhpHwCpuItems[PhpHwCpuItemCount].Group = PhpHwCpuSensors[i].ProcessorNumber.Group;
            PhpHwCpuItems[PhpHwCpuItemCount].Number = PhpHwCpuSensors[i].ProcessorNumber.Number;
            PhpHwCpuItems[PhpHwCpuItemCount].Msr = PhpHwCpuRegisters[reg].Register;
            PhpHwCpuTags[PhpHwCpuItemCount].Reg = reg;
            PhpHwCpuTags[PhpHwCpuItemCount].Processor = i;
            PhpHwCpuItemCount++;
        }
    }

    if (!PhpHwCpuItemCount)
        return STATUS_SUCCESS;

    status = PhpHwCpuReadItems();

    PhpHwCpuLastSampleTime = PhpHwCpuSampleTime;
    PhpHwCpuSampleTime = PhpHwQueryTime();

    for (ULONG i = 0; i < PhpHwCpuItemCount; i++)
    {
        PPHP_HW_CPU_TAG tag = &PhpHwCpuTags[i];

        if (!PhpHwCpuApplyStatus(tag->Reg, PhpHwCpuValues[i].Status))
            continue;

        PhpHwCpuSamples[tag->Processor].Value[tag->Reg] = PhpHwCpuValues[i].Value;
        PhpHwCpuSamples[tag->Processor].Valid |= 1UL << tag->Reg;
    }

    PhpHwCpuPublish();

    return status;
}

static VOID PhpHwCpuFree(
    _Inout_ PVOID Pointer
    )
{
    PVOID* pointer = Pointer;

    if (*pointer)
    {
        PhFree(*pointer);
        *pointer = NULL;
    }
}

/**
 * Forgets the session: the plan, the samples and the identity.
 */
VOID PhpHwCpuEnd(
    VOID
    )
{
    PhpHwCpuClass = PhpHwCpuNone;
    PhpHwCpuSignature = 0;
    PhpHwCpuItemCount = 0;
    PhpHwCpuCapacity = 0;
    memset(PhpHwCpuRegisters, 0, sizeof(PhpHwCpuRegisters));

    PhpHwCpuFree(&PhpHwCpuItems);
    PhpHwCpuFree(&PhpHwCpuValues);
    PhpHwCpuFree(&PhpHwCpuTags);
    PhpHwCpuFree(&PhpHwCpuSamples);
    PhpHwCpuFree(&PhpHwCpuFirstOfCore);
    PhpHwCpuFree(&PhpHwCpuFirstOfPackage);
}

/**
 * Writes the collector's section of the hardware report: the identity, the plan and each register's state.
 *
 * \param Builder The report.
 */
VOID PhpHwCpuDiagnostics(
    _Inout_ PPH_STRING_BUILDER Builder
    )
{
    static CONST PCWSTR classNames[] = { L"none", L"Intel", L"AMD Zen", L"AMD 10h to 16h", L"Centaur" };
    static CONST PCWSTR scopeNames[] = { L"thread", L"core", L"package" };
    CHAR vendor[13];

    if (PhpHwCpuClass == PhpHwCpuNone)
    {
        PhAppendStringBuilder2(Builder, L"  Processor: not identified\n");
        return;
    }

    memcpy(vendor, PhpHwCpuIdentity.Vendor, 12);
    vendor[12] = ANSI_NULL;

    PhAppendFormatStringBuilder(
        Builder,
        L"  Processor: %hs family 0x%x model 0x%x stepping 0x%x, signature 0x%08lx, class %s\n"
        L"  Plan: %lu items a pass, calls of at most %lu\n",
        vendor,
        PhpHwCpuIdentity.DisplayFamily,
        PhpHwCpuIdentity.DisplayModel,
        PhpHwCpuIdentity.Stepping,
        PhpHwCpuSignature,
        classNames[PhpHwCpuClass],
        PhpHwCpuItemCount,
        PhpHwCpuBatch
        );

    for (ULONG reg = 0; reg < PhpHwRegMaximum; reg++)
    {
        PPHP_HW_REGISTER r = &PhpHwCpuRegisters[reg];

        if (!r->Register)
            continue;

        PhAppendFormatStringBuilder(
            Builder,
            L"  Register 0x%08lx (%s): %s\n",
            r->Register,
            scopeNames[r->Scope],
            r->Disabled ? L"left out" : L"read"
            );
    }
}
