/*
 * Process Hacker -
 *   Image Coherency
 *
 * Copyright (C) 2020 jxy-s
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
#include <ph.h>
#include <mapimg.h>

/**
* Process image coherency context, used during the calculation of the process
* image coherency.
*/
typedef struct _PH_PROCESS_IMAGE_COHERENCY_CONTEXT
{
    NTSTATUS MappedImageStatus;               /**< Status of initializing MappedImage */
    PH_MAPPED_IMAGE MappedImage;              /**< On-disk image mapping */

    NTSTATUS RemoteMappedImageStatus;         /**< Status of initializing RemoteMappedImage */
    PH_REMOTE_MAPPED_IMAGE RemoteMappedImage; /**< Process remote image mapping */

    PVOID RemoteImageBase;                    /**< Remote image base address */

} PH_PROCESS_IMAGE_COHERENCY_CONTEXT, *PPH_PROCESS_IMAGE_COHERENCY_CONTEXT;

/**
* Frees the process image coherency context.
*
* \param[in] Context - Context to free, may be NULL.
*/
VOID PhpFreeProcessImageCoherencyContext(
    _In_opt_ PPH_PROCESS_IMAGE_COHERENCY_CONTEXT Context
    )
{
    if (Context)
    {
        PhUnloadMappedImage(&Context->MappedImage);
        PhUnloadRemoteMappedImage(&Context->RemoteMappedImage);

        PhFree(Context);
    }
}

