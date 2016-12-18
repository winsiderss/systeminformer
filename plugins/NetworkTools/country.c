/*
 * Process Hacker Network Tools  -
 *   IP Country support
 *
 * Copyright (C) 2016 dmex
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

#include "nettools.h"
#include "maxminddb\maxminddb.h"
#include <shlobj.h>
#include <locale.h>
#include <math.h>

BOOLEAN GeoDbLoaded = FALSE;
BOOLEAN GeoDbExpired = FALSE;
static MMDB_s GeoDb = { 0 };

/*  Definitions:                                                           */
/*    South latitudes are negative, east longitudes are positive           */
/*                                                                         */
/*  Passed to function:                                                    */
/*    lat1, lon1 = Latitude and Longitude of point 1 (in decimal degrees)  */
/*    lat2, lon2 = Latitude and Longitude of point 2 (in decimal degrees)  */
/*    unit = the unit you desire for results                               */
/*           where: 'M' is statute miles                                   */
/*                  'K' is kilometers (default)                            */
/*                  'N' is nautical miles   (distance = distance * 0.8684) */
#define pi 3.14159265358979323846

/* decimal degrees to radians */
FORCEINLINE
DOUBLE deg2rad(DOUBLE deg)
{
    return (deg * pi / 180);
}

/* radians to decimal degrees */
FORCEINLINE
DOUBLE rad2deg(DOUBLE rad)
{
    return (rad * 180 / pi);
}

VOID LoadGeoLiteDb(
    VOID
    )
{
    PPH_STRING path;
    PPH_STRING directory;

    directory = PH_AUTO(PhGetApplicationDirectory());
    path = PhConcatStrings(2, PhGetString(directory), L"Plugins\\plugindata\\GeoLite2-City.mmdb");

    if (MMDB_open(PhGetString(path), MMDB_MODE_MMAP, &GeoDb) == MMDB_SUCCESS)
    {
        time_t systemTime;

        // Query the current time
        time(&systemTime);

        // Check if the Geoip database is older than 6 months (182 days = approx. 6 months).
        if ((systemTime - GeoDb.metadata.build_epoch) > (182 * 24 * 60 * 60))
        {
            GeoDbExpired = TRUE;
        }

        if (GeoDb.metadata.ip_version == 6)
        {
            // Database includes ipv6 entires.
        }

        GeoDbLoaded = TRUE;
    }
}

VOID FreeGeoLiteDb(
    VOID
    )
{
    if (GeoDbLoaded)
    {
        MMDB_close(&GeoDb);
    }
}

BOOLEAN GeoLocaleIsMetric(
    VOID
    )
{
    static PH_INITONCE initOnce = PH_INITONCE_INIT;
    static BOOLEAN localeIsMetric = FALSE;

    if (PhBeginInitOnce(&initOnce))
    {
        WCHAR buffer[4] = L"";

        if (GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_IMEASURE, buffer, ARRAYSIZE(buffer)))
        {
            if (PhEqualStringZ(buffer, L"1", TRUE))
            {
                localeIsMetric = TRUE;
            }
        }

        PhEndInitOnce(&initOnce);
    }

    return localeIsMetric;
}

PPH_STRING GeoLookupCityDistance(
    _In_ DOUBLE Latitude,
    _In_ DOUBLE Longitude,
    _In_ DOUBLE CompareLatitude,
    _In_ DOUBLE CompareLongitude
    )
{
    DOUBLE theta = 0;
    DOUBLE distance = 0;

    theta = Longitude - CompareLongitude;
    distance = sin(deg2rad(Latitude)) * sin(deg2rad(CompareLatitude)) + cos(deg2rad(Latitude)) * cos(deg2rad(CompareLatitude)) * cos(deg2rad(theta));
    distance = acos(distance);
    distance = rad2deg(distance);
    distance = distance * 60 * 1.1515;

    if (GeoLocaleIsMetric())
    {
        return PhFormatString(L"%.2f mi", distance);
    }
    else
    {
        return PhFormatString(L"%.2f km", distance * 1.609344);
    }
}

