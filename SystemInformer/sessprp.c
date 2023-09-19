/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2010
 *     dmex    2018-2023
 *
 */

#include <phapp.h>
#include <emenu.h>
#include <phsettings.h>
#include <winsta.h>

INT_PTR CALLBACK PhpSessionPropertiesDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

static PH_KEY_VALUE_PAIR PhpConnectStatePairs[] =
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

VOID PhShowSessionProperties(
    _In_ HWND ParentWindowHandle,
    _In_ ULONG SessionId
    )
{
    PhDialogBox(
        PhInstanceHandle,
        MAKEINTRESOURCE(IDD_SESSION),
        PhCsForceNoParent ? NULL : ParentWindowHandle,
        PhpSessionPropertiesDlgProc,
        UlongToPtr(SessionId)
        );
}

typedef struct _PHP_SESSION_PROPERTIES_CONTEXT
{
    HWND ListViewHandle;
    ULONG SessionId;
    PH_LAYOUT_MANAGER LayoutManager;
} PHP_SESSION_PROPERTIES_CONTEXT, *PPHP_SESSION_PROPERTIES_CONTEXT;

VOID PhpSessionPropertiesQueryWinStationInfo(
    _In_ PPHP_SESSION_PROPERTIES_CONTEXT Context,
    _In_ ULONG SessionId
    )
{
    WINSTATIONINFORMATION winStationInfo;
    BOOLEAN haveWinStationInfo;
    WINSTATIONCLIENT clientInfo;
    BOOLEAN haveClientInfo;
    ULONG returnLength;
    PWSTR stateString;

    // Query basic session information

    haveWinStationInfo = WinStationQueryInformationW(
        NULL,
        SessionId,
        WinStationInformation,
        &winStationInfo,
        sizeof(WINSTATIONINFORMATION),
        &returnLength
        );

    // Query client information

    haveClientInfo = WinStationQueryInformationW(
        NULL,
        SessionId,
        WinStationClient,
        &clientInfo,
        sizeof(WINSTATIONCLIENT),
        &returnLength
        );

    if (haveWinStationInfo)
    {
        PH_FORMAT format[3];
        WCHAR formatBuffer[256];

        PhInitFormatS(&format[0], winStationInfo.Domain);
        PhInitFormatC(&format[1], OBJ_NAME_PATH_SEPARATOR);
        PhInitFormatS(&format[2], winStationInfo.UserName);

        if (PhFormatToBuffer(
            format,
            RTL_NUMBER_OF(format),
            formatBuffer,
            sizeof(formatBuffer),
            NULL
            ))
        {
            PhSetListViewSubItem(Context->ListViewHandle, 0, 1, formatBuffer);
        }
    }

    PhSetListViewSubItem(Context->ListViewHandle, 1, 1, PhaFormatString(L"%lu", SessionId)->Buffer);

    if (haveWinStationInfo)
    {
        if (PhFindStringSiKeyValuePairs(
            PhpConnectStatePairs,
            sizeof(PhpConnectStatePairs),
            winStationInfo.ConnectState,
            &stateString
            ))
        {
            PhSetListViewSubItem(Context->ListViewHandle, 2, 1, stateString);
        }
    }

    if (haveWinStationInfo && winStationInfo.LogonTime.QuadPart != 0)
    {
        SYSTEMTIME systemTime;
        PPH_STRING time;

        PhLargeIntegerToLocalSystemTime(&systemTime, &winStationInfo.LogonTime);
        time = PhFormatDateTime(&systemTime);
        PhSetListViewSubItem(Context->ListViewHandle, 3, 1, time->Buffer);
        PhDereferenceObject(time);
    }

    if (haveWinStationInfo && winStationInfo.ConnectTime.QuadPart != 0)
    {
        SYSTEMTIME systemTime;
        PPH_STRING time;

        PhLargeIntegerToLocalSystemTime(&systemTime, &winStationInfo.ConnectTime);
        time = PhFormatDateTime(&systemTime);
        PhSetListViewSubItem(Context->ListViewHandle, 4, 1, time->Buffer);
        PhDereferenceObject(time);
    }

    if (haveWinStationInfo && winStationInfo.DisconnectTime.QuadPart != 0)
    {
        SYSTEMTIME systemTime;
        PPH_STRING time;

        PhLargeIntegerToLocalSystemTime(&systemTime, &winStationInfo.DisconnectTime);
        time = PhFormatDateTime(&systemTime);
        PhSetListViewSubItem(Context->ListViewHandle, 5, 1, time->Buffer);
        PhDereferenceObject(time);
    }

    if (haveWinStationInfo && winStationInfo.LastInputTime.QuadPart != 0)
    {
        SYSTEMTIME systemTime;
        PPH_STRING time;

        PhLargeIntegerToLocalSystemTime(&systemTime, &winStationInfo.LastInputTime);
        time = PhFormatDateTime(&systemTime);
        PhSetListViewSubItem(Context->ListViewHandle, 6, 1, time->Buffer);
        PhDereferenceObject(time);
    }

    if (haveClientInfo && clientInfo.ClientName[0] != UNICODE_NULL)
    {
        WCHAR addressString[INET6_ADDRSTRLEN + 1];

        PhSetListViewSubItem(Context->ListViewHandle, 7, 1, clientInfo.ClientName);

        if (clientInfo.ClientAddressFamily == AF_INET6)
        {
            ULONG addressLength = RTL_NUMBER_OF(addressString);
            IN6_ADDR address;
            ULONG i;
            PUSHORT in;
            PUSHORT out;

            // IPv6 is special - the client address data is a reversed version of
            // the real address.

            in = (PUSHORT)clientInfo.ClientAddress;
            out = (PUSHORT)address.u.Word;

            for (i = 8; i != 0; i--)
            {
                *out = _byteswap_ushort(*in);
                in++;
                out++;
            }

            RtlIpv6AddressToStringEx(&address, 0, 0, addressString, &addressLength);
        }
        else
        {
            wcscpy_s(addressString, 65, clientInfo.ClientAddress);
        }

        PhSetListViewSubItem(Context->ListViewHandle, 8, 1, addressString);
        PhSetListViewSubItem(Context->ListViewHandle, 9, 1,
            PhaFormatString(L"%ux%u@%u", clientInfo.HRes, clientInfo.VRes, clientInfo.ColorDepth)->Buffer
            );
    }
}

