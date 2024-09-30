/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2010-2011
 *
 */

#include "exttools.h"
#include <symprv.h>
#include <secedit.h>
#include <hndlinfo.h>
#include <kphuser.h>

typedef struct _COMMON_PAGE_CONTEXT
{
    PPH_HANDLE_ITEM HandleItem;
    HANDLE ProcessId;
    HWND WindowHandle;
    HWND ListViewHandle;
} COMMON_PAGE_CONTEXT, *PCOMMON_PAGE_CONTEXT;

typedef struct _ET_HANDLE_ENTRY
{
    HANDLE ProcessId;
    PPH_HANDLE_ITEM HandleItem;
    COLORREF Color;
} HANDLE_ENTRY, * PHANDLE_ENTRY;

HPROPSHEETPAGE EtpCommonCreatePage(
    _In_ PPH_PLUGIN_HANDLE_PROPERTIES_CONTEXT Context,
    _In_ PWSTR Template,
    _In_ DLGPROC DlgProc
    );

INT CALLBACK EtpCommonPropPageProc(
    _In_ HWND hwnd,
    _In_ UINT uMsg,
    _In_ LPPROPSHEETPAGE ppsp
    );

INT_PTR CALLBACK EtpTpWorkerFactoryPageDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

INT_PTR CALLBACK EtpObjHandlesPageDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

PPH_STRING EtGetAccessString(
    _In_ PPH_STRING TypeName,
    _In_ ACCESS_MASK access
    );

INT_PTR CALLBACK EtpWinStaPageDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

extern HWND EtObjectManagerDialogHandle;
extern LARGE_INTEGER EtObjectManagerTimeCached;
extern PPH_LIST EtObjectManagerOwnHandles;
extern HICON EtObjectManagerPropWndIcon;
extern PPH_HASHTABLE EtObjectManagerPropWnds;

VOID EtHandlePropertiesInitializing(
    _In_ PVOID Parameter
    )
{
    PPH_PLUGIN_OBJECT_PROPERTIES objectProperties = Parameter;
    PPH_PLUGIN_HANDLE_PROPERTIES_CONTEXT context = objectProperties->Parameter;

    if (PhIsNullOrEmptyString(context->HandleItem->TypeName))
        return;

    if (objectProperties->NumberOfPages < objectProperties->MaximumNumberOfPages)
    {
        HPROPSHEETPAGE page = NULL;

        if (PhEqualString2(context->HandleItem->TypeName, L"WindowStation", TRUE))
        {
            page = EtpCommonCreatePage(
                context,
                MAKEINTRESOURCE(IDD_OBJWINSTA),
                EtpWinStaPageDlgProc
            );

            // Insert our page into the second slot.
            if (page)
            {
                if (objectProperties->NumberOfPages > 1)
                {
                    memmove(&objectProperties->Pages[2], &objectProperties->Pages[1],
                        (objectProperties->NumberOfPages - 1) * sizeof(HPROPSHEETPAGE));
                }

                objectProperties->Pages[1] = page;
                objectProperties->NumberOfPages++;
            }
        }

        // Object Manager
        if (EtObjectManagerDialogHandle && context->OwnerPlugin == PluginInstance)
        {
            page = EtpCommonCreatePage(
                context,
                MAKEINTRESOURCE(IDD_OBJHANDLES),
                EtpObjHandlesPageDlgProc
            );

            // Insert our page into the second slot.
            if (page)
            {
                if (objectProperties->NumberOfPages > 1)
                {
                    memmove(&objectProperties->Pages[2], &objectProperties->Pages[1],
                        (objectProperties->NumberOfPages - 1) * sizeof(HPROPSHEETPAGE));
                }

                objectProperties->Pages[1] = page;
                objectProperties->NumberOfPages++;
            }
        }

        // TpWorkerFactory unnamed, so it won't interact with Object Manager
        if (PhEqualString2(context->HandleItem->TypeName, L"TpWorkerFactory", TRUE))
        {
            page = EtpCommonCreatePage(
                context,
                MAKEINTRESOURCE(IDD_OBJTPWORKERFACTORY),
                EtpTpWorkerFactoryPageDlgProc
            );

            // Insert our page into the second slot.
            if (page)
            {
                if (objectProperties->NumberOfPages > 1)
                {
                    memmove(&objectProperties->Pages[2], &objectProperties->Pages[1],
                        (objectProperties->NumberOfPages - 1) * sizeof(HPROPSHEETPAGE));
                }

                objectProperties->Pages[1] = page;
                objectProperties->NumberOfPages++;
            }
        }
    }
}

typedef enum _ET_OBJECT_GENERAL_CATEGORY
{
    OBJECT_GENERAL_CATEGORY_DEVICE = PH_PLUGIN_HANDLE_GENERAL_CATEGORY_FILE,
    OBJECT_GENERAL_CATEGORY_DRIVER,
    OBJECT_GENERAL_CATEGORY_TYPE,
    OBJECT_GENERAL_CATEGORY_TYPE_ACCESS,
    OBJECT_GENERAL_CATEGORY_WINDOWSTATION,
    OBJECT_GENERAL_CATEGORY_DESKTOP
} ET_OBJECT_GENERAL_CATEGORY;

typedef enum _ET_OBJECT_GENERAL_INDEX {
    OBJECT_GENERAL_INDEX_ATTRIBUTES = PH_PLUGIN_HANDLE_GENERAL_INDEX_ACCESSMASK,
    OBJECT_GENERAL_INDEX_CREATIONTIME,

    OBJECT_GENERAL_INDEX_DEVICEDRVLOW,
    OBJECT_GENERAL_INDEX_DEVICEDRVLOWPATH,
    OBJECT_GENERAL_INDEX_DEVICEDRVHIGH,
    OBJECT_GENERAL_INDEX_DEVICEDRVHIGHPATH,
    OBJECT_GENERAL_INDEX_DEVICEPNPNAME,

    OBJECT_GENERAL_INDEX_DRIVERIMAGE,

    OBJECT_GENERAL_INDEX_TYPEINDEX,
    OBJECT_GENERAL_INDEX_TYPEOBJECTS,
    OBJECT_GENERAL_INDEX_TYPEHANDLES,
    OBJECT_GENERAL_INDEX_TYPEPEAKOBJECTS,
    OBJECT_GENERAL_INDEX_TYPEPEAKHANDLES,
    OBJECT_GENERAL_INDEX_TYPEPOOLTYPE,
    OBJECT_GENERAL_INDEX_TYPEPAGECHARGE,
    OBJECT_GENERAL_INDEX_TYPENPAGECHARGE,

    OBJECT_GENERAL_INDEX_TYPEVALIDMASK,
    OBJECT_GENERAL_INDEX_TYPEGENERICREAD,
    OBJECT_GENERAL_INDEX_TYPEGENERICWRITE,
    OBJECT_GENERAL_INDEX_TYPEGENERICEXECUTE,
    OBJECT_GENERAL_INDEX_TYPEGENERICALL,
    OBJECT_GENERAL_INDEX_TYPEINVALIDATTRIBUTES,

    OBJECT_GENERAL_INDEX_WINSTATYPE,
    OBJECT_GENERAL_INDEX_WINSTAVISIBLE,

    OBJECT_GENERAL_INDEX_DESKTOPIO,
    OBJECT_GENERAL_INDEX_DESKTOPSID,
    OBJECT_GENERAL_INDEX_DESKTOPHEAP,

    OBJECT_GENERAL_INDEX_MAXIMUM
} ET_OBJECT_GENERAL_INDEX;

typedef enum _ET_OBJECT_POOLTYPE {
    PagedPool = 1,
    NonPagedPool = 0,
    NonPagedPoolNx = 0x200,
    NonPagedPoolSessionNx = NonPagedPoolNx + 32,
    PagedPoolSessionNx = NonPagedPoolNx + 33
} ET_OBJECT_POOLTYPE;

