/*
 * Dns Cache Plugin
 *
 * Copyright (C) 2012 dmex
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

#define UPDATE_MENUITEM 1000

#include "phdk.h"
#include "phappresource.h"
#include "resource.h"

#pragma comment(lib, "Dnsapi.lib")
#pragma comment(lib, "Ws2_32.lib")
#include <windns.h>
#include <Winsock2.h>

typedef struct _DNS_CACHE_ENTRY 
{
    struct _DNS_CACHE_ENTRY* pNext; // Pointer to next entry
    PCWSTR pszName; // DNS Record Name
    USHORT wType; // DNS Record Type
    USHORT wDataLength; // Not referenced
    ULONG dwFlags; // DNS Record Flags
} DNS_CACHE_ENTRY, *PDNS_CACHE_ENTRY;

typedef DNS_STATUS (WINAPI* _DnsGetCacheDataTable)(__inout PDNS_CACHE_ENTRY DnsCacheEntry);
typedef BOOL (WINAPI* _DnsFlushResolverCache)(VOID);
typedef BOOL (WINAPI* _DnsFlushResolverCacheEntry)(__in PCWSTR pszName); // , WORD type, DWORD options, PVOID servers, PDNS_RECORDW *result, PVOID *reserved 

static HINSTANCE DnsApiHandle;
static _DnsGetCacheDataTable DnsGetCacheDataTable_I;
static _DnsFlushResolverCache DnsFlushResolverCache_I;
static _DnsFlushResolverCacheEntry DnsFlushResolverCacheEntry_I;

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

            PluginInstance = PhRegisterPlugin(L"ProcessHacker.DnsResolverCachePlugin", Instance, &info);

            if (!PluginInstance)
                return FALSE;

            info->DisplayName = L"DNS Resolver Cache Viewer";
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
    PhPluginAddMenuItem(PluginInstance, PH_MENU_ITEM_LOCATION_TOOLS, L"$", UPDATE_MENUITEM, L"DNS Resolver Cache", NULL);
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
    PDNS_CACHE_ENTRY pEntryInfoPtr;
    WSADATA wsaData;

    pEntryInfoPtr = (PDNS_CACHE_ENTRY)PhAllocate(sizeof(DNS_CACHE_ENTRY));
    memset(pEntryInfoPtr, 0, sizeof(DNS_CACHE_ENTRY));

    WSAStartup(WINSOCK_VERSION, &wsaData);

    if (DnsGetCacheDataTable_I(pEntryInfoPtr))
    {
        pEntryInfoPtr = pEntryInfoPtr->pNext;

        while (pEntryInfoPtr) 
        {  
            DNS_RECORD* pQueryResultsSet = NULL;

            // Add the item to the listview (it might be nameless) 
            INT itemIndex = PhAddListViewItem(
                hwndDlg, 
                MAXINT,
                (PWSTR)pEntryInfoPtr->pszName, 
                NULL
                );

            DNS_STATUS hrstatus = DnsQuery(
                pEntryInfoPtr->pszName, 
                pEntryInfoPtr->wType, 
                DNS_QUERY_NO_WIRE_QUERY | 32768, // Oh?? 
                NULL,
                &pQueryResultsSet, 
                NULL
                );

            //while (ppQueryResultsSet) 
            if (hrstatus == ERROR_SUCCESS)
            {
                PPH_STRING ipAddressString = NULL;
                PPH_STRING ipTtlString = NULL;
                IN_ADDR ipAddress;

                ipAddress.S_un.S_addr = pQueryResultsSet->Data.A.IpAddress;

                ipAddressString = PhFormatString(L"%hs", inet_ntoa(ipAddress));
                ipTtlString = PhFormatString(L"%d", pQueryResultsSet->dwTtl);

                PhSetListViewSubItem(hwndDlg, itemIndex, 1, ipAddressString->Buffer);
                PhSetListViewSubItem(hwndDlg, itemIndex, 2, ipTtlString->Buffer);

                PhDereferenceObject(ipTtlString);
                PhDereferenceObject(ipAddressString);

                DnsRecordListFree(pQueryResultsSet, DnsFreeRecordList);
            }

            pEntryInfoPtr = pEntryInfoPtr->pNext;
        }

        DnsRecordListFree(pEntryInfoPtr, DnsFreeRecordList); 
    }

    WSACleanup();
    PhFree(pEntryInfoPtr);
}

PPH_STRING PhGetSelectedListViewItemText(
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
        LOGICAL result;
        LVITEM item;
        TCHAR szBuffer[1024];

        item.mask = LVIF_TEXT;
        item.iItem = index;
        item.iSubItem = 0;
        item.pszText = szBuffer;
        item.cchTextMax = _countof(szBuffer);

        result = ListView_GetItem(hWnd, &item);

        return PhCreateString(szBuffer);
    }

    return NULL;
}

VOID ShowStatusMenu(
    __in HWND hwndDlg
    )
{
    HMENU menu;
    HMENU subMenu;
    ULONG id;
    POINT cursorPos = { 0, 0 };

    PPH_STRING dnsCacheEntry = PhGetSelectedListViewItemText(ListViewWndHandle);

    if (dnsCacheEntry)
    {
        GetCursorPos(&cursorPos);

        menu = LoadMenu(
            (HINSTANCE)PluginInstance->DllBase,
            MAKEINTRESOURCE(IDR_MENU1)
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
                        (PWSTR)dnsCacheEntry->Buffer,
                        NULL,
                        FALSE
                        ))
                    {
                        if (DnsFlushResolverCacheEntry_I(dnsCacheEntry->Buffer))
                        {
                            ListView_DeleteItem(ListViewWndHandle, lvItemIndex);
                        }
                    }
                }
            }
            break;
        }

        PhDereferenceObject(dnsCacheEntry);
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
            ListViewWndHandle = GetDlgItem(hwndDlg, IDC_LIST1);

            PhCenterWindow(hwndDlg, PhMainWndHandle);

            PhSetListViewStyle(ListViewWndHandle, FALSE, TRUE);
            PhSetControlTheme(ListViewWndHandle, L"explorer");
            PhAddListViewColumn(ListViewWndHandle, 0, 0, 0, LVCFMT_LEFT, 280, L"Name");
            PhAddListViewColumn(ListViewWndHandle, 1, 1, 1, LVCFMT_LEFT, 100, L"IP Address");
            PhAddListViewColumn(ListViewWndHandle, 2, 2, 2, LVCFMT_LEFT, 50, L"TTL");
            PhSetExtendedListView(ListViewWndHandle);

            PhInitializeLayoutManager(&LayoutManager, hwndDlg);
            PhAddLayoutItem(&LayoutManager, ListViewWndHandle, NULL, PH_ANCHOR_ALL);
            PhAddLayoutItem(&LayoutManager, GetDlgItem(hwndDlg, IDC_DNSREFRESH), NULL, PH_ANCHOR_BOTTOM | PH_ANCHOR_LEFT);
            PhAddLayoutItem(&LayoutManager, GetDlgItem(hwndDlg, IDC_DNSCLEAR), NULL, PH_ANCHOR_BOTTOM | PH_ANCHOR_LEFT);
            PhAddLayoutItem(&LayoutManager, GetDlgItem(hwndDlg, IDOK), NULL, PH_ANCHOR_BOTTOM | PH_ANCHOR_RIGHT);

            DnsApiHandle = LoadLibrary(TEXT("dnsapi.dll"));
            DnsGetCacheDataTable_I = (_DnsGetCacheDataTable)GetProcAddress(DnsApiHandle, "DnsGetCacheDataTable");
            DnsFlushResolverCache_I = (_DnsFlushResolverCache)GetProcAddress(DnsApiHandle, "DnsFlushResolverCache");
            DnsFlushResolverCacheEntry_I = (_DnsFlushResolverCacheEntry)GetProcAddress(DnsApiHandle, "DnsFlushResolverCacheEntry_W");

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
            case IDC_DNSREFRESH:
                {
                    ListView_DeleteAllItems(ListViewWndHandle);
                    EnumDnsCacheTable(ListViewWndHandle);
                }
                break;
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