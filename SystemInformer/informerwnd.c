/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     jxy-s   2026
 *
 */

#include <phapp.h>
#include <phsettings.h>
#include <settings.h>
#include <mainwnd.h>
#include <emenu.h>
#include <colmgr.h>
#include <kphmsgdyn.h>
#include <procprv.h>
#include <procprp.h>
#include <appsup.h>
#include <cpysave.h>
#include <secedit.h>
#include <informer.h>

#define WM_PH_INFORMER_FLUSH (WM_APP + 251)

#define PH_INFORMER_CATEGORY_PROCESS    0x01
#define PH_INFORMER_CATEGORY_THREAD     0x02
#define PH_INFORMER_CATEGORY_FILE       0x04
#define PH_INFORMER_CATEGORY_REGISTRY   0x08
#define PH_INFORMER_CATEGORY_HANDLE     0x10
#define PH_INFORMER_CATEGORY_IMAGE      0x20
#define PH_INFORMER_CATEGORY_OTHER      0x40
#define PH_INFORMER_CATEGORY_ALL        0xff

typedef enum _PH_INFORMER_COLUMN_ID
{
    PHIC_TIME = 0,
    PHIC_PROCESS,
    PHIC_PID,
    PHIC_TID,
    PHIC_CATEGORY,
    PHIC_EVENT,
    PHIC_PATH,
    PHIC_RESULT,
    PHIC_DURATION,
    PHIC_DETAILS,
    PHIC_MAXIMUM
} PH_INFORMER_COLUMN_ID;

typedef struct _PH_INFORMERW_NODE
{
    PH_TREENEW_NODE Node;
    PKPH_MESSAGE Message;
    ULONG64 PreOpSequence;

    PPH_STRING TimeText;
    PPH_STRING ProcessText;
    WCHAR PidString[PH_INT32_STR_LEN_1];
    WCHAR TidString[PH_INT32_STR_LEN_1];
    PH_STRINGREF CategoryText;
    PH_STRINGREF EventText;
    WCHAR EventFallback[16];
    PH_STRINGREF PathText;
    PPH_STRING PathAllocated;
    PPH_STRING ResultText;
    PPH_STRING DurationText;
    PPH_STRING DetailsText;
    PPH_STRING TooltipText;

    ULONG Category;
} PH_INFORMERW_NODE, *PPH_INFORMERW_NODE;

typedef struct _PH_INFORMERW_CONTEXT
{
    HWND WindowHandle;
    HWND TreeNewHandle;
    HWND SearchboxHandle;
    HWND CaptureHandle;
    HWND FilterHandle;

    PPH_LIST NodeList;
    PH_CM_MANAGER Cm;
    PH_TN_FILTER_SUPPORT TreeFilterSupport;
    PPH_TN_FILTER_ENTRY SearchFilterEntry;
    PPH_TN_FILTER_ENTRY CategoryFilterEntry;

    BOOLEAN Capturing;
    ULONG CategoryFilter;
    ULONG_PTR SearchMatchHandle;
    PH_CALLBACK_REGISTRATION InformerCallbackRegistration;

    ULONG64 ProcessStartKey;
    HANDLE ProcessId;
    PPH_STRING ProcessName;
    PPH_HASHTABLE PreOpTable;

    PH_QUEUED_LOCK PendingLock;
    PPH_LIST PendingList;
    BOOLEAN PendingPosted;

    ULONG NodeLimit;
} PH_INFORMERW_CONTEXT, *PPH_INFORMERW_CONTEXT;

static const PH_STRINGREF PhpInformerEmptyText = PH_STRINGREF_INIT(L"No events to display.");
static HWND PhpInformerWindowHandle = NULL;
static PH_LAYOUT_MANAGER PhpInformerLayoutManager;
static RECT PhpInformerMinimumSize;

PPH_INFORMERW_CONTEXT PhpCreateInformerContext(
    _In_ ULONG64 ProcessStartKey
    )
{
    PPH_INFORMERW_CONTEXT context;

    context = PhAllocateZero(sizeof(PH_INFORMERW_CONTEXT));
    context->ProcessStartKey = ProcessStartKey;
    context->Capturing = TRUE;
    context->CategoryFilter = (ULONG)PhGetIntegerSetting(SETTING_PROCESS_MONITOR_CATEGORY_FILTER);
    context->NodeLimit = (ULONG)PhGetIntegerSetting(SETTING_PROCESS_MONITOR_NODE_LIMIT);
    context->NodeList = PhCreateList(128);
    context->PreOpTable = PhCreateSimpleHashtable(64);
    context->PendingList = PhCreateList(64);
    PhInitializeQueuedLock(&context->PendingLock);

    return context;
}

VOID PhpInformerNodeFree(
    _In_ PPH_INFORMERW_NODE Node
    )
{
    PhDereferenceObject(Node->Message);

    PhClearReference(&Node->TimeText);
    PhClearReference(&Node->ProcessText);
    PhClearReference(&Node->PathAllocated);
    PhClearReference(&Node->ResultText);
    PhClearReference(&Node->DurationText);
    PhClearReference(&Node->DetailsText);
    PhClearReference(&Node->TooltipText);

    PhFree(Node);
}

VOID PhpInformerClearNodes(
    _In_ PPH_INFORMERW_CONTEXT Context
    )
{
    for (ULONG i = 0; i < Context->NodeList->Count; i++)
        PhpInformerNodeFree(Context->NodeList->Items[i]);
    PhClearList(Context->NodeList);
    PhDereferenceObject(Context->PreOpTable);
    Context->PreOpTable = PhCreateSimpleHashtable(64);
}

VOID PhpInformerDestroyContext(
    _In_ PPH_INFORMERW_CONTEXT Context
    )
{
    PhUnregisterCallback(&PhInformerCallback, &Context->InformerCallbackRegistration);

    PhRemoveTreeNewFilter(&Context->TreeFilterSupport, Context->SearchFilterEntry);
    PhRemoveTreeNewFilter(&Context->TreeFilterSupport, Context->CategoryFilterEntry);
    PhDeleteTreeNewFilterSupport(&Context->TreeFilterSupport);

    PhCmDeleteManager(&Context->Cm);

    PhpInformerClearNodes(Context);
    PhDereferenceObject(Context->NodeList);
    PhDereferenceObject(Context->PreOpTable);

    if (Context->PendingList)
    {
        for (ULONG i = 0; i < Context->PendingList->Count; i++)
            PhDereferenceObject(Context->PendingList->Items[i]);
        PhDereferenceObject(Context->PendingList);
    }

    PhClearReference(&Context->ProcessName);

    PhFree(Context);
}

PPH_PROCESS_ITEM PhpInformerReferenceProcessItem(
    _In_ HANDLE ProcessId,
    _In_ ULONG64 ProcessStartKey
    )
{
    PPH_PROCESS_ITEM processItem;

    processItem = PhReferenceProcessItem(ProcessId);

    if (processItem && ProcessStartKey != 0)
    {
        if (processItem->ProcessStartKey != 0 &&
            processItem->ProcessStartKey != ProcessStartKey)
        {
            //
            // PID was reused — this is a different process.
            //
            PhDereferenceObject(processItem);
            return NULL;
        }
    }

    return processItem;
}

ULONG PhpInformerGetCategory(
    _In_ PCKPH_MESSAGE Message
    )
{
    switch (Message->Header.MessageId)
    {
    case KphMsgProcessCreate:
    case KphMsgProcessExit:
        return PH_INFORMER_CATEGORY_PROCESS;
    case KphMsgThreadCreate:
    case KphMsgThreadExecute:
    case KphMsgThreadExit:
        return PH_INFORMER_CATEGORY_THREAD;
    case KphMsgImageLoad:
    case KphMsgImageVerify:
        return PH_INFORMER_CATEGORY_IMAGE;
    case KphMsgDebugPrint:
    case KphMsgRequiredStateFailure:
        return PH_INFORMER_CATEGORY_OTHER;
    default:
        break;
    }

    if (Message->Header.MessageId >= KphMsgHandlePreCreateProcess &&
        Message->Header.MessageId <= KphMsgHandlePostDuplicateDesktop)
        return PH_INFORMER_CATEGORY_HANDLE;

    if (Message->Header.MessageId >= KphMsgFilePreCreate &&
        Message->Header.MessageId <= KphMsgFilePostVolumeDismount)
        return PH_INFORMER_CATEGORY_FILE;

    if (Message->Header.MessageId >= KphMsgRegPreDeleteKey &&
        Message->Header.MessageId <= KphMsgRegPostSaveMergedKey)
        return PH_INFORMER_CATEGORY_REGISTRY;

    return PH_INFORMER_CATEGORY_OTHER;
}

PPH_STRING PhpInformerGetTimeText(
    _In_ PCKPH_MESSAGE Message
    )
{
    SYSTEMTIME systemTime;
    PH_FORMAT format[8];
    ULONG hour12;
    ULONG64 subSecondTicks;

    PhLargeIntegerToLocalSystemTime(&systemTime, (PLARGE_INTEGER)&Message->Header.TimeStamp.QuadPart);

    hour12 = systemTime.wHour % 12;
    if (hour12 == 0)
        hour12 = 12;

    //
    // Sub-second ticks (100ns units) are timezone-invariant, so extract
    // directly from the UTC timestamp. This gives 7 digits of precision:
    // mmm.mmmm where the first 3 are milliseconds and the last 4 are
    // sub-millisecond 100ns ticks.
    //

    subSecondTicks = Message->Header.TimeStamp.QuadPart % 10000000ULL;

    PhInitFormatD(&format[0], hour12);
    PhInitFormatC(&format[1], L':');
    PhInitFormatI64UWithWidth(&format[2], systemTime.wMinute, 2);
    PhInitFormatC(&format[3], L':');
    PhInitFormatI64UWithWidth(&format[4], systemTime.wSecond, 2);
    PhInitFormatC(&format[5], L'.');
    PhInitFormatI64UWithWidth(&format[6], subSecondTicks, 7);
    PhInitFormatS(&format[7], systemTime.wHour >= 12 ? L" PM" : L" AM");

    return PhFormat(format, RTL_NUMBER_OF(format), 24);
}

VOID PhpInformerPopulateFileEventName(
    _In_ const KPHM_FILE* File,
    _Inout_ PPH_INFORMERW_NODE Node
    )
{
    PCPH_STRINGREF name = NULL;
    PH_FORMAT fmt[3];
    SIZE_T returnLength;

    static const PH_STRINGREF mjNames[] =
    {
        PH_STRINGREF_INIT(L"IRP_MJ_CREATE"),                   // 0x00
        PH_STRINGREF_INIT(L"IRP_MJ_CREATE_NAMED_PIPE"),        // 0x01
        PH_STRINGREF_INIT(L"IRP_MJ_CLOSE"),                    // 0x02
        PH_STRINGREF_INIT(L"IRP_MJ_READ"),                     // 0x03
        PH_STRINGREF_INIT(L"IRP_MJ_WRITE"),                    // 0x04
        PH_STRINGREF_INIT(L"IRP_MJ_QUERY_INFORMATION"),        // 0x05
        PH_STRINGREF_INIT(L"IRP_MJ_SET_INFORMATION"),          // 0x06
        PH_STRINGREF_INIT(L"IRP_MJ_QUERY_EA"),                 // 0x07
        PH_STRINGREF_INIT(L"IRP_MJ_SET_EA"),                   // 0x08
        PH_STRINGREF_INIT(L"IRP_MJ_FLUSH_BUFFERS"),            // 0x09
        PH_STRINGREF_INIT(L"IRP_MJ_QUERY_VOLUME_INFORMATION"), // 0x0A
        PH_STRINGREF_INIT(L"IRP_MJ_SET_VOLUME_INFORMATION"),   // 0x0B
        PH_STRINGREF_INIT(L"IRP_MJ_DIRECTORY_CONTROL"),        // 0x0C
        PH_STRINGREF_INIT(L"IRP_MJ_FILE_SYSTEM_CONTROL"),      // 0x0D
        PH_STRINGREF_INIT(L"IRP_MJ_DEVICE_CONTROL"),           // 0x0E
        PH_STRINGREF_INIT(L"IRP_MJ_INTERNAL_DEVICE_CONTROL"),  // 0x0F
        PH_STRINGREF_INIT(L"IRP_MJ_SHUTDOWN"),                 // 0x10
        PH_STRINGREF_INIT(L"IRP_MJ_LOCK_CONTROL"),             // 0x11
        PH_STRINGREF_INIT(L"IRP_MJ_CLEANUP"),                  // 0x12
        PH_STRINGREF_INIT(L"IRP_MJ_CREATE_MAILSLOT"),          // 0x13
        PH_STRINGREF_INIT(L"IRP_MJ_QUERY_SECURITY"),           // 0x14
        PH_STRINGREF_INIT(L"IRP_MJ_SET_SECURITY"),             // 0x15
        PH_STRINGREF_INIT(L"IRP_MJ_POWER"),                    // 0x16
        PH_STRINGREF_INIT(L"IRP_MJ_SYSTEM_CONTROL"),           // 0x17
        PH_STRINGREF_INIT(L"IRP_MJ_DEVICE_CHANGE"),            // 0x18
        PH_STRINGREF_INIT(L"IRP_MJ_QUERY_QUOTA"),              // 0x19
        PH_STRINGREF_INIT(L"IRP_MJ_SET_QUOTA"),                // 0x1A
        PH_STRINGREF_INIT(L"IRP_MJ_PNP"),                      // 0x1B
    };

    static const PH_STRINGREF mjFastIoNames[] =
    {
        PH_STRINGREF_INIT(L"FASTIO_CREATE"),                   // 0x00
        PH_STRINGREF_INIT(L"FASTIO_CREATE_NAMED_PIPE"),        // 0x01
        PH_STRINGREF_INIT(L"FASTIO_CLOSE"),                    // 0x02
        PH_STRINGREF_INIT(L"FASTIO_READ"),                     // 0x03
        PH_STRINGREF_INIT(L"FASTIO_WRITE"),                    // 0x04
        PH_STRINGREF_INIT(L"FASTIO_QUERY_INFORMATION"),        // 0x05
        PH_STRINGREF_INIT(L"FASTIO_SET_INFORMATION"),          // 0x06
        PH_STRINGREF_INIT(L"FASTIO_QUERY_EA"),                 // 0x07
        PH_STRINGREF_INIT(L"FASTIO_SET_EA"),                   // 0x08
        PH_STRINGREF_INIT(L"FASTIO_FLUSH_BUFFERS"),            // 0x09
        PH_STRINGREF_INIT(L"FASTIO_QUERY_VOLUME_INFORMATION"), // 0x0A
        PH_STRINGREF_INIT(L"FASTIO_SET_VOLUME_INFORMATION"),   // 0x0B
        PH_STRINGREF_INIT(L"FASTIO_DIRECTORY_CONTROL"),        // 0x0C
        PH_STRINGREF_INIT(L"FASTIO_FILE_SYSTEM_CONTROL"),      // 0x0D
        PH_STRINGREF_INIT(L"FASTIO_DEVICE_CONTROL"),           // 0x0E
        PH_STRINGREF_INIT(L"FASTIO_INTERNAL_DEVICE_CONTROL"),  // 0x0F
        PH_STRINGREF_INIT(L"FASTIO_SHUTDOWN"),                 // 0x10
        PH_STRINGREF_INIT(L"FASTIO_LOCK_CONTROL"),             // 0x11
        PH_STRINGREF_INIT(L"FASTIO_CLEANUP"),                  // 0x12
        PH_STRINGREF_INIT(L"FASTIO_CREATE_MAILSLOT"),          // 0x13
        PH_STRINGREF_INIT(L"FASTIO_QUERY_SECURITY"),           // 0x14
        PH_STRINGREF_INIT(L"FASTIO_SET_SECURITY"),             // 0x15
        PH_STRINGREF_INIT(L"FASTIO_POWER"),                    // 0x16
        PH_STRINGREF_INIT(L"FASTIO_SYSTEM_CONTROL"),           // 0x17
        PH_STRINGREF_INIT(L"FASTIO_DEVICE_CHANGE"),            // 0x18
        PH_STRINGREF_INIT(L"FASTIO_QUERY_QUOTA"),              // 0x19
        PH_STRINGREF_INIT(L"FASTIO_SET_QUOTA"),                // 0x1A
        PH_STRINGREF_INIT(L"FASTIO_PNP"),                      // 0x1B
    };

    static const PH_STRINGREF fltNames[] =
    {
        PH_STRINGREF_INIT(L"IRP_MJ_VOLUME_DISMOUNT"),                    // ((UCHAR)-20)
        PH_STRINGREF_INIT(L"IRP_MJ_VOLUME_MOUNT"),                       // ((UCHAR)-19)
        PH_STRINGREF_INIT(L"IRP_MJ_MDL_WRITE_COMPLETE"),                 // ((UCHAR)-18)
        PH_STRINGREF_INIT(L"IRP_MJ_PREPARE_MDL_WRITE"),                  // ((UCHAR)-17)
        PH_STRINGREF_INIT(L"IRP_MJ_MDL_READ_COMPLETE"),                  // ((UCHAR)-16)
        PH_STRINGREF_INIT(L"IRP_MJ_MDL_READ"),                           // ((UCHAR)-15)
        PH_STRINGREF_INIT(L"IRP_MJ_NETWORK_QUERY_OPEN"),                 // ((UCHAR)-14)
        PH_STRINGREF_INIT(L"IRP_MJ_FAST_IO_CHECK_IF_POSSIBLE"),          // ((UCHAR)-13)
        PH_STRINGREF_INIT(L""),                                          // ((UCHAR)-12)
        PH_STRINGREF_INIT(L""),                                          // ((UCHAR)-11)
        PH_STRINGREF_INIT(L""),                                          // ((UCHAR)-10)
        PH_STRINGREF_INIT(L""),                                          // ((UCHAR)-9)
        PH_STRINGREF_INIT(L""),                                          // ((UCHAR)-8)
        PH_STRINGREF_INIT(L"IRP_MJ_QUERY_OPEN"),                         // ((UCHAR)-7)
        PH_STRINGREF_INIT(L"IRP_MJ_RELEASE_FOR_CC_FLUSH"),               // ((UCHAR)-6)
        PH_STRINGREF_INIT(L"IRP_MJ_ACQUIRE_FOR_CC_FLUSH"),               // ((UCHAR)-5)
        PH_STRINGREF_INIT(L"IRP_MJ_RELEASE_FOR_MOD_WRITE"),              // ((UCHAR)-4)
        PH_STRINGREF_INIT(L"IRP_MJ_ACQUIRE_FOR_MOD_WRITE"),              // ((UCHAR)-3)
        PH_STRINGREF_INIT(L"IRP_MJ_RELEASE_FOR_SECTION_SYNCHRONIZATION"),// ((UCHAR)-2)
        PH_STRINGREF_INIT(L"IRP_MJ_ACQUIRE_FOR_SECTION_SYNCHRONIZATION"),// ((UCHAR)-1)
    };

    static const PH_STRINGREF fltFastIoNames[] =
    {
        PH_STRINGREF_INIT(L"FASTIO_VOLUME_DISMOUNT"),                    // ((UCHAR)-20)
        PH_STRINGREF_INIT(L"FASTIO_VOLUME_MOUNT"),                       // ((UCHAR)-19)
        PH_STRINGREF_INIT(L"FASTIO_MDL_WRITE_COMPLETE"),                 // ((UCHAR)-18)
        PH_STRINGREF_INIT(L"FASTIO_PREPARE_MDL_WRITE"),                  // ((UCHAR)-17)
        PH_STRINGREF_INIT(L"FASTIO_MDL_READ_COMPLETE"),                  // ((UCHAR)-16)
        PH_STRINGREF_INIT(L"FASTIO_MDL_READ"),                           // ((UCHAR)-15)
        PH_STRINGREF_INIT(L"FASTIO_NETWORK_QUERY_OPEN"),                 // ((UCHAR)-14)
        PH_STRINGREF_INIT(L"FASTIO_FAST_IO_CHECK_IF_POSSIBLE"),          // ((UCHAR)-13)
        PH_STRINGREF_INIT(L""),                                          // ((UCHAR)-12)
        PH_STRINGREF_INIT(L""),                                          // ((UCHAR)-11)
        PH_STRINGREF_INIT(L""),                                          // ((UCHAR)-10)
        PH_STRINGREF_INIT(L""),                                          // ((UCHAR)-9)
        PH_STRINGREF_INIT(L""),                                          // ((UCHAR)-8)
        PH_STRINGREF_INIT(L"FASTIO_QUERY_OPEN"),                         // ((UCHAR)-7)
        PH_STRINGREF_INIT(L"FASTIO_RELEASE_FOR_CC_FLUSH"),               // ((UCHAR)-6)
        PH_STRINGREF_INIT(L"FASTIO_ACQUIRE_FOR_CC_FLUSH"),               // ((UCHAR)-5)
        PH_STRINGREF_INIT(L"FASTIO_RELEASE_FOR_MOD_WRITE"),              // ((UCHAR)-4)
        PH_STRINGREF_INIT(L"FASTIO_ACQUIRE_FOR_MOD_WRITE"),              // ((UCHAR)-3)
        PH_STRINGREF_INIT(L"FASTIO_RELEASE_FOR_SECTION_SYNCHRONIZATION"),// ((UCHAR)-2)
        PH_STRINGREF_INIT(L"FASTIO_ACQUIRE_FOR_SECTION_SYNCHRONIZATION"),// ((UCHAR)-1)
    };

    C_ASSERT(RTL_NUMBER_OF(mjNames) == RTL_NUMBER_OF(mjFastIoNames));
    C_ASSERT(RTL_NUMBER_OF(fltNames) == RTL_NUMBER_OF(fltFastIoNames));

    if (File->MajorFunction < RTL_NUMBER_OF(mjNames))
    {
        if (FlagOn(File->FltFlags, FLTFL_CALLBACK_DATA_FAST_IO_OPERATION))
            name = &mjFastIoNames[File->MajorFunction];
        else
            name = &mjNames[File->MajorFunction];
    }
    else if (File->MajorFunction >= IRP_MJ_VOLUME_DISMOUNT)
    {
        if (FlagOn(File->FltFlags, FLTFL_CALLBACK_DATA_FAST_IO_OPERATION))
            name = &fltFastIoNames[File->MajorFunction - IRP_MJ_VOLUME_DISMOUNT];
        else
            name = &fltNames[File->MajorFunction - IRP_MJ_VOLUME_DISMOUNT];
    }

    if (name && name->Length > 0)
    {
        Node->EventText = *name;
        return;
    }

    // "IRP_MJ (0x" (10) + hex (2) + ")" (1) + null (1) = characters + null => 15 WCHARs
    C_ASSERT(RTL_NUMBER_OF_FIELD(PH_INFORMERW_NODE, EventFallback) >= 15);
    PhInitFormatS(&fmt[0], L"IRP_MJ (0x");
    PhInitFormatI64XWithWidth(&fmt[1], (ULONG64)File->MajorFunction, 2);
    PhInitFormatC(&fmt[2], L')');
    PhFormatToBuffer(fmt, RTL_NUMBER_OF(fmt), Node->EventFallback, sizeof(Node->EventFallback), &returnLength);
    Node->EventText.Buffer = Node->EventFallback;
    Node->EventText.Length = returnLength - sizeof(UNICODE_NULL);
}

