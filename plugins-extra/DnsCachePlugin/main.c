/*
 * Dns Cache Plugin
 *
 * Copyright (C) 2013 dmex
 *
 * This file is part of Process Hacker.
 *
 * Process Hacker is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Process Hacker is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Process Hacker.  If not, see <http://www.gnu.org/licenses/>.
 */

#define UPDATE_MENUITEM     1000
#define INET_ADDRSTRLEN     22
#define INET6_ADDRSTRLEN    65

#define SETTING_PREFIX L"dmex.DnsCachePlugin."
#define SETTING_NAME_WINDOW_POSITION (SETTING_PREFIX L"WindowPosition")
#define SETTING_NAME_WINDOW_SIZE (SETTING_PREFIX L"WindowSize")
#define SETTING_NAME_COLUMNS (SETTING_PREFIX L".WindowColumns")

#include <phdk.h>
#include <phappresource.h>
#include "resource.h"

#pragma comment(lib, "Ws2_32.lib")
#include <windns.h>
#include <Winsock2.h>

typedef struct _DNS_CACHE_ENTRY
{
    struct _DNS_CACHE_ENTRY* Next;  // Pointer to next entry
    LPCWSTR Name;                   // DNS Record Name
    WORD Type;                      // DNS Record Type
    WORD DataLength;                // Not referenced
    ULONG Flags;                    // DNS Record Flags
} DNS_CACHE_ENTRY, *PDNS_CACHE_ENTRY;

typedef DNS_STATUS (WINAPI* _DnsGetCacheDataTable)(
    __inout PDNS_CACHE_ENTRY* DnsCacheEntry
    );
typedef BOOL (WINAPI* _DnsFlushResolverCache)(
    VOID
    );
typedef BOOL (WINAPI* _DnsFlushResolverCacheEntry)(
    __in LPCWSTR Name
    );
typedef DNS_STATUS (WINAPI* _DnsQuery_W)(
    __in LPCWSTR Name,
    __in WORD Type,
    __in DWORD Options,
    __inout_opt PVOID Extra,
    __out __maybenull PDNS_RECORD* QueryResults,
    __out_opt __maybenull PVOID* Reserved
    );
typedef VOID (WINAPI* _DnsFree)(
    __inout PVOID Data,
    __in DNS_FREE_TYPE FreeType
    );

VOID NTAPI MenuItemCallback(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    );
VOID NTAPI MainWindowShowingCallback(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    );
INT_PTR CALLBACK DnsCacheDlgProc(
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    );

static HINSTANCE DnsApiHandle;
static _DnsQuery_W DnsQuery_I;
static _DnsFree DnsFree_I;
static _DnsGetCacheDataTable DnsGetCacheDataTable_I;
static _DnsFlushResolverCache DnsFlushResolverCache_I;
static _DnsFlushResolverCacheEntry DnsFlushResolverCacheEntry_I;

static HWND ListViewWndHandle;
static PH_LAYOUT_MANAGER LayoutManager;
static PPH_PLUGIN PluginInstance;
static PH_CALLBACK_REGISTRATION PluginMenuItemCallbackRegistration;
static PH_CALLBACK_REGISTRATION MainWindowShowingCallbackRegistration;
static PH_CALLBACK_REGISTRATION PluginShowOptionsCallbackRegistration;

