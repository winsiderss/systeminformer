/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2010-2016
 *     dmex    2017-2023
 *
 */

#ifndef PH_SETTINGS_H
#define PH_SETTINGS_H

//
// Application Typedefs
//

EXTERN_C
VOID
NTAPI
PhAddDefaultSettings(
    VOID
    );

EXTERN_C
VOID
NTAPI
PhUpdateCachedSettings(
    VOID
    );

//
// Cached settings
//

#pragma push_macro("EXT")
#undef EXT

#ifdef PH_SETTINGS_PRIVATE
#define EXT
#else
#define EXT extern
#endif

EXT BOOLEAN PhEnableProcessQueryStage2;
EXT BOOLEAN PhEnableServiceQueryStage2;
EXT BOOLEAN PhEnableWindowText;
EXT BOOLEAN PhEnableTooltipSupport;
EXT BOOLEAN PhEnableLinuxSubsystemSupport;
EXT ULONG PhCsEnableNetworkResolveDoH;
EXT ULONG PhCsEnableVersionSupport;
EXT BOOLEAN PhEnableDeferredLayout;
EXT BOOLEAN PhEnableKsiWarnings;
EXT BOOLEAN PhEnableKsiSupport;

EXT ULONG PhCsForceNoParent;
EXT ULONG PhCsHighlightingDuration;
EXT ULONG PhCsPropagateCpuUsage;
EXT ULONG PhCsScrollToNewProcesses;
EXT ULONG PhCsScrollToRemovedProcesses;
EXT ULONG PhCsShowCpuBelow001;
EXT ULONG PhCsUpdateInterval;

EXT ULONG PhCsColorNew;
EXT ULONG PhCsColorRemoved;
EXT ULONG PhCsUseColorOwnProcesses;
EXT ULONG PhCsColorOwnProcesses;
EXT ULONG PhCsUseColorSystemProcesses;
EXT ULONG PhCsColorSystemProcesses;
EXT ULONG PhCsUseColorServiceProcesses;
EXT ULONG PhCsColorServiceProcesses;
EXT ULONG PhCsUseColorBackgroundProcesses;
EXT ULONG PhCsColorBackgroundProcesses;
EXT ULONG PhCsUseColorJobProcesses;
EXT ULONG PhCsColorJobProcesses;
EXT ULONG PhCsUseColorWow64Processes;
EXT ULONG PhCsColorWow64Processes;
EXT ULONG PhCsUseColorDebuggedProcesses;
EXT ULONG PhCsColorDebuggedProcesses;
EXT ULONG PhCsUseColorElevatedProcesses;
EXT ULONG PhCsColorElevatedProcesses;
EXT ULONG PhCsUseColorHandleFiltered;
EXT ULONG PhCsColorHandleFiltered;
EXT ULONG PhCsUseColorImmersiveProcesses;
EXT ULONG PhCsColorImmersiveProcesses;
EXT ULONG PhCsUseColorUIAccessProcesses;
EXT ULONG PhCsColorUIAccessProcesses;
EXT ULONG PhCsUseColorPicoProcesses;
EXT ULONG PhCsColorPicoProcesses;
EXT ULONG PhCsUseColorSuspended;
EXT ULONG PhCsColorSuspended;
EXT ULONG PhCsUseColorDotNet;
EXT ULONG PhCsColorDotNet;
EXT ULONG PhCsUseColorPacked;
EXT ULONG PhCsColorPacked;
EXT ULONG PhCsUseColorLowImageCoherency;
EXT ULONG PhCsColorLowImageCoherency;
EXT ULONG PhCsUseColorPartiallySuspended;
EXT ULONG PhCsColorPartiallySuspended;
EXT ULONG PhCsUseColorGuiThreads;
EXT ULONG PhCsColorGuiThreads;
EXT ULONG PhCsUseColorRelocatedModules;
EXT ULONG PhCsColorRelocatedModules;
EXT ULONG PhCsUseColorProtectedHandles;
EXT ULONG PhCsColorProtectedHandles;
EXT ULONG PhCsUseColorProtectedInheritHandles;
EXT ULONG PhCsColorProtectedInheritHandles;
EXT ULONG PhCsUseColorProtectedProcess;
EXT ULONG PhCsColorProtectedProcess;
EXT ULONG PhCsUseColorInheritHandles;
EXT ULONG PhCsColorInheritHandles;
EXT ULONG PhCsUseColorEfficiencyMode;
EXT ULONG PhCsColorEfficiencyMode;
EXT ULONG PhCsGraphShowText;
EXT ULONG PhCsGraphColorMode;
EXT ULONG PhCsColorCpuKernel;
EXT ULONG PhCsColorCpuUser;
EXT ULONG PhCsColorIoReadOther;
EXT ULONG PhCsColorIoWrite;
EXT ULONG PhCsColorPrivate;
EXT ULONG PhCsColorPhysical;
EXT ULONG PhCsColorPowerUsage;
EXT ULONG PhCsColorTemperature;
EXT ULONG PhCsColorFanRpm;

EXT ULONG PhCsUseColorUnknown;
EXT ULONG PhCsColorUnknown;
EXT ULONG PhCsUseColorServiceDisabled;
EXT ULONG PhCsColorServiceDisabled;
EXT ULONG PhCsUseColorServiceStop;
EXT ULONG PhCsColorServiceStop;

EXT BOOLEAN PhEnableImageCoherencySupport;
EXT ULONG PhCsImageCoherencyScanLevel;
EXT ULONG PhCsEnableGraphMaxScale;
EXT ULONG PhCsEnableGraphMaxText;
EXT ULONG PhCsEnableAvxSupport;
EXT ULONG PhCsEnableHandleSnapshot;

#pragma pop_macro("EXT")

#define PH_GET_INTEGER_CACHED_SETTING(Name) ((PhCs##Name) = PhGetIntegerSetting(TEXT(#Name)))
#define PH_SET_INTEGER_CACHED_SETTING(Name, Value) (PhSetIntegerSetting(TEXT(#Name), (PhCs##Name) = (Value)))

#endif
