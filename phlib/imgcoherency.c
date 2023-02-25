/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     jxy-s   2020-2022
 *     dmex    2021-2023
 *
 */

#include <ph.h>
#include <mapimg.h>
#include <kphuser.h>

#define PH_IMGCOHERENCY_NORMAL_SCAN_LIMIT         (40 * (1024 * 1024)) // 40Mib
#define PH_IMGCOHERENCY_QUICK_SCAN_LIMIT          (PAGE_SIZE * 2)
#define PH_IMGCOHERENCY_MIN_ENTRY_INSPECT         (PAGE_SIZE * 2)

/**
* Image coherency context, used during the calculation of the process
* image coherency.
*/
typedef struct _PH_IMAGE_COHERENCY_CONTEXT
{
    PH_IMAGE_COHERENCY_SCAN_TYPE Type;        /**< Type of scan to preform */

    SIZE_T CoherentBytes;                     /**< Updated during inspection, coherent bytes */
    SIZE_T TotalBytes;                        /**< Updated during inspection, total bytes analyzed */
    SIZE_T SkippedBytes;                      /**< Bytes skipped during inspection */

    NTSTATUS MappedImageStatus;               /**< Status of initializing MappedImage */
    PH_MAPPED_IMAGE MappedImage;              /**< On-disk image mapping */
    PPH_HASHTABLE MappedImageReloc;           /**< On-disk mapped image relocations table */
    ULONG MappedImageBaseRva;                 /**< On-disk optional header image base RVA. */
    ULONG MappedImageIatRva;                  /**< On-disk import address table RVA */
    ULONG MappedImageIatSize;                 /**< On-disk import address table size */

    NTSTATUS RemoteMappedImageStatus;         /**< Status of initializing RemoteMappedImage */
    PH_REMOTE_MAPPED_IMAGE RemoteMappedImage; /**< Remote image mapping */

    PVOID RemoteImageBase;                    /**< Remote image base address */
    PPH_READ_VIRTUAL_MEMORY_CALLBACK ReadVirtualMemory; /**< Read virtual memory callback */

} PH_IMAGE_COHERENCY_CONTEXT, *PPH_IMAGE_COHERENCY_CONTEXT;

/**
* Inspection skip callback type.
*
* \param[in] Rva - Current rva in the range being inspected.
* \param[in] Context - Context supplied to this callback.
*
* \return Number of bytes to skip, 0 does not skip bytes.
*/
typedef ULONG (CALLBACK* PPH_IMGCOHERENCY_SKIP_BYTE_CALLBACK)(
    _In_ ULONG Rva,
    _In_opt_ PVOID Context
    );

/**
* Retrieves the size of the section to scan given the scan type.
*
* \param[in] Type - Image coherency scan type.
* \param[in] SectionHeader - Image section header.
*
* \return Amount of the section to scan given the scan type.
*/
ULONG PhpGetSectionScanSize(
    _In_ PH_IMAGE_COHERENCY_SCAN_TYPE Type,
    _In_ PIMAGE_SECTION_HEADER SectionHeader
    )
{
    ULONG size;

    size = min(SectionHeader->SizeOfRawData, SectionHeader->Misc.VirtualSize);

    switch (Type)
    {
        case PhImageCoherencyQuick:
        {
            if (size > PH_IMGCOHERENCY_QUICK_SCAN_LIMIT)
            {
                size = PH_IMGCOHERENCY_QUICK_SCAN_LIMIT;
            }
            break;
        }
        case PhImageCoherencyNormal:
        {
            if (size > PH_IMGCOHERENCY_NORMAL_SCAN_LIMIT)
            {
                size = PH_IMGCOHERENCY_NORMAL_SCAN_LIMIT;
            }
            break;
        }
        case PhImageCoherencyFull:
        default:
        {
            break;
        }
    }

    return size;
}

/**
* Determines if the section should be scanned given the scan type.
*
* \param[in] Type - Image coherency scan type.
* \param[in] SectionHeader - Section header to inspect.
*
* \return TRUE if the section should be scanned for the given scan type, FALSE otherwise.
*/
BOOLEAN PhpShouldScanSection(
    _In_ PH_IMAGE_COHERENCY_SCAN_TYPE Type,
    _In_ PIMAGE_SECTION_HEADER SectionHeader
    )
{
    switch (Type)
    {
        case PhImageCoherencyQuick:
        case PhImageCoherencyNormal:
        case PhImageCoherencyFull:
        {
            if ((SectionHeader->Characteristics & IMAGE_SCN_MEM_EXECUTE) != 0)
            {
                //
                // Anything marked execute.
                //
                return TRUE;
            }
            break;
        }
    }

    return FALSE;
}

/**
* Frees the image coherency context.
*
* \param[in] Context - Context to free, may be NULL.
*/
VOID PhpFreeImageCoherencyContext(
    _In_opt_ PPH_IMAGE_COHERENCY_CONTEXT Context
    )
{
    if (Context)
    {
        PhUnloadMappedImage(&Context->MappedImage);
        PhUnloadRemoteMappedImage(&Context->RemoteMappedImage);

        if (Context->MappedImageReloc)
        {
            PhDereferenceObject(Context->MappedImageReloc);
        }

        PhFree(Context);
    }
}

