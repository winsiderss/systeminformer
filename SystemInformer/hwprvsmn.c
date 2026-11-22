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
#include <trace.h>

// Collector smn: the AMD system management network through each package's root complex, KphHwSmnRead once per package
// per update: Tctl and the populated CCD temperatures (Zen), or ReportedTempCtrl (family 15h models 60h to 7Fh); every
// PHP_HW_SMN_MEMORY_INTERVAL passes the memory controller's configuration rides in the same call.

#define PHP_HW_SMN_MAX_ROOTS 64
#define PHP_HW_SMN_AMD_VENDOR_ID 0x1022
#define PHP_HW_SMN_HYGON_VENDOR_ID 0x1D94
#define PHP_HW_SMN_HOST_BRIDGE_CLASS 0x0600
#define PHP_HW_SMN_STRIKE_LIMIT 3 // failures of an item before it leaves the plan for the session
#define PHP_HW_SMN_MEMORY_INTERVAL 5 // passes between reads of the memory controller's configuration
#define PHP_HW_SMN_MAX_CHANNELS 12
#define PHP_HW_SMN_TEMPERATURE 0 // item index of Tctl or ReportedTempCtrl; then each CCD
#define PHP_HW_SMN_MEMORY (1 + PH_HW_MAX_CCD) // item index of the memory channel's DRAM_CONFIGURATION
#define PHP_HW_SMN_ITEMS (PHP_HW_SMN_MEMORY + 1)
#define PHP_HW_SMN_SEARCH_REGISTERS 3 // UMC_CAP, BASE_ADDR_CS0, BASE_ADDR_CS2 of each channel
#define PHP_HW_SMN_TIMING_REGISTERS 7 // CONFIGURATION, DRAM_TIMING1, DRAM_TIMING2, the four refresh registers
#define PHP_HW_SMN_MAX_EXTRA (PHP_HW_SMN_SEARCH_REGISTERS * PHP_HW_SMN_MAX_CHANNELS)
#define PHP_HW_SMN_MAX_CALL (PHP_HW_SMN_ITEMS + PHP_HW_SMN_MAX_EXTRA)

typedef enum _PHP_HW_SMN_CLASS
{
    PhpHwSmnNone,
    PhpHwSmnZen, // family 17h and later: THM_TCON_CUR_TMP, the CCD registers and the memory controller
    PhpHwSmn15h // family 15h models 60h to 7Fh: D0F0xBC_xD8200CA4
} PHP_HW_SMN_CLASS;

typedef struct _PHP_HW_SMN_ITEM
{
    ULONG Address; // 0 when not in the plan
    BOOLEAN Disabled;
    UCHAR Strikes;
} PHP_HW_SMN_ITEM, *PPHP_HW_SMN_ITEM;

typedef struct _PHP_HW_SMN_PACKAGE
{
    ULONG Locator; // the root complex
    BOOLEAN HasRoot;
    BOOLEAN ChannelSearch; // the memory channel is still to be found
    BOOLEAN TimingsPending; // the DDR5 timings are still to be read
    BOOLEAN MemoryValid; // MemoryClockMhz and MemoryDataRate hold the last decode
    BOOLEAN TimingsValid;
    ULONG MemoryChannel; // the UMC channel's SMN base
    FLOAT MemoryClockMhz;
    FLOAT MemoryDataRate;
    PH_HW_MEMORY_TIMINGS Timings;
    PHP_HW_SMN_ITEM Items[PHP_HW_SMN_ITEMS];
} PHP_HW_SMN_PACKAGE, *PPHP_HW_SMN_PACKAGE;

static PHP_HW_SMN_CLASS PhpHwSmnClass = PhpHwSmnNone;
static KPH_HW_SMN_IDENTITY PhpHwSmnRootIdentity;
static PH_CPU_IDENTITY PhpHwSmnIdentity;
static PPHP_HW_SMN_PACKAGE PhpHwSmnPackages = NULL; // per package
static ULONG PhpHwSmnRootCount = 0; // discovered
static ULONG PhpHwSmnCcdBase = 0;
static ULONG PhpHwSmnCcdCount = 0;
static ULONG PhpHwSmnChannelCount = 0; // UMC channels to search; 0 without a known memory controller layout
static FLOAT PhpHwSmnTctlOffset = 0.0f;
static ULONG PhpHwSmnPasses = 0; // passes of the session

