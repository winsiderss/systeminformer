/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2010-2016
 *     dmex    2017-2024
 *
 */

#include <phapp.h>
#include <symprv.h>

#include <hndlprv.h>
#include <mainwnd.h>
#include <memprv.h>
#include <modprv.h>
#include <netprv.h>
#include <phappres.h>
#include <phintrnl.h>
#include <phsettings.h>
#include <procprv.h>
#include <settings.h>
#include <srvprv.h>
#include <thrdprv.h>

static HWND PhAboutWindowHandle = NULL;

static INT_PTR CALLBACK PhpAboutDlgProc(
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
            PPH_STRING versionString;

            PhSetApplicationWindowIcon(hwndDlg);

            PhCenterWindow(hwndDlg, PhMainWndHandle);

            versionString = PhGetApplicationVersionString(TRUE);
            PhMoveReference(&versionString, PhConcatStringRefZ(&versionString->sr, L"\r\n"));
            PhSetDialogItemText(hwndDlg, IDC_ABOUT_NAME, versionString->Buffer);
            PhDereferenceObject(versionString);

            PhSetDialogItemText(hwndDlg, IDC_CREDITS,
                L"Thanks to:\n"
                L"    <a href=\"https://github.com/wj32\">wj32</a> - Wen Jia Liu\n"
                L"    <a href=\"https://github.com/dmex\">dmex</a> - Steven G\n"
                L"    <a href=\"https://github.com/jxy-s\">jxy-s</a> - Johnny Shaw\n"
                L"    <a href=\"https://github.com/ionescu007\">ionescu007</a> - Alex Ionescu\n"
                L"    <a href=\"https://github.com/yardenshafir\">yardenshafir</a> - Yarden Shafir\n"
                L"    <a href=\"https://github.com/winsiderss/systeminformer/graphs/contributors\">Contributors</a> - thank you for your additions!\n"
                L"    Donors - thank you for your support!\n\n"
                L"System Informer uses the following components:\n"
                L"    <a href=\"https://github.com/GameTechDev/PresentMon\">PresentMon</a> by Intel Corporation\n"
                L"    <a href=\"https://github.com/michaelrsweet/mxml\">Mini-XML</a> by Michael Sweet\n"
                L"    <a href=\"https://github.com/PCRE2Project/pcre2\">PCRE2</a> by Philip Hazel\n"
                L"    <a href=\"https://github.com/json-c/json-c\">json-c</a> by Michael Clark\n"
                L"    MD5 code by Jouni Malinen\n"
                L"    SHA1 code by Filip Navara, based on code by Steve Reid\n"
                );

            PhSetDialogFocus(hwndDlg, GetDlgItem(hwndDlg, IDOK));
            PhRegisterWindowCallback(hwndDlg, PH_PLUGIN_WINDOW_EVENT_TYPE_TOPMOST, NULL);

            PhInitializeWindowTheme(hwndDlg, PhEnableThemeSupport);
        }
        break;
    case WM_DESTROY:
        {
            PhUnregisterWindowCallback(hwndDlg);
            PhUnregisterDialog(PhAboutWindowHandle);
            PhAboutWindowHandle = NULL;
        }
        break;
    case WM_COMMAND:
        {
            switch (GET_WM_COMMAND_ID(wParam, lParam))
            {
            case IDCANCEL:
            case IDOK:
                DestroyWindow(hwndDlg);
                break;
            case IDC_DIAGNOSTICS:
                {
                    PhShowInformationDialog(hwndDlg, PH_AUTO_T(PH_STRING, PhGetDiagnosticsString())->Buffer, 0);
                }
                break;
            }
        }
        break;
    case WM_NOTIFY:
        {
            LPNMHDR header = (LPNMHDR)lParam;

            switch (header->code)
            {
            case NM_CLICK:
                {
                    switch (header->idFrom)
                    {
                    case IDC_ABOUT_NAME:
                    case IDC_CREDITS:
                    case IDC_LINK_SF:
                        PhShellExecute(hwndDlg, ((PNMLINK)header)->item.szUrl, NULL);
                        break;
                    }
                }
                break;
            }
        }
        break;
    case WM_CTLCOLORBTN:
        return HANDLE_WM_CTLCOLORBTN(hwndDlg, wParam, lParam, PhWindowThemeControlColor);
    case WM_CTLCOLORDLG:
        return HANDLE_WM_CTLCOLORDLG(hwndDlg, wParam, lParam, PhWindowThemeControlColor);
    case WM_CTLCOLORSTATIC:
        return HANDLE_WM_CTLCOLORSTATIC(hwndDlg, wParam, lParam, PhWindowThemeControlColor);
    }

    return FALSE;
}