INT_PTR CALLBACK PhpSessionPropertiesDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PPHP_SESSION_PROPERTIES_CONTEXT context;

    if (uMsg == WM_INITDIALOG)
    {
        context = PhAllocateZero(sizeof(PHP_SESSION_PROPERTIES_CONTEXT));
        context->SessionId = (ULONG)lParam;

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
            context->ListViewHandle = GetDlgItem(hwndDlg, IDC_LIST);

            PhSetApplicationWindowIcon(hwndDlg);

            PhSetListViewStyle(context->ListViewHandle, FALSE, TRUE);
            PhSetControlTheme(context->ListViewHandle, L"explorer");
            PhAddListViewColumn(context->ListViewHandle, 0, 0, 0, LVCFMT_LEFT, 150, L"Name");
            PhAddListViewColumn(context->ListViewHandle, 1, 1, 1, LVCFMT_LEFT, 250, L"Value");
            PhSetExtendedListView(context->ListViewHandle);

            ListView_EnableGroupView(context->ListViewHandle, TRUE);
            PhAddListViewGroup(context->ListViewHandle, 0, L"User");
            //PhAddListViewGroup(context->ListViewHandle, 1, L"Profile");

            PhAddListViewGroupItem(context->ListViewHandle, 0, 0, L"User name", NULL);
            PhAddListViewGroupItem(context->ListViewHandle, 0, 1, L"Session ID", NULL);
            PhAddListViewGroupItem(context->ListViewHandle, 0, 2, L"State", NULL);
            PhAddListViewGroupItem(context->ListViewHandle, 0, 3, L"Logon time", NULL);
            PhAddListViewGroupItem(context->ListViewHandle, 0, 4, L"Connect time", NULL);
            PhAddListViewGroupItem(context->ListViewHandle, 0, 5, L"Disconnect time", NULL);
            PhAddListViewGroupItem(context->ListViewHandle, 0, 6, L"Last input time", NULL);
            PhAddListViewGroupItem(context->ListViewHandle, 0, 7, L"Client name", NULL);
            PhAddListViewGroupItem(context->ListViewHandle, 0, 8, L"Client address", NULL);
            PhAddListViewGroupItem(context->ListViewHandle, 0, 9, L"Client display", NULL);

            //PhAddListViewGroupItem(context->ListViewHandle, 1, 10, L"LastLogon", NULL);
            //PhAddListViewGroupItem(context->ListViewHandle, 1, 11, L"LastLogoff", NULL);
            //PhAddListViewGroupItem(context->ListViewHandle, 1, 12, L"Home directory", NULL);
            //PhAddListViewGroupItem(context->ListViewHandle, 1, 13, L"Profile directory", NULL);

            PhInitializeLayoutManager(&context->LayoutManager, hwndDlg);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDC_LIST), NULL, PH_ANCHOR_ALL);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDOK), NULL, PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM);

            PhpSessionPropertiesQueryWinStationInfo(context, context->SessionId);
            //PhpSessionPropertiesQuerySamAccountInfo(context, context->SessionId);

            PhSetDialogFocus(hwndDlg, GetDlgItem(hwndDlg, IDOK));

            PhCenterWindow(hwndDlg, GetParent(hwndDlg));

            PhInitializeWindowTheme(hwndDlg, PhEnableThemeSupport); // HACK
        }
        break;
    case WM_DESTROY:
        {
            PhRemoveWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT);

            PhDeleteLayoutManager(&context->LayoutManager);
            PhFree(context);
        }
        break;
    case WM_COMMAND:
        {
            switch (GET_WM_COMMAND_ID(wParam, lParam))
            {
            case IDCANCEL:
            case IDOK:
                EndDialog(hwndDlg, IDOK);
                break;
            }
        }
        break;
    case WM_CONTEXTMENU:
        {
            if ((HWND)wParam == context->ListViewHandle)
            {
                POINT point;
                PPH_EMENU menu;
                PPH_EMENU item;
                PVOID* listviewItems;
                ULONG numberOfItems;

                point.x = GET_X_LPARAM(lParam);
                point.y = GET_Y_LPARAM(lParam);

                if (point.x == -1 && point.y == -1)
                    PhGetListViewContextMenuPoint(context->ListViewHandle, &point);

                PhGetSelectedListViewItemParams(context->ListViewHandle, &listviewItems, &numberOfItems);

                if (numberOfItems != 0)
                {
                    menu = PhCreateEMenu();
                    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, IDC_COPY, L"&Copy", NULL, NULL), ULONG_MAX);
                    PhInsertCopyListViewEMenuItem(menu, IDC_COPY, context->ListViewHandle);

                    item = PhShowEMenu(
                        menu,
                        hwndDlg,
                        PH_EMENU_SHOW_SEND_COMMAND | PH_EMENU_SHOW_LEFTRIGHT,
                        PH_ALIGN_LEFT | PH_ALIGN_TOP,
                        point.x,
                        point.y
                        );

                    if (item)
                    {
                        BOOLEAN handled = FALSE;

                        handled = PhHandleCopyListViewEMenuItem(item);

                        //if (!handled && PhPluginsEnabled)
                        //    handled = PhPluginTriggerEMenuItem(&menuInfo, item);

                        if (!handled)
                        {
                            switch (item->Id)
                            {
                            case IDC_COPY:
                                {
                                    PhCopyListView(context->ListViewHandle);
                                }
                                break;
                            }
                        }
                    }

                    PhDestroyEMenu(menu);
                }

                PhFree(listviewItems);
            }
        }
        break;
    case WM_SIZE:
        {
            PhLayoutManagerLayout(&context->LayoutManager);
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