/**
 * Applies the session failure policy to an item: a refused or misaligned address leaves the plan at once; contention
 * and the all-ones answer are no value, not a failure; anything else strikes.
 *
 * \return TRUE when the value is usable.
 */
static BOOLEAN PhpHwSmnApplyStatus(
    _Inout_ PPHP_HW_SMN_ITEM Item,
    _In_ NTSTATUS Status
    )
{
    if (NT_SUCCESS(Status))
    {
        Item->Strikes = 0;
        return TRUE;
    }

    switch (Status)
    {
    case STATUS_RETRY: // the index moved
    case STATUS_DEVICE_NOT_READY: // the all-ones answer
        break;
    case STATUS_ACCESS_DENIED: // not permitted
    case STATUS_INVALID_PARAMETER: // not aligned
        Item->Disabled = TRUE;
        PhTraceInfo("Hardware sensor SMN 0x%08lx unavailable: 0x%lx", Item->Address, Status);
        break;
    default:
        if (++Item->Strikes >= PHP_HW_SMN_STRIKE_LIMIT)
        {
            Item->Disabled = TRUE;
            PhTraceInfo("Hardware sensor SMN 0x%08lx unavailable: 0x%lx", Item->Address, Status);
        }
        break;
    }

    return FALSE;
}

/**
 * Reads, in one call, the planned items of a package that a mask names and further addresses whose answers the
 * caller judges.
 *
 * \param Package The package.
 * \param ItemMask The items to read, 1 << index.
 * \param Extra The further addresses.
 * \param ExtraCount The number of further addresses.
 * \param Values Receives a value per item; a value is usable when its bit is set in the return.
 * \param ExtraValues Receives the answer of each further address; with a failed call, the call's status.
 * \param Status Receives the status of the call.
 * \return The bits of the items that gave a value.
 */
static ULONG PhpHwSmnReadPackage(
    _Inout_ PPHP_HW_SMN_PACKAGE Package,
    _In_ ULONG ItemMask,
    _In_reads_(ExtraCount) PCULONG Extra,
    _In_ ULONG ExtraCount,
    _Out_writes_(PHP_HW_SMN_ITEMS) PULONG Values,
    _Out_writes_(ExtraCount) PKPH_HW_VALUE32 ExtraValues,
    _Out_ PNTSTATUS Status
    )
{
    ULONG addresses[PHP_HW_SMN_MAX_CALL];
    ULONG items[PHP_HW_SMN_ITEMS];
    KPH_HW_VALUE32 values[PHP_HW_SMN_MAX_CALL];
    ULONG itemCount = 0;
    ULONG answered = 0;
    NTSTATUS status;

    memset(Values, 0, sizeof(ULONG) * PHP_HW_SMN_ITEMS);
    *Status = STATUS_SUCCESS;

    for (ULONG i = 0; i < PHP_HW_SMN_ITEMS; i++)
    {
        if (!(ItemMask & (1UL << i)) || !Package->Items[i].Address || Package->Items[i].Disabled)
            continue;

        addresses[itemCount] = Package->Items[i].Address;
        items[itemCount] = i;
        itemCount++;
    }

    for (ULONG i = 0; i < ExtraCount; i++)
    {
        addresses[itemCount + i] = Extra[i];
        ExtraValues[i].Status = STATUS_RETRY;
        ExtraValues[i].Value = 0;
    }

    if (!itemCount && !ExtraCount)
        return 0;

    status = KphHwSmnRead(Package->Locator, addresses, itemCount + ExtraCount, values);
    *Status = status;

    // Contention backs the whole family off (D9); any other failed call is each item's status.
    if (status == STATUS_RETRY)
        return 0;

    for (ULONG i = 0; i < itemCount; i++)
    {
        if (!PhpHwSmnApplyStatus(&Package->Items[items[i]], NT_SUCCESS(status) ? values[i].Status : status))
            continue;

        Values[items[i]] = values[i].Value;
        answered |= 1UL << items[i];
    }

    for (ULONG i = 0; i < ExtraCount; i++)
    {
        ExtraValues[i].Status = NT_SUCCESS(status) ? values[itemCount + i].Status : status;
        ExtraValues[i].Value = values[itemCount + i].Value;
    }

    return answered;
}

