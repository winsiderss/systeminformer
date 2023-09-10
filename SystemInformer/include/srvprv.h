/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2009-2016
 *     dmex    2017-2023
 *
 */

#ifndef PH_SRVPRV_H
#define PH_SRVPRV_H

extern PPH_OBJECT_TYPE PhServiceItemType;
extern BOOLEAN PhEnableServiceNonPoll;
extern BOOLEAN PhEnableServiceNonPollNotify;
extern ULONG PhServiceNonPollFlushInterval;

// begin_phapppub
typedef enum _VERIFY_RESULT VERIFY_RESULT;
typedef struct _PH_IMAGELIST_ITEM* PPH_IMAGELIST_ITEM;

typedef struct _PH_SERVICE_ITEM
{
    PH_STRINGREF Key; // points to Name
    PPH_STRING Name;
    PPH_STRING DisplayName;
    PPH_STRING FileName; // only available after first update

    PPH_IMAGELIST_ITEM IconEntry;
    volatile LONG JustProcessed;

    // State
    ULONG Type;
    ULONG State;
    ULONG ControlsAccepted;
    ULONG Flags; // e.g. SERVICE_RUNS_IN_SYSTEM_PROCESS
    HANDLE ProcessId;

    // Config
    ULONG StartType;
    ULONG ErrorControl;

    // Signature
    VERIFY_RESULT VerifyResult;
    PPH_STRING VerifySignerName;

    WCHAR ProcessIdString[PH_INT32_STR_LEN_1];

    // end_phapppub
    union
    {
        BOOLEAN BitFlags;
        struct
        {
            BOOLEAN DelayedStart : 1;
            BOOLEAN HasTriggers : 1;
            BOOLEAN PendingProcess : 1;
            BOOLEAN NeedsConfigUpdate : 1;
            BOOLEAN Spare : 4;
        };
    };

    PSC_NOTIFICATION_REGISTRATION NotifyPropertyRegistration;
    PSC_NOTIFICATION_REGISTRATION NotifyStatusRegistration;
    union
    {
        BOOLEAN NotifyFlags;
        struct
        {
            BOOLEAN NotifyCreatedPropertyRegistration : 1;
            BOOLEAN NotifyCreatedStatusRegistration : 1;
            BOOLEAN NotifySpare : 6;
        };
    };
// begin_phapppub
} PH_SERVICE_ITEM, *PPH_SERVICE_ITEM;
// end_phapppub

// begin_phapppub
typedef struct _PH_SERVICE_MODIFIED_DATA
{
    PPH_SERVICE_ITEM ServiceItem;
    PH_SERVICE_ITEM OldService;
} PH_SERVICE_MODIFIED_DATA, *PPH_SERVICE_MODIFIED_DATA;

typedef enum _PH_SERVICE_CHANGE
{
    ServiceNone,
    ServiceStarted,
    ServiceContinued,
    ServicePaused,
    ServiceStopped,
    ServiceModified,
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