VOID PhpInformerPopulateEventName(
    _In_ PCKPH_MESSAGE Message,
    _Inout_ PPH_INFORMERW_NODE Node
    )
{
    static const PH_STRINGREF processCreate = PH_STRINGREF_INIT(L"Process Create");
    static const PH_STRINGREF processExit = PH_STRINGREF_INIT(L"Process Exit");
    static const PH_STRINGREF threadCreate = PH_STRINGREF_INIT(L"Thread Create");
    static const PH_STRINGREF threadExecute = PH_STRINGREF_INIT(L"Thread Execute");
    static const PH_STRINGREF threadExit = PH_STRINGREF_INIT(L"Thread Exit");
    static const PH_STRINGREF imageLoad = PH_STRINGREF_INIT(L"Image Load");
    static const PH_STRINGREF imageVerify = PH_STRINGREF_INIT(L"Image Verify");
    static const PH_STRINGREF debugPrint = PH_STRINGREF_INIT(L"Debug Print");
    static const PH_STRINGREF requiredStateFailure = PH_STRINGREF_INIT(L"Required State Failure");
    static const PH_STRINGREF handleCreateProcess = PH_STRINGREF_INIT(L"Handle Create (Process)");
    static const PH_STRINGREF handleDuplicateProcess = PH_STRINGREF_INIT(L"Handle Duplicate (Process)");
    static const PH_STRINGREF handleCreateThread = PH_STRINGREF_INIT(L"Handle Create (Thread)");
    static const PH_STRINGREF handleDuplicateThread = PH_STRINGREF_INIT(L"Handle Duplicate (Thread)");
    static const PH_STRINGREF handleCreateDesktop = PH_STRINGREF_INIT(L"Handle Create (Desktop)");
    static const PH_STRINGREF handleDuplicateDesktop = PH_STRINGREF_INIT(L"Handle Duplicate (Desktop)");
    static const PH_STRINGREF regDeleteKey = PH_STRINGREF_INIT(L"RegDeleteKey");
    static const PH_STRINGREF regSetValue = PH_STRINGREF_INIT(L"RegSetValue");
    static const PH_STRINGREF regDeleteValue = PH_STRINGREF_INIT(L"RegDeleteValue");
    static const PH_STRINGREF regSetInformation = PH_STRINGREF_INIT(L"RegSetInformation");
    static const PH_STRINGREF regRenameKey = PH_STRINGREF_INIT(L"RegRenameKey");
    static const PH_STRINGREF regEnumKey = PH_STRINGREF_INIT(L"RegEnumKey");
    static const PH_STRINGREF regEnumValue = PH_STRINGREF_INIT(L"RegEnumValue");
    static const PH_STRINGREF regQueryKey = PH_STRINGREF_INIT(L"RegQueryKey");
    static const PH_STRINGREF regQueryValue = PH_STRINGREF_INIT(L"RegQueryValue");
    static const PH_STRINGREF regQueryMultipleValue = PH_STRINGREF_INIT(L"RegQueryMultipleValue");
    static const PH_STRINGREF regKeyHandleClose = PH_STRINGREF_INIT(L"RegKeyHandleClose");
    static const PH_STRINGREF regCreateKey = PH_STRINGREF_INIT(L"RegCreateKey");
    static const PH_STRINGREF regOpenKey = PH_STRINGREF_INIT(L"RegOpenKey");
    static const PH_STRINGREF regFlushKey = PH_STRINGREF_INIT(L"RegFlushKey");
    static const PH_STRINGREF regLoadKey = PH_STRINGREF_INIT(L"RegLoadKey");
    static const PH_STRINGREF regUnloadKey = PH_STRINGREF_INIT(L"RegUnloadKey");
    static const PH_STRINGREF regQueryKeySecurity = PH_STRINGREF_INIT(L"RegQueryKeySecurity");
    static const PH_STRINGREF regSetKeySecurity = PH_STRINGREF_INIT(L"RegSetKeySecurity");
    static const PH_STRINGREF regRestoreKey = PH_STRINGREF_INIT(L"RegRestoreKey");
    static const PH_STRINGREF regSaveKey = PH_STRINGREF_INIT(L"RegSaveKey");
    static const PH_STRINGREF regReplaceKey = PH_STRINGREF_INIT(L"RegReplaceKey");
    static const PH_STRINGREF regQueryKeyName = PH_STRINGREF_INIT(L"RegQueryKeyName");
    static const PH_STRINGREF regSaveMergedKey = PH_STRINGREF_INIT(L"RegSaveMergedKey");
    static const PH_STRINGREF unknown = PH_STRINGREF_INIT(L"Unknown");

    KPH_MESSAGE_ID msgId = Message->Header.MessageId;
    PCPH_STRINGREF name = NULL;

    switch (msgId)
    {
    case KphMsgProcessCreate:
        name = &processCreate;
        break;
    case KphMsgProcessExit:
        name = &processExit;
        break;
    case KphMsgThreadCreate:
        name = &threadCreate;
        break;
    case KphMsgThreadExecute:
        name = &threadExecute;
        break;
    case KphMsgThreadExit:
        name = &threadExit;
        break;
    case KphMsgImageLoad:
        name = &imageLoad;
        break;
    case KphMsgImageVerify:
        name = &imageVerify;
        break;
    case KphMsgDebugPrint:
        name = &debugPrint;
        break;
    case KphMsgRequiredStateFailure:
        name = &requiredStateFailure;
        break;
    //
    // Handle events — only pre-op cases; post-ops are consumed during
    // correlation and never become nodes. If a post-op somehow appears, it
    // falls to "Unknown".
    //
    case KphMsgHandlePreCreateProcess:
        name = &handleCreateProcess;
        break;
    case KphMsgHandlePreDuplicateProcess:
        name = &handleDuplicateProcess;
        break;
    case KphMsgHandlePreCreateThread:
        name = &handleCreateThread;
        break;
    case KphMsgHandlePreDuplicateThread:
        name = &handleDuplicateThread;
        break;
    case KphMsgHandlePreCreateDesktop:
        name = &handleCreateDesktop;
        break;
    case KphMsgHandlePreDuplicateDesktop:
        name = &handleDuplicateDesktop;
        break;
    //
    // Registry events — only pre-op cases; post-ops fall to "Unknown".
    //
    case KphMsgRegPreDeleteKey:
        name = &regDeleteKey;
        break;
    case KphMsgRegPreSetValueKey:
        name = &regSetValue;
        break;
    case KphMsgRegPreDeleteValueKey:
        name = &regDeleteValue;
        break;
    case KphMsgRegPreSetInformationKey:
        name = &regSetInformation;
        break;
    case KphMsgRegPreRenameKey:
        name = &regRenameKey;
        break;
    case KphMsgRegPreEnumerateKey:
        name = &regEnumKey;
        break;
    case KphMsgRegPreEnumerateValueKey:
        name = &regEnumValue;
        break;
    case KphMsgRegPreQueryKey:
        name = &regQueryKey;
        break;
    case KphMsgRegPreQueryValueKey:
        name = &regQueryValue;
        break;
    case KphMsgRegPreQueryMultipleValueKey:
        name = &regQueryMultipleValue;
        break;
    case KphMsgRegPreKeyHandleClose:
        name = &regKeyHandleClose;
        break;
    case KphMsgRegPreCreateKey:
        name = &regCreateKey;
        break;
    case KphMsgRegPreOpenKey:
        name = &regOpenKey;
        break;
    case KphMsgRegPreFlushKey:
        name = &regFlushKey;
        break;
    case KphMsgRegPreLoadKey:
        name = &regLoadKey;
        break;
    case KphMsgRegPreUnLoadKey:
        name = &regUnloadKey;
        break;
    case KphMsgRegPreQueryKeySecurity:
        name = &regQueryKeySecurity;
        break;
    case KphMsgRegPreSetKeySecurity:
        name = &regSetKeySecurity;
        break;
    case KphMsgRegPreRestoreKey:
        name = &regRestoreKey;
        break;
    case KphMsgRegPreSaveKey:
        name = &regSaveKey;
        break;
    case KphMsgRegPreReplaceKey:
        name = &regReplaceKey;
        break;
    case KphMsgRegPreQueryKeyName:
        name = &regQueryKeyName;
        break;
    case KphMsgRegPreSaveMergedKey:
        name = &regSaveMergedKey;
        break;

    default:
        break;
    }

    if (name)
    {
        Node->EventText = *name;
        return;
    }

    //
    // File events: resolve the IRP_MJ_* name
    //
    if (msgId >= KphMsgFilePreCreate && msgId <= KphMsgFilePostVolumeDismount)
    {
        PhpInformerPopulateFileEventName(&Message->Kernel.File, Node);
        return;
    }

    Node->EventText = unknown;
}

PCPH_STRINGREF PhpInformerGetCategoryName(
    _In_ ULONG Category
    )
{
    static const PH_STRINGREF process = PH_STRINGREF_INIT(L"Process");
    static const PH_STRINGREF thread = PH_STRINGREF_INIT(L"Thread");
    static const PH_STRINGREF file = PH_STRINGREF_INIT(L"File");
    static const PH_STRINGREF registry = PH_STRINGREF_INIT(L"Registry");
    static const PH_STRINGREF handle = PH_STRINGREF_INIT(L"Handle");
    static const PH_STRINGREF image = PH_STRINGREF_INIT(L"Image");
    static const PH_STRINGREF other = PH_STRINGREF_INIT(L"Other");

    switch (Category)
    {
    case PH_INFORMER_CATEGORY_PROCESS:
        return &process;
    case PH_INFORMER_CATEGORY_THREAD:
        return &thread;
    case PH_INFORMER_CATEGORY_FILE:
        return &file;
    case PH_INFORMER_CATEGORY_REGISTRY:
        return &registry;
    case PH_INFORMER_CATEGORY_HANDLE:
        return &handle;
    case PH_INFORMER_CATEGORY_IMAGE:
        return &image;
    default:
        return &other;
    }
}

VOID PhpInformerPopulateNodePath(
    _In_ PCKPH_MESSAGE Message,
    _Inout_ PPH_INFORMERW_NODE Node
    )
{
    UNICODE_STRING str;

    if (NT_SUCCESS(KphMsgDynGetUnicodeString(Message, KphMsgFieldFileName, &str)) && str.Length > 0)
    {
        Node->PathText.Buffer = str.Buffer;
        Node->PathText.Length = str.Length;
        return;
    }

    if (NT_SUCCESS(KphMsgDynGetUnicodeString(Message, KphMsgFieldObjectName, &str)) && str.Length > 0)
    {
        Node->PathText.Buffer = str.Buffer;
        Node->PathText.Length = str.Length;

        //
        // For registry value operations, append \ValueName to the key path
        //

        switch (Message->Header.MessageId)
        {
        case KphMsgRegPreSetValueKey:
        case KphMsgRegPostSetValueKey:
        case KphMsgRegPreDeleteValueKey:
        case KphMsgRegPostDeleteValueKey:
        case KphMsgRegPreQueryValueKey:
        case KphMsgRegPostQueryValueKey:
            {
                UNICODE_STRING valueName;

                if (NT_SUCCESS(KphMsgDynGetUnicodeString(Message, KphMsgFieldValueName, &valueName)) &&
                    valueName.Length > 0)
                {
                    PH_STRINGREF separator = PhNtPathSeparatorString;
                    PH_STRINGREF valueRef;

                    valueRef.Buffer = valueName.Buffer;
                    valueRef.Length = valueName.Length;

                    Node->PathAllocated = PhConcatStringRef3(&Node->PathText, &separator, &valueRef);
                    Node->PathText = Node->PathAllocated->sr;
                }
            }
            break;
        }

        return;
    }

    Node->PathText.Buffer = NULL;
    Node->PathText.Length = 0;
}

PPH_STRING PhpInformerGetImmediateResultText(
    _In_ PCKPH_MESSAGE Message
    )
{
    switch (Message->Header.MessageId)
    {
    case KphMsgProcessExit:
        return PhGetStatusMessage(Message->Kernel.ProcessExit.ExitStatus, 0);
    default:
        break;
    }

    return NULL;
}

PCPH_STRINGREF PhpInformerGetDispositionName(
    _In_ ULONG Disposition
    )
{
    static const PH_STRINGREF supersede = PH_STRINGREF_INIT(L"Supersede");
    static const PH_STRINGREF open = PH_STRINGREF_INIT(L"Open");
    static const PH_STRINGREF create = PH_STRINGREF_INIT(L"Create");
    static const PH_STRINGREF openIf = PH_STRINGREF_INIT(L"OpenIf");
    static const PH_STRINGREF overwrite = PH_STRINGREF_INIT(L"Overwrite");
    static const PH_STRINGREF overwriteIf = PH_STRINGREF_INIT(L"OverwriteIf");
    static const PH_STRINGREF unknown = PH_STRINGREF_INIT(L"Unknown");

    switch (Disposition)
    {
    case FILE_SUPERSEDE:
        return &supersede;
    case FILE_OPEN:
        return &open;
    case FILE_CREATE:
        return &create;
    case FILE_OPEN_IF:
        return &openIf;
    case FILE_OVERWRITE:
        return &overwrite;
    case FILE_OVERWRITE_IF:
        return &overwriteIf;
    default:
        return &unknown;
    }
}