/**
 * Lists the channel search's addresses: UMC_CAP, BASE_ADDR_CS0 and BASE_ADDR_CS2 of each channel.
 *
 * \return The number of addresses.
 */
static ULONG PhpHwSmnSearchAddresses(
    _Out_writes_(PHP_HW_SMN_MAX_EXTRA) PULONG Addresses
    )
{
    for (ULONG i = 0; i < PhpHwSmnChannelCount; i++)
    {
        ULONG channel = PH_AMD_SMN_UMC_BASE + (i << PH_AMD_SMN_UMC_CHANNEL_SHIFT);

        Addresses[i * 3 + 0] = channel + PH_AMD_UMC_CAP;
        Addresses[i * 3 + 1] = channel + PH_AMD_UMC_BASE_ADDR_CS0;
        Addresses[i * 3 + 2] = channel + PH_AMD_UMC_BASE_ADDR_CS2;
    }

    return PhpHwSmnChannelCount * 3;
}

/**
 * Takes the first enabled channel with a DIMM from the channel search's answers. A channel not fully answered with
 * contention or the all-ones value leaves the search for a later pass; a refused one, or none active, ends it.
 */
static VOID PhpHwSmnTakeChannel(
    _Inout_ PPHP_HW_SMN_PACKAGE Package,
    _In_reads_(PhpHwSmnChannelCount * 3) PKPH_HW_VALUE32 Values
    )
{
    for (ULONG i = 0; i < PhpHwSmnChannelCount; i++)
    {
        PKPH_HW_VALUE32 channel = &Values[i * 3];

        for (ULONG j = 0; j < 3; j++)
        {
            if (channel[j].Status == STATUS_RETRY || channel[j].Status == STATUS_DEVICE_NOT_READY)
                return;
            if (!NT_SUCCESS(channel[j].Status))
            {
                Package->ChannelSearch = FALSE;
                return;
            }
        }

        if (!(channel[0].Value & (1UL << 19)) && ((channel[1].Value | channel[2].Value) & 1))
        {
            Package->MemoryChannel = PH_AMD_SMN_UMC_BASE + (i << PH_AMD_SMN_UMC_CHANNEL_SHIFT);
            Package->Items[PHP_HW_SMN_MEMORY].Address = Package->MemoryChannel + PH_AMD_UMC_DRAM_CONFIGURATION;
            Package->ChannelSearch = FALSE;
            Package->TimingsPending = PhAmdUmcIsDdr5Layout(&PhpHwSmnIdentity);
            return;
        }
    }

    Package->ChannelSearch = FALSE;
}

/**
 * Lists the DDR5 timing registers of the package's channel.
 *
 * \return The number of addresses.
 */
static ULONG PhpHwSmnTimingAddresses(
    _In_ PPHP_HW_SMN_PACKAGE Package,
    _Out_writes_(PHP_HW_SMN_TIMING_REGISTERS) PULONG Addresses
    )
{
    Addresses[0] = Package->MemoryChannel + PH_AMD_UMC_CONFIGURATION;
    Addresses[1] = Package->MemoryChannel + PH_AMD_UMC_DRAM_TIMING1;
    Addresses[2] = Package->MemoryChannel + PH_AMD_UMC_DRAM_TIMING2;

    for (ULONG i = 0; i < 4; i++)
        Addresses[3 + i] = Package->MemoryChannel + PH_AMD_UMC_DRAM_TIMING_RFC + i * 4;

    return PHP_HW_SMN_TIMING_REGISTERS;
}

