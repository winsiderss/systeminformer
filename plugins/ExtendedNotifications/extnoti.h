#ifndef EXTNOTI_H
#define EXTNOTI_H

#define SETTING_PREFIX L"ProcessHacker.ExtendedNotifications."
#define SETTING_NAME_LOG_FILENAME (SETTING_PREFIX L"LogFileName")
#define SETTING_NAME_PROCESS_LIST (SETTING_PREFIX L"ProcessList")
#define SETTING_NAME_SERVICE_LIST (SETTING_PREFIX L"ServiceList")

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