LOGICAL DllMain(
    __in HINSTANCE Instance,
    __in ULONG Reason,
    __reserved PVOID Reserved
    )
{
    switch (Reason)
    {
    case DLL_PROCESS_ATTACH:
        {
            PPH_PLUGIN_INFORMATION info;

            PluginInstance = PhRegisterPlugin(SETTING_PREFIX, Instance, &info);

            if (!PluginInstance)
                return FALSE;

            info->DisplayName = L"DNS Cache Viewer";
            info->Author = L"dmex";
            info->Description = L"Plugin for viewing the DNS Resolver Cache via the Tools menu";
            info->HasOptions = FALSE;

            PhRegisterCallback(
                PhGetGeneralCallback(GeneralCallbackMainWindowShowing),
                MainWindowShowingCallback,
                NULL,
                &MainWindowShowingCallbackRegistration
                );
            PhRegisterCallback(
                PhGetPluginCallback(PluginInstance, PluginCallbackMenuItem),
                MenuItemCallback,
                NULL,
                &PluginMenuItemCallbackRegistration
                );

            {
                static PH_SETTING_CREATE settings[] =
                {
                    { IntegerPairSettingType, SETTING_NAME_WINDOW_POSITION, L"350,350" },
                    { IntegerPairSettingType, SETTING_NAME_WINDOW_SIZE, L"510,380" },
                    { StringSettingType, SETTING_NAME_COLUMNS, L"" }
                };

                PhAddSettings(settings, _countof(settings));
            }
        }
        break;
    }

    return TRUE;
}

static VOID NTAPI MainWindowShowingCallback(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    )
{
    PhPluginAddMenuItem(PluginInstance, PH_MENU_ITEM_LOCATION_TOOLS, L"$", UPDATE_MENUITEM, L"Resolver Cache", NULL);
}

static VOID NTAPI MenuItemCallback(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    )
{
    PPH_PLUGIN_MENU_ITEM menuItem = (PPH_PLUGIN_MENU_ITEM)Parameter;

    switch (menuItem->Id)
    {
    case UPDATE_MENUITEM:
        {
            DialogBox(
                (HINSTANCE)PluginInstance->DllBase,
                MAKEINTRESOURCE(IDD_DNSVIEW),
                NULL,
                DnsCacheDlgProc
                );
        }
        break;
    }
}

static VOID EnumDnsCacheTable(
    __in HWND hwndDlg
    )
{
    WSADATA wsaData;
    PDNS_CACHE_ENTRY dnsCacheRecordPtr = NULL;

    __try
    {
        // Start Winsock (required for WSAAddressToString)
        WSAStartup(WINSOCK_VERSION, &wsaData);

        if (!DnsGetCacheDataTable_I(&dnsCacheRecordPtr))
            __leave;

        while (dnsCacheRecordPtr)
        {
            PDNS_RECORD dnsQueryResultPtr = NULL;

            DNS_STATUS dnsStatus = DnsQuery_I(
                dnsCacheRecordPtr->Name,
                dnsCacheRecordPtr->Type,
                DNS_QUERY_NO_WIRE_QUERY | 32768, // Undocumented flags
                NULL,
                &dnsQueryResultPtr,
                NULL
                );

            if (dnsStatus == ERROR_SUCCESS)
            {
                ULONG dnsRecordCount = 0;
                PDNS_RECORD dnsRecordPtr = dnsQueryResultPtr;

                while (dnsRecordPtr)
                {
                    INT itemIndex = 0;
                    SOCKADDR_IN sockaddr;
                    TCHAR ipAddrString[INET6_ADDRSTRLEN];
                    ULONG ipAddrStringLength = INET6_ADDRSTRLEN;

                    // Convert the Internet network address into a string in Internet standard dotted format.
                    sockaddr.sin_family = AF_INET;
                    sockaddr.sin_addr.s_addr = dnsRecordPtr->Data.A.IpAddress;
                    sockaddr.sin_port = htons(0);

                    // Add the item to the listview (it might be nameless)...
                    if (dnsRecordCount)
                    {
                        itemIndex = PhAddListViewItem(hwndDlg, MAXINT,
                            PhaFormatString(L"%s [%d]", dnsCacheRecordPtr->Name, dnsRecordCount)->Buffer,
                            NULL
                            );
                    }
                    else
                    {
                        itemIndex = PhAddListViewItem(hwndDlg, MAXINT,
                            PhaFormatString(L"%s", dnsCacheRecordPtr->Name)->Buffer,
                            NULL
                            );
                    }

                    PhSetListViewSubItem(hwndDlg, itemIndex, 2, PhaFormatString(L"%d", dnsRecordPtr->dwTtl)->Buffer);

                    if (WSAAddressToString((SOCKADDR*)&sockaddr, sizeof(SOCKADDR_IN), NULL, ipAddrString, &ipAddrStringLength) != SOCKET_ERROR)
                    {
                        PhSetListViewSubItem(hwndDlg, itemIndex, 1, PhaFormatString(L"%s", ipAddrString)->Buffer);
                    }
                    else
                    {
                        //inet_ntoa(ipaddr));
                        PhSetListViewSubItem(hwndDlg, itemIndex, 1, PhaFormatString(L"Error %d", WSAGetLastError())->Buffer);
                    }

                    dnsRecordCount++;
                    dnsRecordPtr = dnsRecordPtr->pNext;
                }

                DnsFree_I(dnsQueryResultPtr, DnsFreeRecordList);
            }

            dnsCacheRecordPtr = dnsCacheRecordPtr->Next;
        }
    }
    __finally
    {
        if (dnsCacheRecordPtr)
        {
            DnsFree_I(dnsCacheRecordPtr, DnsFreeRecordList);
        }

        WSACleanup();
    }
}