VOID EtHandlePropertiesWindowInitialized(
    _In_ PVOID Parameter
    )
{
    static INT EtListViewRowCache[OBJECT_GENERAL_INDEX_MAXIMUM];

    PPH_PLUGIN_HANDLE_PROPERTIES_WINDOW_CONTEXT context = Parameter;

    if (EtObjectManagerDialogHandle && context->OwnerPlugin == PluginInstance)
    {
        if (context->HandleItem->Handle)
        {
            PhReferenceObject(EtObjectManagerPropWnds);
            PhAddItemSimpleHashtable(EtObjectManagerPropWnds, context->ParentWindow, context->HandleItem);
        }

        // HACK
        if (PhGetIntegerPairSetting(SETTING_NAME_OBJMGR_PROPERTIES_WINDOW_POSITION).X != 0)
            PhLoadWindowPlacementFromSetting(SETTING_NAME_OBJMGR_PROPERTIES_WINDOW_POSITION, NULL, context->ParentWindow);
        else
            PhCenterWindow(context->ParentWindow, GetParent(context->ParentWindow)); // HACK

        PhSetWindowIcon(context->ParentWindow, EtObjectManagerPropWndIcon, NULL, 0);

        // Show real handles count
        WCHAR string[PH_INT64_STR_LEN_1];
        ULONG64 real_count;
        PPH_STRING count = PH_AUTO(PhGetListViewItemText(context->ListViewHandle, PH_PLUGIN_HANDLE_GENERAL_INDEX_HANDLES, 1));
       
        if (!PhIsNullOrEmptyString(count) && PhStringToUInt64(&count->sr, 0, &real_count) && real_count > 0) {
            ULONG own_count = 0;
            PPH_KEY_VALUE_PAIR entry;
            ULONG i = 0;

            while (PhEnumHashtable(EtObjectManagerPropWnds, &entry, &i))
                if (PhEqualString(context->HandleItem->BestObjectName, ((PPH_HANDLE_ITEM)entry->Value)->BestObjectName, TRUE))
                    own_count++;

            PhPrintUInt32(string, (ULONG)real_count - own_count);
            PhSetListViewSubItem(context->ListViewHandle, PH_PLUGIN_HANDLE_GENERAL_INDEX_HANDLES, 1, string);
        }

        PhRemoveListViewItem(context->ListViewHandle, PH_PLUGIN_HANDLE_GENERAL_INDEX_ACCESSMASK);

        EtListViewRowCache[OBJECT_GENERAL_INDEX_ATTRIBUTES] = PhAddListViewGroupItem(
            context->ListViewHandle,
            PH_PLUGIN_HANDLE_GENERAL_CATEGORY_BASICINFO,
            OBJECT_GENERAL_INDEX_ATTRIBUTES,
            L"Object attributes",
            NULL
        );

        // Show object attributes
        PWSTR Attributes = NULL;
        switch (context->HandleItem->Attributes & (OBJ_PERMANENT | OBJ_EXCLUSIVE))
        {
        case OBJ_PERMANENT:
            Attributes = L"Permanent";
            break;
        case OBJ_EXCLUSIVE:
            Attributes = L"Exclusive";
            break;
        case OBJ_PERMANENT | OBJ_EXCLUSIVE:
            Attributes = L"Permanent, Exclusive";
            break;
        }
        PhSetListViewSubItem(context->ListViewHandle, EtListViewRowCache[OBJECT_GENERAL_INDEX_ATTRIBUTES], 1, Attributes);

        // Show creation time
        if (EtObjectManagerTimeCached.QuadPart != 0)
        {
            PPH_STRING startTimeString;
            SYSTEMTIME startTimeFields;

            EtListViewRowCache[OBJECT_GENERAL_INDEX_CREATIONTIME] = PhAddListViewGroupItem(
                context->ListViewHandle,
                PH_PLUGIN_HANDLE_GENERAL_CATEGORY_BASICINFO,
                OBJECT_GENERAL_INDEX_CREATIONTIME,
                L"Creation time",
                NULL
            );

            PhLargeIntegerToLocalSystemTime(&startTimeFields, &EtObjectManagerTimeCached);
            startTimeString = PhaFormatDateTime(&startTimeFields);

            PhSetListViewSubItem(context->ListViewHandle, EtListViewRowCache[OBJECT_GENERAL_INDEX_CREATIONTIME], 1, startTimeString->Buffer);
        }

        // Show Device drivers information
        if (PhEqualString2(context->HandleItem->TypeName, L"Device", TRUE))
        {
            HANDLE objectHandle;
            HANDLE DriverHandle;
            PPH_STRING driverName;

            PhAddListViewGroup(context->ListViewHandle, OBJECT_GENERAL_CATEGORY_DEVICE, L"Device driver information");

            EtListViewRowCache[OBJECT_GENERAL_INDEX_DEVICEDRVLOW] = PhAddListViewGroupItem(context->ListViewHandle,
                OBJECT_GENERAL_CATEGORY_DEVICE, OBJECT_GENERAL_INDEX_DEVICEDRVLOW, L"Lower-edge driver", NULL);
            EtListViewRowCache[OBJECT_GENERAL_INDEX_DEVICEDRVLOWPATH] = PhAddListViewGroupItem(context->ListViewHandle,
                OBJECT_GENERAL_CATEGORY_DEVICE, OBJECT_GENERAL_INDEX_DEVICEDRVLOWPATH, L"Lower-edge driver image", NULL);

            EtListViewRowCache[OBJECT_GENERAL_INDEX_DEVICEDRVHIGH] = PhAddListViewGroupItem(context->ListViewHandle,
                OBJECT_GENERAL_CATEGORY_DEVICE, OBJECT_GENERAL_INDEX_DEVICEDRVHIGH, L"Upper-edge driver", NULL);
            EtListViewRowCache[OBJECT_GENERAL_INDEX_DEVICEDRVHIGHPATH] = PhAddListViewGroupItem(context->ListViewHandle,
                OBJECT_GENERAL_CATEGORY_DEVICE, OBJECT_GENERAL_INDEX_DEVICEDRVHIGHPATH, L"Upper-edge driver Image", NULL);

            EtListViewRowCache[OBJECT_GENERAL_INDEX_DEVICEPNPNAME] = PhAddListViewGroupItem(context->ListViewHandle,
                OBJECT_GENERAL_CATEGORY_DEVICE, OBJECT_GENERAL_INDEX_DEVICEPNPNAME, L"PnP Device Name", NULL);

            if (NT_SUCCESS(PhOpenDevice(&objectHandle, &DriverHandle, READ_CONTROL, &context->HandleItem->BestObjectName->sr, TRUE)))
            {
                if (NT_SUCCESS(PhGetDriverName(DriverHandle, &driverName)))
                {
                    PhSetListViewSubItem(context->ListViewHandle, EtListViewRowCache[OBJECT_GENERAL_INDEX_DEVICEDRVLOW], 1, PhGetString(driverName));
                    PhDereferenceObject(driverName);
                }

                if (NT_SUCCESS(PhGetDriverImageFileName(DriverHandle, &driverName)))
                {
                    PhSetListViewSubItem(context->ListViewHandle, EtListViewRowCache[OBJECT_GENERAL_INDEX_DEVICEDRVLOWPATH], 1, PhGetString(driverName));
                    PhDereferenceObject(driverName);
                }

                NtClose(DriverHandle);
                NtClose(objectHandle);
            }

            if (NT_SUCCESS(PhOpenDevice(&objectHandle, &DriverHandle, READ_CONTROL, &context->HandleItem->BestObjectName->sr, FALSE)))
            {
                if (NT_SUCCESS(PhGetDriverName(DriverHandle, &driverName)))
                {
                    PhSetListViewSubItem(context->ListViewHandle, EtListViewRowCache[OBJECT_GENERAL_INDEX_DEVICEDRVHIGH], 1, PhGetString(driverName));
                    PhDereferenceObject(driverName);
                }

                if (NT_SUCCESS(PhGetDriverImageFileName(DriverHandle, &driverName)))
                {
                    PhSetListViewSubItem(context->ListViewHandle, EtListViewRowCache[OBJECT_GENERAL_INDEX_DEVICEDRVHIGHPATH], 1, PhGetString(driverName));
                    PhDereferenceObject(driverName);
                }

                NtClose(DriverHandle);
                NtClose(objectHandle);
            }

            PPH_STRING devicePdoName = PH_AUTO(PhGetPnPDeviceName(context->HandleItem->BestObjectName));
            if (devicePdoName)
            {
                ULONG_PTR column_pos = PhFindLastCharInString(devicePdoName, 0, L':');
                PPH_STRING devicePdoName2 = PH_AUTO(PhSubstring(devicePdoName, 0, column_pos - 5));
                PhSetListViewSubItem(context->ListViewHandle, EtListViewRowCache[OBJECT_GENERAL_INDEX_DEVICEPNPNAME], 1, PhGetString(devicePdoName2));
            }
        }
        // Show Driver image information
        else if (PhEqualString2(context->HandleItem->TypeName, L"Driver", TRUE))
        {
            PPH_STRING driverName;

            PhAddListViewGroup(context->ListViewHandle, OBJECT_GENERAL_CATEGORY_DRIVER, L"Driver information");

            EtListViewRowCache[OBJECT_GENERAL_INDEX_DRIVERIMAGE] = PhAddListViewGroupItem(
                context->ListViewHandle,
                OBJECT_GENERAL_CATEGORY_DRIVER,
                OBJECT_GENERAL_INDEX_DRIVERIMAGE,
                L"Driver Image",
                NULL
            );

            if (NT_SUCCESS(PhGetDriverImageFileName(context->HandleItem->Handle, &driverName)))
            {
                PhSetListViewSubItem(context->ListViewHandle, EtListViewRowCache[OBJECT_GENERAL_INDEX_DRIVERIMAGE], 1, PhGetString(driverName));
                PhDereferenceObject(driverName);
            }
        }
        else if (PhEqualString2(context->HandleItem->TypeName, L"Type", TRUE))
        {
            POBJECT_TYPES_INFORMATION objectTypes;
            POBJECT_TYPE_INFORMATION objectType;
            ULONG typeIndex;
            PPH_STRING accessString;
            PH_STRING_BUILDER stringBuilder;

            if (NT_SUCCESS(PhEnumObjectTypes(&objectTypes)))
            {
                typeIndex = PhGetObjectTypeNumber(&context->HandleItem->ObjectName->sr);
                objectType = PH_FIRST_OBJECT_TYPE(objectTypes);

                for (ULONG i = 0; i < objectTypes->NumberOfTypes; i++)
                {
                    if (objectType->TypeIndex == typeIndex)
                    {
                        ListView_RemoveGroup(context->ListViewHandle, PH_PLUGIN_HANDLE_GENERAL_CATEGORY_QUOTA);

                        PhAddListViewGroup(context->ListViewHandle, OBJECT_GENERAL_CATEGORY_TYPE, L"Type information");

                        EtListViewRowCache[OBJECT_GENERAL_INDEX_TYPEINDEX] = PhAddListViewGroupItem(context->ListViewHandle,
                            OBJECT_GENERAL_CATEGORY_TYPE, OBJECT_GENERAL_INDEX_TYPEINDEX, L"Index", NULL);
                        EtListViewRowCache[OBJECT_GENERAL_INDEX_TYPEOBJECTS] = PhAddListViewGroupItem(context->ListViewHandle,
                            OBJECT_GENERAL_CATEGORY_TYPE, OBJECT_GENERAL_INDEX_TYPEOBJECTS, L"Objects", NULL);
                        EtListViewRowCache[OBJECT_GENERAL_INDEX_TYPEHANDLES] = PhAddListViewGroupItem(context->ListViewHandle,
                            OBJECT_GENERAL_CATEGORY_TYPE, OBJECT_GENERAL_INDEX_TYPEHANDLES, L"Handles", NULL);
                        EtListViewRowCache[OBJECT_GENERAL_INDEX_TYPEPEAKOBJECTS] = PhAddListViewGroupItem(context->ListViewHandle,
                            OBJECT_GENERAL_CATEGORY_TYPE, OBJECT_GENERAL_INDEX_TYPEPEAKOBJECTS, L"Peak Objects", NULL);
                        EtListViewRowCache[OBJECT_GENERAL_INDEX_TYPEPEAKHANDLES] = PhAddListViewGroupItem(context->ListViewHandle,
                            OBJECT_GENERAL_CATEGORY_TYPE, OBJECT_GENERAL_INDEX_TYPEPEAKHANDLES, L"Peak Handles", NULL);
                        EtListViewRowCache[OBJECT_GENERAL_INDEX_TYPEPOOLTYPE] = PhAddListViewGroupItem(context->ListViewHandle,
                            OBJECT_GENERAL_CATEGORY_TYPE, OBJECT_GENERAL_INDEX_TYPEPOOLTYPE, L"Pool Type", NULL);
                        EtListViewRowCache[OBJECT_GENERAL_INDEX_TYPEPAGECHARGE] = PhAddListViewGroupItem(context->ListViewHandle,
                            OBJECT_GENERAL_CATEGORY_TYPE, OBJECT_GENERAL_INDEX_TYPEPAGECHARGE, L"Default Paged Charge", NULL);
                        EtListViewRowCache[OBJECT_GENERAL_INDEX_TYPENPAGECHARGE] = PhAddListViewGroupItem(context->ListViewHandle,
                            OBJECT_GENERAL_CATEGORY_TYPE, OBJECT_GENERAL_INDEX_TYPENPAGECHARGE, L"Default NP Charge", NULL);

                        PhPrintUInt32(string, objectType->TypeIndex);
                        PhSetListViewSubItem(context->ListViewHandle, EtListViewRowCache[OBJECT_GENERAL_INDEX_TYPEINDEX], 1, string);
                        PhPrintUInt32(string, objectType->TotalNumberOfObjects);
                        PhSetListViewSubItem(context->ListViewHandle, EtListViewRowCache[OBJECT_GENERAL_INDEX_TYPEOBJECTS], 1, string);
                        PhPrintUInt32(string, objectType->TotalNumberOfHandles);
                        PhSetListViewSubItem(context->ListViewHandle, EtListViewRowCache[OBJECT_GENERAL_INDEX_TYPEHANDLES], 1, string);
                        PhPrintUInt32(string, objectType->HighWaterNumberOfObjects);
                        PhSetListViewSubItem(context->ListViewHandle, EtListViewRowCache[OBJECT_GENERAL_INDEX_TYPEPEAKOBJECTS], 1, string);
                        PhPrintUInt32(string, objectType->HighWaterNumberOfHandles);
                        PhSetListViewSubItem(context->ListViewHandle, EtListViewRowCache[OBJECT_GENERAL_INDEX_TYPEPEAKHANDLES], 1, string);
                        PhPrintUInt32(string, objectType->DefaultPagedPoolCharge);

                        PWSTR PoolType = NULL;
                        switch (objectType->PoolType) {
                        case NonPagedPool:
                            PoolType = L"Non Paged";
                            break;
                        case PagedPool:
                            PoolType = L"Paged";
                            break;
                        case NonPagedPoolNx:
                            PoolType = L"Non Paged NX";
                            break;
                        case PagedPoolSessionNx:
                            PoolType = L"Paged Session NX";
                            break;
                        }
                        PhSetListViewSubItem(context->ListViewHandle, EtListViewRowCache[OBJECT_GENERAL_INDEX_TYPEPOOLTYPE], 1, PoolType);

                        PhSetListViewSubItem(context->ListViewHandle, EtListViewRowCache[OBJECT_GENERAL_INDEX_TYPEPAGECHARGE], 1, string);
                        PhPrintUInt32(string, objectType->DefaultNonPagedPoolCharge);
                        PhSetListViewSubItem(context->ListViewHandle, EtListViewRowCache[OBJECT_GENERAL_INDEX_TYPENPAGECHARGE], 1, string);

                        PhAddListViewGroup(context->ListViewHandle, OBJECT_GENERAL_CATEGORY_TYPE_ACCESS, L"Type access information");

                        EtListViewRowCache[OBJECT_GENERAL_INDEX_TYPEVALIDMASK] = PhAddListViewGroupItem(context->ListViewHandle,
                            OBJECT_GENERAL_CATEGORY_TYPE_ACCESS, OBJECT_GENERAL_INDEX_TYPEVALIDMASK, L"Valid Access Mask", NULL);
                        EtListViewRowCache[OBJECT_GENERAL_INDEX_TYPEGENERICREAD] = PhAddListViewGroupItem(context->ListViewHandle,
                            OBJECT_GENERAL_CATEGORY_TYPE_ACCESS, OBJECT_GENERAL_INDEX_TYPEGENERICREAD, L"Generic Read", NULL);
                        EtListViewRowCache[OBJECT_GENERAL_INDEX_TYPEGENERICWRITE] = PhAddListViewGroupItem(context->ListViewHandle,
                            OBJECT_GENERAL_CATEGORY_TYPE_ACCESS, OBJECT_GENERAL_INDEX_TYPEGENERICWRITE, L"Generic Write", NULL);
                        EtListViewRowCache[OBJECT_GENERAL_INDEX_TYPEGENERICEXECUTE] = PhAddListViewGroupItem(context->ListViewHandle,
                            OBJECT_GENERAL_CATEGORY_TYPE_ACCESS, OBJECT_GENERAL_INDEX_TYPEGENERICEXECUTE, L"Generic Execute", NULL);
                        EtListViewRowCache[OBJECT_GENERAL_INDEX_TYPEGENERICALL] = PhAddListViewGroupItem(context->ListViewHandle,
                            OBJECT_GENERAL_CATEGORY_TYPE_ACCESS, OBJECT_GENERAL_INDEX_TYPEGENERICALL, L"Generic All", NULL);
                        EtListViewRowCache[OBJECT_GENERAL_INDEX_TYPEINVALIDATTRIBUTES] = PhAddListViewGroupItem(context->ListViewHandle,
                            OBJECT_GENERAL_CATEGORY_TYPE_ACCESS, OBJECT_GENERAL_INDEX_TYPEINVALIDATTRIBUTES, L"Invalid Attributes", NULL);

                        accessString = PH_AUTO(EtGetAccessString(context->HandleItem->ObjectName, objectType->ValidAccessMask));
                        PhSetListViewSubItem(context->ListViewHandle, EtListViewRowCache[OBJECT_GENERAL_INDEX_TYPEVALIDMASK], 1, PhGetString(accessString));
                        accessString = PH_AUTO(EtGetAccessString(context->HandleItem->ObjectName, objectType->GenericMapping.GenericRead));
                        PhSetListViewSubItem(context->ListViewHandle, EtListViewRowCache[OBJECT_GENERAL_INDEX_TYPEGENERICREAD], 1, PhGetString(accessString));
                        accessString = PH_AUTO(EtGetAccessString(context->HandleItem->ObjectName, objectType->GenericMapping.GenericWrite));
                        PhSetListViewSubItem(context->ListViewHandle, EtListViewRowCache[OBJECT_GENERAL_INDEX_TYPEGENERICWRITE], 1, PhGetString(accessString));
                        accessString = PH_AUTO(EtGetAccessString(context->HandleItem->ObjectName, objectType->GenericMapping.GenericExecute));
                        PhSetListViewSubItem(context->ListViewHandle, EtListViewRowCache[OBJECT_GENERAL_INDEX_TYPEGENERICEXECUTE], 1, PhGetString(accessString));
                        accessString = PH_AUTO(EtGetAccessString(context->HandleItem->ObjectName, objectType->GenericMapping.GenericAll));
                        PhSetListViewSubItem(context->ListViewHandle, EtListViewRowCache[OBJECT_GENERAL_INDEX_TYPEGENERICALL], 1, PhGetString(accessString));

                        PhInitializeStringBuilder(&stringBuilder, 10);
                        if (objectType->InvalidAttributes & OBJ_KERNEL_HANDLE)
                            PhAppendStringBuilder2(&stringBuilder, L"KERNEL_HANDLE, ");
                        if (objectType->InvalidAttributes & OBJ_OPENLINK)
                            PhAppendStringBuilder2(&stringBuilder, L"OPEN_LINK, ");
                        if (objectType->InvalidAttributes & OBJ_OPENIF)
                            PhAppendStringBuilder2(&stringBuilder, L"OBJ_OPENIF, ");
                        if (objectType->InvalidAttributes & OBJ_PERMANENT)
                            PhAppendStringBuilder2(&stringBuilder, L"OBJ_PERMANENT, ");
                        if (objectType->InvalidAttributes & OBJ_EXCLUSIVE)
                            PhAppendStringBuilder2(&stringBuilder, L"OBJ_EXCLUSIVE, ");
                        if (objectType->InvalidAttributes & OBJ_INHERIT)
                            PhAppendStringBuilder2(&stringBuilder, L"OBJ_INHERIT, ");
                        if (objectType->InvalidAttributes & OBJ_CASE_INSENSITIVE)
                            PhAppendStringBuilder2(&stringBuilder, L"OBJ_CASE_INSENSITIVE, ");
                        if (objectType->InvalidAttributes & OBJ_FORCE_ACCESS_CHECK)
                            PhAppendStringBuilder2(&stringBuilder, L"OBJ_FORCE_ACCESS_CHECK, ");
                        if (objectType->InvalidAttributes & OBJ_IGNORE_IMPERSONATED_DEVICEMAP)
                            PhAppendStringBuilder2(&stringBuilder, L"OBJ_IGNORE_IMPERSONATED_DEVICEMAP, ");
                        if (objectType->InvalidAttributes & OBJ_DONT_REPARSE)
                            PhAppendStringBuilder2(&stringBuilder, L"OBJ_DONT_REPARSE, ");

                        // Remove the trailing ", ".
                        if (PhEndsWithString2(stringBuilder.String, L", ", FALSE))
                            PhRemoveEndStringBuilder(&stringBuilder, 2);

                        accessString = PH_AUTO(PhFinalStringBuilderString(&stringBuilder));
                        PhSetListViewSubItem(context->ListViewHandle, EtListViewRowCache[OBJECT_GENERAL_INDEX_TYPEINVALIDATTRIBUTES], 1, PhGetString(accessString));

                        break;
                    }
                    objectType = PH_NEXT_OBJECT_TYPE(objectType);
                }
                PhFree(objectTypes);
            }
        }
        // HACK for \REGISTRY permissions
        else if (PhEqualString2(context->HandleItem->TypeName, L"Key", TRUE) &&
            PhEqualString2(context->HandleItem->BestObjectName, L"\\REGISTRY", TRUE))
        {
            OBJECT_ATTRIBUTES objectAttributes;
            UNICODE_STRING objectName;
            HANDLE registryHandle;

            PhStringRefToUnicodeString(&context->HandleItem->BestObjectName->sr, &objectName);
            InitializeObjectAttributes(&objectAttributes, &objectName, OBJ_CASE_INSENSITIVE, NULL, NULL);
            if (NT_SUCCESS(NtOpenKey(&registryHandle, READ_CONTROL | WRITE_DAC, &objectAttributes)))
            {
                NtClose(context->HandleItem->Handle);
                PhRemoveItemList(EtObjectManagerOwnHandles, PhFindItemList(EtObjectManagerOwnHandles, context->HandleItem->Handle));
                context->HandleItem->Handle = registryHandle;
                PhAddItemList(EtObjectManagerOwnHandles, context->HandleItem->Handle);
            }
        }

        // Remove irrelevant information if we couldn't open real object
        if (PhEqualString2(context->HandleItem->TypeName, L"ALPC Port", TRUE))
        {
            PhRemoveListViewItem(context->ListViewHandle, context->ListViewRowCache[PH_PLUGIN_HANDLE_GENERAL_INDEX_ALPCCLIENT]);
            PhRemoveListViewItem(context->ListViewHandle, context->ListViewRowCache[PH_PLUGIN_HANDLE_GENERAL_INDEX_ALPCSERVER]);
            
            if (!context->HandleItem->Object)
            {
                PhSetListViewSubItem(context->ListViewHandle, context->ListViewRowCache[PH_PLUGIN_HANDLE_GENERAL_INDEX_FLAGS], 1, NULL);
                PhSetListViewSubItem(context->ListViewHandle, context->ListViewRowCache[PH_PLUGIN_HANDLE_GENERAL_INDEX_SEQUENCENUMBER], 1, NULL);
                PhSetListViewSubItem(context->ListViewHandle, context->ListViewRowCache[PH_PLUGIN_HANDLE_GENERAL_INDEX_PORTCONTEXT], 1, NULL);
            }
        }
    }

    // General ET plugin extensions
    if (!PhIsNullOrEmptyString(context->HandleItem->TypeName))
    {
        if (PhEqualString2(context->HandleItem->TypeName, L"WindowStation", TRUE))
        {
            HANDLE hWinStation;
            USEROBJECTFLAGS userFlags;
            PPH_STRING StationType;
            PH_STRINGREF StationName;
            PH_STRINGREF pathPart;

            PhAddListViewGroup(context->ListViewHandle, OBJECT_GENERAL_CATEGORY_WINDOWSTATION, L"Window Station information");

            EtListViewRowCache[OBJECT_GENERAL_INDEX_WINSTATYPE] = PhAddListViewGroupItem(context->ListViewHandle,
                OBJECT_GENERAL_CATEGORY_WINDOWSTATION, OBJECT_GENERAL_INDEX_WINSTATYPE, L"Type", NULL);
            EtListViewRowCache[OBJECT_GENERAL_INDEX_WINSTAVISIBLE] = PhAddListViewGroupItem(context->ListViewHandle,
                OBJECT_GENERAL_CATEGORY_WINDOWSTATION, OBJECT_GENERAL_INDEX_WINSTAVISIBLE, L"Visible", NULL);

            if (!PhIsNullOrEmptyString(context->HandleItem->ObjectName))
            {
                if (PhFindCharInString(context->HandleItem->ObjectName, 0, L'\\') != SIZE_MAX)
                    PhSplitStringRefAtLastChar(&context->HandleItem->ObjectName->sr, L'\\', &pathPart, &StationName);
                else
                    StationName = context->HandleItem->ObjectName->sr;
                StationType = PH_AUTO(EtGetWindowStationType(&StationName));
                PhSetListViewSubItem(context->ListViewHandle, EtListViewRowCache[OBJECT_GENERAL_INDEX_WINSTATYPE], 1, PhGetString(StationType));
            }

            if (NT_SUCCESS(EtDuplicateHandleFromProcessEx(&hWinStation, WINSTA_READATTRIBUTES, context->ProcessId, context->HandleItem->Handle)))
            {
                if (GetUserObjectInformation((HWINSTA)hWinStation,
                    UOI_FLAGS,
                    &userFlags,
                    sizeof(USEROBJECTFLAGS),
                    NULL))
                {
                    PhSetListViewSubItem(context->ListViewHandle, EtListViewRowCache[OBJECT_GENERAL_INDEX_WINSTAVISIBLE], 1,
                        userFlags.dwFlags & WSF_VISIBLE ? L"True" : L"False");
                }
                NtClose(hWinStation);
            }
        }
        else if (PhEqualString2(context->HandleItem->TypeName, L"Desktop", TRUE))
        {
            HANDLE hDesktop;

            PhAddListViewGroup(context->ListViewHandle, OBJECT_GENERAL_CATEGORY_DESKTOP, L"Desktop information");

            EtListViewRowCache[OBJECT_GENERAL_INDEX_DESKTOPIO] = PhAddListViewGroupItem(context->ListViewHandle,
                OBJECT_GENERAL_CATEGORY_DESKTOP, OBJECT_GENERAL_INDEX_DESKTOPIO, L"Input desktop", NULL);
            EtListViewRowCache[OBJECT_GENERAL_INDEX_DESKTOPSID] = PhAddListViewGroupItem(context->ListViewHandle,
                OBJECT_GENERAL_CATEGORY_DESKTOP, OBJECT_GENERAL_INDEX_DESKTOPSID, L"User SID", NULL);
            EtListViewRowCache[OBJECT_GENERAL_INDEX_DESKTOPHEAP] = PhAddListViewGroupItem(context->ListViewHandle,
                OBJECT_GENERAL_CATEGORY_DESKTOP, OBJECT_GENERAL_INDEX_DESKTOPHEAP, L"Heap size", NULL);

            if (NT_SUCCESS(EtDuplicateHandleFromProcessEx(&hDesktop, DESKTOP_READOBJECTS, context->ProcessId, context->HandleItem->Handle)))
            {
                DWORD nLengthNeeded = 0;
                ULONG vInfo = 0;
                PSID UserSid = NULL;

                if (GetUserObjectInformation((HDESK)hDesktop, UOI_IO, &vInfo, sizeof(vInfo), NULL))
                {
                    PhSetListViewSubItem(context->ListViewHandle, EtListViewRowCache[OBJECT_GENERAL_INDEX_DESKTOPIO], 1, !!vInfo ? L"True" : L"False");
                }

                GetUserObjectInformation((HDESK)hDesktop, UOI_USER_SID, NULL, 0, &nLengthNeeded);
                if (nLengthNeeded)
                    UserSid = PhAllocate(nLengthNeeded);
                if (UserSid && GetUserObjectInformation((HDESK)hDesktop, UOI_USER_SID, UserSid, nLengthNeeded, &nLengthNeeded))
                {
                    PPH_STRING sid = PH_AUTO(PhSidToStringSid(UserSid));
                    PhSetListViewSubItem(context->ListViewHandle, EtListViewRowCache[OBJECT_GENERAL_INDEX_DESKTOPSID], 1, PhGetString(sid));
                    PhFree(UserSid);
                }

                if (GetUserObjectInformation((HDESK)hDesktop, UOI_HEAPSIZE, &vInfo, sizeof(vInfo), NULL))
                {
                    PPH_STRING size = PH_AUTO(PhFormatString(L"%d MB", vInfo / 1024));
                    PhSetListViewSubItem(context->ListViewHandle, EtListViewRowCache[OBJECT_GENERAL_INDEX_DESKTOPHEAP], 1, PhGetString(size));
                }

                NtClose(hDesktop);
            }
        }
    }
}