BOOLEAN LookupCountryCode(
    _In_ PH_IP_ADDRESS RemoteAddress,
    _Out_ PPH_STRING *CountryCode,
    _Out_ PPH_STRING *CountryName,
    _Out_opt_ DOUBLE *CityLatitude,
    _Out_opt_ DOUBLE *CityLongitude
    )
{
    PPH_STRING countryCode = NULL;
    PPH_STRING countryName = NULL;
    MMDB_entry_data_s mmdb_entry;
    MMDB_lookup_result_s mmdb_result;
    INT mmdb_error = 0;

    if (!GeoDbLoaded)
        return FALSE;

    if (RemoteAddress.Type == PH_IPV4_NETWORK_TYPE)
    {
        SOCKADDR_IN ipv4SockAddr;

        if (IN4_IS_ADDR_UNSPECIFIED(&RemoteAddress.InAddr))
            return FALSE;

        if (IN4_IS_ADDR_LOOPBACK(&RemoteAddress.InAddr))
            return FALSE;

        memset(&ipv4SockAddr, 0, sizeof(SOCKADDR_IN));
        memset(&mmdb_result, 0, sizeof(MMDB_lookup_result_s));

        ipv4SockAddr.sin_family = AF_INET;
        ipv4SockAddr.sin_addr = RemoteAddress.InAddr;

        mmdb_result = MMDB_lookup_sockaddr(
            &GeoDb,
            (struct sockaddr*)&ipv4SockAddr,
            &mmdb_error
            );
    }
    else
    {
        SOCKADDR_IN6 ipv6SockAddr;

        if (IN6_IS_ADDR_UNSPECIFIED(&RemoteAddress.In6Addr))
            return FALSE;

        if (IN6_IS_ADDR_LOOPBACK(&RemoteAddress.In6Addr))
            return FALSE;

        memset(&ipv6SockAddr, 0, sizeof(SOCKADDR_IN6));
        memset(&mmdb_result, 0, sizeof(MMDB_lookup_result_s));

        ipv6SockAddr.sin6_family = AF_INET6;
        ipv6SockAddr.sin6_addr = RemoteAddress.In6Addr;

        mmdb_result = MMDB_lookup_sockaddr(
            &GeoDb,
            (struct sockaddr*)&ipv6SockAddr,
            &mmdb_error
            );
    }

    if (mmdb_error == 0 && mmdb_result.found_entry)
    {
        memset(&mmdb_entry, 0, sizeof(MMDB_entry_data_s));

        if (MMDB_get_value(&mmdb_result.entry, &mmdb_entry, "country", "iso_code", NULL) == MMDB_SUCCESS)
        {
            if (mmdb_entry.has_data && mmdb_entry.type == MMDB_DATA_TYPE_UTF8_STRING)
            {
                countryCode = PhConvertUtf8ToUtf16Ex((PCHAR)mmdb_entry.utf8_string, mmdb_entry.data_size);
            }
        }

        if (MMDB_get_value(&mmdb_result.entry, &mmdb_entry, "country", "names", "en", NULL) == MMDB_SUCCESS)
        {
            if (mmdb_entry.has_data && mmdb_entry.type == MMDB_DATA_TYPE_UTF8_STRING)
            {
                countryName = PhConvertUtf8ToUtf16Ex((PCHAR)mmdb_entry.utf8_string, mmdb_entry.data_size);
            }
        }

        if (MMDB_get_value(&mmdb_result.entry, &mmdb_entry, "location", "latitude", NULL) == MMDB_SUCCESS)
        {
            if (mmdb_entry.has_data && mmdb_entry.type == MMDB_DATA_TYPE_DOUBLE)
            {
                *CityLatitude = mmdb_entry.double_value;
            }
        }

        if (MMDB_get_value(&mmdb_result.entry, &mmdb_entry, "location", "longitude", NULL) == MMDB_SUCCESS)
        {
            if (mmdb_entry.has_data && mmdb_entry.type == MMDB_DATA_TYPE_DOUBLE)
            {
                *CityLongitude = mmdb_entry.double_value;
            }
        }
    }

    if (countryCode && countryName)
    {
        *CountryCode = countryCode;
        *CountryName = countryName;
        return TRUE;
    }

    if (countryCode)
    {
        PhDereferenceObject(countryCode);
    }

    if (countryName)
    {
        PhDereferenceObject(countryName);
    }

    return FALSE;
}