/**
* Created the image coherency context. This is done best-effort and
* the status is stored in the context.
*
* \param[in] Type - Image coherency scan type.
* \param[in] FileName - Win32 file name of the image to inspect.
* \param[in] ProcessHandle - Handle to process to inspect. Requires
* PROCESS_QUERY_LIMITED_INFORMATION and PROCESS_VM_READ.
* \param[in] RemoteImageBase - Remove image base address, optional.
* \param[in] RemoteImageBaseStatus - If RemoteImageBase is null, this is stored
* in the context instead of attempting to map the image.
* \param[in] ReadVirtualMemoryCallback - Callback to use to read virtual memory.
*
* \return Pointer to newly allocated image coherency context, or NULL on
* allocation failure. The created context must be passed to
* PhpFreeImageCoherencyContext to free.
*/
PPH_IMAGE_COHERENCY_CONTEXT PhpCreateImageCoherencyContext(
    _In_ PH_IMAGE_COHERENCY_SCAN_TYPE Type,
    _In_ PPH_STRING FileName,
    _In_ HANDLE ProcessHandle,
    _In_opt_ PVOID RemoteImageBase,
    _In_ NTSTATUS RemoteImageBaseStatus,
    _In_ PPH_READ_VIRTUAL_MEMORY_CALLBACK ReadVirtualMemoryCallback
    )
{
    PPH_IMAGE_COHERENCY_CONTEXT context;

    //
    // This is best-effort context creation, we don't fail if the mapping
    // fails - the caller of the API may make decisions based on success of
    // each
    //

    context = PhAllocateZero(sizeof(PH_IMAGE_COHERENCY_CONTEXT));
    context->Type = Type;
    context->ReadVirtualMemory = ReadVirtualMemoryCallback;

    //
    // Map the on-disk image
    //
    context->MappedImageStatus = PhLoadMappedImageEx(
        &FileName->sr,
        NULL,
        &context->MappedImage
        );

    if (NT_SUCCESS(context->MappedImageStatus))
    {
        PH_MAPPED_IMAGE_RELOC relocs;
        PH_MAPPED_IMAGE_DYNAMIC_RELOC dynRelocs;
        PIMAGE_DATA_DIRECTORY directory;

        //
        // Build a hash table for the relocation entries to skip later.
        // This hash table will map the RVA to the number of bytes to skip.
        //

        context->MappedImageReloc = PhCreateSimpleHashtable(10);

        if (NT_SUCCESS(PhGetMappedImageRelocations(&context->MappedImage, &relocs)))
        {
            for (ULONG i = 0; i < relocs.NumberOfEntries; i++)
            {
                PPH_IMAGE_RELOC_ENTRY entry;

                entry = &relocs.RelocationEntries[i];

                if ((entry->Record.Type != IMAGE_REL_BASED_ABSOLUTE) &&
                    (entry->Record.Type != IMAGE_REL_BASED_RESERVED))
                {
                    ULONG_PTR rva;

                    rva = (ULONG_PTR)entry->BlockRva + entry->Record.Offset;

                    if (entry->Record.Type == IMAGE_REL_BASED_DIR64)
                    {
                        PhAddItemSimpleHashtable(context->MappedImageReloc,
                                                 (PVOID)rva,
                                                 ULongToPtr(8));
                    }
                    else
                    {
                        //
                        // For now, we're just going to do a 4 byte skip for
                        // all other relocations. This could probably use some
                        // work for higher accuracy.
                        //

                        PhAddItemSimpleHashtable(context->MappedImageReloc,
                                                 (PVOID)rva,
                                                 ULongToPtr(4));
                    }
                }
            }

            PhFreeMappedImageRelocations(&relocs);
        }

        if (NT_SUCCESS(PhGetMappedImageDynamicRelocations(&context->MappedImage, &dynRelocs)))
        {
            for (ULONG i = 0; i < dynRelocs.NumberOfEntries; i++)
            {
                PPH_IMAGE_DYNAMIC_RELOC_ENTRY entry;
                ULONG_PTR rva = 0;
                ULONG_PTR size = 0;

                entry = &dynRelocs.RelocationEntries[i];

                if (entry->Symbol == IMAGE_DYNAMIC_RELOCATION_ARM64X)
                {
                    rva = (ULONG_PTR)entry->ARM64X.BlockRva + entry->ARM64X.RecordFixup.Offset;
                    switch (entry->ARM64X.RecordFixup.Type)
                    {
                        case IMAGE_DVRT_ARM64X_FIXUP_TYPE_ZEROFILL:
                        case IMAGE_DVRT_ARM64X_FIXUP_TYPE_VALUE:
                            size = (ULONG_PTR)(1ull << entry->ARM64X.RecordFixup.Size);
                            break;
                        case IMAGE_DVRT_ARM64X_FIXUP_TYPE_DELTA:
                            size = 4;
                            break;
                    }
                }
                else if (entry->Symbol == IMAGE_DYNAMIC_RELOCATION_FUNCTION_OVERRIDE)
                {
                    rva = (ULONG_PTR)entry->FuncOverride.BlockRva + entry->FuncOverride.Record.Offset;
                    if (context->MappedImage.Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC)
                        size = 4;
                    else if (context->MappedImage.Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC)
                        size = 8;
                }
                else
                {
                    //
                    // This should only be absolute, skipping others.
                    //
                    if (entry->Other.Record.Type == IMAGE_REL_BASED_ABSOLUTE)
                    {
                        rva = (ULONG_PTR)entry->Other.BlockRva + entry->Other.Record.Offset;
                        if (context->MappedImage.Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC)
                            size = 4;
                        else if (context->MappedImage.Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC)
                            size = 8;
                    }
                }

                if (rva && size)
                {
                    PhAddItemSimpleHashtable(context->MappedImageReloc,
                                             (PVOID)rva,
                                             (PVOID)size);
                }
            }

            PhFreeMappedImageDynamicRelocations(&dynRelocs);
        }

        if (NT_SUCCESS(PhGetMappedImageDataEntry(&context->MappedImage,
                                                 IMAGE_DIRECTORY_ENTRY_IAT,
                                                 &directory)))
        {
            context->MappedImageIatRva = directory->VirtualAddress;
            context->MappedImageIatSize = directory->Size;
        }

        if (context->MappedImage.Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC)
        {
            PVOID address;
            address = &context->MappedImage.NtHeaders32->OptionalHeader.ImageBase;
            context->MappedImageBaseRva = PtrToUlong(PTR_SUB_OFFSET(address, context->MappedImage.ViewBase));
        }
        else if (context->MappedImage.Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC)
        {
            PVOID address;
            address = &context->MappedImage.NtHeaders->OptionalHeader.ImageBase;
            context->MappedImageBaseRva = PtrToUlong(PTR_SUB_OFFSET(address, context->MappedImage.ViewBase));
        }
        else
        {
            context->MappedImageBaseRva = 0;
        }
    }

    if (!RemoteImageBase)
    {
        context->RemoteMappedImageStatus = RemoteImageBaseStatus;
        return context;
    }

    context->RemoteImageBase = RemoteImageBase;

    //
    // Map the remote image
    //
    context->RemoteMappedImageStatus =
        PhLoadRemoteMappedImageEx(ProcessHandle,
                                  context->RemoteImageBase,
                                  ReadVirtualMemoryCallback,
                                  &context->RemoteMappedImage);

    return context;
}

