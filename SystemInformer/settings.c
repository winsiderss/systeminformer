/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2010-2016
 *     dmex    2017-2022
 *     jxy-s   2021
 *
 */

#include <phapp.h>
#include <settings.h>
#define PH_SETTINGS_PRIVATE
#include <phsettings.h>

#define PH_UPDATE_SETTING(Name) \
    (PhCs##Name = PhGetIntegerSetting(TEXT(#Name)))

VOID PhAddDefaultSettings(
    VOID
    )
{
    PhpAddIntegerSetting(L"AllowOnlyOneInstance", L"1");
    PhpAddIntegerSetting(L"CloseOnEscape", L"0");
    PhpAddStringSetting(L"DbgHelpSearchPath", L"SRV*C:\\Symbols*https://msdl.microsoft.com/download/symbols");
    PhpAddIntegerSetting(L"DbgHelpUndecorate", L"1");
    PhpAddStringSetting(L"DisabledPlugins", L"");
    PhpAddIntegerSetting(L"ElevationLevel", L"1"); // PromptElevateAction
    PhpAddIntegerSetting(L"EnableAdvancedOptions", L"0");
    PhpAddIntegerSetting(L"EnableBitmapSupport", L"1");
    PhpAddIntegerSetting(L"EnableBreakOnTermination", L"0");
    PhpAddIntegerSetting(L"EnableBootObjectsEnumerate", L"0");
    PhpAddIntegerSetting(L"EnableCycleCpuUsage", L"1");
    PhpAddIntegerSetting(L"EnableArmCycleCpuUsage", L"0");
    PhpAddIntegerSetting(L"EnableImageCoherencySupport", L"0");
    PhpAddIntegerSetting(L"EnableInstantTooltips", L"0");
    PhpAddIntegerSetting(L"EnableHeapReflection", L"0");
    PhpAddIntegerSetting(L"EnableHeapMemoryTagging", L"0");
    PhpAddIntegerSetting(L"EnableKph", L"0");
    PhpAddIntegerSetting(L"EnableKphWarnings", L"1");
    PhpAddIntegerSetting(L"EnableLastProcessShutdown", L"0");
    PhpAddIntegerSetting(L"EnableLinuxSubsystemSupport", L"0");
    PhpAddIntegerSetting(L"EnableHandleSnapshot", L"1");
    PhpAddIntegerSetting(L"EnableMinidumpKernelMinidump", L"0");
    PhpAddIntegerSetting(L"EnableMonospaceFont", L"0");
    PhpAddIntegerSetting(L"EnableNetworkBoundConnections", L"1");
    PhpAddIntegerSetting(L"EnableNetworkResolve", L"1");
    PhpAddIntegerSetting(L"EnableNetworkResolveDoH", L"0");
    PhpAddIntegerSetting(L"EnablePackageIconSupport", L"0");
    PhpAddIntegerSetting(L"EnablePlugins", L"1");
    PhpAddIntegerSetting(L"EnableGraphMaxScale", L"0");
    PhpAddIntegerSetting(L"EnableGraphMaxText", L"1");
    PhpAddIntegerSetting(L"EnableServiceNonPoll", L"0");
    PhpAddIntegerSetting(L"EnableShellExecuteSkipIfeoDebugger", L"1");
    PhpAddIntegerSetting(L"EnableStage2", L"1");
    PhpAddIntegerSetting(L"EnableStreamerMode", L"0");
    PhpAddIntegerSetting(L"EnableServiceStage2", L"0");
    PhpAddIntegerSetting(L"EnableStartAsAdmin", L"0");
    PhpAddIntegerSetting(L"EnableDefaultSafePlugins", L"1");
    PhpAddIntegerSetting(L"EnableSecurityAdvancedDialog", L"1");
    PhpAddIntegerSetting(L"EnableShortRelativeStartTime", L"1");
    PhpAddIntegerSetting(L"EnableShutdownCriticalMenu", L"0");
    PhpAddIntegerSetting(L"EnableShutdownBootMenu", L"1");
    PhpAddIntegerSetting(L"EnableSilentCrashNotify", L"0");
    PhpAddIntegerSetting(L"EnableThemeSupport", L"0");
    PhpAddIntegerSetting(L"EnableThemeAcrylicSupport", L"1");
    PhpAddIntegerSetting(L"EnableThemeAcrylicWindowSupport", L"0");
    PhpAddIntegerSetting(L"EnableThreadStackInlineSymbols", L"1");
    PhpAddIntegerSetting(L"EnableThreadStackLineInformation", L"1");
    PhpAddIntegerSetting(L"EnableTooltipSupport", L"1");
    PhpAddIntegerSetting(L"EnableUpdateDefaultFirmwareBootEntry", L"1");
    PhpAddIntegerSetting(L"EnableVersionSupport", L"1");
    PhpAddIntegerSetting(L"EnableWarnings", L"1");
    PhpAddIntegerSetting(L"EnableWindowText", L"1");
    PhpAddStringSetting(L"EnvironmentTreeListColumns", L"");
    PhpAddStringSetting(L"EnvironmentTreeListSort", L"0,0"); // 0, NoSortOrder
    PhpAddIntegerSetting(L"EnvironmentTreeListFlags", L"0");
    PhpAddIntegerSetting(L"FindObjRegex", L"0");
    PhpAddStringSetting(L"FindObjTreeListColumns", L"");
    PhpAddIntegerPairSetting(L"FindObjWindowPosition", L"0,0");
    PhpAddScalableIntegerPairSetting(L"FindObjWindowSize", L"@96|550,420");
    PhpAddStringSetting(L"FileBrowseExecutable", L"%SystemRoot%\\explorer.exe /select,\"%s\"");
    PhpAddIntegerSetting(L"FirstRun", L"1");
    PhpAddStringSetting(L"Font", L""); // null
    PhpAddStringSetting(L"FontMonospace", L""); // null
    PhpAddIntegerSetting(L"ForceNoParent", L"1");
    PhpAddStringSetting(L"HandleTreeListColumns", L"");
    PhpAddStringSetting(L"HandleTreeListSort", L"0,1"); // 0, AscendingSortOrder
    PhpAddIntegerSetting(L"HandleTreeListFlags", L"3");
    PhpAddIntegerPairSetting(L"HandlePropertiesWindowPosition", L"0,0");
    PhpAddScalableIntegerPairSetting(L"HandlePropertiesWindowSize", L"@96|260,260");
    PhpAddStringSetting(L"HandleStatisticsListViewColumns", L"");
    PhpAddStringSetting(L"HandleStatisticsListViewSort", L"0,1");
    PhpAddScalableIntegerPairSetting(L"HandleStatisticsWindowSize", L"@96|0,0");
    PhpAddIntegerSetting(L"HiddenProcessesMenuEnabled", L"0");
    PhpAddStringSetting(L"HiddenProcessesListViewColumns", L"");
    PhpAddIntegerPairSetting(L"HiddenProcessesWindowPosition", L"400,400");
    PhpAddScalableIntegerPairSetting(L"HiddenProcessesWindowSize", L"@96|520,400");
    PhpAddIntegerSetting(L"HideDefaultServices", L"0");
    PhpAddIntegerSetting(L"HideDriverServices", L"0");
    PhpAddIntegerSetting(L"HideFreeRegions", L"1");
    PhpAddIntegerSetting(L"HideOnClose", L"0");
    PhpAddIntegerSetting(L"HideOnMinimize", L"0");
    PhpAddIntegerSetting(L"HideOtherUserProcesses", L"0");
    PhpAddIntegerSetting(L"HideSignedProcesses", L"0");
    PhpAddIntegerSetting(L"HideWaitingConnections", L"0");
    PhpAddIntegerSetting(L"HighlightingDuration", L"3e8"); // 1000ms
    PhpAddStringSetting(L"IconTrayGuids", L"");
    PhpAddIntegerSetting(L"IconTrayPersistGuidEnabled", L"0");
    PhpAddIntegerSetting(L"IconIgnoreBalloonClick", L"0");
    PhpAddStringSetting(L"IconSettings", L"2|1");
    PhpAddIntegerSetting(L"IconNotifyMask", L"c"); // PH_NOTIFY_SERVICE_CREATE | PH_NOTIFY_SERVICE_DELETE
    PhpAddIntegerSetting(L"IconProcesses", L"f"); // 15
    PhpAddIntegerSetting(L"IconSingleClick", L"0");
    PhpAddIntegerSetting(L"IconTogglesVisibility", L"1");
    PhpAddIntegerSetting(L"IconTransparencyEnabled", L"0");
    //PhpAddIntegerSetting(L"IconTransparency", L"255");
    PhpAddIntegerPairSetting(L"InformationWindowPosition", L"0,0");
    PhpAddScalableIntegerPairSetting(L"InformationWindowSize", L"@96|140,190");
    PhpAddIntegerSetting(L"ImageCoherencyScanLevel", L"1");
    PhpAddStringSetting(L"JobListViewColumns", L"");
    PhpAddIntegerSetting(L"LogEntries", L"200"); // 512
    PhpAddStringSetting(L"LogListViewColumns", L"");
    PhpAddIntegerPairSetting(L"LogWindowPosition", L"0,0");
    PhpAddScalableIntegerPairSetting(L"LogWindowSize", L"@96|450,500");
    PhpAddIntegerSetting(L"MainWindowAlwaysOnTop", L"0");
    PhpAddStringSetting(L"MainWindowClassName", L"MainWindowClassName");
    PhpAddIntegerSetting(L"MainWindowOpacity", L"0"); // means 100%
    PhpAddIntegerPairSetting(L"MainWindowPosition", L"100,100");
    PhpAddScalableIntegerPairSetting(L"MainWindowSize", L"@96|800,600");
    PhpAddIntegerSetting(L"MainWindowState", L"1");
    PhpAddIntegerSetting(L"MainWindowTabRestoreEnabled", L"0");
    PhpAddIntegerSetting(L"MainWindowTabRestoreIndex", L"0");
    PhpAddIntegerSetting(L"MaxSizeUnit", L"6");
    PhpAddIntegerSetting(L"MaxPrecisionUnit", L"2");
    PhpAddIntegerSetting(L"MemEditBytesPerRow", L"10"); // 16
    PhpAddStringSetting(L"MemEditGotoChoices", L"");
    PhpAddIntegerPairSetting(L"MemEditPosition", L"0,0");
    PhpAddScalableIntegerPairSetting(L"MemEditSize", L"@96|600,500");
    PhpAddStringSetting(L"MemFilterChoices", L"");
    PhpAddStringSetting(L"MemResultsListViewColumns", L"");
    PhpAddIntegerPairSetting(L"MemResultsPosition", L"300,300");
    PhpAddScalableIntegerPairSetting(L"MemResultsSize", L"@96|500,520");
    PhpAddIntegerSetting(L"MemoryListFlags", L"3");
    PhpAddStringSetting(L"MemoryTreeListColumns", L"");
    PhpAddStringSetting(L"MemoryTreeListSort", L"0,0"); // 0, NoSortOrder
    PhpAddIntegerPairSetting(L"MemoryListsWindowPosition", L"400,400");
    PhpAddStringSetting(L"MemoryReadWriteAddressChoices", L"");
    PhpAddStringSetting(L"MiniInfoWindowClassName", L"MiniInfoWindowClassName");
    PhpAddIntegerSetting(L"MiniInfoWindowEnabled", L"1");
    PhpAddIntegerSetting(L"MiniInfoWindowOpacity", L"0"); // means 100%
    PhpAddIntegerSetting(L"MiniInfoWindowPinned", L"0");
    PhpAddIntegerPairSetting(L"MiniInfoWindowPosition", L"200,200");
    PhpAddIntegerSetting(L"MiniInfoWindowRefreshAutomatically", L"1");
    PhpAddScalableIntegerPairSetting(L"MiniInfoWindowSize", L"@96|10,200");
    PhpAddIntegerSetting(L"ModuleTreeListFlags", L"1");
    PhpAddStringSetting(L"ModuleTreeListColumns", L"");
    PhpAddStringSetting(L"ModuleTreeListSort", L"0,0"); // 0, NoSortOrder
    PhpAddStringSetting(L"NetworkTreeListColumns", L"");
    PhpAddStringSetting(L"NetworkTreeListSort", L"0,1"); // 0, AscendingSortOrder
    PhpAddIntegerSetting(L"NoPurgeProcessRecords", L"0");
    PhpAddStringSetting(L"OptionsCustomColorList", L"");
    PhpAddStringSetting(L"OptionsWindowAdvancedColumns", L"");
    PhpAddIntegerSetting(L"OptionsWindowAdvancedFlags", L"0");
    PhpAddIntegerPairSetting(L"OptionsWindowPosition", L"0,0");
    PhpAddScalableIntegerPairSetting(L"OptionsWindowSize", L"@96|900,590");
    PhpAddIntegerPairSetting(L"PageFileWindowPosition", L"0,0");
    PhpAddScalableIntegerPairSetting(L"PageFileWindowSize", L"@96|500,300");
    PhpAddStringSetting(L"PageFileListViewColumns", L"");
    PhpAddStringSetting(L"PluginManagerTreeListColumns", L"");
    PhpAddStringSetting(L"ProcessServiceListViewColumns", L"");
    PhpAddStringSetting(L"ProcessTreeColumnSetConfig", L"");
    PhpAddStringSetting(L"ProcessTreeListColumns", L"");
    PhpAddStringSetting(L"ProcessTreeListSort", L"0,0"); // 0, NoSortOrder
    PhpAddIntegerSetting(L"ProcessTreeListNameDefault", L"1");
    PhpAddStringSetting(L"ProcPropPage", L"General");
    PhpAddIntegerPairSetting(L"ProcPropPosition", L"200,200");
    PhpAddScalableIntegerPairSetting(L"ProcPropSize", L"@96|460,580");
    PhpAddStringSetting(L"ProcStatPropPageGroupListViewColumns", L"");
    PhpAddStringSetting(L"ProcStatPropPageGroupListViewSort", L"0,0");
    PhpAddStringSetting(L"ProcStatPropPageGroupStates", L"");
    PhpAddStringSetting(L"ProgramInspectExecutables", L"peview.exe \"%s\"");
    PhpAddIntegerSetting(L"PropagateCpuUsage", L"0");
    PhpAddIntegerSetting(L"RunAsEnableAutoComplete", L"0");
    PhpAddStringSetting(L"RunAsProgram", L"");
    PhpAddStringSetting(L"RunAsUserName", L"");
    PhpAddIntegerPairSetting(L"RunAsWindowPosition", L"0,0");
    PhpAddIntegerSetting(L"RunFileDlgState", L"0");
    PhpAddIntegerSetting(L"SampleCount", L"200"); // 512
    PhpAddIntegerSetting(L"SampleCountAutomatic", L"1");
    PhpAddIntegerSetting(L"ScrollToNewProcesses", L"0");
    PhpAddIntegerSetting(L"ScrollToRemovedProcesses", L"0");
    PhpAddStringSetting(L"SearchEngine", L"https://duckduckgo.com/?q=\"%s\"");
    PhpAddStringSetting(L"SegmentHeapListViewColumns", L"");
    PhpAddStringSetting(L"SegmentHeapListViewSort", L"0,1");
    PhpAddIntegerPairSetting(L"SegmentHeapWindowPosition", L"0,0");
    PhpAddScalableIntegerPairSetting(L"SegmentHeapWindowSize", L"@96|450,500");
    PhpAddIntegerPairSetting(L"ServiceWindowPosition", L"0,0");
    PhpAddStringSetting(L"ServiceListViewColumns", L"");
    PhpAddStringSetting(L"ServiceTreeListColumns", L"");
    PhpAddStringSetting(L"ServiceTreeListSort", L"0,1"); // 0, AscendingSortOrder
    PhpAddIntegerPairSetting(L"SessionShadowHotkey", L"106,2"); // VK_MULTIPLY,KBDCTRL
    PhpAddIntegerSetting(L"ShowPluginLoadErrors", L"1");
    PhpAddIntegerSetting(L"ShowCommitInSummary", L"1");
    PhpAddIntegerSetting(L"ShowCpuBelow001", L"0");
    PhpAddIntegerSetting(L"ShowHexId", L"0");
    PhpAddIntegerSetting(L"StartHidden", L"0");
    PhpAddIntegerSetting(L"SysInfoWindowAlwaysOnTop", L"0");
    PhpAddIntegerSetting(L"SysInfoWindowOneGraphPerCpu", L"0");
    PhpAddIntegerPairSetting(L"SysInfoWindowPosition", L"200,200");
    PhpAddStringSetting(L"SysInfoWindowSection", L"");
    PhpAddScalableIntegerPairSetting(L"SysInfoWindowSize", L"@96|620,590");
    PhpAddIntegerSetting(L"SysInfoWindowState", L"1");
    PhpAddIntegerSetting(L"ThinRows", L"0");
    PhpAddStringSetting(L"ThreadTreeListColumns", L"");
    PhpAddStringSetting(L"ThreadTreeListSort", L"1,2"); // 1, DescendingSortOrder
    PhpAddIntegerSetting(L"ThreadTreeListFlags", L"0");
    PhpAddStringSetting(L"ThreadStackTreeListColumns", L"");
    PhpAddScalableIntegerPairSetting(L"ThreadStackWindowSize", L"@96|420,400");
    PhpAddStringSetting(L"TokenGroupsListViewColumns", L"");
    PhpAddStringSetting(L"TokenGroupsListViewStates", L"");
    PhpAddStringSetting(L"TokenGroupsListViewSort", L"1,2");
    PhpAddStringSetting(L"TokenPrivilegesListViewColumns", L"");
    PhpAddIntegerSetting(L"TreeListBorderEnable", L"0");
    PhpAddIntegerSetting(L"TreeListCustomColorsEnable", L"0");
    PhpAddIntegerSetting(L"TreeListCustomColorText", L"0");
    PhpAddIntegerSetting(L"TreeListCustomColorFocus", L"0");
    PhpAddIntegerSetting(L"TreeListCustomColorSelection", L"0");
    PhpAddIntegerSetting(L"TreeListCustomRowSize", L"0");
    PhpAddIntegerSetting(L"TreeListEnableHeaderTotals", L"1");
    PhpAddIntegerSetting(L"UpdateInterval", L"3e8"); // 1000ms
    PhpAddIntegerSetting(L"WmiProviderEnableHiddenMenu", L"0");
    PhpAddStringSetting(L"WmiProviderTreeListColumns", L"");
    PhpAddStringSetting(L"WmiProviderTreeListSort", L"0,0"); // 0, NoSortOrder
    PhpAddIntegerSetting(L"WmiProviderTreeListFlags", L"0");
    PhpAddStringSetting(L"VdmHostListViewColumns", L"");

    // Colors are specified with R in the lowest byte, then G, then B. So: bbggrr.
    PhpAddIntegerSetting(L"ColorNew", L"00ff7f"); // Chartreuse
    PhpAddIntegerSetting(L"ColorRemoved", L"283cff");
    PhpAddIntegerSetting(L"UseColorOwnProcesses", L"1");
    PhpAddIntegerSetting(L"ColorOwnProcesses", L"aaffff");
    PhpAddIntegerSetting(L"UseColorSystemProcesses", L"1");
    PhpAddIntegerSetting(L"ColorSystemProcesses", L"ffccaa");
    PhpAddIntegerSetting(L"UseColorServiceProcesses", L"1");
    PhpAddIntegerSetting(L"ColorServiceProcesses", L"ffffcc");
    PhpAddIntegerSetting(L"UseColorBackgroundProcesses", L"0");
    PhpAddIntegerSetting(L"ColorBackgroundProcesses", L"bcbc78");
    PhpAddIntegerSetting(L"UseColorJobProcesses", L"0");
    PhpAddIntegerSetting(L"ColorJobProcesses", L"3f85cd"); // Peru
    PhpAddIntegerSetting(L"UseColorWow64Processes", L"0");
    PhpAddIntegerSetting(L"ColorWow64Processes", L"8f8fbc"); // Rosy Brown
    PhpAddIntegerSetting(L"UseColorDebuggedProcesses", L"1");
    PhpAddIntegerSetting(L"ColorDebuggedProcesses", L"ffbbcc");
    PhpAddIntegerSetting(L"UseColorElevatedProcesses", L"1");
    PhpAddIntegerSetting(L"ColorElevatedProcesses", L"00aaff");
    PhpAddIntegerSetting(L"UseColorHandleFiltered", L"1");
    PhpAddIntegerSetting(L"ColorHandleFiltered", L"000000"); // Black
    PhpAddIntegerSetting(L"UseColorImmersiveProcesses", L"1");
    PhpAddIntegerSetting(L"ColorImmersiveProcesses", L"cbc0ff"); // Pink
    PhpAddIntegerSetting(L"UseColorPicoProcesses", L"1");
    PhpAddIntegerSetting(L"ColorPicoProcesses", L"ff8000"); // Blue
    PhpAddIntegerSetting(L"UseColorSuspended", L"1");
    PhpAddIntegerSetting(L"ColorSuspended", L"777777");
    PhpAddIntegerSetting(L"UseColorDotNet", L"1");
    PhpAddIntegerSetting(L"ColorDotNet", L"00ffde");
    PhpAddIntegerSetting(L"UseColorPacked", L"1");
    PhpAddIntegerSetting(L"ColorPacked", L"9314ff"); // Deep Pink
    PhpAddIntegerSetting(L"UseColorLowImageCoherency", L"1");
    PhpAddIntegerSetting(L"ColorLowImageCoherency", L"ff14b9"); // Deep Purple
    PhpAddIntegerSetting(L"UseColorPartiallySuspended", L"0");
    PhpAddIntegerSetting(L"ColorPartiallySuspended", L"c0c0c0");
    PhpAddIntegerSetting(L"UseColorGuiThreads", L"1");
    PhpAddIntegerSetting(L"ColorGuiThreads", L"77ffff");
    PhpAddIntegerSetting(L"UseColorRelocatedModules", L"1");
    PhpAddIntegerSetting(L"ColorRelocatedModules", L"80c0ff");
    PhpAddIntegerSetting(L"UseColorProtectedHandles", L"1");
    PhpAddIntegerSetting(L"ColorProtectedHandles", L"777777");
    PhpAddIntegerSetting(L"UseColorProtectedProcess", L"0");
    PhpAddIntegerSetting(L"ColorProtectedProcess", L"ff8080");
    PhpAddIntegerSetting(L"UseColorInheritHandles", L"1");
    PhpAddIntegerSetting(L"ColorInheritHandles", L"ffff77");
    PhpAddIntegerSetting(L"UseColorServiceDisabled", L"1");
    PhpAddIntegerSetting(L"ColorServiceDisabled", L"6d6d6d"); // Dark grey
    PhpAddIntegerSetting(L"UseColorServiceStop", L"1");
    PhpAddIntegerSetting(L"ColorServiceStop", L"6d6d6d"); // Dark grey
    PhpAddIntegerSetting(L"UseColorUnknown", L"1");
    PhpAddIntegerSetting(L"ColorUnknown", L"8080ff"); // Light Red

    PhpAddIntegerSetting(L"UseColorSystemThreadStack", L"1");
    PhpAddIntegerSetting(L"ColorSystemThreadStack", L"ffccaa");
    PhpAddIntegerSetting(L"UseColorUserThreadStack", L"1");
    PhpAddIntegerSetting(L"ColorUserThreadStack", L"aaffff");
    PhpAddIntegerSetting(L"UseColorInlineThreadStack", L"1");
    PhpAddIntegerSetting(L"ColorInlineThreadStack", L"00ffde");

    PhpAddIntegerSetting(L"GraphShowText", L"1");
    PhpAddIntegerSetting(L"GraphColorMode", L"0");
    PhpAddIntegerSetting(L"ColorCpuKernel", L"00ff00");
    PhpAddIntegerSetting(L"ColorCpuUser", L"0000ff");
    PhpAddIntegerSetting(L"ColorIoReadOther", L"00ffff");
    PhpAddIntegerSetting(L"ColorIoWrite", L"ff0077");
    PhpAddIntegerSetting(L"ColorPrivate", L"0077ff");
    PhpAddIntegerSetting(L"ColorPhysical", L"ff8000"); // Blue

    PhpAddIntegerSetting(L"ColorPowerUsage", L"00ff00");
    PhpAddIntegerSetting(L"ColorTemperature", L"0000ff");
    PhpAddIntegerSetting(L"ColorFanRpm", L"ff0077");

    PhpAddStringSetting(L"KphServiceName", L"");
    PhpAddStringSetting(L"KphObjectName", L"");
    PhpAddStringSetting(L"KphPortName", L"");
    PhpAddStringSetting(L"KphAltitude", L"385400");
    PhpAddIntegerSetting(L"KphDisableImageLoadProtection", L"0");
    PhpAddIntegerSetting(L"KsiEnableSplashScreen", L"0");
    PhpAddIntegerSetting(L"KsiEnableLoadNative", L"0");
    PhpAddIntegerSetting(L"KsiEnableLoadFilter", L"0");
    PhpAddIntegerSetting(L"KsiUnloadOnExitTest", L"0");
}

VOID PhUpdateCachedSettings(
    VOID
    )
{
    PH_UPDATE_SETTING(ForceNoParent);
    PH_UPDATE_SETTING(HighlightingDuration);
    PH_UPDATE_SETTING(HideOtherUserProcesses);
    PH_UPDATE_SETTING(PropagateCpuUsage);
    PH_UPDATE_SETTING(ScrollToNewProcesses);
    PH_UPDATE_SETTING(ScrollToRemovedProcesses);
    PH_UPDATE_SETTING(ShowCpuBelow001);
    PH_UPDATE_SETTING(UpdateInterval);

    PH_UPDATE_SETTING(ColorNew);
    PH_UPDATE_SETTING(ColorRemoved);
    PH_UPDATE_SETTING(UseColorOwnProcesses);
    PH_UPDATE_SETTING(ColorOwnProcesses);
    PH_UPDATE_SETTING(UseColorSystemProcesses);
    PH_UPDATE_SETTING(ColorSystemProcesses);
    PH_UPDATE_SETTING(UseColorServiceProcesses);
    PH_UPDATE_SETTING(ColorServiceProcesses);
    PH_UPDATE_SETTING(UseColorBackgroundProcesses);
    PH_UPDATE_SETTING(ColorBackgroundProcesses);
    PH_UPDATE_SETTING(UseColorJobProcesses);
    PH_UPDATE_SETTING(ColorJobProcesses);
    PH_UPDATE_SETTING(UseColorWow64Processes);
    PH_UPDATE_SETTING(ColorWow64Processes);
    PH_UPDATE_SETTING(UseColorDebuggedProcesses);
    PH_UPDATE_SETTING(ColorDebuggedProcesses);
    PH_UPDATE_SETTING(UseColorHandleFiltered);
    PH_UPDATE_SETTING(ColorHandleFiltered);
    PH_UPDATE_SETTING(UseColorElevatedProcesses);
    PH_UPDATE_SETTING(ColorElevatedProcesses);
    PH_UPDATE_SETTING(UseColorPicoProcesses);
    PH_UPDATE_SETTING(ColorPicoProcesses);
    PH_UPDATE_SETTING(UseColorImmersiveProcesses);
    PH_UPDATE_SETTING(ColorImmersiveProcesses);
    PH_UPDATE_SETTING(UseColorSuspended);
    PH_UPDATE_SETTING(ColorSuspended);
    PH_UPDATE_SETTING(UseColorDotNet);
    PH_UPDATE_SETTING(ColorDotNet);
    PH_UPDATE_SETTING(UseColorPacked);
    PH_UPDATE_SETTING(ColorPacked);
    PH_UPDATE_SETTING(UseColorLowImageCoherency);
    PH_UPDATE_SETTING(ColorLowImageCoherency);
    PH_UPDATE_SETTING(UseColorPartiallySuspended);
    PH_UPDATE_SETTING(ColorPartiallySuspended);
    PH_UPDATE_SETTING(UseColorGuiThreads);
    PH_UPDATE_SETTING(ColorGuiThreads);
    PH_UPDATE_SETTING(UseColorRelocatedModules);
    PH_UPDATE_SETTING(ColorRelocatedModules);
    PH_UPDATE_SETTING(UseColorProtectedHandles);
    PH_UPDATE_SETTING(ColorProtectedHandles);
    PH_UPDATE_SETTING(UseColorInheritHandles);
    PH_UPDATE_SETTING(ColorProtectedProcess);
    PH_UPDATE_SETTING(UseColorProtectedProcess);
    PH_UPDATE_SETTING(ColorInheritHandles);
    PH_UPDATE_SETTING(UseColorServiceDisabled);
    PH_UPDATE_SETTING(ColorServiceDisabled);
    PH_UPDATE_SETTING(UseColorServiceStop);
    PH_UPDATE_SETTING(ColorServiceStop);
    PH_UPDATE_SETTING(UseColorUnknown);
    PH_UPDATE_SETTING(ColorUnknown);

    PH_UPDATE_SETTING(GraphShowText);
    PH_UPDATE_SETTING(GraphColorMode);
    PH_UPDATE_SETTING(ColorCpuKernel);
    PH_UPDATE_SETTING(ColorCpuUser);
    PH_UPDATE_SETTING(ColorIoReadOther);
    PH_UPDATE_SETTING(ColorIoWrite);
    PH_UPDATE_SETTING(ColorPrivate);
    PH_UPDATE_SETTING(ColorPhysical);

    PH_UPDATE_SETTING(ColorPowerUsage);
    PH_UPDATE_SETTING(ColorTemperature);
    PH_UPDATE_SETTING(ColorFanRpm);

    PH_UPDATE_SETTING(ImageCoherencyScanLevel);

    PhCsEnableGraphMaxScale = !!PhGetIntegerSetting(L"EnableGraphMaxScale");
    PhCsEnableGraphMaxText = !!PhGetIntegerSetting(L"EnableGraphMaxText");
    PhEnableNetworkResolveDoHSupport = !!PhGetIntegerSetting(L"EnableNetworkResolveDoH");
    PhEnableVersionShortText = !!PhGetIntegerSetting(L"EnableVersionSupport");
}
