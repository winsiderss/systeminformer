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
#include <hwprv.h>
#include <syssccpuhw.h>

/**
 * Selects the CPU section's sensor graphs: the temperature when there is one, and the power only beside it.
 *
 * \param Capabilities PH_HW_CAP_* flags.
 * \return The PH_SIP_CPU_SENSOR_GRAPHS flags to show.
 */
ULONG PhSipCpuSelectSensorGraphs(
    _In_ ULONG Capabilities
    )
{
    if (!(Capabilities & PH_HW_CAP_PACKAGE_TEMP))
        return 0;

    return Capabilities & PH_SIP_CPU_SENSOR_GRAPHS;
}

/**
 * Sums and picks the CPU section's hardware sensor values over the packages and logical processors.
 *
 * \param Sensors Receives the values; Valid is zero when hardware sensors are unavailable.
 */
VOID PhSipCpuQueryHardwareSensors(
    _Out_ PPH_SIP_CPU_SENSORS Sensors
    )
{
    ULONG valid;
    ULONG activeCount = 0;
    ULONG multiplierCount = 0;
    ULONG packageCount;
    PPH_HW_PACKAGE_SENSOR package;
    PPH_HW_CPU_SENSOR sensor;

    memset(Sensors, 0, sizeof(PH_SIP_CPU_SENSORS));

    if (!PhHwSensorsAvailable())
        return;

    // The processor values, which the Windows counters complete where no package has one.
    valid = PhHwGetCpuValues(&Sensors->Temperature, &Sensors->PackagePower);

    packageCount = PhHwGetPackageCount();

    for (ULONG i = 0; i < packageCount; i++)
    {
        if (!(package = PhHwGetPackageSensor(i)))
            break;

        if (package->Valid & PH_HW_CAP_CCD_TEMP)
        {
            for (ULONG ccd = 0; ccd < PH_HW_MAX_CCD; ccd++)
            {
                if (!(package->CcdValidMask & (1UL << ccd)))
                    continue;

                if (!(valid & PH_HW_CAP_CCD_TEMP) || package->CcdTemperature[ccd] > Sensors->CcdTemperature)
                    Sensors->CcdTemperature = package->CcdTemperature[ccd];

                valid |= PH_HW_CAP_CCD_TEMP;
            }
        }

        if (package->Valid & PH_HW_CAP_RAPL_CORE)
        {
            Sensors->CorePower += package->CorePower;
            valid |= PH_HW_CAP_RAPL_CORE;
        }

        if (package->Valid & PH_HW_CAP_RAPL_DRAM)
        {
            Sensors->DramPower += package->DramPower;
            valid |= PH_HW_CAP_RAPL_DRAM;
        }

        if (package->Valid & PH_HW_CAP_THROTTLE)
        {
            Sensors->ThrottleFlags |= package->ThrottleFlags;
            valid |= PH_HW_CAP_THROTTLE;
        }
    }

    for (ULONG i = 0; i < PhSystemProcessorInformation.NumberOfProcessors; i++)
    {
        if (!(sensor = PhHwGetCpuSensor(i)))
            break;

        if (sensor->Valid & PH_HW_CAP_ACTIVE_CLOCK)
        {
            Sensors->ActiveMhz += sensor->ActiveMhz;
            activeCount++;
        }

        if (sensor->Valid & PH_HW_CAP_MULTIPLIER)
        {
            Sensors->Multiplier += sensor->Multiplier;
            multiplierCount++;
        }

        if (sensor->Valid & PH_HW_CAP_VOLTAGE)
        {
            if (!(valid & PH_HW_CAP_VOLTAGE) || sensor->Voltage > Sensors->Voltage)
                Sensors->Voltage = sensor->Voltage;

            valid |= PH_HW_CAP_VOLTAGE;
        }
    }

    if (activeCount)
    {
        Sensors->ActiveMhz /= (FLOAT)activeCount;
        valid |= PH_HW_CAP_ACTIVE_CLOCK;
    }

    if (multiplierCount)
    {
        Sensors->Multiplier /= (FLOAT)multiplierCount;
        valid |= PH_HW_CAP_MULTIPLIER;
    }

    Sensors->Valid = valid;
}

// Capability is PH_HW_CAP_PACKAGE_TEMP (degrees Celsius) or PH_HW_CAP_RAPL_PACKAGE (watts).
static VOID PhpSipCpuInitSensorFormat(
    _In_ ULONG Capability,
    _In_ FLOAT Value,
    _In_ USHORT Precision,
    _Out_writes_(PH_HW_TEMPERATURE_FORMAT_COUNT) PPH_FORMAT Format
    )
{
    C_ASSERT(PH_HW_TEMPERATURE_FORMAT_COUNT == 2);

    if (Capability == PH_HW_CAP_PACKAGE_TEMP)
    {
        PhHwFormatTemperature(Value, Precision, Format);
    }
    else
    {
        PhInitFormatF(&Format[0], Value, Precision);
        PhInitFormatS(&Format[1], L" W");
    }
}