/**
* Inspects two buffers and adds them to the coherency calculation.
*
* \param[in] LeftBuffer - First buffer to inspect.
* \param[in] LeftCount - Number of bytes in the first buffer.
* \param[in] RightBuffer - Second buffer to inspect.
* \param[in] RightCount - Number of bytes in the second buffer.
* \param[in,out] Context - Context to be updated during inspection.
* \param[in] Rva - RVA from which the buffers were retrieved, informs skip callback.
* \param[in] SkipCallback - Optional, if provided the skip callback is invoked
* for each inspected byte, the callback may return any number of bytes to skip.
* \param[in] SkipCallbackContext - Optional, callback context passed to the skip callback.
*/
VOID PhpAnalyzeImageCoherencyInspect(
    _In_opt_ PBYTE LeftBuffer,
    _In_ ULONG LeftCount,
    _In_opt_ PBYTE RightBuffer,
    _In_ ULONG RightCount,
    _Inout_ PPH_IMAGE_COHERENCY_CONTEXT Context,
    _In_opt_ ULONG Rva,
    _In_opt_ PPH_IMGCOHERENCY_SKIP_BYTE_CALLBACK SkipCallback,
    _In_opt_ PVOID SkipCallbackContext
    )
{
    //
    // For the minimum bytes between the buffers increment the coherent bytes
    // for each match.
    //
    if (LeftBuffer && RightBuffer)
    {
        for (ULONG i = 0; i < min(LeftCount, RightCount); i++)
        {
            if (SkipCallback)
            {
                ULONG skip = SkipCallback(Rva + i, SkipCallbackContext);
                if (skip != 0)
                {
                    Context->CoherentBytes += skip;
                    Context->SkippedBytes += skip;
                    Context->TotalBytes += skip;
                    i += (skip - 1);
                    continue;
                }
            }

            Context->TotalBytes++;

            if (LeftBuffer[i] == RightBuffer[i])
            {
                Context->CoherentBytes++;
            }
        }
    }

    //
    // Buffers of mismatched sizes are incoherent over mismatched range.
    // Include any diff in the total bytes.
    //
    if (LeftCount > RightCount)
    {
        Context->TotalBytes += (LeftCount - RightCount);
    }
    else if (LeftCount < RightCount)
    {
        Context->TotalBytes += (RightCount - LeftCount);
    }
}

/**
* Analyzes the image coherency as if it were a native application.
*
* \param[in] ProcessHandle - Handle to the process requires PROCESS_VM_READ.
* \param[in] Rva - Relative virtual address to inspect.
* \param[in] Size - Size of data to inspect from the RVA.
* \param[in] Context - Image coherency context.
* \param[in] SkipCallback - Optional skip callback used to skip analysis.
* \param[in] SkipCallbackContext - Context passed to the skip callback.
*/
VOID PhpAnalyzeImageCoherencyCommonByRva(
    _In_ HANDLE ProcessHandle,
    _In_ ULONG Rva,
    _In_ ULONG Size,
    _In_ PPH_IMAGE_COHERENCY_CONTEXT Context,
    _In_opt_ PPH_IMGCOHERENCY_SKIP_BYTE_CALLBACK SkipCallback,
    _In_opt_ PVOID SkipCallbackContext
    )
{
    BYTE buffer[PAGE_SIZE];
    ULONG remainingBytes;
    ULONG chunk;
    PBYTE fileBytes;
    SIZE_T bytesRead;
    SIZE_T remainingView;
    SIZE_T bytes;
    ULONG rva;

    remainingBytes = Size;
    rva = Rva;

    while (remainingBytes > 0)
    {
        chunk = PAGE_SIZE;
        if (chunk > remainingBytes)
        {
            chunk = remainingBytes;
        }

        //
        // Try to read the remote process
        //
        if (!NT_SUCCESS(Context->ReadVirtualMemory(ProcessHandle,
                                                   PTR_ADD_OFFSET(Context->RemoteImageBase,
                                                                  rva),
                                                   buffer,
                                                   chunk,
                                                   &bytesRead)))
        {
            //
            // Force 0, we'll handle this below
            //
            bytesRead = 0;
        }

        fileBytes = PhMappedImageRvaToVa(&Context->MappedImage, rva, NULL);
        if (fileBytes)
        {
            //
            // Calculate the remaining view from the VA
            //
            remainingView = (SIZE_T)PTR_SUB_OFFSET(Context->MappedImage.Size,
                                                   PTR_SUB_OFFSET(fileBytes,
                                                                  Context->MappedImage.ViewBase));
        }
        else
        {
            //
            // Force 0, we'll handle this below
            //
            remainingView = 0;
        }

        //
        // We will cast to ULONG below, should never have over PAGE_SIZE here.
        //
        bytes = min(bytesRead, remainingView);
        assert(bytes <= PAGE_SIZE);

        //
        // Do the inspection, clamp the bytes to the minimum
        //
        PhpAnalyzeImageCoherencyInspect(fileBytes,
                                        (ULONG)bytes,
                                        buffer,
                                        (ULONG)bytes,
                                        Context,
                                        rva,
                                        SkipCallback,
                                        SkipCallbackContext);

        rva += chunk;
        remainingBytes -= chunk;
    }
}