PCPH_STRINGREF PhpInformerGetRegTypeName(
    _In_ ULONG Type
    )
{
    static const PH_STRINGREF regNone = PH_STRINGREF_INIT(L"REG_NONE");
    static const PH_STRINGREF regSz = PH_STRINGREF_INIT(L"REG_SZ");
    static const PH_STRINGREF regExpandSz = PH_STRINGREF_INIT(L"REG_EXPAND_SZ");
    static const PH_STRINGREF regBinary = PH_STRINGREF_INIT(L"REG_BINARY");
    static const PH_STRINGREF regDword = PH_STRINGREF_INIT(L"REG_DWORD");
    static const PH_STRINGREF regDwordBe = PH_STRINGREF_INIT(L"REG_DWORD_BIG_ENDIAN");
    static const PH_STRINGREF regLink = PH_STRINGREF_INIT(L"REG_LINK");
    static const PH_STRINGREF regMultiSz = PH_STRINGREF_INIT(L"REG_MULTI_SZ");
    static const PH_STRINGREF regResList = PH_STRINGREF_INIT(L"REG_RESOURCE_LIST");
    static const PH_STRINGREF regFullResDesc = PH_STRINGREF_INIT(L"REG_FULL_RESOURCE_DESCRIPTOR");
    static const PH_STRINGREF regResReqList = PH_STRINGREF_INIT(L"REG_RESOURCE_REQUIREMENTS_LIST");
    static const PH_STRINGREF regQword = PH_STRINGREF_INIT(L"REG_QWORD");
    static const PH_STRINGREF unknown = PH_STRINGREF_INIT(L"Unknown");

    switch (Type)
    {
    case REG_NONE:
        return &regNone;
    case REG_SZ:
        return &regSz;
    case REG_EXPAND_SZ:
        return &regExpandSz;
    case REG_BINARY:
        return &regBinary;
    case REG_DWORD:
        return &regDword;
    case REG_DWORD_BIG_ENDIAN:
        return &regDwordBe;
    case REG_LINK:
        return &regLink;
    case REG_MULTI_SZ:
        return &regMultiSz;
    case REG_RESOURCE_LIST:
        return &regResList;
    case REG_FULL_RESOURCE_DESCRIPTOR:
        return &regFullResDesc;
    case REG_RESOURCE_REQUIREMENTS_LIST:
        return &regResReqList;
    case REG_QWORD:
        return &regQword;
    default:
        return &unknown;
    }
}

PCPH_STRINGREF PhpInformerGetFileInfoClassName(
    _In_ FILE_INFORMATION_CLASS Class
    )
{
    static const PH_STRINGREF fileDirectoryInformation = PH_STRINGREF_INIT(L"FileDirectoryInformation");
    static const PH_STRINGREF fileFullDirectoryInformation = PH_STRINGREF_INIT(L"FileFullDirectoryInformation");
    static const PH_STRINGREF fileBothDirectoryInformation = PH_STRINGREF_INIT(L"FileBothDirectoryInformation");
    static const PH_STRINGREF fileBasicInformation = PH_STRINGREF_INIT(L"FileBasicInformation");
    static const PH_STRINGREF fileStandardInformation = PH_STRINGREF_INIT(L"FileStandardInformation");
    static const PH_STRINGREF fileInternalInformation = PH_STRINGREF_INIT(L"FileInternalInformation");
    static const PH_STRINGREF fileEaInformation = PH_STRINGREF_INIT(L"FileEaInformation");
    static const PH_STRINGREF fileAccessInformation = PH_STRINGREF_INIT(L"FileAccessInformation");
    static const PH_STRINGREF fileNameInformation = PH_STRINGREF_INIT(L"FileNameInformation");
    static const PH_STRINGREF fileRenameInformation = PH_STRINGREF_INIT(L"FileRenameInformation");
    static const PH_STRINGREF fileLinkInformation = PH_STRINGREF_INIT(L"FileLinkInformation");
    static const PH_STRINGREF fileNamesInformation = PH_STRINGREF_INIT(L"FileNamesInformation");
    static const PH_STRINGREF fileDispositionInformation = PH_STRINGREF_INIT(L"FileDispositionInformation");
    static const PH_STRINGREF filePositionInformation = PH_STRINGREF_INIT(L"FilePositionInformation");
    static const PH_STRINGREF fileModeInformation = PH_STRINGREF_INIT(L"FileModeInformation");
    static const PH_STRINGREF fileAlignmentInformation = PH_STRINGREF_INIT(L"FileAlignmentInformation");
    static const PH_STRINGREF fileAllInformation = PH_STRINGREF_INIT(L"FileAllInformation");
    static const PH_STRINGREF fileAllocationInformation = PH_STRINGREF_INIT(L"FileAllocationInformation");
    static const PH_STRINGREF fileEndOfFileInformation = PH_STRINGREF_INIT(L"FileEndOfFileInformation");
    static const PH_STRINGREF fileStreamInformation = PH_STRINGREF_INIT(L"FileStreamInformation");
    static const PH_STRINGREF fileCompressionInformation = PH_STRINGREF_INIT(L"FileCompressionInformation");
    static const PH_STRINGREF fileNetworkOpenInformation = PH_STRINGREF_INIT(L"FileNetworkOpenInformation");
    static const PH_STRINGREF fileAttributeTagInformation = PH_STRINGREF_INIT(L"FileAttributeTagInformation");
    static const PH_STRINGREF fileIdBothDirectoryInformation = PH_STRINGREF_INIT(L"FileIdBothDirectoryInformation");
    static const PH_STRINGREF fileIdFullDirectoryInformation = PH_STRINGREF_INIT(L"FileIdFullDirectoryInformation");
    static const PH_STRINGREF fileValidDataLengthInformation = PH_STRINGREF_INIT(L"FileValidDataLengthInformation");
    static const PH_STRINGREF fileShortNameInformation = PH_STRINGREF_INIT(L"FileShortNameInformation");
    static const PH_STRINGREF fileIoCompletionNotificationInformation = PH_STRINGREF_INIT(L"FileIoCompletionNotificationInformation");
    static const PH_STRINGREF fileIoPriorityHintInformation = PH_STRINGREF_INIT(L"FileIoPriorityHintInformation");
    static const PH_STRINGREF fileHardLinkInformation = PH_STRINGREF_INIT(L"FileHardLinkInformation");
    static const PH_STRINGREF fileNormalizedNameInformation = PH_STRINGREF_INIT(L"FileNormalizedNameInformation");
    static const PH_STRINGREF fileNetworkPhysicalNameInformation = PH_STRINGREF_INIT(L"FileNetworkPhysicalNameInformation");
    static const PH_STRINGREF fileIdGlobalTxDirectoryInformation = PH_STRINGREF_INIT(L"FileIdGlobalTxDirectoryInformation");
    static const PH_STRINGREF fileStandardLinkInformation = PH_STRINGREF_INIT(L"FileStandardLinkInformation");
    static const PH_STRINGREF fileRemoteProtocolInformation = PH_STRINGREF_INIT(L"FileRemoteProtocolInformation");
    static const PH_STRINGREF fileRenameInformationBypassAccessCheck = PH_STRINGREF_INIT(L"FileRenameInformationBypassAccessCheck");
    static const PH_STRINGREF fileLinkInformationBypassAccessCheck = PH_STRINGREF_INIT(L"FileLinkInformationBypassAccessCheck");
    static const PH_STRINGREF fileIdInformation = PH_STRINGREF_INIT(L"FileIdInformation");
    static const PH_STRINGREF fileIdExtdDirectoryInformation = PH_STRINGREF_INIT(L"FileIdExtdDirectoryInformation");
    static const PH_STRINGREF fileIdExtdBothDirectoryInformation = PH_STRINGREF_INIT(L"FileIdExtdBothDirectoryInformation");
    static const PH_STRINGREF fileDispositionInformationEx = PH_STRINGREF_INIT(L"FileDispositionInformationEx");
    static const PH_STRINGREF fileRenameInformationEx = PH_STRINGREF_INIT(L"FileRenameInformationEx");
    static const PH_STRINGREF fileRenameInformationExBypassAccessCheck = PH_STRINGREF_INIT(L"FileRenameInformationExBypassAccessCheck");
    static const PH_STRINGREF fileStatInformation = PH_STRINGREF_INIT(L"FileStatInformation");
    static const PH_STRINGREF fileCaseSensitiveInformation = PH_STRINGREF_INIT(L"FileCaseSensitiveInformation");
    static const PH_STRINGREF fileLinkInformationEx = PH_STRINGREF_INIT(L"FileLinkInformationEx");
    static const PH_STRINGREF fileLinkInformationExBypassAccessCheck = PH_STRINGREF_INIT(L"FileLinkInformationExBypassAccessCheck");
    static const PH_STRINGREF fileStatBasicInformation = PH_STRINGREF_INIT(L"FileStatBasicInformation");

    switch (Class)
    {
    case FileDirectoryInformation:
        return &fileDirectoryInformation;
    case FileFullDirectoryInformation:
        return &fileFullDirectoryInformation;
    case FileBothDirectoryInformation:
        return &fileBothDirectoryInformation;
    case FileBasicInformation:
        return &fileBasicInformation;
    case FileStandardInformation:
        return &fileStandardInformation;
    case FileInternalInformation:
        return &fileInternalInformation;
    case FileEaInformation:
        return &fileEaInformation;
    case FileAccessInformation:
        return &fileAccessInformation;
    case FileNameInformation:
        return &fileNameInformation;
    case FileRenameInformation:
        return &fileRenameInformation;
    case FileLinkInformation:
        return &fileLinkInformation;
    case FileNamesInformation:
        return &fileNamesInformation;
    case FileDispositionInformation:
        return &fileDispositionInformation;
    case FilePositionInformation:
        return &filePositionInformation;
    case FileModeInformation:
        return &fileModeInformation;
    case FileAlignmentInformation:
        return &fileAlignmentInformation;
    case FileAllInformation:
        return &fileAllInformation;
    case FileAllocationInformation:
        return &fileAllocationInformation;
    case FileEndOfFileInformation:
        return &fileEndOfFileInformation;
    case FileStreamInformation:
        return &fileStreamInformation;
    case FileCompressionInformation:
        return &fileCompressionInformation;
    case FileNetworkOpenInformation:
        return &fileNetworkOpenInformation;
    case FileAttributeTagInformation:
        return &fileAttributeTagInformation;
    case FileIdBothDirectoryInformation:
        return &fileIdBothDirectoryInformation;
    case FileIdFullDirectoryInformation:
        return &fileIdFullDirectoryInformation;
    case FileValidDataLengthInformation:
        return &fileValidDataLengthInformation;
    case FileShortNameInformation:
        return &fileShortNameInformation;
    case FileIoCompletionNotificationInformation:
        return &fileIoCompletionNotificationInformation;
    case FileIoPriorityHintInformation:
        return &fileIoPriorityHintInformation;
    case FileHardLinkInformation:
        return &fileHardLinkInformation;
    case FileNormalizedNameInformation:
        return &fileNormalizedNameInformation;
    case FileNetworkPhysicalNameInformation:
        return &fileNetworkPhysicalNameInformation;
    case FileIdGlobalTxDirectoryInformation:
        return &fileIdGlobalTxDirectoryInformation;
    case FileStandardLinkInformation:
        return &fileStandardLinkInformation;
    case FileRemoteProtocolInformation:
        return &fileRemoteProtocolInformation;
    case FileRenameInformationBypassAccessCheck:
        return &fileRenameInformationBypassAccessCheck;
    case FileLinkInformationBypassAccessCheck:
        return &fileLinkInformationBypassAccessCheck;
    case FileIdInformation:
        return &fileIdInformation;
    case FileIdExtdDirectoryInformation:
        return &fileIdExtdDirectoryInformation;
    case FileIdExtdBothDirectoryInformation:
        return &fileIdExtdBothDirectoryInformation;
    case FileDispositionInformationEx:
        return &fileDispositionInformationEx;
    case FileRenameInformationEx:
        return &fileRenameInformationEx;
    case FileRenameInformationExBypassAccessCheck:
        return &fileRenameInformationExBypassAccessCheck;
    case FileStatInformation:
        return &fileStatInformation;
    case FileCaseSensitiveInformation:
        return &fileCaseSensitiveInformation;
    case FileLinkInformationEx:
        return &fileLinkInformationEx;
    case FileLinkInformationExBypassAccessCheck:
        return &fileLinkInformationExBypassAccessCheck;
    case FileStatBasicInformation:
        return &fileStatBasicInformation;
    default:
        return NULL;
    }
}

PCPH_STRINGREF PhpInformerGetFsInfoClassName(
    _In_ ULONG Class
    )
{
    static const PH_STRINGREF fileFsVolumeInformation = PH_STRINGREF_INIT(L"FileFsVolumeInformation");
    static const PH_STRINGREF fileFsLabelInformation = PH_STRINGREF_INIT(L"FileFsLabelInformation");
    static const PH_STRINGREF fileFsSizeInformation = PH_STRINGREF_INIT(L"FileFsSizeInformation");
    static const PH_STRINGREF fileFsDeviceInformation = PH_STRINGREF_INIT(L"FileFsDeviceInformation");
    static const PH_STRINGREF fileFsAttributeInformation = PH_STRINGREF_INIT(L"FileFsAttributeInformation");
    static const PH_STRINGREF fileFsControlInformation = PH_STRINGREF_INIT(L"FileFsControlInformation");
    static const PH_STRINGREF fileFsFullSizeInformation = PH_STRINGREF_INIT(L"FileFsFullSizeInformation");
    static const PH_STRINGREF fileFsObjectIdInformation = PH_STRINGREF_INIT(L"FileFsObjectIdInformation");
    static const PH_STRINGREF fileFsDriverPathInformation = PH_STRINGREF_INIT(L"FileFsDriverPathInformation");
    static const PH_STRINGREF fileFsVolumeFlagsInformation = PH_STRINGREF_INIT(L"FileFsVolumeFlagsInformation");
    static const PH_STRINGREF fileFsSectorSizeInformation = PH_STRINGREF_INIT(L"FileFsSectorSizeInformation");
    static const PH_STRINGREF fileFsDataCopyInformation = PH_STRINGREF_INIT(L"FileFsDataCopyInformation");
    static const PH_STRINGREF fileFsMetadataSizeInformation = PH_STRINGREF_INIT(L"FileFsMetadataSizeInformation");
    static const PH_STRINGREF fileFsFullSizeInformationEx = PH_STRINGREF_INIT(L"FileFsFullSizeInformationEx");
    static const PH_STRINGREF fileFsGuidInformation = PH_STRINGREF_INIT(L"FileFsGuidInformation");

    switch (Class)
    {
    case FileFsVolumeInformation:
        return &fileFsVolumeInformation;
    case FileFsLabelInformation:
        return &fileFsLabelInformation;
    case FileFsSizeInformation:
        return &fileFsSizeInformation;
    case FileFsDeviceInformation:
        return &fileFsDeviceInformation;
    case FileFsAttributeInformation:
        return &fileFsAttributeInformation;
    case FileFsControlInformation:
        return &fileFsControlInformation;
    case FileFsFullSizeInformation:
        return &fileFsFullSizeInformation;
    case FileFsObjectIdInformation:
        return &fileFsObjectIdInformation;
    case FileFsDriverPathInformation:
        return &fileFsDriverPathInformation;
    case FileFsVolumeFlagsInformation:
        return &fileFsVolumeFlagsInformation;
    case FileFsSectorSizeInformation:
        return &fileFsSectorSizeInformation;
    case FileFsDataCopyInformation:
        return &fileFsDataCopyInformation;
    case FileFsMetadataSizeInformation:
        return &fileFsMetadataSizeInformation;
    case FileFsFullSizeInformationEx:
        return &fileFsFullSizeInformationEx;
    case FileFsGuidInformation:
        return &fileFsGuidInformation;
    default:
        return NULL;
    }
}

PCPH_STRINGREF PhpInformerGetImageTypeName(
    _In_ ULONG ImageType
    )
{
    static const PH_STRINGREF elamDriver = PH_STRINGREF_INIT(L"ELAM Driver");
    static const PH_STRINGREF driver = PH_STRINGREF_INIT(L"Driver");
    static const PH_STRINGREF platform = PH_STRINGREF_INIT(L"Platform");
    static const PH_STRINGREF unknown = PH_STRINGREF_INIT(L"Unknown");

    switch (ImageType)
    {
    case 0: // SeImageTypeElamDriver
        return &elamDriver;
    case 1: // SeImageTypeDriver
        return &driver;
    case 2: // SeImageTypePlatform
        return &platform;
    default:
        return &unknown;
    }
}

PCPH_STRINGREF PhpInformerGetClassificationName(
    _In_ ULONG Classification
    )
{
    static const PH_STRINGREF unknownImage = PH_STRINGREF_INIT(L"Unknown");
    static const PH_STRINGREF knownGood = PH_STRINGREF_INIT(L"Known Good");
    static const PH_STRINGREF knownBad = PH_STRINGREF_INIT(L"Known Bad");
    static const PH_STRINGREF knownBadBootCritical = PH_STRINGREF_INIT(L"Known Bad (Boot Critical)");

    switch (Classification)
    {
    case 0: // BdCbClassificationUnknownImage
        return &unknownImage;
    case 1: // BdCbClassificationKnownGoodImage
        return &knownGood;
    case 2: // BdCbClassificationKnownBadImage
        return &knownBad;
    case 3: // BdCbClassificationKnownBadImageBootCritical
        return &knownBadBootCritical;
    default:
        return &unknownImage;
    }
}

PPH_STRING PhpInformerFormatAccessMask(
    _In_ PCWSTR TypeName,
    _In_ ACCESS_MASK Access
    )
{
    PPH_ACCESS_ENTRY accessEntries;
    ULONG numberOfAccessEntries;

    if (PhGetAccessEntries(TypeName, &accessEntries, &numberOfAccessEntries))
    {
        PPH_STRING symbolic = PhGetAccessString(Access, accessEntries, numberOfAccessEntries);

        PhFree(accessEntries);

        if (symbolic && symbolic->Length > 0)
        {
            PH_FORMAT format[4];
            PPH_STRING result;

            PhInitFormatSR(&format[0], symbolic->sr);
            PhInitFormatS(&format[1], L" (0x");
            PhInitFormatI64XWithWidth(&format[2], (ULONG64)Access, 8);
            PhInitFormatC(&format[3], L')');
            result = PhFormat(format, 4, 60);

            PhDereferenceObject(symbolic);
            return result;
        }

        PhClearReference(&symbolic);
    }

    {
        PH_FORMAT format[2];

        PhInitFormatS(&format[0], L"0x");
        PhInitFormatI64XWithWidth(&format[1], (ULONG64)Access, 8);

        return PhFormat(format, 2, 10);
    }
}

