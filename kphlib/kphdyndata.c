/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     jxy-s   2023-2025
 *
 */

#include <kphlibbase.h>
#include <kphdyndata.h>

#ifndef _KERNEL_MODE
#include <ntintsafe.h>
#ifndef Add2Ptr
#define Add2Ptr(P,I) ((PVOID)((PUCHAR)(P) + (I)))
#endif
#endif

/**
 * \brief Searches for a dynamic data configuration.
 *
 * \param[in] Config The dynamic data configuration.
 * \param[in] Length The length of the dynamic data configuration.
 * \param[in] Class The class to search for.
 * \param[in] Machine The machine to search for.
 * \param[in] TimeDateStamp The time date stamp to search for.
 * \param[in] SizeOfImage The size of image to search for.
 * \param[out] Data Receives a pointer into the dynamic data configuration if
 * the requested data is found.
 * \param[out] Fields Receives a pointer into the dynamic data configuration for
 * the related fields if the requested data is found.
 *
 * \return Successful or errant status.
 */
_Must_inspect_result_
NTSTATUS KphDynDataLookup(
    _In_reads_bytes_(Length) PKPH_DYN_CONFIG Config,
    _In_ ULONG Length,
    _In_ USHORT Class,
    _In_ USHORT Machine,
    _In_ ULONG TimeDateStamp,
    _In_ ULONG SizeOfImage,
    _Out_opt_ PKPH_DYN_DATA* Data,
    _Out_opt_ PVOID* Fields
    )
{
    NTSTATUS status;
    ULONG dataLength;
    ULONG length;

    if (Data)
    {
        *Data = NULL;
    }

    if (Fields)
    {
        *Fields = NULL;
    }

    status = RtlULongSub(Length,
                         RTL_SIZEOF_THROUGH_FIELD(KPH_DYN_CONFIG, Count),
                         &length);
    if (!NT_SUCCESS(status))
    {
        return STATUS_SI_DYNDATA_INVALID_LENGTH;
    }

    if (Config->Version != KPH_DYN_CONFIGURATION_VERSION)
    {
        return STATUS_SI_DYNDATA_VERSION_MISMATCH;
    }

    status = RtlULongMult(sizeof(KPH_DYN_DATA), Config->Count, &dataLength);
    if (!NT_SUCCESS(status))
    {
        return STATUS_SI_DYNDATA_INVALID_LENGTH;
    }

    status = RtlULongSub(length, dataLength, &length);
    if (!NT_SUCCESS(status))
    {
        return STATUS_SI_DYNDATA_INVALID_LENGTH;
    }

    for (ULONG i = 0; i < Config->Count; i++)
    {
        PKPH_DYN_DATA data;

        data = &Config->Data[i];

        if ((data->Class != Class) ||
            (data->Machine != Machine) ||
            (data->TimeDateStamp != TimeDateStamp) ||
            (data->SizeOfImage != SizeOfImage))
        {
            continue;
        }

        if (data->Offset >= length)
        {
            return STATUS_SI_DYNDATA_INVALID_LENGTH;
        }

        if (Data)
        {
            *Data = data;
        }

        if (Fields)
        {
            PVOID start;

            start = &Config->Data[Config->Count];

            *Fields = Add2Ptr(start, data->Offset);
        }

        return STATUS_SUCCESS;
    }

    return STATUS_SI_DYNDATA_UNSUPPORTED_KERNEL;
}