/**
* Analyzes the image coherency as if it were a native application. And expects
* certain bytes over the range, used for code cave scanning.
*
* \param[in] ProcessHandle - Handle to the process requires PROCESS_VM_READ.
* \param[in] Rva - Relative virtual address to inspect.
* \param[in] Size - Size of data to inspect from the RVA.
* \param[in] Context - Image coherency context.
* \param[in] ExpectedByte - Expected byte in the entire range.
*/
VOID PhpAnalyzeImageCoherencyCommonByRvaExpectBytes(
    _In_ HANDLE ProcessHandle,
    _In_ ULONG Rva,
    _In_ ULONG Size,
    _In_ PPH_IMAGE_COHERENCY_CONTEXT Context,
    _In_ BYTE ExpectedByte
    )
{
    BYTE buffer[PAGE_SIZE];
    BYTE expected[PAGE_SIZE];
    ULONG remainingBytes;
    ULONG chunk;
    SIZE_T bytesRead;
    ULONG rva;

    remainingBytes = Size;
    rva = Rva;

    RtlFillMemory(expected, PAGE_SIZE, ExpectedByte);

    while (remainingBytes > 0)
    {
        chunk = PAGE_SIZE;
        if (chunk > remainingBytes)
        {
            chunk = remainingBytes;
        }

        //
        // Try to read the remote process
        //
        if (NT_SUCCESS(Context->ReadVirtualMemory(ProcessHandle,
                                                  PTR_ADD_OFFSET(Context->RemoteImageBase,
                                                                 rva),
                                                  buffer,
                                                  chunk,
                                                  &bytesRead)))
        {
            assert(bytesRead <= PAGE_SIZE);

            //
            // Do the inspection
            //
            PhpAnalyzeImageCoherencyInspect(expected,
                                            (ULONG)bytesRead,
                                            buffer,
                                            (ULONG)bytesRead,
                                            Context,
                                            rva,
                                            NULL,
                                            NULL);
        }

        rva += chunk;
        remainingBytes -= chunk;
    }
}

/**
* Skip bytes callback to skip bytes during inspection.
*
* \param[in] Rva - Current rva in the range being inspected.
* \param[in] Context - Relocation skip context.
*
* \return Number of bytes to skip, 0 otherwise.
*/
ULONG CALLBACK PhpImgCoherencySkip(
    _In_ ULONG Rva,
    _In_opt_ PVOID Context
    )
{
    PPH_IMAGE_COHERENCY_CONTEXT context;
    PVOID* entry;

    if (!Context)
    {
        return 0;
    }

    context = (PPH_IMAGE_COHERENCY_CONTEXT)Context;

    if (context->MappedImageBaseRva && (context->MappedImageBaseRva == Rva))
    {
        if (context->MappedImage.Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC)
            return RTL_FIELD_SIZE(IMAGE_OPTIONAL_HEADER32, ImageBase);
        else if (context->MappedImage.Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC)
            return RTL_FIELD_SIZE(IMAGE_OPTIONAL_HEADER64, ImageBase);
        else
            return 0;
    }

    if (context->MappedImageReloc)
    {
        //
        // Look up the RVA in our hash table, if we find one we will skip the
        // number of bytes stored in the hash table for that entry.
        //
        entry = PhFindItemSimpleHashtable(context->MappedImageReloc,
                                          PTR_ADD_OFFSET(NULL, Rva));
        if (entry)
        {
            return PtrToUlong(*entry);
        }
    }

    if (context->MappedImageIatRva && (context->MappedImageIatRva == Rva))
    {
        //
        // Skip over the import address table.
        //
        return context->MappedImageIatSize;
    }

    return 0;
}