PPH_STRING PhpInformerGetTargetProcessName(
    _In_ PPH_INFORMERW_CONTEXT Context,
    _In_ HANDLE TargetPid
    )
{
    PPH_PROCESS_ITEM processItem = PhReferenceProcessItem(TargetPid);

    if (processItem)
    {
        PPH_STRING name = PhReferenceObject(processItem->ProcessName);
        PhDereferenceObject(processItem);
        return name;
    }

    //
    // If we're in process-scoped mode and the target PID matches the
    // monitored process, use the cached name.
    //

    if (Context->ProcessStartKey != 0 &&
        Context->ProcessName &&
        Context->ProcessId == TargetPid)
    {
        return PhReferenceObject(Context->ProcessName);
    }

    return NULL;
}

PCPH_STRINGREF PhpInformerGetIrqlName(
    _In_ UCHAR Irql
    )
{
    static const PH_STRINGREF irqlPassive = PH_STRINGREF_INIT(L"Passive");
    static const PH_STRINGREF irqlApc = PH_STRINGREF_INIT(L"APC");
    static const PH_STRINGREF irqlDispatch = PH_STRINGREF_INIT(L"Dispatch");

    switch (Irql)
    {
    case 0: // PASSIVE_LEVEL
        return &irqlPassive;
    case 1: // APC_LEVEL
        return &irqlApc;
    case 2: // DISPATCH_LEVEL
        return &irqlDispatch;
    }

    return NULL;
}

PPH_STRING PhpInformerAppendFileCommonDetails(
    _In_ const KPHM_FILE* File,
    _In_opt_ PPH_STRING PerOpDetails
    )
{
    static const PH_STRINGREF separator = PH_STRINGREF_INIT(L", ");

#define PH_IRP_FLAG(x, n) { TEXT(#x), x, FALSE, FALSE, n }
    static const PH_ACCESS_ENTRY irpFlagEntries[] =
    {
        PH_IRP_FLAG(IRP_NOCACHE, L"No cache"),
        PH_IRP_FLAG(IRP_PAGING_IO, L"Paging I/O"),
        PH_IRP_FLAG(IRP_SYNCHRONOUS_API, L"Synchronous"),
        PH_IRP_FLAG(IRP_ASSOCIATED_IRP, L"Associated IRP"),
        PH_IRP_FLAG(IRP_BUFFERED_IO, L"Buffered I/O"),
        PH_IRP_FLAG(IRP_DEALLOCATE_BUFFER, L"De-allocate buffer"),
        PH_IRP_FLAG(IRP_CREATE_OPERATION, L"Create"),
        PH_IRP_FLAG(IRP_READ_OPERATION, L"Read"),
        PH_IRP_FLAG(IRP_WRITE_OPERATION, L"Write"),
        PH_IRP_FLAG(IRP_CLOSE_OPERATION, L"Close"),
        PH_IRP_FLAG(IRP_DEFER_IO_COMPLETION, L"Defer"),
        PH_IRP_FLAG(IRP_OB_QUERY_NAME, L"Object query name"),
        PH_IRP_FLAG(IRP_HOLD_DEVICE_QUEUE, L"Hold device queue"),
        PH_IRP_FLAG(IRP_UM_DRIVER_INITIATED_IO, L"User driver I/O"),
    };
#undef PH_IRP_FLAG

    PH_STRING_BUILDER sb;
    BOOLEAN hasContent = FALSE;

    PhInitializeStringBuilder(&sb, 200);

    //
    // Start with the per-operation details if present.
    //

    if (PerOpDetails)
    {
        PhAppendStringBuilder(&sb, &PerOpDetails->sr);
        PhDereferenceObject(PerOpDetails);
        hasContent = TRUE;
    }

    //
    // IRQL — only if above PASSIVE_LEVEL.
    //

    if (File->Irql > 0)
    {
        PCPH_STRINGREF irqlName = PhpInformerGetIrqlName(File->Irql);

        if (hasContent)
            PhAppendStringBuilder(&sb, &separator);

        PhAppendStringBuilder2(&sb, L"IRQL: ");

        if (irqlName)
        {
            PhAppendStringBuilder(&sb, irqlName);
        }
        else
        {
            PH_FORMAT format[1];

            PhInitFormatU(&format[0], File->Irql);
            {
                PPH_STRING val = PhFormat(format, 1, 4);
                PhAppendStringBuilder(&sb, &val->sr);
                PhDereferenceObject(val);
            }
        }

        hasContent = TRUE;
    }

    //
    // RequestorMode — only if KernelMode (0). UserMode is the default.
    //

    if (hasContent)
        PhAppendStringBuilder(&sb, &separator);
    if (File->RequestorMode == 0)
        PhAppendStringBuilder2(&sb, L"Mode: Kernel");
    else
        PhAppendStringBuilder2(&sb, L"Mode: User");
    hasContent = TRUE;

    //
    // MinorFunction — only if non-zero.
    //

    if (File->MinorFunction != 0)
    {
        PH_FORMAT format[2];

        if (hasContent)
            PhAppendStringBuilder(&sb, &separator);

        PhInitFormatS(&format[0], L"MinorFunction: 0x");
        PhInitFormatI64XWithWidth(&format[1], (ULONG64)File->MinorFunction, 2);
        {
            PPH_STRING val = PhFormat(format, 2, 12);
            PhAppendStringBuilder(&sb, &val->sr);
            PhDereferenceObject(val);
        }
        hasContent = TRUE;
    }

    //
    // OperationFlags (SL_*) — only if non-zero. Context-dependent, so show hex.
    //

    if (File->OperationFlags != 0)
    {
        PH_FORMAT format[2];

        if (hasContent)
            PhAppendStringBuilder(&sb, &separator);

        PhInitFormatS(&format[0], L"OpFlags: 0x");
        PhInitFormatI64XWithWidth(&format[1], (ULONG64)File->OperationFlags, 2);
        {
            PPH_STRING val = PhFormat(format, 2, 16);
            PhAppendStringBuilder(&sb, &val->sr);
            PhDereferenceObject(val);
        }
        hasContent = TRUE;
    }

    //
    // IrpFlags — symbolicated via PhGetAccessString.
    //

    if (File->IrpFlags != 0)
    {
        PPH_STRING flags;
        PH_FORMAT hexFmt[4];

        if (hasContent)
            PhAppendStringBuilder(&sb, &separator);

        PhAppendStringBuilder2(&sb, L"IrpFlags: ");
        flags = PhGetAccessString(File->IrpFlags, (PPH_ACCESS_ENTRY)irpFlagEntries, RTL_NUMBER_OF(irpFlagEntries));

        PhInitFormatSR(&hexFmt[0], flags->sr);
        PhInitFormatS(&hexFmt[1], L" (0x");
        PhInitFormatX(&hexFmt[2], File->IrpFlags);
        PhInitFormatC(&hexFmt[3], L')');
        PhDereferenceObject(flags);
        flags = PhFormat(hexFmt, 4, 40);

        PhAppendStringBuilder(&sb, &flags->sr);
        PhDereferenceObject(flags);
        hasContent = TRUE;
    }

    //
    // FileObject Information bitfield flags — group interesting FILE_OBJECT state.
    //

    {
        BOOLEAN foHasContent = FALSE;

        if (File->DeletePending || File->Busy || File->LockOperation)
        {
            if (hasContent)
                PhAppendStringBuilder(&sb, &separator);

            PhAppendStringBuilder2(&sb, L"FileObject: ");

            if (File->DeletePending)
            {
                PhAppendStringBuilder2(&sb, L"DeletePending");
                foHasContent = TRUE;
            }

            if (File->Busy)
            {
                if (foHasContent)
                    PhAppendStringBuilder2(&sb, L", ");
                PhAppendStringBuilder2(&sb, L"Busy");
                foHasContent = TRUE;
            }

            if (File->LockOperation)
            {
                if (foHasContent)
                    PhAppendStringBuilder2(&sb, L", ");
                PhAppendStringBuilder2(&sb, L"LockOperation");
            }

            hasContent = TRUE;
        }
    }

    //
    // Special flags — only when set.
    //

    if (File->IsPagingFile)
    {
        if (hasContent)
            PhAppendStringBuilder(&sb, &separator);

        if (File->IsSystemPagingFile)
            PhAppendStringBuilder2(&sb, L"PagingFile: System");
        else
            PhAppendStringBuilder2(&sb, L"PagingFile: Yes");

        hasContent = TRUE;
    }

    if (File->OriginRemote)
    {
        if (hasContent)
            PhAppendStringBuilder(&sb, &separator);
        PhAppendStringBuilder2(&sb, L"RemoteOrigin: Yes");
        hasContent = TRUE;
    }

    if (File->IgnoringSharing)
    {
        if (hasContent)
            PhAppendStringBuilder(&sb, &separator);
        PhAppendStringBuilder2(&sb, L"IgnoringSharing: Yes");
        hasContent = TRUE;
    }

    if (File->InStackFileObject)
    {
        if (hasContent)
            PhAppendStringBuilder(&sb, &separator);
        PhAppendStringBuilder2(&sb, L"InStackFileObject: Yes");
        hasContent = TRUE;
    }

    //
    // SectionObjects — if either DataSectionObject or ImageSectionObject is non-NULL.
    //

    if (File->DataSectionObject || File->ImageSectionObject)
    {
        BOOLEAN sectHasContent = FALSE;

        if (hasContent)
            PhAppendStringBuilder(&sb, &separator);

        PhAppendStringBuilder2(&sb, L"SectionObjects: ");

        if (File->DataSectionObject)
        {
            PhAppendStringBuilder2(&sb, L"Data");
            sectHasContent = TRUE;
        }

        if (File->ImageSectionObject)
        {
            if (sectHasContent)
                PhAppendStringBuilder2(&sb, L", ");
            PhAppendStringBuilder2(&sb, L"Image");
        }

        hasContent = TRUE;
    }

    //
    // Transaction — if non-NULL.
    //

    if (File->Transaction)
    {
        if (hasContent)
            PhAppendStringBuilder(&sb, &separator);
        PhAppendStringBuilder2(&sb, L"Transaction: Yes");
        hasContent = TRUE;
    }

    //
    // OpenedAsCopy — if either bit set.
    //

    if (File->OpenedAsCopySource || File->OpenedAsCopyDestination)
    {
        BOOLEAN copyHasContent = FALSE;

        if (hasContent)
            PhAppendStringBuilder(&sb, &separator);

        PhAppendStringBuilder2(&sb, L"OpenedAsCopy: ");

        if (File->OpenedAsCopySource)
        {
            PhAppendStringBuilder2(&sb, L"Source");
            copyHasContent = TRUE;
        }

        if (File->OpenedAsCopyDestination)
        {
            if (copyHasContent)
                PhAppendStringBuilder2(&sb, L", ");
            PhAppendStringBuilder2(&sb, L"Destination");
        }

        hasContent = TRUE;
    }

    if (!hasContent)
    {
        PhDeleteStringBuilder(&sb);
        return NULL;
    }

    return PhFinalStringBuilderString(&sb);
}