/**
 * Decodes the DDR5 timings of a channel. A register not answered leaves them for a later read.
 */
static VOID PhpHwSmnDecodeTimings(
    _Inout_ PPHP_HW_SMN_PACKAGE Package,
    _In_ ULONG DramConfiguration,
    _In_reads_(PHP_HW_SMN_TIMING_REGISTERS) PKPH_HW_VALUE32 Values
    )
{
    ULONG refresh = 0;

    for (ULONG i = 0; i < PHP_HW_SMN_TIMING_REGISTERS; i++)
    {
        if (Values[i].Status == STATUS_RETRY || Values[i].Status == STATUS_DEVICE_NOT_READY)
            return;
        if (!NT_SUCCESS(Values[i].Status))
        {
            Package->TimingsPending = FALSE;
            return;
        }
    }

    Package->TimingsPending = FALSE;

    if ((Values[0].Value & 0x7) != 1)
        return;

    // An unused refresh register reads zero or the reset value 00C00138h.
    for (ULONG i = 0; i < 4; i++)
    {
        if (Values[3 + i].Value && Values[3 + i].Value != 0x00C00138)
        {
            refresh = Values[3 + i].Value & 0xFFFF;
            break;
        }
    }

    Package->Timings.CasLatency = Values[1].Value & 0x3F;
    Package->Timings.RasToCasDelay = (Values[1].Value >> 16) & 0x3F;
    Package->Timings.RowActive = (Values[1].Value >> 8) & 0x7F;
    Package->Timings.RowPrecharge = (Values[2].Value >> 16) & 0x3F;
    Package->Timings.RowCycle = Values[2].Value & 0xFF;
    Package->Timings.RefreshCycle = refresh;
    Package->Timings.CommandRate = (DramConfiguration & (1UL << 17)) ? 2 : 1;
    Package->Timings.GearDownValid = TRUE;
    Package->Timings.GearDown = !!(DramConfiguration & (1UL << 18));
    Package->TimingsValid = Package->Timings.CasLatency != 0;
}

/**
 * Reads a package's planned items, with the memory controller's turn when it is due: its configuration (and the DDR5
 * timings still pending), or the channel search while no channel is known.
 *
 * \param Package The package.
 * \param ItemMask The temperature items to read.
 * \param Memory The memory controller's turn.
 * \param Probe The probe's read: a configuration that does not decode leaves the plan.
 * \param Values Receives a value per item; see PhpHwSmnReadPackage.
 * \param Status Receives the status of the call.
 * \return The bits of the items that gave a value.
 */
static ULONG PhpHwSmnReadWithMemory(
    _Inout_ PPHP_HW_SMN_PACKAGE Package,
    _In_ ULONG ItemMask,
    _In_ BOOLEAN Memory,
    _In_ BOOLEAN Probe,
    _Out_writes_(PHP_HW_SMN_ITEMS) PULONG Values,
    _Out_ PNTSTATUS Status
    )
{
    ULONG extra[PHP_HW_SMN_MAX_EXTRA];
    KPH_HW_VALUE32 extraValues[PHP_HW_SMN_MAX_EXTRA];
    ULONG extraCount = 0;
    BOOLEAN search = FALSE;
    BOOLEAN timings = FALSE;
    ULONG answered;
    FLOAT mhz;
    FLOAT dataRate;

    if (Memory && Package->ChannelSearch)
    {
        extraCount = PhpHwSmnSearchAddresses(extra);
        search = TRUE;
    }
    else if (Memory && Package->Items[PHP_HW_SMN_MEMORY].Address && !Package->Items[PHP_HW_SMN_MEMORY].Disabled)
    {
        ItemMask |= 1UL << PHP_HW_SMN_MEMORY;

        if (Package->TimingsPending)
        {
            extraCount = PhpHwSmnTimingAddresses(Package, extra);
            timings = TRUE;
        }
    }

    answered = PhpHwSmnReadPackage(Package, ItemMask, extra, extraCount, Values, extraValues, Status);

    if (*Status == STATUS_RETRY)
        return 0;

    if (search)
        PhpHwSmnTakeChannel(Package, extraValues);

    if (answered & (1UL << PHP_HW_SMN_MEMORY))
    {
        if (PhAmdDramConfigurationToMhz(Values[PHP_HW_SMN_MEMORY], &PhpHwSmnIdentity, &mhz, &dataRate))
        {
            Package->MemoryClockMhz = mhz;
            Package->MemoryDataRate = dataRate;
            Package->MemoryValid = TRUE;

            if (timings)
                PhpHwSmnDecodeTimings(Package, Values[PHP_HW_SMN_MEMORY], extraValues);
        }
        else if (Probe)
        {
            Package->Items[PHP_HW_SMN_MEMORY].Disabled = TRUE;
        }
    }

    if (Package->Items[PHP_HW_SMN_MEMORY].Disabled)
    {
        Package->MemoryValid = FALSE;
        Package->TimingsValid = FALSE;
    }

    return answered;
}