#define T_WINSTA_INTERACTIVE L"WinSta0"
#define T_WINSTA_SYSTEM L"-0x0-3e7$"
#define T_WINSTA_ANONYMOUS L"-0x0-3e6$"
#define T_WINSTA_LOCALSERVICE L"-0x0-3e5$"
#define T_WINSTA_NETWORK_SERVICE L"-0x0-3e4$"

PPH_STRING EtGetWindowStationType(
    _In_ PPH_STRINGREF StationName
)
{
#define WINSTA_LOGON_SESSION L"logon session"

    if (PhEqualStringRef2(StationName, T_WINSTA_INTERACTIVE, TRUE))
        return PhCreateString(L"Interactive Window Station");
    if (PhFindStringInStringRefZ(StationName, T_WINSTA_SYSTEM, TRUE) != SIZE_MAX)
        return PhFormatString(L"%s %s", L"System", WINSTA_LOGON_SESSION);
    if (PhFindStringInStringRefZ(StationName, T_WINSTA_ANONYMOUS, TRUE) != SIZE_MAX)
        return PhFormatString(L"%s %s", L"Anonymous", WINSTA_LOGON_SESSION);
    if (PhFindStringInStringRefZ(StationName, T_WINSTA_LOCALSERVICE, TRUE) != SIZE_MAX)
        return PhFormatString(L"%s %s", L"Local Service", WINSTA_LOGON_SESSION);
    if (PhFindStringInStringRefZ(StationName, T_WINSTA_NETWORK_SERVICE, TRUE) != SIZE_MAX)
        return PhFormatString(L"%s %s", L"Network Service", WINSTA_LOGON_SESSION);

    return NULL;
}