PPH_STRING PhpInformerFormatDetailsText(
    _In_ PPH_INFORMERW_CONTEXT Context,
    _In_ PCKPH_MESSAGE Message
    )
{
    UNICODE_STRING str;
    KPH_MESSAGE_ID msgId = Message->Header.MessageId;

    switch (msgId)
    {
    //
    // Process events
    //

    case KphMsgProcessCreate:
        {
            HANDLE targetPid = Message->Kernel.ProcessCreate.TargetProcessId;
            PPH_STRING cmdLine = NULL;
            PH_FORMAT format[4];
            ULONG count = 0;
            PPH_STRING result;

            if (NT_SUCCESS(KphMsgDynGetUnicodeString(Message, KphMsgFieldCommandLine, &str)) && str.Length > 0)
                cmdLine = PhCreateStringFromUnicodeString(&str);

            PhInitFormatS(&format[count++], L"Target PID: ");
            PhInitFormatU(&format[count++], HandleToUlong(targetPid));

            if (cmdLine)
            {
                PhInitFormatS(&format[count++], L", ");
                PhInitFormatSR(&format[count++], cmdLine->sr);
            }

            result = PhFormat(format, count, 80);
            PhClearReference(&cmdLine);
            return result;
        }

    case KphMsgProcessExit:
        {
            NTSTATUS exitStatus = Message->Kernel.ProcessExit.ExitStatus;
            PPH_STRING statusMsg = PhGetStatusMessage(exitStatus, 0);
            PH_FORMAT format[5];
            ULONG count = 0;
            PPH_STRING result;

            PhInitFormatS(&format[count++], L"Exit: 0x");
            PhInitFormatI64XWithWidth(&format[count++], (ULONG64)(ULONG)exitStatus, 8);

            if (statusMsg)
            {
                PhInitFormatS(&format[count++], L" (");
                PhInitFormatSR(&format[count++], statusMsg->sr);
                PhInitFormatC(&format[count++], L')');
            }

            result = PhFormat(format, count, 60);
            PhClearReference(&statusMsg);
            return result;
        }

    //
    // Thread events
    //

    case KphMsgThreadCreate:
        {
            HANDLE targetPid = Message->Kernel.ThreadCreate.TargetClientId.UniqueProcess;
            HANDLE targetTid = Message->Kernel.ThreadCreate.TargetClientId.UniqueThread;
            PPH_STRING targetName = PhpInformerGetTargetProcessName(Context, targetPid);
            PH_FORMAT format[7];
            ULONG count = 0;
            PPH_STRING result;

            PhInitFormatS(&format[count++], L"Target: ");

            if (targetName)
            {
                PhInitFormatSR(&format[count++], targetName->sr);
                PhInitFormatS(&format[count++], L" (");
            }
            else
            {
                PhInitFormatS(&format[count++], L"Non-existent process (");
            }

            PhInitFormatU(&format[count++], HandleToUlong(targetPid));
            PhInitFormatC(&format[count++], L'.');
            PhInitFormatU(&format[count++], HandleToUlong(targetTid));
            PhInitFormatC(&format[count++], L')');

            result = PhFormat(format, count, 80);
            PhClearReference(&targetName);
            return result;
        }

    case KphMsgThreadExit:
        {
            NTSTATUS exitStatus = Message->Kernel.ThreadExit.ExitStatus;
            PPH_STRING statusMsg = PhGetStatusMessage(exitStatus, 0);
            PH_FORMAT format[5];
            ULONG count = 0;
            PPH_STRING result;

            PhInitFormatS(&format[count++], L"Exit: 0x");
            PhInitFormatI64XWithWidth(&format[count++], (ULONG64)(ULONG)exitStatus, 8);

            if (statusMsg)
            {
                PhInitFormatS(&format[count++], L" (");
                PhInitFormatSR(&format[count++], statusMsg->sr);
                PhInitFormatC(&format[count++], L')');
            }

            result = PhFormat(format, count, 60);
            PhClearReference(&statusMsg);
            return result;
        }

    //
    // Image events
    //

    case KphMsgImageLoad:
        {
            PH_FORMAT format[2];

            PhInitFormatS(&format[0], L"Base: 0x");
            PhInitFormatIXPadZeros(&format[1], (ULONG_PTR)Message->Kernel.ImageLoad.ImageBase);

            return PhFormat(format, 2, 30);
        }

    case KphMsgImageVerify:
        {
            PH_FORMAT format[4];

            PhInitFormatS(&format[0], L"Type: ");
            PhInitFormatSR(&format[1], *PhpInformerGetImageTypeName(Message->Kernel.ImageVerify.ImageType));
            PhInitFormatS(&format[2], L", Classification: ");
            PhInitFormatSR(&format[3], *PhpInformerGetClassificationName(Message->Kernel.ImageVerify.Classification));

            return PhFormat(format, 4, 60);
        }

    //
    // Registry events
    //

    case KphMsgRegPreSetValueKey:
        {
            PH_FORMAT format[4];

            PhInitFormatS(&format[0], L"Type: ");
            PhInitFormatSR(&format[1], *PhpInformerGetRegTypeName(Message->Kernel.Reg.Parameters.SetValueKey.Type));
            PhInitFormatS(&format[2], L", Size: ");
            PhInitFormatU(&format[3], Message->Kernel.Reg.Parameters.SetValueKey.DataSize);

            return PhFormat(format, 4, 40);
        }

    case KphMsgRegPreDeleteValueKey:
        break;

    case KphMsgRegPreQueryValueKey:
        break;

    case KphMsgRegPreEnumerateKey:
        {
            PH_FORMAT format[2];

            PhInitFormatS(&format[0], L"Index: ");
            PhInitFormatU(&format[1], Message->Kernel.Reg.Parameters.EnumerateKey.Index);

            return PhFormat(format, 2, 20);
        }

    case KphMsgRegPreEnumerateValueKey:
        {
            PH_FORMAT format[2];

            PhInitFormatS(&format[0], L"Index: ");
            PhInitFormatU(&format[1], Message->Kernel.Reg.Parameters.EnumerateValueKey.Index);

            return PhFormat(format, 2, 20);
        }

    case KphMsgRegPreRenameKey:
        if (NT_SUCCESS(KphMsgDynGetUnicodeString(Message, KphMsgFieldNewName, &str)) && str.Length > 0)
            return PhCreateStringFromUnicodeString(&str);
        break;

    //
    // Handle events — process
    //

    case KphMsgHandlePreCreateProcess:
        {
            HANDLE targetPid = Message->Kernel.Handle.Pre.Create.Process.ProcessId;
            ACCESS_MASK desiredAccess = Message->Kernel.Handle.Pre.DesiredAccess;
            PPH_STRING targetName = PhpInformerGetTargetProcessName(Context, targetPid);
            PPH_STRING accessStr = PhpInformerFormatAccessMask(L"Process", desiredAccess);
            PH_FORMAT format[7];
            ULONG count = 0;
            PPH_STRING result;

            PhInitFormatS(&format[count++], L"Target: ");

            if (targetName)
            {
                PhInitFormatSR(&format[count++], targetName->sr);
                PhInitFormatS(&format[count++], L" (");
            }
            else
            {
                PhInitFormatS(&format[count++], L"Non-existent process (");
            }

            PhInitFormatU(&format[count++], HandleToUlong(targetPid));
            PhInitFormatS(&format[count++], L"), Access: ");
            PhInitFormatSR(&format[count++], accessStr->sr);

            result = PhFormat(format, count, 100);
            PhClearReference(&targetName);
            PhDereferenceObject(accessStr);
            return result;
        }

    case KphMsgHandlePreDuplicateProcess:
        {
            HANDLE targetPid = Message->Kernel.Handle.Pre.Duplicate.Process.ProcessId;
            HANDLE sourcePid = Message->Kernel.Handle.Pre.Duplicate.SourceProcessId;
            HANDLE destPid = Message->Kernel.Handle.Pre.Duplicate.TargetProcessId;
            ACCESS_MASK desiredAccess = Message->Kernel.Handle.Pre.DesiredAccess;
            PPH_STRING targetName = PhpInformerGetTargetProcessName(Context, targetPid);
            PPH_STRING accessStr = PhpInformerFormatAccessMask(L"Process", desiredAccess);
            PH_FORMAT format[11];
            ULONG count = 0;
            PPH_STRING result;

            PhInitFormatS(&format[count++], L"Target: ");

            if (targetName)
            {
                PhInitFormatSR(&format[count++], targetName->sr);
                PhInitFormatS(&format[count++], L" (");
            }
            else
            {
                PhInitFormatS(&format[count++], L"Non-existent process (");
            }

            PhInitFormatU(&format[count++], HandleToUlong(targetPid));
            PhInitFormatS(&format[count++], L"), Source PID: ");
            PhInitFormatU(&format[count++], HandleToUlong(sourcePid));
            PhInitFormatS(&format[count++], L", Dest PID: ");
            PhInitFormatU(&format[count++], HandleToUlong(destPid));
            PhInitFormatS(&format[count++], L", Access: ");
            PhInitFormatSR(&format[count++], accessStr->sr);

            result = PhFormat(format, count, 120);
            PhClearReference(&targetName);
            PhDereferenceObject(accessStr);
            return result;
        }

    //
    // Handle events — thread
    //

    case KphMsgHandlePreCreateThread:
        {
            CLIENT_ID targetCid = Message->Kernel.Handle.Pre.Create.Thread.ClientId;
            ACCESS_MASK desiredAccess = Message->Kernel.Handle.Pre.DesiredAccess;
            PPH_STRING targetName = PhpInformerGetTargetProcessName(Context, targetCid.UniqueProcess);
            PPH_STRING accessStr = PhpInformerFormatAccessMask(L"Thread", desiredAccess);
            PH_FORMAT format[9];
            ULONG count = 0;
            PPH_STRING result;

            PhInitFormatS(&format[count++], L"Target: ");

            if (targetName)
            {
                PhInitFormatSR(&format[count++], targetName->sr);
                PhInitFormatS(&format[count++], L" (");
            }
            else
            {
                PhInitFormatS(&format[count++], L"Non-existent process (");
            }

            PhInitFormatU(&format[count++], HandleToUlong(targetCid.UniqueProcess));
            PhInitFormatC(&format[count++], L'.');
            PhInitFormatU(&format[count++], HandleToUlong(targetCid.UniqueThread));
            PhInitFormatS(&format[count++], L"), Access: ");
            PhInitFormatSR(&format[count++], accessStr->sr);

            result = PhFormat(format, count, 100);
            PhClearReference(&targetName);
            PhDereferenceObject(accessStr);
            return result;
        }

    case KphMsgHandlePreDuplicateThread:
        {
            CLIENT_ID targetCid = Message->Kernel.Handle.Pre.Duplicate.Thread.ClientId;
            HANDLE sourcePid = Message->Kernel.Handle.Pre.Duplicate.SourceProcessId;
            HANDLE destPid = Message->Kernel.Handle.Pre.Duplicate.TargetProcessId;
            ACCESS_MASK desiredAccess = Message->Kernel.Handle.Pre.DesiredAccess;
            PPH_STRING targetName = PhpInformerGetTargetProcessName(Context, targetCid.UniqueProcess);
            PPH_STRING accessStr = PhpInformerFormatAccessMask(L"Thread", desiredAccess);
            PH_FORMAT format[13];
            ULONG count = 0;
            PPH_STRING result;

            PhInitFormatS(&format[count++], L"Target: ");

            if (targetName)
            {
                PhInitFormatSR(&format[count++], targetName->sr);
                PhInitFormatS(&format[count++], L" (");
            }
            else
            {
                PhInitFormatS(&format[count++], L"Non-existent process (");
            }

            PhInitFormatU(&format[count++], HandleToUlong(targetCid.UniqueProcess));
            PhInitFormatC(&format[count++], L'.');
            PhInitFormatU(&format[count++], HandleToUlong(targetCid.UniqueThread));
            PhInitFormatS(&format[count++], L"), Source PID: ");
            PhInitFormatU(&format[count++], HandleToUlong(sourcePid));
            PhInitFormatS(&format[count++], L", Dest PID: ");
            PhInitFormatU(&format[count++], HandleToUlong(destPid));
            PhInitFormatS(&format[count++], L", Access: ");
            PhInitFormatSR(&format[count++], accessStr->sr);

            result = PhFormat(format, count, 140);
            PhClearReference(&targetName);
            PhDereferenceObject(accessStr);
            return result;
        }

    //
    // Handle events — desktop
    //

    case KphMsgHandlePreCreateDesktop:
        {
            ACCESS_MASK desiredAccess = Message->Kernel.Handle.Pre.DesiredAccess;
            PPH_STRING accessStr = PhpInformerFormatAccessMask(L"Desktop", desiredAccess);
            PPH_STRING objectName = NULL;
            PH_FORMAT format[4];
            ULONG count = 0;
            PPH_STRING result;

            if (NT_SUCCESS(KphMsgDynGetUnicodeString(Message, KphMsgFieldObjectName, &str)) && str.Length > 0)
                objectName = PhCreateStringFromUnicodeString(&str);

            if (objectName)
            {
                PhInitFormatSR(&format[count++], objectName->sr);
                PhInitFormatS(&format[count++], L", ");
            }

            PhInitFormatS(&format[count++], L"Access: ");
            PhInitFormatSR(&format[count++], accessStr->sr);

            result = PhFormat(format, count, 80);
            PhClearReference(&objectName);
            PhDereferenceObject(accessStr);
            return result;
        }

    case KphMsgHandlePreDuplicateDesktop:
        {
            HANDLE sourcePid = Message->Kernel.Handle.Pre.Duplicate.SourceProcessId;
            HANDLE destPid = Message->Kernel.Handle.Pre.Duplicate.TargetProcessId;
            ACCESS_MASK desiredAccess = Message->Kernel.Handle.Pre.DesiredAccess;
            PPH_STRING accessStr = PhpInformerFormatAccessMask(L"Desktop", desiredAccess);
            PPH_STRING objectName = NULL;
            PH_FORMAT format[8];
            ULONG count = 0;
            PPH_STRING result;

            if (NT_SUCCESS(KphMsgDynGetUnicodeString(Message, KphMsgFieldObjectName, &str)) && str.Length > 0)
                objectName = PhCreateStringFromUnicodeString(&str);

            if (objectName)
            {
                PhInitFormatSR(&format[count++], objectName->sr);
                PhInitFormatS(&format[count++], L", ");
            }

            PhInitFormatS(&format[count++], L"Source PID: ");
            PhInitFormatU(&format[count++], HandleToUlong(sourcePid));
            PhInitFormatS(&format[count++], L", Dest PID: ");
            PhInitFormatU(&format[count++], HandleToUlong(destPid));
            PhInitFormatS(&format[count++], L", Access: ");
            PhInitFormatSR(&format[count++], accessStr->sr);

            result = PhFormat(format, count, 100);
            PhClearReference(&objectName);
            PhDereferenceObject(accessStr);
            return result;
        }

    default:
        break;
    }

    //
    // File events — keyed by MajorFunction within the file message range.
    //

    if (msgId >= KphMsgFilePreCreate && msgId <= KphMsgFilePostVolumeDismount)
    {
        UCHAR majorFunction = Message->Kernel.File.MajorFunction;
        PPH_STRING perOpDetails = NULL;

        switch (majorFunction)
        {
        case IRP_MJ_CREATE:
            {
                static const PH_STRINGREF shareNames[] =
                {
                    PH_STRINGREF_INIT(L"None"),                 // 0
                    PH_STRINGREF_INIT(L"Read"),                 // 1
                    PH_STRINGREF_INIT(L"Write"),                // 2
                    PH_STRINGREF_INIT(L"Read, Write"),          // 3
                    PH_STRINGREF_INIT(L"Delete"),               // 4
                    PH_STRINGREF_INIT(L"Read, Delete"),         // 5
                    PH_STRINGREF_INIT(L"Write, Delete"),        // 6
                    PH_STRINGREF_INIT(L"Read, Write, Delete"),  // 7
                };

#define PH_CREATE_OPT(x, n) { TEXT(#x), x, FALSE, FALSE, n }
                static const PH_ACCESS_ENTRY createOptionEntries[] =
                {
                    PH_CREATE_OPT(FILE_DIRECTORY_FILE, L"Directory"),
                    PH_CREATE_OPT(FILE_WRITE_THROUGH, L"Write through"),
                    PH_CREATE_OPT(FILE_SEQUENTIAL_ONLY, L"Sequential"),
                    PH_CREATE_OPT(FILE_NO_INTERMEDIATE_BUFFERING, L"No buffering"),
                    PH_CREATE_OPT(FILE_SYNCHRONOUS_IO_ALERT, L"Sync I/O alert"),
                    PH_CREATE_OPT(FILE_SYNCHRONOUS_IO_NONALERT, L"Sync I/O"),
                    PH_CREATE_OPT(FILE_NON_DIRECTORY_FILE, L"Non-directory"),
                    PH_CREATE_OPT(FILE_CREATE_TREE_CONNECTION, L"Tree connect"),
                    PH_CREATE_OPT(FILE_COMPLETE_IF_OPLOCKED, L"Complete if oplocked"),
                    PH_CREATE_OPT(FILE_NO_EA_KNOWLEDGE, L"No EA knowledge"),
                    PH_CREATE_OPT(FILE_OPEN_REMOTE_INSTANCE, L"Remote instance"),
                    PH_CREATE_OPT(FILE_RANDOM_ACCESS, L"Random"),
                    PH_CREATE_OPT(FILE_DELETE_ON_CLOSE, L"Delete on close"),
                    PH_CREATE_OPT(FILE_OPEN_BY_FILE_ID, L"By file ID"),
                    PH_CREATE_OPT(FILE_OPEN_FOR_BACKUP_INTENT, L"Backup intent"),
                    PH_CREATE_OPT(FILE_NO_COMPRESSION, L"No compression"),
                    PH_CREATE_OPT(FILE_OPEN_REQUIRING_OPLOCK, L"Requiring oplock"),
                    PH_CREATE_OPT(FILE_DISALLOW_EXCLUSIVE, L"Disallow exclusive"),
                    PH_CREATE_OPT(FILE_SESSION_AWARE, L"Session aware"),
                    PH_CREATE_OPT(FILE_RESERVE_OPFILTER, L"Reserve opfilter"),
                    PH_CREATE_OPT(FILE_OPEN_REPARSE_POINT, L"Reparse point"),
                    PH_CREATE_OPT(FILE_OPEN_NO_RECALL, L"No recall"),
                    PH_CREATE_OPT(FILE_OPEN_FOR_FREE_SPACE_QUERY, L"Free space query"),
                };
#undef PH_CREATE_OPT

#define PH_FILE_ATTR(x, n) { TEXT(#x), x, FALSE, FALSE, n }
                static const PH_ACCESS_ENTRY fileAttributeEntries[] =
                {
                    PH_FILE_ATTR(FILE_ATTRIBUTE_READONLY, L"Read only"),
                    PH_FILE_ATTR(FILE_ATTRIBUTE_HIDDEN, L"Hidden"),
                    PH_FILE_ATTR(FILE_ATTRIBUTE_SYSTEM, L"System"),
                    PH_FILE_ATTR(FILE_ATTRIBUTE_DIRECTORY, L"Directory"),
                    PH_FILE_ATTR(FILE_ATTRIBUTE_ARCHIVE, L"Archive"),
                    PH_FILE_ATTR(FILE_ATTRIBUTE_NORMAL, L"Normal"),
                    PH_FILE_ATTR(FILE_ATTRIBUTE_TEMPORARY, L"Temporary"),
                    PH_FILE_ATTR(FILE_ATTRIBUTE_SPARSE_FILE, L"Sparse"),
                    PH_FILE_ATTR(FILE_ATTRIBUTE_REPARSE_POINT, L"Reparse point"),
                    PH_FILE_ATTR(FILE_ATTRIBUTE_COMPRESSED, L"Compressed"),
                    PH_FILE_ATTR(FILE_ATTRIBUTE_OFFLINE, L"Offline"),
                    PH_FILE_ATTR(FILE_ATTRIBUTE_NOT_CONTENT_INDEXED, L"Not indexed"),
                    PH_FILE_ATTR(FILE_ATTRIBUTE_ENCRYPTED, L"Encrypted"),
                };
#undef PH_FILE_ATTR

                ULONG options = Message->Kernel.File.Parameters.Create.Options;
                ULONG disposition = options >> 24;
                ULONG createOptions = options & 0x00FFFFFF;
                USHORT shareAccess = Message->Kernel.File.Parameters.Create.ShareAccess;
                USHORT fileAttributes = Message->Kernel.File.Parameters.Create.FileAttributes;
                PH_STRING_BUILDER createSb;
                PPH_STRING optionsStr;
                PPH_STRING attribStr;

                PhInitializeStringBuilder(&createSb, 120);

                //
                // Disposition
                //

                PhAppendStringBuilder2(&createSb, L"Disposition: ");
                PhAppendStringBuilder(&createSb, PhpInformerGetDispositionName(disposition));

                //
                // CreateOptions — symbolicated.
                //

                if (createOptions)
                {
                    PH_FORMAT hexFmt[4];

                    static const PH_STRINGREF optSep = PH_STRINGREF_INIT(L", Options: ");
                    PhAppendStringBuilder(&createSb, &optSep);
                    optionsStr = PhGetAccessString(createOptions, (PPH_ACCESS_ENTRY)createOptionEntries, RTL_NUMBER_OF(createOptionEntries));

                    PhInitFormatSR(&hexFmt[0], optionsStr->sr);
                    PhInitFormatS(&hexFmt[1], L" (0x");
                    PhInitFormatI64XWithWidth(&hexFmt[2], (ULONG64)createOptions, 6);
                    PhInitFormatC(&hexFmt[3], L')');
                    PhDereferenceObject(optionsStr);
                    optionsStr = PhFormat(hexFmt, 4, 40);

                    PhAppendStringBuilder(&createSb, &optionsStr->sr);
                    PhDereferenceObject(optionsStr);
                }

                //
                // ShareMode
                //

                {
                    static const PH_STRINGREF shareSep = PH_STRINGREF_INIT(L", ShareMode: ");
                    PhAppendStringBuilder(&createSb, &shareSep);
                    PhAppendStringBuilder(&createSb, &shareNames[shareAccess & 7]);
                }

                //
                // FileAttributes — symbolicated.
                //

                if (fileAttributes && fileAttributes != FILE_ATTRIBUTE_NORMAL)
                {
                    PH_FORMAT hexFmt[4];

                    static const PH_STRINGREF attrSep = PH_STRINGREF_INIT(L", Attributes: ");
                    PhAppendStringBuilder(&createSb, &attrSep);
                    attribStr = PhGetAccessString(fileAttributes, (PPH_ACCESS_ENTRY)fileAttributeEntries, RTL_NUMBER_OF(fileAttributeEntries));

                    PhInitFormatSR(&hexFmt[0], attribStr->sr);
                    PhInitFormatS(&hexFmt[1], L" (0x");
                    PhInitFormatI64XWithWidth(&hexFmt[2], (ULONG64)fileAttributes, 4);
                    PhInitFormatC(&hexFmt[3], L')');
                    PhDereferenceObject(attribStr);
                    attribStr = PhFormat(hexFmt, 4, 30);

                    PhAppendStringBuilder(&createSb, &attribStr->sr);
                    PhDereferenceObject(attribStr);
                }

                perOpDetails = PhFinalStringBuilderString(&createSb);
            }
            break;

        case IRP_MJ_READ:
            {
                PH_FORMAT format[4];

                PhInitFormatS(&format[0], L"Offset: ");
                PhInitFormatI64D(&format[1], Message->Kernel.File.Parameters.Read.ByteOffset.QuadPart);
                PhInitFormatS(&format[2], L", Length: ");
                PhInitFormatU(&format[3], Message->Kernel.File.Parameters.Read.Length);

                perOpDetails = PhFormat(format, 4, 40);
            }
            break;

        case IRP_MJ_WRITE:
            {
                PH_FORMAT format[4];

                PhInitFormatS(&format[0], L"Offset: ");
                PhInitFormatI64D(&format[1], Message->Kernel.File.Parameters.Write.ByteOffset.QuadPart);
                PhInitFormatS(&format[2], L", Length: ");
                PhInitFormatU(&format[3], Message->Kernel.File.Parameters.Write.Length);

                perOpDetails = PhFormat(format, 4, 40);
            }
            break;

        case IRP_MJ_QUERY_INFORMATION:
            {
                ULONG fileInfoClass = (ULONG)Message->Kernel.File.Parameters.QueryInformation.FileInformationClass;
                PCPH_STRINGREF className = PhpInformerGetFileInfoClassName(fileInfoClass);
                PH_FORMAT format[2];

                PhInitFormatS(&format[0], L"Class: ");

                if (className)
                    PhInitFormatSR(&format[1], *className);
                else
                    PhInitFormatU(&format[1], fileInfoClass);

                perOpDetails = PhFormat(format, 2, 40);
            }
            break;

        case IRP_MJ_SET_INFORMATION:
            {
                ULONG fileInfoClass = (ULONG)Message->Kernel.File.Parameters.SetInformation.FileInformationClass;
                PCPH_STRINGREF className = PhpInformerGetFileInfoClassName(fileInfoClass);
                PPH_STRING destName = NULL;
                PH_FORMAT format[5];
                ULONG count = 0;

                //
                // For rename/link operations, show the destination file name.
                //

                if (NT_SUCCESS(KphMsgDynGetUnicodeString(Message, KphMsgFieldDestinationFileName, &str)) && str.Length > 0)
                    destName = PhCreateStringFromUnicodeString(&str);

                PhInitFormatS(&format[count++], L"Class: ");

                if (className)
                    PhInitFormatSR(&format[count++], *className);
                else
                    PhInitFormatU(&format[count++], fileInfoClass);

                if (destName)
                {
                    PhInitFormatS(&format[count++], L", Dest: ");
                    PhInitFormatSR(&format[count++], destName->sr);
                }

                perOpDetails = PhFormat(format, count, 80);
                PhClearReference(&destName);
            }
            break;

        case IRP_MJ_QUERY_VOLUME_INFORMATION:
            {
                ULONG fsInfoClass = (ULONG)Message->Kernel.File.Parameters.QueryVolumeInformation.FsInformationClass;
                PCPH_STRINGREF className = PhpInformerGetFsInfoClassName(fsInfoClass);
                PH_FORMAT format[2];

                PhInitFormatS(&format[0], L"Class: ");

                if (className)
                    PhInitFormatSR(&format[1], *className);
                else
                    PhInitFormatU(&format[1], fsInfoClass);

                perOpDetails = PhFormat(format, 2, 40);
            }
            break;

        case IRP_MJ_SET_VOLUME_INFORMATION:
            {
                ULONG fsInfoClass = (ULONG)Message->Kernel.File.Parameters.SetVolumeInformation.FsInformationClass;
                PCPH_STRINGREF className = PhpInformerGetFsInfoClassName(fsInfoClass);
                PH_FORMAT format[2];

                PhInitFormatS(&format[0], L"Class: ");

                if (className)
                    PhInitFormatSR(&format[1], *className);
                else
                    PhInitFormatU(&format[1], fsInfoClass);

                perOpDetails = PhFormat(format, 2, 40);
            }
            break;

        case IRP_MJ_DIRECTORY_CONTROL:
            {
                ULONG dirInfoClass = (ULONG)Message->Kernel.File.Parameters.DirectoryControl.QueryDirectory.FileInformationClass;
                PCPH_STRINGREF className = PhpInformerGetFileInfoClassName(dirInfoClass);
                PH_FORMAT format[2];

                PhInitFormatS(&format[0], L"Class: ");

                if (className)
                    PhInitFormatSR(&format[1], *className);
                else
                    PhInitFormatU(&format[1], dirInfoClass);

                perOpDetails = PhFormat(format, 2, 40);
            }
            break;

        case IRP_MJ_FILE_SYSTEM_CONTROL:
            {
                PH_FORMAT format[2];

                PhInitFormatS(&format[0], L"FsControlCode: 0x");
                PhInitFormatI64XWithWidth(&format[1], (ULONG64)Message->Kernel.File.Parameters.FileSystemControl.Common.FsControlCode, 8);

                perOpDetails = PhFormat(format, 2, 30);
            }
            break;

        case IRP_MJ_DEVICE_CONTROL:
            {
                PH_FORMAT format[2];

                PhInitFormatS(&format[0], L"IoControlCode: 0x");
                PhInitFormatI64XWithWidth(&format[1], (ULONG64)Message->Kernel.File.Parameters.DeviceControl.Common.IoControlCode, 8);

                perOpDetails = PhFormat(format, 2, 30);
            }
            break;

        case IRP_MJ_LOCK_CONTROL:
            {
                PH_FORMAT format[6];

                PhInitFormatS(&format[0], L"Offset: ");
                PhInitFormatI64D(&format[1], Message->Kernel.File.Parameters.LockControl.ByteOffset.QuadPart);
                PhInitFormatS(&format[2], L", Length: ");
                PhInitFormatI64D(&format[3], Message->Kernel.File.Pre.LockControl.Length.QuadPart);
                PhInitFormatS(&format[4], L", Exclusive: ");
                PhInitFormatS(&format[5], Message->Kernel.File.Parameters.LockControl.ExclusiveLock ? L"Yes" : L"No");

                perOpDetails = PhFormat(format, 6, 60);
            }
            break;
        }

        return PhpInformerAppendFileCommonDetails(&Message->Kernel.File, perOpDetails);
    }

    return NULL;
}