/**
 * Splits the root complexes evenly over the packages in bus order; when they do not split evenly, the first package
 * takes the first.
 */
static VOID PhpHwSmnAssignRootComplexes(
    VOID
    )
{
    ULONG roots[PHP_HW_SMN_MAX_ROOTS];

    PhpHwSmnRootCount = PhpHwEnumeratePciFunctions(
        PHP_HW_SMN_AMD_VENDOR_ID,
        PHP_HW_SMN_HYGON_VENDOR_ID,
        PHP_HW_SMN_HOST_BRIDGE_CLASS,
        NULL,
        0,
        TRUE,
        roots,
        RTL_NUMBER_OF(roots)
        );

    if (!PhpHwSmnRootCount)
        return;

    if (PhpHwSmnRootCount % PhpHwPackageCount == 0)
    {
        ULONG step = PhpHwSmnRootCount / PhpHwPackageCount;

        for (ULONG i = 0; i < PhpHwPackageCount; i++)
        {
            PhpHwSmnPackages[i].HasRoot = TRUE;
            PhpHwSmnPackages[i].Locator = roots[i * step];
        }
    }
    else
    {
        PhpHwSmnPackages[0].HasRoot = TRUE;
        PhpHwSmnPackages[0].Locator = roots[0];
    }
}

/**
 * Identifies the root complex of the first package and plans every package: the temperatures, the CCD slots that are
 * populated and the memory channel. Called once a session.
 *
 * \return STATUS_SUCCESS, STATUS_NOT_SUPPORTED for a machine or processor the collector does not serve, or the
 * status of the identify call.
 */