VOID EtHandlePropertiesWindowUninitializing(
    _In_ PVOID Parameter
)
{
    PPH_PLUGIN_HANDLE_PROPERTIES_WINDOW_CONTEXT context = Parameter;

    if (context->OwnerPlugin == PluginInstance)
    {
        if (context->HandleItem->Handle)
        {
            NtClose(context->HandleItem->Handle);

            PhRemoveItemList(EtObjectManagerOwnHandles, PhFindItemList(EtObjectManagerOwnHandles, context->HandleItem->Handle));
            PhDereferenceObject(EtObjectManagerOwnHandles);

            PhRemoveItemSimpleHashtable(EtObjectManagerPropWnds, context->ParentWindow);
            PhDereferenceObject(EtObjectManagerPropWnds);
        }

        PhSaveWindowPlacementToSetting(SETTING_NAME_OBJMGR_PROPERTIES_WINDOW_POSITION, NULL, context->ParentWindow);

        PhDereferenceObject(context->HandleItem);
    }
}

static HPROPSHEETPAGE EtpCommonCreatePage(
    _In_ PPH_PLUGIN_HANDLE_PROPERTIES_CONTEXT Context,
    _In_ PWSTR Template,
    _In_ DLGPROC DlgProc
    )
{
    HPROPSHEETPAGE propSheetPageHandle;
    PROPSHEETPAGE propSheetPage;
    PCOMMON_PAGE_CONTEXT pageContext;

    pageContext = PhCreateAlloc(sizeof(COMMON_PAGE_CONTEXT));
    memset(pageContext, 0, sizeof(COMMON_PAGE_CONTEXT));
    pageContext->HandleItem = Context->HandleItem;
    pageContext->ProcessId = Context->ProcessId;

    memset(&propSheetPage, 0, sizeof(PROPSHEETPAGE));
    propSheetPage.dwSize = sizeof(PROPSHEETPAGE);
    propSheetPage.dwFlags = PSP_USECALLBACK;
    propSheetPage.hInstance = PluginInstance->DllBase;
    propSheetPage.pszTemplate = Template;
    propSheetPage.pfnDlgProc = DlgProc;
    propSheetPage.lParam = (LPARAM)pageContext;
    propSheetPage.pfnCallback = EtpCommonPropPageProc;

    propSheetPageHandle = CreatePropertySheetPage(&propSheetPage);
    PhDereferenceObject(pageContext); // already got a ref from above call

    return propSheetPageHandle;
}

