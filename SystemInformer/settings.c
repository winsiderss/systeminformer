/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2010-2016
 *     dmex    2017-2023
 *     jxy-s   2021
 *
 */

#include <phapp.h>
#include <settings.h>
#define PH_SETTINGS_PRIVATE
#include <phsettings.h>

VOID PhAddDefaultSettings(
    VOID
    )
{
    PhpAddIntegerSetting(SETTING_ALLOW_ONLY_ONE_INSTANCE, L"1");
    PhpAddIntegerSetting(SETTING_CLOSE_ON_ESCAPE, L"0");
    PhpAddStringSetting(SETTING_DBGHELP_SEARCH_PATH, L"SRV*C:\\Symbols*https://msdl.microsoft.com/download/symbols");
    PhpAddIntegerSetting(SETTING_DBGHELP_UNDECORATE, L"1");
    PhpAddStringSetting(SETTING_DISABLED_PLUGINS, L"");
    PhpAddIntegerSetting(SETTING_ELEVATION_LEVEL, L"1"); // PromptElevateAction
    PhpAddIntegerSetting(SETTING_ENABLE_ADVANCED_OPTIONS, L"0");
    PhpAddIntegerSetting(SETTING_ENABLE_AVX_SUPPORT, L"0");
    PhpAddIntegerSetting(SETTING_ENABLE_BITMAP_SUPPORT, L"1");
    PhpAddIntegerSetting(SETTING_ENABLE_BREAK_ON_TERMINATION, L"0");
    PhpAddIntegerSetting(SETTING_ENABLE_BOOT_OBJECTS_ENUMERATE, L"0");
    PhpAddIntegerSetting(SETTING_ENABLE_COMMAND_LINE_TOOLTIPS, L"0");
    PhpAddIntegerSetting(SETTING_ENABLE_CYCLE_CPU_USAGE, L"1");
    PhpAddIntegerSetting(SETTING_ENABLE_DEFERRED_LAYOUT, L"1");
    PhpAddIntegerSetting(SETTING_ENABLE_DEVICE_SUPPORT, L"1");
    PhpAddIntegerSetting(SETTING_ENABLE_DEVICE_NOTIFY_SUPPORT, L"1");
    PhpAddIntegerSetting(SETTING_ENABLE_IMAGE_COHERENCY_SUPPORT, L"0");
    PhpAddIntegerSetting(SETTING_ENABLE_INSTANT_TOOLTIPS, L"0");
    PhpAddIntegerSetting(SETTING_ENABLE_HEAP_REFLECTION, L"0");
    PhpAddIntegerSetting(SETTING_ENABLE_HEAP_MEMORY_TAGGING, L"0");
    PhpAddIntegerSetting(SETTING_ENABLE_LAST_PROCESS_SHUTDOWN, L"0");
    PhpAddIntegerSetting(SETTING_ENABLE_LINUX_SUBSYSTEM_SUPPORT, L"0");
    PhpAddIntegerSetting(SETTING_ENABLE_HANDLE_SNAPSHOT, L"1");
    PhpAddIntegerSetting(SETTING_ENABLE_MINIDUMP_KERNEL_MINIDUMP, L"0");
    PhpAddIntegerSetting(SETTING_ENABLE_MINIDUMP_SNAPSHOT, L"0");
    PhpAddIntegerSetting(SETTING_ENABLE_MONOSPACE_FONT, L"0");
    PhpAddIntegerSetting(SETTING_ENABLE_NETWORK_BOUND_CONNECTIONS, L"1");
    PhpAddIntegerSetting(SETTING_ENABLE_NETWORK_RESOLVE, L"1");
    PhpAddIntegerSetting(SETTING_ENABLE_NETWORK_RESOLVE_DOH, L"0");
    PhpAddIntegerSetting(SETTING_ENABLE_MEM_STRINGS_TREE_DIALOG, L"0");
    PhpAddIntegerSetting(SETTING_ENABLE_PACKAGE_ICON_SUPPORT, L"0");
    PhpAddIntegerSetting(SETTING_ENABLE_PROCESS_HANDLE_PNP_DEVICE_NAME_SUPPORT, L"0");
    PhpAddIntegerSetting(SETTING_ENABLE_PLUGINS, L"1");
    PhpAddIntegerSetting(SETTING_ENABLE_PLUGINS_NATIVE, L"0");
    PhpAddIntegerSetting(SETTING_ENABLE_GRAPH_MAX_SCALE, L"0");
    PhpAddIntegerSetting(SETTING_ENABLE_GRAPH_MAX_TEXT, L"1");
    PhpAddIntegerSetting(SETTING_ENABLE_SERVICE_NON_POLL, L"0");
    PhpAddIntegerSetting(SETTING_ENABLE_SERVICE_NON_POLL_NOTIFY, L"1");
    PhpAddIntegerSetting(SETTING_ENABLE_SERVICE_STAGE2, L"0");
    PhpAddIntegerSetting(SETTING_ENABLE_SERVICE_PROGRESS_DIALOG, L"1");
    PhpAddIntegerSetting(SETTING_ENABLE_SHELL_EXECUTE_SKIP_IFEO_DEBUGGER, L"1");
    PhpAddIntegerSetting(SETTING_ENABLE_STAGE2, L"1");
    PhpAddIntegerSetting(SETTING_ENABLE_STREAMER_MODE, L"0");
    PhpAddIntegerSetting(SETTING_ENABLE_START_AS_ADMIN, L"0");
    PhpAddIntegerSetting(SETTING_ENABLE_START_AS_ADMIN_ALWAYS_ON_TOP, L"0");
    PhpAddIntegerSetting(SETTING_ENABLE_DEFAULT_SAFE_PLUGINS, L"1");
    PhpAddIntegerSetting(SETTING_ENABLE_SECURITY_ADVANCED_DIALOG, L"1");
    PhpAddIntegerSetting(SETTING_ENABLE_SHORT_RELATIVE_START_TIME, L"1");
    PhpAddIntegerSetting(SETTING_ENABLE_SHUTDOWN_CRITICAL_MENU, L"0");
    PhpAddIntegerSetting(SETTING_ENABLE_SHUTDOWN_BOOT_MENU, L"1");
    PhpAddIntegerSetting(SETTING_ENABLE_SILENT_CRASH_NOTIFY, L"0");
    PhpAddIntegerSetting(SETTING_ENABLE_THEME_SUPPORT, L"0");
    PhpAddIntegerSetting(SETTING_ENABLE_THEME_ACRYLIC_SUPPORT, L"0");
    PhpAddIntegerSetting(SETTING_ENABLE_THEME_ACRYLIC_WINDOW_SUPPORT, L"0");
    PhpAddIntegerSetting(SETTING_ENABLE_THEME_ANIMATION, L"1");
    PhpAddIntegerSetting(SETTING_ENABLE_THEME_NATIVE_BUTTONS, L"0");
    PhpAddIntegerSetting(SETTING_ENABLE_THREAD_STACK_INLINE_SYMBOLS, L"1");
    PhpAddIntegerSetting(SETTING_ENABLE_THREAD_STACK_LINE_INFORMATION, L"1");
    PhpAddIntegerSetting(SETTING_ENABLE_TOKEN_REMOVED_PRIVILEGES, L"0");
    PhpAddIntegerSetting(SETTING_ENABLE_TOOLTIP_SUPPORT, L"1");
    PhpAddIntegerSetting(SETTING_ENABLE_UPDATE_DEFAULT_FIRMWARE_BOOT_ENTRY, L"1");
    PhpAddIntegerSetting(SETTING_ENABLE_VERSION_SUPPORT, L"1");
    PhpAddIntegerSetting(SETTING_ENABLE_WARNINGS, L"1");
    PhpAddIntegerSetting(SETTING_ENABLE_WARNINGS_RUNAS, L"1");
    PhpAddIntegerSetting(SETTING_ENABLE_WINDOW_TEXT, L"1");
    PhpAddStringSetting(SETTING_ENVIRONMENT_TREE_LIST_COLUMNS, L"");
    PhpAddStringSetting(SETTING_ENVIRONMENT_TREE_LIST_SORT, L"0,0"); // 0, NoSortOrder
    PhpAddIntegerSetting(SETTING_ENVIRONMENT_TREE_LIST_FLAGS, L"0");
    PhpAddIntegerSetting(SETTING_SEARCH_CONTROL_REGEX, L"0");
    PhpAddIntegerSetting(SETTING_SEARCH_CONTROL_CASE_SENSITIVE, L"0");
    PhpAddStringSetting(SETTING_FIND_OBJ_TREE_LIST_COLUMNS, L"");
    PhpAddIntegerPairSetting(SETTING_FIND_OBJ_WINDOW_POSITION, L"0,0");
    PhpAddScalableIntegerPairSetting(SETTING_FIND_OBJ_WINDOW_SIZE, L"@96|550,420");
    PhpAddStringSetting(SETTING_THREAD_STACKS_TREE_LIST_COLUMNS, L"");
    PhpAddIntegerPairSetting(SETTING_THREAD_STACKS_WINDOW_POSITION, L"0,0");
    PhpAddScalableIntegerPairSetting(SETTING_THREAD_STACKS_WINDOW_SIZE, L"@96|550,420");
    PhpAddStringSetting(SETTING_FILE_BROWSE_EXECUTABLE, L"%SystemRoot%\\explorer.exe /select,\"%s\"");
    PhpAddIntegerSetting(SETTING_FIRST_RUN, L"1");
    PhpAddStringSetting(SETTING_FONT, L""); // null
    PhpAddStringSetting(SETTING_FONT_MONOSPACE, L""); // null
    PhpAddIntegerSetting(SETTING_FONT_QUALITY, L"0");
    PhpAddIntegerSetting(SETTING_FORCE_NO_PARENT, L"1");
    PhpAddStringSetting(SETTING_HANDLE_TREE_LIST_COLUMNS, L"");
    PhpAddStringSetting(SETTING_HANDLE_TREE_LIST_SORT, L"0,1"); // 0, AscendingSortOrder
    PhpAddIntegerSetting(SETTING_HANDLE_TREE_LIST_FLAGS, L"3");
    PhpAddIntegerPairSetting(SETTING_HANDLE_PROPERTIES_WINDOW_POSITION, L"0,0");
    PhpAddScalableIntegerPairSetting(SETTING_HANDLE_PROPERTIES_WINDOW_SIZE, L"@96|260,260");
    PhpAddStringSetting(SETTING_HANDLE_STATISTICS_LIST_VIEW_COLUMNS, L"");
    PhpAddStringSetting(SETTING_HANDLE_STATISTICS_LIST_VIEW_SORT, L"0,1");
    PhpAddIntegerPairSetting(SETTING_HANDLE_STATISTICS_WINDOW_POSITION, L"0,0");
    PhpAddScalableIntegerPairSetting(SETTING_HANDLE_STATISTICS_WINDOW_SIZE, L"@96|0,0");
    PhpAddIntegerSetting(SETTING_HIDE_DEFAULT_SERVICES, L"0");
    PhpAddIntegerSetting(SETTING_HIDE_DRIVER_SERVICES, L"0");
    PhpAddIntegerSetting(SETTING_HIDE_FREE_REGIONS, L"1");
    PhpAddIntegerSetting(SETTING_HIDE_ON_CLOSE, L"0");
    PhpAddIntegerSetting(SETTING_HIDE_ON_MINIMIZE, L"0");
    PhpAddIntegerSetting(SETTING_HIDE_OTHER_USER_PROCESSES, L"0");
    PhpAddIntegerSetting(SETTING_HIDE_SIGNED_PROCESSES, L"0");
    PhpAddIntegerSetting(SETTING_HIDE_MICROSOFT_PROCESSES, L"0");
    PhpAddIntegerSetting(SETTING_HIDE_WAITING_CONNECTIONS, L"0");
    PhpAddIntegerSetting(SETTING_HIGHLIGHTING_DURATION, L"3e8"); // 1000ms
    PhpAddIntegerSetting(SETTING_ICON_BALLOON_SHOW_ICON, L"0");
    PhpAddIntegerSetting(SETTING_ICON_BALLOON_MUTE_SOUND, L"0");
    PhpAddIntegerSetting(SETTING_TOAST_NOTIFY_ENABLED, L"0");
    PhpAddStringSetting(SETTING_ICON_TRAY_GUIDS, L"");
    PhpAddIntegerSetting(SETTING_ICON_TRAY_PERSIST_GUID_ENABLED, L"0");
    PhpAddIntegerSetting(SETTING_ICON_TRAY_LAZY_START_DELAY, L"1");
    PhpAddIntegerSetting(SETTING_ICON_IGNORE_BALLOON_CLICK, L"0");
    PhpAddStringSetting(SETTING_ICON_SETTINGS, L"");
    PhpAddIntegerSetting(SETTING_ICON_NOTIFY_MASK, L"c"); // PH_NOTIFY_SERVICE_CREATE | PH_NOTIFY_SERVICE_DELETE
    PhpAddIntegerSetting(SETTING_ICON_PROCESSES, L"f"); // 15
    PhpAddIntegerSetting(SETTING_ICON_SINGLE_CLICK, L"0");
    PhpAddIntegerSetting(SETTING_ICON_TOGGLES_VISIBILITY, L"1");
    PhpAddIntegerSetting(SETTING_ICON_TRANSPARENCY_ENABLED, L"0");
    PhpAddIntegerPairSetting(SETTING_INFORMATION_WINDOW_POSITION, L"0,0");
    PhpAddScalableIntegerPairSetting(SETTING_INFORMATION_WINDOW_SIZE, L"@96|140,190");
    PhpAddIntegerSetting(SETTING_IMAGE_COHERENCY_SCAN_LEVEL, L"1");
    PhpAddStringSetting(SETTING_JOB_LIST_VIEW_COLUMNS, L"");
    PhpAddIntegerSetting(SETTING_LOG_ENTRIES, L"200"); // 512
    PhpAddStringSetting(SETTING_LOG_LIST_VIEW_COLUMNS, L"");
    PhpAddIntegerPairSetting(SETTING_LOG_WINDOW_POSITION, L"0,0");
    PhpAddScalableIntegerPairSetting(SETTING_LOG_WINDOW_SIZE, L"@96|450,500");
    PhpAddIntegerSetting(SETTING_MAIN_WINDOW_ALWAYS_ON_TOP, L"0");
    PhpAddStringSetting(SETTING_MAIN_WINDOW_CLASS_NAME, L"MainWindowClassName");
    PhpAddIntegerSetting(SETTING_MAIN_WINDOW_OPACITY, L"0"); // means 100%
    PhpAddIntegerPairSetting(SETTING_MAIN_WINDOW_POSITION, L"100,100");
    PhpAddScalableIntegerPairSetting(SETTING_MAIN_WINDOW_SIZE, L"@96|800,600");
    PhpAddIntegerSetting(SETTING_MAIN_WINDOW_STATE, L"1");
    PhpAddIntegerSetting(SETTING_MAIN_WINDOW_TAB_RESTORE_ENABLED, L"0");
    PhpAddIntegerSetting(SETTING_MAIN_WINDOW_TAB_RESTORE_INDEX, L"0");
    PhpAddIntegerSetting(SETTING_MAX_SIZE_UNIT, L"6");
    PhpAddIntegerSetting(SETTING_MAX_PRECISION_UNIT, L"2");
    PhpAddIntegerSetting(SETTING_MEM_EDIT_BYTES_PER_ROW, L"10"); // 16
    PhpAddStringSetting(SETTING_MEM_EDIT_GOTO_CHOICES, L"");
    PhpAddIntegerPairSetting(SETTING_MEM_EDIT_POSITION, L"0,0");
    PhpAddScalableIntegerPairSetting(SETTING_MEM_EDIT_SIZE, L"@96|600,500");
    PhpAddStringSetting(SETTING_MEM_FILTER_CHOICES, L"");
    PhpAddStringSetting(SETTING_MEM_RESULTS_LIST_VIEW_COLUMNS, L"");
    PhpAddIntegerPairSetting(SETTING_MEM_RESULTS_POSITION, L"300,300");
    PhpAddScalableIntegerPairSetting(SETTING_MEM_RESULTS_SIZE, L"@96|500,520");
    PhpAddIntegerSetting(SETTING_MEMORY_LIST_FLAGS, L"3");
    PhpAddStringSetting(SETTING_MEMORY_TREE_LIST_COLUMNS, L"");
    PhpAddStringSetting(SETTING_MEMORY_TREE_LIST_SORT, L"0,0"); // 0, NoSortOrder
    PhpAddIntegerPairSetting(SETTING_MEMORY_LISTS_WINDOW_POSITION, L"400,400");
    PhpAddStringSetting(SETTING_MEMORY_READ_WRITE_ADDRESS_CHOICES, L"");
    PhpAddIntegerPairSetting(SETTING_MEMORY_MODIFIED_WINDOW_POSITION, L"0,0");
    PhpAddScalableIntegerPairSetting(SETTING_MEMORY_MODIFIED_WINDOW_SIZE, L"@96|450,500");
    PhpAddStringSetting(SETTING_MEMORY_MODIFIED_LIST_VIEW_COLUMNS, L"");
    PhpAddStringSetting(SETTING_MEMORY_MODIFIED_LIST_VIEW_SORT, L"0,0"); // 0, NoSortOrder
    PhpAddStringSetting(SETTING_MEM_STRINGS_TREE_LIST_COLUMNS, L"");
    PhpAddStringSetting(SETTING_MEM_STRINGS_TREE_LIST_SORT, L"0,1"); // 0, AscendingSortOrder
    PhpAddIntegerSetting(SETTING_MEM_STRINGS_TREE_LIST_FLAGS, L"b"); // ANSI, Unicode, Private
    PhpAddIntegerSetting(SETTING_MEM_STRINGS_MINIMUM_LENGTH, L"a"); // 10
    PhpAddIntegerPairSetting(SETTING_MEM_STRINGS_WINDOW_POSITION, L"0,0");
    PhpAddScalableIntegerPairSetting(SETTING_MEM_STRINGS_WINDOW_SIZE, L"@96|550,420");
    PhpAddStringSetting(SETTING_MINI_INFO_CONTAINER_CLASS_NAME, L"MiniInfoContainerClassName");
    PhpAddStringSetting(SETTING_MINI_INFO_WINDOW_CLASS_NAME, L"MiniInfoWindowClassName");
    PhpAddIntegerSetting(SETTING_MINI_INFO_WINDOW_ENABLED, L"1");
    PhpAddIntegerSetting(SETTING_MINI_INFO_WINDOW_OPACITY, L"0"); // means 100%
    PhpAddIntegerSetting(SETTING_MINI_INFO_WINDOW_PINNED, L"0");
    PhpAddIntegerPairSetting(SETTING_MINI_INFO_WINDOW_POSITION, L"200,200");
    PhpAddIntegerSetting(SETTING_MINI_INFO_WINDOW_REFRESH_AUTOMATICALLY, L"3");
    PhpAddScalableIntegerPairSetting(SETTING_MINI_INFO_WINDOW_SIZE, L"@96|10,200");
    PhpAddIntegerSetting(SETTING_MODULE_TREE_LIST_FLAGS, L"1");
    PhpAddStringSetting(SETTING_MODULE_TREE_LIST_COLUMNS, L"");
    PhpAddStringSetting(SETTING_MODULE_TREE_LIST_SORT, L"0,0"); // 0, NoSortOrder
    PhpAddStringSetting(SETTING_NETWORK_TREE_LIST_COLUMNS, L"");
    PhpAddStringSetting(SETTING_NETWORK_TREE_LIST_SORT, L"0,1"); // 0, AscendingSortOrder
    PhpAddIntegerSetting(SETTING_NON_POLL_FLUSH_INTERVAL, L"A"); // % 10
    PhpAddIntegerSetting(SETTING_NO_PURGE_PROCESS_RECORDS, L"0");
    PhpAddStringSetting(SETTING_OPTIONS_CUSTOM_COLOR_LIST, L"");
    PhpAddStringSetting(SETTING_OPTIONS_WINDOW_ADVANCED_COLUMNS, L"");
    PhpAddIntegerSetting(SETTING_OPTIONS_WINDOW_ADVANCED_FLAGS, L"0");
    PhpAddIntegerPairSetting(SETTING_OPTIONS_WINDOW_POSITION, L"0,0");
    PhpAddScalableIntegerPairSetting(SETTING_OPTIONS_WINDOW_SIZE, L"@96|900,590");
    PhpAddIntegerPairSetting(SETTING_PAGE_FILE_WINDOW_POSITION, L"0,0");
    PhpAddScalableIntegerPairSetting(SETTING_PAGE_FILE_WINDOW_SIZE, L"@96|500,300");
    PhpAddStringSetting(SETTING_PAGE_FILE_LIST_VIEW_COLUMNS, L"");
    PhpAddStringSetting(SETTING_PLUGIN_MANAGER_TREE_LIST_COLUMNS, L"");
    PhpAddStringSetting(SETTING_PROCESS_SERVICE_LIST_VIEW_COLUMNS, L"");
    PhpAddStringSetting(SETTING_PROCESS_TREE_COLUMN_SET_CONFIG, L"");
    PhpAddStringSetting(SETTING_PROCESS_TREE_LIST_COLUMNS, L"");
    PhpAddStringSetting(SETTING_PROCESS_TREE_LIST_SORT, L"0,0"); // 0, NoSortOrder
    PhpAddIntegerSetting(SETTING_PROCESS_TREE_LIST_NAME_DEFAULT, L"1");
    PhpAddStringSetting(SETTING_PROC_PROP_PAGE, L"General");
    PhpAddIntegerPairSetting(SETTING_PROC_PROP_POSITION, L"200,200");
    PhpAddScalableIntegerPairSetting(SETTING_PROC_PROP_SIZE, L"@96|460,580");
    PhpAddStringSetting(SETTING_PROC_STAT_PROP_PAGE_GROUP_LIST_VIEW_COLUMNS, L"");
    PhpAddStringSetting(SETTING_PROC_STAT_PROP_PAGE_GROUP_LIST_VIEW_SORT, L"0,0");
    PhpAddStringSetting(SETTING_PROC_STAT_PROP_PAGE_GROUP_STATES, L"");
    PhpAddStringSetting(SETTING_PROGRAM_INSPECT_EXECUTABLES, L"peview.exe \"%s\"");
    PhpAddIntegerSetting(SETTING_PROPAGATE_CPU_USAGE, L"0");
    PhpAddIntegerSetting(SETTING_RELEASE_CHANNEL, L"0"); // PhReleaseChannel
    PhpAddIntegerSetting(SETTING_RUN_AS_ENABLE_AUTO_COMPLETE, L"0");
    PhpAddStringSetting(SETTING_RUN_AS_PROGRAM, L"");
    PhpAddStringSetting(SETTING_RUN_AS_USER_NAME, L"");
    PhpAddIntegerPairSetting(SETTING_RUN_AS_WINDOW_POSITION, L"0,0");
    PhpAddIntegerPairSetting(SETTING_RUN_AS_PACKAGE_WINDOW_POSITION, L"0,0");
    PhpAddScalableIntegerPairSetting(SETTING_RUN_AS_PACKAGE_WINDOW_SIZE, L"@96|500,300");
    PhpAddIntegerSetting(SETTING_RUN_FILE_DLG_STATE, L"0");
    PhpAddIntegerSetting(SETTING_SAMPLE_COUNT, L"200"); // 512
    PhpAddIntegerSetting(SETTING_SAMPLE_COUNT_AUTOMATIC, L"1");
    PhpAddIntegerSetting(SETTING_SCROLL_TO_NEW_PROCESSES, L"0");
    PhpAddIntegerSetting(SETTING_SCROLL_TO_REMOVED_PROCESSES, L"0");
    PhpAddStringSetting(SETTING_SEARCH_ENGINE, L"https://duckduckgo.com/?q=\"%s\"");
    PhpAddStringSetting(SETTING_SEGMENT_HEAP_LIST_VIEW_COLUMNS, L"");
    PhpAddStringSetting(SETTING_SEGMENT_HEAP_LIST_VIEW_SORT, L"0,1");
    PhpAddIntegerPairSetting(SETTING_SEGMENT_HEAP_WINDOW_POSITION, L"0,0");
    PhpAddScalableIntegerPairSetting(SETTING_SEGMENT_HEAP_WINDOW_SIZE, L"@96|450,500");
    PhpAddStringSetting(SETTING_SEGMENT_LOCKS_LIST_VIEW_COLUMNS, L"");
    PhpAddStringSetting(SETTING_SEGMENT_LOCKS_LIST_VIEW_SORT, L"0,1");
    PhpAddIntegerPairSetting(SETTING_SEGMENT_LOCKS_WINDOW_POSITION, L"0,0");
    PhpAddScalableIntegerPairSetting(SETTING_SEGMENT_LOCKS_WINDOW_SIZE, L"@96|450,500");
    PhpAddIntegerPairSetting(SETTING_SERVICE_WINDOW_POSITION, L"0,0");
    PhpAddStringSetting(SETTING_SERVICE_LIST_VIEW_COLUMNS, L"");
    PhpAddStringSetting(SETTING_SERVICE_TREE_LIST_COLUMNS, L"");
    PhpAddStringSetting(SETTING_SERVICE_TREE_LIST_SORT, L"0,1"); // 0, AscendingSortOrder
    PhpAddIntegerPairSetting(SETTING_SESSION_SHADOW_HOTKEY, L"106,2"); // VK_MULTIPLY,KBDCTRL
    PhpAddIntegerSetting(SETTING_SHOW_PLUGIN_LOAD_ERRORS, L"1");
    PhpAddIntegerSetting(SETTING_SHOW_COMMIT_IN_SUMMARY, L"1");
    PhpAddIntegerSetting(SETTING_SHOW_CPU_BELOW_001, L"0");
    PhpAddIntegerSetting(SETTING_SHOW_HEX_ID, L"0");
    PhpAddIntegerSetting(SETTING_SORT_CHILD_PROCESSES, L"0");
    PhpAddIntegerSetting(SETTING_SORT_ROOT_PROCESSES, L"0");
    PhpAddIntegerSetting(SETTING_START_HIDDEN, L"0");
    PhpAddIntegerSetting(SETTING_SYSINFO_SHOW_CPU_SPEED_MHZ, L"0");
    PhpAddIntegerSetting(SETTING_SYSINFO_SHOW_CPU_SPEED_PER_CPU, L"0");
    PhpAddIntegerSetting(SETTING_SYSINFO_WINDOW_ALWAYS_ON_TOP, L"0");
    PhpAddIntegerSetting(SETTING_SYSINFO_WINDOW_ONE_GRAPH_PER_CPU, L"0");
    PhpAddIntegerPairSetting(SETTING_SYSINFO_WINDOW_POSITION, L"200,200");
    PhpAddStringSetting(SETTING_SYSINFO_WINDOW_SECTION, L"");
    PhpAddScalableIntegerPairSetting(SETTING_SYSINFO_WINDOW_SIZE, L"@96|900,590");
    PhpAddIntegerSetting(SETTING_SYSINFO_WINDOW_STATE, L"1");
    PhpAddIntegerSetting(SETTING_TASKMGR_WINDOW_STATE, L"0");
    PhpAddIntegerSetting(SETTING_THEME_WINDOW_FOREGROUND_COLOR, L"1c1c1c"); // RGB(28, 28, 28)
    PhpAddIntegerSetting(SETTING_THEME_WINDOW_BACKGROUND_COLOR, L"2b2b2b"); // RGB(43, 43, 43)
    PhpAddIntegerSetting(SETTING_THEME_WINDOW_BACKGROUND2_COLOR, L"414141"); // RGB(65, 65, 65)
    PhpAddIntegerSetting(SETTING_THEME_WINDOW_HIGHLIGHT_COLOR, L"808080"); // RGB(128, 128, 128)
    PhpAddIntegerSetting(SETTING_THEME_WINDOW_HIGHLIGHT2_COLOR, L"8f8f8f"); // RGB(143, 143, 143)
    PhpAddIntegerSetting(SETTING_THEME_WINDOW_TEXT_COLOR, L"ffffff"); // RGB(255, 255, 255)
    PhpAddIntegerSetting(SETTING_THIN_ROWS, L"0");
    PhpAddStringSetting(SETTING_THREAD_TREE_LIST_COLUMNS, L"");
    PhpAddStringSetting(SETTING_THREAD_TREE_LIST_SORT, L"1,2"); // 1, DescendingSortOrder
    PhpAddIntegerSetting(SETTING_THREAD_TREE_LIST_FLAGS, L"60");
    PhpAddStringSetting(SETTING_THREAD_STACK_TREE_LIST_COLUMNS, L"");
    PhpAddScalableIntegerPairSetting(SETTING_THREAD_STACK_WINDOW_SIZE, L"@96|420,400");
    PhpAddIntegerPairSetting(SETTING_TOKEN_WINDOW_POSITION, L"0,0");
    PhpAddScalableIntegerPairSetting(SETTING_TOKEN_WINDOW_SIZE, L"@96|0,0");
    PhpAddStringSetting(SETTING_TOKEN_GROUPS_LIST_VIEW_COLUMNS, L"");
    PhpAddStringSetting(SETTING_TOKEN_GROUPS_LIST_VIEW_STATES, L"");
    PhpAddStringSetting(SETTING_TOKEN_GROUPS_LIST_VIEW_SORT, L"1,2");
    PhpAddStringSetting(SETTING_TOKEN_PRIVILEGES_LIST_VIEW_COLUMNS, L"");
    PhpAddIntegerSetting(SETTING_TREE_LIST_BORDER_ENABLE, L"0");
    PhpAddIntegerSetting(SETTING_TREE_LIST_CUSTOM_COLORS_ENABLE, L"0");
    PhpAddIntegerSetting(SETTING_TREE_LIST_CUSTOM_COLOR_TEXT, L"0");
    PhpAddIntegerSetting(SETTING_TREE_LIST_CUSTOM_COLOR_FOCUS, L"0");
    PhpAddIntegerSetting(SETTING_TREE_LIST_CUSTOM_COLOR_SELECTION, L"0");
    PhpAddIntegerSetting(SETTING_TREE_LIST_CUSTOM_ROW_SIZE, L"0");
    PhpAddIntegerSetting(SETTING_TREE_LIST_ENABLE_HEADER_TOTALS, L"1");
    PhpAddIntegerSetting(SETTING_TREE_LIST_ENABLE_DRAG_REORDER, L"0");
    PhpAddIntegerSetting(SETTING_UPDATE_INTERVAL, L"3e8"); // 1000ms
    PhpAddStringSetting(SETTING_USER_LIST_TREE_LIST_COLUMNS, L"");
    PhpAddIntegerPairSetting(SETTING_USER_LIST_WINDOW_POSITION, L"0,0");
    PhpAddScalableIntegerPairSetting(SETTING_USER_LIST_WINDOW_SIZE, L"@96|550,420");
    PhpAddIntegerSetting(SETTING_WMI_PROVIDER_ENABLE_HIDDEN_MENU, L"0");
    PhpAddIntegerSetting(SETTING_WMI_PROVIDER_ENABLE_TOOLTIP_SUPPORT, L"0");
    PhpAddStringSetting(SETTING_WMI_PROVIDER_TREE_LIST_COLUMNS, L"");
    PhpAddStringSetting(SETTING_WMI_PROVIDER_TREE_LIST_SORT, L"0,0"); // 0, NoSortOrder
    PhpAddIntegerSetting(SETTING_WMI_PROVIDER_TREE_LIST_FLAGS, L"0");
    PhpAddStringSetting(SETTING_VDM_HOST_LIST_VIEW_COLUMNS, L"");
    PhpAddStringSetting(SETTING_ZOMBIE_PROCESSES_LIST_VIEW_COLUMNS, L"");
    PhpAddIntegerPairSetting(SETTING_ZOMBIE_PROCESSES_WINDOW_POSITION, L"400,400");
    PhpAddScalableIntegerPairSetting(SETTING_ZOMBIE_PROCESSES_WINDOW_SIZE, L"@96|520,400");

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
    PhpAddIntegerSetting(L"UseColorUIAccessProcesses", L"1");
    PhpAddIntegerSetting(L"ColorUIAccessProcesses", L"00aaff"); // Orange
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
    PhpAddIntegerSetting(L"UseColorProtectedInheritHandles", L"1");
    PhpAddIntegerSetting(L"ColorProtectedInheritHandles", L"c0c0c0");
    PhpAddIntegerSetting(L"UseColorProtectedProcess", L"0");
    PhpAddIntegerSetting(L"ColorProtectedProcess", L"ff8080");
    PhpAddIntegerSetting(L"UseColorInheritHandles", L"1");
    PhpAddIntegerSetting(L"ColorInheritHandles", L"ffff77");
    PhpAddIntegerSetting(L"UseColorEfficiencyMode", L"1");
    PhpAddIntegerSetting(L"ColorEfficiencyMode", L"80ff00");
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

    PhpAddIntegerSetting(SETTING_KSI_ENABLE, L"0");
    PhpAddIntegerSetting(SETTING_KSI_ENABLE_WARNINGS, L"1");
    PhpAddStringSetting(SETTING_KSI_SERVICE_NAME, L"");
    PhpAddStringSetting(SETTING_KSI_OBJECT_NAME, L"");
    PhpAddStringSetting(SETTING_KSI_PORT_NAME, L"");
    PhpAddStringSetting(SETTING_KSI_ALTITUDE, L"");
    PhpAddStringSetting(SETTING_KSI_SYSTEM_PROCESS_NAME, L"");
    PhpAddIntegerSetting(SETTING_KSI_DISABLE_IMAGE_LOAD_PROTECTION, L"0");
    PhpAddIntegerSetting(SETTING_KSI_ENABLE_SPLASH_SCREEN, L"0");
    PhpAddIntegerSetting(SETTING_KSI_ENABLE_LOAD_NATIVE, L"0");
    PhpAddIntegerSetting(SETTING_KSI_ENABLE_LOAD_FILTER, L"0");
    PhpAddIntegerSetting(SETTING_KSI_UNLOAD_ON_EXIT, L"0");
    PhpAddIntegerSetting(SETTING_KSI_RANDOMIZED_POOL_TAG, L"0");
    PhpAddIntegerSetting(SETTING_KSI_ENABLE_UNLOAD_PROTECTION, L"1");
    PhpAddIntegerSetting(SETTING_KSI_DYN_DATA_NO_EMBEDDED, L"0");
    PhpAddIntegerSetting(SETTING_KSI_DISABLE_SYSTEM_PROCESS, L"0");
    PhpAddIntegerSetting(SETTING_KSI_DISABLE_THREAD_NAMES, L"0");
    PhpAddIntegerSetting(SETTING_KSI_CLIENT_PROCESS_PROTECTION_LEVEL, L"0");
    PhpAddStringSetting(SETTING_KSI_PREVIOUS_TEMPORARY_DRIVER_FILE, L"");
    PhpAddIntegerSetting(SETTING_KSI_ENABLE_FS_FEATURE_OFFLOAD_READ, L"1");  // SUPPORTED_FS_FEATURES_OFFLOAD_READ
    PhpAddIntegerSetting(SETTING_KSI_ENABLE_FS_FEATURE_OFFLOAD_WRITE, L"1"); // SUPPORTED_FS_FEATURES_OFFLOAD_WRITE
    PhpAddIntegerSetting(SETTING_KSI_ENABLE_FS_FEATURE_QUERY_OPEN, L"1");    // SUPPORTED_FS_FEATURES_QUERY_OPEN
    PhpAddIntegerSetting(SETTING_KSI_ENABLE_FS_FEATURE_BYPASS_IO, L"1");     // SUPPORTED_FS_FEATURES_BYPASS_IO
    PhpAddIntegerSetting(SETTING_KSI_RING_BUFFER_LENGTH, L"4000000"); // bytes

    PhpAddIntegerSetting(SETTING_ENABLE_PROCESS_MONITOR, L"0");
    PhpAddIntegerSetting(SETTING_PROCESS_MONITOR_LOOKBACK, L"1e");
    PhpAddIntegerSetting(SETTING_PROCESS_MONITOR_CACHE_LIMIT, L"20000");
}