/**
* Analyzes the image coherency as if it were a native application.
*
* \param[in] ProcessHandle - Handle to the process requires PROCESS_VM_READ.
* \param[in] Context - Image coherency context.
*/
VOID PhpAnalyzeImageCoherencyCommonAsNative(
    _In_ HANDLE ProcessHandle,
    _In_ PPH_IMAGE_COHERENCY_CONTEXT Context
    )
{
    ULONG addressOfEntry = 0;
    PIMAGE_SECTION_HEADER entrySection;

    switch (Context->MappedImage.Magic)
    {
        case IMAGE_NT_OPTIONAL_HDR32_MAGIC:
        {
            addressOfEntry = Context->MappedImage.NtHeaders32->OptionalHeader.AddressOfEntryPoint;
            break;
        }
        case IMAGE_NT_OPTIONAL_HDR64_MAGIC:
        {
            addressOfEntry = Context->MappedImage.NtHeaders->OptionalHeader.AddressOfEntryPoint;
            break;
        }
        default:
        {
            break;
        }
    }

    if (addressOfEntry != 0)
    {
        entrySection = PhMappedImageRvaToSection(&Context->MappedImage, addressOfEntry);
    }
    else
    {
        entrySection = NULL;
    }

    //
    // Here we will inspect each executable section.
    //
    for (ULONG i = 0;
         i < max(Context->MappedImage.NumberOfSections,
                 Context->RemoteMappedImage.NumberOfSections);
         i++)
    {
        if ((i < Context->MappedImage.NumberOfSections) &&
            (i < Context->RemoteMappedImage.NumberOfSections))
        {
            PIMAGE_SECTION_HEADER mappedSection;
            PIMAGE_SECTION_HEADER remoteMappedSection;

            mappedSection = &Context->MappedImage.Sections[i];
            remoteMappedSection = &Context->MappedImage.Sections[i];

            if (PhpShouldScanSection(Context->Type, mappedSection) ||
                PhpShouldScanSection(Context->Type, remoteMappedSection))
            {
                ULONG size;
                SIZE_T prevTotal;
                SIZE_T bytesInspected;
                SIZE_T prevSkipped;
                SIZE_T bytesSkipped;

                size = PhpGetSectionScanSize(Context->Type, mappedSection);

                prevTotal = Context->TotalBytes;
                prevSkipped = Context->SkippedBytes;

                PhpAnalyzeImageCoherencyCommonByRva(ProcessHandle,
                                                    mappedSection->VirtualAddress,
                                                    size,
                                                    Context,
                                                    PhpImgCoherencySkip,
                                                    Context);

                bytesInspected = (Context->TotalBytes - prevTotal);
                bytesSkipped = (Context->SkippedBytes - prevSkipped);

                if (entrySection == mappedSection)
                {
                    ULONG length;
                    //
                    // Make sure we scanned enough of the entry point.
                    // If not, force scan that part.
                    //
                    length = min(entrySection->SizeOfRawData,
                                 entrySection->Misc.VirtualSize);
                    length -= (addressOfEntry - entrySection->VirtualAddress);
                    if (length > PH_IMGCOHERENCY_MIN_ENTRY_INSPECT)
                    {
                        length = PH_IMGCOHERENCY_MIN_ENTRY_INSPECT;
                    }

                    if ((bytesInspected + bytesSkipped) <
                        (((ULONGLONG)addressOfEntry - entrySection->VirtualAddress) + length))
                    {
                        PhpAnalyzeImageCoherencyCommonByRva(ProcessHandle,
                                                            addressOfEntry,
                                                            length,
                                                            Context,
                                                            PhpImgCoherencySkip,
                                                            Context);
                    }
                }

                //
                // If we're doing a full scan, inspect for possible code caves
                // beyond on-disk content.
                // If VirtualAddress is greater than SizeOfRawData the loader
                // will zero initialize the bytes to extend the mapping.
                // If there is non-zero content here someone has written
                // into the code cave.
                //
                if ((Context->Type == PhImageCoherencyFull) &&
                    ((mappedSection->Characteristics & IMAGE_SCN_MEM_EXECUTE) != 0) &&
                    (mappedSection->Misc.VirtualSize > mappedSection->SizeOfRawData))
                {
                    PhpAnalyzeImageCoherencyCommonByRvaExpectBytes(
                        ProcessHandle,
                        mappedSection->VirtualAddress + mappedSection->SizeOfRawData,
                        mappedSection->Misc.VirtualSize - mappedSection->SizeOfRawData,
                        Context,
                        0x00);
                }
            }
        }
        else
        {
            //
            // There are a mismatched number of sections
            // Inflate the total bytes
            //
            if (i < Context->MappedImage.NumberOfSections)
            {
                Context->TotalBytes += min(Context->MappedImage.Sections[i].SizeOfRawData,
                                           Context->MappedImage.Sections[i].Misc.VirtualSize);
            }
            else
            {
                Context->TotalBytes += min(Context->RemoteMappedImage.Sections[i].SizeOfRawData,
                                           Context->RemoteMappedImage.Sections[i].Misc.VirtualSize);
            }
        }
    }
}

/**
* Analyzes the image coherency as if it were a managed (.NET) application.
*
* \param[in] ProcessHandle - Handle to the process requires PROCESS_VM_READ.
* \param[in] Context - Image coherency context.
*/
VOID PhpAnalyzeImageCoherencyCommonAsManaged(
    _In_ HANDLE ProcessHandle,
    _In_ PPH_IMAGE_COHERENCY_CONTEXT Context
    )
{
    PIMAGE_DATA_DIRECTORY dataDirectory;
    PIMAGE_COR20_HEADER dotNet;

    //
    // We will check for coherency across two blocks here
    //     1. The COR20 header
    //     2. The .NET Meta Data
    //

    //
    // Get the COM32 directory bytes
    //
    if (!NT_SUCCESS(PhGetMappedImageDataEntry(&Context->MappedImage,
                                              IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR,
                                              &dataDirectory)))
    {
        return;
    }

    //
    // Inspect the COR20 header
    //
    PhpAnalyzeImageCoherencyCommonByRva(ProcessHandle,
                                        dataDirectory->VirtualAddress,
                                        dataDirectory->Size,
                                        Context,
                                        NULL,
                                        NULL);

    //
    // Get the .NET MetaData
    //
    dotNet = PhMappedImageRvaToVa(&Context->MappedImage,
                                  dataDirectory->VirtualAddress,
                                  NULL);
    if (!dotNet ||
        (dotNet->MetaData.Size == 0) ||
        !dotNet->MetaData.VirtualAddress)
    {
        return;
    }

    //
    // Inspect the .NET MetaData
    //
    PhpAnalyzeImageCoherencyCommonByRva(ProcessHandle,
                                        dotNet->MetaData.VirtualAddress,
                                        dotNet->MetaData.Size,
                                        Context,
                                        NULL,
                                        NULL);
}