INT CALLBACK EtpCommonPropPageProc(
    _In_ HWND hwnd,
    _In_ UINT uMsg,
    _In_ LPPROPSHEETPAGE ppsp
    )
{
    PCOMMON_PAGE_CONTEXT pageContext;

    pageContext = (PCOMMON_PAGE_CONTEXT)ppsp->lParam;

    if (uMsg == PSPCB_ADDREF)
        PhReferenceObject(pageContext);
    else if (uMsg == PSPCB_RELEASE)
        PhDereferenceObject(pageContext);

    return 1;
}

static NTSTATUS EtpDuplicateHandleFromProcess(
    _Out_ PHANDLE Handle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ PCOMMON_PAGE_CONTEXT Context
    )
{
    NTSTATUS status;
    HANDLE processHandle;

    if (!NT_SUCCESS(status = PhOpenProcess(
        &processHandle,
        PROCESS_DUP_HANDLE,
        Context->ProcessId
        )))
        return status;

    status = NtDuplicateObject(
        processHandle,
        Context->HandleItem->Handle,
        NtCurrentProcess(),
        Handle,
        DesiredAccess,
        0,
        0
        );
    NtClose(processHandle);

    return status;
}

static BOOLEAN NTAPI EnumGenericModulesCallback(
    _In_ PPH_MODULE_INFO Module,
    _In_ PVOID Context
    )
{
    if (
        Module->Type == PH_MODULE_TYPE_MODULE ||
        Module->Type == PH_MODULE_TYPE_WOW64_MODULE
        )
    {
        PhLoadModuleSymbolProvider(
            Context,
            Module->FileName,
            (ULONG64)Module->BaseAddress,
            Module->Size
            );
    }

    return TRUE;
}

INT_PTR CALLBACK EtpTpWorkerFactoryPageDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            LPPROPSHEETPAGE propSheetPage = (LPPROPSHEETPAGE)lParam;
            PCOMMON_PAGE_CONTEXT context = (PCOMMON_PAGE_CONTEXT)propSheetPage->lParam;
            HANDLE workerFactoryHandle;

            if (NT_SUCCESS(EtpDuplicateHandleFromProcess(&workerFactoryHandle, WORKER_FACTORY_QUERY_INFORMATION, context)))
            {
                WORKER_FACTORY_BASIC_INFORMATION basicInfo;

                if (NT_SUCCESS(NtQueryInformationWorkerFactory(
                    workerFactoryHandle,
                    WorkerFactoryBasicInformation,
                    &basicInfo,
                    sizeof(WORKER_FACTORY_BASIC_INFORMATION),
                    NULL
                    )))
                {
                    PPH_SYMBOL_PROVIDER symbolProvider;
                    PPH_STRING symbol = NULL;
                    WCHAR value[PH_PTR_STR_LEN_1];

                    if (symbolProvider = PhCreateSymbolProvider(NULL))
                    {
                        PhLoadSymbolProviderOptions(symbolProvider);

                        PhEnumGenericModules(
                            basicInfo.ProcessId,
                            NULL,
                            0,
                            EnumGenericModulesCallback,
                            symbolProvider
                            );

                        symbol = PhGetSymbolFromAddress(
                            symbolProvider,
                            (ULONG64)basicInfo.StartRoutine,
                            NULL,
                            NULL,
                            NULL,
                            NULL
                            );

                        PhDereferenceObject(symbolProvider);
                    }

                    if (symbol)
                    {
                        PhSetDialogItemText(
                            hwndDlg,
                            IDC_WORKERTHREADSTART,
                            PhaFormatString(L"Worker Thread Start: %s", symbol->Buffer)->Buffer
                            );
                        PhDereferenceObject(symbol);
                    }
                    else
                    {
                        PhPrintPointer(value, basicInfo.StartRoutine);
                        PhSetDialogItemText(
                            hwndDlg,
                            IDC_WORKERTHREADSTART,
                            PhaFormatString(L"Worker Thread Start: %s", value)->Buffer
                            );
                    }

                    PhPrintPointer(value, basicInfo.StartParameter);
                    PhSetDialogItemText(
                        hwndDlg,
                        IDC_WORKERTHREADCONTEXT,
                        PhaFormatString(L"Worker Thread Context: %s", value)->Buffer
                        );
                }

                NtClose(workerFactoryHandle);
            }

            PhInitializeWindowTheme(hwndDlg, !!PhGetIntegerSetting(L"EnableThemeSupport"));
        }
        break;
    }

    return FALSE;
}

PPH_STRING EtGetAccessString(
    _In_ PPH_STRING TypeName,
    _In_ ACCESS_MASK access
    )
{
    PPH_STRING AccessString = NULL;
    PPH_ACCESS_ENTRY accessEntries;
    ULONG numberOfAccessEntries;

    if (PhGetAccessEntries(
        PhGetStringOrEmpty(TypeName),
        &accessEntries,
        &numberOfAccessEntries
    ))
    {
        PPH_STRING accessString;

        accessString = PH_AUTO(PhGetAccessString(
            access,
            accessEntries,
            numberOfAccessEntries
        ));

        if (accessString->Length != 0)
        {
            AccessString = PhFormatString(
                L"0x%x (%s)",
                access,
                accessString->Buffer
            );
        }
        else
        {
            AccessString = PhFormatString(L"0x%x", access);
        }

        PhFree(accessEntries);
    }
    else
    {
        AccessString = PhFormatString(L"0x%x", access);
    }

    return AccessString;
}

