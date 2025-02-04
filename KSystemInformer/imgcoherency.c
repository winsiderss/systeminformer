/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     jxy-s   2024-2025
 *
 */

#include <kph.h>

#include <trace.h>

KPH_PAGED_FILE();

/**
 * \brief Checks an image for coherency against the data backing the image.
 *
 * \details Always called from within structured exception handling.
 *
 * \param[in] ImageBase Base address of the image mapping.
 * \param[in] ImageSize Size of the image mapping.
 * \param[in] DataBase Base address of the data mapping.
 * \param[in] DataSize Size of the data mapping.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphpCheckImageCoherency(
    _In_ PVOID ImageBase,
    _In_ SIZE_T ImageSize,
    _In_ PVOID DataBase,
    _In_ SIZE_T DataSize
    )
{
    NTSTATUS status;
    KPH_MEMORY_REGION regions[4];
    KPH_IMAGE_NT_HEADERS image;
    SIZE_T size;
    PVOID dataEnd;
    PVOID imageEnd;

    KPH_PAGED_CODE_PASSIVE();

    RtlZeroMemory(&regions, sizeof(regions));

    regions[0].Start = DataBase;
    regions[0].End = Add2Ptr(DataBase, sizeof(IMAGE_DOS_HEADER));

    status = KphImageNtHeader(DataBase, DataSize, &image);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      GENERAL,
                      "KphImageNtHeader failed: %!STATUS!",
                      status);

        return status;
    }

    size = (image.Headers->FileHeader.NumberOfSections * sizeof(IMAGE_SECTION_HEADER));
    regions[1].Start = IMAGE_FIRST_SECTION(image.Headers);
    regions[1].End = Add2Ptr(regions[1].Start, size);

    regions[2].Start = &image.Headers->FileHeader.NumberOfSections;

    if (!RTL_CONTAINS_FIELD(&image.Headers->OptionalHeader,
                            image.Headers->FileHeader.SizeOfOptionalHeader,
                            Magic))
    {
        regions[2].End = &image.Headers->OptionalHeader.Magic;
        goto VerifyRegions;
    }

    if (image.Headers->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC)
    {
        if (!RTL_CONTAINS_FIELD(&image.Headers32->OptionalHeader,
                                image.Headers->FileHeader.SizeOfOptionalHeader,
                                ImageBase))
        {
            regions[2].End = Add2Ptr(&image.Headers32->OptionalHeader,
                                     image.Headers->FileHeader.SizeOfOptionalHeader);
            goto VerifyRegions;
        }

        regions[2].End = &image.Headers32->OptionalHeader.ImageBase;
        regions[3].Start = &image.Headers32->OptionalHeader.SectionAlignment;
        regions[3].End = Add2Ptr(&image.Headers32->OptionalHeader,
                                 image.Headers->FileHeader.SizeOfOptionalHeader);
    }
    else if (image.Headers->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC)
    {
        if (!RTL_CONTAINS_FIELD(&image.Headers64->OptionalHeader,
                                image.Headers->FileHeader.SizeOfOptionalHeader,
                                ImageBase))
        {
            regions[2].End = Add2Ptr(&image.Headers64->OptionalHeader,
                                     image.Headers->FileHeader.SizeOfOptionalHeader);
            goto VerifyRegions;
        }

        regions[2].End = &image.Headers64->OptionalHeader.ImageBase;
        regions[3].Start = &image.Headers64->OptionalHeader.SectionAlignment;
        regions[3].End = Add2Ptr(&image.Headers64->OptionalHeader,
                                 image.Headers->FileHeader.SizeOfOptionalHeader);
    }
    else
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      GENERAL,
                      "Invalid image optional header magic: 0x%04x",
                      image.Headers->OptionalHeader.Magic);

        return STATUS_INVALID_IMAGE_FORMAT;
    }

VerifyRegions:

    imageEnd = Add2Ptr(ImageBase, ImageSize);
    dataEnd = Add2Ptr(DataBase, DataSize);

    for (ULONG i = 0; i < ARRAYSIZE(regions); i++)
    {
        PKPH_MEMORY_REGION region;
        KPH_MEMORY_REGION other;

        region = &regions[i];

        if (!region->Start)
        {
            break;
        }

        if (!KphIsValidMemoryRegion(region, DataBase, dataEnd))
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          GENERAL,
                          "Image region overflows mapping.");

            return STATUS_BUFFER_OVERFLOW;
        }

        other.Start = RebasePtr(region->Start, DataBase, ImageBase);
        other.End = RebasePtr(region->End, DataBase, ImageBase);

        if (!KphIsValidMemoryRegion(&other, ImageBase, imageEnd))
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          GENERAL,
                          "Data region overflows mapping.");

            return STATUS_BUFFER_OVERFLOW;
        }

        size = PtrOffset(region->Start, region->End);

        if (!KphEqualMemory(region->Start, other.Start, size))
        {
            return STATUS_IMAGE_CHECKSUM_MISMATCH;
        }
    }

    return STATUS_SUCCESS;
}

/**
 * \brief Checks an image for coherency against the data backing the image.
 *
 * \param[in] ImageBase Base address of the image mapping.
 * \param[in] ImageSize Size of the image mapping.
 * \param[in] DataBase Base address of the data mapping.
 * \param[in] DataSize Size of the data mapping.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphCheckImageCoherency(
    _In_ PVOID ImageBase,
    _In_ SIZE_T ImageSize,
    _In_ PVOID DataBase,
    _In_ SIZE_T DataSize
    )
{
    NTSTATUS status;

    KPH_PAGED_CODE_PASSIVE();

    __try
    {
        status = KphpCheckImageCoherency(ImageBase,
                                         ImageSize,
                                         DataBase,
                                         DataSize);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        status = GetExceptionCode();
    }

    return status;
}