PPH_STRING PhpInformerGetProcessText(
    _In_ PPH_INFORMERW_CONTEXT Context,
    _In_ PCKPH_MESSAGE Message
    )
{
    HANDLE processId = NULL;
    ULONG64 processStartKey = 0;

    switch (Message->Header.MessageId)
    {
    case KphMsgProcessCreate:
        processId = Message->Kernel.ProcessCreate.CreatingClientId.UniqueProcess;
        processStartKey = Message->Kernel.ProcessCreate.CreatingProcessStartKey;
        break;
    case KphMsgProcessExit:
        processId = Message->Kernel.ProcessExit.ClientId.UniqueProcess;
        processStartKey = Message->Kernel.ProcessExit.ProcessStartKey;
        break;
    case KphMsgThreadCreate:
        processId = Message->Kernel.ThreadCreate.CreatingClientId.UniqueProcess;
        processStartKey = Message->Kernel.ThreadCreate.CreatingProcessStartKey;
        break;
    case KphMsgThreadExecute:
        processId = Message->Kernel.ThreadExecute.ClientId.UniqueProcess;
        processStartKey = Message->Kernel.ThreadExecute.ProcessStartKey;
        break;
    case KphMsgThreadExit:
        processId = Message->Kernel.ThreadExit.ClientId.UniqueProcess;
        processStartKey = Message->Kernel.ThreadExit.ProcessStartKey;
        break;
    case KphMsgImageLoad:
        processId = Message->Kernel.ImageLoad.LoadingClientId.UniqueProcess;
        processStartKey = Message->Kernel.ImageLoad.LoadingProcessStartKey;
        break;
    case KphMsgImageVerify:
        processId = Message->Kernel.ImageVerify.ClientId.UniqueProcess;
        processStartKey = Message->Kernel.ImageVerify.ProcessStartKey;
        break;
    default:
        if (Message->Header.MessageId >= KphMsgHandlePreCreateProcess &&
            Message->Header.MessageId <= KphMsgHandlePostDuplicateDesktop)
        {
            processId = Message->Kernel.Handle.ContextClientId.UniqueProcess;
            processStartKey = Message->Kernel.Handle.ContextProcessStartKey;
        }
        else if (Message->Header.MessageId >= KphMsgFilePreCreate &&
            Message->Header.MessageId <= KphMsgFilePostVolumeDismount)
        {
            processId = Message->Kernel.File.ClientId.UniqueProcess;
            processStartKey = Message->Kernel.File.ProcessStartKey;
        }
        else if (Message->Header.MessageId >= KphMsgRegPreDeleteKey &&
            Message->Header.MessageId <= KphMsgRegPostSaveMergedKey)
        {
            processId = Message->Kernel.Reg.ClientId.UniqueProcess;
            processStartKey = Message->Kernel.Reg.ProcessStartKey;
        }
        break;
    }

    if (processId)
    {
        PPH_PROCESS_ITEM processItem;

        processItem = PhpInformerReferenceProcessItem(processId, processStartKey);
        if (processItem)
        {
            PPH_STRING name = PhReferenceObject(processItem->ProcessName);
            PhDereferenceObject(processItem);
            return name;
        }

        //
        // If this is a process-scoped monitor and the event's start key
        // matches, use the cached process name to avoid showing
        // "Non-existent process" for the monitored process after it exits.
        //
        if (Context->ProcessName && processStartKey == Context->ProcessStartKey)
            return PhReferenceObject(Context->ProcessName);

        //
        // Fallback when process is not in the snapshot (e.g. already exited)
        //
        {
            PH_FORMAT format[3];

            PhInitFormatS(&format[0], L"Non-existent process (");
            PhInitFormatU(&format[1], HandleToUlong(processId));
            PhInitFormatC(&format[2], L')');

            return PhFormat(format, 3, 40);
        }
    }

    return NULL;
}

VOID PhpInformerEnsureTexts(
    _In_ PPH_INFORMERW_CONTEXT Context,
    _In_ PPH_INFORMERW_NODE Node
    )
{
    PCKPH_MESSAGE msg = Node->Message;

    if (!Node->TimeText)
        Node->TimeText = PhpInformerGetTimeText(msg);

    if (!Node->ProcessText)
        Node->ProcessText = PhpInformerGetProcessText(Context, msg);

    if (!Node->ResultText)
        Node->ResultText = PhpInformerGetImmediateResultText(msg);

    if (!Node->DetailsText)
        Node->DetailsText = PhpInformerFormatDetailsText(Context, msg);
}

HANDLE PhpInformerGetTid(
    _In_ PCKPH_MESSAGE Message
    )
{
    if (Message->Header.MessageId >= KphMsgFilePreCreate &&
        Message->Header.MessageId <= KphMsgFilePostVolumeDismount)
    {
        return Message->Kernel.File.ClientId.UniqueThread;
    }

    if (Message->Header.MessageId >= KphMsgRegPreDeleteKey &&
        Message->Header.MessageId <= KphMsgRegPostSaveMergedKey)
    {
        return Message->Kernel.Reg.ClientId.UniqueThread;
    }

    switch (Message->Header.MessageId)
    {
    case KphMsgThreadCreate:
        return Message->Kernel.ThreadCreate.CreatingClientId.UniqueThread;
    case KphMsgThreadExecute:
        return Message->Kernel.ThreadExecute.ClientId.UniqueThread;
    case KphMsgThreadExit:
        return Message->Kernel.ThreadExit.ClientId.UniqueThread;
    case KphMsgProcessCreate:
        return Message->Kernel.ProcessCreate.CreatingClientId.UniqueThread;
    case KphMsgProcessExit:
        return Message->Kernel.ProcessExit.ClientId.UniqueThread;
    case KphMsgImageLoad:
        return Message->Kernel.ImageLoad.LoadingClientId.UniqueThread;
    case KphMsgImageVerify:
        return Message->Kernel.ImageVerify.ClientId.UniqueThread;
    default:
        {
            if (Message->Header.MessageId >= KphMsgHandlePreCreateProcess &&
                Message->Header.MessageId <= KphMsgHandlePostDuplicateDesktop)
            {
                return Message->Kernel.Handle.ContextClientId.UniqueThread;
            }
        }
        break;
    }

    return NULL;
}

HANDLE PhpInformerGetPid(
    _In_ PCKPH_MESSAGE Message
    )
{
    if (Message->Header.MessageId >= KphMsgFilePreCreate &&
        Message->Header.MessageId <= KphMsgFilePostVolumeDismount)
    {
        return Message->Kernel.File.ClientId.UniqueProcess;
    }

    if (Message->Header.MessageId >= KphMsgRegPreDeleteKey &&
        Message->Header.MessageId <= KphMsgRegPostSaveMergedKey)
    {
        return Message->Kernel.Reg.ClientId.UniqueProcess;
    }

    switch (Message->Header.MessageId)
    {
    case KphMsgThreadCreate:
        return Message->Kernel.ThreadCreate.CreatingClientId.UniqueProcess;
    case KphMsgThreadExecute:
        return Message->Kernel.ThreadExecute.ClientId.UniqueProcess;
    case KphMsgThreadExit:
        return Message->Kernel.ThreadExit.ClientId.UniqueProcess;
    case KphMsgProcessCreate:
        return Message->Kernel.ProcessCreate.CreatingClientId.UniqueProcess;
    case KphMsgProcessExit:
        return Message->Kernel.ProcessExit.ClientId.UniqueProcess;
    case KphMsgImageLoad:
        return Message->Kernel.ImageLoad.LoadingClientId.UniqueProcess;
    case KphMsgImageVerify:
        return Message->Kernel.ImageVerify.ClientId.UniqueProcess;
    default:
        {
            if (Message->Header.MessageId >= KphMsgHandlePreCreateProcess &&
                Message->Header.MessageId <= KphMsgHandlePostDuplicateDesktop)
            {
                return Message->Kernel.Handle.ContextClientId.UniqueProcess;
            }
        }
        break;
    }

    return NULL;
}

_Function_class_(PH_TN_FILTER_FUNCTION)
BOOLEAN NTAPI PhpInformerSearchFilterCallback(
    _In_ PPH_TREENEW_NODE Node,
    _In_opt_ PVOID Context
    )
{
    PPH_INFORMERW_CONTEXT context = Context;
    PPH_INFORMERW_NODE node = (PPH_INFORMERW_NODE)Node;

    if (!context->SearchMatchHandle)
        return TRUE;

    PhpInformerEnsureTexts(context, node);

    if (!PhIsNullOrEmptyString(node->ProcessText) && PhSearchControlMatch(context->SearchMatchHandle, &node->ProcessText->sr))
        return TRUE;
    if (!PhIsNullOrEmptyStringRef(&node->CategoryText) && PhSearchControlMatch(context->SearchMatchHandle, &node->CategoryText))
        return TRUE;
    if (!PhIsNullOrEmptyStringRef(&node->EventText) && PhSearchControlMatch(context->SearchMatchHandle, &node->EventText))
        return TRUE;
    if (!PhIsNullOrEmptyStringRef(&node->PathText) && PhSearchControlMatch(context->SearchMatchHandle, &node->PathText))
        return TRUE;
    if (!PhIsNullOrEmptyString(node->ResultText) && PhSearchControlMatch(context->SearchMatchHandle, &node->ResultText->sr))
        return TRUE;
    if (!PhIsNullOrEmptyString(node->DetailsText) && PhSearchControlMatch(context->SearchMatchHandle, &node->DetailsText->sr))
        return TRUE;
    if (node->PidString[0] && PhSearchControlMatchZ(context->SearchMatchHandle, node->PidString))
        return TRUE;
    if (node->TidString[0] && PhSearchControlMatchZ(context->SearchMatchHandle, node->TidString))
        return TRUE;

    return FALSE;
}

_Function_class_(PH_TN_FILTER_FUNCTION)
BOOLEAN NTAPI PhpInformerCategoryFilterCallback(
    _In_ PPH_TREENEW_NODE Node,
    _In_opt_ PVOID Context
    )
{
    PPH_INFORMERW_CONTEXT context = Context;
    PPH_INFORMERW_NODE node = (PPH_INFORMERW_NODE)Node;

    NT_ASSERT(context);

    return !!(context->CategoryFilter & node->Category);
}

PPH_STRING PhpInformerFormatTooltipText(
    _In_ PPH_STRING DetailsText
    )
{
    PH_STRING_BUILDER sb;
    PWCHAR buffer;
    SIZE_T length;
    SIZE_T i;

    buffer = DetailsText->Buffer;
    length = DetailsText->Length / sizeof(WCHAR);

    if (length == 0)
        return PhReferenceObject(DetailsText);

    PhInitializeStringBuilder(&sb, DetailsText->Length + 20);

    for (i = 0; i < length; i++)
    {
        //
        // Look for ", " separator sequences. When we find one, check if the
        // text following it is a field label (alphabetic characters or spaces
        // followed by ": "). If so, replace with newline. Otherwise keep the
        // comma as-is (e.g. inside access mask lists like "Read, Write").
        //

        if (i + 2 <= length &&
            buffer[i] == L',' &&
            buffer[i + 1] == L' ')
        {
            SIZE_T j;
            BOOLEAN isFieldBoundary = FALSE;

            //
            // Scan ahead past the ", " to see if we find ": " before
            // the next "," or end of string. The label must start with
            // an alphabetic character.
            //

            j = i + 2;

            if (j < length && ((buffer[j] >= L'A' && buffer[j] <= L'Z') ||
                (buffer[j] >= L'a' && buffer[j] <= L'z')))
            {
                for (j = i + 2; j + 1 < length; j++)
                {
                    if (buffer[j] == L':' && buffer[j + 1] == L' ')
                    {
                        isFieldBoundary = TRUE;
                        break;
                    }

                    if (buffer[j] == L',')
                        break;
                }
            }

            if (isFieldBoundary)
            {
                PhAppendCharStringBuilder(&sb, L'\n');
                i++; // skip the space after comma
                continue;
            }
        }

        PhAppendCharStringBuilder(&sb, buffer[i]);
    }

    return PhFinalStringBuilderString(&sb);
}