typedef struct _SEARCH_HANDLE_CONTEXT
{
    PPH_STRING SearchObjectName;
    PSYSTEM_HANDLE_TABLE_ENTRY_INFO_EX HandleInfo;
    HANDLE ProcessHandle;
    PPH_LIST SearchResults;
    PPH_QUEUED_LOCK SearchResultsLock;
} SEARCH_HANDLE_CONTEXT, * PSEARCH_HANDLE_CONTEXT;

static NTSTATUS NTAPI EtpSearchHandleFunction(
    _In_ PVOID Parameter
)
{
    PSEARCH_HANDLE_CONTEXT handleContext = Parameter;
    PPH_STRING ObjectName;

    if (NT_SUCCESS(PhGetHandleInformation(handleContext->ProcessHandle, (HANDLE)handleContext->HandleInfo->HandleValue,
        handleContext->HandleInfo->ObjectTypeIndex, NULL, NULL, &ObjectName, NULL)))
    {
        if (PhStartsWithString(ObjectName, handleContext->SearchObjectName, TRUE))
        {
            PhAcquireQueuedLockExclusive(handleContext->SearchResultsLock);
            PhAddItemList(handleContext->SearchResults, handleContext->HandleInfo);
            PhReleaseQueuedLockExclusive(handleContext->SearchResultsLock);
        }
        PhDereferenceObject(ObjectName);
    }

    PhFree(handleContext);

    return STATUS_SUCCESS;
}

INT EtpEnumObjectHandles(
    _In_ PCOMMON_PAGE_CONTEXT context
    )
{
    static PH_INITONCE initOnce = PH_INITONCE_INIT;
    static ULONG FileTypeIndex;

    if (PhBeginInitOnce(&initOnce))
    {
        FileTypeIndex = PhGetObjectTypeNumberZ(L"File");
        PhEndInitOnce(&initOnce);
    }

    COLORREF ColorNormal = !!PhGetIntegerSetting(L"EnableThemeSupport") ? RGB(43, 43, 43) : GetSysColor(COLOR_WINDOW);
    COLORREF ColorOwnObject = PhGetIntegerSetting(L"ColorOwnProcesses");
    COLORREF ColorInherit = PhGetIntegerSetting(L"ColorInheritHandles");
    COLORREF ColorProtected = PhGetIntegerSetting(L"ColorProtectedHandles");
    COLORREF ColorProtectedInherit = PhGetIntegerSetting(L"ColorPartiallySuspended");

    INT OwnHandlesIndex = 0;
    PSYSTEM_HANDLE_INFORMATION_EX handles;
    ULONG_PTR i;
    ULONG SourceTypeIndex;

    BOOLEAN isDevice = PhEqualString2(context->HandleItem->TypeName, L"Device", TRUE);
    BOOLEAN isAlpcPort = PhEqualString2(context->HandleItem->TypeName, L"ALPC Port", TRUE);
    BOOLEAN isRegKey = PhEqualString2(context->HandleItem->TypeName, L"Key", TRUE);
    BOOLEAN isWinSta = PhEqualString2(context->HandleItem->TypeName, L"WindowStation", TRUE);
    BOOLEAN isTypeObject = PhEqualString2(context->HandleItem->TypeName, L"Type", TRUE);

    if (isDevice)
        SourceTypeIndex = FileTypeIndex;    // HACK
    else if (isTypeObject)
        SourceTypeIndex = PhGetObjectTypeNumber(&context->HandleItem->ObjectName->sr);   // HACK
    else
        SourceTypeIndex = context->HandleItem->TypeIndex;

    if (NT_SUCCESS(PhEnumHandlesEx(&handles)))
    {
        PPH_LIST SearchResults = PhCreateList(128);
        PH_WORK_QUEUE workQueue;
        PH_QUEUED_LOCK SearchResultsLock = { 0 };

        PPH_HASHTABLE processHandleHashtable = PhCreateSimpleHashtable(8);
        PVOID* processHandlePtr;
        HANDLE processHandle;
        PPH_STRING ObjectName;
        BOOLEAN ObjectNameMath;
        BOOLEAN useWorkQueue = isDevice && KsiLevel() < KphLevelMed;

        if (useWorkQueue)
            PhInitializeWorkQueue(&workQueue, 1, 20, 1000);

        for (i = 0; i < handles->NumberOfHandles; i++)
        {
            PSYSTEM_HANDLE_TABLE_ENTRY_INFO_EX handleInfo = &handles->Handles[i];

            // Skip other types
            if (handleInfo->ObjectTypeIndex != SourceTypeIndex)
                continue;

            ObjectNameMath = isTypeObject;

            // Lookup for matches in object name to find more handles for ALPC Port, File, Key
            if (isAlpcPort || isDevice || isRegKey || isWinSta)
            {
                // Open a handle to the process if we don't already have one.
                processHandlePtr = PhFindItemSimpleHashtable(
                    processHandleHashtable,
                    (PVOID)handleInfo->UniqueProcessId
                );

                if (processHandlePtr)
                    processHandle = (HANDLE)*processHandlePtr;
                else
                {
                    if (NT_SUCCESS(PhOpenProcess(
                        &processHandle,
                        (KsiLevel() >= KphLevelMed ? PROCESS_QUERY_LIMITED_INFORMATION : PROCESS_DUP_HANDLE),
                        (HANDLE)handleInfo->UniqueProcessId
                    )))
                    {
                        PhAddItemSimpleHashtable(
                            processHandleHashtable,
                            (PVOID)handleInfo->UniqueProcessId,
                            processHandle
                        );
                    }
                    else
                        continue;
                }

                if (isAlpcPort)
                {
                    if (NT_SUCCESS(PhGetHandleInformation(processHandle, (HANDLE)handleInfo->HandleValue,
                        handleInfo->ObjectTypeIndex, NULL, NULL, NULL, &ObjectName)))
                    {
                        if (PhEndsWithString(ObjectName, context->HandleItem->BestObjectName, TRUE))    // HACK
                            ObjectNameMath = TRUE;
                        PhDereferenceObject(ObjectName);
                    }
                }
                else
                {
                    // If we're dealing with a file handle we must take special precautions so we don't hang.
                    if (useWorkQueue)
                    {
                        PSEARCH_HANDLE_CONTEXT searchContext = PhAllocate(sizeof(SEARCH_HANDLE_CONTEXT));
                        searchContext->SearchObjectName = context->HandleItem->BestObjectName;
                        searchContext->ProcessHandle = processHandle;
                        searchContext->HandleInfo = handleInfo;
                        searchContext->SearchResults = SearchResults;
                        searchContext->SearchResultsLock = &SearchResultsLock;

                        PhQueueItemWorkQueue(&workQueue, EtpSearchHandleFunction, searchContext);
                        continue;
                    }

                    if (NT_SUCCESS(PhGetHandleInformation(processHandle, (HANDLE)handleInfo->HandleValue,
                        handleInfo->ObjectTypeIndex, NULL, NULL, &ObjectName, NULL)))
                    {
                        if (PhStartsWithString(ObjectName, context->HandleItem->BestObjectName, TRUE))
                            ObjectNameMath = TRUE;
                        PhDereferenceObject(ObjectName);
                    }
                }
            }

            if (handleInfo->Object == context->HandleItem->Object || ObjectNameMath)
            {
                PhAcquireQueuedLockExclusive(&SearchResultsLock);
                PhAddItemList(SearchResults, handleInfo);
                PhReleaseQueuedLockExclusive(&SearchResultsLock);
            }
        }

        if (useWorkQueue)
        {
            PhWaitForWorkQueue(&workQueue);
            PhDeleteWorkQueue(&workQueue);
        }

        {
            PPH_KEY_VALUE_PAIR entry;
            ULONG i = 0;

            while (PhEnumHashtable(processHandleHashtable, &entry, &i))
                NtClose((HANDLE)entry->Value);

            PhDereferenceObject(processHandleHashtable);
        }

        INT lvItemIndex;
        WCHAR value[PH_INT64_STR_LEN_1];
        PHANDLE_ENTRY entry;
        PPH_STRING columnString;
        CLIENT_ID ClientId = { 0 };

        for (i = 0; i < SearchResults->Count; i++)
        {
            PSYSTEM_HANDLE_TABLE_ENTRY_INFO_EX handleInfo = SearchResults->Items[i];

            // Skip Object Manager own handles
            if ((HANDLE)handleInfo->UniqueProcessId == NtCurrentProcessId() &&
                PhFindItemList(EtObjectManagerOwnHandles, (PVOID)handleInfo->HandleValue) != ULONG_MAX)
            {
                continue;
            }

            entry = PhAllocateZero(sizeof(HANDLE_ENTRY));
            entry->ProcessId = (HANDLE)handleInfo->UniqueProcessId;
            entry->HandleItem = PhCreateHandleItem(handleInfo);
            entry->Color = ColorNormal;

            ClientId.UniqueProcess = entry->ProcessId;
            columnString = PH_AUTO(PhGetClientIdName(&ClientId));
            lvItemIndex = PhAddListViewItem(
                context->ListViewHandle,
                handleInfo->Object == context->HandleItem->Object ? OwnHandlesIndex++ : MAXINT,     // object own handles first
                PhGetString(columnString), entry);

            PhPrintPointer(value, (PVOID)handleInfo->HandleValue);
            PhSetListViewSubItem(context->ListViewHandle, lvItemIndex, 1, value);

            columnString = PH_AUTO(EtGetAccessString(context->HandleItem->TypeName, handleInfo->GrantedAccess));
            PhSetListViewSubItem(context->ListViewHandle, lvItemIndex, 2, PhGetString(columnString));

            switch (handleInfo->HandleAttributes & (OBJ_PROTECT_CLOSE | OBJ_INHERIT))
            {
            case OBJ_PROTECT_CLOSE:
                PhSetListViewSubItem(context->ListViewHandle, lvItemIndex, 3, L"Protected");
                entry->Color = ColorProtected;
                break;
            case OBJ_INHERIT:
                PhSetListViewSubItem(context->ListViewHandle, lvItemIndex, 3, L"Inherit");
                entry->Color = ColorInherit;
                break;
            case OBJ_PROTECT_CLOSE | OBJ_INHERIT:
                PhSetListViewSubItem(context->ListViewHandle, lvItemIndex, 3, L"Protected, Inherit");
                entry->Color = ColorProtectedInherit;
                break;
            }

            // Highlight own object handles
            if (handleInfo->Object == context->HandleItem->Object)
                entry->Color = ColorOwnObject;
        }

        PhDereferenceObject(SearchResults);
        PhFree(handles);
    }

    return OwnHandlesIndex;
}