BOOLEAN LookupSockAddrCountryCode(
    _In_ IN_ADDR RemoteAddress,
    _Out_ PPH_STRING *CountryCode,
    _Out_ PPH_STRING *CountryName,
    _Out_opt_ DOUBLE *CityLatitude,
    _Out_opt_ DOUBLE *CityLongitude
    )
{
    PPH_STRING countryCode = NULL;
    PPH_STRING countryName = NULL;
    MMDB_entry_data_s mmdb_entry;
    MMDB_lookup_result_s mmdb_result;
    SOCKADDR_IN ipv4SockAddr;
    INT mmdb_error = 0;

    if (!GeoDbLoaded)
        return FALSE;

    if (IN4_IS_ADDR_UNSPECIFIED(&RemoteAddress))
        return FALSE;

    if (IN4_IS_ADDR_LOOPBACK(&RemoteAddress))
        return FALSE;

    memset(&ipv4SockAddr, 0, sizeof(SOCKADDR_IN));
    memset(&mmdb_result, 0, sizeof(MMDB_lookup_result_s));

    ipv4SockAddr.sin_family = AF_INET;
    ipv4SockAddr.sin_addr = RemoteAddress;

    mmdb_result = MMDB_lookup_sockaddr(
        &GeoDb,
        (PSOCKADDR)&ipv4SockAddr,
        &mmdb_error
        );

    if (mmdb_error == 0 && mmdb_result.found_entry)
    {
        memset(&mmdb_entry, 0, sizeof(MMDB_entry_data_s));

        if (MMDB_get_value(&mmdb_result.entry, &mmdb_entry, "country", "iso_code", NULL) == MMDB_SUCCESS)
        {
            if (mmdb_entry.has_data && mmdb_entry.type == MMDB_DATA_TYPE_UTF8_STRING)
            {
                countryCode = PhConvertUtf8ToUtf16Ex((PCHAR)mmdb_entry.utf8_string, mmdb_entry.data_size);
            }
        }

        if (MMDB_get_value(&mmdb_result.entry, &mmdb_entry, "country", "names", "en", NULL) == MMDB_SUCCESS)
        {
            if (mmdb_entry.has_data && mmdb_entry.type == MMDB_DATA_TYPE_UTF8_STRING)
            {
                countryName = PhConvertUtf8ToUtf16Ex((PCHAR)mmdb_entry.utf8_string, mmdb_entry.data_size);
            }
        }

        if (MMDB_get_value(&mmdb_result.entry, &mmdb_entry, "location", "latitude", NULL) == MMDB_SUCCESS)
        {
            if (mmdb_entry.has_data && mmdb_entry.type == MMDB_DATA_TYPE_DOUBLE)
            {
                *CityLatitude = mmdb_entry.double_value;
            }
        }

        if (MMDB_get_value(&mmdb_result.entry, &mmdb_entry, "location", "longitude", NULL) == MMDB_SUCCESS)
        {
            if (mmdb_entry.has_data && mmdb_entry.type == MMDB_DATA_TYPE_DOUBLE)
            {
                *CityLongitude = mmdb_entry.double_value;
            }
        }
    }

    if (countryCode && countryName)
    {
        *CountryCode = countryCode;
        *CountryName = countryName;
        return TRUE;
    }

    if (countryCode)
    {
        PhDereferenceObject(countryCode);
    }

    if (countryName)
    {
        PhDereferenceObject(countryName);
    }

    return FALSE;
}