NTSTATUS PhpHwSmnProbe(
    VOID
    )
{
    NTSTATUS status;
    PPH_STRING brand;

    if (PhpHwQueryMachine() == IMAGE_FILE_MACHINE_ARM64 || !PhpHwPackageCount)
        return STATUS_NOT_SUPPORTED;

    PhpHwSmnPackages = PhAllocateZero(sizeof(PHP_HW_SMN_PACKAGE) * PhpHwPackageCount);
    PhpHwSmnPasses = 0;
    PhpHwSmnAssignRootComplexes();

    if (!PhpHwSmnPackages[0].HasRoot)
        return STATUS_NOT_SUPPORTED;

    // The identity of the processor comes from the script, so this family does not depend on cpu.
    status = KphHwSmnIdentify(PhpHwSmnPackages[0].Locator, &PhpHwSmnRootIdentity);

    if (!NT_SUCCESS(status))
        return status;

    memset(&PhpHwSmnIdentity, 0, sizeof(PH_CPU_IDENTITY));
    memcpy(PhpHwSmnIdentity.Vendor, PhpHwSmnRootIdentity.Vendor == PHP_HW_SMN_HYGON_VENDOR_ID ? "HygonGenuine" : "AuthenticAMD", 12);
    PhpHwSmnIdentity.DisplayFamily = (USHORT)PhpHwSmnRootIdentity.Family;
    PhpHwSmnIdentity.DisplayModel = (UCHAR)PhpHwSmnRootIdentity.Model;
    PhpHwSmnIdentity.Stepping = (UCHAR)PhpHwSmnRootIdentity.Stepping;

    if (PhpHwSmnIdentity.DisplayFamily >= 0x17)
        PhpHwSmnClass = PhpHwSmnZen;
    else if (PhpHwSmnIdentity.DisplayFamily == 0x15 && PhpHwSmnIdentity.DisplayModel >= 0x60 && PhpHwSmnIdentity.DisplayModel <= 0x7F)
        PhpHwSmnClass = PhpHwSmn15h;
    else
        return STATUS_NOT_SUPPORTED;

    if (PhpHwSmnClass == PhpHwSmnZen)
    {
        if (!PhAmdTccdAddressRange(&PhpHwSmnIdentity, &PhpHwSmnCcdBase, &PhpHwSmnCcdCount))
        {
            PhpHwSmnCcdBase = 0;
            PhpHwSmnCcdCount = 0;
        }

        if (!PhAmdUmcChannelCount(&PhpHwSmnIdentity, &PhpHwSmnChannelCount))
            PhpHwSmnChannelCount = 0;

        PhpHwSmnCcdCount = min(PhpHwSmnCcdCount, PH_HW_MAX_CCD);
        PhpHwSmnChannelCount = min(PhpHwSmnChannelCount, PHP_HW_SMN_MAX_CHANNELS);

        if (brand = PhpHwQueryProcessorName())
        {
            PhpHwSmnTctlOffset = PhAmdTctlOffset(&PhpHwSmnIdentity, &brand->sr);
            PhDereferenceObject(brand);
        }
    }

    // The probe reads every CCD slot and searches the memory channels in one call; a slot whose valid bit is clear is
    // not populated. A found channel's configuration and timings are a second call.
    for (ULONG i = 0; i < PhpHwPackageCount; i++)
    {
        PPHP_HW_SMN_PACKAGE package = &PhpHwSmnPackages[i];
        ULONG values[PHP_HW_SMN_ITEMS];
        ULONG answered;
        FLOAT celsius;

        if (!package->HasRoot)
            continue;

        if (PhpHwSmnClass == PhpHwSmnZen)
        {
            package->Items[PHP_HW_SMN_TEMPERATURE].Address = PH_AMD_SMN_THM_TCON_CUR_TMP;

            for (ULONG j = 0; j < PhpHwSmnCcdCount; j++)
                package->Items[1 + j].Address = PhpHwSmnCcdBase + j * 4;

            package->ChannelSearch = PhpHwSmnChannelCount != 0;
        }
        else
        {
            package->Items[PHP_HW_SMN_TEMPERATURE].Address = PH_AMD_D0F0XBC_REPORTED_TEMP_CTRL;
        }

        answered = PhpHwSmnReadWithMemory(package, ~(1UL << PHP_HW_SMN_MEMORY), TRUE, TRUE, values, &status);

        for (ULONG j = 0; j < PhpHwSmnCcdCount; j++)
        {
            if ((answered & (1UL << (1 + j))) && !PhAmdTccdToCelsius(values[1 + j], &celsius))
                package->Items[1 + j].Disabled = TRUE;
        }

        if (package->Items[PHP_HW_SMN_MEMORY].Address)
            PhpHwSmnReadWithMemory(package, 0, TRUE, TRUE, values, &status);
    }

    return STATUS_SUCCESS;
}

/**
 * Reads and publishes the temperatures of every package, one call a package; every PHP_HW_SMN_MEMORY_INTERVAL passes
 * the memory controller rides in it.
 *
 * \return STATUS_RETRY when every call met contention, else STATUS_SUCCESS.
 */
