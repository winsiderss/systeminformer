/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32         2010-2011
 *     Dart Vanya   2024
 *
 */

#include "exttools.h"
#include <symprv.h>
#include <secedit.h>
#include <hndlinfo.h>
#include <kphuser.h>

 // Columns

#define ETHNLVC_PROCESS 0
#define ETHNLVC_HANDLE 1
#define ETHNLVC_ACCESS 2
#define ETHNLVC_ATTRIBUTES 3
#define ETHNLVC_NAME 4
#define ETHNLVC_ORIGNAME 5

#define ETDTLVC_NAME 0
#define ETDTLVC_SID 1
#define ETDTLVC_HEAP 2
#define ETDTLVC_IO 3

typedef struct _COMMON_PAGE_CONTEXT
{
    PH_LAYOUT_MANAGER LayoutManager;

    PPH_HANDLE_ITEM HandleItem;
    HANDLE ProcessId;
    HWND WindowHandle;
    HWND ListViewHandle;

    // Handles tab
    ULONG TotalHandlesCount;
    ULONG OwnHandlesCount;
    BOOLEAN ColumnsAdded;
} COMMON_PAGE_CONTEXT, *PCOMMON_PAGE_CONTEXT;

typedef struct _ET_HANDLE_ENTRY
{
    HANDLE ProcessId;
    PPH_HANDLE_ITEM HandleItem;
    COLORREF Color;
    BOOLEAN OwnHandle;
    BOOLEAN UseCustomColor;
} ET_HANDLE_ENTRY, *PET_HANDLE_ENTRY;

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
    _In_ ACCESS_MASK Access
    );

PPH_STRING EtGetAccessString2(
    _In_ PPH_STRINGREF TypeName,
    _In_ ACCESS_MASK Access
    );

PPH_STRING EtGetAccessStringZ(
    _In_ PCWSTR TypeName,
    _In_ ACCESS_MASK Access
    );

INT_PTR CALLBACK EtpWinStaPageDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

VOID EtUpdateHandleItem(
    _In_ HANDLE ProcessId,
    _Inout_ PPH_HANDLE_ITEM HandleItem
    );

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
                    memmove(
                        &objectProperties->Pages[2],
                        &objectProperties->Pages[1],
                        (objectProperties->NumberOfPages - 1) * sizeof(HPROPSHEETPAGE)
                        );
                }

                objectProperties->Pages[1] = page;
                objectProperties->NumberOfPages++;
            }
        }

        // Object Manager
        if (EtObjectManagerDialogHandle)
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
                    memmove(
                        &objectProperties->Pages[2],
                        &objectProperties->Pages[1],
                        (objectProperties->NumberOfPages - 1) * sizeof(HPROPSHEETPAGE)
                        );
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
    OBJECT_GENERAL_CATEGORY_DEVICE = PH_PLUGIN_HANDLE_GENERAL_CATEGORY_MAXIMUM,
    OBJECT_GENERAL_CATEGORY_DRIVER,
    OBJECT_GENERAL_CATEGORY_TYPE,
    OBJECT_GENERAL_CATEGORY_TYPE_ACCESS,
    OBJECT_GENERAL_CATEGORY_WINDOWSTATION,
    OBJECT_GENERAL_CATEGORY_DESKTOP,
    OBJECT_GENERAL_CATEGORY_SESSION
} ET_OBJECT_GENERAL_CATEGORY;

#define OBJECT_GENERAL_INDEX_ATTRIBUTES            0
#define OBJECT_GENERAL_INDEX_CREATIONTIME          1

#define OBJECT_GENERAL_INDEX_DEVICEDRVLOW          0
#define OBJECT_GENERAL_INDEX_DEVICEDRVLOWPATH      1
#define OBJECT_GENERAL_INDEX_DEVICEDRVHIGH         2
#define OBJECT_GENERAL_INDEX_DEVICEDRVHIGHPATH     3
#define OBJECT_GENERAL_INDEX_DEVICEPNPNAME         4

#define OBJECT_GENERAL_INDEX_DRIVERIMAGE           0
#define OBJECT_GENERAL_INDEX_DRIVERSERVICE         1
#define OBJECT_GENERAL_INDEX_DRIVERSIZE            2
#define OBJECT_GENERAL_INDEX_DRIVERSTART           3
#define OBJECT_GENERAL_INDEX_DRIVERFLAGS           4

#define OBJECT_GENERAL_INDEX_TYPEINDEX             0
#define OBJECT_GENERAL_INDEX_TYPEOBJECTS           1
#define OBJECT_GENERAL_INDEX_TYPEHANDLES           2
#define OBJECT_GENERAL_INDEX_TYPEPEAKOBJECTS       3
#define OBJECT_GENERAL_INDEX_TYPEPEAKHANDLES       4
#define OBJECT_GENERAL_INDEX_TYPEPOOLTYPE          5
#define OBJECT_GENERAL_INDEX_TYPEPAGECHARGE        6
#define OBJECT_GENERAL_INDEX_TYPENPAGECHARGE       7

#define OBJECT_GENERAL_INDEX_TYPEVALIDMASK         0
#define OBJECT_GENERAL_INDEX_TYPEGENERICREAD       1
#define OBJECT_GENERAL_INDEX_TYPEGENERICWRITE      2
#define OBJECT_GENERAL_INDEX_TYPEGENERICEXECUTE    3
#define OBJECT_GENERAL_INDEX_TYPEGENERICALL        4
#define OBJECT_GENERAL_INDEX_TYPEINVALIDATTRIBUTES 5

#define OBJECT_GENERAL_INDEX_WINSTATYPE            0
#define OBJECT_GENERAL_INDEX_WINSTAVISIBLE         1

#define OBJECT_GENERAL_INDEX_DESKTOPIO             0
#define OBJECT_GENERAL_INDEX_DESKTOPSID            1
#define OBJECT_GENERAL_INDEX_DESKTOPHEAP           2

#define OBJECT_GENERAL_INDEX_SESSIONNAME           0
#define OBJECT_GENERAL_INDEX_SESSIONID             1
#define OBJECT_GENERAL_INDEX_SESSIONUSERNAME       2
#define OBJECT_GENERAL_INDEX_SESSIONSTATE          3
#define OBJECT_GENERAL_INDEX_SESSIONLOGON          4
#define OBJECT_GENERAL_INDEX_SESSIONCONNECT        5
#define OBJECT_GENERAL_INDEX_SESSIONDISCONNECT     6
#define OBJECT_GENERAL_INDEX_SESSIONLASTINPUT      7

typedef enum _ET_OBJECT_POOLTYPE
{
    PagedPool = 1,
    NonPagedPool = 0,
    NonPagedPoolNx = 0x200,
    NonPagedPoolSessionNx = NonPagedPoolNx + 32,
    PagedPoolSessionNx = NonPagedPoolNx + 33
} ET_OBJECT_POOLTYPE;

#define OBJECT_CHILD_HANDLEPROP_WINDOW 1
#define OBJECT_CORRECT_HANDLES_COUNT(real_count) ((ULONG)(real_count) - 1)

typedef struct _ET_GENERAL_PAGE_CONTEXT
{
    WNDPROC OldWndProc;
    HWND ParentWindow;
    BOOLEAN PageSwitched;
} ET_GENERAL_PAGE_CONTEXT, *PET_GENERAL_PAGE_CONTEXT;

LRESULT CALLBACK EtpGeneralPageWindowSubclassProc(
    _In_ HWND hWnd,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PET_GENERAL_PAGE_CONTEXT context;
    WNDPROC oldWndProc;

    if (!(context = PhGetWindowContext(hWnd, 'OBJH')))
        return FALSE;

    oldWndProc = context->OldWndProc;

    switch (uMsg)
    {
    case WM_NCDESTROY:
        {
            PhSetWindowProcedure(hWnd, oldWndProc);
            PhRemoveWindowContext(hWnd, 'OBJH');
            PhFree(context);
        }
        break;
    }

    //if (uMsg == WM_NOTIFY && ((LPNMHDR)lParam)->code == PSN_QUERYINITIALFOCUS)  // HACK
    //{
    //    if (!context->PageSwitched)
    //    {
    //        PropSheet_SetCurSel(context->ParentWindow, NULL, 1);
    //        context->PageSwitched = TRUE;
    //    }
    //}

    return CallWindowProc(oldWndProc, hWnd, uMsg, wParam, lParam);
}