struct
{
    PWSTR CountryCode;
    INT ResourceID;
}
CountryResourceTable[] =
{
    { L"AD", AD_PNG }, { L"AE", AE_PNG }, { L"AF", AF_PNG }, { L"AG", AG_PNG },
    { L"AI", AI_PNG }, { L"AL", AL_PNG }, { L"AM", AM_PNG }, { L"AN", AN_PNG },
    { L"AO", AO_PNG }, { L"AR", AR_PNG }, { L"AS", AS_PNG }, { L"AT", AT_PNG },
    { L"AU", AU_PNG }, { L"AW", AW_PNG }, { L"AX", AX_PNG }, { L"AZ", AZ_PNG },
    { L"BA", BA_PNG }, { L"BB", BB_PNG }, { L"BD", BD_PNG }, { L"BE", BE_PNG },
    { L"BF", BF_PNG }, { L"BG", BG_PNG }, { L"BH", BH_PNG }, { L"BI", BI__PNG },
    { L"BJ", BJ_PNG }, { L"BM", BM_PNG }, { L"BN", BN_PNG }, { L"BO", BO_PNG },
    { L"BR", BR_PNG }, { L"BS", BS_PNG }, { L"BT", BT_PNG }, { L"BV", BV_PNG },
    { L"BW", BW_PNG }, { L"BY", BY_PNG }, { L"BZ", BZ_PNG },
    { L"CA", CA_PNG }, { L"CC", CC_PNG }, { L"CD", CD_PNG }, { L"CF", CF_PNG },
    { L"CG", CG_PNG }, { L"CH", CH_PNG }, { L"CI", CI_PNG }, { L"CK", CK_PNG },
    { L"CL", CL_PNG }, { L"CM", CM_PNG }, { L"CN", CN_PNG }, { L"CO", CO_PNG },
    { L"CR", CR_PNG }, { L"CS", CS_PNG }, { L"CU", CU_PNG }, { L"CV", CV_PNG },
    { L"CX", CX_PNG }, { L"CY", CY_PNG }, { L"CZ", CZ_PNG }, { L"DE", DE_PNG },
    { L"DJ", DJ_PNG }, { L"DK", DK_PNG }, { L"DM", DM_PNG }, { L"DO", DO_PNG },
    { L"DZ", DZ_PNG },
    { L"EC", EC_PNG }, { L"EE", EE_PNG }, { L"EG", EG_PNG }, { L"EH", EH_PNG },
    { L"ER", ER_PNG }, { L"ES", ES_PNG }, { L"ET", ET_PNG },
    { L"FI", FI_PNG }, { L"FJ", FJ_PNG }, { L"FK", FK_PNG }, { L"FO", FO_PNG },
    { L"FR", FR_PNG },
    { L"GA", GA_PNG }, { L"GB", GB_PNG }, { L"GD", GD_PNG }, { L"GE", GE_PNG },
    { L"GF", GF_PNG }, { L"GH", GH_PNG }, { L"GI", GI_PNG }, { L"GL", GL_PNG },
    { L"GM", GM_PNG }, { L"GN", GN_PNG }, { L"GP", GP_PNG }, { L"GQ", GQ_PNG },
    { L"GR", GR_PNG }, { L"GS", GS_PNG }, { L"GT", GT_PNG }, { L"GU", GU_PNG },
    { L"GW", GW_PNG }, { L"GY", GY_PNG },
    { L"HK", HK_PNG }, { L"HM", HM_PNG }, { L"HN", HN_PNG }, { L"HR", HR_PNG },
    { L"HT", HT_PNG }, { L"HU", HU_PNG },
    { L"ID", ID_PNG }, { L"IE", IE_PNG }, { L"IL", IL_PNG }, { L"IN", IN_PNG },
    { L"IO", IO_PNG }, { L"IQ", IQ_PNG }, { L"IR", IR_PNG }, { L"IS", IS_PNG },
    { L"IT", IT_PNG },
    { L"JM", JM_PNG }, { L"JO", JO_PNG }, { L"JP", JP_PNG },
    { L"KE", KE_PNG }, { L"KG", KG_PNG }, { L"KH", KH_PNG }, { L"KI", KI_PNG },
    { L"KM", KM_PNG }, { L"KN", KN_PNG }, { L"KP", KP_PNG }, { L"KR", KR_PNG },
    { L"KW", KW_PNG }, { L"KY", KY_PNG }, { L"KZ", KZ_PNG },
    { L"LA", LA_PNG }, { L"LB", LB_PNG }, { L"LC", LC_PNG }, { L"LI", LI_PNG },
    { L"LK", LK_PNG }, { L"LR", LR_PNG }, { L"LS", LS_PNG }, { L"LT", LT_PNG },
    { L"LU", LU_PNG }, { L"LV", LV_PNG }, { L"LY", LY_PNG },
    { L"MA", MA_PNG }, { L"MC", MC_PNG }, { L"MD", MD_PNG }, { L"ME", ME_PNG },
    { L"MG", MG_PNG }, { L"MH", MH_PNG }, { L"MK", MK_PNG }, { L"ML", ML_PNG },
    { L"MM", MM_PNG }, { L"MN", MN_PNG }, { L"MO", MO_PNG }, { L"MP", MP_PNG },
    { L"MQ", MQ_PNG }, { L"MR", MR_PNG }, { L"MS", MS_PNG }, { L"MT", MT_PNG },
    { L"MU", MU_PNG }, { L"MV", MV_PNG }, { L"MW", MW_PNG }, { L"MX", MX_PNG },
    { L"MY", MY_PNG }, { L"MZ", MZ_PNG },
    { L"NA", NA_PNG }, { L"NC", NC_PNG }, { L"NE", NE_PNG }, { L"NF", NF_PNG },
    { L"NG", NG_PNG }, { L"NI", NI_PNG }, { L"NL", NL_PNG }, { L"NO", NO_PNG },
    { L"NP", NP_PNG }, { L"NR", NR_PNG }, { L"NU", NU_PNG }, { L"NZ", NZ_PNG },
    { L"OM", OM_PNG },
    { L"PA", PA_PNG }, { L"PE", PE_PNG }, { L"PF", PF_PNG }, { L"PG", PG_PNG },
    { L"PH", PH_PNG }, { L"PK", PK_PNG }, { L"PL", PL_PNG }, { L"PM", PM_PNG },
    { L"PN", PN_PNG }, { L"PR", PR_PNG }, { L"PS", PS_PNG }, { L"PT", PT_PNG },
    { L"PW", PW_PNG }, { L"PY", PY_PNG },
    { L"QA", QA_PNG },
    { L"RE", RE_PNG }, { L"RO", RO_PNG }, { L"RS", RS_PNG }, { L"RU", RU_PNG },
    { L"RW", RW_PNG },
    { L"SA", SA_PNG }, { L"SB", SB_PNG }, { L"SC", SC_PNG }, { L"SD", SD_PNG },
    { L"SE", SE_PNG }, { L"SG", SG_PNG }, { L"SH", SH_PNG }, { L"SI", SI_PNG },
    { L"SJ", SJ_PNG }, { L"SK", SK_PNG }, { L"SL", SL_PNG }, { L"SM", SM_PNG },
    { L"SN", SN_PNG }, { L"SO", SO_PNG }, { L"SR", SR_PNG }, { L"ST", ST_PNG },
    { L"SV", SV_PNG }, { L"SY", SY_PNG }, { L"SZ", SZ_PNG },
    { L"TC", TC_PNG }, { L"TD", TD_PNG }, { L"TF", TF_PNG }, { L"TG", TG_PNG },
    { L"TH", TH_PNG }, { L"TJ", TJ_PNG }, { L"TK", TK_PNG }, { L"TL", TL_PNG },
    { L"TM", TM_PNG }, { L"TN", TN_PNG }, { L"TO", TO_PNG }, { L"TR", TR_PNG },
    { L"TT", TT_PNG }, { L"TV", TV_PNG }, { L"TW", TW_PNG }, { L"TZ", TZ_PNG },
    { L"UA", UA_PNG }, { L"UG", UG_PNG }, { L"UM", UM_PNG }, { L"US", US_PNG },
    { L"UY", UY_PNG }, { L"UZ", UZ_PNG },
    { L"VA", VA_PNG }, { L"VC", VC_PNG }, { L"VE", VE_PNG }, { L"VG", VG_PNG },
    { L"VI", VI_PNG }, { L"VN", VN_PNG }, { L"VU", VU_PNG },
    { L"WF", WF_PNG }, { L"WS", WS_PNG },
    { L"YE", YE_PNG }, { L"YT", YT_PNG },
    { L"ZA", ZA_PNG }, { L"ZM", ZM_PNG }, { L"ZW", ZW_PNG }
};

