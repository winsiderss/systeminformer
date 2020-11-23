/*
 * Process Hacker Network Tools  -
 *   IP Country support
 *
 * Copyright (C) 2016-2019 dmex
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
#include <commonutil.h>
#include "maxminddb\maxminddb.h"

BOOLEAN GeoDbLoaded = FALSE;
BOOLEAN GeoDbExpired = FALSE;
HIMAGELIST GeoImageList = NULL;
MMDB_s GeoDbCountry = { 0 };
PPH_HASHTABLE NetworkToolsGeoDbCacheHashtable = NULL;
PH_QUEUED_LOCK NetworkToolsGeoDbCacheHashtableLock = PH_QUEUED_LOCK_INIT;

typedef struct _GEODB_IPADDR_CACHE_ENTRY
{
    PH_IP_ADDRESS RemoteAddress;
    PPH_STRING CountryCode;
    PPH_STRING CountryName;
} GEODB_IPADDR_CACHE_ENTRY, *PGEODB_IPADDR_CACHE_ENTRY;

PPH_STRING NetToolsGetGeoLiteDbPath(
    _In_ PWSTR SettingName
    )
{
    PPH_STRING databaseFile;
    PPH_STRING directory;
    PPH_STRING path;

    if (!(databaseFile = PhGetExpandStringSetting(SettingName)))
        return NULL;

    PhMoveReference(&databaseFile, PhGetBaseName(databaseFile));

    directory = PH_AUTO(PhGetApplicationDirectory());
    path = PH_AUTO(PhConcatStringRef2(&directory->sr, &databaseFile->sr));

    if (PhDoesFileExistsWin32(path->Buffer))
    {
        return PhReferenceObject(path);
    }
    else
    {
        path = PhaGetStringSetting(SettingName);
        path = PH_AUTO(PhExpandEnvironmentStrings(&path->sr));

        if (PhDetermineDosPathNameType(path->Buffer) == RtlPathTypeRelative)
        {
            directory = PH_AUTO(PhGetApplicationDirectory());
            path = PH_AUTO(PhConcatStringRef2(&directory->sr, &path->sr));
        }

        return PhReferenceObject(path);
    }

    return NULL;
}

VOID LoadGeoLiteDb(
    VOID
    )
{
    PPH_STRING dbpath;

    if (dbpath = NetToolsGetGeoLiteDbPath(SETTING_NAME_DB_LOCATION))
    {
        if (MMDB_open(dbpath->Buffer, MMDB_MODE_MMAP, &GeoDbCountry) == MMDB_SUCCESS)
        {
            LARGE_INTEGER systemTime;
            ULONG secondsSince1970;

            // Query the current time
            PhQuerySystemTime(&systemTime);
            // Convert to unix epoch time
            RtlTimeToSecondsSince1970(&systemTime, &secondsSince1970);

            // Check if the Geoip database is older than 6 months (182 days = approx. 6 months).
            if ((secondsSince1970 - GeoDbCountry.metadata.build_epoch) > (182 * 24 * 60 * 60))
            {
                GeoDbExpired = TRUE;
            }

            GeoDbLoaded = TRUE;
        }

        if (GeoDbLoaded)
        {
            GeoImageList = ImageList_Create(16, 11, ILC_MASK | ILC_COLOR32, 20, 20);
        }

        PhDereferenceObject(dbpath);
    }
}

VOID FreeGeoLiteDb(
    VOID
    )
{
    if (GeoImageList)
    {
        ImageList_RemoveAll(GeoImageList);
        ImageList_Destroy(GeoImageList);
    }

    if (GeoDbLoaded)
    {
        MMDB_close(&GeoDbCountry);
    }
}

_Success_(return)
BOOLEAN GeoDbGetCityData(
    _In_ MMDB_entry_s* GeoDbEntry,
    _Out_ DOUBLE *CityLatitude,
    _Out_ DOUBLE *CityLongitude
    )
{
    MMDB_entry_data_s mmdb_entry;
    DOUBLE cityLatitude = 0.0;
    DOUBLE cityLongitude = 0.0;

    if (MMDB_get_value(GeoDbEntry, &mmdb_entry, "location", "latitude", NULL) == MMDB_SUCCESS)
    {
        if (mmdb_entry.has_data && mmdb_entry.type == MMDB_DATA_TYPE_DOUBLE)
        {
            cityLatitude = mmdb_entry.double_value;
        }
    }

    if (MMDB_get_value(GeoDbEntry, &mmdb_entry, "location", "longitude", NULL) == MMDB_SUCCESS)
    {
        if (mmdb_entry.has_data && mmdb_entry.type == MMDB_DATA_TYPE_DOUBLE)
        {
            cityLongitude = mmdb_entry.double_value;
        }
    }

    if (cityLatitude && cityLongitude)
    {
        *CityLatitude = cityLatitude;
        *CityLongitude = cityLongitude;
        return TRUE;
    }

    return FALSE;
}

_Success_(return)
BOOLEAN GeoDbGetCountryData(
    _In_ MMDB_entry_s* GeoDbEntry,
    _Out_ PPH_STRING *CountryCode,
    _Out_ PPH_STRING *CountryName
    )
{
    MMDB_entry_data_s mmdb_entry;
    PPH_STRING countryCode = NULL;
    PPH_STRING countryName = NULL;

    if (MMDB_get_value(GeoDbEntry, &mmdb_entry, "country", "iso_code", NULL) == MMDB_SUCCESS)
    {
        if (mmdb_entry.has_data && mmdb_entry.type == MMDB_DATA_TYPE_UTF8_STRING)
        {
            countryCode = PhConvertUtf8ToUtf16Ex((PCHAR)mmdb_entry.utf8_string, mmdb_entry.data_size);
        }
    }

    if (MMDB_get_value(GeoDbEntry, &mmdb_entry, "country", "names", "en", NULL) == MMDB_SUCCESS)
    {
        if (mmdb_entry.has_data && mmdb_entry.type == MMDB_DATA_TYPE_UTF8_STRING)
        {
            countryName = PhConvertUtf8ToUtf16Ex((PCHAR)mmdb_entry.utf8_string, mmdb_entry.data_size);
        }
    }

    if (countryCode && countryName)
    {
        *CountryCode = countryCode;
        *CountryName = countryName;
        return TRUE;
    }

    if (countryCode)
        PhDereferenceObject(countryCode);
    if (countryName)
        PhDereferenceObject(countryName);
    return FALSE;
}

_Success_(return)
BOOLEAN GeoDbGetContinentData(
    _In_ MMDB_entry_s* GeoDbEntry,
    _Out_ PPH_STRING *ContinentCode,
    _Out_ PPH_STRING *ContinentName
    )
{
    MMDB_entry_data_s mmdb_entry;
    PPH_STRING continentCode = NULL;
    PPH_STRING continentName = NULL;

    if (MMDB_get_value(GeoDbEntry, &mmdb_entry, "continent", "code", NULL) == MMDB_SUCCESS)
    {
        if (mmdb_entry.has_data && mmdb_entry.type == MMDB_DATA_TYPE_UTF8_STRING)
        {
            continentCode = PhConvertUtf8ToUtf16Ex((PCHAR)mmdb_entry.utf8_string, mmdb_entry.data_size);
        }
    }

    if (MMDB_get_value(GeoDbEntry, &mmdb_entry, "country", "names", "en", NULL) == MMDB_SUCCESS)
    {
        if (mmdb_entry.has_data && mmdb_entry.type == MMDB_DATA_TYPE_UTF8_STRING)
        {
            continentName = PhConvertUtf8ToUtf16Ex((PCHAR)mmdb_entry.utf8_string, mmdb_entry.data_size);
        }
    }

    if (continentCode && continentName)
    {
        *ContinentCode = continentCode;
        *ContinentName = continentName;
        return TRUE;
    }

    if (continentCode)
        PhDereferenceObject(continentCode);
    if (continentName)
        PhDereferenceObject(continentName);
    return FALSE;
}

_Success_(return)
BOOLEAN LookupCountryCodeFromMmdb(
    _In_ PH_IP_ADDRESS RemoteAddress,
    _Out_ PPH_STRING *CountryCode,
    _Out_ PPH_STRING *CountryName
    )
{
    MMDB_lookup_result_s mmdb_result;
    INT mmdb_error = 0;

    if (!GeoDbLoaded)
        return FALSE;

    if (RemoteAddress.Type == PH_IPV4_NETWORK_TYPE)
    {
        SOCKADDR_IN ipv4SockAddr;

        if (
            IN4_IS_ADDR_UNSPECIFIED(&RemoteAddress.InAddr) ||
            IN4_IS_ADDR_LOOPBACK(&RemoteAddress.InAddr) ||
            IN4_IS_ADDR_BROADCAST(&RemoteAddress.InAddr) ||
            IN4_IS_ADDR_MULTICAST(&RemoteAddress.InAddr) ||
            IN4_IS_ADDR_LINKLOCAL(&RemoteAddress.InAddr) ||
            IN4_IS_ADDR_MC_LINKLOCAL(&RemoteAddress.InAddr) ||
            IN4_IS_ADDR_RFC1918(&RemoteAddress.InAddr)
            )
        {
            return FALSE;
        }

        memset(&ipv4SockAddr, 0, sizeof(SOCKADDR_IN));
        memset(&mmdb_result, 0, sizeof(MMDB_lookup_result_s));

        ipv4SockAddr.sin_family = AF_INET;
        ipv4SockAddr.sin_addr = RemoteAddress.InAddr;

        mmdb_result = MMDB_lookup_sockaddr(
            &GeoDbCountry,
            (struct sockaddr*)&ipv4SockAddr,
            &mmdb_error
            );
    }
    else
    {
        SOCKADDR_IN6 ipv6SockAddr;

        if (
            IN6_IS_ADDR_UNSPECIFIED(&RemoteAddress.In6Addr) ||
            IN6_IS_ADDR_LOOPBACK(&RemoteAddress.In6Addr) ||
            IN6_IS_ADDR_MULTICAST(&RemoteAddress.In6Addr) ||
            IN6_IS_ADDR_LINKLOCAL(&RemoteAddress.In6Addr) ||
            IN6_IS_ADDR_MC_LINKLOCAL(&RemoteAddress.In6Addr)
            )
        {
            return FALSE;
        }

        memset(&ipv6SockAddr, 0, sizeof(SOCKADDR_IN6));
        memset(&mmdb_result, 0, sizeof(MMDB_lookup_result_s));

        ipv6SockAddr.sin6_family = AF_INET6;
        ipv6SockAddr.sin6_addr = RemoteAddress.In6Addr;

        mmdb_result = MMDB_lookup_sockaddr(
            &GeoDbCountry,
            (struct sockaddr*)&ipv6SockAddr,
            &mmdb_error
            );
    }

    if (mmdb_error == 0 && mmdb_result.found_entry)
    {
        if (GeoDbGetCountryData(&mmdb_result.entry, CountryCode, CountryName))
            return TRUE;
        if (GeoDbGetContinentData(&mmdb_result.entry, CountryCode, CountryName))
            return TRUE;
    }

    return FALSE;
}

_Success_(return)
BOOLEAN LookupSockInAddr4CountryCode(
    _In_ IN_ADDR RemoteAddress,
    _Out_ PPH_STRING *CountryCode,
    _Out_ PPH_STRING *CountryName
    )
{
    MMDB_lookup_result_s mmdb_result;
    SOCKADDR_IN ipv4SockAddr;
    INT mmdb_error = 0;

    if (!GeoDbLoaded)
        return FALSE;

    if (
        IN4_IS_ADDR_UNSPECIFIED(&RemoteAddress) ||
        IN4_IS_ADDR_LOOPBACK(&RemoteAddress) ||
        IN4_IS_ADDR_BROADCAST(&RemoteAddress) ||
        IN4_IS_ADDR_MULTICAST(&RemoteAddress) ||
        IN4_IS_ADDR_LINKLOCAL(&RemoteAddress) ||
        IN4_IS_ADDR_MC_LINKLOCAL(&RemoteAddress) ||
        IN4_IS_ADDR_RFC1918(&RemoteAddress)
        )
    {
        return FALSE;
    }

    memset(&ipv4SockAddr, 0, sizeof(SOCKADDR_IN));
    memset(&mmdb_result, 0, sizeof(MMDB_lookup_result_s));

    ipv4SockAddr.sin_family = AF_INET;
    ipv4SockAddr.sin_addr = RemoteAddress;

    mmdb_result = MMDB_lookup_sockaddr(
        &GeoDbCountry,
        (PSOCKADDR)&ipv4SockAddr,
        &mmdb_error
        );

    if (mmdb_error == 0 && mmdb_result.found_entry)
    {
        if (GeoDbGetCountryData(&mmdb_result.entry, CountryCode, CountryName))
            return TRUE;
        if (GeoDbGetContinentData(&mmdb_result.entry, CountryCode, CountryName))
            return TRUE;
    }

    return FALSE;
}

_Success_(return)
BOOLEAN LookupSockInAddr6CountryCode(
    _In_ IN6_ADDR RemoteAddress,
    _Out_ PPH_STRING *CountryCode,
    _Out_ PPH_STRING *CountryName
    )
{
    MMDB_lookup_result_s mmdb_result;
    SOCKADDR_IN6 ipv6SockAddr;
    INT mmdb_error = 0;

    if (!GeoDbLoaded)
        return FALSE;

    if (
        IN6_IS_ADDR_UNSPECIFIED(&RemoteAddress) ||
        IN6_IS_ADDR_LOOPBACK(&RemoteAddress) ||
        IN6_IS_ADDR_MULTICAST(&RemoteAddress) ||
        IN6_IS_ADDR_LINKLOCAL(&RemoteAddress) ||
        IN6_IS_ADDR_MC_LINKLOCAL(&RemoteAddress)
        )
    {
        return FALSE;
    }

    memset(&ipv6SockAddr, 0, sizeof(SOCKADDR_IN6));
    memset(&mmdb_result, 0, sizeof(MMDB_lookup_result_s));

    ipv6SockAddr.sin6_family = AF_INET6;
    ipv6SockAddr.sin6_addr = RemoteAddress;

    mmdb_result = MMDB_lookup_sockaddr(
        &GeoDbCountry,
        (PSOCKADDR)&ipv6SockAddr,
        &mmdb_error
        );

    if (mmdb_error == 0 && mmdb_result.found_entry)
    {
        if (GeoDbGetCountryData(&mmdb_result.entry, CountryCode, CountryName))
            return TRUE;
        if (GeoDbGetContinentData(&mmdb_result.entry, CountryCode, CountryName))
            return TRUE;
    }

    return FALSE;
}

struct
{
    PWSTR CountryCode;
    INT ResourceID;
    INT IconIndex;
}
CountryResourceTable[] =
{
    { L"AD", AD_PNG, INT_MAX }, { L"AE", AE_PNG, INT_MAX }, { L"AF", AF_PNG, INT_MAX }, { L"AG", AG_PNG, INT_MAX },
    { L"AI", AI_PNG, INT_MAX }, { L"AL", AL_PNG, INT_MAX }, { L"AM", AM_PNG, INT_MAX }, { L"AN", AN_PNG, INT_MAX },
    { L"AO", AO_PNG, INT_MAX }, { L"AR", AR_PNG, INT_MAX }, { L"AS", AS_PNG, INT_MAX }, { L"AT", AT_PNG, INT_MAX },
    { L"AU", AU_PNG, INT_MAX }, { L"AW", AW_PNG, INT_MAX }, { L"AX", AX_PNG, INT_MAX }, { L"AZ", AZ_PNG, INT_MAX },
    { L"BA", BA_PNG, INT_MAX }, { L"BB", BB_PNG, INT_MAX }, { L"BD", BD_PNG, INT_MAX }, { L"BE", BE_PNG, INT_MAX },
    { L"BF", BF_PNG, INT_MAX }, { L"BG", BG_PNG, INT_MAX }, { L"BH", BH_PNG, INT_MAX }, { L"BI", BI__PNG, INT_MAX },
    { L"BJ", BJ_PNG, INT_MAX }, { L"BM", BM_PNG, INT_MAX }, { L"BN", BN_PNG, INT_MAX }, { L"BO", BO_PNG, INT_MAX },
    { L"BR", BR_PNG, INT_MAX }, { L"BS", BS_PNG, INT_MAX }, { L"BT", BT_PNG, INT_MAX }, { L"BV", BV_PNG, INT_MAX },
    { L"BW", BW_PNG, INT_MAX }, { L"BY", BY_PNG, INT_MAX }, { L"BZ", BZ_PNG, INT_MAX },
    { L"CA", CA_PNG, INT_MAX }, { L"CC", CC_PNG, INT_MAX }, { L"CD", CD_PNG, INT_MAX }, { L"CF", CF_PNG, INT_MAX },
    { L"CG", CG_PNG, INT_MAX }, { L"CH", CH_PNG, INT_MAX }, { L"CI", CI_PNG, INT_MAX }, { L"CK", CK_PNG, INT_MAX },
    { L"CL", CL_PNG, INT_MAX }, { L"CM", CM_PNG, INT_MAX }, { L"CN", CN_PNG, INT_MAX }, { L"CO", CO_PNG, INT_MAX },
    { L"CR", CR_PNG, INT_MAX }, { L"CS", CS_PNG, INT_MAX }, { L"CU", CU_PNG, INT_MAX }, { L"CV", CV_PNG, INT_MAX },
    { L"CX", CX_PNG, INT_MAX }, { L"CY", CY_PNG, INT_MAX }, { L"CZ", CZ_PNG, INT_MAX }, { L"DE", DE_PNG, INT_MAX },
    { L"DJ", DJ_PNG, INT_MAX }, { L"DK", DK_PNG, INT_MAX }, { L"DM", DM_PNG, INT_MAX }, { L"DO", DO_PNG, INT_MAX },
    { L"DZ", DZ_PNG, INT_MAX },
    { L"EC", EC_PNG, INT_MAX }, { L"EE", EE_PNG, INT_MAX }, { L"EG", EG_PNG, INT_MAX }, { L"EH", EH_PNG, INT_MAX },
    { L"ER", ER_PNG, INT_MAX }, { L"ES", ES_PNG, INT_MAX }, { L"ET", ET_PNG, INT_MAX },
    { L"FI", FI_PNG, INT_MAX }, { L"FJ", FJ_PNG, INT_MAX }, { L"FK", FK_PNG, INT_MAX }, { L"FO", FO_PNG, INT_MAX },
    { L"FR", FR_PNG, INT_MAX },
    { L"GA", GA_PNG, INT_MAX }, { L"GB", GB_PNG, INT_MAX }, { L"GD", GD_PNG, INT_MAX }, { L"GE", GE_PNG, INT_MAX },
    { L"GF", GF_PNG, INT_MAX }, { L"GH", GH_PNG, INT_MAX }, { L"GI", GI_PNG, INT_MAX }, { L"GL", GL_PNG, INT_MAX },
    { L"GM", GM_PNG, INT_MAX }, { L"GN", GN_PNG, INT_MAX }, { L"GP", GP_PNG, INT_MAX }, { L"GQ", GQ_PNG, INT_MAX },
    { L"GR", GR_PNG, INT_MAX }, { L"GS", GS_PNG, INT_MAX }, { L"GT", GT_PNG, INT_MAX }, { L"GU", GU_PNG, INT_MAX },
    { L"GW", GW_PNG, INT_MAX }, { L"GY", GY_PNG, INT_MAX },
    { L"HK", HK_PNG, INT_MAX }, { L"HM", HM_PNG, INT_MAX }, { L"HN", HN_PNG, INT_MAX }, { L"HR", HR_PNG, INT_MAX },
    { L"HT", HT_PNG, INT_MAX }, { L"HU", HU_PNG, INT_MAX },
    { L"ID", ID_PNG, INT_MAX }, { L"IE", IE_PNG, INT_MAX }, { L"IL", IL_PNG, INT_MAX }, { L"IN", IN_PNG, INT_MAX },
    { L"IO", IO_PNG, INT_MAX }, { L"IQ", IQ_PNG, INT_MAX }, { L"IR", IR_PNG, INT_MAX }, { L"IS", IS_PNG, INT_MAX },
    { L"IT", IT_PNG, INT_MAX },
    { L"JM", JM_PNG, INT_MAX }, { L"JO", JO_PNG, INT_MAX }, { L"JP", JP_PNG, INT_MAX },
    { L"KE", KE_PNG, INT_MAX }, { L"KG", KG_PNG, INT_MAX }, { L"KH", KH_PNG, INT_MAX }, { L"KI", KI_PNG, INT_MAX },
    { L"KM", KM_PNG, INT_MAX }, { L"KN", KN_PNG, INT_MAX }, { L"KP", KP_PNG, INT_MAX }, { L"KR", KR_PNG, INT_MAX },
    { L"KW", KW_PNG, INT_MAX }, { L"KY", KY_PNG, INT_MAX }, { L"KZ", KZ_PNG, INT_MAX },
    { L"LA", LA_PNG, INT_MAX }, { L"LB", LB_PNG, INT_MAX }, { L"LC", LC_PNG, INT_MAX }, { L"LI", LI_PNG, INT_MAX },
    { L"LK", LK_PNG, INT_MAX }, { L"LR", LR_PNG, INT_MAX }, { L"LS", LS_PNG, INT_MAX }, { L"LT", LT_PNG, INT_MAX },
    { L"LU", LU_PNG, INT_MAX }, { L"LV", LV_PNG, INT_MAX }, { L"LY", LY_PNG, INT_MAX },
    { L"MA", MA_PNG, INT_MAX }, { L"MC", MC_PNG, INT_MAX }, { L"MD", MD_PNG, INT_MAX }, { L"ME", ME_PNG, INT_MAX },
    { L"MG", MG_PNG, INT_MAX }, { L"MH", MH_PNG, INT_MAX }, { L"MK", MK_PNG, INT_MAX }, { L"ML", ML_PNG, INT_MAX },
    { L"MM", MM_PNG, INT_MAX }, { L"MN", MN_PNG, INT_MAX }, { L"MO", MO_PNG, INT_MAX }, { L"MP", MP_PNG, INT_MAX },
    { L"MQ", MQ_PNG, INT_MAX }, { L"MR", MR_PNG, INT_MAX }, { L"MS", MS_PNG, INT_MAX }, { L"MT", MT_PNG, INT_MAX },
    { L"MU", MU_PNG, INT_MAX }, { L"MV", MV_PNG, INT_MAX }, { L"MW", MW_PNG, INT_MAX }, { L"MX", MX_PNG, INT_MAX },
    { L"MY", MY_PNG, INT_MAX }, { L"MZ", MZ_PNG, INT_MAX },
    { L"NA", NA_PNG, INT_MAX }, { L"NC", NC_PNG, INT_MAX }, { L"NE", NE_PNG, INT_MAX }, { L"NF", NF_PNG, INT_MAX },
    { L"NG", NG_PNG, INT_MAX }, { L"NI", NI_PNG, INT_MAX }, { L"NL", NL_PNG, INT_MAX }, { L"NO", NO_PNG, INT_MAX },
    { L"NP", NP_PNG, INT_MAX }, { L"NR", NR_PNG, INT_MAX }, { L"NU", NU_PNG, INT_MAX }, { L"NZ", NZ_PNG, INT_MAX },
    { L"OM", OM_PNG, INT_MAX },
    { L"PA", PA_PNG, INT_MAX }, { L"PE", PE_PNG, INT_MAX }, { L"PF", PF_PNG, INT_MAX }, { L"PG", PG_PNG, INT_MAX },
    { L"PH", PH_PNG, INT_MAX }, { L"PK", PK_PNG, INT_MAX }, { L"PL", PL_PNG, INT_MAX }, { L"PM", PM_PNG, INT_MAX },
    { L"PN", PN_PNG, INT_MAX }, { L"PR", PR_PNG, INT_MAX }, { L"PS", PS_PNG, INT_MAX }, { L"PT", PT_PNG, INT_MAX },
    { L"PW", PW_PNG, INT_MAX }, { L"PY", PY_PNG, INT_MAX },
    { L"QA", QA_PNG, INT_MAX },
    { L"RE", RE_PNG, INT_MAX }, { L"RO", RO_PNG, INT_MAX }, { L"RS", RS_PNG, INT_MAX }, { L"RU", RU_PNG, INT_MAX },
    { L"RW", RW_PNG, INT_MAX },
    { L"SA", SA_PNG, INT_MAX }, { L"SB", SB_PNG, INT_MAX }, { L"SC", SC_PNG, INT_MAX }, { L"SD", SD_PNG, INT_MAX },
    { L"SE", SE_PNG, INT_MAX }, { L"SG", SG_PNG, INT_MAX }, { L"SH", SH_PNG, INT_MAX }, { L"SI", SI_PNG, INT_MAX },
    { L"SJ", SJ_PNG, INT_MAX }, { L"SK", SK_PNG, INT_MAX }, { L"SL", SL_PNG, INT_MAX }, { L"SM", SM_PNG, INT_MAX },
    { L"SN", SN_PNG, INT_MAX }, { L"SO", SO_PNG, INT_MAX }, { L"SR", SR_PNG, INT_MAX }, { L"ST", ST_PNG, INT_MAX },
    { L"SV", SV_PNG, INT_MAX }, { L"SY", SY_PNG, INT_MAX }, { L"SZ", SZ_PNG, INT_MAX },
    { L"TC", TC_PNG, INT_MAX }, { L"TD", TD_PNG, INT_MAX }, { L"TF", TF_PNG, INT_MAX }, { L"TG", TG_PNG, INT_MAX },
    { L"TH", TH_PNG, INT_MAX }, { L"TJ", TJ_PNG, INT_MAX }, { L"TK", TK_PNG, INT_MAX }, { L"TL", TL_PNG, INT_MAX },
    { L"TM", TM_PNG, INT_MAX }, { L"TN", TN_PNG, INT_MAX }, { L"TO", TO_PNG, INT_MAX }, { L"TR", TR_PNG, INT_MAX },
    { L"TT", TT_PNG, INT_MAX }, { L"TV", TV_PNG, INT_MAX }, { L"TW", TW_PNG, INT_MAX }, { L"TZ", TZ_PNG, INT_MAX },
    { L"UA", UA_PNG, INT_MAX }, { L"UG", UG_PNG, INT_MAX }, { L"UM", UM_PNG, INT_MAX }, { L"US", US_PNG, INT_MAX },
    { L"UY", UY_PNG, INT_MAX }, { L"UZ", UZ_PNG, INT_MAX },
    { L"VA", VA_PNG, INT_MAX }, { L"VC", VC_PNG, INT_MAX }, { L"VE", VE_PNG, INT_MAX }, { L"VG", VG_PNG, INT_MAX },
    { L"VI", VI_PNG, INT_MAX }, { L"VN", VN_PNG, INT_MAX }, { L"VU", VU_PNG, INT_MAX },
    { L"WF", WF_PNG, INT_MAX }, { L"WS", WS_PNG, INT_MAX },
    { L"YE", YE_PNG, INT_MAX }, { L"YT", YT_PNG, INT_MAX },
    { L"ZA", ZA_PNG, INT_MAX }, { L"ZM", ZM_PNG, INT_MAX }, { L"ZW", ZW_PNG, INT_MAX }
};

INT LookupCountryIcon(
    _In_ PPH_STRING Name
    )
{
    if (!GeoImageList)
        return INT_MAX;

    for (INT i = 0; i < ARRAYSIZE(CountryResourceTable); i++)
    {
        if (PhEqualString2(Name, CountryResourceTable[i].CountryCode, TRUE))
        {
            if (CountryResourceTable[i].IconIndex == INT_MAX)
            {
                HBITMAP countryBitmap;

                if (countryBitmap = PhLoadPngImageFromResource(
                    PluginInstance->DllBase,
                    16,
                    11,
                    MAKEINTRESOURCE(CountryResourceTable[i].ResourceID),
                    TRUE
                    ))
                {
                    CountryResourceTable[i].IconIndex = ImageList_Add(
                        GeoImageList, 
                        countryBitmap, 
                        NULL
                        );
                    DeleteBitmap(countryBitmap);
                }
            }

            return CountryResourceTable[i].IconIndex;
        }
    }

    return INT_MAX;
}

VOID DrawCountryIcon(
    _In_ HDC hdc, 
    _In_ RECT rect, 
    _In_ INT Index
    )
{
    if (!GeoImageList)
        return;

    ImageList_Draw(
        GeoImageList, 
        Index, 
        hdc,
        rect.left, 
        rect.top + ((rect.bottom - rect.top) - 11) / 2, 
        ILD_NORMAL
        );
}

BOOLEAN NetworkToolsGeoDbCacheHashtableEqualFunction(
    _In_ PVOID Entry1,
    _In_ PVOID Entry2
    )
{
    PGEODB_IPADDR_CACHE_ENTRY entry1 = Entry1;
    PGEODB_IPADDR_CACHE_ENTRY entry2 = Entry2;

    if (entry1->RemoteAddress.Type == PH_IPV4_NETWORK_TYPE && entry2->RemoteAddress.Type == PH_IPV4_NETWORK_TYPE)
    {
        return IN4_ADDR_EQUAL(&entry1->RemoteAddress.InAddr, &entry2->RemoteAddress.InAddr);
    }
    else  if (entry1->RemoteAddress.Type == PH_IPV6_NETWORK_TYPE && entry2->RemoteAddress.Type == PH_IPV6_NETWORK_TYPE)
    {
        return IN6_ADDR_EQUAL(&entry1->RemoteAddress.In6Addr, &entry2->RemoteAddress.In6Addr);
    }

    return FALSE;
}

ULONG NetworkToolsGeoDbCacheHashtableHashFunction(
    _In_ PVOID Entry
    )
{
    PGEODB_IPADDR_CACHE_ENTRY entry = Entry;

    return PhHashIpAddress(&entry->RemoteAddress);
}

_Success_(return)
BOOLEAN LookupCountryCode(
    _In_ PH_IP_ADDRESS RemoteAddress,
    _Out_ PPH_STRING* CountryCode,
    _Out_ PPH_STRING* CountryName
    )
{
    PPH_STRING countryCode = NULL;
    PPH_STRING countryName = NULL;
    GEODB_IPADDR_CACHE_ENTRY newEntry;

    PhAcquireQueuedLockShared(&NetworkToolsGeoDbCacheHashtableLock);

    if (NetworkToolsGeoDbCacheHashtable)
    {
        PGEODB_IPADDR_CACHE_ENTRY entry;
        GEODB_IPADDR_CACHE_ENTRY lookupEntry;

        lookupEntry.RemoteAddress = RemoteAddress;
        entry = PhFindEntryHashtable(NetworkToolsGeoDbCacheHashtable, &lookupEntry);

        if (entry)
        {
            if (CountryCode)
                *CountryCode = PhReferenceObject(entry->CountryCode);
            if (CountryName)
                *CountryName = PhReferenceObject(entry->CountryName);

            PhReleaseQueuedLockShared(&NetworkToolsGeoDbCacheHashtableLock);

            return TRUE;
        }
    }

    PhReleaseQueuedLockShared(&NetworkToolsGeoDbCacheHashtableLock);

    if (!LookupCountryCodeFromMmdb(RemoteAddress, &countryCode, &countryName))
        return FALSE;

    PhAcquireQueuedLockExclusive(&NetworkToolsGeoDbCacheHashtableLock);

    if (!NetworkToolsGeoDbCacheHashtable)
    {
        NetworkToolsGeoDbCacheHashtable = PhCreateHashtable(
            sizeof(GEODB_IPADDR_CACHE_ENTRY),
            NetworkToolsGeoDbCacheHashtableEqualFunction,
            NetworkToolsGeoDbCacheHashtableHashFunction,
            32
            );
    }
  
    newEntry.CountryCode = countryCode;
    newEntry.CountryName = countryName;
    memcpy_s(&newEntry.RemoteAddress, sizeof(newEntry.RemoteAddress), &RemoteAddress, sizeof(PH_IP_ADDRESS));
    PhAddEntryHashtable(NetworkToolsGeoDbCacheHashtable, &newEntry);

    if (CountryCode)
        *CountryCode = PhReferenceObject(countryCode);
    if (CountryName)
        *CountryName = PhReferenceObject(countryName);

    PhReleaseQueuedLockExclusive(&NetworkToolsGeoDbCacheHashtableLock);

    return TRUE;
}

VOID NetworkToolsGeoDbFlushCache(
    VOID
    )
{
    PH_HASHTABLE_ENUM_CONTEXT enumContext;
    PGEODB_IPADDR_CACHE_ENTRY entry;

    if (!NetworkToolsGeoDbCacheHashtable)
        return;

    PhAcquireQueuedLockExclusive(&NetworkToolsGeoDbCacheHashtableLock);

    PhBeginEnumHashtable(NetworkToolsGeoDbCacheHashtable, &enumContext);

    while (entry = PhNextEnumHashtable(&enumContext))
    {
        PhDereferenceObject(entry->CountryCode);
        PhDereferenceObject(entry->CountryName);
    }

    PhClearReference(&NetworkToolsGeoDbCacheHashtable);
    NetworkToolsGeoDbCacheHashtable = PhCreateHashtable(
        sizeof(GEODB_IPADDR_CACHE_ENTRY),
        NetworkToolsGeoDbCacheHashtableEqualFunction,
        NetworkToolsGeoDbCacheHashtableHashFunction,
        32
        );

    PhReleaseQueuedLockExclusive(&NetworkToolsGeoDbCacheHashtableLock);
}