static PPH_STRING PhGetSelectedListViewItemText(
    __in HWND hWnd
    )
{
    INT index = PhFindListViewItemByFlags(
        hWnd,
        -1,
        LVNI_SELECTED
        );

    if (index != -1)
    {
        WCHAR textBuffer[MAX_PATH + 1];

        LVITEM item;
        item.mask = LVIF_TEXT;
        item.iItem = index;
        item.iSubItem = 0;
        item.pszText = textBuffer;
        item.cchTextMax = MAX_PATH;

        if (ListView_GetItem(hWnd, &item))
            return PhCreateString(textBuffer);
    }

    return NULL;
}

static VOID ShowStatusMenu(
    __in HWND hwndDlg
    )
{
    HMENU menu;
    HMENU subMenu;
    ULONG id;
    POINT cursorPos = { 0, 0 };

    PPH_STRING cacheEntryName = PhGetSelectedListViewItemText(ListViewWndHandle);

    if (cacheEntryName)
    {
        GetCursorPos(&cursorPos);

        menu = LoadMenu(
            (HINSTANCE)PluginInstance->DllBase,
            MAKEINTRESOURCE(IDR_MAIN_MENU)
            );

        subMenu = GetSubMenu(menu, 0);

        id = (ULONG)TrackPopupMenu(
            subMenu,
            TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RIGHTBUTTON | TPM_NONOTIFY | TPM_RETURNCMD,
            cursorPos.x,
            cursorPos.y,
            0,
            hwndDlg,
            NULL
            );

        DestroyMenu(menu);

        switch (id)
        {
        case ID_DNSENTRY_FLUSH:
            {
                INT lvItemIndex = PhFindListViewItemByFlags(
                    ListViewWndHandle,
                    -1,
                    LVNI_SELECTED
                    );

                if (lvItemIndex != -1)
                {
                    if (!PhGetIntegerSetting(L"EnableWarnings") || PhShowConfirmMessage(
                        hwndDlg,
                        L"remove",
                        cacheEntryName->Buffer,
                        NULL,
                        FALSE
                        ))
                    {
                        if (DnsFlushResolverCacheEntry_I(cacheEntryName->Buffer))
                        {
                            ListView_DeleteItem(ListViewWndHandle, lvItemIndex);
                        }
                    }
                }
            }
            break;
        }

        PhDereferenceObject(cacheEntryName);
    }
}

