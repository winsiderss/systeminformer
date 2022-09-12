/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     dmex    2016-2022
 *
 */

#include "nettools.h"
#include <commonutil.h>
#include "..\..\tools\thirdparty\maxminddb\maxminddb.h"

BOOLEAN GeoDbInitialized = FALSE;
BOOLEAN GeoDbExpired = FALSE;
HIMAGELIST GeoImageList = NULL;
MMDB_s GeoDbCountry = { 0 };
PH_STRINGREF GeoDbFileName = PH_STRINGREF_INIT(L"GeoLite2-Country.mmdb");
PPH_HASHTABLE NetworkToolsGeoDbCacheHashtable = NULL;
PH_QUEUED_LOCK NetworkToolsGeoDbCacheHashtableLock = PH_QUEUED_LOCK_INIT;

typedef struct _GEODB_IPADDR_CACHE_ENTRY
{
    PH_IP_ADDRESS RemoteAddress;
    PPH_STRING CountryCode;
    PPH_STRING CountryName;
} GEODB_IPADDR_CACHE_ENTRY, *PGEODB_IPADDR_CACHE_ENTRY;

BOOLEAN NetToolsGeoLiteInitialized(
    VOID
    )
{
    static PH_INITONCE initOnce = PH_INITONCE_INIT;

    if (PhBeginInitOnce(&initOnce))
    {
        PPH_STRING dbpath;

        if (dbpath = PhGetApplicationDataFileName(&GeoDbFileName, FALSE))
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

                GeoImageList = PhImageListCreate(
                    16,
                    11,
                    ILC_MASK | ILC_COLOR32,
                    20,
                    20
                    );

                GeoDbInitialized = TRUE;
            }

            PhDereferenceObject(dbpath);
        }

        PhEndInitOnce(&initOnce);
    }

    return GeoDbInitialized;
}

