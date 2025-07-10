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
EXT ULONG PhCsSortChildProcesses;
EXT ULONG PhCsSortRootProcesses;
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

EXT BOOLEAN PhEnableProcessMonitor;
EXT ULONG PhProcessMonitorLookback;
EXT ULONG PhProcessMonitorCacheLimit;

#pragma pop_macro("EXT")

#define PH_GET_INTEGER_CACHED_SETTING(Name) ((PhCs##Name) = PhGetIntegerSetting(TEXT(#Name)))
#define PH_SET_INTEGER_CACHED_SETTING(Name, Value) (PhSetIntegerSetting(TEXT(#Name), (PhCs##Name) = (Value)))

#define SETTING_ALLOW_ONLY_ONE_INSTANCE                             L"AllowOnlyOneInstance"
#define SETTING_CLOSE_ON_ESCAPE                                     L"CloseOnEscape"
#define SETTING_DBGHELP_SEARCH_PATH                                 L"DbgHelpSearchPath"
#define SETTING_DBGHELP_UNDECORATE                                  L"DbgHelpUndecorate"
#define SETTING_DISABLED_PLUGINS                                    L"DisabledPlugins"
#define SETTING_ELEVATION_LEVEL                                     L"ElevationLevel"
#define SETTING_ENABLE_ADVANCED_OPTIONS                             L"EnableAdvancedOptions"
#define SETTING_ENABLE_AVX_SUPPORT                                  L"EnableAvxSupport"
#define SETTING_ENABLE_BITMAP_SUPPORT                               L"EnableBitmapSupport"
#define SETTING_ENABLE_BREAK_ON_TERMINATION                         L"EnableBreakOnTermination"
#define SETTING_ENABLE_BOOT_OBJECTS_ENUMERATE                       L"EnableBootObjectsEnumerate"
#define SETTING_ENABLE_COMMAND_LINE_TOOLTIPS                        L"EnableCommandLineTooltips"
#define SETTING_ENABLE_CYCLE_CPU_USAGE                              L"EnableCycleCpuUsage"
#define SETTING_ENABLE_DEFERRED_LAYOUT                              L"EnableDeferredLayout"
#define SETTING_ENABLE_DEVICE_SUPPORT                               L"EnableDeviceSupport"
#define SETTING_ENABLE_DEVICE_NOTIFY_SUPPORT                        L"EnableDeviceNotifySupport"
#define SETTING_ENABLE_IMAGE_COHERENCY_SUPPORT                      L"EnableImageCoherencySupport"
#define SETTING_ENABLE_INSTANT_TOOLTIPS                             L"EnableInstantTooltips"
#define SETTING_ENABLE_HEAP_REFLECTION                              L"EnableHeapReflection"
#define SETTING_ENABLE_HEAP_MEMORY_TAGGING                          L"EnableHeapMemoryTagging"
#define SETTING_ENABLE_LAST_PROCESS_SHUTDOWN                        L"EnableLastProcessShutdown"
#define SETTING_ENABLE_LINUX_SUBSYSTEM_SUPPORT                      L"EnableLinuxSubsystemSupport"
#define SETTING_ENABLE_HANDLE_SNAPSHOT                              L"EnableHandleSnapshot"
#define SETTING_ENABLE_MINIDUMP_KERNEL_MINIDUMP                     L"EnableMinidumpKernelMinidump"
#define SETTING_ENABLE_MINIDUMP_SNAPSHOT                            L"EnableMinidumpSnapshot"
#define SETTING_ENABLE_MONOSPACE_FONT                               L"EnableMonospaceFont"
#define SETTING_ENABLE_NETWORK_BOUND_CONNECTIONS                    L"EnableNetworkBoundConnections"
#define SETTING_ENABLE_NETWORK_RESOLVE                              L"EnableNetworkResolve"
#define SETTING_ENABLE_NETWORK_RESOLVE_DOH                          L"EnableNetworkResolveDoH"
#define SETTING_ENABLE_MEM_STRINGS_TREE_DIALOG                      L"EnableMemStringsTreeDialog"
#define SETTING_ENABLE_PACKAGE_ICON_SUPPORT                         L"EnablePackageIconSupport"
#define SETTING_ENABLE_PROCESS_HANDLE_PNP_DEVICE_NAME_SUPPORT       L"EnableProcessHandlePnPDeviceNameSupport"
#define SETTING_ENABLE_PLUGINS                                      L"EnablePlugins"
#define SETTING_ENABLE_PLUGINS_NATIVE                               L"EnablePluginsNative"
#define SETTING_ENABLE_GRAPH_MAX_SCALE                              L"EnableGraphMaxScale"
#define SETTING_ENABLE_GRAPH_MAX_TEXT                               L"EnableGraphMaxText"
#define SETTING_ENABLE_SERVICE_NON_POLL                             L"EnableServiceNonPoll"
#define SETTING_ENABLE_SERVICE_NON_POLL_NOTIFY                      L"EnableServiceNonPollNotify"
#define SETTING_ENABLE_SERVICE_STAGE2                               L"EnableServiceStage2"
#define SETTING_ENABLE_SERVICE_PROGRESS_DIALOG                      L"EnableServiceProgressDialog"
#define SETTING_ENABLE_SHELL_EXECUTE_SKIP_IFEO_DEBUGGER             L"EnableShellExecuteSkipIfeoDebugger"
#define SETTING_ENABLE_STAGE2                                       L"EnableStage2"
#define SETTING_ENABLE_STREAMER_MODE                                L"EnableStreamerMode"
#define SETTING_ENABLE_START_AS_ADMIN                               L"EnableStartAsAdmin"
#define SETTING_ENABLE_START_AS_ADMIN_ALWAYS_ON_TOP                 L"EnableStartAsAdminAlwaysOnTop"
#define SETTING_ENABLE_DEFAULT_SAFE_PLUGINS                         L"EnableDefaultSafePlugins"
#define SETTING_ENABLE_SECURITY_ADVANCED_DIALOG                     L"EnableSecurityAdvancedDialog"
#define SETTING_ENABLE_SHORT_RELATIVE_START_TIME                    L"EnableShortRelativeStartTime"
#define SETTING_ENABLE_SHUTDOWN_CRITICAL_MENU                       L"EnableShutdownCriticalMenu"
#define SETTING_ENABLE_SHUTDOWN_BOOT_MENU                           L"EnableShutdownBootMenu"
#define SETTING_ENABLE_SILENT_CRASH_NOTIFY                          L"EnableSilentCrashNotify"
#define SETTING_ENABLE_THEME_SUPPORT                                L"EnableThemeSupport"
#define SETTING_ENABLE_THEME_ACRYLIC_SUPPORT                        L"EnableThemeAcrylicSupport"
#define SETTING_ENABLE_THEME_ACRYLIC_WINDOW_SUPPORT                 L"EnableThemeAcrylicWindowSupport"
#define SETTING_ENABLE_THEME_ANIMATION                              L"EnableThemeAnimation"
#define SETTING_ENABLE_THEME_NATIVE_BUTTONS                         L"EnableThemeNativeButtons"
#define SETTING_ENABLE_THREAD_STACK_INLINE_SYMBOLS                  L"EnableThreadStackInlineSymbols"
#define SETTING_ENABLE_THREAD_STACK_LINE_INFORMATION                L"EnableThreadStackLineInformation"
#define SETTING_ENABLE_TOKEN_REMOVED_PRIVILEGES                     L"EnableTokenRemovedPrivileges"
#define SETTING_ENABLE_TOOLTIP_SUPPORT                              L"EnableTooltipSupport"
#define SETTING_ENABLE_UPDATE_DEFAULT_FIRMWARE_BOOT_ENTRY           L"EnableUpdateDefaultFirmwareBootEntry"
#define SETTING_ENABLE_VERSION_SUPPORT                              L"EnableVersionSupport"
#define SETTING_ENABLE_WARNINGS                                     L"EnableWarnings"
#define SETTING_ENABLE_WARNINGS_RUNAS                               L"EnableWarningsRunas"
#define SETTING_ENABLE_WINDOW_TEXT                                  L"EnableWindowText"
#define SETTING_ENVIRONMENT_TREE_LIST_COLUMNS                       L"EnvironmentTreeListColumns"
#define SETTING_ENVIRONMENT_TREE_LIST_SORT                          L"EnvironmentTreeListSort"
#define SETTING_ENVIRONMENT_TREE_LIST_FLAGS                         L"EnvironmentTreeListFlags"
#define SETTING_SEARCH_CONTROL_REGEX                                L"SearchControlRegex"
#define SETTING_SEARCH_CONTROL_CASE_SENSITIVE                       L"SearchControlCaseSensitive"
#define SETTING_FIND_OBJ_TREE_LIST_COLUMNS                          L"FindObjTreeListColumns"
#define SETTING_FIND_OBJ_WINDOW_POSITION                            L"FindObjWindowPosition"
#define SETTING_FIND_OBJ_WINDOW_SIZE                                L"FindObjWindowSize"
#define SETTING_THREAD_STACKS_TREE_LIST_COLUMNS                     L"ThreadStacksTreeListColumns"
#define SETTING_THREAD_STACKS_WINDOW_POSITION                       L"ThreadStacksWindowPosition"
#define SETTING_THREAD_STACKS_WINDOW_SIZE                           L"ThreadStacksWindowSize"
#define SETTING_FILE_BROWSE_EXECUTABLE                              L"FileBrowseExecutable"
#define SETTING_FIRST_RUN                                           L"FirstRun"
#define SETTING_FONT                                                L"Font"
#define SETTING_FONT_MONOSPACE                                      L"FontMonospace"
#define SETTING_FONT_QUALITY                                        L"FontQuality"
#define SETTING_FORCE_NO_PARENT                                     L"ForceNoParent"
#define SETTING_HANDLE_TREE_LIST_COLUMNS                            L"HandleTreeListColumns"
#define SETTING_HANDLE_TREE_LIST_SORT                               L"HandleTreeListSort"
#define SETTING_HANDLE_TREE_LIST_FLAGS                              L"HandleTreeListFlags"
#define SETTING_HANDLE_PROPERTIES_WINDOW_POSITION                   L"HandlePropertiesWindowPosition"
#define SETTING_HANDLE_PROPERTIES_WINDOW_SIZE                       L"HandlePropertiesWindowSize"
#define SETTING_HANDLE_STATISTICS_LIST_VIEW_COLUMNS                 L"HandleStatisticsListViewColumns"
#define SETTING_HANDLE_STATISTICS_LIST_VIEW_SORT                    L"HandleStatisticsListViewSort"
#define SETTING_HANDLE_STATISTICS_WINDOW_POSITION                   L"HandleStatisticsWindowPosition"
#define SETTING_HANDLE_STATISTICS_WINDOW_SIZE                       L"HandleStatisticsWindowSize"
#define SETTING_HIDE_DEFAULT_SERVICES                               L"HideDefaultServices"
#define SETTING_HIDE_DRIVER_SERVICES                                L"HideDriverServices"
#define SETTING_HIDE_FREE_REGIONS                                   L"HideFreeRegions"
#define SETTING_HIDE_ON_CLOSE                                       L"HideOnClose"
#define SETTING_HIDE_ON_MINIMIZE                                    L"HideOnMinimize"
#define SETTING_HIDE_OTHER_USER_PROCESSES                           L"HideOtherUserProcesses"
#define SETTING_HIDE_SIGNED_PROCESSES                               L"HideSignedProcesses"
#define SETTING_HIDE_MICROSOFT_PROCESSES                            L"HideMicrosoftProcesses"
#define SETTING_HIDE_WAITING_CONNECTIONS                            L"HideWaitingConnections"
#define SETTING_HIGHLIGHTING_DURATION                               L"HighlightingDuration"
#define SETTING_ICON_BALLOON_SHOW_ICON                              L"IconBalloonShowIcon"
#define SETTING_ICON_BALLOON_MUTE_SOUND                             L"IconBalloonMuteSound"
#define SETTING_TOAST_NOTIFY_ENABLED                                L"ToastNotifyEnabled"
#define SETTING_ICON_TRAY_GUIDS                                     L"IconTrayGuids"
#define SETTING_ICON_TRAY_PERSIST_GUID_ENABLED                      L"IconTrayPersistGuidEnabled"
#define SETTING_ICON_TRAY_LAZY_START_DELAY                          L"IconTrayLazyStartDelay"
#define SETTING_ICON_IGNORE_BALLOON_CLICK                           L"IconIgnoreBalloonClick"
#define SETTING_ICON_SETTINGS                                       L"IconSettings"
#define SETTING_ICON_NOTIFY_MASK                                    L"IconNotifyMask"
#define SETTING_ICON_PROCESSES                                      L"IconProcesses"
#define SETTING_ICON_SINGLE_CLICK                                   L"IconSingleClick"
#define SETTING_ICON_TOGGLES_VISIBILITY                             L"IconTogglesVisibility"
#define SETTING_ICON_TRANSPARENCY_ENABLED                           L"IconTransparencyEnabled"
#define SETTING_INFORMATION_WINDOW_POSITION                         L"InformationWindowPosition"
#define SETTING_INFORMATION_WINDOW_SIZE                             L"InformationWindowSize"
#define SETTING_IMAGE_COHERENCY_SCAN_LEVEL                          L"ImageCoherencyScanLevel"
#define SETTING_JOB_LIST_VIEW_COLUMNS                               L"JobListViewColumns"
#define SETTING_LOG_ENTRIES                                         L"LogEntries"
#define SETTING_LOG_LIST_VIEW_COLUMNS                               L"LogListViewColumns"
#define SETTING_LOG_WINDOW_POSITION                                 L"LogWindowPosition"
#define SETTING_LOG_WINDOW_SIZE                                     L"LogWindowSize"
#define SETTING_MAIN_WINDOW_ALWAYS_ON_TOP                           L"MainWindowAlwaysOnTop"
#define SETTING_MAIN_WINDOW_CLASS_NAME                              L"MainWindowClassName"
#define SETTING_MAIN_WINDOW_OPACITY                                 L"MainWindowOpacity"
#define SETTING_MAIN_WINDOW_POSITION                                L"MainWindowPosition"
#define SETTING_MAIN_WINDOW_SIZE                                    L"MainWindowSize"
#define SETTING_MAIN_WINDOW_STATE                                   L"MainWindowState"
#define SETTING_MAIN_WINDOW_TAB_RESTORE_ENABLED                     L"MainWindowTabRestoreEnabled"
#define SETTING_MAIN_WINDOW_TAB_RESTORE_INDEX                       L"MainWindowTabRestoreIndex"
#define SETTING_MAX_SIZE_UNIT                                       L"MaxSizeUnit"
#define SETTING_MAX_PRECISION_UNIT                                  L"MaxPrecisionUnit"
#define SETTING_MEM_EDIT_BYTES_PER_ROW                              L"MemEditBytesPerRow"
#define SETTING_MEM_EDIT_GOTO_CHOICES                               L"MemEditGotoChoices"
#define SETTING_MEM_EDIT_POSITION                                   L"MemEditPosition"
#define SETTING_MEM_EDIT_SIZE                                       L"MemEditSize"
#define SETTING_MEM_FILTER_CHOICES                                  L"MemFilterChoices"
#define SETTING_MEM_RESULTS_LIST_VIEW_COLUMNS                       L"MemResultsListViewColumns"
#define SETTING_MEM_RESULTS_POSITION                                L"MemResultsPosition"
#define SETTING_MEM_RESULTS_SIZE                                    L"MemResultsSize"
#define SETTING_MEMORY_LIST_FLAGS                                   L"MemoryListFlags"
#define SETTING_MEMORY_TREE_LIST_COLUMNS                            L"MemoryTreeListColumns"
#define SETTING_MEMORY_TREE_LIST_SORT                               L"MemoryTreeListSort"
#define SETTING_MEMORY_LISTS_WINDOW_POSITION                        L"MemoryListsWindowPosition"
#define SETTING_MEMORY_READ_WRITE_ADDRESS_CHOICES                   L"MemoryReadWriteAddressChoices"
#define SETTING_MEMORY_MODIFIED_WINDOW_POSITION                     L"MemoryModifiedWindowPosition"
#define SETTING_MEMORY_MODIFIED_WINDOW_SIZE                         L"MemoryModifiedWindowSize"
#define SETTING_MEMORY_MODIFIED_LIST_VIEW_COLUMNS                   L"MemoryModifiedListViewColumns"
#define SETTING_MEMORY_MODIFIED_LIST_VIEW_SORT                      L"MemoryModifiedListViewSort"
#define SETTING_MEM_STRINGS_TREE_LIST_COLUMNS                       L"MemStringsTreeListColumns"
#define SETTING_MEM_STRINGS_TREE_LIST_SORT                          L"MemStringsTreeListSort"
#define SETTING_MEM_STRINGS_TREE_LIST_FLAGS                         L"MemStringsTreeListFlags"
#define SETTING_MEM_STRINGS_MINIMUM_LENGTH                          L"MemStringsMinimumLength"
#define SETTING_MEM_STRINGS_WINDOW_POSITION                         L"MemStringsWindowPosition"
#define SETTING_MEM_STRINGS_WINDOW_SIZE                             L"MemStringsWindowSize"
#define SETTING_MINI_INFO_CONTAINER_CLASS_NAME                      L"MiniInfoContainerClassName"
#define SETTING_MINI_INFO_WINDOW_CLASS_NAME                         L"MiniInfoWindowClassName"
#define SETTING_MINI_INFO_WINDOW_ENABLED                            L"MiniInfoWindowEnabled"
#define SETTING_MINI_INFO_WINDOW_OPACITY                            L"MiniInfoWindowOpacity"
#define SETTING_MINI_INFO_WINDOW_PINNED                             L"MiniInfoWindowPinned"
#define SETTING_MINI_INFO_WINDOW_POSITION                           L"MiniInfoWindowPosition"
#define SETTING_MINI_INFO_WINDOW_REFRESH_AUTOMATICALLY              L"MiniInfoWindowRefreshAutomatically"
#define SETTING_MINI_INFO_WINDOW_SIZE                               L"MiniInfoWindowSize"
#define SETTING_MODULE_TREE_LIST_FLAGS                              L"ModuleTreeListFlags"
#define SETTING_MODULE_TREE_LIST_COLUMNS                            L"ModuleTreeListColumns"
#define SETTING_MODULE_TREE_LIST_SORT                               L"ModuleTreeListSort"
#define SETTING_NETWORK_TREE_LIST_COLUMNS                           L"NetworkTreeListColumns"
#define SETTING_NETWORK_TREE_LIST_SORT                              L"NetworkTreeListSort"
#define SETTING_NON_POLL_FLUSH_INTERVAL                             L"NonPollFlushInterval"
#define SETTING_NO_PURGE_PROCESS_RECORDS                            L"NoPurgeProcessRecords"
#define SETTING_OPTIONS_CUSTOM_COLOR_LIST                           L"OptionsCustomColorList"
#define SETTING_OPTIONS_WINDOW_ADVANCED_COLUMNS                     L"OptionsWindowAdvancedColumns"
#define SETTING_OPTIONS_WINDOW_ADVANCED_FLAGS                       L"OptionsWindowAdvancedFlags"
#define SETTING_OPTIONS_WINDOW_POSITION                             L"OptionsWindowPosition"
#define SETTING_OPTIONS_WINDOW_SIZE                                 L"OptionsWindowSize"
#define SETTING_PAGE_FILE_WINDOW_POSITION                           L"PageFileWindowPosition"
#define SETTING_PAGE_FILE_WINDOW_SIZE                               L"PageFileWindowSize"
#define SETTING_PAGE_FILE_LIST_VIEW_COLUMNS                         L"PageFileListViewColumns"
#define SETTING_PLUGIN_MANAGER_TREE_LIST_COLUMNS                    L"PluginManagerTreeListColumns"
#define SETTING_PROCESS_SERVICE_LIST_VIEW_COLUMNS                   L"ProcessServiceListViewColumns"
#define SETTING_PROCESS_TREE_COLUMN_SET_CONFIG                      L"ProcessTreeColumnSetConfig"
#define SETTING_PROCESS_TREE_LIST_COLUMNS                           L"ProcessTreeListColumns"
#define SETTING_PROCESS_TREE_LIST_SORT                              L"ProcessTreeListSort"
#define SETTING_PROCESS_TREE_LIST_NAME_DEFAULT                      L"ProcessTreeListNameDefault"
#define SETTING_PROC_PROP_PAGE                                      L"ProcPropPage"
#define SETTING_PROC_PROP_POSITION                                  L"ProcPropPosition"
#define SETTING_PROC_PROP_SIZE                                      L"ProcPropSize"
#define SETTING_PROC_STAT_PROP_PAGE_GROUP_LIST_VIEW_COLUMNS         L"ProcStatPropPageGroupListViewColumns"
#define SETTING_PROC_STAT_PROP_PAGE_GROUP_LIST_VIEW_SORT            L"ProcStatPropPageGroupListViewSort"
#define SETTING_PROC_STAT_PROP_PAGE_GROUP_STATES                    L"ProcStatPropPageGroupStates"
#define SETTING_PROGRAM_INSPECT_EXECUTABLES                         L"ProgramInspectExecutables"
#define SETTING_PROPAGATE_CPU_USAGE                                 L"PropagateCpuUsage"
#define SETTING_RELEASE_CHANNEL                                     L"ReleaseChannel"
#define SETTING_RUN_AS_ENABLE_AUTO_COMPLETE                         L"RunAsEnableAutoComplete"
#define SETTING_RUN_AS_PROGRAM                                      L"RunAsProgram"
#define SETTING_RUN_AS_USER_NAME                                    L"RunAsUserName"
#define SETTING_RUN_AS_WINDOW_POSITION                              L"RunAsWindowPosition"
#define SETTING_RUN_AS_PACKAGE_WINDOW_POSITION                      L"RunAsPackageWindowPosition"
#define SETTING_RUN_AS_PACKAGE_WINDOW_SIZE                          L"RunAsPackageWindowSize"
#define SETTING_RUN_FILE_DLG_STATE                                  L"RunFileDlgState"
#define SETTING_SAMPLE_COUNT                                        L"SampleCount"
#define SETTING_SAMPLE_COUNT_AUTOMATIC                              L"SampleCountAutomatic"
#define SETTING_SCROLL_TO_NEW_PROCESSES                             L"ScrollToNewProcesses"
#define SETTING_SCROLL_TO_REMOVED_PROCESSES                         L"ScrollToRemovedProcesses"
#define SETTING_SEARCH_ENGINE                                       L"SearchEngine"
#define SETTING_SEGMENT_HEAP_LIST_VIEW_COLUMNS                      L"SegmentHeapListViewColumns"
#define SETTING_SEGMENT_HEAP_LIST_VIEW_SORT                         L"SegmentHeapListViewSort"
#define SETTING_SEGMENT_HEAP_WINDOW_POSITION                        L"SegmentHeapWindowPosition"
#define SETTING_SEGMENT_HEAP_WINDOW_SIZE                            L"SegmentHeapWindowSize"
#define SETTING_SEGMENT_LOCKS_LIST_VIEW_COLUMNS                     L"SegmentLocksListViewColumns"
#define SETTING_SEGMENT_LOCKS_LIST_VIEW_SORT                        L"SegmentLocksListViewSort"
#define SETTING_SEGMENT_LOCKS_WINDOW_POSITION                       L"SegmentLocksWindowPosition"
#define SETTING_SEGMENT_LOCKS_WINDOW_SIZE                           L"SegmentLocksWindowSize"
#define SETTING_SERVICE_WINDOW_POSITION                             L"ServiceWindowPosition"
#define SETTING_SERVICE_LIST_VIEW_COLUMNS                           L"ServiceListViewColumns"
#define SETTING_SERVICE_TREE_LIST_COLUMNS                           L"ServiceTreeListColumns"
#define SETTING_SERVICE_TREE_LIST_SORT                              L"ServiceTreeListSort"
#define SETTING_SESSION_SHADOW_HOTKEY                               L"SessionShadowHotkey"
#define SETTING_SHOW_PLUGIN_LOAD_ERRORS                             L"ShowPluginLoadErrors"
#define SETTING_SHOW_COMMIT_IN_SUMMARY                              L"ShowCommitInSummary"
#define SETTING_SHOW_CPU_BELOW_001                                  L"ShowCpuBelow001"
#define SETTING_SHOW_HEX_ID                                         L"ShowHexId"
#define SETTING_SORT_CHILD_PROCESSES                                L"SortChildProcesses"
#define SETTING_SORT_ROOT_PROCESSES                                 L"SortRootProcesses"
#define SETTING_START_HIDDEN                                        L"StartHidden"
#define SETTING_SYSINFO_SHOW_CPU_SPEED_MHZ                          L"SysInfoShowCpuSpeedMhz"
#define SETTING_SYSINFO_SHOW_CPU_SPEED_PER_CPU                      L"SysInfoShowCpuSpeedPerCpu"
#define SETTING_SYSINFO_WINDOW_ALWAYS_ON_TOP                        L"SysInfoWindowAlwaysOnTop"
#define SETTING_SYSINFO_WINDOW_ONE_GRAPH_PER_CPU                    L"SysInfoWindowOneGraphPerCpu"
#define SETTING_SYSINFO_WINDOW_POSITION                             L"SysInfoWindowPosition"
#define SETTING_SYSINFO_WINDOW_SECTION                              L"SysInfoWindowSection"
#define SETTING_SYSINFO_WINDOW_SIZE                                 L"SysInfoWindowSize"
#define SETTING_SYSINFO_WINDOW_STATE                                L"SysInfoWindowState"
#define SETTING_TASKMGR_WINDOW_STATE                                L"TaskmgrWindowState"
#define SETTING_THEME_WINDOW_FOREGROUND_COLOR                       L"ThemeWindowForegroundColor"
#define SETTING_THEME_WINDOW_BACKGROUND_COLOR                       L"ThemeWindowBackgroundColor"
#define SETTING_THEME_WINDOW_BACKGROUND2_COLOR                      L"ThemeWindowBackground2Color"
#define SETTING_THEME_WINDOW_HIGHLIGHT_COLOR                        L"ThemeWindowHighlightColor"
#define SETTING_THEME_WINDOW_HIGHLIGHT2_COLOR                       L"ThemeWindowHighlight2Color"
#define SETTING_THEME_WINDOW_TEXT_COLOR                             L"ThemeWindowTextColor"
#define SETTING_THIN_ROWS                                           L"ThinRows"
#define SETTING_THREAD_TREE_LIST_COLUMNS                            L"ThreadTreeListColumns"
#define SETTING_THREAD_TREE_LIST_SORT                               L"ThreadTreeListSort"
#define SETTING_THREAD_TREE_LIST_FLAGS                              L"ThreadTreeListFlags"
#define SETTING_THREAD_STACK_TREE_LIST_COLUMNS                      L"ThreadStackTreeListColumns"
#define SETTING_THREAD_STACK_WINDOW_SIZE                            L"ThreadStackWindowSize"
#define SETTING_TOKEN_WINDOW_POSITION                               L"TokenWindowPosition"
#define SETTING_TOKEN_WINDOW_SIZE                                   L"TokenWindowSize"
#define SETTING_TOKEN_GROUPS_LIST_VIEW_COLUMNS                      L"TokenGroupsListViewColumns"
#define SETTING_TOKEN_GROUPS_LIST_VIEW_STATES                       L"TokenGroupsListViewStates"
#define SETTING_TOKEN_GROUPS_LIST_VIEW_SORT                         L"TokenGroupsListViewSort"
#define SETTING_TOKEN_PRIVILEGES_LIST_VIEW_COLUMNS                  L"TokenPrivilegesListViewColumns"
#define SETTING_TREE_LIST_BORDER_ENABLE                             L"TreeListBorderEnable"
#define SETTING_TREE_LIST_CUSTOM_COLORS_ENABLE                      L"TreeListCustomColorsEnable"
#define SETTING_TREE_LIST_CUSTOM_COLOR_TEXT                         L"TreeListCustomColorText"
#define SETTING_TREE_LIST_CUSTOM_COLOR_FOCUS                        L"TreeListCustomColorFocus"
#define SETTING_TREE_LIST_CUSTOM_COLOR_SELECTION                    L"TreeListCustomColorSelection"
#define SETTING_TREE_LIST_CUSTOM_ROW_SIZE                           L"TreeListCustomRowSize"
#define SETTING_TREE_LIST_ENABLE_HEADER_TOTALS                      L"TreeListEnableHeaderTotals"
#define SETTING_TREE_LIST_ENABLE_DRAG_REORDER                       L"TreeListEnableDragReorder"
#define SETTING_UPDATE_INTERVAL                                     L"UpdateInterval"
#define SETTING_USER_LIST_TREE_LIST_COLUMNS                         L"UserListTreeListColumns"
#define SETTING_USER_LIST_WINDOW_POSITION                           L"UserListWindowPosition"
#define SETTING_USER_LIST_WINDOW_SIZE                               L"UserListWindowSize"
#define SETTING_WMI_PROVIDER_ENABLE_HIDDEN_MENU                     L"WmiProviderEnableHiddenMenu"
#define SETTING_WMI_PROVIDER_ENABLE_TOOLTIP_SUPPORT                 L"WmiProviderEnableTooltipSupport"
#define SETTING_WMI_PROVIDER_TREE_LIST_COLUMNS                      L"WmiProviderTreeListColumns"
#define SETTING_WMI_PROVIDER_TREE_LIST_SORT                         L"WmiProviderTreeListSort"
#define SETTING_WMI_PROVIDER_TREE_LIST_FLAGS                        L"WmiProviderTreeListFlags"
#define SETTING_VDM_HOST_LIST_VIEW_COLUMNS                          L"VdmHostListViewColumns"
#define SETTING_ZOMBIE_PROCESSES_LIST_VIEW_COLUMNS                  L"ZombieProcessesListViewColumns"
#define SETTING_ZOMBIE_PROCESSES_WINDOW_POSITION                    L"ZombieProcessesWindowPosition"
#define SETTING_ZOMBIE_PROCESSES_WINDOW_SIZE                        L"ZombieProcessesWindowSize"
#define SETTING_COLOR_NEW                                           L"ColorNew"
#define SETTING_COLOR_REMOVED                                       L"ColorRemoved"
#define SETTING_USE_COLOR_OWN_PROCESSES                             L"UseColorOwnProcesses"
#define SETTING_COLOR_OWN_PROCESSES                                 L"ColorOwnProcesses"
#define SETTING_USE_COLOR_SYSTEM_PROCESSES                          L"UseColorSystemProcesses"
#define SETTING_COLOR_SYSTEM_PROCESSES                              L"ColorSystemProcesses"
#define SETTING_USE_COLOR_SERVICE_PROCESSES                         L"UseColorServiceProcesses"
#define SETTING_COLOR_SERVICE_PROCESSES                             L"ColorServiceProcesses"
#define SETTING_USE_COLOR_BACKGROUND_PROCESSES                      L"UseColorBackgroundProcesses"
#define SETTING_COLOR_BACKGROUND_PROCESSES                          L"ColorBackgroundProcesses"
#define SETTING_USE_COLOR_JOB_PROCESSES                             L"UseColorJobProcesses"
#define SETTING_COLOR_JOB_PROCESSES                                 L"ColorJobProcesses"
#define SETTING_USE_COLOR_WOW64_PROCESSES                           L"UseColorWow64Processes"
#define SETTING_COLOR_WOW64_PROCESSES                               L"ColorWow64Processes"
#define SETTING_USE_COLOR_DEBUGGED_PROCESSES                        L"UseColorDebuggedProcesses"
#define SETTING_COLOR_DEBUGGED_PROCESSES                            L"ColorDebuggedProcesses"
#define SETTING_USE_COLOR_ELEVATED_PROCESSES                        L"UseColorElevatedProcesses"
#define SETTING_COLOR_ELEVATED_PROCESSES                            L"ColorElevatedProcesses"
#define SETTING_USE_COLOR_HANDLE_FILTERED                           L"UseColorHandleFiltered"
#define SETTING_COLOR_HANDLE_FILTERED                               L"ColorHandleFiltered"
#define SETTING_USE_COLOR_IMMERSIVE_PROCESSES                       L"UseColorImmersiveProcesses"
#define SETTING_COLOR_IMMERSIVE_PROCESSES                           L"ColorImmersiveProcesses"
#define SETTING_USE_COLOR_UI_ACCESS_PROCESSES                       L"UseColorUIAccessProcesses"
#define SETTING_COLOR_UI_ACCESS_PROCESSES                           L"ColorUIAccessProcesses"
#define SETTING_USE_COLOR_PICO_PROCESSES                            L"UseColorPicoProcesses"
#define SETTING_COLOR_PICO_PROCESSES                                L"ColorPicoProcesses"
#define SETTING_USE_COLOR_SUSPENDED                                 L"UseColorSuspended"
#define SETTING_COLOR_SUSPENDED                                     L"ColorSuspended"
#define SETTING_USE_COLOR_DOT_NET                                   L"UseColorDotNet"
#define SETTING_COLOR_DOT_NET                                       L"ColorDotNet"
#define SETTING_USE_COLOR_PACKED                                    L"UseColorPacked"
#define SETTING_COLOR_PACKED                                        L"ColorPacked"
#define SETTING_USE_COLOR_LOW_IMAGE_COHERENCY                       L"UseColorLowImageCoherency"
#define SETTING_COLOR_LOW_IMAGE_COHERENCY                           L"ColorLowImageCoherency"
#define SETTING_USE_COLOR_PARTIALLY_SUSPENDED                       L"UseColorPartiallySuspended"
#define SETTING_COLOR_PARTIALLY_SUSPENDED                           L"ColorPartiallySuspended"
#define SETTING_USE_COLOR_GUI_THREADS                               L"UseColorGuiThreads"
#define SETTING_COLOR_GUI_THREADS                                   L"ColorGuiThreads"
#define SETTING_USE_COLOR_RELOCATED_MODULES                         L"UseColorRelocatedModules"
#define SETTING_COLOR_RELOCATED_MODULES                             L"ColorRelocatedModules"
#define SETTING_USE_COLOR_PROTECTED_HANDLES                         L"UseColorProtectedHandles"
#define SETTING_COLOR_PROTECTED_HANDLES                             L"ColorProtectedHandles"
#define SETTING_USE_COLOR_PROTECTED_INHERIT_HANDLES                 L"UseColorProtectedInheritHandles"
#define SETTING_COLOR_PROTECTED_INHERIT_HANDLES                     L"ColorProtectedInheritHandles"
#define SETTING_USE_COLOR_PROTECTED_PROCESS                         L"UseColorProtectedProcess"
#define SETTING_COLOR_PROTECTED_PROCESS                             L"ColorProtectedProcess"
#define SETTING_USE_COLOR_INHERIT_HANDLES                           L"UseColorInheritHandles"
#define SETTING_COLOR_INHERIT_HANDLES                               L"ColorInheritHandles"
#define SETTING_USE_COLOR_EFFICIENCY_MODE                           L"UseColorEfficiencyMode"
#define SETTING_COLOR_EFFICIENCY_MODE                               L"ColorEfficiencyMode"
#define SETTING_USE_COLOR_SERVICE_DISABLED                          L"UseColorServiceDisabled"
#define SETTING_COLOR_SERVICE_DISABLED                              L"ColorServiceDisabled"
#define SETTING_USE_COLOR_SERVICE_STOP                              L"UseColorServiceStop"
#define SETTING_COLOR_SERVICE_STOP                                  L"ColorServiceStop"
#define SETTING_USE_COLOR_UNKNOWN                                   L"UseColorUnknown"
#define SETTING_COLOR_UNKNOWN                                       L"ColorUnknown"
#define SETTING_USE_COLOR_SYSTEM_THREAD_STACK                       L"UseColorSystemThreadStack"
#define SETTING_COLOR_SYSTEM_THREAD_STACK                           L"ColorSystemThreadStack"
#define SETTING_USE_COLOR_USER_THREAD_STACK                         L"UseColorUserThreadStack"
#define SETTING_COLOR_USER_THREAD_STACK                             L"ColorUserThreadStack"
#define SETTING_USE_COLOR_INLINE_THREAD_STACK                       L"UseColorInlineThreadStack"
#define SETTING_COLOR_INLINE_THREAD_STACK                           L"ColorInlineThreadStack"
#define SETTING_GRAPH_SHOW_TEXT                                     L"GraphShowText"
#define SETTING_GRAPH_COLOR_MODE                                    L"GraphColorMode"
#define SETTING_COLOR_CPU_KERNEL                                    L"ColorCpuKernel"
#define SETTING_COLOR_CPU_USER                                      L"ColorCpuUser"
#define SETTING_COLOR_IO_READ_OTHER                                 L"ColorIoReadOther"
#define SETTING_COLOR_IO_WRITE                                      L"ColorIoWrite"
#define SETTING_COLOR_PRIVATE                                       L"ColorPrivate"
#define SETTING_COLOR_PHYSICAL                                      L"ColorPhysical"
#define SETTING_COLOR_POWER_USAGE                                   L"ColorPowerUsage"
#define SETTING_COLOR_TEMPERATURE                                   L"ColorTemperature"
#define SETTING_COLOR_FAN_RPM                                       L"ColorFanRpm"