BOOLEAN NTAPI PhpInformerTreeNewCallback(
    _In_ HWND hwnd,
    _In_ PH_TREENEW_MESSAGE Message,
    _In_ PVOID Parameter1,
    _In_ PVOID Parameter2,
    _In_opt_ PVOID Context
    )
{
    PPH_INFORMERW_CONTEXT context = Context;

    switch (Message)
    {
    case TreeNewGetChildren:
        {
            PPH_TREENEW_GET_CHILDREN getChildren = Parameter1;

            NT_ASSERT(context);

            if (!getChildren->Node)
            {
                getChildren->Children = (PPH_TREENEW_NODE *)context->NodeList->Items;
                getChildren->NumberOfChildren = context->NodeList->Count;
            }
        }
        return TRUE;

    case TreeNewIsLeaf:
        {
            PPH_TREENEW_IS_LEAF isLeaf = Parameter1;
            isLeaf->IsLeaf = TRUE;
        }
        return TRUE;

    case TreeNewGetCellText:
        {
            PPH_TREENEW_GET_CELL_TEXT getCellText = Parameter1;
            PPH_INFORMERW_NODE node = (PPH_INFORMERW_NODE)getCellText->Node;

            NT_ASSERT(context);

            PhpInformerEnsureTexts(context, node);

            switch (getCellText->Id)
            {
            case PHIC_TIME:
                getCellText->Text = PhGetStringRef(node->TimeText);
                break;
            case PHIC_PROCESS:
                getCellText->Text = PhGetStringRef(node->ProcessText);
                break;
            case PHIC_PID:
                PhInitializeStringRefLongHint(&getCellText->Text, node->PidString);
                break;
            case PHIC_TID:
                PhInitializeStringRefLongHint(&getCellText->Text, node->TidString);
                break;
            case PHIC_CATEGORY:
                getCellText->Text = node->CategoryText;
                break;
            case PHIC_EVENT:
                getCellText->Text = node->EventText;
                break;
            case PHIC_PATH:
                getCellText->Text = node->PathText;
                break;
            case PHIC_RESULT:
                getCellText->Text = PhGetStringRef(node->ResultText);
                break;
            case PHIC_DURATION:
                getCellText->Text = PhGetStringRef(node->DurationText);
                break;
            case PHIC_DETAILS:
                getCellText->Text = PhGetStringRef(node->DetailsText);
                break;
            default:
                return FALSE;
            }
        }
        return TRUE;

    case TreeNewGetCellTooltip:
        {
            PPH_TREENEW_GET_CELL_TOOLTIP getCellTooltip = Parameter1;
            PPH_INFORMERW_NODE node = (PPH_INFORMERW_NODE)getCellTooltip->Node;

            if (getCellTooltip->Column->Id != PHIC_DETAILS)
                return FALSE;

            if (PhIsNullOrEmptyString(node->DetailsText))
                return FALSE;

            if (!node->TooltipText)
                node->TooltipText = PhpInformerFormatTooltipText(node->DetailsText);

            if (!PhIsNullOrEmptyString(node->TooltipText))
            {
                getCellTooltip->Text = node->TooltipText->sr;
                getCellTooltip->Unfolding = FALSE;
                getCellTooltip->MaximumWidth = ULONG_MAX;
            }
            else
            {
                return FALSE;
            }
        }
        return TRUE;

    case TreeNewKeyDown:
        {
            PPH_TREENEW_KEY_EVENT keyEvent = Parameter1;

            NT_ASSERT(context);

            switch (keyEvent->VirtualKey)
            {
            case 'C':
                if (GetKeyState(VK_CONTROL) < 0)
                {
                    PPH_STRING text;

                    text = PhGetTreeNewText(context->TreeNewHandle, 0);
                    PhSetClipboardString(context->TreeNewHandle, &text->sr);
                    PhDereferenceObject(text);
                }
                break;
            }
        }
        return TRUE;

    case TreeNewContextMenu:
        {
            PPH_TREENEW_CONTEXT_MENU contextMenu = Parameter1;
            PPH_EMENU menu;
            PPH_EMENU_ITEM selectedItem;

            NT_ASSERT(context);

            menu = PhCreateEMenu();
            PhInsertEMenuItem(menu, PhCreateEMenuItem(0, IDC_COPY, L"&Copy\bCtrl+C", NULL, NULL), ULONG_MAX);
            PhInsertCopyCellEMenuItem(menu, IDC_COPY, context->TreeNewHandle, contextMenu->Column);

            selectedItem = PhShowEMenu(
                menu,
                hwnd,
                PH_EMENU_SHOW_LEFTRIGHT,
                PH_ALIGN_LEFT | PH_ALIGN_TOP,
                contextMenu->Location.x,
                contextMenu->Location.y
                );

            if (selectedItem && selectedItem->Id != ULONG_MAX)
            {
                if (!PhHandleCopyCellEMenuItem(selectedItem))
                {
                    if (selectedItem->Id == IDC_COPY)
                    {
                        PPH_STRING text;

                        text = PhGetTreeNewText(context->TreeNewHandle, 0);
                        PhSetClipboardString(context->TreeNewHandle, &text->sr);
                        PhDereferenceObject(text);
                    }
                }
            }

            PhDestroyEMenu(menu);
        }
        return TRUE;
    }

    return FALSE;
}

_Function_class_(PH_CM_POST_SORT_FUNCTION)
LONG NTAPI PhpInformerPostSortFunction(
    _In_ LONG Result,
    _In_ PVOID Node1,
    _In_ PVOID Node2,
    _In_ PH_SORT_ORDER SortOrder
    )
{
    if (Result == 0)
    {
        Result = int64cmp(
            ((PPH_INFORMERW_NODE)Node1)->Message->Header.TimeStamp.QuadPart,
            ((PPH_INFORMERW_NODE)Node2)->Message->Header.TimeStamp.QuadPart
            );
    }

    return PhModifySort(Result, SortOrder);
}

VOID PhpInformerInitializeColumns(
    _In_ PPH_INFORMERW_CONTEXT Context,
    _In_ PCWSTR ColumnSettingName
    )
{
    HWND tn = Context->TreeNewHandle;
    PPH_STRING colStr;

    TreeNew_SetSort(tn, PHIC_TIME, NoSortOrder);

    PhAddTreeNewColumn(tn, PHIC_TIME, TRUE, L"Time", 140, PH_ALIGN_RIGHT, 0, DT_RIGHT);
    PhAddTreeNewColumn(tn, PHIC_DURATION, TRUE, L"Duration", 70, PH_ALIGN_RIGHT, 1, DT_RIGHT);
    PhAddTreeNewColumn(tn, PHIC_PROCESS,TRUE, L"Process", 120, PH_ALIGN_LEFT, 2, 0);
    PhAddTreeNewColumn(tn, PHIC_PID, TRUE, L"PID", 50, PH_ALIGN_RIGHT, 3, DT_RIGHT);
    PhAddTreeNewColumn(tn, PHIC_TID, TRUE, L"TID", 50, PH_ALIGN_RIGHT, 4, DT_RIGHT);
    PhAddTreeNewColumn(tn, PHIC_CATEGORY, TRUE, L"Category", 60, PH_ALIGN_LEFT, 5, 0);
    PhAddTreeNewColumn(tn, PHIC_EVENT, TRUE, L"Event", 100, PH_ALIGN_LEFT, 6, 0);
    PhAddTreeNewColumn(tn, PHIC_PATH, TRUE, L"Path", 200, PH_ALIGN_LEFT, 7, 0);
    PhAddTreeNewColumn(tn, PHIC_RESULT, TRUE, L"Result", 60, PH_ALIGN_LEFT, 8, 0);
    PhAddTreeNewColumn(tn, PHIC_DETAILS, TRUE, L"Details", 200, PH_ALIGN_LEFT, 9, 0);

    PhCmInitializeManager(&Context->Cm, tn, PHIC_MAXIMUM, PhpInformerPostSortFunction);

    colStr = PhGetStringSetting(ColumnSettingName);
    PhCmLoadSettingsEx(tn, &Context->Cm, 0, &colStr->sr, NULL);
    PhDereferenceObject(colStr);

    PhInitializeTreeNewFilterSupport(&Context->TreeFilterSupport, tn, Context->NodeList);
    Context->SearchFilterEntry = PhAddTreeNewFilter(&Context->TreeFilterSupport, PhpInformerSearchFilterCallback, Context);
    Context->CategoryFilterEntry = PhAddTreeNewFilter(&Context->TreeFilterSupport, PhpInformerCategoryFilterCallback, Context);
}

VOID PhpInformerSaveColumns(
    _In_ PPH_INFORMERW_CONTEXT Context,
    _In_ PCWSTR ColumnSettingName
    )
{
    PPH_STRING colStr;

    colStr = PhCmSaveSettingsEx(Context->TreeNewHandle, &Context->Cm, 0, NULL);

    if (colStr)
    {
        PhSetStringSetting2(ColumnSettingName, &colStr->sr);
        PhDereferenceObject(colStr);
    }
}

BOOLEAN PhpInformerIsPostOp(
    _In_ PCKPH_MESSAGE Message,
    _Out_ PULONG64 PreSequence,
    _Out_ PNTSTATUS ResultStatus,
    _Out_ PLARGE_INTEGER PreTimeStamp
    )
{
    KPH_MESSAGE_ID msgId = Message->Header.MessageId;

    *PreSequence = 0;
    *ResultStatus = STATUS_PENDING;
    PreTimeStamp->QuadPart = 0;

    if (msgId >= KphMsgFilePreCreate && msgId <= KphMsgFilePostVolumeDismount)
    {
        if (FlagOn(Message->Kernel.File.FltFlags, FLTFL_CALLBACK_DATA_POST_OPERATION))
        {
            *PreSequence = Message->Kernel.File.Post.PreSequence;
            *ResultStatus = Message->Kernel.File.Post.IoStatus.Status;
            *PreTimeStamp = Message->Kernel.File.Post.PreTimeStamp;
            return TRUE;
        }
    }
    else if (msgId >= KphMsgRegPreDeleteKey && msgId <= KphMsgRegPostSaveMergedKey)
    {
        if (Message->Kernel.Reg.PostOperation)
        {
            *PreSequence = Message->Kernel.Reg.Post.PreSequence;
            *ResultStatus = Message->Kernel.Reg.Post.Status;
            *PreTimeStamp = Message->Kernel.Reg.Post.PreTimeStamp;
            return TRUE;
        }
    }
    else if (msgId >= KphMsgHandlePreCreateProcess && msgId <= KphMsgHandlePostDuplicateDesktop)
    {
        if (Message->Kernel.Handle.PostOperation)
        {
            *PreSequence = Message->Kernel.Handle.Post.PreSequence;
            *ResultStatus = Message->Kernel.Handle.Post.ReturnStatus;
            *PreTimeStamp = Message->Kernel.Handle.Post.PreTimeStamp;
            return TRUE;
        }
    }

    return FALSE;
}

ULONG64 PhpInformerGetPreOpSequence(
    _In_ PCKPH_MESSAGE Message
    )
{
    KPH_MESSAGE_ID msgId = Message->Header.MessageId;

    if (msgId >= KphMsgFilePreCreate && msgId <= KphMsgFilePostVolumeDismount)
    {
        if (!FlagOn(Message->Kernel.File.FltFlags, FLTFL_CALLBACK_DATA_POST_OPERATION))
            return Message->Kernel.File.Sequence;
    }
    else if (msgId >= KphMsgRegPreDeleteKey && msgId <= KphMsgRegPostSaveMergedKey)
    {
        if (!Message->Kernel.Reg.PostOperation)
            return Message->Kernel.Reg.Sequence;
    }
    else if (msgId >= KphMsgHandlePreCreateProcess && msgId <= KphMsgHandlePostDuplicateDesktop)
    {
        if (!Message->Kernel.Handle.PostOperation)
            return Message->Kernel.Handle.Sequence;
    }

    return 0;
}

PCPH_STRINGREF PhpInformerGetOpenResultName(
    _In_ ULONG_PTR Information
    )
{
    static const PH_STRINGREF superseded = PH_STRINGREF_INIT(L"Superseded");
    static const PH_STRINGREF opened = PH_STRINGREF_INIT(L"Opened");
    static const PH_STRINGREF created = PH_STRINGREF_INIT(L"Created");
    static const PH_STRINGREF overwritten = PH_STRINGREF_INIT(L"Overwritten");
    static const PH_STRINGREF exists = PH_STRINGREF_INIT(L"Exists");
    static const PH_STRINGREF doesNotExist = PH_STRINGREF_INIT(L"Does Not Exist");

    switch (Information)
    {
    case FILE_SUPERSEDED:
        return &superseded;
    case FILE_OPENED:
        return &opened;
    case FILE_CREATED:
        return &created;
    case FILE_OVERWRITTEN:
        return &overwritten;
    case FILE_EXISTS:
        return &exists;
    case FILE_DOES_NOT_EXIST:
        return &doesNotExist;
    default:
        return NULL;
    }
}

VOID PhpInformerUpdateDetailsFromPostOp(
    _In_ PPH_INFORMERW_NODE PreNode,
    _In_ PCKPH_MESSAGE PostMsg
    )
{
    static const PH_STRINGREF grantedPrefix = PH_STRINGREF_INIT(L"Granted: ");
    static const PH_STRINGREF dispCreated = PH_STRINGREF_INIT(L"Disposition: Created");
    static const PH_STRINGREF dispOpened = PH_STRINGREF_INIT(L"Disposition: Opened");
    static const PH_STRINGREF openResultPrefix = PH_STRINGREF_INIT(L"OpenResult: ");
    static const PH_STRINGREF separator = PH_STRINGREF_INIT(L", ");
    KPH_MESSAGE_ID msgId = PostMsg->Header.MessageId;

    //
    // Handle post-ops: append granted access.
    //

    switch (msgId)
    {
    case KphMsgHandlePostCreateProcess:
    case KphMsgHandlePostDuplicateProcess:
    case KphMsgHandlePostCreateThread:
    case KphMsgHandlePostDuplicateThread:
    case KphMsgHandlePostCreateDesktop:
    case KphMsgHandlePostDuplicateDesktop:
        {
            PCWSTR typeName;
            ACCESS_MASK granted = PostMsg->Kernel.Handle.Post.GrantedAccess;
            PPH_STRING accessStr;
            PH_FORMAT format[4];
            ULONG count = 0;

            switch (msgId)
            {
            case KphMsgHandlePostCreateProcess:
            case KphMsgHandlePostDuplicateProcess:
                typeName = L"Process";
                break;
            case KphMsgHandlePostCreateThread:
            case KphMsgHandlePostDuplicateThread:
                typeName = L"Thread";
                break;
            default:
                typeName = L"Desktop";
                break;
            }

            accessStr = PhpInformerFormatAccessMask(typeName, granted);

            if (!PhIsNullOrEmptyString(PreNode->DetailsText))
            {
                PhInitFormatSR(&format[count++], PreNode->DetailsText->sr);
                PhInitFormatSR(&format[count++], separator);
            }

            PhInitFormatSR(&format[count++], grantedPrefix);
            PhInitFormatSR(&format[count++], accessStr->sr);

            PhMoveReference(&PreNode->DetailsText, PhFormat(format, count, 100));
            PhClearReference(&PreNode->TooltipText);
            PhDereferenceObject(accessStr);
        }
        break;

    //
    // Registry post-ops: append disposition for CreateKey/OpenKey.
    //

    case KphMsgRegPostCreateKey:
    case KphMsgRegPostOpenKey:
        {
            ULONG disposition;
            PCPH_STRINGREF dispText;
            PH_FORMAT format[3];
            ULONG count = 0;

            if (msgId == KphMsgRegPostCreateKey)
                disposition = PostMsg->Kernel.Reg.Post.CreateKey.Disposition;
            else
                disposition = PostMsg->Kernel.Reg.Post.OpenKey.Disposition;

            if (disposition == REG_CREATED_NEW_KEY)
                dispText = &dispCreated;
            else if (disposition == REG_OPENED_EXISTING_KEY)
                dispText = &dispOpened;
            else
                break;

            if (!PhIsNullOrEmptyString(PreNode->DetailsText))
            {
                PhInitFormatSR(&format[count++], PreNode->DetailsText->sr);
                PhInitFormatSR(&format[count++], separator);
            }

            PhInitFormatSR(&format[count++], *dispText);

            PhMoveReference(&PreNode->DetailsText, PhFormat(format, count, 80));
            PhClearReference(&PreNode->TooltipText);
        }
        break;

    //
    // File create post-ops: append open result.
    //

    case KphMsgFilePostCreate:
    case KphMsgFilePostCreateNamedPipe:
    case KphMsgFilePostCreateMailslot:
        {
            ULONG_PTR information = PostMsg->Kernel.File.Post.IoStatus.Information;
            PCPH_STRINGREF resultName = PhpInformerGetOpenResultName(information);

            if (resultName)
            {
                PH_FORMAT format[4];
                ULONG count = 0;

                if (!PhIsNullOrEmptyString(PreNode->DetailsText))
                {
                    PhInitFormatSR(&format[count++], PreNode->DetailsText->sr);
                    PhInitFormatSR(&format[count++], separator);
                }

                PhInitFormatSR(&format[count++], openResultPrefix);
                PhInitFormatSR(&format[count++], *resultName);

                PhMoveReference(&PreNode->DetailsText, PhFormat(format, count, 100));
                PhClearReference(&PreNode->TooltipText);
            }
        }
        break;
    }
}

VOID PhpInformerAddMessage(
    _In_ PPH_INFORMERW_CONTEXT Context,
    _In_ PKPH_MESSAGE Message
    )
{
    PPH_INFORMERW_NODE node;
    ULONG category;
    ULONG64 preSeq;
    ULONG64 postPreSeq;
    NTSTATUS postStatus;
    LARGE_INTEGER preTimeStamp;

    //
    // For process-scoped views, only keep events where this process is
    // associated with at least one of the process start keys.
    //
    if (Context->ProcessStartKey != 0)
    {
        ULONG64 keys[5];

        PhInformerGetProcessStartKeys(Message, keys);

        if (keys[0] != Context->ProcessStartKey &&
            keys[1] != Context->ProcessStartKey &&
            keys[2] != Context->ProcessStartKey &&
            keys[3] != Context->ProcessStartKey &&
            keys[4] != Context->ProcessStartKey)
        {
            PhDereferenceObject(Message);
            return;
        }
    }

    //
    // Pre/post correlation: if this is a post-op, try to back-fill the result
    // onto the matching pre-op node and suppress the post as a standalone row.
    //
    if (PhpInformerIsPostOp(Message, &postPreSeq, &postStatus, &preTimeStamp))
    {
        PVOID *entry;

        entry = PhFindItemSimpleHashtable(Context->PreOpTable, (PVOID)postPreSeq);
        if (entry)
        {
            PPH_INFORMERW_NODE preNode = (PPH_INFORMERW_NODE)*entry;
            LONGLONG durationTicks;

            PhMoveReference(&preNode->ResultText, PhGetStatusMessage(postStatus, 0));

            durationTicks = Message->Header.TimeStamp.QuadPart - preTimeStamp.QuadPart;
            if (durationTicks > 0)
            {
                PH_FORMAT durationFmt[1];

                PhClearReference(&preNode->DurationText);
                PhInitFormatFD(&durationFmt[0], (DOUBLE)durationTicks / 10000000.0, 7);
                preNode->DurationText = PhFormat(durationFmt, 1, 20);
            }

            //
            // Ensure the pre-op details text is materialized before
            // appending post-op information. DetailsText is lazily
            // initialized, so if the post-op arrives before the UI
            // renders the pre-op row, it would still be NULL and the
            // pre-op details would be lost.
            //
            if (!preNode->DetailsText)
                preNode->DetailsText = PhpInformerFormatDetailsText(Context, preNode->Message);

            PhpInformerUpdateDetailsFromPostOp(preNode, Message);

            TreeNew_InvalidateNode(Context->TreeNewHandle, &preNode->Node);

            PhRemoveItemSimpleHashtable(Context->PreOpTable, (PVOID)postPreSeq);
        }

        //
        // Post-op is fully consumed — drop its reference and do not add a row.
        //
        PhDereferenceObject(Message);
        return;
    }

    //
    // Drop events whose category is not in the active filter mask. This avoids
    // consuming node limit slots for events the user doesn't want to see.
    // Post-ops are already handled above.
    //
    category = PhpInformerGetCategory(Message);
    if (!(Context->CategoryFilter & category))
    {
        PhDereferenceObject(Message);
        return;
    }

    //
    // Tree view struggles with a large number of nodes, cap it for now. We
    // should improve tree view or use a more appropriate view for this. This is
    // fine for now since the process monitor is labeled an experimental option.
    //
    // N.B. NodeLimit is immutable and this is the only location that appends to
    // NodeList, so the count can exceed the limit by at most one. A single
    // removal is sufficient to maintain the invariant.
    //
    if (Context->NodeList->Count >= Context->NodeLimit)
    {
        PPH_INFORMERW_NODE oldest = Context->NodeList->Items[0];
        PhRemoveItemList(Context->NodeList, 0);
        if (oldest->PreOpSequence)
            PhRemoveItemSimpleHashtable(Context->PreOpTable, (PVOID)oldest->PreOpSequence);
        PhpInformerNodeFree(oldest);
    }

    node = PhAllocateZero(sizeof(PH_INFORMERW_NODE));
    PhInitializeTreeNewNode(&node->Node);
    node->Message = Message; // takes ownership of the reference
    node->Category = category;

    node->CategoryText = *PhpInformerGetCategoryName(node->Category);
    PhpInformerPopulateEventName(Message, node);
    PhpInformerPopulateNodePath(Message, node);

    HANDLE pid = PhpInformerGetPid(Message);
    HANDLE tid = PhpInformerGetTid(Message);
    if (pid)
        PhPrintUInt32(node->PidString, HandleToUlong(pid));
    if (tid)
        PhPrintUInt32(node->TidString, HandleToUlong(tid));

    preSeq = PhpInformerGetPreOpSequence(Message);
    if (preSeq)
    {
        node->PreOpSequence = preSeq;
        PhAddItemSimpleHashtable(Context->PreOpTable, (PVOID)preSeq, node);
    }

    PhAddItemList(Context->NodeList, node);

    if (Context->TreeFilterSupport.FilterList)
        node->Node.Visible = PhApplyTreeNewFiltersToNode(&Context->TreeFilterSupport, &node->Node);

    //
    // Note: caller is responsible for calling TreeNew_NodesStructured after
    // batch-adding (see WM_PH_INFORMER_FLUSH and PhpInformerLoadFromDatabase).
    //
}