VOID FreeGeoLiteDb(
    VOID
    )
{
    if (GeoImageList)
    {
        PhImageListRemoveAll(GeoImageList);
        PhImageListDestroy(GeoImageList);
    }

    if (GeoDbInitialized)
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

    if (!NetToolsGeoLiteInitialized())
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

    if (!NetToolsGeoLiteInitialized())
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

    if (!NetToolsGeoLiteInitialized())
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
    PH_STRINGREF CountryCode;
    INT ResourceID;
    INT IconIndex;
}
CountryResourceTable[] =
{
    { PH_STRINGREF_INIT(L"AD"), AD_PNG, INT_MAX }, { PH_STRINGREF_INIT(L"AE"), AE_PNG, INT_MAX },
    { PH_STRINGREF_INIT(L"AF"), AF_PNG, INT_MAX }, { PH_STRINGREF_INIT(L"AG"), AG_PNG, INT_MAX },
    { PH_STRINGREF_INIT(L"AI"), AI_PNG, INT_MAX }, { PH_STRINGREF_INIT(L"AL"), AL_PNG, INT_MAX },
    { PH_STRINGREF_INIT(L"AM"), AM_PNG, INT_MAX }, { PH_STRINGREF_INIT(L"AN"), AN_PNG, INT_MAX },
    { PH_STRINGREF_INIT(L"AO"), AO_PNG, INT_MAX }, { PH_STRINGREF_INIT(L"AR"), AR_PNG, INT_MAX },
    { PH_STRINGREF_INIT(L"AS"), AS_PNG, INT_MAX }, { PH_STRINGREF_INIT(L"AT"), AT_PNG, INT_MAX },
    { PH_STRINGREF_INIT(L"AU"), AU_PNG, INT_MAX }, { PH_STRINGREF_INIT(L"AW"), AW_PNG, INT_MAX },
    { PH_STRINGREF_INIT(L"AX"), AX_PNG, INT_MAX }, { PH_STRINGREF_INIT(L"AZ"), AZ_PNG, INT_MAX },
    { PH_STRINGREF_INIT(L"BA"), BA_PNG, INT_MAX }, { PH_STRINGREF_INIT(L"BB"), BB_PNG, INT_MAX },
    { PH_STRINGREF_INIT(L"BD"), BD_PNG, INT_MAX }, { PH_STRINGREF_INIT(L"BE"), BE_PNG, INT_MAX },
    { PH_STRINGREF_INIT(L"BF"), BF_PNG, INT_MAX }, { PH_STRINGREF_INIT(L"BG"), BG_PNG, INT_MAX },
    { PH_STRINGREF_INIT(L"BH"), BH_PNG, INT_MAX }, { PH_STRINGREF_INIT(L"BI"), BI__PNG, INT_MAX },
    { PH_STRINGREF_INIT(L"BJ"), BJ_PNG, INT_MAX }, { PH_STRINGREF_INIT(L"BM"), BM_PNG, INT_MAX },
    { PH_STRINGREF_INIT(L"BN"), BN_PNG, INT_MAX }, { PH_STRINGREF_INIT(L"BO"), BO_PNG, INT_MAX },
    { PH_STRINGREF_INIT(L"BR"), BR_PNG, INT_MAX }, { PH_STRINGREF_INIT(L"BS"), BS_PNG, INT_MAX },
    { PH_STRINGREF_INIT(L"BT"), BT_PNG, INT_MAX }, { PH_STRINGREF_INIT(L"BV"), BV_PNG, INT_MAX },
    { PH_STRINGREF_INIT(L"BW"), BW_PNG, INT_MAX }, { PH_STRINGREF_INIT(L"BY"), BY_PNG, INT_MAX },
    { PH_STRINGREF_INIT(L"BZ"), BZ_PNG, INT_MAX },
    { PH_STRINGREF_INIT(L"CA"), CA_PNG, INT_MAX }, { PH_STRINGREF_INIT(L"CC"), CC_PNG, INT_MAX },
    { PH_STRINGREF_INIT(L"CD"), CD_PNG, INT_MAX }, { PH_STRINGREF_INIT(L"CF"), CF_PNG, INT_MAX },
    { PH_STRINGREF_INIT(L"CG"), CG_PNG, INT_MAX }, { PH_STRINGREF_INIT(L"CH"), CH_PNG, INT_MAX },
    { PH_STRINGREF_INIT(L"CI"), CI_PNG, INT_MAX }, { PH_STRINGREF_INIT(L"CK"), CK_PNG, INT_MAX },
    { PH_STRINGREF_INIT(L"CL"), CL_PNG, INT_MAX }, { PH_STRINGREF_INIT(L"CM"), CM_PNG, INT_MAX },
    { PH_STRINGREF_INIT(L"CN"), CN_PNG, INT_MAX }, { PH_STRINGREF_INIT(L"CO"), CO_PNG, INT_MAX },
    { PH_STRINGREF_INIT(L"CR"), CR_PNG, INT_MAX }, { PH_STRINGREF_INIT(L"CS"), CS_PNG, INT_MAX },
    { PH_STRINGREF_INIT(L"CU"), CU_PNG, INT_MAX }, { PH_STRINGREF_INIT(L"CV"), CV_PNG, INT_MAX },
    { PH_STRINGREF_INIT(L"CX"), CX_PNG, INT_MAX }, { PH_STRINGREF_INIT(L"CY"), CY_PNG, INT_MAX },
    { PH_STRINGREF_INIT(L"CZ"), CZ_PNG, INT_MAX }, { PH_STRINGREF_INIT(L"DE"), DE_PNG, INT_MAX },
    { PH_STRINGREF_INIT(L"DJ"), DJ_PNG, INT_MAX }, { PH_STRINGREF_INIT(L"DK"), DK_PNG, INT_MAX },
    { PH_STRINGREF_INIT(L"DM"), DM_PNG, INT_MAX }, { PH_STRINGREF_INIT(L"DO"), DO_PNG, INT_MAX },
    { PH_STRINGREF_INIT(L"DZ"), DZ_PNG, INT_MAX },
    { PH_STRINGREF_INIT(L"EC"), EC_PNG, INT_MAX }, { PH_STRINGREF_INIT(L"EE"), EE_PNG, INT_MAX },
    { PH_STRINGREF_INIT(L"EG"), EG_PNG, INT_MAX }, { PH_STRINGREF_INIT(L"EH"), EH_PNG, INT_MAX },
    { PH_STRINGREF_INIT(L"ER"), ER_PNG, INT_MAX }, { PH_STRINGREF_INIT(L"ES"), ES_PNG, INT_MAX },
    { PH_STRINGREF_INIT(L"ET"), ET_PNG, INT_MAX },
    { PH_STRINGREF_INIT(L"FI"), FI_PNG, INT_MAX }, { PH_STRINGREF_INIT(L"FJ"), FJ_PNG, INT_MAX },
    { PH_STRINGREF_INIT(L"FK"), FK_PNG, INT_MAX }, { PH_STRINGREF_INIT(L"FO"), FO_PNG, INT_MAX },
    { PH_STRINGREF_INIT(L"FR"), FR_PNG, INT_MAX },
    { PH_STRINGREF_INIT(L"GA"), GA_PNG, INT_MAX }, { PH_STRINGREF_INIT(L"GB"), GB_PNG, INT_MAX },
    { PH_STRINGREF_INIT(L"GD"), GD_PNG, INT_MAX }, { PH_STRINGREF_INIT(L"GE"), GE_PNG, INT_MAX },
    { PH_STRINGREF_INIT(L"GF"), GF_PNG, INT_MAX }, { PH_STRINGREF_INIT(L"GH"), GH_PNG, INT_MAX },
    { PH_STRINGREF_INIT(L"GI"), GI_PNG, INT_MAX }, { PH_STRINGREF_INIT(L"GL"), GL_PNG, INT_MAX },
    { PH_STRINGREF_INIT(L"GM"), GM_PNG, INT_MAX }, { PH_STRINGREF_INIT(L"GN"), GN_PNG, INT_MAX },
    { PH_STRINGREF_INIT(L"GP"), GP_PNG, INT_MAX }, { PH_STRINGREF_INIT(L"GQ"), GQ_PNG, INT_MAX },
    { PH_STRINGREF_INIT(L"GR"), GR_PNG, INT_MAX }, { PH_STRINGREF_INIT(L"GS"), GS_PNG, INT_MAX },
    { PH_STRINGREF_INIT(L"GT"), GT_PNG, INT_MAX }, { PH_STRINGREF_INIT(L"GU"), GU_PNG, INT_MAX },
    { PH_STRINGREF_INIT(L"GW"), GW_PNG, INT_MAX }, { PH_STRINGREF_INIT(L"GY"), GY_PNG, INT_MAX },
    { PH_STRINGREF_INIT(L"HK"), HK_PNG, INT_MAX }, { PH_STRINGREF_INIT(L"HM"), HM_PNG, INT_MAX },
    { PH_STRINGREF_INIT(L"HN"), HN_PNG, INT_MAX }, { PH_STRINGREF_INIT(L"HR"), HR_PNG, INT_MAX },
    { PH_STRINGREF_INIT(L"HT"), HT_PNG, INT_MAX }, { PH_STRINGREF_INIT(L"HU"), HU_PNG, INT_MAX },
    { PH_STRINGREF_INIT(L"ID"), ID_PNG, INT_MAX }, { PH_STRINGREF_INIT(L"IE"), IE_PNG, INT_MAX },
    { PH_STRINGREF_INIT(L"IL"), IL_PNG, INT_MAX }, { PH_STRINGREF_INIT(L"IN"), IN_PNG, INT_MAX },
    { PH_STRINGREF_INIT(L"IO"), IO_PNG, INT_MAX }, { PH_STRINGREF_INIT(L"IQ"), IQ_PNG, INT_MAX },
    { PH_STRINGREF_INIT(L"IR"), IR_PNG, INT_MAX }, { PH_STRINGREF_INIT(L"IS"), IS_PNG, INT_MAX },
    { PH_STRINGREF_INIT(L"IT"), IT_PNG, INT_MAX },
    { PH_STRINGREF_INIT(L"JM"), JM_PNG, INT_MAX }, { PH_STRINGREF_INIT(L"JO"), JO_PNG, INT_MAX },
    { PH_STRINGREF_INIT(L"JP"), JP_PNG, INT_MAX },
    { PH_STRINGREF_INIT(L"KE"), KE_PNG, INT_MAX }, { PH_STRINGREF_INIT(L"KG"), KG_PNG, INT_MAX },
    { PH_STRINGREF_INIT(L"KH"), KH_PNG, INT_MAX }, { PH_STRINGREF_INIT(L"KI"), KI_PNG, INT_MAX },
    { PH_STRINGREF_INIT(L"KM"), KM_PNG, INT_MAX }, { PH_STRINGREF_INIT(L"KN"), KN_PNG, INT_MAX },
    { PH_STRINGREF_INIT(L"KP"), KP_PNG, INT_MAX }, { PH_STRINGREF_INIT(L"KR"), KR_PNG, INT_MAX },
    { PH_STRINGREF_INIT(L"KW"), KW_PNG, INT_MAX }, { PH_STRINGREF_INIT(L"KY"), KY_PNG, INT_MAX },
    { PH_STRINGREF_INIT(L"KZ"), KZ_PNG, INT_MAX },
    { PH_STRINGREF_INIT(L"LA"), LA_PNG, INT_MAX }, { PH_STRINGREF_INIT(L"LB"), LB_PNG, INT_MAX },
    { PH_STRINGREF_INIT(L"LC"), LC_PNG, INT_MAX }, { PH_STRINGREF_INIT(L"LI"), LI_PNG, INT_MAX },
    { PH_STRINGREF_INIT(L"LK"), LK_PNG, INT_MAX }, { PH_STRINGREF_INIT(L"LR"), LR_PNG, INT_MAX },
    { PH_STRINGREF_INIT(L"LS"), LS_PNG, INT_MAX }, { PH_STRINGREF_INIT(L"LT"), LT_PNG, INT_MAX },
    { PH_STRINGREF_INIT(L"LU"), LU_PNG, INT_MAX }, { PH_STRINGREF_INIT(L"LV"), LV_PNG, INT_MAX },
    { PH_STRINGREF_INIT(L"LY"), LY_PNG, INT_MAX },
    { PH_STRINGREF_INIT(L"MA"), MA_PNG, INT_MAX }, { PH_STRINGREF_INIT(L"MC"), MC_PNG, INT_MAX },
    { PH_STRINGREF_INIT(L"MD"), MD_PNG, INT_MAX }, { PH_STRINGREF_INIT(L"ME"), ME_PNG, INT_MAX },
    { PH_STRINGREF_INIT(L"MG"), MG_PNG, INT_MAX }, { PH_STRINGREF_INIT(L"MH"), MH_PNG, INT_MAX },
    { PH_STRINGREF_INIT(L"MK"), MK_PNG, INT_MAX }, { PH_STRINGREF_INIT(L"ML"), ML_PNG, INT_MAX },
    { PH_STRINGREF_INIT(L"MM"), MM_PNG, INT_MAX }, { PH_STRINGREF_INIT(L"MN"), MN_PNG, INT_MAX },
    { PH_STRINGREF_INIT(L"MO"), MO_PNG, INT_MAX }, { PH_STRINGREF_INIT(L"MP"), MP_PNG, INT_MAX },
    { PH_STRINGREF_INIT(L"MQ"), MQ_PNG, INT_MAX }, { PH_STRINGREF_INIT(L"MR"), MR_PNG, INT_MAX },
    { PH_STRINGREF_INIT(L"MS"), MS_PNG, INT_MAX }, { PH_STRINGREF_INIT(L"MT"), MT_PNG, INT_MAX },
    { PH_STRINGREF_INIT(L"MU"), MU_PNG, INT_MAX }, { PH_STRINGREF_INIT(L"MV"), MV_PNG, INT_MAX },
    { PH_STRINGREF_INIT(L"MW"), MW_PNG, INT_MAX }, { PH_STRINGREF_INIT(L"MX"), MX_PNG, INT_MAX },
    { PH_STRINGREF_INIT(L"MY"), MY_PNG, INT_MAX }, { PH_STRINGREF_INIT(L"MZ"), MZ_PNG, INT_MAX },
    { PH_STRINGREF_INIT(L"NA"), NA_PNG, INT_MAX }, { PH_STRINGREF_INIT(L"NC"), NC_PNG, INT_MAX },
    { PH_STRINGREF_INIT(L"NE"), NE_PNG, INT_MAX }, { PH_STRINGREF_INIT(L"NF"), NF_PNG, INT_MAX },
    { PH_STRINGREF_INIT(L"NG"), NG_PNG, INT_MAX }, { PH_STRINGREF_INIT(L"NI"), NI_PNG, INT_MAX },
    { PH_STRINGREF_INIT(L"NL"), NL_PNG, INT_MAX }, { PH_STRINGREF_INIT(L"NO"), NO_PNG, INT_MAX },
    { PH_STRINGREF_INIT(L"NP"), NP_PNG, INT_MAX }, { PH_STRINGREF_INIT(L"NR"), NR_PNG, INT_MAX },
    { PH_STRINGREF_INIT(L"NU"), NU_PNG, INT_MAX }, { PH_STRINGREF_INIT(L"NZ"), NZ_PNG, INT_MAX },
    { PH_STRINGREF_INIT(L"OM"), OM_PNG, INT_MAX },
    { PH_STRINGREF_INIT(L"PA"), PA_PNG, INT_MAX }, { PH_STRINGREF_INIT(L"PE"), PE_PNG, INT_MAX },
    { PH_STRINGREF_INIT(L"PF"), PF_PNG, INT_MAX }, { PH_STRINGREF_INIT(L"PG"), PG_PNG, INT_MAX },
    { PH_STRINGREF_INIT(L"PH"), PH_PNG, INT_MAX }, { PH_STRINGREF_INIT(L"PK"), PK_PNG, INT_MAX },
    { PH_STRINGREF_INIT(L"PL"), PL_PNG, INT_MAX }, { PH_STRINGREF_INIT(L"PM"), PM_PNG, INT_MAX },
    { PH_STRINGREF_INIT(L"PN"), PN_PNG, INT_MAX }, { PH_STRINGREF_INIT(L"PR"), PR_PNG, INT_MAX },
    { PH_STRINGREF_INIT(L"PS"), PS_PNG, INT_MAX }, { PH_STRINGREF_INIT(L"PT"), PT_PNG, INT_MAX },
    { PH_STRINGREF_INIT(L"PW"), PW_PNG, INT_MAX }, { PH_STRINGREF_INIT(L"PY"), PY_PNG, INT_MAX },
    { PH_STRINGREF_INIT(L"QA"), QA_PNG, INT_MAX },
    { PH_STRINGREF_INIT(L"RE"), RE_PNG, INT_MAX }, { PH_STRINGREF_INIT(L"RO"), RO_PNG, INT_MAX },
    { PH_STRINGREF_INIT(L"RS"), RS_PNG, INT_MAX }, { PH_STRINGREF_INIT(L"RU"), RU_PNG, INT_MAX },
    { PH_STRINGREF_INIT(L"RW"), RW_PNG, INT_MAX },
    { PH_STRINGREF_INIT(L"SA"), SA_PNG, INT_MAX }, { PH_STRINGREF_INIT(L"SB"), SB_PNG, INT_MAX },
    { PH_STRINGREF_INIT(L"SC"), SC_PNG, INT_MAX }, { PH_STRINGREF_INIT(L"SD"), SD_PNG, INT_MAX },
    { PH_STRINGREF_INIT(L"SE"), SE_PNG, INT_MAX }, { PH_STRINGREF_INIT(L"SG"), SG_PNG, INT_MAX },
    { PH_STRINGREF_INIT(L"SH"), SH_PNG, INT_MAX }, { PH_STRINGREF_INIT(L"SI"), SI_PNG, INT_MAX },
    { PH_STRINGREF_INIT(L"SJ"), SJ_PNG, INT_MAX }, { PH_STRINGREF_INIT(L"SK"), SK_PNG, INT_MAX },
    { PH_STRINGREF_INIT(L"SL"), SL_PNG, INT_MAX }, { PH_STRINGREF_INIT(L"SM"), SM_PNG, INT_MAX },
    { PH_STRINGREF_INIT(L"SN"), SN_PNG, INT_MAX }, { PH_STRINGREF_INIT(L"SO"), SO_PNG, INT_MAX },
    { PH_STRINGREF_INIT(L"SR"), SR_PNG, INT_MAX }, { PH_STRINGREF_INIT(L"ST"), ST_PNG, INT_MAX },
    { PH_STRINGREF_INIT(L"SV"), SV_PNG, INT_MAX }, { PH_STRINGREF_INIT(L"SY"), SY_PNG, INT_MAX },
    { PH_STRINGREF_INIT(L"SZ"), SZ_PNG, INT_MAX },
    { PH_STRINGREF_INIT(L"TC"), TC_PNG, INT_MAX }, { PH_STRINGREF_INIT(L"TD"), TD_PNG, INT_MAX },
    { PH_STRINGREF_INIT(L"TF"), TF_PNG, INT_MAX }, { PH_STRINGREF_INIT(L"TG"), TG_PNG, INT_MAX },
    { PH_STRINGREF_INIT(L"TH"), TH_PNG, INT_MAX }, { PH_STRINGREF_INIT(L"TJ"), TJ_PNG, INT_MAX },
    { PH_STRINGREF_INIT(L"TK"), TK_PNG, INT_MAX }, { PH_STRINGREF_INIT(L"TL"), TL_PNG, INT_MAX },
    { PH_STRINGREF_INIT(L"TM"), TM_PNG, INT_MAX }, { PH_STRINGREF_INIT(L"TN"), TN_PNG, INT_MAX },
    { PH_STRINGREF_INIT(L"TO"), TO_PNG, INT_MAX }, { PH_STRINGREF_INIT(L"TR"), TR_PNG, INT_MAX },
    { PH_STRINGREF_INIT(L"TT"), TT_PNG, INT_MAX }, { PH_STRINGREF_INIT(L"TV"), TV_PNG, INT_MAX },
    { PH_STRINGREF_INIT(L"TW"), TW_PNG, INT_MAX }, { PH_STRINGREF_INIT(L"TZ"), TZ_PNG, INT_MAX },
    { PH_STRINGREF_INIT(L"UA"), UA_PNG, INT_MAX }, { PH_STRINGREF_INIT(L"UG"), UG_PNG, INT_MAX },
    { PH_STRINGREF_INIT(L"UM"), UM_PNG, INT_MAX }, { PH_STRINGREF_INIT(L"US"), US_PNG, INT_MAX },
    { PH_STRINGREF_INIT(L"UY"), UY_PNG, INT_MAX }, { PH_STRINGREF_INIT(L"UZ"), UZ_PNG, INT_MAX },
    { PH_STRINGREF_INIT(L"VA"), VA_PNG, INT_MAX }, { PH_STRINGREF_INIT(L"VC"), VC_PNG, INT_MAX },
    { PH_STRINGREF_INIT(L"VE"), VE_PNG, INT_MAX }, { PH_STRINGREF_INIT(L"VG"), VG_PNG, INT_MAX },
    { PH_STRINGREF_INIT(L"VI"), VI_PNG, INT_MAX }, { PH_STRINGREF_INIT(L"VN"), VN_PNG, INT_MAX },
    { PH_STRINGREF_INIT(L"VU"), VU_PNG, INT_MAX },
    { PH_STRINGREF_INIT(L"WF"), WF_PNG, INT_MAX }, { PH_STRINGREF_INIT(L"WS"), WS_PNG, INT_MAX },
    { PH_STRINGREF_INIT(L"YE"), YE_PNG, INT_MAX }, { PH_STRINGREF_INIT(L"YT"), YT_PNG, INT_MAX },
    { PH_STRINGREF_INIT(L"ZA"), ZA_PNG, INT_MAX }, { PH_STRINGREF_INIT(L"ZM"), ZM_PNG, INT_MAX },
    { PH_STRINGREF_INIT(L"ZW"), ZW_PNG, INT_MAX }
};

INT LookupCountryIcon(
    _In_ PPH_STRING Name
    )
{
    if (!GeoImageList)
        return INT_MAX;

    for (UINT i = 0; i < RTL_NUMBER_OF(CountryResourceTable); i++)
    {
        if (PhEqualStringRef(&Name->sr, &CountryResourceTable[i].CountryCode, TRUE))
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
                    CountryResourceTable[i].IconIndex = PhImageListAddBitmap(
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

    PhImageListDrawIcon(
        GeoImageList, 
        Index, 
        hdc,
        rect.left, 
        rect.top + ((rect.bottom - rect.top) - 11) / 2, 
        ILD_NORMAL,
        FALSE
        );
}

BOOLEAN NetworkToolsGeoDbCacheHashtableEqualFunction(
    _In_ PVOID Entry1,
    _In_ PVOID Entry2
    )
{
    PGEODB_IPADDR_CACHE_ENTRY entry1 = Entry1;
    PGEODB_IPADDR_CACHE_ENTRY entry2 = Entry2;

    return PhEqualIpAddress(&entry1->RemoteAddress, &entry2->RemoteAddress);
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