/**
 * Formats the overlay text of a sensor graph: the temperature graph's hottest package and CCD and the throttle
 * reasons, or the power graph's package, core and DRAM power and the highest voltage.
 *
 * \param Capability PH_HW_CAP_PACKAGE_TEMP or PH_HW_CAP_RAPL_PACKAGE.
 * \return The text, or NULL when the graphed value is not current.
 */
_Ret_maybenull_
PPH_STRING PhSipCpuFormatSensorGraphText(
    _In_ ULONG Capability
    )
{
    static CONST struct { ULONG Flag; PCWSTR Name; } throttleNames[] =
    {
        { PH_HW_THROTTLE_THERMAL, L"Therm" },
        { PH_HW_THROTTLE_PROCHOT, L"PROCHOT" },
        { PH_HW_THROTTLE_CRITICAL, L"Crit" },
        { PH_HW_THROTTLE_POWER_LIMIT, L"PL" },
        { PH_HW_THROTTLE_PL1, L"PL1" },
        { PH_HW_THROTTLE_PL2, L"PL2" },
        { PH_HW_THROTTLE_VR_THERMAL, L"VR" },
        { PH_HW_THROTTLE_VR_CURRENT, L"TDC" },
        { PH_HW_THROTTLE_ELECTRICAL, L"EDP" },
        { PH_HW_THROTTLE_RUNNING_AVERAGE_THERMAL, L"RATL" },
        { PH_HW_THROTTLE_GRAPHICS, L"GFX" },
        { PH_HW_THROTTLE_AUTONOMOUS, L"Auto" },
    };
    PH_SIP_CPU_SENSORS sensors;
    PH_FORMAT format[PH_HW_TEMPERATURE_FORMAT_COUNT * 2 + 2 + RTL_NUMBER_OF(throttleNames) * 2];
    ULONG count = 0;

    PhSipCpuQueryHardwareSensors(&sensors);

    if (!(sensors.Valid & Capability))
        return NULL;

    if (Capability == PH_HW_CAP_PACKAGE_TEMP)
    {
        // %.1f C, CCD %.1f C, throttle reasons
        PhHwFormatTemperature(sensors.Temperature, 1, &format[count]);
        count += PH_HW_TEMPERATURE_FORMAT_COUNT;

        if (sensors.Valid & PH_HW_CAP_CCD_TEMP)
        {
            PhInitFormatS(&format[count++], L" \u00b7 CCD ");
            PhHwFormatTemperature(sensors.CcdTemperature, 1, &format[count]);
            count += PH_HW_TEMPERATURE_FORMAT_COUNT;
        }

        if ((sensors.Valid & PH_HW_CAP_THROTTLE) && sensors.ThrottleFlags)
        {
            PCWSTR separator = L" \u00b7 Throttling: ";

            for (ULONG i = 0; i < RTL_NUMBER_OF(throttleNames); i++)
            {
                if (!(sensors.ThrottleFlags & throttleNames[i].Flag))
                    continue;

                PhInitFormatS(&format[count++], separator);
                PhInitFormatS(&format[count++], throttleNames[i].Name);
                separator = L", ";
            }
        }
    }
    else
    {
        // %.1f W, core %.1f W, DRAM %.1f W, %.2f V
        PhInitFormatF(&format[count++], sensors.PackagePower, 1);
        PhInitFormatS(&format[count++], L" W");

        if (sensors.Valid & PH_HW_CAP_RAPL_CORE)
        {
            PhInitFormatS(&format[count++], L" \u00b7 Core ");
            PhInitFormatF(&format[count++], sensors.CorePower, 1);
            PhInitFormatS(&format[count++], L" W");
        }

        if (sensors.Valid & PH_HW_CAP_RAPL_DRAM)
        {
            PhInitFormatS(&format[count++], L" \u00b7 DRAM ");
            PhInitFormatF(&format[count++], sensors.DramPower, 1);
            PhInitFormatS(&format[count++], L" W");
        }

        if (sensors.Valid & PH_HW_CAP_VOLTAGE)
        {
            PhInitFormatS(&format[count++], L" \u00b7 ");
            PhInitFormatF(&format[count++], sensors.Voltage, 2);
            PhInitFormatS(&format[count++], L" V");
        }
    }

    return PhFormat(format, count, 64);
}

/**
 * Formats a sensor graph's Y axis label.
 *
 * \param Capability PH_HW_CAP_PACKAGE_TEMP or PH_HW_CAP_RAPL_PACKAGE.
 * \param Value The value at the label, in degrees Celsius or watts.
 * \return The label, empty for zero.
 */