/**
* Checks if the image is a .NET application.
*
* \param[in] Context - Image coherency context.
*
* \return TRUE if the image is a .NET application, FALSE otherwise.
*/
BOOLEAN PhpAnalyzeImageCoherencyIsDotNet (
    _In_ PPH_IMAGE_COHERENCY_CONTEXT Context
    )
{
    PIMAGE_DATA_DIRECTORY dataDirectory;
    PIMAGE_COR20_HEADER dotNet;
    PULONG dotNetMagic;

    //
    // Get the com descriptor directly, if it doesn't exist it isn't .NET
    //
    if (!NT_SUCCESS(PhGetMappedImageDataEntry(&Context->MappedImage,
                                              IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR,
                                              &dataDirectory)))
    {
        return FALSE;
    }

    //
    // Check for the COR20 header
    //
    dotNet = PhMappedImageRvaToVa(&Context->MappedImage,
                                  dataDirectory->VirtualAddress,
                                  NULL);
    if (!dotNet || (dotNet->cb != sizeof(IMAGE_COR20_HEADER)))
    {
        return FALSE;
    }

    dotNetMagic = PhMappedImageRvaToVa(&Context->MappedImage,
                                       dotNet->MetaData.VirtualAddress,
                                       NULL);
    //
    // If we can locate the magic number and it equal the .NET magic then we
    // are reasonably confident it is .NET.
    //
    return (dotNetMagic && *dotNetMagic == 0x424A5342);
}

/**
* Common analysis for image coherency.
*
* \param[in] ProcessHandle - Handle to the process requires PROCESS_VM_READ.
* \param[in] Context - Image coherency context.
*/
VOID PhpAnalyzeImageCoherencyCommon(
    _In_ HANDLE ProcessHandle,
    _In_ PPH_IMAGE_COHERENCY_CONTEXT Context
    )
{
    //
    // Loop over the number of sections and include them in the calculation
    //
    for (ULONG i = 0;
         i < max(Context->MappedImage.NumberOfSections,
                 Context->RemoteMappedImage.NumberOfSections);
         i++)
    {
        if ((i < Context->MappedImage.NumberOfSections) &&
            (i < Context->RemoteMappedImage.NumberOfSections))
        {
            PhpAnalyzeImageCoherencyInspect((PBYTE)&Context->MappedImage.Sections[i],
                                            sizeof(IMAGE_SECTION_HEADER),
                                            (PBYTE)&Context->RemoteMappedImage.Sections[i],
                                            sizeof(IMAGE_SECTION_HEADER),
                                            Context,
                                            0,
                                            NULL,
                                            NULL);
        }
        else
        {
            //
            // There are a mismatched number of sections
            // Inflate the total bytes
            //
            Context->TotalBytes += sizeof(IMAGE_SECTION_HEADER);
        }
    }

    //
    // If we detect the on-disk image is a .NET application use the managed
    // path, this will assume the remote image is a .NET app, if it isn't
    // we'll detect lots of incoherent bytes.
    //
    // Otherwise use the native path and inspect by the supplied RVAs
    //
    if (PhpAnalyzeImageCoherencyIsDotNet(Context))
    {
        PhpAnalyzeImageCoherencyCommonAsManaged(ProcessHandle, Context);
    }
    else
    {
        PhpAnalyzeImageCoherencyCommonAsNative(ProcessHandle, Context);
    }
}

/**
* Analyzes image coherency for 32 bit images.
*
* \param[in] ProcessHandle - Handle to the process requires PROCESS_VM_READ.
* \param[in] Context - Image coherency context.
*
* \return Success status or failure.
*/
NTSTATUS PhpAnalyzeImageCoherencyNt32(
    _In_ HANDLE ProcessHandle,
    _In_ PPH_IMAGE_COHERENCY_CONTEXT Context
    )
{
    PIMAGE_OPTIONAL_HEADER32 fileOptHeader;
    PIMAGE_OPTIONAL_HEADER32 procOptHeader;

    fileOptHeader = (PIMAGE_OPTIONAL_HEADER32)&Context->MappedImage.NtHeaders32->OptionalHeader;
    procOptHeader = (PIMAGE_OPTIONAL_HEADER32)&Context->RemoteMappedImage.NtHeaders32->OptionalHeader;

    //
    // Inspect the header
    //
    PhpAnalyzeImageCoherencyInspect((PBYTE)Context->MappedImage.NtHeaders32,
                                    UFIELD_OFFSET(IMAGE_NT_HEADERS32, OptionalHeader),
                                    (PBYTE)Context->RemoteMappedImage.NtHeaders32,
                                    UFIELD_OFFSET(IMAGE_NT_HEADERS32, OptionalHeader),
                                    Context,
                                    PtrToUlong(PTR_SUB_OFFSET(Context->MappedImage.NtHeaders32, Context->MappedImage.ViewBase)),
                                    PhpImgCoherencySkip,
                                    Context);

    //
    // Inspect the optional header
    //
    PhpAnalyzeImageCoherencyInspect((PBYTE)fileOptHeader,
                                    sizeof(IMAGE_OPTIONAL_HEADER32),
                                    (PBYTE)procOptHeader,
                                    sizeof(IMAGE_OPTIONAL_HEADER32),
                                    Context,
                                    PtrToUlong(PTR_SUB_OFFSET(fileOptHeader, Context->MappedImage.ViewBase)),
                                    PhpImgCoherencySkip,
                                    Context);

    //
    // Do the common inspection
    //
    PhpAnalyzeImageCoherencyCommon(ProcessHandle, Context);

    if (fileOptHeader->CheckSum != procOptHeader->CheckSum)
    {
        return STATUS_INVALID_IMAGE_HASH;
    }
    return STATUS_SUCCESS;
}

