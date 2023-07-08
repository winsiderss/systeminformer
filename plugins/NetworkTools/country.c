/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     dmex    2016-2023
 *
 */

#include "nettools.h"
#include "../../tools/thirdparty/maxminddb/maxminddb.h"

BOOLEAN GeoDbInitialized = FALSE;
BOOLEAN GeoDbExpired = FALSE;
BOOLEAN GeoDbDatabaseType = FALSE;
HIMAGELIST GeoImageList = NULL;
MMDB_s GeoDbCountry = { 0 };
PH_STRINGREF GeoDbCityFileName = PH_STRINGREF_INIT(L"GeoLite2-City.mmdb");
PH_STRINGREF GeoDbCountryFileName = PH_STRINGREF_INIT(L"GeoLite2-Country.mmdb");
PPH_HASHTABLE NetworkToolsGeoDbCacheHashtable = NULL;
PH_QUEUED_LOCK NetworkToolsGeoDbCacheHashtableLock = PH_QUEUED_LOCK_INIT;

typedef struct _GEODB_IPADDR_CACHE_ENTRY
{
    PH_IP_ADDRESS RemoteAddress;
    ULONG CountryCode;
    PPH_STRING CountryName;
} GEODB_IPADDR_CACHE_ENTRY, *PGEODB_IPADDR_CACHE_ENTRY;

typedef struct _GEODB_GEONAME_CACHE_TABLE
{
    PCCH CountryCode;
    ULONG GeoNameID;
    INT32 ResourceID;
} GEODB_GEONAME_CACHE_TABLE, *PGEODB_GEONAME_CACHE_TABLE;

typedef struct _GEODB_IMAGE_CACHE_TABLE
{
    INT IconIndex;
} GEODB_IMAGE_CACHE_TABLE, *PGEODB_IMAGE_CACHE_TABLE;

