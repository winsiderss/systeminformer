/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2010-2011
 *     dmex    2013-2023
 *
 */

#ifndef _NETWORKTOOLSINTF_H
#define _NETWORKTOOLSINTF_H

#define NETWORKTOOLS_PLUGIN_NAME L"ProcessHacker.NetworkTools"
#define NETWORKTOOLS_INTERFACE_VERSION 1

typedef BOOLEAN (NTAPI* PNETWORKTOOLS_GET_COUNTRYCODE)(
    _In_ PH_IP_ADDRESS RemoteAddress,
    _Out_ ULONG* CountryCode,
    _Out_ PPH_STRING* CountryName
    );

typedef LONG (NTAPI* PNETWORKTOOLS_GET_COUNTRYICON)(
    _In_ ULONG CountryCode
    );

typedef BOOLEAN (NTAPI* PNETWORKTOOLS_GET_SERVICENAME)(
    _In_ ULONG Port,
    _Out_ PPH_STRINGREF* ServiceName
    );

typedef VOID (NTAPI* PNETWORKTOOLS_DRAW_COUNTRYICON)(
    _In_ HDC hdc,
    _In_ RECT rect,
    _In_ INT Index
    );

typedef VOID (NTAPI* PNETWORKTOOLS_SHOWWINDOW_PING)(
    _In_ HWND ParentWindowHandle,
    _In_ PH_IP_ENDPOINT Endpoint
    );

typedef VOID (NTAPI* PNETWORKTOOLS_SHOWWINDOW_TRACERT)(
    _In_ HWND ParentWindowHandle,
    _In_ PH_IP_ENDPOINT Endpoint
    );

typedef VOID (NTAPI* PNETWORKTOOLS_SHOWWINDOW_WHOIS)(
    _In_ HWND ParentWindowHandle,
    _In_ PH_IP_ENDPOINT Endpoint
    );

typedef struct _NETWORKTOOLS_INTERFACE
{
    ULONG Version;
    PNETWORKTOOLS_GET_COUNTRYCODE LookupCountryCode;
    PNETWORKTOOLS_GET_COUNTRYICON LookupCountryIcon;
    PNETWORKTOOLS_GET_SERVICENAME LookupPortServiceName;
    PNETWORKTOOLS_DRAW_COUNTRYICON DrawCountryIcon;
    PNETWORKTOOLS_SHOWWINDOW_PING ShowPingWindow;
    PNETWORKTOOLS_SHOWWINDOW_TRACERT ShowTracertWindow;
    PNETWORKTOOLS_SHOWWINDOW_WHOIS ShowWhoisWindow;
} NETWORKTOOLS_INTERFACE, *PNETWORKTOOLS_INTERFACE;

#endif