VOID EtpShowHandleProperties(
    _In_ HWND hwndDlg,
    _In_ PHANDLE_ENTRY entry
    )
{
    HANDLE processHandle;

    if (NT_SUCCESS(PhOpenProcess(
        &processHandle,
        (KsiLevel() >= KphLevelMed ? PROCESS_QUERY_LIMITED_INFORMATION : PROCESS_DUP_HANDLE),
        entry->ProcessId
    )))
    {
        PhGetHandleInformation(
            processHandle,
            entry->HandleItem->Handle,
            entry->HandleItem->TypeIndex,
            NULL,
            &entry->HandleItem->TypeName,
            &entry->HandleItem->ObjectName,
            &entry->HandleItem->BestObjectName
        );

        NtClose(processHandle);
    }

    if (!entry->HandleItem->TypeName)
        entry->HandleItem->TypeName = PhGetObjectTypeIndexName(entry->HandleItem->TypeIndex);

    PhShowHandlePropertiesEx(hwndDlg, entry->ProcessId, entry->HandleItem, NULL, NULL);
}

VOID EtpCloseObjectHandles(
    PCOMMON_PAGE_CONTEXT context,
    PHANDLE_ENTRY* listviewItems,
    ULONG numberOfItems
    )
{
    HANDLE oldHandle;

    for (ULONG i = 0; i < numberOfItems; i++)
    {
        if (PhUiCloseHandles(context->WindowHandle, listviewItems[i]->ProcessId, &listviewItems[i]->HandleItem, 1, TRUE))
        {
            oldHandle = NULL;
            if (EtDuplicateHandleFromProcessEx(&oldHandle, 0, listviewItems[i]->ProcessId, listviewItems[i]->HandleItem->Handle)
                == STATUS_INVALID_HANDLE)   // HACK for protected handles
            {
                PhRemoveListViewItem(context->ListViewHandle, PhFindListViewItemByParam(context->ListViewHandle, INT_ERROR, listviewItems[i]));
                PhClearReference(&listviewItems[i]->HandleItem);
                PhFree(listviewItems[i]);
            }
            else if (oldHandle)
                NtClose(oldHandle);
        }
        else
            break;
    }
}

COLORREF NTAPI EtpColorItemColorFunction(
    _In_ INT Index,
    _In_ PVOID Param,
    _In_opt_ PVOID Context
)
{
    PHANDLE_ENTRY entry = Param;

    return entry->Color;
}


INT_PTR CALLBACK EtpObjHandlesPageDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
)
{
    PCOMMON_PAGE_CONTEXT context = NULL;

    if (uMsg == WM_INITDIALOG)
    {
        LPPROPSHEETPAGE propSheetPage = (LPPROPSHEETPAGE)lParam;
        context = (PCOMMON_PAGE_CONTEXT)propSheetPage->lParam;

        PhSetWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT, context);
    }
    else
    {
        context = PhGetWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT);
    }

    if (!context)
        return TRUE;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            context->WindowHandle = hwndDlg;
            context->ListViewHandle = GetDlgItem(hwndDlg, IDC_LIST);

            PhSetListViewStyle(context->ListViewHandle, TRUE, TRUE);
            PhSetControlTheme(context->ListViewHandle, L"explorer");
            PhAddListViewColumn(context->ListViewHandle, 0, 0, 0, LVCFMT_LEFT, 118, L"Process");
            PhAddListViewColumn(context->ListViewHandle, 1, 1, 1, LVCFMT_LEFT, 50, L"Handle");
            PhAddListViewColumn(context->ListViewHandle, 2, 2, 2, LVCFMT_LEFT, 120, L"Access");
            PhAddListViewColumn(context->ListViewHandle, 3, 3, 3, LVCFMT_LEFT, 60, L"Attributes");
            PhSetExtendedListView(context->ListViewHandle);
            ExtendedListView_SetSort(context->ListViewHandle, 0, NoSortOrder);
            ExtendedListView_SetItemColorFunction(context->ListViewHandle, EtpColorItemColorFunction);

            ExtendedListView_SetRedraw(context->ListViewHandle, FALSE);

            PhSetCursor(PhLoadCursor(NULL, IDC_WAIT));

            INT OwnHandlesCount = EtpEnumObjectHandles(context);
            INT TotalHandlesCount = ListView_GetItemCount(context->ListViewHandle);

            PhSetCursor(PhLoadCursor(NULL, IDC_ARROW));

            PhInitializeWindowTheme(hwndDlg, !!PhGetIntegerSetting(L"EnableThemeSupport"));

            ExtendedListView_SetRedraw(context->ListViewHandle, TRUE);

            WCHAR string[PH_INT64_STR_LEN_1];
            PhPrintUInt32(string, TotalHandlesCount);
            PhSetWindowText(GetDlgItem(hwndDlg, IDC_OBJ_HANDLESTOTAL), string);
            PhPrintUInt32(string, OwnHandlesCount);
            PhSetWindowText(GetDlgItem(hwndDlg, IDC_OBJ_HANDLESBYOBJECT), string);
            PhPrintUInt32(string, TotalHandlesCount - OwnHandlesCount);
            PhSetWindowText(GetDlgItem(hwndDlg, IDC_OBJ_HANDLESBYNAME), string);

            if (PhEqualString2(context->HandleItem->TypeName, L"Type", TRUE))
                PhSetWindowText(GetDlgItem(hwndDlg, IDC_OBJ_HANDLESBYNAME_L), L"By type:");
        }
        break;
    case WM_DESTROY:
        {
            INT index = INT_ERROR;

            while ((index = PhFindListViewItemByFlags(
                context->ListViewHandle,
                index,
                LVNI_ALL
            )) != INT_ERROR)
            {
                PHANDLE_ENTRY entry;
                if (PhGetListViewItemParam(context->ListViewHandle, index, &entry))
                {
                    PhClearReference(&entry->HandleItem);
                    PhFree(entry);
                }
            }
        }
        break;
    case WM_NCDESTROY:
        {
            PhRemoveWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT);
            PhFree(context);
        }
        break;
    case WM_KEYDOWN:
        {
            PHANDLE_ENTRY* listviewItems;
            ULONG numberOfItems;

            switch (LOWORD(wParam))
            {
            case VK_RETURN:
                if (GetFocus() == context->ListViewHandle)
                {
                    PhGetSelectedListViewItemParams(context->ListViewHandle, &listviewItems, &numberOfItems);
                    if (numberOfItems == 1)
                    {
                        if (GetKeyState(VK_CONTROL) < 0)
                        {
                            PPH_PROCESS_ITEM processItem;

                            if (processItem = PhReferenceProcessItem(listviewItems[0]->ProcessId))
                            {
                                SystemInformer_ShowProcessProperties(processItem);
                                PhDereferenceObject(processItem);
                                return TRUE;
                            }
                        }
                        else
                        {
                            EtpShowHandleProperties(hwndDlg, listviewItems[0]);
                            return TRUE;
                        }
                    }
                }
                break;
            case VK_DELETE:
                if (GetFocus() == context->ListViewHandle)
                {
                    PhGetSelectedListViewItemParams(context->ListViewHandle, &listviewItems, &numberOfItems);
                    if (numberOfItems != 0)
                    {
                        EtpCloseObjectHandles(context, listviewItems, numberOfItems);
                        return TRUE;
                    }
                }
                break;
            }
        }
        break;
    case WM_NOTIFY:
        {
            PhHandleListViewNotifyBehaviors(lParam, context->ListViewHandle, PH_LIST_VIEW_DEFAULT_1_BEHAVIORS);

            REFLECT_MESSAGE_DLG(hwndDlg, context->ListViewHandle, uMsg, wParam, lParam);

            LPNMHDR header = (LPNMHDR)lParam;

            if (header->code == NM_DBLCLK)
            {
                if (header->hwndFrom != context->ListViewHandle)
                    break;

                LPNMITEMACTIVATE info = (LPNMITEMACTIVATE)header;
                PHANDLE_ENTRY entry;

                if (entry = PhGetSelectedListViewItemParam(context->ListViewHandle))
                {
                    if (GetKeyState(VK_CONTROL) < 0)
                    {
                        PPH_PROCESS_ITEM processItem;

                        if (processItem = PhReferenceProcessItem(entry->ProcessId))
                        {
                            SystemInformer_ShowProcessProperties(processItem);
                            PhDereferenceObject(processItem);
                        }
                    }
                    else
                    {
                        EtpShowHandleProperties(hwndDlg, entry);
                    }
                }  
            }
        }
        break;
    case WM_CONTEXTMENU:
        {
            PHANDLE_ENTRY* listviewItems;
            ULONG numberOfItems;

            PhGetSelectedListViewItemParams(context->ListViewHandle, &listviewItems, &numberOfItems);
            if (numberOfItems != 0)
            {
                POINT point;
                PPH_EMENU menu;
                PPH_EMENU_ITEM item;
                PPH_EMENU_ITEM propMenuItem;
                PPH_EMENU_ITEM gotoMenuItem;

                point.x = GET_X_LPARAM(lParam);
                point.y = GET_Y_LPARAM(lParam);

                if (point.x == -1 && point.y == -1)
                    PhGetListViewContextMenuPoint(context->ListViewHandle, &point);

                menu = PhCreateEMenu();

                PhInsertEMenuItem(menu, PhCreateEMenuItem(0, IDC_CLOSEHANDLE, L"C&lose\bDel", NULL, NULL), ULONG_MAX);
                PhInsertEMenuItem(menu, propMenuItem = PhCreateEMenuItem(0, IDC_PROPERTIES, L"Prope&rties\bEnter", NULL, NULL), ULONG_MAX);
                PhInsertEMenuItem(menu, gotoMenuItem = PhCreateEMenuItem(0, IDC_GOTOPROCESS, L"&Go to process\bCtrl+Enter", NULL, NULL), ULONG_MAX);
                PhInsertEMenuItem(menu, PhCreateEMenuSeparator(), ULONG_MAX);
                PhInsertEMenuItem(menu, PhCreateEMenuItem(0, IDC_COPY, L"&Copy\bCtrl+C", NULL, NULL), ULONG_MAX);
                PhInsertCopyListViewEMenuItem(menu, IDC_COPY, context->ListViewHandle);
                PhSetFlagsEMenuItem(menu, IDC_PROPERTIES, PH_EMENU_DEFAULT, PH_EMENU_DEFAULT);
                if (numberOfItems > 1)
                {
                    PhSetDisabledEMenuItem(propMenuItem);
                    PhSetDisabledEMenuItem(gotoMenuItem);
                }

                item = PhShowEMenu(
                    menu,
                    hwndDlg,
                    PH_EMENU_SHOW_LEFTRIGHT,
                    PH_ALIGN_LEFT | PH_ALIGN_TOP,
                    point.x,
                    point.y
                );

                if (item && item->Id != ULONG_MAX && !PhHandleCopyListViewEMenuItem(item))
                {
                    switch (item->Id)
                    {
                    case IDC_CLOSEHANDLE:
                    {
                        EtpCloseObjectHandles(context, listviewItems, numberOfItems);
                    }
                    break;
                    case IDC_PROPERTIES:
                    {
                        EtpShowHandleProperties(hwndDlg, listviewItems[0]);
                    }
                    break;
                    case IDC_GOTOPROCESS:
                    {
                        PPH_PROCESS_ITEM processItem;

                        if (processItem = PhReferenceProcessItem(listviewItems[0]->ProcessId))
                        {
                            SystemInformer_ShowProcessProperties(processItem);
                            PhDereferenceObject(processItem);
                        }
                    }
                    break;
                    case IDC_COPY:
                    {
                        PhCopyListView(context->ListViewHandle);
                    }
                    break;
                    }

                    PhDestroyEMenu(menu);
                }
            }
        }
        break;
    } 

    return FALSE;
}