INT_PTR CALLBACK DnsCacheDlgProc(
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    )
{
    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            PhCenterWindow(hwndDlg, PhMainWndHandle);
            ListViewWndHandle = GetDlgItem(hwndDlg, IDC_LIST1);

            PhInitializeLayoutManager(&LayoutManager, hwndDlg);
            PhAddLayoutItem(&LayoutManager, ListViewWndHandle, NULL, PH_ANCHOR_ALL);
            PhAddLayoutItem(&LayoutManager, GetDlgItem(hwndDlg, IDC_DNSCLEAR), NULL, PH_ANCHOR_BOTTOM | PH_ANCHOR_LEFT);
            PhAddLayoutItem(&LayoutManager, GetDlgItem(hwndDlg, IDOK), NULL, PH_ANCHOR_BOTTOM | PH_ANCHOR_RIGHT);

            PhRegisterDialog(hwndDlg);
            PhLoadWindowPlacementFromSetting(SETTING_NAME_WINDOW_POSITION, SETTING_NAME_WINDOW_SIZE, hwndDlg);

            PhSetListViewStyle(ListViewWndHandle, FALSE, TRUE);
            PhSetControlTheme(ListViewWndHandle, L"explorer");
            PhAddListViewColumn(ListViewWndHandle, 0, 0, 0, LVCFMT_LEFT, 280, L"Name");
            PhAddListViewColumn(ListViewWndHandle, 1, 1, 1, LVCFMT_LEFT, 100, L"IP Address");
            PhAddListViewColumn(ListViewWndHandle, 2, 2, 2, LVCFMT_LEFT, 50, L"TTL");
            PhSetExtendedListView(ListViewWndHandle);
            PhLoadListViewColumnsFromSetting(SETTING_NAME_COLUMNS, ListViewWndHandle);

            DnsApiHandle = LoadLibrary(L"dnsapi.dll");
            if (DnsApiHandle)
            {
                DnsQuery_I = (_DnsQuery_W)GetProcAddress(DnsApiHandle, "DnsQuery_W");
                DnsFree_I = (_DnsFree)GetProcAddress(DnsApiHandle, "DnsFree");
                DnsGetCacheDataTable_I = (_DnsGetCacheDataTable)GetProcAddress(DnsApiHandle, "DnsGetCacheDataTable");
                DnsFlushResolverCache_I = (_DnsFlushResolverCache)GetProcAddress(DnsApiHandle, "DnsFlushResolverCache");
                DnsFlushResolverCacheEntry_I = (_DnsFlushResolverCacheEntry)GetProcAddress(DnsApiHandle, "DnsFlushResolverCacheEntry_W");
            }

            EnumDnsCacheTable(ListViewWndHandle);
        }
        break;
    case WM_DESTROY:
        {
            if (DnsApiHandle)
            {
                FreeLibrary(DnsApiHandle);
                DnsApiHandle = NULL;
            }

            PhSaveWindowPlacementToSetting(SETTING_NAME_WINDOW_POSITION, SETTING_NAME_WINDOW_SIZE, hwndDlg);
            PhSaveListViewColumnsToSetting(SETTING_NAME_COLUMNS, ListViewWndHandle);
            PhDeleteLayoutManager(&LayoutManager);
        }
        break;
    case WM_SIZE:
        {
            PhLayoutManagerLayout(&LayoutManager);
        }
        break;
    case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
            case IDC_DNSCLEAR:
                {
                    if (!PhGetIntegerSetting(L"EnableWarnings") || PhShowConfirmMessage(
                        hwndDlg,
                        L"Flush",
                        L"the dns cache",
                        NULL,
                        FALSE
                        ))
                    {
                        if (DnsFlushResolverCache_I)
                            DnsFlushResolverCache_I();

                        ListView_DeleteAllItems(ListViewWndHandle);
                        EnumDnsCacheTable(ListViewWndHandle);
                    }
                }
                break;
            case IDCANCEL:
            case IDOK:
                EndDialog(hwndDlg, IDOK);
                break;
            }
        }
        break;
    case WM_NOTIFY:
        {
            LPNMHDR hdr = (LPNMHDR)lParam;

            switch (hdr->code)
            {
            case NM_RCLICK:
                {
                    if (hdr->hwndFrom == ListViewWndHandle)
                        ShowStatusMenu(hwndDlg);
                }
                break;
            }
        }
        break;
    }

    return FALSE;
}