NTSTATUS PhpHwSmnPass(
    VOID
    )
{
    BOOLEAN answered = FALSE;
    BOOLEAN retried = FALSE;
    BOOLEAN memory;

    memory = ++PhpHwSmnPasses % PHP_HW_SMN_MEMORY_INTERVAL == 0;

    for (ULONG i = 0; i < PhpHwPackageCount; i++)
    {
        PPHP_HW_SMN_PACKAGE package = &PhpHwSmnPackages[i];
        PPH_HW_PACKAGE_SENSOR sensor = &PhpHwPackageSensors[i];
        PPHP_HW_SMN_ITEM memoryItem = &package->Items[PHP_HW_SMN_MEMORY];
        ULONG values[PHP_HW_SMN_ITEMS];
        ULONG owned = 0;
        ULONG valid = 0;
        ULONG ccdValidMask = 0;
        ULONG got;
        NTSTATUS status;
        FLOAT celsius;

        if (!package->HasRoot)
            continue;

        for (ULONG j = 0; j < PHP_HW_SMN_MEMORY; j++)
        {
            if (package->Items[j].Address && !package->Items[j].Disabled)
                owned |= j ? PH_HW_CAP_CCD_TEMP : PH_HW_CAP_PACKAGE_TEMP;
        }

        if (package->ChannelSearch || (memoryItem->Address && !memoryItem->Disabled))
            owned |= PH_HW_CAP_MEMORY_CLOCK | PH_HW_CAP_MEMORY_TIMINGS;

        if (!owned)
            continue;

        got = PhpHwSmnReadWithMemory(package, ~(1UL << PHP_HW_SMN_MEMORY), memory, FALSE, values, &status);

        if (status == STATUS_RETRY)
            retried = TRUE;
        else
            answered = TRUE;

        if (got & (1UL << PHP_HW_SMN_TEMPERATURE))
        {
            BOOLEAN decoded = PhpHwSmnClass == PhpHwSmnZen ?
                PhAmdTctlToCelsius(values[PHP_HW_SMN_TEMPERATURE], &celsius) :
                PhAmdReportedTempCtrlToCelsius(values[PHP_HW_SMN_TEMPERATURE], &PhpHwSmnIdentity, &celsius);

            if (decoded)
            {
                sensor->Temperature = celsius - PhpHwSmnTctlOffset;
                valid |= PH_HW_CAP_PACKAGE_TEMP;
            }
        }

        for (ULONG j = 0; j < PhpHwSmnCcdCount; j++)
        {
            if (!(got & (1UL << (1 + j))) || !PhAmdTccdToCelsius(values[1 + j], &celsius))
                continue;

            sensor->CcdTemperature[j] = celsius;

            if (!ccdValidMask || celsius > sensor->MaxCoreTemperature)
                sensor->MaxCoreTemperature = celsius;

            ccdValidMask |= 1UL << j;
            valid |= PH_HW_CAP_CCD_TEMP;
        }

        sensor->CcdValidMask = ccdValidMask;

        // The SMU's memory clock of its last pass takes the UMC's place; the UMC's last decode stands between turns.
        if (PhpHwSmuQueryMemoryClock(i, &celsius))
        {
            sensor->MemoryClockMhz = celsius;
            sensor->MemoryDataRate = celsius * 2.0f;
            owned |= PH_HW_CAP_MEMORY_CLOCK;
            valid |= PH_HW_CAP_MEMORY_CLOCK;
        }
        else if (package->MemoryValid)
        {
            sensor->MemoryClockMhz = package->MemoryClockMhz;
            sensor->MemoryDataRate = package->MemoryDataRate;
            valid |= PH_HW_CAP_MEMORY_CLOCK;
        }

        if (package->TimingsValid)
        {
            sensor->MemoryTimings = package->Timings;
            valid |= PH_HW_CAP_MEMORY_TIMINGS;
        }

        // Only the bits this collector owns are replaced; the others belong to cpu.
        WriteULongRelease(&sensor->Valid, (ReadULongAcquire(&sensor->Valid) & ~owned) | (valid & owned));
    }

    return retried && !answered ? STATUS_RETRY : STATUS_SUCCESS;
}