#define SETTING_KSI_ENABLE                                          L"KsiEnable"
#define SETTING_KSI_ENABLE_WARNINGS                                 L"KsiEnableWarnings"
#define SETTING_KSI_SERVICE_NAME                                    L"KsiServiceName"
#define SETTING_KSI_OBJECT_NAME                                     L"KsiObjectName"
#define SETTING_KSI_PORT_NAME                                       L"KsiPortName"
#define SETTING_KSI_ALTITUDE                                        L"KsiAltitude"
#define SETTING_KSI_SYSTEM_PROCESS_NAME                             L"KsiSystemProcessName"
#define SETTING_KSI_DISABLE_IMAGE_LOAD_PROTECTION                   L"KsiDisableImageLoadProtection"
#define SETTING_KSI_ENABLE_SPLASH_SCREEN                            L"KsiEnableSplashScreen"
#define SETTING_KSI_ENABLE_LOAD_NATIVE                              L"KsiEnableLoadNative"
#define SETTING_KSI_ENABLE_LOAD_FILTER                              L"KsiEnableLoadFilter"
#define SETTING_KSI_UNLOAD_ON_EXIT                                  L"KsiUnloadOnExit"
#define SETTING_KSI_RANDOMIZED_POOL_TAG                             L"KsiRandomizedPoolTag"
#define SETTING_KSI_ENABLE_UNLOAD_PROTECTION                        L"KsiEnableUnloadProtection"
#define SETTING_KSI_DYN_DATA_NO_EMBEDDED                            L"KsiDynDataNoEmbedded"
#define SETTING_KSI_DISABLE_SYSTEM_PROCESS                          L"KsiDisableSystemProcess"
#define SETTING_KSI_DISABLE_THREAD_NAMES                            L"KsiDisableThreadNames"
#define SETTING_KSI_CLIENT_PROCESS_PROTECTION_LEVEL                 L"KsiClientProcessProtectionLevel"
#define SETTING_KSI_PREVIOUS_TEMPORARY_DRIVER_FILE                  L"KsiPreviousTemporaryDriverFile"
#define SETTING_KSI_ENABLE_FS_FEATURE_OFFLOAD_READ                  L"KsiEnableFsFeatureOffloadRead"
#define SETTING_KSI_ENABLE_FS_FEATURE_OFFLOAD_WRITE                 L"KsiEnableFsFeatureOffloadWrite"
#define SETTING_KSI_ENABLE_FS_FEATURE_QUERY_OPEN                    L"KsiEnableFsFeatureQueryOpen"
#define SETTING_KSI_ENABLE_FS_FEATURE_BYPASS_IO                     L"KsiEnableFsFeatureBypassIO"
#define SETTING_KSI_RING_BUFFER_LENGTH                              L"KsiRingBufferLength"

#define SETTING_ENABLE_PROCESS_MONITOR                              L"EnableProcessMonitor"
#define SETTING_PROCESS_MONITOR_LOOKBACK                            L"ProcessMonitorLookback"
#define SETTING_PROCESS_MONITOR_CACHE_LIMIT                         L"ProcessMonitorCacheLimit"

#endif
