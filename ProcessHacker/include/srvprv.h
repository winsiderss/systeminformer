#ifndef PH_SRVPRV_H
#define PH_SRVPRV_H

extern PPH_OBJECT_TYPE PhServiceItemType;

PHAPPAPI extern PH_CALLBACK PhServiceAddedEvent; // phapppub
PHAPPAPI extern PH_CALLBACK PhServiceModifiedEvent; // phapppub
PHAPPAPI extern PH_CALLBACK PhServiceRemovedEvent; // phapppub
PHAPPAPI extern PH_CALLBACK PhServicesUpdatedEvent; // phapppub

extern BOOLEAN PhEnableServiceNonPoll;

// begin_phapppub
typedef struct _PH_SERVICE_ITEM
{
    PH_STRINGREF Key; // points to Name
    PPH_STRING Name;
    PPH_STRING DisplayName;

    // State
    ULONG Type;
    ULONG State;
    ULONG ControlsAccepted;
    ULONG Flags; // e.g. SERVICE_RUNS_IN_SYSTEM_PROCESS
    HANDLE ProcessId;

    // Config
    ULONG StartType;
    ULONG ErrorControl;
// end_phapppub
    BOOLEAN DelayedStart;
    BOOLEAN HasTriggers;

    BOOLEAN PendingProcess;
    BOOLEAN NeedsConfigUpdate;

    WCHAR ProcessIdString[PH_INT32_STR_LEN_1];
// begin_phapppub
} PH_SERVICE_ITEM, *PPH_SERVICE_ITEM;
// end_phapppub

// begin_phapppub
typedef struct _PH_SERVICE_MODIFIED_DATA
{
    PPH_SERVICE_ITEM Service;
    PH_SERVICE_ITEM OldService;
} PH_SERVICE_MODIFIED_DATA, *PPH_SERVICE_MODIFIED_DATA;

typedef enum _PH_SERVICE_CHANGE
{
    ServiceStarted,
    ServiceContinued,
    ServicePaused,
    ServiceStopped
} PH_SERVICE_CHANGE, *PPH_SERVICE_CHANGE;
// end_phapppub

BOOLEAN PhServiceProviderInitialization(
    VOID
    );

PPH_SERVICE_ITEM PhCreateServiceItem(
    _In_opt_ LPENUM_SERVICE_STATUS_PROCESS Information
    );

// begin_phapppub
PHAPPAPI
PPH_SERVICE_ITEM
NTAPI
PhReferenceServiceItem(
    _In_ PWSTR Name
    );
// end_phapppub

VOID PhMarkNeedsConfigUpdateServiceItem(
    _In_ PPH_SERVICE_ITEM ServiceItem
    );

// begin_phapppub
PHAPPAPI
PH_SERVICE_CHANGE
NTAPI
PhGetServiceChange(
    _In_ PPH_SERVICE_MODIFIED_DATA Data
    );
// end_phapppub

VOID PhUpdateProcessItemServices(
    _In_ PPH_PROCESS_ITEM ProcessItem
    );

VOID PhServiceProviderUpdate(
    _In_ PVOID Object
    );

#endif