/**
* Created the process image coherency context. This is done best-effort and
* the status is stored in the context.
*
* \param[in] FileName - Win32 file name of the image to inspect.
* \param[in] ProcessHandle - Handle to process to inspect. Requires
* PROCESS_QUERY_LIMITED_INFORMATION and PROCESS_VM_READ.
*
* \return Pointer to newly allocated image coherency context, or NULL on
* allocation failure. The created context must be passed to
* PhpFreeProcessImageCoherencyContext to free.
*/
PPH_PROCESS_IMAGE_COHERENCY_CONTEXT PhpCreateProcessImageCoherencyContext(
    _In_ PWSTR FileName,
    _In_ HANDLE ProcessHandle
    )
{
    NTSTATUS status;
    PPH_PROCESS_IMAGE_COHERENCY_CONTEXT context;
    PROCESS_BASIC_INFORMATION basicInfo;

    //
    // This is best-effort context creation, we don't fail if the mapping
    // fails - the caller of the API may make decisions based on success of
    // each
    //

    context = PhAllocateZero(sizeof(PH_PROCESS_IMAGE_COHERENCY_CONTEXT));
    if (!context)
    {
        return NULL;
    }

    //
    // Map the on-disk image
    //
    context->MappedImageStatus = PhLoadMappedImageEx(
        FileName,
        NULL,
        &context->MappedImage
        );

    //
    // Get the remote image base 
    //
    status = PhGetProcessBasicInformation(ProcessHandle, &basicInfo);
    if (!NT_SUCCESS(status))
    {
        context->RemoteMappedImageStatus = status;
        return context;
    }

    status = NtReadVirtualMemory(ProcessHandle,
                                 PTR_ADD_OFFSET(basicInfo.PebBaseAddress,
                                                FIELD_OFFSET(PEB, ImageBaseAddress)),
                                 &context->RemoteImageBase,
                                 sizeof(PVOID),
                                 NULL);
    if (!NT_SUCCESS(status))
    {
        context->RemoteMappedImageStatus = status;
        return context;
    }

    //
    // Map the remote image 
    //
    context->RemoteMappedImageStatus =
        PhLoadRemoteMappedImage(ProcessHandle,
                                context->RemoteImageBase,
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
* \param[in,out] CoherentBytes - Incremented by the number of matching bytes
* between the left and right buffers.
* \param[in,out] TotalBytes - Incremented by the maximum number of bytes
* available to inspect.
*/
VOID PhpAnalyzeImageCoherencyInsepct(
    _In_opt_ PBYTE LeftBuffer,
    _In_ SIZE_T LeftCount,
    _In_opt_ PBYTE RightBuffer,
    _In_ SIZE_T RightCount,
    _Inout_ PSIZE_T CoherentBytes,
    _Inout_ PSIZE_T TotalBytes
    )
{
    //
    // For the minimum bytes between the buffers increment the coherent bytes
    // for each match.
    //
    if (LeftBuffer && RightBuffer)
    {
        for (SIZE_T i = 0; i < min(LeftCount, RightCount); i++)
        {
            if (LeftBuffer[i] == RightBuffer[i])
            {
                (*CoherentBytes)++;
            }
        }
    }

    //
    // Add the maximum to the total bytes. Buffers of mismatched sizes are
    // incoherent over mismatched range. Include the maximum of the two in
    // the total bytes.
    //
    *TotalBytes += max(LeftCount, RightCount);
}

/**
* Analyzes the image coherency as if it were a native application.
* 
* \param[in] ProcessHandle - Handle to the process requires PROCESS_VM_READ.
* \param[in] Rva - Relative virtual address to inspect.
* \param[in] Buffer - Supplied buffer to use, this is an optimization to
* minimize re-allocations in some situations. This buffer is used to store a
* specified number of bytes read from the process at the supplied RVA.
* \param[in] Size - Size of the supplied buffer.
* \param[in] Context - Process image coherency context.
* \param[in,out] CoherentBytes - Updated during inspection to contain the
* number of coherent bytes.
* \param[in,out] TotalBytes - Updated during inspection to contain the total
* number of bytes inspected.
*/
VOID PhpAnalyzeImageCoherencyCommonByRva(
    _In_ HANDLE ProcessHandle,
    _In_ DWORD Rva,
    _In_ PBYTE Buffer,
    _In_ ULONG Size,
    _In_ PPH_PROCESS_IMAGE_COHERENCY_CONTEXT Context,
    _Inout_ PSIZE_T CoherentBytes,
    _Inout_ PSIZE_T TotalBytes
    )
{
    PBYTE fileBytes;
    SIZE_T bytesRead;
    SIZE_T remainingView;

    //
    // Try to read the remote process
    //
    if (!NT_SUCCESS(NtReadVirtualMemory(ProcessHandle,
                                        PTR_ADD_OFFSET(Context->RemoteImageBase,
                                                       Rva),
                                        Buffer,
                                        Size,
                                        &bytesRead)))
    {
        //
        // Force 0, we'll handle this below 
        //
        bytesRead = 0;
    }

    fileBytes = PhMappedImageRvaToVa(&Context->MappedImage, Rva, NULL);
    if (fileBytes)
    {
        //
        // Calculate the remaining view from the VA
        //
        remainingView = (ULONG_PTR)PTR_SUB_OFFSET(Context->MappedImage.Size,
                                                  PTR_SUB_OFFSET(fileBytes,
                                                                 Context->MappedImage.ViewBase));
    }
    else
    {
        //
        // Force 0, we'll having this below 
        //
        remainingView = 0;
    }

    //
    // Do the inspection, clamp the bytes to the minimum 
    //
    PhpAnalyzeImageCoherencyInsepct(fileBytes,
                                    min(bytesRead, remainingView),
                                    Buffer,
                                    min(bytesRead, remainingView),
                                    CoherentBytes,
                                    TotalBytes);
}

/**
* Analyzes the image coherency as if it were a native application.
* 
* \param[in] ProcessHandle - Handle to the process requires PROCESS_VM_READ.
* \param[in] AddressOfEntryPoint - Entry point RVA.
* \param[in] BaseOfCode - Base of code (.text) section RVA.
* \param[in] Context - Process image coherency context.
* \param[in,out] CoherentBytes - Updated during inspection to contain the
* number of coherent bytes.
* \param[in,out] TotalBytes - Updated during inspection to contain the total
* number of bytes inspected.
*/
VOID PhpAnalyzeImageCoherencyCommonAsNative(
    _In_ HANDLE ProcessHandle,
    _In_ DWORD AddressOfEntryPoint,
    _In_ DWORD BaseOfCode,
    _In_ PPH_PROCESS_IMAGE_COHERENCY_CONTEXT Context,
    _Inout_ PSIZE_T CoherentBytes,
    _Inout_ PSIZE_T TotalBytes
    )
{
    PBYTE buffer;

    //
    // Here we will inspect each provided RVA
    //    1. The entry point
    //    2. The base of the code (.text) section
    //

    //
    // The RVA helper function uses a buffer we supply. For each RVA we will
    // try to inspect a page - make one
    //
    // We only inspect 1 page for each for native code block for simplicity.
    // Once the native PE is mapped parts the code are "fixed up" by the
    // loader. It would take substantial work to "unmap" the remote image
    // to something more coherent and would be time-complex. This gets us
    // most of the way at the cost accuracy - but it's good enough.
    //
    buffer = PhAllocateZero(PAGE_SIZE);
    if (!buffer)
    {
        goto CleanupExit;
    }

    //
    // Inspect the entry point bytes
    //
    PhpAnalyzeImageCoherencyCommonByRva(ProcessHandle,
                                        AddressOfEntryPoint,
                                        buffer,
                                        PAGE_SIZE,
                                        Context,
                                        CoherentBytes,
                                        TotalBytes);

    //
    // Inspect the base of the code (.text) section
    //
    PhpAnalyzeImageCoherencyCommonByRva(ProcessHandle,
                                        BaseOfCode,
                                        buffer,
                                        PAGE_SIZE,
                                        Context,
                                        CoherentBytes,
                                        TotalBytes);

CleanupExit:

    if (buffer)
    {
        PhFree(buffer);
    }
}

/**
* Analyzes the image coherency as if it were a managed (.NET) application.
* 
* \param[in] ProcessHandle - Handle to the process requires PROCESS_VM_READ.
* \param[in] Context - Process image coherency context.
* \param[in,out] CoherentBytes - Updated during inspection to contain the
* number of coherent bytes.
* \param[in,out] TotalBytes - Updated during inspection to contain the total
* number of bytes inspected.
*/
VOID PhpAnalyzeImageCoherencyCommonAsManaged(
    _In_ HANDLE ProcessHandle,
    _In_ PPH_PROCESS_IMAGE_COHERENCY_CONTEXT Context,
    _Inout_ PSIZE_T CoherentBytes,
    _Inout_ PSIZE_T TotalBytes
    )
{
    PIMAGE_DATA_DIRECTORY dataDirectory;
    PIMAGE_COR20_HEADER dotNet;
    PBYTE buffer;

    buffer = NULL;

    //
    // We will check for coherency accross two blocks here
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
        goto CleanupExit;
    }

    //
    // The helper by RVA function function uses a buffer we supply - make one
    //
    buffer = PhAllocateZero(dataDirectory->Size);
    if (!buffer)
    {
        goto CleanupExit;
    }

    //
    // Insepct the COR20 header
    //
    PhpAnalyzeImageCoherencyCommonByRva(ProcessHandle,
                                        dataDirectory->VirtualAddress,
                                        buffer,
                                        dataDirectory->Size,
                                        Context,
                                        CoherentBytes,
                                        TotalBytes);

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
        goto CleanupExit;
    }

    //
    // Reallocate the buffer for the RVA helper function to use
    //
    buffer = PhReAllocate(buffer, dotNet->MetaData.Size);
    if (!buffer)
    {
        goto CleanupExit;
    }

    //
    // Inspect the .NET MetaData
    //
    PhpAnalyzeImageCoherencyCommonByRva(ProcessHandle,
                                        dotNet->MetaData.VirtualAddress,
                                        buffer,
                                        dotNet->MetaData.Size,
                                        Context,
                                        CoherentBytes,
                                        TotalBytes);

CleanupExit:

    if (buffer)
    {
        PhFree(buffer);
    }

    return;
}

