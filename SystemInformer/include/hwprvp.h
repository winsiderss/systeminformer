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

#ifndef PH_HWPRVP_H
#define PH_HWPRVP_H

#include <phapp.h>
#include <hwprv.h>

extern ULONG PhpHwProcessorCount;
extern ULONG PhpHwPackageCount;
extern PPH_HW_CPU_SENSOR PhpHwCpuSensors;
extern PPH_HW_PACKAGE_SENSOR PhpHwPackageSensors;

extern BOOLEAN PhpHwGateOpen;
extern BOOLEAN PhpHwProbed;
extern BOOLEAN PhpHwAvailable;
extern ULONG PhpHwCapabilities;

extern BOOLEAN PhpHwChipsetTemperatureValid;
extern FLOAT PhpHwChipsetTemperature;

extern PH_QUEUED_LOCK PhpHwNamedSensorLock;
extern PPH_HW_NAMED_SENSOR PhpHwNamedSensors;
extern ULONG PhpHwNamedSensorCount;

#endif