/**
* Analyzes image coherency for 64 bit images.
*
* \param[in] ProcessHandle - Handle to the process requires PROCESS_VM_READ.
* \param[in] Context - Image coherency context.
*
* \return Success status or failure.
*/
NTSTATUS PhpAnalyzeImageCoherencyNt64(
    _In_ HANDLE ProcessHandle,
    _In_ PPH_IMAGE_COHERENCY_CONTEXT Context
    )
{
    PIMAGE_OPTIONAL_HEADER64 fileOptHeader;
    PIMAGE_OPTIONAL_HEADER64 procOptHeader;

    fileOptHeader = (PIMAGE_OPTIONAL_HEADER64)&Context->MappedImage.NtHeaders->OptionalHeader;
    procOptHeader = (PIMAGE_OPTIONAL_HEADER64)&Context->RemoteMappedImage.NtHeaders->OptionalHeader;

    //
    // Inspect the header
    //
    PhpAnalyzeImageCoherencyInspect((PBYTE)Context->MappedImage.NtHeaders,
                                    UFIELD_OFFSET(IMAGE_NT_HEADERS64, OptionalHeader),
                                    (PBYTE)Context->RemoteMappedImage.NtHeaders,
                                    UFIELD_OFFSET(IMAGE_NT_HEADERS64, OptionalHeader),
                                    Context,
                                    PtrToUlong(PTR_SUB_OFFSET(Context->MappedImage.NtHeaders, Context->MappedImage.ViewBase)),
                                    PhpImgCoherencySkip,
                                    Context);
    //
    // And the optional header
    //
    PhpAnalyzeImageCoherencyInspect((PBYTE)fileOptHeader,
                                    sizeof(IMAGE_OPTIONAL_HEADER64),
                                    (PBYTE)procOptHeader,
                                    sizeof(IMAGE_OPTIONAL_HEADER64),
                                    Context,
                                    PtrToUlong(PTR_SUB_OFFSET(fileOptHeader, Context->MappedImage.ViewBase)),
                                    PhpImgCoherencySkip,
                                    Context);

    //
    // Do the common inspection
    //
    PhpAnalyzeImageCoherencyCommon(ProcessHandle, Context);

    if (fileOptHeader->CheckSum != procOptHeader->CheckSum)
    {
        //
        // The optional header checksum doesn't match the in memory checksum
        // return status to reflect that. We could inflate the TotalBytes here
        // but choose not.
        //
        return STATUS_INVALID_IMAGE_HASH;
    }
    return STATUS_SUCCESS;
}

/**
* Inspects the process image coherency compared to the file on disk.
*
* \param[in] ProcessHandle - Handle to the process requires PROCESS_VM_READ.
* \param[in] Context - Image coherency context.
* \param[out] ImageCoherency Image coherency value between 0 and 1. This
* indicates how similar the image on-disk is compared to what is mapped into
* the process. A value of 1 means coherent while a value lower than 1
* indicates how incoherent the image is.
*
* \return Status indicating the coherency calculation, note errors may indicate
* partial success.
* STATUS_SUCCESS The coherency calculation was successful.
* STATUS_INVALID_IMAGE_HASH The coherency calculation was successful or
* partially successful and unusually incoherent.
* STATUS_INVALID_IMAGE_FORMAT The coherency calculation was successful or
* partially successful and unusually incoherent.
* STATUS_SUBSYSTEM_NOT_PRESENT The coherency calculation was successful or
* partially successful and unusually incoherent.
* All other errors are failures to calculate.
*/
NTSTATUS PhpInspectForImageCoherency(
    _In_ HANDLE ProcessHandle,
    _In_ PPH_IMAGE_COHERENCY_CONTEXT Context,
    _Out_ PFLOAT ImageCoherency
   )
{
    NTSTATUS status;

    //
    // Creating the coherency context is best-effort and will store the status
    // in the context. We can infer some state if one fails, do so here.
    //
    if (!NT_SUCCESS(Context->MappedImageStatus) ||
        !NT_SUCCESS(Context->RemoteMappedImageStatus))
    {
        if (NT_SUCCESS(Context->RemoteMappedImageStatus) &&
            ((Context->MappedImageStatus == STATUS_INVALID_IMAGE_FORMAT) ||
             (Context->MappedImageStatus == STATUS_INVALID_IMAGE_HASH) ||
             (Context->MappedImageStatus == STATUS_IMAGE_SUBSYSTEM_NOT_PRESENT)))
        {
            //
            // If we succeeded to map the remote image but the on-disk image
            // is inherently incoherent, return the status from the on-disk
            // mapping.
            //
            status = Context->MappedImageStatus;
        }
        else
        {
            //
            // If we failed for any other reason, return the error from the
            // remote mapping.
            //
            status = Context->RemoteMappedImageStatus;
        }
        goto CleanupExit;
    }

    if (Context->MappedImage.Magic != Context->RemoteMappedImage.Magic)
    {
        //
        // The image types don't match. This is incoherent but we have one more
        // case to check. .NET can change this in the loaded PE to run "AnyCPU"
        // in some cases, if it's a .NET mapping do the managed calculation.
        //
        if (PhpAnalyzeImageCoherencyIsDotNet(Context))
        {
            PhpAnalyzeImageCoherencyCommonAsManaged(ProcessHandle, Context);
        }
        status = STATUS_INVALID_IMAGE_FORMAT;
        goto CleanupExit;
    }

    switch (Context->MappedImage.Magic)
    {
        case IMAGE_NT_OPTIONAL_HDR32_MAGIC:
        {
            status = PhpAnalyzeImageCoherencyNt32(ProcessHandle, Context);
            break;
        }
        case IMAGE_NT_OPTIONAL_HDR64_MAGIC:
        {
            status = PhpAnalyzeImageCoherencyNt64(ProcessHandle, Context);
            break;
        }
        default:
        {
            //
            // Not supporting ELF for WSL yet. Note however, if the image type
            // is mismatched above we will still reflect that
            // (e.g. ELF<->non-ELF). But we don't yet support coherency checks
            // for ELF<->ELF. The remote image mapping should be updated to
            // handle remote mapping ELF.
            //
            status = STATUS_NOT_IMPLEMENTED;
            break;
        }
    }

CleanupExit:

    if (Context->TotalBytes)
    {
        *ImageCoherency = (FLOAT)Context->CoherentBytes / (FLOAT)Context->TotalBytes;
    }
    else
    {
        *ImageCoherency = 0.0f;
    }

    return status;
}