/**
 * Retrieves the root complex this session assigned to a package, for smu and hsmp.
 *
 * \param Package The package.
 * \param Locator Receives the root complex.
 * \return TRUE when the package has one.
 */
_Success_(return)
BOOLEAN PhpHwSmnQueryRoot(
    _In_ ULONG Package,
    _Out_ PULONG Locator
    )
{
    if (!PhpHwSmnPackages || Package >= PhpHwPackageCount || !PhpHwSmnPackages[Package].HasRoot)
        return FALSE;

    *Locator = PhpHwSmnPackages[Package].Locator;
    return TRUE;
}

/**
 * Ends the session: the plan and the root complexes are found again at the next probe.
 */
VOID PhpHwSmnEnd(
    VOID
    )
{
    if (PhpHwSmnPackages)
    {
        PhFree(PhpHwSmnPackages);
        PhpHwSmnPackages = NULL;
    }

    PhpHwSmnClass = PhpHwSmnNone;
    PhpHwSmnRootCount = 0;
    PhpHwSmnCcdBase = 0;
    PhpHwSmnCcdCount = 0;
    PhpHwSmnChannelCount = 0;
    PhpHwSmnTctlOffset = 0.0f;
    PhpHwSmnPasses = 0;
}

/**
 * Writes the collector's section of the hardware report: the root complexes, the identity, the memory channel and
 * each item's state.
 *
 * \param Builder The report.
 */
VOID PhpHwSmnDiagnostics(
    _Inout_ PPH_STRING_BUILDER Builder
    )
{
    if (PhpHwSmnClass == PhpHwSmnNone)
    {
        PhAppendFormatStringBuilder(Builder, L"  Root complexes: %lu found\n", PhpHwSmnRootCount);
        return;
    }

    PhAppendFormatStringBuilder(
        Builder,
        L"  Root complexes: %lu found; %04x:%04x, family 0x%lx model 0x%lx stepping 0x%lx\n"
        L"  CCD registers: %lu from 0x%08lx; Tctl offset %.0f\n",
        PhpHwSmnRootCount,
        PhpHwSmnRootIdentity.Vendor,
        PhpHwSmnRootIdentity.Device,
        PhpHwSmnRootIdentity.Family,
        PhpHwSmnRootIdentity.Model,
        PhpHwSmnRootIdentity.Stepping,
        PhpHwSmnCcdCount,
        PhpHwSmnCcdBase,
        PhpHwSmnTctlOffset
        );

    for (ULONG i = 0; i < PhpHwPackageCount; i++)
    {
        PPHP_HW_SMN_PACKAGE package = &PhpHwSmnPackages[i];

        if (!package->HasRoot)
        {
            PhAppendFormatStringBuilder(Builder, L"  Package %lu: no root complex\n", i);
            continue;
        }

        PhAppendFormatStringBuilder(Builder, L"  Package %lu: root complex 0x%06lx\n", i, package->Locator);

        if (package->Items[PHP_HW_SMN_MEMORY].Address)
        {
            PhAppendFormatStringBuilder(
                Builder,
                L"    Memory channel 0x%08lx: %.0f MHz, %.0f MT/s, timings %s\n",
                package->MemoryChannel,
                package->MemoryClockMhz,
                package->MemoryDataRate,
                package->TimingsValid ? L"read" : package->TimingsPending ? L"pending" : L"none"
                );
        }
        else if (PhpHwSmnChannelCount)
        {
            PhAppendFormatStringBuilder(Builder, L"    Memory channel: %s\n", package->ChannelSearch ? L"searching" : L"none active");
        }

        for (ULONG j = 0; j < PHP_HW_SMN_ITEMS; j++)
        {
            if (!package->Items[j].Address)
                continue;

            PhAppendFormatStringBuilder(
                Builder,
                L"    SMN 0x%08lx: %s\n",
                package->Items[j].Address,
                package->Items[j].Disabled ? L"left out" : L"read"
                );
        }
    }
}