BOOLEAN NetToolsGeoLiteInitialized(
    VOID
    )
{
    static PH_INITONCE initOnce = PH_INITONCE_INIT;

    if (PhBeginInitOnce(&initOnce))
    {
        PPH_STRING dbpath;

        if (GeoDbDatabaseType)
            dbpath = PhGetApplicationDataFileName(&GeoDbCityFileName, TRUE);
        else
            dbpath = PhGetApplicationDataFileName(&GeoDbCountryFileName, TRUE);

        if (dbpath)
        {
            if (MMDB_open(&dbpath->sr, MMDB_MODE_MMAP, &GeoDbCountry) == MMDB_SUCCESS)
            {
                LARGE_INTEGER systemTime;
                ULONG secondsSince1970;

                // Query the current time
                PhQuerySystemTime(&systemTime);
                // Convert to unix epoch time
                PhTimeToSecondsSince1970(&systemTime, &secondsSince1970);

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
    _Out_ ULONG *CountryCode,
    _Out_ PPH_STRING *CountryName
    )
{
    MMDB_entry_data_s mmdb_entry;
    ULONG countryCode = 0;
    PPH_STRING countryName = NULL;
    //PPH_BYTES countryCode = NULL;

    if (MMDB_get_value(GeoDbEntry, &mmdb_entry, "country", "geoname_id", NULL) == MMDB_SUCCESS)
    {
        if (mmdb_entry.has_data && mmdb_entry.type == MMDB_DATA_TYPE_UINT32)
        {
            countryCode = mmdb_entry.uint32;
        }
    }

    if (MMDB_get_value(GeoDbEntry, &mmdb_entry, "country", "names", "en", NULL) == MMDB_SUCCESS)
    {
        if (mmdb_entry.has_data && mmdb_entry.type == MMDB_DATA_TYPE_UTF8_STRING)
        {
            countryName = PhConvertUtf8ToUtf16Ex((PCHAR)mmdb_entry.utf8_string, mmdb_entry.data_size);
        }
    }

    //if (MMDB_get_value(GeoDbEntry, &mmdb_entry, "country", "iso_code", NULL) == MMDB_SUCCESS)
    //{
    //    if (mmdb_entry.has_data && mmdb_entry.type == MMDB_DATA_TYPE_UTF8_STRING)
    //    {
    //        countryCode = PhCreateBytesEx((PCHAR)mmdb_entry.utf8_string, mmdb_entry.data_size);
    //    }
    //}

    if (countryCode && countryName)
    {
        *CountryCode = countryCode;
        *CountryName = countryName;
        return TRUE;
    }

    PhClearReference(&countryName);
    return FALSE;
}

_Success_(return)
BOOLEAN GeoDbGetContinentData(
    _In_ MMDB_entry_s* GeoDbEntry,
    _Out_ ULONG *ContinentCode,
    _Out_ PPH_STRING *ContinentName
    )
{
    MMDB_entry_data_s mmdb_entry;
    ULONG continentCode = 0;
    PPH_STRING continentName = NULL;

    if (MMDB_get_value(GeoDbEntry, &mmdb_entry, "country", "geoname_id", NULL) == MMDB_SUCCESS)
    {
        if (mmdb_entry.has_data && mmdb_entry.type == MMDB_DATA_TYPE_UINT32)
        {
            continentCode = mmdb_entry.uint32;
        }
    }

    if (MMDB_get_value(GeoDbEntry, &mmdb_entry, "country", "names", "en", NULL) == MMDB_SUCCESS)
    {
        if (mmdb_entry.has_data && mmdb_entry.type == MMDB_DATA_TYPE_UTF8_STRING)
        {
            continentName = PhConvertUtf8ToUtf16Ex((PCHAR)mmdb_entry.utf8_string, mmdb_entry.data_size);
        }
    }

    //if (MMDB_get_value(GeoDbEntry, &mmdb_entry, "continent", "code", NULL) == MMDB_SUCCESS)
    //{
    //    if (mmdb_entry.has_data && mmdb_entry.type == MMDB_DATA_TYPE_UTF8_STRING)
    //    {
    //        continentCode = PhCreateBytesEx((PCHAR)mmdb_entry.utf8_string, mmdb_entry.data_size);
    //    }
    //}

    if (continentCode && continentName)
    {
        *ContinentCode = continentCode;
        *ContinentName = continentName;
        return TRUE;
    }

    PhClearReference(&continentName);
    return FALSE;
}

_Success_(return)
BOOLEAN LookupCountryCodeFromMmdb(
    _In_ PH_IP_ADDRESS RemoteAddress,
    _Out_ ULONG *CountryCode,
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
    _Out_ ULONG *CountryCode,
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
    _Out_ ULONG *CountryCode,
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

CONST GEODB_GEONAME_CACHE_TABLE GeoCountryResourceTable[] =
{
    { "AD", 3041565, AD_PNG }, { "AE", 290557, AE_PNG },
    { "AF", 1149361, AF_PNG }, { "AG", 3576396, AG_PNG },
    { "AI", 3573511, AI_PNG }, { "AL", 783754, AL_PNG },
    { "AM", 174982, AM_PNG },  { "AN", 8505032, AN_PNG },
    { "AO", 3351879, AO_PNG }, { "AR", 3865483, AR_PNG },
    { "AS", 5880801, AS_PNG }, { "AT", 2782113, AT_PNG },
    { "AU", 2077456, AU_PNG }, { "AW", 3577279, AW_PNG },
    { "AX", 661882, AX_PNG },  { "AZ", 587116, AZ_PNG },
    { "BA", 3277605, BA_PNG }, { "BB", 3374084, BB_PNG },
    { "BD", 1210997, BD_PNG }, { "BE", 2802361, BE_PNG },
    { "BF", 2361809, BF_PNG }, { "BG", 732800, BG_PNG },
    { "BH", 290291, BH_PNG },  { "BI", 433561, BI__PNG },
    { "BJ", 2395170, BJ_PNG }, { "BM", 3573345, BM_PNG },
    { "BN", 1820814, BN_PNG }, { "BO", 3923057, BO_PNG },
    { "BR", 3469034, BR_PNG }, { "BS", 3572887, BS_PNG },
    { "BT", 1252634, BT_PNG }, { "BV", 3371123, BV_PNG },
    { "BW", 933860, BW_PNG },  { "BY", 630336, BY_PNG },
    { "BZ", 3582678, BZ_PNG },
    { "CA", 6251999, CA_PNG }, { "CC", 1547376, CC_PNG },
    { "CD", 203312, CD_PNG },  { "CF", 239880, CF_PNG },
    { "CG", 2260494, CG_PNG }, { "CH", 2658434, CH_PNG },
    { "CI", 2287781, CI_PNG }, { "CK", 1899402, CK_PNG },
    { "CL", 3895114, CL_PNG }, { "CM", 2233387, CM_PNG },
    { "CN", 1814991, CN_PNG }, { "CO", 3686110, CO_PNG },
    { "CR", 3624060, CR_PNG }, { "CS", 8505033, CS_PNG },
    { "CU", 3562981, CU_PNG }, { "CV", 3374766, CV_PNG },
    { "CX", 2078138, CX_PNG }, { "CY", 146669, CY_PNG },
    { "CZ", 3077311, CZ_PNG }, { "DE", 2921044, DE_PNG },
    { "DJ", 223816, DJ_PNG },  { "DK", 2623032, DK_PNG },
    { "DM", 3575830, DM_PNG }, { "DO", 3508796, DO_PNG },
    { "DZ", 2589581, DZ_PNG },
    { "EC", 3658394, EC_PNG }, { "EE", 453733, EE_PNG },
    { "EG", 357994, EG_PNG },  { "EH", 2461445, EH_PNG },
    { "ER", 338010, ER_PNG },  { "ES", 2510769, ES_PNG },
    { "ET", 337996, ET_PNG },
    { "FI", 660013, FI_PNG },  { "FJ", 2205218, FJ_PNG },
    { "FK", 3474414, FK_PNG }, { "FO", 2622320, FO_PNG },
    { "FR", 3017382, FR_PNG },
    { "GA", 2400553, GA_PNG }, { "GB", 2635167, GB_PNG },
    { "GD", 3580239, GD_PNG }, { "GE", 614540, GE_PNG },
    { "GF", 3381670, GF_PNG }, { "GH", 2300660, GH_PNG },
    { "GI", 2411586, GI_PNG }, { "GL", 3425505, GL_PNG },
    { "GM", 2413451, GM_PNG }, { "GN", 2420477, GN_PNG },
    { "GP", 3579143, GP_PNG }, { "GQ", 2309096, GQ_PNG },
    { "GR", 390903, GR_PNG },  { "GS", 3474415, GS_PNG },
    { "GT", 3595528, GT_PNG }, { "GU", 4043988, GU_PNG },
    { "GW", 2372248, GW_PNG }, { "GY", 3378535, GY_PNG },
    { "HK", 1819730, HK_PNG }, { "HM", 1547314, HM_PNG },
    { "HN", 3608932, HN_PNG }, { "HR", 3202326, HR_PNG },
    { "HT", 3723988, HT_PNG }, { "HU", 719819, HU_PNG },
    { "ID", 1643084, ID_PNG }, { "IE", 2963597, IE_PNG },
    { "IL", 294640, IL_PNG },  { "IN", 1269750, IN_PNG },
    { "IO", 1282588, IO_PNG }, { "IQ", 99237, IQ_PNG },
    { "IR", 130758, IR_PNG },  { "IS", 2629691, IS_PNG },
    { "IT", 3175395, IT_PNG },
    { "JM", 3489940, JM_PNG }, { "JO", 248816, JO_PNG },
    { "JP", 1861060, JP_PNG },
    { "KE", 192950, KE_PNG },  { "KG", 1527747, KG_PNG },
    { "KH", 1831722, KH_PNG }, { "KI", 4030945, KI_PNG },
    { "KM", 921929, KM_PNG },  { "KN", 3575174, KN_PNG },
    { "KP", 1873107, KP_PNG }, { "KR", 1835841, KR_PNG },
    { "KW", 285570, KW_PNG },  { "KY", 3580718, KY_PNG },
    { "KZ", 1522867, KZ_PNG },
    { "LA", 1655842, LA_PNG }, { "LB", 272103, LB_PNG },
    { "LC", 3576468, LC_PNG }, { "LI", 3042058, LI_PNG },
    { "LK", 1227603, LK_PNG }, { "LR", 2275384, LR_PNG },
    { "LS", 932692, LS_PNG },  { "LT", 597427, LT_PNG },
    { "LU", 2960313, LU_PNG }, { "LV", 458258, LV_PNG },
    { "LY", 2215636, LY_PNG },
    { "MA", 2542007, MA_PNG }, { "MC", 2993457, MC_PNG },
    { "MD", 617790, MD_PNG },  { "ME", 3194884, ME_PNG },
    { "MG", 1062947, MG_PNG }, { "MH", 2080185, MH_PNG },
    { "MK", 718075, MK_PNG },  { "ML", 2453866, ML_PNG },
    { "MM", 1327865, MM_PNG }, { "MN", 2029969, MN_PNG },
    { "MO", 1821275, MO_PNG }, { "MP", 4041468, MP_PNG },
    { "MQ", 3570311, MQ_PNG }, { "MR", 2378080, MR_PNG },
    { "MS", 3578097, MS_PNG }, { "MT", 2562770, MT_PNG },
    { "MU", 934292, MU_PNG },  { "MV", 1282028, MV_PNG },
    { "MW", 927384, MW_PNG },  { "MX", 3996063, MX_PNG },
    { "MY", 1733045, MY_PNG }, { "MZ", 1036973, MZ_PNG },
    { "NA", 3355338, NA_PNG }, { "NC", 2139685, NC_PNG },
    { "NE", 2440476, NE_PNG }, { "NF", 2155115, NF_PNG },
    { "NG", 2328926, NG_PNG }, { "NI", 3617476, NI_PNG },
    { "NL", 2750405, NL_PNG }, { "NO", 3144096, NO_PNG },
    { "NP", 1282988, NP_PNG }, { "NR", 2110425, NR_PNG },
    { "NU", 4036232, NU_PNG }, { "NZ", 2186224, NZ_PNG },
    { "OM", 286963, OM_PNG },
    { "PA", 3703430, PA_PNG }, { "PE", 3932488, PE_PNG },
    { "PF", 4030656, PF_PNG }, { "PG", 2088628, PG_PNG },
    { "PH", 1694008, PH_PNG }, { "PK", 1168579, PK_PNG },
    { "PL", 798544, PL_PNG },  { "PM", 3424932, PM_PNG },
    { "PN", 4030699, PN_PNG }, { "PR", 4566966, PR_PNG },
    { "PS", 6254930, PS_PNG }, { "PT", 2264397, PT_PNG },
    { "PW", 1559582, PW_PNG }, { "PY", 3437598, PY_PNG },
    { "QA", 289688, QA_PNG },
    { "RE", 935317, RE_PNG },  { "RO", 798549, RO_PNG },
    { "RS", 6290252, RS_PNG }, { "RU", 2017370, RU_PNG },
    { "RW", 49518, RW_PNG },
    { "SA", 102358, SA_PNG },  { "SB", 2103350, SB_PNG },
    { "SC", 241170, SC_PNG },  { "SD", 366755, SD_PNG },
    { "SE", 2661886, SE_PNG }, { "SG", 1880251, SG_PNG },
    { "SH", 3370751, SH_PNG }, { "SI", 3190538, SI_PNG },
    { "SJ", 607072, SJ_PNG },  { "SK", 3057568, SK_PNG },
    { "SL", 2403846, SL_PNG }, { "SM", 3168068, SM_PNG },
    { "SN", 2245662, SN_PNG }, { "SO", 51537, SO_PNG },
    { "SR", 3382998, SR_PNG }, { "ST", 2410758, ST_PNG },
    { "SV", 3585968, SV_PNG }, { "SY", 163843, SY_PNG },
    { "SZ", 934841, SZ_PNG },
    { "TC", 3576916, TC_PNG }, { "TD", 2434508, TD_PNG },
    { "TF", 1546748, TF_PNG }, { "TG", 2363686, TG_PNG },
    { "TH", 1605651, TH_PNG }, { "TJ", 1220409, TJ_PNG },
    { "TK", 4031074, TK_PNG }, { "TL", 1966436, TL_PNG },
    { "TM", 1218197, TM_PNG }, { "TN", 2464461, TN_PNG },
    { "TO", 4032283, TO_PNG }, { "TR", 298795, TR_PNG },
    { "TT", 3573591, TT_PNG }, { "TV", 2110297, TV_PNG },
    { "TW", 1668284, TW_PNG }, { "TZ", 149590, TZ_PNG },
    { "UA", 690791, UA_PNG },  { "UG", 226074, UG_PNG },
    { "UM", 5854968, UM_PNG }, { "US", 6252001, US_PNG },
    { "UY", 3439705, UY_PNG }, { "UZ", 1512440, UZ_PNG },
    { "VA", 3164670, VA_PNG }, { "VC", 3577815, VC_PNG },
    { "VE", 3625428, VE_PNG }, { "VG", 3577718, VG_PNG },
    { "VI", 4796775, VI_PNG }, { "VN", 1562822, VN_PNG },
    { "VU", 2134431, VU_PNG },
    { "WF", 4034749, WF_PNG }, { "WS", 4034894, WS_PNG },
    { "YE", 69543, YE_PNG },   { "YT", 1024031, YT_PNG },
    { "ZA", 953987, ZA_PNG },  { "ZM", 895949, ZM_PNG },
    { "ZW", 878675, ZW_PNG }
};

GEODB_IMAGE_CACHE_TABLE GeoCountryImageTable[RTL_NUMBER_OF(GeoCountryResourceTable)] = { 0 };

INT LookupCountryIcon(
    _In_ ULONG Name
    )
{
    if (!GeoImageList)
        return INT_ERROR;

    for (UINT i = 0; i < RTL_NUMBER_OF(GeoCountryResourceTable); i++)
    {
        if (Name == GeoCountryResourceTable[i].GeoNameID)
        {
            if (GeoCountryImageTable[i].IconIndex == INT_ERROR)
            {
                HBITMAP countryBitmap;

                if (countryBitmap = PhLoadImageFormatFromResource(
                    PluginInstance->DllBase,
                    MAKEINTRESOURCE(GeoCountryResourceTable[i].ResourceID),
                    L"PNG",
                    PH_IMAGE_FORMAT_TYPE_PNG,
                    16,
                    11
                    ))
                {
                    GeoCountryImageTable[i].IconIndex = PhImageListAddBitmap(
                        GeoImageList,
                        countryBitmap,
                        NULL
                        );
                    DeleteBitmap(countryBitmap);
                }
            }

            return GeoCountryImageTable[i].IconIndex;
        }
    }

    return INT_ERROR;
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
    _Out_ ULONG* CountryCode,
    _Out_ PPH_STRING* CountryName
    )
{
    ULONG countryCode = 0;
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
                *CountryCode = entry->CountryCode;
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
        memset(GeoCountryImageTable, INT_ERROR, sizeof(GeoCountryImageTable));
    }

    newEntry.CountryCode = countryCode;
    newEntry.CountryName = countryName;
    memcpy_s(&newEntry.RemoteAddress, sizeof(newEntry.RemoteAddress), &RemoteAddress, sizeof(PH_IP_ADDRESS));
    PhAddEntryHashtable(NetworkToolsGeoDbCacheHashtable, &newEntry);

    if (CountryCode)
        *CountryCode = countryCode;
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
        PhDereferenceObject(entry->CountryName);
    }

    PhClearReference(&NetworkToolsGeoDbCacheHashtable);
    NetworkToolsGeoDbCacheHashtable = PhCreateHashtable(
        sizeof(GEODB_IPADDR_CACHE_ENTRY),
        NetworkToolsGeoDbCacheHashtableEqualFunction,
        NetworkToolsGeoDbCacheHashtableHashFunction,
        32
        );
    memset(GeoCountryImageTable, INT_ERROR, sizeof(GeoCountryImageTable));

    PhReleaseQueuedLockExclusive(&NetworkToolsGeoDbCacheHashtableLock);
}
