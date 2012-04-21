#ifndef FWMON_H
#define FWMON_H

#include <phdk.h>

extern PPH_PLUGIN PluginInstance;

#define SETTING_PREFIX L"ProcessHacker.FirewallMonitor."
#define SETTING_NAME_FW_TREE_LIST_COLUMNS (SETTING_PREFIX L"FwTreeListColumns")
#define SETTING_NAME_FW_TREE_LIST_SORT (SETTING_PREFIX L"FwTreeListSort")

typedef struct _FW_EVENT_ITEM
{
    LARGE_INTEGER Time;
} FW_EVENT_ITEM, *PFW_EVENT_ITEM;

#define FWTNC_TIME 0
#define FWTNC_PROCESS 1
#define FWTNC_USER 2
#define FWTNC_LOCALADDRESS 3
#define FWTNC_LOCALPORT 4
#define FWTNC_REMOTEADDRESS 5
#define FWTNC_REMOTEPORT 6
#define FWTNC_PROTOCOL 7
#define FWTNC_MAXIMUM 8

typedef struct _FW_EVENT_NODE
{
    PH_TREENEW_NODE Node;

    PH_STRINGREF TextCache[FWTNC_MAXIMUM];

    PFW_EVENT_ITEM EventItem;

    PPH_STRING TooltipText;
} FW_EVENT_NODE, *PFW_EVENT_NODE;

// monitor

extern PH_CALLBACK FwItemAddedEvent;
extern PH_CALLBACK FwItemModifiedEvent;
extern PH_CALLBACK FwItemRemovedEvent;
extern PH_CALLBACK FwItemsUpdatedEvent;

ULONG StartFwMonitor(
    VOID
    );

VOID StopFwMonitor(
    VOID
    );

// fwtab

VOID InitializeFwTab(
    VOID
    );

VOID LoadSettingsFwTreeList(
    VOID
    );

VOID SaveSettingsFwTreeList(
    VOID
    );

#endif
