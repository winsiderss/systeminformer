#ifndef EXTNOTI_H
#define EXTNOTI_H

#define PLUGIN_NAME L"ProcessHacker.ExtendedNotifications"
#define SETTING_NAME_LOG_FILENAME (PLUGIN_NAME L".LogFileName")
#define SETTING_NAME_PROCESS_LIST (PLUGIN_NAME L".ProcessList")
#define SETTING_NAME_SERVICE_LIST (PLUGIN_NAME L".ServiceList")
#define SETTING_NAME_DEVICE_LIST (PLUGIN_NAME L".DeviceList")

// main

typedef enum _FILTER_TYPE
{
    FilterInclude,
    FilterExclude
} FILTER_TYPE;

typedef struct _FILTER_ENTRY
{
    FILTER_TYPE Type;
    PPH_STRING Filter;
} FILTER_ENTRY, *PFILTER_ENTRY;

// filelog

VOID FileLogInitialization(
    VOID
    );

#endif
