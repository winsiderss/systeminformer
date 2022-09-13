#ifndef PH_SETTINGS_H
#define PH_SETTINGS_H

_User_set_
VOID
NTAPI
PhAddDefaultSettings(
    VOID
    );

_User_set_
VOID
NTAPI
PhUpdateCachedSettings(
    VOID
    );

// Cached settings

#undef EXT

#ifdef PH_SETTINGS_PRIVATE
#define EXT
#else
#define EXT extern
#endif

EXT BOOLEAN PhEnableProcessQueryStage2;
EXT BOOLEAN PhEnableServiceQueryStage2;
EXT BOOLEAN PhEnableThemeSupport;
EXT BOOLEAN PhEnableTooltipSupport;
EXT BOOLEAN PhEnableLinuxSubsystemSupport;
EXT BOOLEAN PhEnableNetworkResolveDoHSupport;
EXT BOOLEAN PhEnableVersionShortText;

EXT ULONG PhCsForceNoParent;
EXT ULONG PhCsHighlightingDuration;
EXT ULONG PhCsHideOtherUserProcesses;
EXT ULONG PhCsPropagateCpuUsage;
EXT ULONG PhCsScrollToNewProcesses;
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
EXT ULONG PhCsUseColorProtectedProcess;
EXT ULONG PhCsColorProtectedProcess;
EXT ULONG PhCsUseColorInheritHandles;
EXT ULONG PhCsColorInheritHandles;
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
EXT BOOLEAN PhCsEnableGraphMaxScale;
EXT BOOLEAN PhCsEnableGraphMaxText;

#define PH_SET_INTEGER_CACHED_SETTING(Name, Value) (PhSetIntegerSetting(TEXT(#Name), PhCs##Name = (Value)))

#endif