VOID PhShowAboutDialog(
    _In_ HWND ParentWindowHandle
    )
{
    if (!PhAboutWindowHandle)
    {
        PhAboutWindowHandle = PhCreateDialog(
            PhInstanceHandle,
            MAKEINTRESOURCE(IDD_ABOUT),
            PhCsForceNoParent ? NULL : ParentWindowHandle,
            PhpAboutDlgProc,
            NULL
            );
        PhRegisterDialog(PhAboutWindowHandle);
        ShowWindow(PhAboutWindowHandle, SW_SHOW);
    }

    if (IsMinimized(PhAboutWindowHandle))
        ShowWindow(PhAboutWindowHandle, SW_RESTORE);
    else
        SetForegroundWindow(PhAboutWindowHandle);
}

FORCEINLINE ULONG PhpGetObjectTypeObjectCount(
    _In_ PPH_OBJECT_TYPE ObjectType
    )
{
    PH_OBJECT_TYPE_INFORMATION info;

    memset(&info, 0, sizeof(PH_OBJECT_TYPE_INFORMATION));
    if (ObjectType) PhGetObjectTypeInformation(ObjectType, &info);

    return info.NumberOfObjects;
}

PPH_STRING PhGetDiagnosticsString(
    VOID
    )
{
    PPH_STRING versionString;
    PH_STRING_BUILDER stringBuilder;

    PhInitializeStringBuilder(&stringBuilder, 50);

    versionString = PhGetApplicationVersionString(FALSE);
    PhAppendStringBuilder(&stringBuilder, &versionString->sr);
    PhAppendStringBuilder2(&stringBuilder, L"\r\n");
    PhDereferenceObject(versionString);

    PhAppendStringBuilder2(&stringBuilder, L"OBJECT INFORMATION\r\n");

#define OBJECT_TYPE_COUNT(Type) PhAppendFormatStringBuilder(&stringBuilder, \
    TEXT(#Type) L": %lu objects\r\n", PhpGetObjectTypeObjectCount(Type))

    // ref
    OBJECT_TYPE_COUNT(PhObjectTypeObject);

    // basesup
    OBJECT_TYPE_COUNT(PhStringType);
    OBJECT_TYPE_COUNT(PhBytesType);
    OBJECT_TYPE_COUNT(PhListType);
    OBJECT_TYPE_COUNT(PhPointerListType);
    OBJECT_TYPE_COUNT(PhHashtableType);
    OBJECT_TYPE_COUNT(PhFileStreamType);

    // ph
    OBJECT_TYPE_COUNT(PhSymbolProviderType);
    OBJECT_TYPE_COUNT(PhProcessItemType);
    OBJECT_TYPE_COUNT(PhServiceItemType);
    OBJECT_TYPE_COUNT(PhNetworkItemType);
    OBJECT_TYPE_COUNT(PhModuleProviderType);
    OBJECT_TYPE_COUNT(PhModuleItemType);
    OBJECT_TYPE_COUNT(PhThreadProviderType);
    OBJECT_TYPE_COUNT(PhThreadItemType);
    OBJECT_TYPE_COUNT(PhHandleProviderType);
    OBJECT_TYPE_COUNT(PhHandleItemType);
    OBJECT_TYPE_COUNT(PhMemoryItemType);
    OBJECT_TYPE_COUNT(PhImageListItemType);

#ifdef DEBUG
    PhAppendStringBuilder2(&stringBuilder, L"STATISTIC INFORMATION\r\n");

#define PRINT_STATISTIC(Name) PhAppendFormatStringBuilder(&stringBuilder, \
    TEXT(#Name) L": %u\r\n", PhLibStatisticsBlock.Name)

    PRINT_STATISTIC(BaseThreadsCreated);
    PRINT_STATISTIC(BaseThreadsCreateFailed);
    PRINT_STATISTIC(BaseStringBuildersCreated);
    PRINT_STATISTIC(BaseStringBuildersResized);
    PRINT_STATISTIC(RefObjectsCreated);
    PRINT_STATISTIC(RefObjectsDestroyed);
    PRINT_STATISTIC(RefObjectsAllocated);
    PRINT_STATISTIC(RefObjectsFreed);
    PRINT_STATISTIC(RefObjectsAllocatedFromSmallFreeList);
    PRINT_STATISTIC(RefObjectsFreedToSmallFreeList);
    PRINT_STATISTIC(RefObjectsAllocatedFromTypeFreeList);
    PRINT_STATISTIC(RefObjectsFreedToTypeFreeList);
    PRINT_STATISTIC(RefObjectsDeleteDeferred);
    PRINT_STATISTIC(RefAutoPoolsCreated);
    PRINT_STATISTIC(RefAutoPoolsDestroyed);
    PRINT_STATISTIC(RefAutoPoolsDynamicAllocated);
    PRINT_STATISTIC(RefAutoPoolsDynamicResized);
    PRINT_STATISTIC(QlBlockSpins);
    PRINT_STATISTIC(QlBlockWaits);
    PRINT_STATISTIC(QlAcquireExclusiveBlocks);
    PRINT_STATISTIC(QlAcquireSharedBlocks);
    PRINT_STATISTIC(WqWorkQueueThreadsCreated);
    PRINT_STATISTIC(WqWorkQueueThreadsCreateFailed);
    PRINT_STATISTIC(WqWorkItemsQueued);
#endif

    return PhFinalStringBuilderString(&stringBuilder);
}

PPH_STRING PhGetApplicationVersionString(
    _In_ BOOLEAN LinkToCommit
    )
{
    PCWSTR channelName = PhGetPhReleaseChannelString();

#if (PHAPP_VERSION_REVISION != 0)
    if (LinkToCommit)
    {
        PH_FORMAT format[14];

        // "System Informer %lu.%lu.%lu (<a href=\"https://github.com/winsiderss/systeminformer/commit/%hs\">%hs</a>) %ls"
        PhInitFormatS(&format[0], L"System Informer ");
        PhInitFormatU(&format[1], PHAPP_VERSION_MAJOR);
        PhInitFormatC(&format[2], L'.');
        PhInitFormatU(&format[3], PHAPP_VERSION_MINOR);
        PhInitFormatC(&format[4], L'.');
        PhInitFormatU(&format[5], PHAPP_VERSION_BUILD);
        PhInitFormatC(&format[6], L'.');
        PhInitFormatU(&format[7], PHAPP_VERSION_REVISION);
        PhInitFormatS(&format[8], L" (<a href=\"https://systeminformer.sourceforge.io/fwlink?commit=");
        PhInitFormatMultiByteS(&format[9], PHAPP_VERSION_COMMIT);
        PhInitFormatS(&format[10], L"\">");
        PhInitFormatMultiByteS(&format[11], PHAPP_VERSION_COMMIT);
        PhInitFormatS(&format[12], L"</a>) ");
        PhInitFormatS(&format[13], (PWSTR)channelName);

        return PhFormat(format, RTL_NUMBER_OF(format), 0);
    }
    else
    {
        PH_FORMAT format[12];

        // "System Informer %lu.%lu.%lu (%hs) %ls"
        PhInitFormatS(&format[0], L"System Informer ");
        PhInitFormatU(&format[1], PHAPP_VERSION_MAJOR);
        PhInitFormatC(&format[2], L'.');
        PhInitFormatU(&format[3], PHAPP_VERSION_MINOR);
        PhInitFormatC(&format[4], L'.');
        PhInitFormatU(&format[5], PHAPP_VERSION_BUILD);
        PhInitFormatC(&format[6], L'.');
        PhInitFormatU(&format[7], PHAPP_VERSION_REVISION);
        PhInitFormatS(&format[8], L" (");
        PhInitFormatMultiByteS(&format[9], PHAPP_VERSION_COMMIT);
        PhInitFormatS(&format[10], L") ");
        PhInitFormatS(&format[11], (PWSTR)channelName);

        return PhFormat(format, RTL_NUMBER_OF(format), 0);
    }
#else
    PH_FORMAT format[10];

    // "System Informer %lu.%lu %ls"
    PhInitFormatS(&format[0], L"System Informer ");
    PhInitFormatU(&format[1], PHAPP_VERSION_MAJOR);
    PhInitFormatC(&format[2], L'.');
    PhInitFormatU(&format[3], PHAPP_VERSION_MINOR);
    PhInitFormatC(&format[4], L'.');
    PhInitFormatU(&format[5], PHAPP_VERSION_BUILD);
    PhInitFormatC(&format[6], L'.');
    PhInitFormatU(&format[7], PHAPP_VERSION_REVISION);
    PhInitFormatC(&format[8], L' ');
    PhInitFormatS(&format[9], (PWSTR)channelName);

    return PhFormat(format, RTL_NUMBER_OF(format), 0);
#endif
}