VOID PhUpdateCachedSettings(
    VOID
    )
{
    PH_GET_INTEGER_CACHED_SETTING(ForceNoParent);
    PH_GET_INTEGER_CACHED_SETTING(HighlightingDuration);
    PH_GET_INTEGER_CACHED_SETTING(PropagateCpuUsage);
    PH_GET_INTEGER_CACHED_SETTING(ScrollToNewProcesses);
    PH_GET_INTEGER_CACHED_SETTING(ScrollToRemovedProcesses);
    PH_GET_INTEGER_CACHED_SETTING(SortChildProcesses);
    PH_GET_INTEGER_CACHED_SETTING(SortRootProcesses);
    PH_GET_INTEGER_CACHED_SETTING(ShowCpuBelow001);
    PH_GET_INTEGER_CACHED_SETTING(UpdateInterval);

    PH_GET_INTEGER_CACHED_SETTING(ColorNew);
    PH_GET_INTEGER_CACHED_SETTING(ColorRemoved);
    PH_GET_INTEGER_CACHED_SETTING(UseColorOwnProcesses);
    PH_GET_INTEGER_CACHED_SETTING(ColorOwnProcesses);
    PH_GET_INTEGER_CACHED_SETTING(UseColorSystemProcesses);
    PH_GET_INTEGER_CACHED_SETTING(ColorSystemProcesses);
    PH_GET_INTEGER_CACHED_SETTING(UseColorServiceProcesses);
    PH_GET_INTEGER_CACHED_SETTING(ColorServiceProcesses);
    PH_GET_INTEGER_CACHED_SETTING(UseColorBackgroundProcesses);
    PH_GET_INTEGER_CACHED_SETTING(ColorBackgroundProcesses);
    PH_GET_INTEGER_CACHED_SETTING(UseColorJobProcesses);
    PH_GET_INTEGER_CACHED_SETTING(ColorJobProcesses);
    PH_GET_INTEGER_CACHED_SETTING(UseColorWow64Processes);
    PH_GET_INTEGER_CACHED_SETTING(ColorWow64Processes);
    PH_GET_INTEGER_CACHED_SETTING(UseColorDebuggedProcesses);
    PH_GET_INTEGER_CACHED_SETTING(ColorDebuggedProcesses);
    PH_GET_INTEGER_CACHED_SETTING(UseColorHandleFiltered);
    PH_GET_INTEGER_CACHED_SETTING(ColorHandleFiltered);
    PH_GET_INTEGER_CACHED_SETTING(UseColorElevatedProcesses);
    PH_GET_INTEGER_CACHED_SETTING(ColorElevatedProcesses);
    PH_GET_INTEGER_CACHED_SETTING(UseColorUIAccessProcesses);
    PH_GET_INTEGER_CACHED_SETTING(ColorUIAccessProcesses);
    PH_GET_INTEGER_CACHED_SETTING(UseColorPicoProcesses);
    PH_GET_INTEGER_CACHED_SETTING(ColorPicoProcesses);
    PH_GET_INTEGER_CACHED_SETTING(UseColorImmersiveProcesses);
    PH_GET_INTEGER_CACHED_SETTING(ColorImmersiveProcesses);
    PH_GET_INTEGER_CACHED_SETTING(UseColorSuspended);
    PH_GET_INTEGER_CACHED_SETTING(ColorSuspended);
    PH_GET_INTEGER_CACHED_SETTING(UseColorDotNet);
    PH_GET_INTEGER_CACHED_SETTING(ColorDotNet);
    PH_GET_INTEGER_CACHED_SETTING(UseColorPacked);
    PH_GET_INTEGER_CACHED_SETTING(ColorPacked);
    PH_GET_INTEGER_CACHED_SETTING(UseColorLowImageCoherency);
    PH_GET_INTEGER_CACHED_SETTING(ColorLowImageCoherency);
    PH_GET_INTEGER_CACHED_SETTING(UseColorPartiallySuspended);
    PH_GET_INTEGER_CACHED_SETTING(ColorPartiallySuspended);
    PH_GET_INTEGER_CACHED_SETTING(UseColorGuiThreads);
    PH_GET_INTEGER_CACHED_SETTING(ColorGuiThreads);
    PH_GET_INTEGER_CACHED_SETTING(UseColorRelocatedModules);
    PH_GET_INTEGER_CACHED_SETTING(ColorRelocatedModules);
    PH_GET_INTEGER_CACHED_SETTING(UseColorProtectedHandles);
    PH_GET_INTEGER_CACHED_SETTING(ColorProtectedHandles);
    PH_GET_INTEGER_CACHED_SETTING(UseColorProtectedInheritHandles);
    PH_GET_INTEGER_CACHED_SETTING(ColorProtectedInheritHandles);
    PH_GET_INTEGER_CACHED_SETTING(UseColorInheritHandles);
    PH_GET_INTEGER_CACHED_SETTING(ColorInheritHandles);
    PH_GET_INTEGER_CACHED_SETTING(ColorProtectedProcess);
    PH_GET_INTEGER_CACHED_SETTING(UseColorProtectedProcess);
    PH_GET_INTEGER_CACHED_SETTING(ColorEfficiencyMode);
    PH_GET_INTEGER_CACHED_SETTING(UseColorEfficiencyMode);
    PH_GET_INTEGER_CACHED_SETTING(UseColorServiceDisabled);
    PH_GET_INTEGER_CACHED_SETTING(ColorServiceDisabled);
    PH_GET_INTEGER_CACHED_SETTING(UseColorServiceStop);
    PH_GET_INTEGER_CACHED_SETTING(ColorServiceStop);
    PH_GET_INTEGER_CACHED_SETTING(UseColorUnknown);
    PH_GET_INTEGER_CACHED_SETTING(ColorUnknown);

    PH_GET_INTEGER_CACHED_SETTING(GraphShowText);
    PH_GET_INTEGER_CACHED_SETTING(GraphColorMode);
    PH_GET_INTEGER_CACHED_SETTING(ColorCpuKernel);
    PH_GET_INTEGER_CACHED_SETTING(ColorCpuUser);
    PH_GET_INTEGER_CACHED_SETTING(ColorIoReadOther);
    PH_GET_INTEGER_CACHED_SETTING(ColorIoWrite);
    PH_GET_INTEGER_CACHED_SETTING(ColorPrivate);
    PH_GET_INTEGER_CACHED_SETTING(ColorPhysical);

    PH_GET_INTEGER_CACHED_SETTING(ColorPowerUsage);
    PH_GET_INTEGER_CACHED_SETTING(ColorTemperature);
    PH_GET_INTEGER_CACHED_SETTING(ColorFanRpm);

    PH_GET_INTEGER_CACHED_SETTING(ImageCoherencyScanLevel);

    PH_GET_INTEGER_CACHED_SETTING(EnableAvxSupport);
    PH_GET_INTEGER_CACHED_SETTING(EnableGraphMaxScale);
    PH_GET_INTEGER_CACHED_SETTING(EnableGraphMaxText);
    PH_GET_INTEGER_CACHED_SETTING(EnableNetworkResolveDoH);
    PH_GET_INTEGER_CACHED_SETTING(EnableVersionSupport);
    PH_GET_INTEGER_CACHED_SETTING(EnableHandleSnapshot);

    PhEnableProcessMonitor = !!PhGetIntegerSetting(SETTING_ENABLE_PROCESS_MONITOR);
    PhProcessMonitorLookback = PhGetIntegerSetting(SETTING_PROCESS_MONITOR_LOOKBACK);
    PhProcessMonitorCacheLimit = PhGetIntegerSetting(SETTING_PROCESS_MONITOR_CACHE_LIMIT);
}