/**
* Checks if the image is a .NET application.
* 
* \param[in] Context - Process image coherency context.
*
* \return TRUE if the image is a .NET application, FALSE otherwise.
*/
BOOLEAN PhpAnalyzeImageCoherencyIsDotNet (
    _In_ PPH_PROCESS_IMAGE_COHERENCY_CONTEXT Context
    )
{
    PIMAGE_DATA_DIRECTORY dataDirectory;
    PIMAGE_COR20_HEADER dotNet;
    PDWORD dotNetMagic;

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
* \param[in] AddressOfEntryPoint - Entry point RVA.
* \param[in] BaseOfCode - Base of code (.text) section RVA.
* \param[in] Context - Process image coherency context.
* \param[in,out] CoherentBytes - Updated during inspection to contain the
* number of coherent bytes.
* \param[in,out] TotalBytes - Updated during inspection to contain the total
* number of bytes inspected.
*
* \return Success status or failure.
*/
VOID PhpAnalyzeImageCoherencyCommon(
    _In_ HANDLE ProcessHandle,
    _In_ DWORD AddressOfEntryPoint,
    _In_ DWORD BaseOfCode,
    _In_ PPH_PROCESS_IMAGE_COHERENCY_CONTEXT Context,
    _Inout_ PSIZE_T CoherentBytes,
    _Inout_ PSIZE_T TotalBytes
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
            PhpAnalyzeImageCoherencyInsepct((PBYTE)&Context->MappedImage.Sections[i],
                                            sizeof(IMAGE_SECTION_HEADER),
                                            (PBYTE)&Context->RemoteMappedImage.Sections[i],
                                            sizeof(IMAGE_SECTION_HEADER),
                                            CoherentBytes,
                                            TotalBytes);
        }
        else
        {
            //
            // There are a missmatched number of sections
            // Inflate the total bytes
            //
            *TotalBytes += sizeof(IMAGE_SECTION_HEADER);
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
        PhpAnalyzeImageCoherencyCommonAsManaged(ProcessHandle,
                                                Context,
                                                CoherentBytes,
                                                TotalBytes);
    }
    else
    {
        PhpAnalyzeImageCoherencyCommonAsNative(ProcessHandle,
                                               AddressOfEntryPoint,
                                               BaseOfCode,
                                               Context,
                                               CoherentBytes,
                                               TotalBytes);
    }
}

