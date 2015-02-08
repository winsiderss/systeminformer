/*
 * Process Hacker Extra Plugins -
 *   DNS Cache Plugin
 *
 * Copyright (C) 2014 dmex
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

#include "main.h"

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

static VOID EnumDnsCacheTable(
    _In_ HWND hwndDlg
    )
{
    PDNS_CACHE_ENTRY dnsCacheDataTable = NULL;

    __try
    {
        if (!DnsGetCacheDataTable_I(&dnsCacheDataTable))
            __leave;

        while (dnsCacheDataTable)
        {
            PDNS_RECORD dnsQueryResultPtr = NULL;

            DNS_STATUS dnsStatus = DnsQuery_I(
                dnsCacheDataTable->Name,
                dnsCacheDataTable->Type,
                DNS_QUERY_NO_WIRE_QUERY | 32768, // Undocumented flags
                NULL,
                &dnsQueryResultPtr,
                NULL
                );

            if (dnsStatus == ERROR_SUCCESS)
            {
                PDNS_RECORD dnsRecordPtr = dnsQueryResultPtr;

                while (dnsRecordPtr)
                {
                    INT itemIndex = MAXINT;
                    ULONG ipAddrStringLength = INET6_ADDRSTRLEN;
                    TCHAR ipAddrString[INET6_ADDRSTRLEN] = { '\0' };
                           
                    itemIndex = PhAddListViewItem(hwndDlg, MAXINT,
                        PhaFormatString(L"%s", dnsCacheDataTable->Name)->Buffer,
                        NULL
                        );

                    if (dnsRecordPtr->wType == DNS_TYPE_A)
                    {
                        IN_ADDR ipv4Address = { 0 };

                        ipv4Address.s_addr = dnsRecordPtr->Data.A.IpAddress;

                        RtlIpv4AddressToString(&ipv4Address, ipAddrString);
                       
                        PhSetListViewSubItem(hwndDlg, itemIndex, 1, PhaFormatString(L"A")->Buffer);
                        PhSetListViewSubItem(hwndDlg, itemIndex, 2, PhaFormatString(L"%s", ipAddrString)->Buffer);
                    }
                    else if (dnsRecordPtr->wType == DNS_TYPE_AAAA)
                    {
                        IN6_ADDR ipv6Address = { 0 };

                        memcpy_s(
                            ipv6Address.s6_addr, 
                            sizeof(ipv6Address.s6_addr),
                            dnsRecordPtr->Data.AAAA.Ip6Address.IP6Byte, 
                            sizeof(dnsRecordPtr->Data.AAAA.Ip6Address.IP6Byte)
                            );

                        RtlIpv6AddressToString(&ipv6Address, ipAddrString);

                        PhSetListViewSubItem(hwndDlg, itemIndex, 1, PhaFormatString(L"AAAA")->Buffer);
                        PhSetListViewSubItem(hwndDlg, itemIndex, 2, PhaFormatString(L"%s", ipAddrString)->Buffer);              
                    }
                    else if (dnsRecordPtr->wType == DNS_TYPE_PTR)
                    {
                        PhSetListViewSubItem(hwndDlg, itemIndex, 1, PhaFormatString(L"PTR")->Buffer);
                        PhSetListViewSubItem(hwndDlg, itemIndex, 2, PhaFormatString(L"%s", dnsRecordPtr->Data.PTR.pNameHost)->Buffer);
                    }
                    else if (dnsRecordPtr->wType == DNS_TYPE_CNAME)
                    {
                        PhSetListViewSubItem(hwndDlg, itemIndex, 1, PhaFormatString(L"CNAME")->Buffer);
                        PhSetListViewSubItem(hwndDlg, itemIndex, 2, PhaFormatString(L"%s", dnsRecordPtr->Data.CNAME.pNameHost)->Buffer);
                    }
                    else
                    {          
                        PhSetListViewSubItem(hwndDlg, itemIndex, 1, PhaFormatString(L"UNKNOWN")->Buffer);
                        PhSetListViewSubItem(hwndDlg, itemIndex, 2, PhaFormatString(L""));
                    }   
                                                  
                    PhSetListViewSubItem(hwndDlg, itemIndex, 3, PhaFormatString(L"%d", dnsRecordPtr->dwTtl)->Buffer);

                    dnsRecordPtr = dnsRecordPtr->pNext;
                }

                DnsFree_I(dnsQueryResultPtr, DnsFreeRecordList);
            }

            dnsCacheDataTable = dnsCacheDataTable->Next;
        }
    }
    __finally
    {
        if (dnsCacheDataTable)
        {
            DnsFree_I(dnsCacheDataTable, DnsFreeRecordList);
        }
    }
}

static PPH_STRING PhGetSelectedListViewItemText(
    _In_ HWND hWnd
    )
{
    INT index = PhFindListViewItemByFlags(
        hWnd,
        -1,
        LVNI_SELECTED
        );

    if (index != -1)
    {
        WCHAR textBuffer[MAX_PATH + 1] = { '\0' };

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
    _In_ HWND hwndDlg
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

static INT_PTR CALLBACK DnsCacheDlgProc(
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
            PhCenterWindow(hwndDlg, PhMainWndHandle);
            ListViewWndHandle = GetDlgItem(hwndDlg, IDC_DNSLIST);
                   
            PhRegisterDialog(hwndDlg);          
            PhSetListViewStyle(ListViewWndHandle, FALSE, TRUE);
            PhSetControlTheme(ListViewWndHandle, L"explorer");
            PhAddListViewColumn(ListViewWndHandle, 0, 0, 0, LVCFMT_LEFT, 280, L"Host Name");
            PhAddListViewColumn(ListViewWndHandle, 1, 1, 1, LVCFMT_LEFT, 70, L"Type");
            PhAddListViewColumn(ListViewWndHandle, 2, 2, 2, LVCFMT_LEFT, 100, L"IP Address");
            PhAddListViewColumn(ListViewWndHandle, 3, 3, 3, LVCFMT_LEFT, 50, L"TTL");
            PhSetExtendedListView(ListViewWndHandle);

            PhInitializeLayoutManager(&LayoutManager, hwndDlg);
            PhAddLayoutItem(&LayoutManager, ListViewWndHandle, NULL, PH_ANCHOR_ALL);
            PhAddLayoutItem(&LayoutManager, GetDlgItem(hwndDlg, IDC_DNS_REFRESH), NULL, PH_ANCHOR_BOTTOM | PH_ANCHOR_LEFT);
            PhAddLayoutItem(&LayoutManager, GetDlgItem(hwndDlg, IDC_DNS_CLEAR), NULL, PH_ANCHOR_BOTTOM | PH_ANCHOR_LEFT);
            PhAddLayoutItem(&LayoutManager, GetDlgItem(hwndDlg, IDOK), NULL, PH_ANCHOR_BOTTOM | PH_ANCHOR_RIGHT);     
            PhLoadWindowPlacementFromSetting(SETTING_NAME_WINDOW_POSITION, SETTING_NAME_WINDOW_SIZE, hwndDlg);
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
            case IDC_DNS_CLEAR:
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
            case IDC_DNS_REFRESH:
                ListView_DeleteAllItems(ListViewWndHandle);
                EnumDnsCacheTable(ListViewWndHandle);
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

static VOID NTAPI MainWindowShowingCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    PhPluginAddMenuItem(PluginInstance, PH_MENU_ITEM_LOCATION_TOOLS, L"$", DNSCACHE_MENUITEM, L"DNS Resolver Cache", NULL);
}

static VOID NTAPI MenuItemCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    PPH_PLUGIN_MENU_ITEM menuItem = (PPH_PLUGIN_MENU_ITEM)Parameter;

    switch (menuItem->Id)
    {
    case DNSCACHE_MENUITEM:
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

LOGICAL DllMain(
    _In_ HINSTANCE Instance,
    _In_ ULONG Reason,
    _Reserved_ PVOID Reserved
    )
{
    switch (Reason)
    {
    case DLL_PROCESS_ATTACH:
        {
            PPH_PLUGIN_INFORMATION info;
            PH_SETTING_CREATE settings[] =
            {
                { IntegerPairSettingType, SETTING_NAME_WINDOW_POSITION, L"350,350" },
                { IntegerPairSettingType, SETTING_NAME_WINDOW_SIZE, L"510,380" },
                { StringSettingType, SETTING_NAME_COLUMNS, L"" }
            };

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

            PhAddSettings(settings, _countof(settings));
        }
        break;
    }

    return TRUE;
}