PPH_STRING PhSipCpuFormatSensorGraphLabelY(
    _In_ ULONG Capability,
    _In_ FLOAT Value
    )
{
    PH_FORMAT format[PH_HW_TEMPERATURE_FORMAT_COUNT];

    if (Value == 0)
        return PhReferenceEmptyString();

    PhpSipCpuInitSensorFormat(Capability, Value, 0, format);
    return PhFormat(format, RTL_NUMBER_OF(format), 0);
}

/**
 * Formats the values of a sensor graph's tooltip: the aggregate, then each package when there are several, each
 * line ending in a line break; the caller appends the sample time.
 *
 * \param Capability PH_HW_CAP_PACKAGE_TEMP or PH_HW_CAP_RAPL_PACKAGE.
 * \param Index The sample, 0 the newest.
 */
PPH_STRING PhSipCpuFormatSensorGraphTooltipText(
    _In_ ULONG Capability,
    _In_ ULONG Index
    )
{
    PPH_CIRCULAR_BUFFER_FLOAT history;
    PH_STRING_BUILDER stringBuilder;
    PH_FORMAT format[4 + PH_HW_TEMPERATURE_FORMAT_COUNT];
    WCHAR formatBuffer[64];
    SIZE_T returnLength;
    ULONG packageCount;
    PPH_HW_PACKAGE_SENSOR package;
    FLOAT value;

    history = Capability == PH_HW_CAP_PACKAGE_TEMP ? &PhHwCpuTemperatureHistory : &PhHwCpuPowerHistory;
    PhInitializeStringBuilder(&stringBuilder, 64);

    // %.1f%s\n; a sample of 0 had no reading.
    value = PhGetItemCircularBuffer_FLOAT(history, Index);

    if (value != 0.0f)
    {
        PhpSipCpuInitSensorFormat(Capability, value, 1, format);
        PhInitFormatC(&format[PH_HW_TEMPERATURE_FORMAT_COUNT], L'\n');

        if (PhFormatToBuffer(format, PH_HW_TEMPERATURE_FORMAT_COUNT + 1, formatBuffer, sizeof(formatBuffer), &returnLength))
            PhAppendStringBuilderEx(&stringBuilder, formatBuffer, returnLength - sizeof(UNICODE_NULL));
    }

    packageCount = PhHwGetPackageCount();

    if (packageCount > 1)
    {
        for (ULONG i = 0; i < packageCount; i++)
        {
            PPH_CIRCULAR_BUFFER_FLOAT packageHistory;

            if (!(package = PhHwGetPackageSensor(i)))
                break;

            packageHistory = Capability == PH_HW_CAP_PACKAGE_TEMP ? &package->TemperatureHistory : &package->PackagePowerHistory;

            if (Index >= packageHistory->Count)
                continue;

            value = PhGetItemCircularBuffer_FLOAT(packageHistory, Index);

            if (value == 0.0f)
                continue;

            // Package %u: %.1f%s\n
            PhInitFormatS(&format[0], L"Package ");
            PhInitFormatU(&format[1], i);
            PhInitFormatS(&format[2], L": ");
            PhpSipCpuInitSensorFormat(Capability, value, 1, &format[3]);
            PhInitFormatC(&format[3 + PH_HW_TEMPERATURE_FORMAT_COUNT], L'\n');

            if (PhFormatToBuffer(format, RTL_NUMBER_OF(format), formatBuffer, sizeof(formatBuffer), &returnLength))
                PhAppendStringBuilderEx(&stringBuilder, formatBuffer, returnLength - sizeof(UNICODE_NULL));
        }
    }

    return PhFinalStringBuilderString(&stringBuilder);
}

/**
 * Determines the top of a sensor graph's scale: the highest temperature limit for temperatures, else the largest
 * sample in view, at least PH_SIP_CPU_POWER_GRAPH_MINIMUM watts.
 *
 * \param Capability PH_HW_CAP_PACKAGE_TEMP or PH_HW_CAP_RAPL_PACKAGE.
 * \param Data The samples in view.
 * \param Count The number of samples.
 */
FLOAT PhSipCpuSensorGraphScale(
    _In_ ULONG Capability,
    _In_reads_(Count) PCFLOAT Data,
    _In_ ULONG Count
    )
{
    FLOAT max;

    if (Capability == PH_HW_CAP_PACKAGE_TEMP)
        return PhHwGetCpuTemperatureLimit();

    max = PH_SIP_CPU_POWER_GRAPH_MINIMUM;

    for (ULONG i = 0; i < Count; i++)
    {
        if (max < Data[i])
            max = Data[i];
    }

    return max;
}