VOID EtHandlePropertiesWindowInitialized(
    _In_ PVOID Parameter
    )
{
    PPH_PLUGIN_HANDLE_PROPERTIES_WINDOW_CONTEXT context = Parameter;
    WCHAR string[PH_INT64_STR_LEN_1];

    if (EtObjectManagerDialogHandle)
    {
        // HACK
        //if (PhGetIntegerPairSetting(SETTING_NAME_OBJMGR_PROPERTIES_WINDOW_POSITION).X != 0)
        //    PhLoadWindowPlacementFromSetting(SETTING_NAME_OBJMGR_PROPERTIES_WINDOW_POSITION, NULL, context->ParentWindow);
        //else
        //    PhCenterWindow(context->ParentWindow, GetParent(context->ParentWindow)); // HACK

        PhSetWindowIcon(context->ParentWindow, EtObjectManagerPropIcon, NULL, 0);

        // Open Handles page if needed
        //if (EtObjectManagerShowHandlesPage)
        //{
        //    HWND generalPage = GetParent(context->ListViewHandle);  // HACK
        //    PET_GENERAL_PAGE_CONTEXT pageContext = PhAllocateZero(sizeof(ET_GENERAL_PAGE_CONTEXT));
        //    pageContext->OldWndProc = PhGetWindowProcedure(generalPage);
        //    pageContext->ParentWindow = context->ParentWindow;
        //    PhSetWindowContext(generalPage, 'OBJH', pageContext);
        //    PhSetWindowProcedure(generalPage, EtpGeneralPageWindowSubclassProc);
        //
        //    EtObjectManagerShowHandlesPage = FALSE;
        //}

        // Show real handles count
        if (context->ProcessId == NtCurrentProcessId())
        {
            ULONG64 real_count = ULONG64_MAX;
            PPH_STRING count;

            count = PH_AUTO(PhGetListViewItemText(context->ListViewHandle, PH_PLUGIN_HANDLE_GENERAL_INDEX_HANDLES, 1));

            if (!PhIsNullOrEmptyString(count) && PhStringToUInt64(&count->sr, 0, &real_count) && real_count > 0)
            {
                PhPrintUInt32(string, OBJECT_CORRECT_HANDLES_COUNT(real_count));
                PhSetListViewSubItem(context->ListViewHandle, PH_PLUGIN_HANDLE_GENERAL_INDEX_HANDLES, 1, string);
            }

            // HACK for \REGISTRY permissions
            if (PhEqualString2(context->HandleItem->TypeName, L"Key", TRUE) &&
                PhEqualString2(context->HandleItem->ObjectName, L"\\REGISTRY", TRUE))
            {
                HANDLE registryHandle;
                OBJECT_ATTRIBUTES objectAttributes;
                UNICODE_STRING objectName;

                RtlInitUnicodeString(&objectName, L"\\REGISTRY");
                InitializeObjectAttributes(
                    &objectAttributes,
                    &objectName,
                    OBJ_CASE_INSENSITIVE,
                    NULL,
                    NULL
                    );

                if (NT_SUCCESS(NtOpenKey(&registryHandle, READ_CONTROL | WRITE_DAC, &objectAttributes)))
                {
                    PhRemoveItemList(EtObjectManagerOwnHandles, PhFindItemList(EtObjectManagerOwnHandles, context->HandleItem->Handle));
                    NtClose(context->HandleItem->Handle);
                    context->HandleItem->Handle = registryHandle;
                    PhAddItemList(EtObjectManagerOwnHandles, registryHandle);
                }
            }
        }

        // Removing of row breaks cached indexes, so hide reference value instead
        //PhSetListViewSubItem(context->ListViewHandle, context->ListViewRowCache[PH_PLUGIN_HANDLE_GENERAL_INDEX_REFERENCES], 1, L"");

        PhAddListViewGroupItem(context->ListViewHandle, PH_PLUGIN_HANDLE_GENERAL_CATEGORY_BASICINFO, OBJECT_GENERAL_INDEX_ATTRIBUTES, L"Object attributes", NULL);

        {
            // Show object attributes
            PPH_STRING Attributes;
            PH_STRING_BUILDER stringBuilder;

            PhInitializeStringBuilder(&stringBuilder, 10);
            if (context->HandleItem->Attributes & OBJ_PERMANENT)
                PhAppendStringBuilder2(&stringBuilder, L"Permanent, ");
            if (context->HandleItem->Attributes & OBJ_EXCLUSIVE)
                PhAppendStringBuilder2(&stringBuilder, L"Exclusive, ");
            if (context->HandleItem->Attributes & OBJ_KERNEL_HANDLE)
                PhAppendStringBuilder2(&stringBuilder, L"Kernel object, ");
            if (context->HandleItem->Attributes & PH_OBJ_KERNEL_ACCESS_ONLY)
                PhAppendStringBuilder2(&stringBuilder, L"Kernel only access, ");

            // Remove the trailing ", ".
            if (PhEndsWithString2(stringBuilder.String, L", ", FALSE))
                PhRemoveEndStringBuilder(&stringBuilder, 2);

            Attributes = PH_AUTO(PhFinalStringBuilderString(&stringBuilder));
            PhSetListViewSubItem(context->ListViewHandle, OBJECT_GENERAL_INDEX_ATTRIBUTES, 1, PhGetString(Attributes));
        }
    }

    // General ET plugin extensions
    if (!PhIsNullOrEmptyString(context->HandleItem->TypeName))
    {
        // Show Driver image information
        if (PhEqualString2(context->HandleItem->TypeName, L"Driver", TRUE))
        {
            HANDLE driverObject;
            PPH_STRING driverName;
            KPH_DRIVER_BASIC_INFORMATION basicInfo;

            PhAddListViewGroup(context->ListViewHandle, OBJECT_GENERAL_CATEGORY_DRIVER, L"Driver information");
            PhAddListViewGroupItem(context->ListViewHandle, OBJECT_GENERAL_CATEGORY_DRIVER, OBJECT_GENERAL_INDEX_DRIVERIMAGE, L"Driver Image", NULL);
            PhAddListViewGroupItem(context->ListViewHandle, OBJECT_GENERAL_CATEGORY_DRIVER, OBJECT_GENERAL_INDEX_DRIVERSERVICE, L"Driver Service Name", NULL);
            PhAddListViewGroupItem(context->ListViewHandle, OBJECT_GENERAL_CATEGORY_DRIVER, OBJECT_GENERAL_INDEX_DRIVERSIZE, L"Driver Size", NULL);
            PhAddListViewGroupItem(context->ListViewHandle, OBJECT_GENERAL_CATEGORY_DRIVER, OBJECT_GENERAL_INDEX_DRIVERSTART, L"Driver Start Address", NULL);
            PhAddListViewGroupItem(context->ListViewHandle, OBJECT_GENERAL_CATEGORY_DRIVER, OBJECT_GENERAL_INDEX_DRIVERFLAGS, L"Driver Flags", NULL);

            if (KsiLevel() == KphLevelMax &&
                NT_SUCCESS(EtDuplicateHandleFromProcessEx(
                &driverObject,
                READ_CONTROL,
                context->ProcessId,
                NULL,
                context->HandleItem->Handle
                )))
            {
                if (NT_SUCCESS(PhGetDriverImageFileName(driverObject, &driverName)))
                {
                    PhSetListViewSubItem(context->ListViewHandle, OBJECT_GENERAL_INDEX_DRIVERIMAGE, 1, PhGetString(driverName));
                    PhDereferenceObject(driverName);
                }

                if (NT_SUCCESS(PhGetDriverServiceKeyName(driverObject, &driverName)))
                {
                    PhSetListViewSubItem(context->ListViewHandle, OBJECT_GENERAL_INDEX_DRIVERSERVICE, 1, PhGetString(driverName));
                    PhDereferenceObject(driverName);
                }

                if (NT_SUCCESS(KphQueryInformationDriver(
                    driverObject,
                    KphDriverBasicInformation,
                    &basicInfo,
                    sizeof(KPH_DRIVER_BASIC_INFORMATION),
                    NULL
                    )))
                {
                    PhSetListViewSubItem(
                        context->ListViewHandle,
                        OBJECT_GENERAL_INDEX_DRIVERSIZE, 1,
                        PhGetString(PH_AUTO_T(PH_STRING, PhFormatSize(basicInfo.DriverSize, ULONG_MAX)))
                        );

                    PhPrintPointer(string, basicInfo.DriverStart);
                    PhSetListViewSubItem(context->ListViewHandle, OBJECT_GENERAL_INDEX_DRIVERSTART, 1, string);

                    PhPrintPointer(string, ULongToPtr(basicInfo.Flags));
                    PhSetListViewSubItem(context->ListViewHandle, OBJECT_GENERAL_INDEX_DRIVERFLAGS, 1, string);
                }

                NtClose(driverObject);
            }
        }
        // Show Device drivers information
        else if (PhEqualString2(context->HandleItem->TypeName, L"Device", TRUE))
        {
            HANDLE deviceObject;
            HANDLE deviceBaseObject;
            HANDLE driverObject;
            PPH_STRING driverName;
            OBJECT_ATTRIBUTES objectAttributes;
            UNICODE_STRING objectName;

            PhAddListViewGroup(context->ListViewHandle, OBJECT_GENERAL_CATEGORY_DEVICE, L"Device information");
            PhAddListViewGroupItem(context->ListViewHandle, OBJECT_GENERAL_CATEGORY_DEVICE, OBJECT_GENERAL_INDEX_DEVICEDRVLOW, L"Lower-edge driver", NULL);
            PhAddListViewGroupItem(context->ListViewHandle, OBJECT_GENERAL_CATEGORY_DEVICE, OBJECT_GENERAL_INDEX_DEVICEDRVLOWPATH, L"Lower-edge driver image", NULL);
            PhAddListViewGroupItem(context->ListViewHandle, OBJECT_GENERAL_CATEGORY_DEVICE, OBJECT_GENERAL_INDEX_DEVICEDRVHIGH, L"Upper-edge driver", NULL);
            PhAddListViewGroupItem(context->ListViewHandle, OBJECT_GENERAL_CATEGORY_DEVICE, OBJECT_GENERAL_INDEX_DEVICEDRVHIGHPATH, L"Upper-edge driver Image", NULL);
            PhAddListViewGroupItem(context->ListViewHandle, OBJECT_GENERAL_CATEGORY_DEVICE, OBJECT_GENERAL_INDEX_DEVICEPNPNAME, L"PnP Device Name", NULL);

            if (KsiLevel() == KphLevelMax &&
                !PhIsNullOrEmptyString(context->HandleItem->ObjectName))
            {
                PhStringRefToUnicodeString(&context->HandleItem->ObjectName->sr, &objectName);
                InitializeObjectAttributes(&objectAttributes, &objectName, OBJ_CASE_INSENSITIVE, NULL, NULL);

                if (NT_SUCCESS(KphOpenDevice(&deviceObject, READ_CONTROL, &objectAttributes)))
                {
                    if (NT_SUCCESS(KphOpenDeviceDriver(deviceObject, READ_CONTROL, &driverObject)))
                    {
                        if (NT_SUCCESS(PhGetDriverName(driverObject, &driverName)))
                        {
                            PhSetListViewSubItem(context->ListViewHandle, OBJECT_GENERAL_INDEX_DEVICEDRVHIGH, 1, PhGetString(driverName));
                            PhDereferenceObject(driverName);
                        }

                        if (NT_SUCCESS(PhGetDriverImageFileName(driverObject, &driverName)))
                        {
                            PhSetListViewSubItem(context->ListViewHandle, OBJECT_GENERAL_INDEX_DEVICEDRVHIGHPATH, 1, PhGetString(driverName));
                            PhDereferenceObject(driverName);
                        }

                        NtClose(driverObject);
                    }

                    if (NT_SUCCESS(KphOpenDeviceBaseDevice(deviceObject, READ_CONTROL, &deviceBaseObject)))
                    {
                        if (NT_SUCCESS(KphOpenDeviceDriver(deviceBaseObject, READ_CONTROL, &driverObject)))
                        {
                            if (NT_SUCCESS(PhGetDriverName(driverObject, &driverName)))
                            {
                                PhSetListViewSubItem(context->ListViewHandle, OBJECT_GENERAL_INDEX_DEVICEDRVLOW, 1, PhGetString(driverName));
                                PhDereferenceObject(driverName);
                            }

                            if (NT_SUCCESS(PhGetDriverImageFileName(driverObject, &driverName)))
                            {
                                PhSetListViewSubItem(context->ListViewHandle, OBJECT_GENERAL_INDEX_DEVICEDRVLOWPATH, 1, PhGetString(driverName));
                                PhDereferenceObject(driverName);
                            }

                            NtClose(driverObject);
                        }

                        NtClose(deviceBaseObject);
                    }

                    NtClose(deviceObject);
                }
            }

            if (!PhIsNullOrEmptyString(context->HandleItem->ObjectName) &&
                (driverName = PhGetPnPDeviceName(context->HandleItem->ObjectName)))
            {
                ULONG_PTR columnPos = PhFindLastCharInString(driverName, 0, L':');
                PPH_STRING devicePdoName = PhSubstring(driverName, 0, columnPos - 5);
                PPH_STRING driveLetter;

                if (PhStartsWithString2(context->HandleItem->ObjectName, L"\\Device\\HarddiskVolume", TRUE) &&
                    (driveLetter = PhResolveDevicePrefix(&context->HandleItem->ObjectName->sr)))
                {
                    PhMoveReference(&devicePdoName, PhFormatString(L"%s (%s)", PhGetString(devicePdoName), PhGetString(driveLetter)));
                    PhDereferenceObject(driveLetter);
                }
                PhSetListViewSubItem(context->ListViewHandle, OBJECT_GENERAL_INDEX_DEVICEPNPNAME, 1, PhGetString(devicePdoName));
                PhDereferenceObject(devicePdoName);
                PhDereferenceObject(driverName);
            }
        }
        else if (PhEqualString2(context->HandleItem->TypeName, L"WindowStation", TRUE))
        {
            HWINSTA hWinStation;
            USEROBJECTFLAGS userFlags;
            PPH_STRING stationType;
            PH_STRINGREF stationName;
            PH_STRINGREF pathPart;

            PhAddListViewGroup(context->ListViewHandle, OBJECT_GENERAL_CATEGORY_WINDOWSTATION, L"Window Station information");
            PhAddListViewGroupItem(context->ListViewHandle, OBJECT_GENERAL_CATEGORY_WINDOWSTATION, OBJECT_GENERAL_INDEX_WINSTATYPE, L"Type", NULL);
            PhAddListViewGroupItem(context->ListViewHandle, OBJECT_GENERAL_CATEGORY_WINDOWSTATION, OBJECT_GENERAL_INDEX_WINSTAVISIBLE, L"Visible", NULL);

            if (!PhIsNullOrEmptyString(context->HandleItem->ObjectName))
            {
                if (!PhSplitStringRefAtLastChar(&context->HandleItem->ObjectName->sr, OBJ_NAME_PATH_SEPARATOR, &pathPart, &stationName))
                    stationName = context->HandleItem->ObjectName->sr;
                stationType = PH_AUTO(EtGetWindowStationType(&stationName));
                PhSetListViewSubItem(context->ListViewHandle, OBJECT_GENERAL_INDEX_WINSTATYPE, 1, PhGetString(stationType));
            }

            if (NT_SUCCESS(EtDuplicateHandleFromProcessEx(
                (PHANDLE)&hWinStation,
                WINSTA_READATTRIBUTES,
                context->ProcessId,
                NULL,
                context->HandleItem->Handle
                )))
            {
                if (GetUserObjectInformation(
                    hWinStation,
                    UOI_FLAGS,
                    &userFlags,
                    sizeof(USEROBJECTFLAGS),
                    NULL
                    ))
                {
                    PhSetListViewSubItem(context->ListViewHandle, OBJECT_GENERAL_INDEX_WINSTAVISIBLE, 1, userFlags.dwFlags & WSF_VISIBLE ? L"True" : L"False");
                }

                CloseWindowStation(hWinStation);
            }
        }
        else if (PhEqualString2(context->HandleItem->TypeName, L"Desktop", TRUE))
        {
            HDESK hDesktop;

            PhAddListViewGroup(context->ListViewHandle, OBJECT_GENERAL_CATEGORY_DESKTOP, L"Desktop information");
            PhAddListViewGroupItem(context->ListViewHandle, OBJECT_GENERAL_CATEGORY_DESKTOP, OBJECT_GENERAL_INDEX_DESKTOPIO, L"Input desktop", NULL);
            PhAddListViewGroupItem(context->ListViewHandle, OBJECT_GENERAL_CATEGORY_DESKTOP, OBJECT_GENERAL_INDEX_DESKTOPSID, L"User SID", NULL);
            PhAddListViewGroupItem(context->ListViewHandle, OBJECT_GENERAL_CATEGORY_DESKTOP, OBJECT_GENERAL_INDEX_DESKTOPHEAP, L"Heap size", NULL);

            if (NT_SUCCESS(EtDuplicateHandleFromProcessEx(
                (PHANDLE)&hDesktop,
                DESKTOP_READOBJECTS,
                context->ProcessId,
                NULL,
                context->HandleItem->Handle
                )))
            {
                ULONG vInfo = 0;
                ULONG length = SECURITY_MAX_SID_SIZE;
                UCHAR buffer[SECURITY_MAX_SID_SIZE];
                PSID UserSid = (PSID)buffer;

                if (GetUserObjectInformation(hDesktop, UOI_IO, &vInfo, sizeof(vInfo), NULL))
                {
                    PhSetListViewSubItem(context->ListViewHandle, OBJECT_GENERAL_INDEX_DESKTOPIO, 1, !!vInfo ? L"True" : L"False");
                }

                if (GetUserObjectInformation(hDesktop, UOI_USER_SID, UserSid, SECURITY_MAX_SID_SIZE, &length))
                {
                    PPH_STRING sid = PH_AUTO(PhSidToStringSid(UserSid));
                    PhSetListViewSubItem(context->ListViewHandle, OBJECT_GENERAL_INDEX_DESKTOPSID, 1, PhGetString(sid));
                }

                if (GetUserObjectInformation(hDesktop, UOI_HEAPSIZE, &vInfo, sizeof(vInfo), NULL))
                {
                    PPH_STRING size = PH_AUTO(PhFormatSize(vInfo * 1024, ULONG_MAX));
                    PhSetListViewSubItem(context->ListViewHandle, OBJECT_GENERAL_INDEX_DESKTOPHEAP, 1, PhGetString(size));
                }

                CloseDesktop(hDesktop);
            }
        }
        else if (PhEqualString2(context->HandleItem->TypeName, L"Type", TRUE))
        {
            PH_STRINGREF firstPart;
            PH_STRINGREF typeName;
            POBJECT_TYPES_INFORMATION objectTypes;
            POBJECT_TYPE_INFORMATION objectType;
            ULONG typeIndex;
            PPH_STRING accessString;
            PH_STRING_BUILDER stringBuilder;

            PhAddListViewGroup(context->ListViewHandle, OBJECT_GENERAL_CATEGORY_TYPE, L"Type information");
            PhAddListViewGroupItem(context->ListViewHandle,OBJECT_GENERAL_CATEGORY_TYPE, OBJECT_GENERAL_INDEX_TYPEINDEX, L"Index", NULL);
            PhAddListViewGroupItem(context->ListViewHandle, OBJECT_GENERAL_CATEGORY_TYPE, OBJECT_GENERAL_INDEX_TYPEOBJECTS, L"Objects", NULL);
            PhAddListViewGroupItem(context->ListViewHandle, OBJECT_GENERAL_CATEGORY_TYPE, OBJECT_GENERAL_INDEX_TYPEHANDLES, L"Handles", NULL);
            PhAddListViewGroupItem(context->ListViewHandle, OBJECT_GENERAL_CATEGORY_TYPE, OBJECT_GENERAL_INDEX_TYPEPEAKOBJECTS, L"Peak Objects", NULL);
            PhAddListViewGroupItem(context->ListViewHandle, OBJECT_GENERAL_CATEGORY_TYPE, OBJECT_GENERAL_INDEX_TYPEPEAKHANDLES, L"Peak Handles", NULL);
            PhAddListViewGroupItem(context->ListViewHandle, OBJECT_GENERAL_CATEGORY_TYPE, OBJECT_GENERAL_INDEX_TYPEPOOLTYPE, L"Pool Type", NULL);
            PhAddListViewGroupItem(context->ListViewHandle, OBJECT_GENERAL_CATEGORY_TYPE, OBJECT_GENERAL_INDEX_TYPEPAGECHARGE, L"Default Paged Charge", NULL);
            PhAddListViewGroupItem(context->ListViewHandle, OBJECT_GENERAL_CATEGORY_TYPE, OBJECT_GENERAL_INDEX_TYPENPAGECHARGE, L"Default NP Charge", NULL);

            if (!PhIsNullOrEmptyString(context->HandleItem->ObjectName) &&
                PhSplitStringRefAtLastChar(&context->HandleItem->ObjectName->sr, OBJ_NAME_PATH_SEPARATOR, &firstPart, &typeName))
            {
                typeIndex = PhGetObjectTypeNumber(&typeName);

                if (typeIndex != ULONG_MAX && NT_SUCCESS(PhEnumObjectTypes(&objectTypes)))
                {
                    objectType = PH_FIRST_OBJECT_TYPE(objectTypes);

                    for (ULONG i = 0; i < objectTypes->NumberOfTypes; i++)
                    {
                        if (objectType->TypeIndex == typeIndex)
                        {
                            PhPrintUInt32(string, objectType->TypeIndex);
                            PhSetListViewSubItem(context->ListViewHandle, OBJECT_GENERAL_INDEX_TYPEINDEX, 1, string);
                            PhPrintUInt32(string, objectType->TotalNumberOfObjects);
                            PhSetListViewSubItem(context->ListViewHandle, OBJECT_GENERAL_INDEX_TYPEOBJECTS, 1, string);
                            PhPrintUInt32(string, objectType->TotalNumberOfHandles);
                            PhSetListViewSubItem(context->ListViewHandle, OBJECT_GENERAL_INDEX_TYPEHANDLES, 1, string);
                            PhPrintUInt32(string, objectType->HighWaterNumberOfObjects);
                            PhSetListViewSubItem(context->ListViewHandle, OBJECT_GENERAL_INDEX_TYPEPEAKOBJECTS, 1, string);
                            PhPrintUInt32(string, objectType->HighWaterNumberOfHandles);
                            PhSetListViewSubItem(context->ListViewHandle, OBJECT_GENERAL_INDEX_TYPEPEAKHANDLES, 1, string);
                            PhPrintUInt32(string, objectType->DefaultPagedPoolCharge);

                            PWSTR poolTypeString = L"";
                            switch (objectType->PoolType)
                            {
                                case NonPagedPool: poolTypeString = L"Non Paged"; break;
                                case PagedPool: poolTypeString = L"Paged"; break;
                                case NonPagedPoolNx: poolTypeString = L"Non Paged NX"; break;
                                case PagedPoolSessionNx: poolTypeString = L"Paged Session NX"; break;
                            }

                            PhSetListViewSubItem(context->ListViewHandle, OBJECT_GENERAL_INDEX_TYPEPOOLTYPE, 1, poolTypeString);
                            PhSetListViewSubItem(context->ListViewHandle, OBJECT_GENERAL_INDEX_TYPEPAGECHARGE, 1, string);
                            PhPrintUInt32(string, objectType->DefaultNonPagedPoolCharge);
                            PhSetListViewSubItem(context->ListViewHandle, OBJECT_GENERAL_INDEX_TYPENPAGECHARGE, 1, string);

                            PhAddListViewGroup(context->ListViewHandle, OBJECT_GENERAL_CATEGORY_TYPE_ACCESS, L"Type access information");
                            PhAddListViewGroupItem(context->ListViewHandle, OBJECT_GENERAL_CATEGORY_TYPE_ACCESS, OBJECT_GENERAL_INDEX_TYPEVALIDMASK, L"Valid Access Mask", NULL);
                            PhAddListViewGroupItem(context->ListViewHandle, OBJECT_GENERAL_CATEGORY_TYPE_ACCESS, OBJECT_GENERAL_INDEX_TYPEGENERICREAD, L"Generic Read", NULL);
                            PhAddListViewGroupItem(context->ListViewHandle, OBJECT_GENERAL_CATEGORY_TYPE_ACCESS, OBJECT_GENERAL_INDEX_TYPEGENERICWRITE, L"Generic Write", NULL);
                            PhAddListViewGroupItem(context->ListViewHandle, OBJECT_GENERAL_CATEGORY_TYPE_ACCESS, OBJECT_GENERAL_INDEX_TYPEGENERICEXECUTE, L"Generic Execute", NULL);
                            PhAddListViewGroupItem(context->ListViewHandle, OBJECT_GENERAL_CATEGORY_TYPE_ACCESS, OBJECT_GENERAL_INDEX_TYPEGENERICALL, L"Generic All", NULL);
                            PhAddListViewGroupItem(context->ListViewHandle, OBJECT_GENERAL_CATEGORY_TYPE_ACCESS, OBJECT_GENERAL_INDEX_TYPEINVALIDATTRIBUTES, L"Invalid Attributes", NULL);

                            accessString = PH_AUTO(EtGetAccessString2(&typeName, objectType->ValidAccessMask));
                            PhSetListViewSubItem(context->ListViewHandle, OBJECT_GENERAL_INDEX_TYPEVALIDMASK, 1, PhGetString(accessString));
                            accessString = PH_AUTO(EtGetAccessString2(&typeName, objectType->GenericMapping.GenericRead));
                            PhSetListViewSubItem(context->ListViewHandle, OBJECT_GENERAL_INDEX_TYPEGENERICREAD, 1, PhGetString(accessString));
                            accessString = PH_AUTO(EtGetAccessString2(&typeName, objectType->GenericMapping.GenericWrite));
                            PhSetListViewSubItem(context->ListViewHandle, OBJECT_GENERAL_INDEX_TYPEGENERICWRITE, 1, PhGetString(accessString));
                            accessString = PH_AUTO(EtGetAccessString2(&typeName, objectType->GenericMapping.GenericExecute));
                            PhSetListViewSubItem(context->ListViewHandle, OBJECT_GENERAL_INDEX_TYPEGENERICEXECUTE, 1, PhGetString(accessString));
                            accessString = PH_AUTO(EtGetAccessString2(&typeName, objectType->GenericMapping.GenericAll));
                            PhSetListViewSubItem(context->ListViewHandle, OBJECT_GENERAL_INDEX_TYPEGENERICALL, 1, PhGetString(accessString));

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
                            PhSetListViewSubItem(context->ListViewHandle, OBJECT_GENERAL_INDEX_TYPEINVALIDATTRIBUTES, 1, PhGetString(accessString));

                            break;
                        }
                        objectType = PH_NEXT_OBJECT_TYPE(objectType);
                    }
                    PhFree(objectTypes);
                }
            }
        }
        else if (PhEqualString2(context->HandleItem->TypeName, L"Session", TRUE))
        {
            PH_STRINGREF firstPart;
            PH_STRINGREF sessionName;
            WINSTATIONINFORMATION winStationInfo;
            ULONG returnLength;
            ULONG sessionId;
            SYSTEMTIME systemTime;

            PhAddListViewGroup(context->ListViewHandle, OBJECT_GENERAL_CATEGORY_SESSION, L"Session information");
            PhAddListViewGroupItem(context->ListViewHandle, OBJECT_GENERAL_CATEGORY_SESSION, OBJECT_GENERAL_INDEX_SESSIONNAME, L"Session Name", NULL);
            PhAddListViewGroupItem(context->ListViewHandle, OBJECT_GENERAL_CATEGORY_SESSION, OBJECT_GENERAL_INDEX_SESSIONID, L"Session ID", NULL);
            PhAddListViewGroupItem(context->ListViewHandle, OBJECT_GENERAL_CATEGORY_SESSION, OBJECT_GENERAL_INDEX_SESSIONUSERNAME, L"User name", NULL);
            PhAddListViewGroupItem(context->ListViewHandle, OBJECT_GENERAL_CATEGORY_SESSION, OBJECT_GENERAL_INDEX_SESSIONSTATE, L"State", NULL);
            PhAddListViewGroupItem(context->ListViewHandle, OBJECT_GENERAL_CATEGORY_SESSION, OBJECT_GENERAL_INDEX_SESSIONLOGON, L"Logon time", NULL);
            PhAddListViewGroupItem(context->ListViewHandle, OBJECT_GENERAL_CATEGORY_SESSION, OBJECT_GENERAL_INDEX_SESSIONCONNECT, L"Connect time", NULL);
            PhAddListViewGroupItem(context->ListViewHandle, OBJECT_GENERAL_CATEGORY_SESSION, OBJECT_GENERAL_INDEX_SESSIONDISCONNECT, L"Disconnect time", NULL);
            PhAddListViewGroupItem(context->ListViewHandle, OBJECT_GENERAL_CATEGORY_SESSION, OBJECT_GENERAL_INDEX_SESSIONLASTINPUT, L"Last input time", NULL);

            if (!PhIsNullOrEmptyString(context->HandleItem->ObjectName) &&
                PhSplitStringRefAtLastChar(&context->HandleItem->ObjectName->sr, OBJ_NAME_PATH_SEPARATOR, &firstPart, &sessionName) &&
                (sessionId = EtSessionIdFromObjectName(&sessionName)) != ULONG_MAX)
            {
                if (WinStationQueryInformationW(
                    WINSTATION_CURRENT_SERVER,
                    sessionId,
                    WinStationInformation,
                    &winStationInfo,
                    sizeof(WINSTATIONINFORMATION),
                    &returnLength
                    ))
                {
                    PhSetListViewSubItem(context->ListViewHandle, OBJECT_GENERAL_INDEX_SESSIONNAME, 1, winStationInfo.WinStationName);
                    PhPrintUInt32(string, sessionId);
                    PhSetListViewSubItem(context->ListViewHandle, OBJECT_GENERAL_INDEX_SESSIONID, 1, string);

                    PhSetListViewSubItem(context->ListViewHandle, OBJECT_GENERAL_INDEX_SESSIONUSERNAME, 1, PhaFormatString(
                            L"%s%c%s",
                            winStationInfo.Domain,
                            winStationInfo.Domain[0] != UNICODE_NULL ? OBJ_NAME_PATH_SEPARATOR : UNICODE_NULL,
                            winStationInfo.UserName)->Buffer);

                    PhSetListViewSubItem(context->ListViewHandle, OBJECT_GENERAL_INDEX_SESSIONSTATE, 1, EtMapSessionConnectState(winStationInfo.ConnectState));
                    PhLargeIntegerToLocalSystemTime(&systemTime, &winStationInfo.LogonTime);
                    PhSetListViewSubItem(context->ListViewHandle, OBJECT_GENERAL_INDEX_SESSIONLOGON, 1, PhaFormatDateTime(&systemTime)->Buffer);
                    PhLargeIntegerToLocalSystemTime(&systemTime, &winStationInfo.ConnectTime);
                    PhSetListViewSubItem(context->ListViewHandle, OBJECT_GENERAL_INDEX_SESSIONCONNECT, 1, PhaFormatDateTime(&systemTime)->Buffer);
                    PhLargeIntegerToLocalSystemTime(&systemTime, &winStationInfo.DisconnectTime);
                    PhSetListViewSubItem(context->ListViewHandle, OBJECT_GENERAL_INDEX_SESSIONDISCONNECT, 1, PhaFormatDateTime(&systemTime)->Buffer);
                    PhLargeIntegerToLocalSystemTime(&systemTime, &winStationInfo.LastInputTime);
                    PhSetListViewSubItem(context->ListViewHandle, OBJECT_GENERAL_INDEX_SESSIONLASTINPUT, 1, PhaFormatDateTime(&systemTime)->Buffer);
                }
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
    if (PhEqualStringRef2(StationName, T_WINSTA_INTERACTIVE, TRUE))
    {
        return PhCreateString(L"Interactive Window Station");
    }

    PH_FORMAT format[3];

    PhInitFormatC(&format[0], UNICODE_NULL);
    PhInitFormatC(&format[1], L' ');
    PhInitFormatS(&format[2], L"logon session");

    if (PhFindStringInStringRefZ(StationName, T_WINSTA_SYSTEM, TRUE) != SIZE_MAX)
        PhInitFormatS(&format[0], L"System");
    if (PhFindStringInStringRefZ(StationName, T_WINSTA_ANONYMOUS, TRUE) != SIZE_MAX)
        PhInitFormatS(&format[0], L"Anonymous");
    if (PhFindStringInStringRefZ(StationName, T_WINSTA_LOCALSERVICE, TRUE) != SIZE_MAX)
        PhInitFormatS(&format[0], L"Local Service");
    if (PhFindStringInStringRefZ(StationName, T_WINSTA_NETWORK_SERVICE, TRUE) != SIZE_MAX)
        PhInitFormatS(&format[0], L"Network Service");

    return format[0].u.Char != UNICODE_NULL ? PhFormat(format, RTL_NUMBER_OF(format), 0) : NULL;
}

ULONG EtSessionIdFromObjectName(
    _In_ PPH_STRINGREF Name
    )
{
    static PH_STRINGREF session = PH_STRINGREF_INIT(L"Session");
    ULONG sessionId = ULONG_MAX;
    PH_STRINGREF firstPart;
    PH_STRINGREF idString;

    if (PhSplitStringRefAtString(Name, &session, TRUE, &firstPart, &idString) && idString.Length > 0)
    {
        LONG64 id;
        if (PhStringToInteger64(&idString, 0, &id))
            sessionId = (ULONG)id;
    }

    return sessionId;
}

PCWSTR EtMapSessionConnectState(
    _In_ WINSTATIONSTATECLASS State
    )
{
    static CONST PH_KEY_VALUE_PAIR EtpConnectStatePairs[] =
    {
        SIP(L"Active", State_Active),
        SIP(L"Connected", State_Connected),
        SIP(L"ConnectQuery", State_ConnectQuery),
        SIP(L"Shadow", State_Shadow),
        SIP(L"Disconnected", State_Disconnected),
        SIP(L"Idle", State_Idle),
        SIP(L"Listen", State_Listen),
        SIP(L"Reset", State_Reset),
        SIP(L"Down", State_Down),
        SIP(L"Init", State_Init)
    };

    PCWSTR stateString = NULL;

    PhIndexStringSiKeyValuePairs(
        EtpConnectStatePairs,
        sizeof(EtpConnectStatePairs),
        State,
        &stateString
        );

    return stateString;
}

VOID EtHandlePropertiesWindowUninitializing(
    _In_ PVOID Parameter
    )
{
    PPH_PLUGIN_HANDLE_PROPERTIES_WINDOW_CONTEXT context = Parameter;

    if (context->HandleItem->Handle && context->ProcessId == NtCurrentProcessId())
    {
        PhRemoveItemList(EtObjectManagerOwnHandles, PhFindItemList(EtObjectManagerOwnHandles, context->HandleItem->Handle));
        PhDereferenceObject(EtObjectManagerOwnHandles);

        NtClose(context->HandleItem->Handle);
    }

    //PhRemoveItemSimpleHashtable(EtObjectManagerPropWindows, context->ParentWindow);
    //PhDereferenceObject(EtObjectManagerPropWindows);

    PhSaveWindowPlacementToSetting(SETTING_NAME_OBJMGR_PROPERTIES_WINDOW_POSITION, NULL, context->ParentWindow);

    PhDereferenceObject(context->HandleItem);
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

            if (NT_SUCCESS(EtDuplicateHandleFromProcessEx(
                &workerFactoryHandle,
                WORKER_FACTORY_QUERY_INFORMATION,
                context->ProcessId,
                NULL,
                context->HandleItem->Handle
                )))
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
                        PhLoadSymbolProviderModules(symbolProvider, context->ProcessId);

                        symbol = PhGetSymbolFromAddress(
                            symbolProvider,
                            basicInfo.StartRoutine,
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
    _In_ ACCESS_MASK Access
    )
{
    return EtGetAccessStringZ(PhGetStringOrEmpty(TypeName), Access);
}

PPH_STRING EtGetAccessString2(
    _In_ PPH_STRINGREF TypeName,
    _In_ ACCESS_MASK Access
    )
{
    return EtGetAccessStringZ(PhGetStringRefZ(TypeName), Access);
}

PPH_STRING EtGetAccessStringZ(
    _In_ PCWSTR TypeName,
    _In_ ACCESS_MASK Access
    )
{
    PPH_STRING accessString = NULL;
    PPH_ACCESS_ENTRY accessEntries;
    ULONG numberOfAccessEntries;

    if (PhGetAccessEntries(
        TypeName,
        &accessEntries,
        &numberOfAccessEntries
        ))
    {
        accessString = PH_AUTO(PhGetAccessString(
            Access,
            accessEntries,
            numberOfAccessEntries
            ));

        if (accessString->Length != 0)
        {
            accessString = PhFormatString(
                L"0x%x (%s)",
                Access,
                accessString->Buffer
                );
        }
        else
        {
            accessString = PhFormatString(L"0x%x", Access);
        }

        PhFree(accessEntries);
    }
    else
    {
        accessString = PhFormatString(L"0x%x", Access);
    }

    return accessString;
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
    PPH_STRING objectName;

    if (NT_SUCCESS(PhGetHandleInformation(
        handleContext->ProcessHandle,
        handleContext->HandleInfo->HandleValue,
        handleContext->HandleInfo->ObjectTypeIndex,
        NULL,
        NULL,
        &objectName,
        NULL
        )))
    {
        if (PhStartsWithString(objectName, handleContext->SearchObjectName, TRUE))
        {
            PhAcquireQueuedLockExclusive(handleContext->SearchResultsLock);
            PhAddItemList(handleContext->SearchResults, handleContext->HandleInfo);
            PhReleaseQueuedLockExclusive(handleContext->SearchResultsLock);
        }
        PhDereferenceObject(objectName);
    }

    PhFree(handleContext);

    return STATUS_SUCCESS;
}

static NTSTATUS NTAPI EtpUpdateHandleFunction(
    _In_ PVOID Parameter
    )
{
    PET_HANDLE_ENTRY entry = Parameter;

    EtUpdateHandleItem(entry->ProcessId, entry->HandleItem);

    return STATUS_SUCCESS;
}

VOID EtpEnumObjectHandles(
    _In_ PCOMMON_PAGE_CONTEXT Context
    )
{
    PhSetCursor(PhLoadCursor(NULL, IDC_WAIT));

    COLORREF colorOwnObject = PhGetIntegerSetting(L"ColorOwnProcesses");
    COLORREF colorInherit = PhGetIntegerSetting(L"ColorInheritHandles");
    COLORREF colorProtected = PhGetIntegerSetting(L"ColorProtectedHandles");
    COLORREF colorProtectedInherit = PhGetIntegerSetting(L"ColorPartiallySuspended");

    WCHAR string[PH_INT64_STR_LEN_1];
    PSYSTEM_HANDLE_INFORMATION_EX handles;
    ULONG_PTR i;
    ULONG searchTypeIndex;
    ULONG findBySameTypeIndex = ULONG_MAX;

    BOOLEAN isDevice = Context->HandleItem->TypeIndex == EtDeviceTypeIndex || Context->HandleItem->TypeIndex == EtFileTypeIndex;
    BOOLEAN isAlpcPort = Context->HandleItem->TypeIndex == EtAlpcPortTypeIndex;
    BOOLEAN isRegKey = Context->HandleItem->TypeIndex == EtKeyTypeIndex;
    BOOLEAN isTypeObject = PhEqualString2(Context->HandleItem->TypeName, L"Type", TRUE);

    searchTypeIndex = Context->HandleItem->TypeIndex;

    if (isTypeObject)
    {
        PH_STRINGREF firstPart;
        PH_STRINGREF typeName;
        ULONG typeIndex;

        if (PhSplitStringRefAtLastChar(&Context->HandleItem->ObjectName->sr, OBJ_NAME_PATH_SEPARATOR, &firstPart, &typeName))
            if ((typeIndex = PhGetObjectTypeNumber(&typeName)) != ULONG_MAX)
                findBySameTypeIndex = typeIndex;
    }

    if (NT_SUCCESS(PhEnumHandlesEx(&handles)))
    {
        PPH_LIST searchResults = PhCreateList(128);
        PH_WORK_QUEUE workQueue;
        PH_QUEUED_LOCK searchResultsLock = { 0 };

        PPH_HASHTABLE processHandleHashtable = PhCreateSimpleHashtable(8);
        PVOID* processHandlePtr;
        HANDLE processHandle;
        PPH_KEY_VALUE_PAIR procEntry;
        ULONG j = 0;

        PPH_STRING objectName;
        BOOLEAN objectNameMatched;
        BOOLEAN useWorkQueue = KsiLevel() < KphLevelMed && (isDevice || isTypeObject && (Context->HandleItem->TypeIndex == EtFileTypeIndex));

        if (useWorkQueue)
            PhInitializeWorkQueue(&workQueue, 1, 20, 1000);

        for (i = 0; i < handles->NumberOfHandles; i++)
        {
            PSYSTEM_HANDLE_TABLE_ENTRY_INFO_EX handleInfo = &handles->Handles[i];

            // Skip other types
            if (handleInfo->ObjectTypeIndex == searchTypeIndex ||
                (isDevice && handleInfo->ObjectTypeIndex == EtFileTypeIndex) ||
                (isTypeObject && handleInfo->ObjectTypeIndex == findBySameTypeIndex))
            {
                // Skip object name checking if we enumerate handles by type
                if (!(objectNameMatched = isTypeObject))
                {
                    // Lookup for matches in object name to find more handles for ALPC Port, Device/File, Key
                    if (isAlpcPort || isDevice || isRegKey)
                    {
                        // Open a handle to the process if we don't already have one.
                        processHandlePtr = PhFindItemSimpleHashtable(
                            processHandleHashtable,
                            handleInfo->UniqueProcessId
                            );

                        if (processHandlePtr)
                        {
                            processHandle = *processHandlePtr;
                        }
                        else
                        {
                            if (NT_SUCCESS(PhOpenProcess(
                                &processHandle,
                                (KsiLevel() >= KphLevelMed ? PROCESS_QUERY_LIMITED_INFORMATION : PROCESS_DUP_HANDLE),
                                handleInfo->UniqueProcessId
                                )))
                            {
                                PhAddItemSimpleHashtable(
                                    processHandleHashtable,
                                    handleInfo->UniqueProcessId,
                                    processHandle
                                    );
                            }
                            else
                            {
                                continue;
                            }
                        }

                        if (isAlpcPort)
                        {
                            if (NT_SUCCESS(PhGetHandleInformation(
                                processHandle,
                                handleInfo->HandleValue,
                                handleInfo->ObjectTypeIndex,
                                NULL,
                                NULL,
                                NULL,
                                &objectName
                                )))
                            {
                                objectNameMatched = PhEndsWithString(objectName, Context->HandleItem->ObjectName, TRUE);   // HACK
                                PhDereferenceObject(objectName);
                            }
                        }
                        else
                        {
                            // If we're dealing with a file handle we must take special precautions so we don't hang.
                            if (useWorkQueue)
                            {
                                PSEARCH_HANDLE_CONTEXT searchContext = PhAllocate(sizeof(SEARCH_HANDLE_CONTEXT));
                                searchContext->SearchObjectName = Context->HandleItem->ObjectName;
                                searchContext->ProcessHandle = processHandle;
                                searchContext->HandleInfo = handleInfo;
                                searchContext->SearchResults = searchResults;
                                searchContext->SearchResultsLock = &searchResultsLock;

                                PhQueueItemWorkQueue(&workQueue, EtpSearchHandleFunction, searchContext);
                                continue;
                            }

                            if (NT_SUCCESS(PhGetHandleInformation(
                                processHandle,
                                handleInfo->HandleValue,
                                handleInfo->ObjectTypeIndex,
                                NULL,
                                NULL,
                                &objectName,
                                NULL
                                )))
                            {
                                objectNameMatched = PhStartsWithString(objectName, Context->HandleItem->ObjectName, TRUE);
                                PhDereferenceObject(objectName);
                            }
                        }
                    }
                }

                if ((handleInfo->Object && handleInfo->Object == Context->HandleItem->Object) || objectNameMatched)
                {
                    if (useWorkQueue) PhAcquireQueuedLockExclusive(&searchResultsLock);
                    PhAddItemList(searchResults, handleInfo);
                    if (useWorkQueue) PhReleaseQueuedLockExclusive(&searchResultsLock);
                }
            }
        }

        if (useWorkQueue)
        {
            PhWaitForWorkQueue(&workQueue);
        }

        while (PhEnumHashtable(processHandleHashtable, &procEntry, &j))
            NtClose(procEntry->Value);

        PhDereferenceObject(processHandleHashtable);

        ULONG ownHandlesIndex = 0;
        INT lvItemIndex;
        WCHAR value[PH_INT64_STR_LEN_1];
        PET_HANDLE_ENTRY entry;
        PPH_STRING columnString;
        CLIENT_ID ClientId = { 0 };

        ExtendedListView_SetRedraw(Context->ListViewHandle, FALSE);

        for (i = 0; i < searchResults->Count; i++)
        {
            PSYSTEM_HANDLE_TABLE_ENTRY_INFO_EX handleInfo = searchResults->Items[i];

            // Skip Object Manager own handles
            if (handleInfo->UniqueProcessId == NtCurrentProcessId() &&
                PhFindItemList(EtObjectManagerOwnHandles, handleInfo->HandleValue) != ULONG_MAX)
            {
                continue;
            }

            entry = PhAllocateZero(sizeof(ET_HANDLE_ENTRY));
            entry->ProcessId = handleInfo->UniqueProcessId;
            entry->HandleItem = PhCreateHandleItem(handleInfo);
            entry->OwnHandle = handleInfo->Object == Context->HandleItem->Object;

            ClientId.UniqueProcess = entry->ProcessId;
            columnString = PH_AUTO(PhGetClientIdName(&ClientId));
            lvItemIndex = PhAddListViewItem(
                Context->ListViewHandle,
                entry->OwnHandle ? ownHandlesIndex++ : MAXINT,     // object own handles first
                PhGetString(columnString),
                entry
                );

            PhPrintPointer(value, (PVOID)handleInfo->HandleValue);
            PhSetListViewSubItem(Context->ListViewHandle, lvItemIndex, ETHNLVC_HANDLE, value);

            columnString = PH_AUTO(EtGetAccessString(Context->HandleItem->TypeName, handleInfo->GrantedAccess));
            PhSetListViewSubItem(Context->ListViewHandle, lvItemIndex, ETHNLVC_ACCESS, PhGetString(columnString));

            switch (handleInfo->HandleAttributes & (OBJ_PROTECT_CLOSE | OBJ_INHERIT))
            {
                case OBJ_PROTECT_CLOSE:
                    PhSetListViewSubItem(Context->ListViewHandle, lvItemIndex, ETHNLVC_ATTRIBUTES, L"Protected");
                    entry->Color = colorProtected, entry->UseCustomColor = TRUE;
                    break;
                case OBJ_INHERIT:
                    PhSetListViewSubItem(Context->ListViewHandle, lvItemIndex, ETHNLVC_ATTRIBUTES, L"Inherit");
                    entry->Color = colorInherit, entry->UseCustomColor = TRUE;
                    break;
                case OBJ_PROTECT_CLOSE | OBJ_INHERIT:
                    PhSetListViewSubItem(Context->ListViewHandle, lvItemIndex, ETHNLVC_ATTRIBUTES, L"Protected, Inherit");
                    entry->Color = colorProtectedInherit, entry->UseCustomColor = TRUE;
                    break;
            }

            // Highlight own object handles
            if (entry->OwnHandle)
                entry->Color = colorOwnObject, entry->UseCustomColor = TRUE;

            if (isTypeObject ||
                entry->HandleItem->TypeIndex == EtFileTypeIndex ||
                entry->HandleItem->TypeIndex == EtKeyTypeIndex ||
                entry->HandleItem->TypeIndex == EtAlpcPortTypeIndex)
            {
                if (!Context->ColumnsAdded)
                {
                    Context->ColumnsAdded = !!PhAddListViewColumn(Context->ListViewHandle, ETHNLVC_NAME, ETHNLVC_NAME, ETHNLVC_NAME, LVCFMT_LEFT, 200, L"Name");
                    PhAddListViewColumn(Context->ListViewHandle, ETHNLVC_ORIGNAME, ETHNLVC_ORIGNAME, ETHNLVC_ORIGNAME, LVCFMT_LEFT, 200, L"Original name");
                }

                if (useWorkQueue && entry->HandleItem->TypeIndex == EtFileTypeIndex)
                {
                    PhQueueItemWorkQueue(&workQueue, EtpUpdateHandleFunction, entry);
                }
                else
                {
                    EtUpdateHandleItem(entry->ProcessId, entry->HandleItem);
                }
            }
            else if (entry->HandleItem->TypeIndex == EtSectionTypeIndex)
            {
                EtUpdateHandleItem(entry->ProcessId, entry->HandleItem);
            }
        }

        PhDereferenceObject(searchResults);
        PhFree(handles);

        if (useWorkQueue)
        {
            PhWaitForWorkQueue(&workQueue);
            PhDeleteWorkQueue(&workQueue);
        }

        Context->TotalHandlesCount = ListView_GetItemCount(Context->ListViewHandle);
        Context->OwnHandlesCount = ownHandlesIndex;
    }

    if (isTypeObject)
        PhSetDialogItemText(Context->WindowHandle, IDC_OBJ_HANDLESBYNAME_L, L"By type:");

    PhPrintUInt32(string, Context->TotalHandlesCount);
    PhSetDialogItemText(Context->WindowHandle, IDC_OBJ_HANDLESTOTAL, string);
    PhPrintUInt32(string, Context->OwnHandlesCount);
    PhSetDialogItemText(Context->WindowHandle, IDC_OBJ_HANDLESBYOBJECT, string);
    PhPrintUInt32(string, Context->TotalHandlesCount - Context->OwnHandlesCount);
    PhSetDialogItemText(Context->WindowHandle, IDC_OBJ_HANDLESBYNAME, string);

    ExtendedListView_SetRedraw(Context->ListViewHandle, TRUE);
    PhSetCursor(PhLoadCursor(NULL, IDC_ARROW));
}

VOID EtUpdateHandleItem(
    _In_ HANDLE ProcessId,
    _Inout_ PPH_HANDLE_ITEM HandleItem
    )
{
    HANDLE processHandle;

    if (NT_SUCCESS(PhOpenProcess(
        &processHandle,
        (KsiLevel() >= KphLevelMed ? PROCESS_QUERY_LIMITED_INFORMATION : PROCESS_DUP_HANDLE),
        ProcessId
        )))
    {
        PhGetHandleInformation(
            processHandle,
            HandleItem->Handle,
            HandleItem->TypeIndex,
            NULL,
            &HandleItem->TypeName,
            &HandleItem->ObjectName,
            &HandleItem->BestObjectName
            );

        NtClose(processHandle);
    }

    if (PhIsNullOrEmptyString(HandleItem->TypeName))
        HandleItem->TypeName = PhGetObjectTypeIndexName(HandleItem->TypeIndex);
}

VOID EtpShowHandleProperties(
    _In_ HWND WindowHandle,
    _In_ PET_HANDLE_ENTRY Entry
    )
{
    EtUpdateHandleItem(Entry->ProcessId, Entry->HandleItem);

    PhShowHandlePropertiesEx(WindowHandle, Entry->ProcessId, Entry->HandleItem, NULL, NULL);
}

VOID EtpUpdateGeneralTab(
    _In_ PCOMMON_PAGE_CONTEXT Context
    )
{
    WCHAR string[PH_INT64_STR_LEN_1];
    HANDLE processHandle = NtCurrentProcess();
    HWND generalPageList;

    if (generalPageList = FindWindowEx(PropSheet_IndexToHwnd(GetParent(Context->WindowHandle), 0), NULL, WC_LISTVIEW, NULL))  // HACK
    {
        if (Context->ProcessId == NtCurrentProcessId() ||
            NT_SUCCESS(PhOpenProcess(
            &processHandle,
            (KsiLevel() >= KphLevelMed ? PROCESS_QUERY_LIMITED_INFORMATION : PROCESS_DUP_HANDLE),
            Context->ProcessId
            )))
        {
            OBJECT_BASIC_INFORMATION basicInfo;

            if (NT_SUCCESS(PhGetHandleInformation(
                processHandle,
                Context->HandleItem->Handle,
                Context->HandleItem->TypeIndex,
                &basicInfo,
                NULL,
                NULL,
                NULL
                )))
            {
                PhPrintUInt32(string, basicInfo.PointerCount);
                PhSetListViewSubItem(generalPageList, PH_PLUGIN_HANDLE_GENERAL_INDEX_REFERENCES, 1, string);

                PhPrintUInt32(string, Context->ProcessId == NtCurrentProcessId() ? OBJECT_CORRECT_HANDLES_COUNT(basicInfo.HandleCount) : basicInfo.HandleCount);
                PhSetListViewSubItem(generalPageList, PH_PLUGIN_HANDLE_GENERAL_INDEX_HANDLES, 1, string);
            }

            if (processHandle != NtCurrentProcess())
                NtClose(processHandle);
        }
    }
}

VOID EtpCloseObjectHandles(
    _In_ PCOMMON_PAGE_CONTEXT Context,
    _In_ PET_HANDLE_ENTRY* ListviewItems,
    _In_ ULONG NumberOfItems
    )
{
    WCHAR string[PH_INT64_STR_LEN_1];
    BOOLEAN ownHandle;

    for (ULONG i = 0; i < NumberOfItems; i++)
    {
        if (PhUiCloseHandles(Context->WindowHandle, ListviewItems[i]->ProcessId, &ListviewItems[i]->HandleItem, 1, TRUE))
        {
            ownHandle = !!ListviewItems[i]->OwnHandle;
            PhRemoveListViewItem(Context->ListViewHandle, PhFindListViewItemByParam(Context->ListViewHandle, INT_ERROR, ListviewItems[i]));
            PhClearReference(&ListviewItems[i]->HandleItem);
            PhFree(ListviewItems[i]);

            PhPrintUInt32(string, --Context->TotalHandlesCount);
            PhSetDialogItemText(Context->WindowHandle, IDC_OBJ_HANDLESTOTAL, string);

            Context->OwnHandlesCount -= ownHandle;
            PhPrintUInt32(string, ownHandle ? Context->OwnHandlesCount : Context->TotalHandlesCount - Context->OwnHandlesCount);
            PhSetDialogItemText(Context->WindowHandle, ownHandle ? IDC_OBJ_HANDLESBYOBJECT : IDC_OBJ_HANDLESBYNAME, string);
        }
    }

    // Update General page references and handles count
    EtpUpdateGeneralTab(Context);
}

static COLORREF NTAPI EtpColorItemColorFunction(
    _In_ INT Index,
    _In_ PVOID Param,
    _In_opt_ PVOID Context
    )
{
    PET_HANDLE_ENTRY entry = Param;
    COLORREF color = entry->UseCustomColor ?
        entry->Color :
        !!PhGetIntegerSetting(L"EnableThemeSupport") ? PhGetIntegerSetting(L"ThemeWindowBackgroundColor") : GetSysColor(COLOR_WINDOW);

    return color;
}

typedef struct _ET_HANDLE_OPEN_CONTEXT
{
    HANDLE ProcessId;
    PPH_HANDLE_ITEM HandleItem;
} ET_HANDLE_OPEN_CONTEXT, * PET_HANDLE_OPEN_CONTEXT;

static NTSTATUS EtpProcessHandleOpenCallback(
    _Out_ PHANDLE Handle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ PVOID Context
)
{
    PET_HANDLE_OPEN_CONTEXT context = Context;

    return EtDuplicateHandleFromProcessEx(Handle, DesiredAccess, context->ProcessId, NULL, context->HandleItem->Handle);
}

static NTSTATUS EtpProcessHandleCloseCallback(
    _In_ HANDLE Handle,
    _In_ BOOLEAN Release,
    _In_opt_ PVOID Context
    )
{
    PET_HANDLE_OPEN_CONTEXT context = Context;

    if (Handle)
    {
        NtClose(Handle);
    }

    if (Release && context)
    {
        PhDereferenceObject(context->HandleItem);
        PhFree(context);
    }

    return STATUS_SUCCESS;
}

VOID EtpHandlesFreeListViewItems(
    PCOMMON_PAGE_CONTEXT Context
    )
{
    INT index = INT_ERROR;

    while ((index = PhFindListViewItemByFlags(
        Context->ListViewHandle,
        index,
        LVNI_ALL
        )) != INT_ERROR)
    {
        PET_HANDLE_ENTRY entry;
        if (PhGetListViewItemParam(Context->ListViewHandle, index, &entry))
        {
            PhClearReference(&entry->HandleItem);
            PhFree(entry);
        }
    }
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
            PhAddListViewColumn(context->ListViewHandle, ETHNLVC_PROCESS, ETHNLVC_PROCESS, ETHNLVC_PROCESS, LVCFMT_LEFT, 120, L"Process");
            PhAddListViewColumn(context->ListViewHandle, ETHNLVC_HANDLE, ETHNLVC_HANDLE, ETHNLVC_HANDLE, LVCFMT_LEFT, 50, L"Handle");
            PhAddListViewColumn(context->ListViewHandle, ETHNLVC_ACCESS, ETHNLVC_ACCESS, ETHNLVC_ACCESS, LVCFMT_LEFT, 125, L"Access");
            PhAddListViewColumn(context->ListViewHandle, ETHNLVC_ATTRIBUTES, ETHNLVC_ATTRIBUTES, ETHNLVC_ATTRIBUTES, LVCFMT_LEFT, 70, L"Attributes");
            PhSetExtendedListView(context->ListViewHandle);
            ExtendedListView_SetSort(context->ListViewHandle, 0, NoSortOrder);
            ExtendedListView_SetItemColorFunction(context->ListViewHandle, EtpColorItemColorFunction);

            PhInitializeLayoutManager(&context->LayoutManager, hwndDlg);
            PhAddLayoutItem(&context->LayoutManager, context->ListViewHandle, NULL, PH_ANCHOR_ALL);

            EtpEnumObjectHandles(context);

            PhInitializeWindowTheme(hwndDlg, !!PhGetIntegerSetting(L"EnableThemeSupport"));
        }
        break;
    case WM_SIZE:
        {
            PhLayoutManagerLayout(&context->LayoutManager);
            ExtendedListView_SetColumnWidth(context->ListViewHandle, 0, ELVSCW_AUTOSIZE_REMAININGSPACE);
        }
        break;
    case WM_DESTROY:
        {
            EtpHandlesFreeListViewItems(context);
        }
        break;
    case WM_NCDESTROY:
        {
            PhDeleteLayoutManager(&context->LayoutManager);
            PhRemoveWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT);
        }
        break;
    case WM_KEYDOWN:
        {
            PET_HANDLE_ENTRY* listviewItems;
            ULONG numberOfItems;

            switch (LOWORD(wParam))
            {
            case VK_RETURN:
                {
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
                                    PhFree(listviewItems);
                                    return TRUE;
                                }
                            }
                            else
                            {
                                EtpShowHandleProperties(hwndDlg, listviewItems[0]);
                                PhFree(listviewItems);
                                return TRUE;
                            }
                        }

                        PhFree(listviewItems);
                    }
                }
                break;
            case VK_DELETE:
                {
                    if (GetFocus() == context->ListViewHandle)
                    {
                        PhGetSelectedListViewItemParams(context->ListViewHandle, &listviewItems, &numberOfItems);

                        if (numberOfItems != 0)
                        {
                            EtpCloseObjectHandles(context, listviewItems, numberOfItems);
                            return TRUE;
                        }

                        PhFree(listviewItems);
                    }
                }
                break;
            case VK_F5:
                SendMessage(hwndDlg, WM_COMMAND, IDC_REFRESH, 0);
                return TRUE;
            }
        }
        break;
    case WM_NOTIFY:
        {
            PhHandleListViewNotifyBehaviors(lParam, context->ListViewHandle, PH_LIST_VIEW_DEFAULT_1_BEHAVIORS);

            REFLECT_MESSAGE_DLG(hwndDlg, context->ListViewHandle, uMsg, wParam, lParam);

            LPNMHDR header = (LPNMHDR)lParam;

            if (header->code == NM_DBLCLK &&
                header->hwndFrom == context->ListViewHandle)
            {
                LPNMITEMACTIVATE info = (LPNMITEMACTIVATE)header;
                PET_HANDLE_ENTRY entry;

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
            else if (header->code == LVN_GETDISPINFOA &&    // I don't know why prop sheet sends ANSI message while window is Unicode
                header->hwndFrom == context->ListViewHandle)
            {
                NMLVDISPINFOA* dispInfo = (NMLVDISPINFOA*)header;

                if (FlagOn(dispInfo->item.mask, TVIF_TEXT))
                {
                    PET_HANDLE_ENTRY entry = (PET_HANDLE_ENTRY)dispInfo->item.lParam;

                    switch (dispInfo->item.iSubItem)
                    {
                    case ETHNLVC_NAME:
                        if (entry->HandleItem->BestObjectName)
                        {
                            dispInfo->item.mask |= LVIF_DI_SETITEM;
                            dispInfo->item.pszText = PH_AUTO_T(PH_BYTES, PhConvertUtf16ToMultiByte(PhGetString(entry->HandleItem->BestObjectName)))->Buffer;
                        }
                        break;
                    case ETHNLVC_ORIGNAME:
                        if (entry->HandleItem->ObjectName)
                        {
                            dispInfo->item.mask |= LVIF_DI_SETITEM;
                            dispInfo->item.pszText = PH_AUTO_T(PH_BYTES, PhConvertUtf16ToMultiByte(PhGetString(entry->HandleItem->ObjectName)))->Buffer;
                        }
                        break;
                    }
                }
            }
        }
        break;
    case WM_CONTEXTMENU:
        {
            PET_HANDLE_ENTRY* listviewItems;
            ULONG numberOfItems;

            PhGetSelectedListViewItemParams(context->ListViewHandle, &listviewItems, &numberOfItems);

            if (numberOfItems != 0)
            {
                POINT point;
                PPH_EMENU menu;
                PPH_EMENU_ITEM item;
                PPH_EMENU_ITEM propMenuItem;
                PPH_EMENU_ITEM protectedMenuItem;
                PPH_EMENU_ITEM inheritMenuItem;
                PPH_EMENU_ITEM gotoMenuItem;
                PPH_EMENU_ITEM secMenuItem;
                ULONG attributes = 0;
                PH_HANDLE_ITEM_INFO info = { 0 };

                point.x = GET_X_LPARAM(lParam);
                point.y = GET_Y_LPARAM(lParam);

                if (point.x == -1 && point.y == -1)
                    PhGetListViewContextMenuPoint(context->ListViewHandle, &point);

                menu = PhCreateEMenu();
                PhInsertEMenuItem(menu, PhCreateEMenuItem(0, IDC_CLOSEHANDLE, L"C&lose\bDel", NULL, NULL), ULONG_MAX);
                PhInsertEMenuItem(menu, protectedMenuItem = PhCreateEMenuItem(0, IDC_HANDLE_PROTECTED, L"&Protected", NULL, NULL), ULONG_MAX);
                PhInsertEMenuItem(menu, inheritMenuItem = PhCreateEMenuItem(0, IDC_HANDLE_INHERIT, L"&Inherit", NULL, NULL), ULONG_MAX);
                PhInsertEMenuItem(menu, PhCreateEMenuSeparator(), ULONG_MAX);
                PhInsertEMenuItem(menu, gotoMenuItem = PhCreateEMenuItem(0, IDC_GOTOPROCESS, L"&Go to process\bCtrl+Enter", NULL, NULL), ULONG_MAX);
                PhInsertEMenuItem(menu, PhCreateEMenuSeparator(), ULONG_MAX);
                PhInsertEMenuItem(menu, secMenuItem = PhCreateEMenuItem(0, IDC_SECURITY, L"&Security", NULL, NULL), ULONG_MAX);
                PhInsertEMenuItem(menu, PhCreateEMenuSeparator(), ULONG_MAX);
                PhInsertEMenuItem(menu, propMenuItem = PhCreateEMenuItem(0, IDC_PROPERTIES, L"Prope&rties\bEnter", NULL, NULL), ULONG_MAX);
                PhInsertEMenuItem(menu, PhCreateEMenuSeparator(), ULONG_MAX);
                PhInsertEMenuItem(menu, PhCreateEMenuItem(0, IDC_COPY, L"&Copy\bCtrl+C", NULL, NULL), ULONG_MAX);
                PhInsertCopyListViewEMenuItem(menu, IDC_COPY, context->ListViewHandle);
                PhSetFlagsEMenuItem(menu, IDC_PROPERTIES, PH_EMENU_DEFAULT, PH_EMENU_DEFAULT);

                if (numberOfItems > 1)
                {
                    PhSetDisabledEMenuItem(protectedMenuItem);
                    PhSetDisabledEMenuItem(inheritMenuItem);
                    PhSetDisabledEMenuItem(propMenuItem);
                    PhSetDisabledEMenuItem(gotoMenuItem);
                    PhSetDisabledEMenuItem(secMenuItem);
                }
                else if (numberOfItems == 1)
                {
                    // Re-create the attributes.

                    if (listviewItems[0]->HandleItem->Attributes & OBJ_PROTECT_CLOSE)
                    {
                        attributes |= OBJ_PROTECT_CLOSE;
                        PhSetFlagsEMenuItem(menu, IDC_HANDLE_PROTECTED, PH_EMENU_CHECKED, PH_EMENU_CHECKED);
                    }
                    if (listviewItems[0]->HandleItem->Attributes & OBJ_INHERIT)
                    {
                        attributes |= OBJ_INHERIT;
                        PhSetFlagsEMenuItem(menu, IDC_HANDLE_INHERIT, PH_EMENU_CHECKED, PH_EMENU_CHECKED);
                    }

                    info.ProcessId = listviewItems[0]->ProcessId;
                    info.Handle = listviewItems[0]->HandleItem->Handle;
                    info.TypeName = listviewItems[0]->HandleItem->TypeName;
                    info.BestObjectName = listviewItems[0]->HandleItem->BestObjectName;
                    PhInsertHandleObjectPropertiesEMenuItems(menu, IDC_COPY, FALSE, &info);
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
                            if (numberOfItems != 0)
                            {
                                EtpCloseObjectHandles(context, listviewItems, numberOfItems);
                            }
                        }
                        break;
                    case IDC_HANDLE_PROTECTED:
                    case IDC_HANDLE_INHERIT:
                        {
                            // Toggle the appropriate bit.

                            if (item->Id == IDC_HANDLE_PROTECTED)
                                attributes ^= OBJ_PROTECT_CLOSE;
                            else if (item->Id == IDC_HANDLE_INHERIT)
                                attributes ^= OBJ_INHERIT;

                            if (PhUiSetAttributesHandle(hwndDlg, listviewItems[0]->ProcessId, listviewItems[0]->HandleItem, attributes))
                            {
                                if (item->Id == IDC_HANDLE_PROTECTED)
                                    listviewItems[0]->HandleItem->Attributes ^= OBJ_PROTECT_CLOSE;
                                else if (item->Id == IDC_HANDLE_INHERIT)
                                    listviewItems[0]->HandleItem->Attributes ^= OBJ_INHERIT;

                                // Update list row

                                LONG lvItemIndex = PhFindListViewItemByParam(context->ListViewHandle, INT_ERROR, listviewItems[0]);
                                listviewItems[0]->UseCustomColor = listviewItems[0]->OwnHandle;

                                switch (listviewItems[0]->HandleItem->Attributes & (OBJ_PROTECT_CLOSE | OBJ_INHERIT))
                                {
                                case OBJ_PROTECT_CLOSE:
                                    PhSetListViewSubItem(context->ListViewHandle, lvItemIndex, ETHNLVC_ATTRIBUTES, L"Protected");
                                    listviewItems[0]->Color = PhGetIntegerSetting(L"ColorProtectedHandles"), listviewItems[0]->UseCustomColor = TRUE;
                                    break;
                                case OBJ_INHERIT:
                                    PhSetListViewSubItem(context->ListViewHandle, lvItemIndex, ETHNLVC_ATTRIBUTES, L"Inherit");
                                    listviewItems[0]->Color = PhGetIntegerSetting(L"ColorInheritHandles"), listviewItems[0]->UseCustomColor = TRUE;
                                    break;
                                case OBJ_PROTECT_CLOSE | OBJ_INHERIT:
                                    PhSetListViewSubItem(context->ListViewHandle, lvItemIndex, ETHNLVC_ATTRIBUTES, L"Protected, Inherit");
                                    listviewItems[0]->Color = PhGetIntegerSetting(L"ColorPartiallySuspended"), listviewItems[0]->UseCustomColor = TRUE;
                                    break;
                                default:
                                    PhSetListViewSubItem(context->ListViewHandle, lvItemIndex, ETHNLVC_ATTRIBUTES, L"");
                                    break;
                                }

                                if (listviewItems[0]->OwnHandle)
                                    listviewItems[0]->Color = PhGetIntegerSetting(L"ColorOwnProcesses");

                                ListView_SetItemState(context->ListViewHandle, -1, 0, LVIS_SELECTED);
                            }
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
                    case PHAPP_ID_HANDLE_OBJECTPROPERTIES1:
                        {
                            PhShowHandleObjectProperties1(hwndDlg, &info);
                        }
                        break;
                    case PHAPP_ID_HANDLE_OBJECTPROPERTIES2:
                        {
                            PhShowHandleObjectProperties2(hwndDlg, &info);
                        }
                        break;
                    case IDC_SECURITY:
                        {
                            PET_HANDLE_OPEN_CONTEXT handleContext;

                            handleContext = PhAllocateZero(sizeof(ET_HANDLE_OPEN_CONTEXT));
                            handleContext->HandleItem = PhReferenceObject(listviewItems[0]->HandleItem);
                            handleContext->ProcessId = listviewItems[0]->ProcessId;
                            EtUpdateHandleItem(handleContext->ProcessId, handleContext->HandleItem);

                            PhEditSecurity(
                                !!PhGetIntegerSetting(L"ForceNoParent") ? NULL : hwndDlg,
                                PhGetString(handleContext->HandleItem->ObjectName),
                                PhGetString(handleContext->HandleItem->TypeName),
                                EtpProcessHandleOpenCallback,
                                EtpProcessHandleCloseCallback,
                                handleContext
                                );
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

            PhFree(listviewItems);
        }
        break;
    case WM_COMMAND:
        switch (GET_WM_COMMAND_ID(wParam, lParam))
        {
        case IDC_REFRESH:
            {
                ULONG sortColumn;
                PH_SORT_ORDER sortOrder;

                EtpHandlesFreeListViewItems(context);
                ListView_DeleteAllItems(context->ListViewHandle);
                PhSetDialogItemText(context->WindowHandle, IDC_OBJ_HANDLESTOTAL, L"");
                PhSetDialogItemText(context->WindowHandle, IDC_OBJ_HANDLESBYOBJECT, L"");
                PhSetDialogItemText(context->WindowHandle, IDC_OBJ_HANDLESBYNAME, L"");

                EtpEnumObjectHandles(context);

                ExtendedListView_GetSort(context->ListViewHandle, &sortColumn, &sortOrder);
                if (sortOrder != NoSortOrder)
                    ExtendedListView_SortItems(context->ListViewHandle);
                EtpUpdateGeneralTab(context);
            }
            break;
        }
        break;
    }

    return FALSE;
}

static BOOL CALLBACK EtpEnumDesktopsCallback(
    _In_ LPWSTR lpszDesktop,
    _In_ LPARAM lParam
    )
{
    PCOMMON_PAGE_CONTEXT context = (PCOMMON_PAGE_CONTEXT)lParam;
    HDESK hDesktop;
    UINT lvItemIndex;

    lvItemIndex = PhAddListViewItem(context->ListViewHandle, MAXINT, lpszDesktop, PhCreateString(lpszDesktop));

    if (hDesktop = OpenDesktop(lpszDesktop, 0, FALSE, DESKTOP_READOBJECTS))
    {
        ULONG length = SECURITY_MAX_SID_SIZE;
        UCHAR buffer[SECURITY_MAX_SID_SIZE];
        PSID UserSid = (PSID)buffer;
        ULONG vInfo = 0;

        if (GetUserObjectInformation(hDesktop, UOI_USER_SID, UserSid, SECURITY_MAX_SID_SIZE, &length))
        {
            PPH_STRING sid = PH_AUTO(PhSidToStringSid(UserSid));
            PhSetListViewSubItem(context->ListViewHandle, lvItemIndex, ETDTLVC_SID, PhGetString(sid));
            PhFree(UserSid);
        }

        if (GetUserObjectInformation(hDesktop, UOI_HEAPSIZE, &vInfo, sizeof(vInfo), NULL))
        {
            PPH_STRING size = PH_AUTO(PhFormatString(L"%d MB", vInfo / 1024));
            PhSetListViewSubItem(context->ListViewHandle, lvItemIndex, ETDTLVC_HEAP, PhGetString(size));
        }

        if (GetUserObjectInformation(hDesktop, UOI_IO, &vInfo, sizeof(vInfo), NULL))
        {
            PhSetListViewSubItem(context->ListViewHandle, lvItemIndex, ETDTLVC_IO, !!vInfo ? L"True" : L"False");
        }

        CloseDesktop(hDesktop);
    }

    return TRUE;
}

typedef struct _OPEN_DESKTOP_CONTEXT
{
    PPH_STRING DesktopName;
    HWINSTA DesktopWinStation;
    HWINSTA CurrentWinStation;
} OPEN_DESKTOP_CONTEXT, * POPEN_DESKTOP_CONTEXT;

static NTSTATUS EtpOpenSecurityDesktopHandle(
    _Out_ PHANDLE Handle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ PVOID Context
    )
{
    POPEN_DESKTOP_CONTEXT context = Context;
    NTSTATUS status = STATUS_UNSUCCESSFUL;
    HDESK desktopHandle;

    assert(context);

    if (context->DesktopWinStation)
        SetProcessWindowStation(context->DesktopWinStation);

    if (desktopHandle = OpenDesktop(
        PhGetString(context->DesktopName),
        0,
        FALSE,
        MAXIMUM_ALLOWED
        ))
    {
        *Handle = (HANDLE)desktopHandle;
        status = STATUS_SUCCESS;
    }

    if (context->DesktopWinStation)
        SetProcessWindowStation(context->CurrentWinStation);

    return status;
}

static NTSTATUS EtpCloseSecurityDesktop(
    _In_ HANDLE Handle,
    _In_ BOOLEAN Release,
    _In_opt_ PVOID Context
    )
{
    POPEN_DESKTOP_CONTEXT context = Context;

    if (Handle)
    {
        CloseDesktop(Handle);
    }

    if (Release && context)
    {
        PhClearReference(&context->DesktopName);
        if (context->DesktopWinStation) CloseWindowStation(context->DesktopWinStation);
        PhFree(context);
    }

    return STATUS_SUCCESS;
}

VOID EtpOpenDesktopSecurity(
    PCOMMON_PAGE_CONTEXT context,
    PPH_STRING deskName
    )
{
    POPEN_DESKTOP_CONTEXT OpenContext = PhAllocateZero(sizeof(OPEN_DESKTOP_CONTEXT));
    HWINSTA hWinStation;

    OpenContext->DesktopName = PhReferenceObject(deskName);
    OpenContext->CurrentWinStation = GetProcessWindowStation();
    if (NT_SUCCESS(EtDuplicateHandleFromProcessEx(
        (PHANDLE)&hWinStation,
        WINSTA_ENUMDESKTOPS,
        context->ProcessId,
        NULL,
        context->HandleItem->Handle
        )))
    {
        if (PhCompareObjects((HANDLE)OpenContext->CurrentWinStation, (HANDLE)hWinStation) == STATUS_NOT_SAME_OBJECT)
            OpenContext->DesktopWinStation = hWinStation;
        else
            CloseWindowStation(hWinStation);
    }

    PhEditSecurity(
        !!PhGetIntegerSetting(L"ForceNoParent") ? NULL : context->WindowHandle,
        PhGetString(deskName),
        L"Desktop",
        EtpOpenSecurityDesktopHandle,
        EtpCloseSecurityDesktop,
        OpenContext
        );
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
            HWINSTA hWinStation;
            context->WindowHandle = hwndDlg;
            context->ListViewHandle = GetDlgItem(hwndDlg, IDC_LIST);

            PhSetListViewStyle(context->ListViewHandle, TRUE, TRUE);
            PhSetControlTheme(context->ListViewHandle, L"explorer");
            PhAddListViewColumn(context->ListViewHandle, ETDTLVC_NAME, ETDTLVC_NAME, ETDTLVC_NAME, LVCFMT_LEFT, 152, L"Name");
            PhAddListViewColumn(context->ListViewHandle, ETDTLVC_SID, ETDTLVC_SID, ETDTLVC_SID, LVCFMT_LEFT, 105, L"SID");
            PhAddListViewColumn(context->ListViewHandle, ETDTLVC_HEAP, ETDTLVC_HEAP, ETDTLVC_HEAP, LVCFMT_LEFT, 62, L"Heap Size");
            PhAddListViewColumn(context->ListViewHandle, ETDTLVC_IO, ETDTLVC_IO, ETDTLVC_IO, LVCFMT_LEFT, 45, L"Input");
            PhSetExtendedListView(context->ListViewHandle);
            ExtendedListView_SetSort(context->ListViewHandle, 0, NoSortOrder);
            PhInitializeLayoutManager(&context->LayoutManager, hwndDlg);
            PhAddLayoutItem(&context->LayoutManager, context->ListViewHandle, NULL, PH_ANCHOR_ALL);

            ExtendedListView_SetRedraw(context->ListViewHandle, FALSE);

            if (NT_SUCCESS(EtDuplicateHandleFromProcessEx(
                (PHANDLE)&hWinStation,
                WINSTA_ENUMDESKTOPS,
                context->ProcessId,
                NULL,
                context->HandleItem->Handle
                )))
            {
                HWINSTA currentStation = GetProcessWindowStation();
                NTSTATUS status = PhCompareObjects((HANDLE)currentStation, (HANDLE)hWinStation);
                if (status == STATUS_NOT_SAME_OBJECT)
                    SetProcessWindowStation(hWinStation);
                EnumDesktops(hWinStation, EtpEnumDesktopsCallback, (LPARAM)context);
                if (status == STATUS_NOT_SAME_OBJECT)
                    SetProcessWindowStation(currentStation);
                CloseWindowStation(hWinStation);
            }

            ExtendedListView_SetRedraw(context->ListViewHandle, TRUE);

            PhInitializeWindowTheme(hwndDlg, !!PhGetIntegerSetting(L"EnableThemeSupport"));
        }
        break;
    case WM_SIZE:
        {
            PhLayoutManagerLayout(&context->LayoutManager);
            ExtendedListView_SetColumnWidth(context->ListViewHandle, 0, ELVSCW_AUTOSIZE_REMAININGSPACE);
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
                PPH_STRING deskName;
                if (PhGetListViewItemParam(context->ListViewHandle, index, &deskName))
                {
                    PhDereferenceObject(deskName);
                }
            }
        }
        break;
    case WM_NCDESTROY:
        {
            PhDeleteLayoutManager(&context->LayoutManager);
            PhRemoveWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT);
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
                PPH_STRING deskName;

                if (deskName = PhGetSelectedListViewItemParam(context->ListViewHandle))
                {
                    EtpOpenDesktopSecurity(context, deskName);
                }
            }
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
                PPH_EMENU_ITEM secMenuItem;

                point.x = GET_X_LPARAM(lParam);
                point.y = GET_Y_LPARAM(lParam);

                if (point.x == -1 && point.y == -1)
                    PhGetListViewContextMenuPoint(context->ListViewHandle, &point);

                menu = PhCreateEMenu();
                PhInsertEMenuItem(menu, secMenuItem = PhCreateEMenuItem(0, IDC_SECURITY, L"&Security\bEnter", NULL, NULL), ULONG_MAX);
                PhInsertEMenuItem(menu, PhCreateEMenuSeparator(), ULONG_MAX);
                PhInsertEMenuItem(menu, PhCreateEMenuItem(0, IDC_COPY, L"&Copy\bCtrl+C", NULL, NULL), ULONG_MAX);
                PhInsertCopyListViewEMenuItem(menu, IDC_COPY, context->ListViewHandle);
                PhSetFlagsEMenuItem(menu, IDC_SECURITY, PH_EMENU_DEFAULT, PH_EMENU_DEFAULT);
                if (numberOfItems > 1)
                    PhSetDisabledEMenuItem(secMenuItem);

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
                        PhCopyListView(context->ListViewHandle);
                        break;
                    case IDC_SECURITY:
                        EtpOpenDesktopSecurity(context, listviewItems[0]);
                        break;
                    }
                }

                PhDestroyEMenu(menu);
            }
        }
        break;
    case WM_KEYDOWN:
        {
            PPH_STRING* listviewItems;
            ULONG numberOfItems;

            switch (LOWORD(wParam))
            {
            case VK_RETURN:
                if (GetFocus() == context->ListViewHandle)
                {
                    PhGetSelectedListViewItemParams(context->ListViewHandle, &listviewItems, &numberOfItems);
                    if (numberOfItems == 1)
                    {
                        EtpOpenDesktopSecurity(context, listviewItems[0]);
                        return TRUE;
                    }
                }
                break;
            }
            break;
        }
    }

    return FALSE;
}