BOOL CALLBACK EtpEnumDesktopsCallback(
    _In_ LPWSTR lpszDesktop,
    _In_ LPARAM lParam
)
{
    PCOMMON_PAGE_CONTEXT context = (PCOMMON_PAGE_CONTEXT)lParam;
    HANDLE hDesktop;
    UINT lvItemIndex;

    lvItemIndex = PhAddListViewItem(context->ListViewHandle, MAXINT, lpszDesktop, 0);

    if (hDesktop = OpenDesktop(lpszDesktop, 0, FALSE, DESKTOP_READOBJECTS))
    {
        DWORD nLengthNeeded = 0;
        ULONG vInfo = 0;
        PSID UserSid = NULL;
        
        GetUserObjectInformation((HDESK)hDesktop, UOI_USER_SID, NULL, 0, &nLengthNeeded);
        if (nLengthNeeded)
            UserSid = PhAllocate(nLengthNeeded);
        if (UserSid && GetUserObjectInformation((HDESK)hDesktop, UOI_USER_SID, UserSid, nLengthNeeded, &nLengthNeeded))
        {
            PPH_STRING sid = PH_AUTO(PhSidToStringSid(UserSid));
            PhSetListViewSubItem(context->ListViewHandle, lvItemIndex, 1, PhGetString(sid));
            PhFree(UserSid);
        }

        if (GetUserObjectInformation((HDESK)hDesktop, UOI_HEAPSIZE, &vInfo, sizeof(vInfo), NULL))
        {
            PPH_STRING size = PH_AUTO(PhFormatString(L"%d MB", vInfo / 1024));
            PhSetListViewSubItem(context->ListViewHandle, lvItemIndex, 2, PhGetString(size));
        }

        if (GetUserObjectInformation((HDESK)hDesktop, UOI_IO, &vInfo, sizeof(vInfo), NULL))
        {
            PhSetListViewSubItem(context->ListViewHandle, lvItemIndex, 3, !!vInfo ? L"True" : L"False");
        }

        NtClose(hDesktop);
    }

    return TRUE;
}

INT_PTR CALLBACK EtpWinStaPageDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
)
{
    PCOMMON_PAGE_CONTEXT context = NULL;

    if (uMsg == WM_INITDIALOG)
    {
        LPPROPSHEETPAGE propSheetPage = (LPPROPSHEETPAGE)lParam;
        context = (PCOMMON_PAGE_CONTEXT)propSheetPage->lParam;

        PhSetWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT, context);
    }
    else
    {
        context = PhGetWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT);
    }

    if (!context)
        return TRUE;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            HANDLE hWinStation;
            context->WindowHandle = hwndDlg;
            context->ListViewHandle = GetDlgItem(hwndDlg, IDC_LIST);

            PhSetListViewStyle(context->ListViewHandle, TRUE, TRUE);
            PhSetControlTheme(context->ListViewHandle, L"explorer");
            PhAddListViewColumn(context->ListViewHandle, 0, 0, 0, LVCFMT_LEFT, 152, L"Name");
            PhAddListViewColumn(context->ListViewHandle, 1, 1, 1, LVCFMT_LEFT, 105, L"SID");
            PhAddListViewColumn(context->ListViewHandle, 2, 2, 2, LVCFMT_LEFT, 62, L"Heap Size");
            PhAddListViewColumn(context->ListViewHandle, 3, 3, 3, LVCFMT_LEFT, 45, L"Input");
            PhSetExtendedListView(context->ListViewHandle);
            ExtendedListView_SetSort(context->ListViewHandle, 0, NoSortOrder);

            ExtendedListView_SetRedraw(context->ListViewHandle, FALSE);

            if (NT_SUCCESS(EtDuplicateHandleFromProcessEx(&hWinStation, WINSTA_ENUMDESKTOPS, context->ProcessId, context->HandleItem->Handle)))
            {
                EnumDesktops(hWinStation, EtpEnumDesktopsCallback, (LPARAM)context);
                NtClose(hWinStation);
            }

            ExtendedListView_SetRedraw(context->ListViewHandle, TRUE);

            PhInitializeWindowTheme(hwndDlg, !!PhGetIntegerSetting(L"EnableThemeSupport"));
        }
        break;
    case WM_NCDESTROY:
        {
            PhRemoveWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT);
            PhFree(context);
        }
        break;
    case WM_NOTIFY:
        {
            PhHandleListViewNotifyBehaviors(lParam, context->ListViewHandle, PH_LIST_VIEW_DEFAULT_1_BEHAVIORS);
            REFLECT_MESSAGE_DLG(hwndDlg, context->ListViewHandle, uMsg, wParam, lParam);
        }
        break;
    case WM_CONTEXTMENU:
        {
            PVOID* listviewItems;
            ULONG numberOfItems;

            PhGetSelectedListViewItemParams(context->ListViewHandle, &listviewItems, &numberOfItems);
            if (numberOfItems != 0)
            {
                POINT point;
                PPH_EMENU menu;
                PPH_EMENU_ITEM item;
                HWND ListViewHandle = GetDlgItem(hwndDlg, IDC_LIST);

                point.x = GET_X_LPARAM(lParam);
                point.y = GET_Y_LPARAM(lParam);

                if (point.x == -1 && point.y == -1)
                    PhGetListViewContextMenuPoint(ListViewHandle, &point);

                menu = PhCreateEMenu();

                PhInsertEMenuItem(menu, PhCreateEMenuItem(0, IDC_COPY, L"&Copy\bCtrl+C", NULL, NULL), ULONG_MAX);
                PhInsertCopyListViewEMenuItem(menu, IDC_COPY, ListViewHandle);

                item = PhShowEMenu(
                    menu,
                    hwndDlg,
                    PH_EMENU_SHOW_LEFTRIGHT,
                    PH_ALIGN_LEFT | PH_ALIGN_TOP,
                    point.x,
                    point.y
                );

                if (item && item->Id != ULONG_MAX && !PhHandleCopyListViewEMenuItem(item))
                {
                    switch (item->Id)
                    {
                    case IDC_COPY:
                        PhCopyListView(ListViewHandle);
                        break;
                    }
                }

                PhDestroyEMenu(menu);
            }
        }
        break;
    }

    return FALSE;
}
