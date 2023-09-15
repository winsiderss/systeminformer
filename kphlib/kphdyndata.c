/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     jxy-s   2023
 *
 */

#include <kphlibbase.h>
#include <kphdyndata.h>

/**
 * \brief Searches for a dynamic data configuration for a given kernel version.
 *
 * \param[in] DynData Pointer to dynamic data.
 * \param[in] MajorVersion Kernel major version.
 * \param[in] MinorVersion Kernel minor version.
 * \param[in] BuildNumber Kernel build number.
 * \param[in] Revision Kernel revision.
 * \param[out] Config Optional pointer to a variable that receives a pointer
 * to the found dynamic configuration in the dynamic data.
 *
 * \return Successful or errant status.
 */
_Must_inspect_result_
NTSTATUS KphDynDataGetConfiguration(
    _In_ PKPH_DYNDATA DynData,
    _In_ USHORT MajorVersion,
    _In_ USHORT MinorVersion,
    _In_ USHORT BuildNumber,
    _In_ USHORT Revision,
    _Out_opt_ PKPH_DYN_CONFIGURATION* Config
    )
{
    if (Config)
    {
        *Config = NULL;
    }

    if (DynData->Version != KPH_DYN_CONFIGURATION_VERSION)
    {
        return STATUS_SI_DYNDATA_VERSION_MISMATCH;
    }

    for (ULONG i = 0; i < DynData->Count; i++)
    {
        PKPH_DYN_CONFIGURATION config;

        config = &DynData->Configs[i];

        if ((config->MajorVersion != MajorVersion) ||
            (config->MinorVersion != MinorVersion))
        {
            continue;
        }

        if ((BuildNumber < config->BuildNumberMin) ||
            (BuildNumber > config->BuildNumberMax))
        {
            continue;
        }

        if ((BuildNumber == config->BuildNumberMin) &&
            (Revision < config->RevisionMin))
        {
            continue;
        }

        if ((BuildNumber == config->BuildNumberMax) &&
            (Revision > config->RevisionMax))
        {
            continue;
        }

        if (Config)
        {
            *Config = config;
        }

        return STATUS_SUCCESS;
    }

    return STATUS_SI_DYNDATA_UNSUPPORTED_KERNEL;
}