_Function_class_(PH_SEARCHCONTROL_CALLBACK)
VOID NTAPI PhpInformerSearchCallback(
    _In_ ULONG_PTR MatchHandle,
    _In_opt_ PVOID Context
    )
{
    PPH_INFORMERW_CONTEXT context = Context;

    if (!context)
        return;

    context->SearchMatchHandle = MatchHandle;
    PhApplyTreeNewFilters(&context->TreeFilterSupport);
}

VOID PhpInformerLoadFromDatabase(
    _In_ PPH_INFORMERW_CONTEXT Context
    )
{
    PPH_LIST messages;

    messages = PhInformerDatabaseQuery(
        Context->ProcessStartKey,
        NULL
        );

    for (ULONG i = 0; i < messages->Count; i++)
    {
        PKPH_MESSAGE msg = messages->Items[i];
        PhpInformerAddMessage(Context, msg);
    }

    if (messages->Count > 0)
        TreeNew_NodesStructured(Context->TreeNewHandle);

    PhDereferenceObject(messages);
}

_Function_class_(PH_CALLBACK_FUNCTION)
VOID NTAPI PhpInformerLiveCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    PPH_INFORMERW_CONTEXT context = (PPH_INFORMERW_CONTEXT)Context;
    PPH_INFORMER_CONTEXT informer = (PPH_INFORMER_CONTEXT)Parameter;

    if (!context->Capturing || !context->WindowHandle)
        return;

    //
    // Add a reference so the message survives until the UI thread consumes it
    //
    PhReferenceObject(informer->Message);

    PhAcquireQueuedLockExclusive(&context->PendingLock);
    PhAddItemList(context->PendingList, informer->Message);
    if (!context->PendingPosted)
    {
        context->PendingPosted = TRUE;
        PhReleaseQueuedLockExclusive(&context->PendingLock);
        PostMessage(context->WindowHandle, WM_PH_INFORMER_FLUSH, 0, 0);
    }
    else
    {
        PhReleaseQueuedLockExclusive(&context->PendingLock);
    }
}

VOID PhpInformerInitializeDialog(
    _In_ PPH_INFORMERW_CONTEXT Context,
    _In_ HWND WindowHandle,
    _In_ PCWSTR ColumnSettingName
    )
{
    Context->WindowHandle = WindowHandle;
    Context->TreeNewHandle = GetDlgItem(WindowHandle, IDC_LIST);
    Context->SearchboxHandle = GetDlgItem(WindowHandle, IDC_SEARCH);
    Context->CaptureHandle = GetDlgItem(WindowHandle, IDC_CAPTURE);
    Context->FilterHandle = GetDlgItem(WindowHandle, IDC_FILTER);

    Button_SetText(Context->CaptureHandle, L"Pause");

    PhSetControlTheme(Context->TreeNewHandle, L"explorer");
    TreeNew_SetCallback(Context->TreeNewHandle, PhpInformerTreeNewCallback, Context);
    TreeNew_SetEmptyText(Context->TreeNewHandle, &PhpInformerEmptyText, 0);
    TreeNew_SetExtendedFlags(Context->TreeNewHandle, TN_FLAG_ITEM_DRAG_SELECT, TN_FLAG_ITEM_DRAG_SELECT);

    PhpInformerInitializeColumns(Context, ColumnSettingName);

    PhCreateSearchControl(
        WindowHandle,
        Context->SearchboxHandle,
        L"Search Monitor (Ctrl+K)",
        PhpInformerSearchCallback,
        Context
        );

    PhRegisterCallback(
        &PhInformerCallback,
        PhpInformerLiveCallback,
        Context,
        &Context->InformerCallbackRegistration
        );

    PhpInformerLoadFromDatabase(Context);

    PhInitializeWindowTheme(WindowHandle, PhEnableThemeSupport);
}

VOID PhpInformerDestroyDialog(
    _In_ PPH_INFORMERW_CONTEXT Context,
    _In_ PCWSTR ColumnSettingName
    )
{
    PhAcquireQueuedLockExclusive(&Context->PendingLock);

    for (ULONG i = 0; i < Context->PendingList->Count; i++)
        PhDereferenceObject(Context->PendingList->Items[i]);

    PhClearList(Context->PendingList);
    PhReleaseQueuedLockExclusive(&Context->PendingLock);

    PhpInformerSaveColumns(Context, ColumnSettingName);
    PhpInformerDestroyContext(Context);
}

VOID PhpInformerShowFilterMenu(
    _In_ PPH_INFORMERW_CONTEXT Context,
    _In_ HWND ButtonHandle
    )
{
    RECT rect;
    PPH_EMENU menu;
    PPH_EMENU_ITEM item;
    ULONG filter = Context->CategoryFilter;

    GetWindowRect(ButtonHandle, &rect);

    menu = PhCreateEMenu();

    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, PH_INFORMER_CATEGORY_PROCESS, L"Process", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, PH_INFORMER_CATEGORY_THREAD, L"Thread", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, PH_INFORMER_CATEGORY_FILE, L"File", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, PH_INFORMER_CATEGORY_REGISTRY, L"Registry", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, PH_INFORMER_CATEGORY_HANDLE, L"Handle", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, PH_INFORMER_CATEGORY_IMAGE, L"Image", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, PH_INFORMER_CATEGORY_OTHER, L"Other", NULL, NULL), ULONG_MAX);

    //
    // Set checkmarks
    //
    for (ULONG i = 0; i < menu->Items->Count; i++)
    {
        item = menu->Items->Items[i];
        if (filter & item->Id)
            item->Flags |= PH_EMENU_CHECKED;
    }

    item = PhShowEMenu(
        menu,
        Context->WindowHandle,
        PH_EMENU_SHOW_LEFTRIGHT,
        PH_ALIGN_LEFT | PH_ALIGN_TOP,
        rect.left,
        rect.bottom
        );

    if (item)
    {
        Context->CategoryFilter ^= item->Id;
        PhSetIntegerSetting(SETTING_PROCESS_MONITOR_CATEGORY_FILTER, Context->CategoryFilter);
        PhApplyTreeNewFilters(&Context->TreeFilterSupport);
    }

    PhDestroyEMenu(menu);
}

INT_PTR PhpInformerHandleMessage(
    _In_ PPH_INFORMERW_CONTEXT Context,
    _In_ HWND WindowHandle,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    switch (uMsg)
    {
    case WM_COMMAND:
        {
            switch (GET_WM_COMMAND_ID(wParam, lParam))
            {
            case IDC_CAPTURE:
                {
                    Context->Capturing = !Context->Capturing;
                    Button_SetText(Context->CaptureHandle, Context->Capturing ? L"Pause" : L"Resume");
                }
                break;

            case IDC_FILTER:
                PhpInformerShowFilterMenu(Context, Context->FilterHandle);
                break;

            case IDC_CLEAR:
                {
                    PhpInformerClearNodes(Context);
                    TreeNew_NodesStructured(Context->TreeNewHandle);
                }
                break;
            }
        }
        return TRUE;

    case WM_PH_INFORMER_FLUSH:
        {
            PPH_LIST batch;

            PhAcquireQueuedLockExclusive(&Context->PendingLock);
            batch = Context->PendingList;
            Context->PendingList = PhCreateList(64);
            Context->PendingPosted = FALSE;
            PhReleaseQueuedLockExclusive(&Context->PendingLock);

            for (ULONG i = 0; i < batch->Count; i++)
            {
                PhpInformerAddMessage(Context, (PKPH_MESSAGE)batch->Items[i]);
            }

            if (batch->Count > 0)
                TreeNew_NodesStructured(Context->TreeNewHandle);

            PhDereferenceObject(batch);
        }
        return TRUE;

    case WM_CTLCOLORBTN:
    case WM_CTLCOLORDLG:
    case WM_CTLCOLORSTATIC:
    case WM_CTLCOLORLISTBOX:
        return HANDLE_WM_CTLCOLORBTN(WindowHandle, wParam, lParam, PhWindowThemeControlColor);
    }

    return FALSE;
}

INT_PTR CALLBACK PhpInformerDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PPH_INFORMERW_CONTEXT context;

    if (uMsg == WM_INITDIALOG)
    {
        context = PhpCreateInformerContext(0);
        PhSetWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT, context);
    }
    else
    {
        context = PhGetWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT);
    }

    if (!context)
        return FALSE;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            PhSetApplicationWindowIcon(hwndDlg);
            PhpInformerInitializeDialog(context, hwndDlg, SETTING_PROCESS_MONITOR_TREE_LIST_COLUMNS);
            PhInitializeLayoutManager(&PhpInformerLayoutManager, hwndDlg);
            PhAddLayoutItem(&PhpInformerLayoutManager, context->TreeNewHandle, NULL, PH_ANCHOR_ALL);
            PhAddLayoutItem(&PhpInformerLayoutManager, context->SearchboxHandle, NULL, PH_ANCHOR_TOP | PH_ANCHOR_RIGHT);
            PhAddLayoutItem(&PhpInformerLayoutManager, context->CaptureHandle, NULL, PH_ANCHOR_TOP | PH_ANCHOR_LEFT);
            PhAddLayoutItem(&PhpInformerLayoutManager, context->FilterHandle, NULL, PH_ANCHOR_TOP | PH_ANCHOR_LEFT);
            PhAddLayoutItem(&PhpInformerLayoutManager, GetDlgItem(hwndDlg, IDC_CLEAR), NULL, PH_ANCHOR_TOP | PH_ANCHOR_LEFT);

            PhpInformerMinimumSize.left = 0;
            PhpInformerMinimumSize.top = 0;
            PhpInformerMinimumSize.right = 400;
            PhpInformerMinimumSize.bottom = 200;
            MapDialogRect(hwndDlg, &PhpInformerMinimumSize);

            if (PhValidWindowPlacementFromSetting(SETTING_PROCESS_MONITOR_WINDOW_POSITION))
                PhLoadWindowPlacementFromSetting(SETTING_PROCESS_MONITOR_WINDOW_POSITION, SETTING_PROCESS_MONITOR_WINDOW_SIZE, hwndDlg);
            else
                PhCenterWindow(hwndDlg, PhMainWndHandle);

            // Set initial focus to the TreeNew, not the search box
            SetFocus(context->TreeNewHandle);
        }
        return FALSE;  // We handled focus ourselves

    case WM_DESTROY:
        {
            PhpInformerDestroyDialog(context, SETTING_PROCESS_MONITOR_TREE_LIST_COLUMNS);
            PhSaveWindowPlacementToSetting(SETTING_PROCESS_MONITOR_WINDOW_POSITION, SETTING_PROCESS_MONITOR_WINDOW_SIZE, hwndDlg);
            PhDeleteLayoutManager(&PhpInformerLayoutManager);
            PhRemoveWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT);
            PhUnregisterDialog(PhpInformerWindowHandle);
            PhpInformerWindowHandle = NULL;
        }
        break;

    case WM_SIZE:
        PhLayoutManagerLayout(&PhpInformerLayoutManager);
        break;

    case WM_GETMINMAXINFO:
        {
            PMINMAXINFO minMaxInfo = (PMINMAXINFO)lParam;
            if (PhpInformerMinimumSize.right != -1)
            {
                minMaxInfo->ptMinTrackSize.x = PhpInformerMinimumSize.right;
                minMaxInfo->ptMinTrackSize.y = PhpInformerMinimumSize.bottom;
            }
        }
        break;

    case WM_COMMAND:
        {
            switch (GET_WM_COMMAND_ID(wParam, lParam))
            {
            case IDCANCEL:
                DestroyWindow(hwndDlg);
                return TRUE;
            }
        }
        return PhpInformerHandleMessage(context, hwndDlg, uMsg, wParam, lParam);

    default:
        return PhpInformerHandleMessage(context, hwndDlg, uMsg, wParam, lParam);
    }

    return FALSE;
}

VOID PhShowInformerWindow(
    _In_opt_ HWND ParentWindowHandle
    )
{
    if (!PhpInformerWindowHandle)
    {
        PhpInformerWindowHandle = PhCreateDialog(
            PhInstanceHandle,
            MAKEINTRESOURCE(IDD_INFORMER),
            NULL,
            PhpInformerDlgProc,
            NULL
            );
        PhRegisterDialog(PhpInformerWindowHandle);
        ShowWindow(PhpInformerWindowHandle, SW_SHOW);
    }

    if (IsMinimized(PhpInformerWindowHandle))
        ShowWindow(PhpInformerWindowHandle, SW_RESTORE);
    else
        SetForegroundWindow(PhpInformerWindowHandle);
}

INT_PTR CALLBACK PhpProcessInformerDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PPH_INFORMERW_CONTEXT context;
    LPPROPSHEETPAGE propSheetPage;
    PPH_PROCESS_PROPPAGECONTEXT propPageContext;
    PPH_PROCESS_ITEM processItem;

    if (PhPropPageDlgProcHeader(hwndDlg, uMsg, lParam, &propSheetPage, &propPageContext, &processItem))
    {
        context = (PPH_INFORMERW_CONTEXT)propPageContext->Context;
    }
    else
    {
        return FALSE;
    }

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            NTSTATUS status = STATUS_UNSUCCESSFUL;
            ULONG64 processStartKey = processItem->ProcessStartKey;
            if (processStartKey == 0 && processItem->QueryHandle)
            {
                if (!NT_SUCCESS(status = PhGetProcessStartKey(processItem->QueryHandle, &processStartKey)))
                    processStartKey = 0;
            }

            if (processStartKey == 0)
            {
                //
                // On failure, set a value that will not match any real process
                // start key. And one which does not collide with the default
                // "global" value of 0.
                //
                processStartKey = ULONG64_MAX;
                PhShowStatus(hwndDlg, L"Failed to get process start key.", status, 0);
            }

            context = PhpCreateInformerContext(processStartKey);
            context->ProcessId = processItem->ProcessId;
            context->ProcessName = PhReferenceObject(processItem->ProcessName);
            propPageContext->Context = context;
            PhpInformerInitializeDialog(context, hwndDlg, SETTING_PROCESS_MONITOR_TAB_TREE_LIST_COLUMNS);
        }
        return TRUE;

    case WM_DESTROY:
        {
            PhpInformerDestroyDialog(context, SETTING_PROCESS_MONITOR_TAB_TREE_LIST_COLUMNS);
            propPageContext->Context = NULL;
        }
        break;

    case WM_SHOWWINDOW:
        {
            PPH_LAYOUT_ITEM dialogItem;

            if (dialogItem = PhBeginPropPageLayout(hwndDlg, propPageContext))
            {
                PhAddPropPageLayoutItem(hwndDlg, context->CaptureHandle, dialogItem, PH_ANCHOR_TOP | PH_ANCHOR_LEFT);
                PhAddPropPageLayoutItem(hwndDlg, context->FilterHandle, dialogItem, PH_ANCHOR_TOP | PH_ANCHOR_LEFT);
                PhAddPropPageLayoutItem(hwndDlg, GetDlgItem(hwndDlg, IDC_CLEAR), dialogItem, PH_ANCHOR_TOP | PH_ANCHOR_LEFT);
                PhAddPropPageLayoutItem(hwndDlg, context->SearchboxHandle, dialogItem, PH_ANCHOR_TOP | PH_ANCHOR_RIGHT);
                PhAddPropPageLayoutItem(hwndDlg, context->TreeNewHandle, dialogItem, PH_ANCHOR_ALL);
                PhEndPropPageLayout(hwndDlg, propPageContext);
            }
        }
        break;

    case WM_NOTIFY:
        {
            LPNMHDR header = (LPNMHDR)lParam;

            switch (header->code)
            {
            case PSN_QUERYINITIALFOCUS:
                SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, (LPARAM)context->CaptureHandle);
                return TRUE;
            }
        }
        break;

    default:
        return PhpInformerHandleMessage(context, hwndDlg, uMsg, wParam, lParam);
    }

    return FALSE;
}