INT LookupResourceCode(
    _In_ PPH_STRING Name
    )
{
    for (INT i = 0; i < ARRAYSIZE(CountryResourceTable); i++)
    {
        if (PhEqualString2(Name, CountryResourceTable[i].CountryCode, TRUE))
        {
            return CountryResourceTable[i].ResourceID;
        }
    }

    return 0;
}


VOID LookupGeoIpCurrentCity(
    _Out_ DOUBLE *CurrentLatitude, 
    _Out_ DOUBLE *CurrentLongitude
    )
{
    static PH_INITONCE initOnce = PH_INITONCE_INIT;
    static DOUBLE currentLatitude = 0.0;
    static DOUBLE currentLongitude = 0.0;

    if (PhBeginInitOnce(&initOnce))
    {
        HINTERNET httpSessionHandle = NULL;
        HINTERNET httpConnectionHandle = NULL;
        HINTERNET httpRequestHandle = NULL;
        WINHTTP_CURRENT_USER_IE_PROXY_CONFIG proxyConfig = { 0 };
        ULONG xmlStringBufferLength = 0;
        PSTR xmlStringBuffer = NULL;
        //PPH_STRING versionHeader = UpdateVersionString();
        //PPH_STRING windowsHeader = UpdateWindowsString();

        // Query the current system proxy
        WinHttpGetIEProxyConfigForCurrentUser(&proxyConfig);

        // Open the HTTP session
        if (!(httpSessionHandle = WinHttpOpen(
            NULL,
            proxyConfig.lpszProxy ? WINHTTP_ACCESS_TYPE_NAMED_PROXY : WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
            proxyConfig.lpszProxy,
            proxyConfig.lpszProxyBypass,
            0
        )))
        {
            goto CleanupExit;
        }

        if (WindowsVersion >= WINDOWS_8_1)
        {
            // Enable GZIP and DEFLATE support on Windows 8.1 and above using undocumented flags.
            ULONG httpFlags = WINHTTP_DECOMPRESSION_FLAG_GZIP | WINHTTP_DECOMPRESSION_FLAG_DEFLATE;

            WinHttpSetOption(
                httpSessionHandle,
                WINHTTP_OPTION_DECOMPRESSION,
                &httpFlags,
                sizeof(ULONG)
            );
        }

        if (!(httpConnectionHandle = WinHttpConnect(
            httpSessionHandle,
            L"wj32.org",
            INTERNET_DEFAULT_HTTPS_PORT,
            0
        )))
        {
            goto CleanupExit;
        }

        if (!(httpRequestHandle = WinHttpOpenRequest(
            httpConnectionHandle,
            NULL,
            L"/processhacker/fwlink/myaddr.php",
            NULL,
            WINHTTP_NO_REFERER,
            WINHTTP_DEFAULT_ACCEPT_TYPES,
            WINHTTP_FLAG_REFRESH | WINHTTP_FLAG_SECURE
        )))
        {
            goto CleanupExit;
        }

        if (WindowsVersion >= WINDOWS_7)
        {
            ULONG keepAlive = WINHTTP_DISABLE_KEEP_ALIVE;
            WinHttpSetOption(httpRequestHandle, WINHTTP_OPTION_DISABLE_FEATURE, &keepAlive, sizeof(ULONG));
        }

        //if (versionHeader)
        //{
        //    WinHttpAddRequestHeaders(
        //        httpRequestHandle,
        //        versionHeader->Buffer,
        //        (ULONG)versionHeader->Length / sizeof(WCHAR),
        //        WINHTTP_ADDREQ_FLAG_ADD
        //        );
        //}

        //if (windowsHeader)
        //{
        //    WinHttpAddRequestHeaders(
        //        httpRequestHandle,
        //        windowsHeader->Buffer,
        //        (ULONG)windowsHeader->Length / sizeof(WCHAR),
        //        WINHTTP_ADDREQ_FLAG_ADD
        //        );
        //}

        if (!WinHttpSendRequest(
            httpRequestHandle,
            WINHTTP_NO_ADDITIONAL_HEADERS,
            0,
            WINHTTP_NO_REQUEST_DATA,
            0,
            WINHTTP_IGNORE_REQUEST_TOTAL_LENGTH,
            0
        ))
        {
            goto CleanupExit;
        }

        if (!WinHttpReceiveResponse(httpRequestHandle, NULL))
            goto CleanupExit;

        PSTR data;
        ULONG allocatedLength;
        ULONG dataLength;
        ULONG returnLength;
        BYTE buffer[PAGE_SIZE];

        allocatedLength = sizeof(buffer);
        data = (PSTR)PhAllocate(allocatedLength);
        dataLength = 0;

        memset(buffer, 0, PAGE_SIZE);
        memset(data, 0, allocatedLength);

        while (WinHttpReadData(httpRequestHandle, buffer, PAGE_SIZE, &returnLength))
        {
            if (returnLength == 0)
                break;

            if (allocatedLength < dataLength + returnLength)
            {
                allocatedLength *= 2;
                data = (PSTR)PhReAllocate(data, allocatedLength);
            }

            memcpy(data + dataLength, buffer, returnLength);

            dataLength += returnLength;
        }

        if (allocatedLength < dataLength + 1)
        {
            allocatedLength++;
            data = (PSTR)PhReAllocate(data, allocatedLength);
        }

        data[dataLength] = 0;

        xmlStringBufferLength = dataLength;
        xmlStringBuffer = data;

        // Check the buffer for valid data.
        if (xmlStringBuffer == NULL || xmlStringBuffer[0] == '\0')
            goto CleanupExit;


        PH_IP_ENDPOINT remoteEndpoint = { 0 };
        PPH_STRING currentAddress = PhConvertUtf8ToUtf16Ex(xmlStringBuffer, xmlStringBufferLength);
        PWSTR terminator = NULL;

        if (NT_SUCCESS(RtlIpv4StringToAddress(currentAddress->Buffer, TRUE, &terminator, &remoteEndpoint.Address.InAddr)))
        {
            remoteEndpoint.Address.Type = PH_IPV4_NETWORK_TYPE;
        }

        if (NT_SUCCESS(RtlIpv6StringToAddress(currentAddress->Buffer, &terminator, &remoteEndpoint.Address.In6Addr)))
        {
            remoteEndpoint.Address.Type = PH_IPV6_NETWORK_TYPE;
        }

        PPH_STRING remoteCountryCode;
        PPH_STRING remoteCountryName;

        LookupCountryCode(
            remoteEndpoint.Address,
            &remoteCountryCode,
            &remoteCountryName,
            &currentLatitude,
            &currentLongitude
        );

    CleanupExit:
        if (httpRequestHandle)
            WinHttpCloseHandle(httpRequestHandle);

        if (httpConnectionHandle)
            WinHttpCloseHandle(httpConnectionHandle);

        if (httpSessionHandle)
            WinHttpCloseHandle(httpSessionHandle);

        if (xmlStringBuffer)
            PhFree(xmlStringBuffer);

        //PhClearReference(&versionHeader);
        //PhClearReference(&windowsHeader);

        PhEndInitOnce(&initOnce);
    }

    *CurrentLatitude = currentLatitude;
    *CurrentLongitude = currentLongitude;
}