/**
* Inspects the process image coherency compared to the file on disk.
*
* \param[in] FileName Win32 path to the image file on disk.
* \param[in] ProcessId Process ID of the active process to compare to.
* \param[in] Type - Image coherency scan type.
* \param[out] ImageCoherency Image coherency value between 0 and 1. This
* indicates how similar the image on-disk is compared to what is mapped into
* the process. A value of 1 means coherent while a value lower than 1
* indicates how incoherent the image is.
*
* \return Status indicating the coherency calculation, note errors may indicate
* partial success.
* STATUS_SUCCESS The coherency calculation was successful.
* STATUS_INVALID_IMAGE_HASH The coherency calculation was successful or
* partially successful and unusually incoherent.
* STATUS_INVALID_IMAGE_FORMAT The coherency calculation was successful or
* partially successful and unusually incoherent.
* STATUS_SUBSYSTEM_NOT_PRESENT The coherency calculation was successful or
* partially successful and unusually incoherent.
* All other errors are failures to calculate.
*/
NTSTATUS PhGetProcessImageCoherency(
    _In_ PPH_STRING FileName,
    _In_ HANDLE ProcessId,
    _In_ PH_IMAGE_COHERENCY_SCAN_TYPE Type,
    _Out_ PFLOAT ImageCoherency
    )
{
    NTSTATUS status;
    HANDLE processHandle;
    PPH_IMAGE_COHERENCY_CONTEXT context;
    PVOID remoteImageBase;

    context = NULL;
    remoteImageBase = NULL;

    *ImageCoherency = 0.0f;

    status = PhOpenProcess(&processHandle,
                           PROCESS_QUERY_LIMITED_INFORMATION | PROCESS_VM_READ,
                           ProcessId);
    if (!NT_SUCCESS(status))
    {
        processHandle = NULL;
        goto CleanupExit;
    }

    //
    // Get the remote image base
    //
    status = PhGetProcessImageBaseAddress(processHandle, &remoteImageBase);
    if (!NT_SUCCESS(status))
    {
        remoteImageBase = NULL;
    }

    context = PhpCreateImageCoherencyContext(Type,
                                             FileName,
                                             processHandle,
                                             remoteImageBase,
                                             status,
                                             NtReadVirtualMemory);

    status = PhpInspectForImageCoherency(processHandle,
                                         context,
                                         ImageCoherency);

CleanupExit:

    PhpFreeImageCoherencyContext(context);

    if (processHandle)
    {
        NtClose(processHandle);
    }

    return status;
}

/**
* Inspects a module image coherency compared to the file on disk.
*
* \param[in] FileName Win32 path to the image file on disk.
* \param[in] ProcessHandle - Handle to the process where the module is mapped
* requires PROCESS_VM_READ.
* \param[in] ImageBaseAddress - Base address of the image.
* \param[in] IsKernelModule - Notes if this is a kernel module.
* \param[in] Type - Image coherency scan type.
* \param[out] ImageCoherency Image coherency value between 0 and 1. This
* indicates how similar the image on-disk is compared to what is mapped into
* the process. A value of 1 means coherent while a value lower than 1
* indicates how incoherent the image is.
*
* \return Status indicating the coherency calculation, note errors may indicate
* partial success.
* STATUS_SUCCESS The coherency calculation was successful.
* STATUS_INVALID_IMAGE_HASH The coherency calculation was successful or
* partially successful and unusually incoherent.
* STATUS_INVALID_IMAGE_FORMAT The coherency calculation was successful or
* partially successful and unusually incoherent.
* STATUS_SUBSYSTEM_NOT_PRESENT The coherency calculation was successful or
* partially successful and unusually incoherent.
* All other errors are failures to calculate.
*/
NTSTATUS PhGetProcessModuleImageCoherency(
    _In_ PPH_STRING FileName,
    _In_ HANDLE ProcessHandle,
    _In_ PVOID ImageBaseAddress,
    _In_ BOOLEAN IsKernelModule,
    _In_ PH_IMAGE_COHERENCY_SCAN_TYPE Type,
    _Out_ PFLOAT ImageCoherency
    )
{
    NTSTATUS status;
    PPH_IMAGE_COHERENCY_CONTEXT context;

    *ImageCoherency = 0.0f;

    context = PhpCreateImageCoherencyContext(Type,
                                             FileName,
                                             ProcessHandle,
                                             ImageBaseAddress,
                                             STATUS_UNSUCCESSFUL,
                                             IsKernelModule ? KphReadVirtualMemoryUnsafe : NtReadVirtualMemory);

    status = PhpInspectForImageCoherency(ProcessHandle,
                                         context,
                                         ImageCoherency);

    PhpFreeImageCoherencyContext(context);

    return status;
}