/**
* Analyzes image coherency for 32 bit images.
* 
* \param[in] ProcessHandle - Handle to the process requires PROCESS_VM_READ.
* \param[in] Context - Process image coherency context.
* \param[in,out] CoherentBytes - Updated during inspection to contain the
* number of coherent bytes.
* \param[in,out] TotalBytes - Updated during inspection to contain the total
* number of bytes inspected.
*
* \return Success status or failure.
*/
NTSTATUS
PhpAnalyzeImageCoherencyNt32(
    _In_ HANDLE ProcessHandle,
    _In_ PPH_PROCESS_IMAGE_COHERENCY_CONTEXT Context,
    _Inout_ PSIZE_T CoherentBytes,
    _Inout_ PSIZE_T TotalBytes 
    )
{
    PIMAGE_OPTIONAL_HEADER32 fileOptHeader;
    PIMAGE_OPTIONAL_HEADER32 procOptHeader;

    fileOptHeader = (PIMAGE_OPTIONAL_HEADER32)&Context->MappedImage.NtHeaders32->OptionalHeader;
    procOptHeader = (PIMAGE_OPTIONAL_HEADER32)&Context->RemoteMappedImage.NtHeaders32->OptionalHeader;

    //
    // Inspect the header
    //
    PhpAnalyzeImageCoherencyInsepct((PBYTE)&Context->MappedImage.NtHeaders32->FileHeader,
                                    sizeof(IMAGE_NT_HEADERS32),
                                    (PBYTE)&Context->RemoteMappedImage.NtHeaders32->FileHeader,
                                    sizeof(IMAGE_NT_HEADERS32),
                                    CoherentBytes,
                                    TotalBytes);

    //
    // Inspect the optional header
    //
    PhpAnalyzeImageCoherencyInsepct((PBYTE)fileOptHeader,
                                    sizeof(IMAGE_OPTIONAL_HEADER32),
                                    (PBYTE)procOptHeader,
                                    sizeof(IMAGE_OPTIONAL_HEADER32),
                                    CoherentBytes,
                                    TotalBytes);

    //
    // Do the common inspection 
    //
    PhpAnalyzeImageCoherencyCommon(ProcessHandle,
                                   fileOptHeader->AddressOfEntryPoint,
                                   fileOptHeader->BaseOfCode,
                                   Context,
                                   CoherentBytes,
                                   TotalBytes);

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
* \param[in] Context - Process image coherency context.
* \param[in,out] CoherentBytes - Updated during inspection to contain the
* number of coherent bytes.
* \param[in,out] TotalBytes - Updated during inspection to contain the total
* number of bytes inspected.
*
* \return Success status or failure.
*/
NTSTATUS
PhpAnalyzeImageCoherencyNt64(
    _In_ HANDLE ProcessHandle,
    _In_ PPH_PROCESS_IMAGE_COHERENCY_CONTEXT Context,
    _Inout_ PSIZE_T CoherentBytes,
    _Inout_ PSIZE_T TotalBytes 
    )
{
    PIMAGE_OPTIONAL_HEADER64 fileOptHeader;
    PIMAGE_OPTIONAL_HEADER64 procOptHeader;

    fileOptHeader = (PIMAGE_OPTIONAL_HEADER64)&Context->MappedImage.NtHeaders->OptionalHeader;
    procOptHeader = (PIMAGE_OPTIONAL_HEADER64)&Context->RemoteMappedImage.NtHeaders->OptionalHeader;

    //
    // Inspect the header
    //
    PhpAnalyzeImageCoherencyInsepct((PBYTE)&Context->MappedImage.NtHeaders->FileHeader,
                                    sizeof(IMAGE_NT_HEADERS64),
                                    (PBYTE)&Context->RemoteMappedImage.NtHeaders->FileHeader,
                                    sizeof(IMAGE_NT_HEADERS64),
                                    CoherentBytes,
                                    TotalBytes);
    //
    // And the optional header
    //
    PhpAnalyzeImageCoherencyInsepct((PBYTE)fileOptHeader,
                                    sizeof(IMAGE_OPTIONAL_HEADER64),
                                    (PBYTE)procOptHeader,
                                    sizeof(IMAGE_OPTIONAL_HEADER64),
                                    CoherentBytes,
                                    TotalBytes);

    //
    // Do the common inspection 
    //
    PhpAnalyzeImageCoherencyCommon(ProcessHandle,
                                   fileOptHeader->AddressOfEntryPoint,
                                   fileOptHeader->BaseOfCode,
                                   Context,
                                   CoherentBytes,
                                   TotalBytes);

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
* \param[in] FileName Win32 path to the image file on disk.
* \param[in] ProcessId Process ID of the active process to compare to.
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
NTSTATUS
PhGetProcessImageCoherency(
    _In_ PWSTR FileName,
    _In_ HANDLE ProcessId,
    _Out_ PFLOAT ImageCoherency
    )
{
    NTSTATUS status;
    HANDLE processHandle;
    PPH_PROCESS_IMAGE_COHERENCY_CONTEXT context;
    SIZE_T coherentBytes;
    SIZE_T totalBytes;

    context = NULL;
    coherentBytes = 0;
    totalBytes = 0;

    status = PhOpenProcess(&processHandle,
                           PROCESS_QUERY_LIMITED_INFORMATION | PROCESS_VM_READ,
                           ProcessId);
    if (!NT_SUCCESS(status))
    {
        processHandle = NULL;
        goto CleanupExit;
    }

    context = PhpCreateProcessImageCoherencyContext(FileName, processHandle);
    if (!context)
    {
        goto CleanupExit;
    }

    //
    // Creating the coherency context is best-effort and will store the status
    // in the context. We can infer some state if one fails, do so here.
    //
    if (!NT_SUCCESS(context->MappedImageStatus) ||
        !NT_SUCCESS(context->RemoteMappedImageStatus))
    {
        if (NT_SUCCESS(context->RemoteMappedImageStatus) &&
            ((context->MappedImageStatus == STATUS_INVALID_IMAGE_FORMAT) ||
             (context->MappedImageStatus == STATUS_INVALID_IMAGE_HASH) ||
             (context->MappedImageStatus == STATUS_IMAGE_SUBSYSTEM_NOT_PRESENT)))
        {
            //
            // If we succeeded to map the remote image but the on-disk image
            // is inherently incoherent, return the status from the on-disk
            // mapping.
            //
            status = context->MappedImageStatus;
        }
        else
        {
            //
            // If we failed for any other reason, return the error from the
            // remote mapping.
            //
            status = context->RemoteMappedImageStatus;
        }
        goto CleanupExit;
    }

    if (context->MappedImage.Magic != context->RemoteMappedImage.Magic)
    {
        //
        // The image types don't match. This is incoherent but we have one more
        // case to check. .NET can change this in the loaded PE to run "AnyCPU"
        // in some cases, if it's a .NET mapping do the managed calculation.
        //
        if (PhpAnalyzeImageCoherencyIsDotNet(context))
        {
            PhpAnalyzeImageCoherencyCommonAsManaged(processHandle,
                                                    context,
                                                    &coherentBytes,
                                                    &totalBytes);
        }
        status = STATUS_INVALID_IMAGE_FORMAT;
        goto CleanupExit;
    }

    switch (context->MappedImage.Magic)
    {
        case IMAGE_NT_OPTIONAL_HDR32_MAGIC:
        {
            status = PhpAnalyzeImageCoherencyNt32(processHandle,
                                                  context,
                                                  &coherentBytes,
                                                  &totalBytes);
            break;
        }
        case IMAGE_NT_OPTIONAL_HDR64_MAGIC:
        {
            status = PhpAnalyzeImageCoherencyNt64(processHandle,
                                                  context,
                                                  &coherentBytes,
                                                  &totalBytes);
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

    if (totalBytes)
    {
        *ImageCoherency = ((FLOAT)coherentBytes / totalBytes);
    }
    else
    {
        *ImageCoherency = 0.0f;
    }

    PhpFreeProcessImageCoherencyContext(context);

    if (processHandle)
    {
        NtClose(processHandle);
    }

    return status;
}
