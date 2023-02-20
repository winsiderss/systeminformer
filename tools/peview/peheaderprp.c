/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     dmex    2021
 *
 */

#include <peview.h>

typedef struct _PVP_PE_HEADER_CONTEXT
{
    HWND WindowHandle;
    HWND ListViewHandle;
    PH_LAYOUT_MANAGER LayoutManager;
    PPV_PROPPAGECONTEXT PropSheetContext;
} PVP_PE_HEADER_CONTEXT, *PPVP_PE_HEADER_CONTEXT;

typedef enum _PVP_IMAGE_HEADER_CATEGORY
{
    PVP_IMAGE_HEADER_CATEGORY_DOSHDR,
    PVP_IMAGE_HEADER_CATEGORY_DOSSTUB,
    PVP_IMAGE_HEADER_CATEGORY_FILEHDR,
    PVP_IMAGE_HEADER_CATEGORY_OPTHDR,
    PVP_IMAGE_HEADER_CATEGORY_OVERLAY,
    PVP_IMAGE_HEADER_CATEGORY_MAXIMUM
} PVP_IMAGE_HEADER_CATEGORY;

typedef enum _PVP_IMAGE_HEADER_INDEX
{
    PVP_IMAGE_HEADER_INDEX_DOS_MAGIC,
    PVP_IMAGE_HEADER_INDEX_DOS_BYTES,
    PVP_IMAGE_HEADER_INDEX_DOS_PAGES,
    PVP_IMAGE_HEADER_INDEX_DOS_RELOCATIONS,
    PVP_IMAGE_HEADER_INDEX_DOS_PARAGRAPH,
    PVP_IMAGE_HEADER_INDEX_DOS_MINPARA,
    PVP_IMAGE_HEADER_INDEX_DOS_MAXPARA,
    PVP_IMAGE_HEADER_INDEX_DOS_INITRELSS,
    PVP_IMAGE_HEADER_INDEX_DOS_INITRELSP,
    PVP_IMAGE_HEADER_INDEX_DOS_CHECKSUM,
    PVP_IMAGE_HEADER_INDEX_DOS_INITIP,
    PVP_IMAGE_HEADER_INDEX_DOS_INITCS,
    PVP_IMAGE_HEADER_INDEX_DOS_RELOCADDR,
    PVP_IMAGE_HEADER_INDEX_DOS_OVERLAY,
    //PVP_IMAGE_HEADER_INDEX_DOS_RESERVED1,
    PVP_IMAGE_HEADER_INDEX_DOS_OEMID,
    PVP_IMAGE_HEADER_INDEX_DOS_OEMINFO,
    //PVP_IMAGE_HEADER_INDEX_DOS_RESERVED2,
    PVP_IMAGE_HEADER_INDEX_DOS_EXEHDRADDR,
    PVP_IMAGE_HEADER_INDEX_DOS_STUBSIZE,
    PVP_IMAGE_HEADER_INDEX_DOS_STUBENTROPY,
    PVP_IMAGE_HEADER_INDEX_DOS_STUBHASH,
    PVP_IMAGE_HEADER_INDEX_DOS_RICHSIZE,
    PVP_IMAGE_HEADER_INDEX_DOS_RICHENTROPY,
    PVP_IMAGE_HEADER_INDEX_DOS_RICHHASH,
    PVP_IMAGE_HEADER_INDEX_DOS_SIZE,
    PVP_IMAGE_HEADER_INDEX_DOS_ENTROPY,
    PVP_IMAGE_HEADER_INDEX_DOS_HASH,
    PVP_IMAGE_HEADER_INDEX_FILE_NTSIG,
    PVP_IMAGE_HEADER_INDEX_FILE_MACHINE,
    PVP_IMAGE_HEADER_INDEX_FILE_SECTIONS,
    PVP_IMAGE_HEADER_INDEX_FILE_TIMESTAMP,
    PVP_IMAGE_HEADER_INDEX_FILE_SYMTABLEADDR,
    PVP_IMAGE_HEADER_INDEX_FILE_SYMTABLECOUNT,
    PVP_IMAGE_HEADER_INDEX_FILE_OPTHDRSIZE,
    PVP_IMAGE_HEADER_INDEX_FILE_CHARACTERISTICS,
    PVP_IMAGE_HEADER_INDEX_OPT_MAGIC,
    PVP_IMAGE_HEADER_INDEX_OPT_LINKERVERSION,
    PVP_IMAGE_HEADER_INDEX_OPT_SIZEOFCODE,
    PVP_IMAGE_HEADER_INDEX_OPT_INITSIZE,
    PVP_IMAGE_HEADER_INDEX_OPT_UNINITSIZE,
    PVP_IMAGE_HEADER_INDEX_OPT_ENTRYPOINT,
    PVP_IMAGE_HEADER_INDEX_OPT_BASEOFCODE,
    PVP_IMAGE_HEADER_INDEX_OPT_BASEOFDATA,
    PVP_IMAGE_HEADER_INDEX_OPT_IMAGEBASE,
    PVP_IMAGE_HEADER_INDEX_OPT_SECTIONALIGN,
    PVP_IMAGE_HEADER_INDEX_OPT_FILEALIGN,
    PVP_IMAGE_HEADER_INDEX_OPT_OSVERSION,
    PVP_IMAGE_HEADER_INDEX_OPT_IMGVERSION,
    PVP_IMAGE_HEADER_INDEX_OPT_SUBSYSTEMVERSION,
    PVP_IMAGE_HEADER_INDEX_OPT_WIN32VERSION,
    PVP_IMAGE_HEADER_INDEX_OPT_SIZEOFIMAGE,
    PVP_IMAGE_HEADER_INDEX_OPT_SIZEOFHEADERS,
    PVP_IMAGE_HEADER_INDEX_OPT_CHECKSUM,
    PVP_IMAGE_HEADER_INDEX_OPT_SUBSYSTEM,
    PVP_IMAGE_HEADER_INDEX_OPT_DLLCHARACTERISTICS,
    PVP_IMAGE_HEADER_INDEX_OPT_SIZEOFSTACKRESERVE,
    PVP_IMAGE_HEADER_INDEX_OPT_SIZEOFSTACKCOMMIT,
    PVP_IMAGE_HEADER_INDEX_OPT_SIZEOFHEAPRESERVE,
    PVP_IMAGE_HEADER_INDEX_OPT_SIZEOFHEAPCOMMIT,
    PVP_IMAGE_HEADER_INDEX_OPT_LOADERFLAGS,
    PVP_IMAGE_HEADER_INDEX_OPT_NUMBEROFRVA,
    PVP_IMAGE_HEADER_INDEX_PE_OVERLAY_SIZE,
    PVP_IMAGE_HEADER_INDEX_PE_OVERLAY_ENTROPY,
    PVP_IMAGE_HEADER_INDEX_PE_OVERLAY_HASH,
    PVP_IMAGE_HEADER_INDEX_MAXIMUM
} PVP_IMAGE_HEADER_INDEX;

VOID PvSetPeImageDosHeaderProperties(
    _In_ PPVP_PE_HEADER_CONTEXT Context
    )
{
    PIMAGE_DOS_HEADER imageDosHeader = PvMappedImage.ViewBase;
    WCHAR value[PH_PTR_STR_LEN_1];

    PhPrintPointer(value, UlongToPtr(imageDosHeader->e_magic));
    PhSetListViewSubItem(Context->ListViewHandle, PVP_IMAGE_HEADER_INDEX_DOS_MAGIC, 1, value);

    PhPrintPointer(value, UlongToPtr(imageDosHeader->e_cblp));
    PhSetListViewSubItem(Context->ListViewHandle, PVP_IMAGE_HEADER_INDEX_DOS_BYTES, 1, value);

    PhPrintPointer(value, UlongToPtr(imageDosHeader->e_cp));
    PhSetListViewSubItem(Context->ListViewHandle, PVP_IMAGE_HEADER_INDEX_DOS_PAGES, 1, value);

    PhPrintPointer(value, UlongToPtr(imageDosHeader->e_crlc));
    PhSetListViewSubItem(Context->ListViewHandle, PVP_IMAGE_HEADER_INDEX_DOS_RELOCATIONS, 1, value);

    PhPrintPointer(value, UlongToPtr(imageDosHeader->e_cparhdr));
    PhSetListViewSubItem(Context->ListViewHandle, PVP_IMAGE_HEADER_INDEX_DOS_PARAGRAPH, 1, value);

    PhPrintPointer(value, UlongToPtr(imageDosHeader->e_minalloc));
    PhSetListViewSubItem(Context->ListViewHandle, PVP_IMAGE_HEADER_INDEX_DOS_MINPARA, 1, value);

    PhPrintPointer(value, UlongToPtr(imageDosHeader->e_maxalloc));
    PhSetListViewSubItem(Context->ListViewHandle, PVP_IMAGE_HEADER_INDEX_DOS_MAXPARA, 1, value);

    PhPrintPointer(value, UlongToPtr(imageDosHeader->e_ss));
    PhSetListViewSubItem(Context->ListViewHandle, PVP_IMAGE_HEADER_INDEX_DOS_INITRELSS, 1, value);

    PhPrintPointer(value, UlongToPtr(imageDosHeader->e_sp));
    PhSetListViewSubItem(Context->ListViewHandle, PVP_IMAGE_HEADER_INDEX_DOS_INITRELSP, 1, value);

    PhPrintPointer(value, UlongToPtr(imageDosHeader->e_csum));
    PhSetListViewSubItem(Context->ListViewHandle, PVP_IMAGE_HEADER_INDEX_DOS_CHECKSUM, 1, value);

    PhPrintPointer(value, UlongToPtr(imageDosHeader->e_ip));
    PhSetListViewSubItem(Context->ListViewHandle, PVP_IMAGE_HEADER_INDEX_DOS_INITIP, 1, value);

    PhPrintPointer(value, UlongToPtr(imageDosHeader->e_cs));
    PhSetListViewSubItem(Context->ListViewHandle, PVP_IMAGE_HEADER_INDEX_DOS_INITCS, 1, value);

    PhPrintPointer(value, UlongToPtr(imageDosHeader->e_lfarlc));
    PhSetListViewSubItem(Context->ListViewHandle, PVP_IMAGE_HEADER_INDEX_DOS_RELOCADDR, 1, value);

    PhPrintPointer(value, UlongToPtr(imageDosHeader->e_ovno));
    PhSetListViewSubItem(Context->ListViewHandle, PVP_IMAGE_HEADER_INDEX_DOS_OVERLAY, 1, value);

    //PPH_STRING reserved1 = PhBufferToHexString((PUCHAR)imageDosHeader->e_res, sizeof(imageDosHeader->e_res));
    //PhSetListViewSubItem(Context->ListViewHandle, PVP_IMAGE_HEADER_INDEX_DOS_RESERVED1, 1, PhGetString(reserved1));
    //PhDereferenceObject(reserved1);

    PhPrintPointer(value, UlongToPtr(imageDosHeader->e_oemid));
    PhSetListViewSubItem(Context->ListViewHandle, PVP_IMAGE_HEADER_INDEX_DOS_OEMID, 1, value);

    PhPrintPointer(value, UlongToPtr(imageDosHeader->e_oeminfo));
    PhSetListViewSubItem(Context->ListViewHandle, PVP_IMAGE_HEADER_INDEX_DOS_OEMINFO, 1, value);

    //PPH_STRING reserved2 = PhBufferToHexString((PUCHAR)imageDosHeader->e_res2, sizeof(imageDosHeader->e_res2));
    //PhSetListViewSubItem(Context->ListViewHandle, PVP_IMAGE_HEADER_INDEX_DOS_RESERVED2, 1, PhGetString(reserved2));
    //PhDereferenceObject(reserved2);

    PhPrintPointer(value, UlongToPtr(imageDosHeader->e_lfanew));
    PhSetListViewSubItem(Context->ListViewHandle, PVP_IMAGE_HEADER_INDEX_DOS_EXEHDRADDR, 1, value);
}

VOID PvSetPeImageDosStubHeaderProperties(
    _In_ PPVP_PE_HEADER_CONTEXT Context
    )
{
    PIMAGE_DOS_HEADER imageDosHeader = PvMappedImage.ViewBase;
    ULONG imageDosStubDataLength = imageDosHeader->e_lfanew - RTL_SIZEOF_THROUGH_FIELD(IMAGE_DOS_HEADER, e_lfanew);
    PVOID imageDosStubData = PTR_ADD_OFFSET(PvMappedImage.ViewBase, RTL_SIZEOF_THROUGH_FIELD(IMAGE_DOS_HEADER, e_lfanew));
    ULONG imageDosStubRichStart;
    ULONG imageDosStubRichEnd;
    ULONG imageDosStubRichLength = 0;
    ULONG imageDosStubActualDataLength = 0;
    PVOID imageDosStubRichData = NULL;
    WCHAR value[PH_PTR_STR_LEN_1];
    WCHAR size[PH_PTR_STR_LEN_1];
    PPH_STRING string;
    PPH_STRING hashString;

    if (imageDosStubDataLength == 0)
        return;

    if (NT_SUCCESS(PhGetMappedImageProdIdExtents(&PvMappedImage, &imageDosStubRichStart, &imageDosStubRichEnd)))
    {
        imageDosStubRichData = PTR_ADD_OFFSET(PvMappedImage.ViewBase, imageDosStubRichStart);
        imageDosStubRichLength = imageDosStubRichEnd - imageDosStubRichStart;
        imageDosStubActualDataLength = imageDosStubDataLength - imageDosStubRichLength;
    }

    if (imageDosStubActualDataLength)
    {
        PhPrintPointer(value, UlongToPtr(RTL_SIZEOF_THROUGH_FIELD(IMAGE_DOS_HEADER, e_lfanew)));
        PhPrintPointer(size, PTR_ADD_OFFSET(RTL_SIZEOF_THROUGH_FIELD(IMAGE_DOS_HEADER, e_lfanew), imageDosStubActualDataLength));
        string = PhaFormatString(L"%s-%s", value, size);
        PhSetListViewSubItem(Context->ListViewHandle, PVP_IMAGE_HEADER_INDEX_DOS_STUBSIZE, 1, PhaFormatString(
            L"%s (%s)",
            PhGetString(string),
            PhaFormatSize(imageDosStubActualDataLength, ULONG_MAX)->Buffer
            )->Buffer);

        __try
        {
            DOUBLE imageDosStubEntropy;
            DOUBLE imageDosStubMean = 0;
            PPH_STRING entropyString;
            PPH_STRING stringEntropy;
            PPH_STRING stringMean;

            imageDosStubEntropy = PvCalculateEntropyBuffer(
                imageDosStubData,
                imageDosStubActualDataLength,
                &imageDosStubMean
                );

            stringEntropy = PvFormatDoubleCropZero(imageDosStubEntropy, 6);
            stringMean = PvFormatDoubleCropZero(imageDosStubMean, 4);
            entropyString = PhFormatString(
                L"%s S (%s X)",
                PhGetStringOrEmpty(stringEntropy),
                PhGetStringOrEmpty(stringMean)
                );

            PhSetListViewSubItem(Context->ListViewHandle, PVP_IMAGE_HEADER_INDEX_DOS_STUBENTROPY, 1, PhGetStringOrEmpty(entropyString));

            PhDereferenceObject(entropyString);
            PhDereferenceObject(stringMean);
            PhDereferenceObject(stringEntropy);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            PPH_STRING message;

            if (message = PhGetWin32Message(PhNtStatusToDosError(GetExceptionCode())))
            {
                PhSetListViewSubItem(Context->ListViewHandle, PVP_IMAGE_HEADER_INDEX_DOS_STUBENTROPY, 1, PhGetString(message));
                PhDereferenceObject(message);
            }
        }

        if (hashString = PvHashBuffer(imageDosStubData, imageDosStubActualDataLength))
        {
            PhSetListViewSubItem(Context->ListViewHandle, PVP_IMAGE_HEADER_INDEX_DOS_STUBHASH, 1, PhGetString(hashString));
            PhDereferenceObject(hashString);
        }
    }

    if (imageDosStubRichLength)
    {
        PhPrintPointer(value, UlongToPtr(imageDosStubRichStart));
        PhPrintPointer(size, PTR_ADD_OFFSET(imageDosStubRichStart, imageDosStubRichLength));
        string = PhaFormatString(L"%s-%s", value, size);
        PhSetListViewSubItem(Context->ListViewHandle, PVP_IMAGE_HEADER_INDEX_DOS_RICHSIZE, 1, PhaFormatString(
            L"%s (%s)",
            PhGetString(string),
            PhaFormatSize(imageDosStubRichLength, ULONG_MAX)->Buffer
            )->Buffer);

        __try
        {
            DOUBLE imageDosStubEntropy;
            DOUBLE imageDosStubMean = 0;
            PPH_STRING entropyString;
            PPH_STRING stringEntropy;
            PPH_STRING stringMean;

            imageDosStubEntropy = PvCalculateEntropyBuffer(
                imageDosStubRichData,
                imageDosStubRichLength,
                &imageDosStubMean
                );

            stringEntropy = PvFormatDoubleCropZero(imageDosStubEntropy, 6);
            stringMean = PvFormatDoubleCropZero(imageDosStubMean, 4);
            entropyString = PhFormatString(
                L"%s S (%s X)",
                PhGetStringOrEmpty(stringEntropy),
                PhGetStringOrEmpty(stringMean)
                );

            PhSetListViewSubItem(Context->ListViewHandle, PVP_IMAGE_HEADER_INDEX_DOS_RICHENTROPY, 1, PhGetStringOrEmpty(entropyString));

            PhDereferenceObject(entropyString);
            PhDereferenceObject(stringMean);
            PhDereferenceObject(stringEntropy);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            PPH_STRING message;

            if (message = PhGetWin32Message(PhNtStatusToDosError(GetExceptionCode())))
            {
                PhSetListViewSubItem(Context->ListViewHandle, PVP_IMAGE_HEADER_INDEX_DOS_RICHENTROPY, 1, PhGetString(message));
                PhDereferenceObject(message);
            }
        }

        if (hashString = PvHashBuffer(imageDosStubRichData, imageDosStubRichLength))
        {
            PhSetListViewSubItem(Context->ListViewHandle, PVP_IMAGE_HEADER_INDEX_DOS_RICHHASH, 1, PhGetString(hashString));
            PhDereferenceObject(hashString);
        }
    }

    if (imageDosStubDataLength)
    {
        PhPrintPointer(value, UlongToPtr(RTL_SIZEOF_THROUGH_FIELD(IMAGE_DOS_HEADER, e_lfanew)));
        PhPrintPointer(size, PTR_ADD_OFFSET(RTL_SIZEOF_THROUGH_FIELD(IMAGE_DOS_HEADER, e_lfanew), imageDosStubDataLength));
        string = PhaFormatString(L"%s-%s", value, size);
        PhSetListViewSubItem(Context->ListViewHandle, PVP_IMAGE_HEADER_INDEX_DOS_SIZE, 1, PhaFormatString(
            L"%s (%s)",
            PhGetString(string),
            PhaFormatSize(imageDosStubDataLength, ULONG_MAX)->Buffer
            )->Buffer);

        __try
        {
            DOUBLE imageDosStubEntropy;
            DOUBLE imageDosStubMean = 0;
            PPH_STRING entropyString;
            PPH_STRING stringEntropy;
            PPH_STRING stringMean;

            imageDosStubEntropy = PvCalculateEntropyBuffer(
                imageDosStubData,
                imageDosStubDataLength,
                &imageDosStubMean
                );

            stringEntropy = PvFormatDoubleCropZero(imageDosStubEntropy, 6);
            stringMean = PvFormatDoubleCropZero(imageDosStubMean, 4);
            entropyString = PhFormatString(
                L"%s S (%s X)",
                PhGetStringOrEmpty(stringEntropy),
                PhGetStringOrEmpty(stringMean)
                );

            PhSetListViewSubItem(Context->ListViewHandle, PVP_IMAGE_HEADER_INDEX_DOS_ENTROPY, 1, PhGetStringOrEmpty(entropyString));

            PhDereferenceObject(entropyString);
            PhDereferenceObject(stringMean);
            PhDereferenceObject(stringEntropy);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            PPH_STRING message;

            if (message = PhGetWin32Message(PhNtStatusToDosError(GetExceptionCode())))
            {
                PhSetListViewSubItem(Context->ListViewHandle, PVP_IMAGE_HEADER_INDEX_DOS_ENTROPY, 1, PhGetString(message));
                PhDereferenceObject(message);
            }
        }

        if (hashString = PvHashBuffer(imageDosStubData, imageDosStubDataLength))
        {
            PhSetListViewSubItem(Context->ListViewHandle, PVP_IMAGE_HEADER_INDEX_DOS_HASH, 1, PhGetString(hashString));
            PhDereferenceObject(hashString);
        }
    }
}

VOID PvSetPeImageFileHeaderProperties(
    _In_ PPVP_PE_HEADER_CONTEXT Context
    )
{
    PIMAGE_NT_HEADERS imageNtHeader = PvMappedImage.NtHeaders;
    WCHAR value[PH_PTR_STR_LEN_1];
    PPH_STRING string;

    PhPrintPointer(value, UlongToPtr(imageNtHeader->Signature));
    PhSetListViewSubItem(Context->ListViewHandle, PVP_IMAGE_HEADER_INDEX_FILE_NTSIG, 1, value);

    PhPrintPointer(value, UlongToPtr(imageNtHeader->FileHeader.Machine));
    PhSetListViewSubItem(Context->ListViewHandle, PVP_IMAGE_HEADER_INDEX_FILE_MACHINE, 1, value);

    PhPrintUInt32(value, imageNtHeader->FileHeader.NumberOfSections);
    PhSetListViewSubItem(Context->ListViewHandle, PVP_IMAGE_HEADER_INDEX_FILE_SECTIONS, 1, value);

    PhPrintPointer(value, UlongToPtr(imageNtHeader->FileHeader.TimeDateStamp));
    PhSetListViewSubItem(Context->ListViewHandle, PVP_IMAGE_HEADER_INDEX_FILE_TIMESTAMP, 1, value);

    PhPrintPointer(value, UlongToPtr(imageNtHeader->FileHeader.PointerToSymbolTable));
    PhSetListViewSubItem(Context->ListViewHandle, PVP_IMAGE_HEADER_INDEX_FILE_SYMTABLEADDR, 1, value);

    PhPrintUInt32(value, imageNtHeader->FileHeader.NumberOfSymbols);
    PhSetListViewSubItem(Context->ListViewHandle, PVP_IMAGE_HEADER_INDEX_FILE_SYMTABLECOUNT, 1, value);

    PhPrintPointer(value, UlongToPtr(imageNtHeader->FileHeader.SizeOfOptionalHeader));
    string = PhaFormatString(L"%s (%s)", value, PhaFormatSize(imageNtHeader->FileHeader.SizeOfOptionalHeader, ULONG_MAX)->Buffer);
    PhSetListViewSubItem(Context->ListViewHandle, PVP_IMAGE_HEADER_INDEX_FILE_OPTHDRSIZE, 1, PhGetString(string));

    PhPrintPointer(value, UlongToPtr(imageNtHeader->FileHeader.Characteristics));
    PhSetListViewSubItem(Context->ListViewHandle, PVP_IMAGE_HEADER_INDEX_FILE_CHARACTERISTICS, 1, value);
}

VOID PvSetPeImageOptionalHeaderProperties(
    _In_ PPVP_PE_HEADER_CONTEXT Context
    )
{
    if (PvMappedImage.Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC)
    {
        PIMAGE_NT_HEADERS32 imageNtHeader = PvMappedImage.NtHeaders32;
        WCHAR value[PH_PTR_STR_LEN_1];
        PPH_STRING string;

        PhPrintPointer(value, UlongToPtr(imageNtHeader->OptionalHeader.Magic));
        PhSetListViewSubItem(Context->ListViewHandle, PVP_IMAGE_HEADER_INDEX_OPT_MAGIC, 1, value);

        string = PhaFormatString(L"%lu.%lu", imageNtHeader->OptionalHeader.MajorLinkerVersion, imageNtHeader->OptionalHeader.MinorLinkerVersion);
        PhSetListViewSubItem(Context->ListViewHandle, PVP_IMAGE_HEADER_INDEX_OPT_LINKERVERSION, 1, PhGetString(string));

        PhPrintPointer(value, UlongToPtr(imageNtHeader->OptionalHeader.SizeOfCode));
        string = PhaFormatString(L"%s (%s)", value, PhaFormatSize(imageNtHeader->OptionalHeader.SizeOfCode, ULONG_MAX)->Buffer);
        PhSetListViewSubItem(Context->ListViewHandle, PVP_IMAGE_HEADER_INDEX_OPT_SIZEOFCODE, 1, PhGetString(string));

        PhPrintPointer(value, UlongToPtr(imageNtHeader->OptionalHeader.SizeOfInitializedData));
        string = PhaFormatString(L"%s (%s)", value, PhaFormatSize(imageNtHeader->OptionalHeader.SizeOfInitializedData, ULONG_MAX)->Buffer);
        PhSetListViewSubItem(Context->ListViewHandle, PVP_IMAGE_HEADER_INDEX_OPT_INITSIZE, 1, PhGetString(string));

        PhPrintPointer(value, UlongToPtr(imageNtHeader->OptionalHeader.SizeOfUninitializedData));
        string = PhaFormatString(L"%s (%s)", value, PhaFormatSize(imageNtHeader->OptionalHeader.SizeOfUninitializedData, ULONG_MAX)->Buffer);
        PhSetListViewSubItem(Context->ListViewHandle, PVP_IMAGE_HEADER_INDEX_OPT_UNINITSIZE, 1, PhGetString(string));

        PhPrintPointer(value, UlongToPtr(imageNtHeader->OptionalHeader.AddressOfEntryPoint));
        PhSetListViewSubItem(Context->ListViewHandle, PVP_IMAGE_HEADER_INDEX_OPT_ENTRYPOINT, 1, value);

        PhPrintPointer(value, UlongToPtr(imageNtHeader->OptionalHeader.BaseOfCode));
        PhSetListViewSubItem(Context->ListViewHandle, PVP_IMAGE_HEADER_INDEX_OPT_BASEOFCODE, 1, value);

        PhPrintPointer(value, UlongToPtr(imageNtHeader->OptionalHeader.BaseOfData));
        PhSetListViewSubItem(Context->ListViewHandle, PVP_IMAGE_HEADER_INDEX_OPT_BASEOFDATA, 1, value);

        PhPrintPointer(value, UlongToPtr(imageNtHeader->OptionalHeader.ImageBase));
        PhSetListViewSubItem(Context->ListViewHandle, PVP_IMAGE_HEADER_INDEX_OPT_IMAGEBASE, 1, value);

        PhPrintPointer(value, UlongToPtr(imageNtHeader->OptionalHeader.SectionAlignment));
        PhSetListViewSubItem(Context->ListViewHandle, PVP_IMAGE_HEADER_INDEX_OPT_SECTIONALIGN, 1, value);

        PhPrintPointer(value, UlongToPtr(imageNtHeader->OptionalHeader.FileAlignment));
        PhSetListViewSubItem(Context->ListViewHandle, PVP_IMAGE_HEADER_INDEX_OPT_FILEALIGN, 1, value);

        string = PhaFormatString(L"%lu.%lu", imageNtHeader->OptionalHeader.MajorOperatingSystemVersion, imageNtHeader->OptionalHeader.MinorOperatingSystemVersion);
        PhSetListViewSubItem(Context->ListViewHandle, PVP_IMAGE_HEADER_INDEX_OPT_OSVERSION, 1, PhGetString(string));

        string = PhaFormatString(L"%lu.%lu", imageNtHeader->OptionalHeader.MajorImageVersion, imageNtHeader->OptionalHeader.MinorImageVersion);
        PhSetListViewSubItem(Context->ListViewHandle, PVP_IMAGE_HEADER_INDEX_OPT_IMGVERSION, 1, PhGetString(string));

        string = PhaFormatString(L"%lu.%lu", imageNtHeader->OptionalHeader.MajorSubsystemVersion, imageNtHeader->OptionalHeader.MinorSubsystemVersion);
        PhSetListViewSubItem(Context->ListViewHandle, PVP_IMAGE_HEADER_INDEX_OPT_SUBSYSTEMVERSION, 1, PhGetString(string));

        PhPrintPointer(value, UlongToPtr(imageNtHeader->OptionalHeader.Win32VersionValue));
        PhSetListViewSubItem(Context->ListViewHandle, PVP_IMAGE_HEADER_INDEX_OPT_WIN32VERSION, 1, value);

        PhPrintPointer(value, UlongToPtr(imageNtHeader->OptionalHeader.SizeOfImage));
        string = PhaFormatString(L"%s (%s)", value, PhaFormatSize(imageNtHeader->OptionalHeader.SizeOfImage, ULONG_MAX)->Buffer);
        PhSetListViewSubItem(Context->ListViewHandle, PVP_IMAGE_HEADER_INDEX_OPT_SIZEOFIMAGE, 1, PhGetString(string));

        PhPrintPointer(value, UlongToPtr(imageNtHeader->OptionalHeader.SizeOfHeaders));
        string = PhaFormatString(L"%s (%s)", value, PhaFormatSize(imageNtHeader->OptionalHeader.SizeOfHeaders, ULONG_MAX)->Buffer);
        PhSetListViewSubItem(Context->ListViewHandle, PVP_IMAGE_HEADER_INDEX_OPT_SIZEOFHEADERS, 1, PhGetString(string));

        PhPrintPointer(value, UlongToPtr(imageNtHeader->OptionalHeader.CheckSum));
        PhSetListViewSubItem(Context->ListViewHandle, PVP_IMAGE_HEADER_INDEX_OPT_CHECKSUM, 1, value);

        PhPrintUInt32(value, imageNtHeader->OptionalHeader.Subsystem);
        PhSetListViewSubItem(Context->ListViewHandle, PVP_IMAGE_HEADER_INDEX_OPT_SUBSYSTEM, 1, value);

        PhPrintPointer(value, UlongToPtr(imageNtHeader->OptionalHeader.DllCharacteristics));
        PhSetListViewSubItem(Context->ListViewHandle, PVP_IMAGE_HEADER_INDEX_OPT_DLLCHARACTERISTICS, 1, value);

        PhPrintPointer(value, UlongToPtr(imageNtHeader->OptionalHeader.SizeOfStackReserve));
        PhSetListViewSubItem(Context->ListViewHandle, PVP_IMAGE_HEADER_INDEX_OPT_SIZEOFSTACKRESERVE, 1, value);

        PhPrintPointer(value, UlongToPtr(imageNtHeader->OptionalHeader.SizeOfStackCommit));
        PhSetListViewSubItem(Context->ListViewHandle, PVP_IMAGE_HEADER_INDEX_OPT_SIZEOFSTACKCOMMIT, 1, value);

        PhPrintPointer(value, UlongToPtr(imageNtHeader->OptionalHeader.SizeOfHeapReserve));
        string = PhaFormatString(L"%s (%s)", value, PhaFormatSize(imageNtHeader->OptionalHeader.SizeOfHeapReserve, ULONG_MAX)->Buffer);
        PhSetListViewSubItem(Context->ListViewHandle, PVP_IMAGE_HEADER_INDEX_OPT_SIZEOFHEAPRESERVE, 1, PhGetString(string));

        PhPrintPointer(value, UlongToPtr(imageNtHeader->OptionalHeader.SizeOfHeapCommit));
        string = PhaFormatString(L"%s (%s)", value, PhaFormatSize(imageNtHeader->OptionalHeader.SizeOfHeapCommit, ULONG_MAX)->Buffer);
        PhSetListViewSubItem(Context->ListViewHandle, PVP_IMAGE_HEADER_INDEX_OPT_SIZEOFHEAPCOMMIT, 1, PhGetString(string));

        PhPrintPointer(value, UlongToPtr(imageNtHeader->OptionalHeader.LoaderFlags));
        PhSetListViewSubItem(Context->ListViewHandle, PVP_IMAGE_HEADER_INDEX_OPT_LOADERFLAGS, 1, value);

        PhPrintUInt32(value, imageNtHeader->OptionalHeader.NumberOfRvaAndSizes);
        PhSetListViewSubItem(Context->ListViewHandle, PVP_IMAGE_HEADER_INDEX_OPT_NUMBEROFRVA, 1, value);
    }
    else
    {
        PIMAGE_NT_HEADERS imageNtHeader = PvMappedImage.NtHeaders;
        WCHAR value[PH_PTR_STR_LEN_1];
        PPH_STRING string;

        PhPrintPointer(value, UlongToPtr(imageNtHeader->OptionalHeader.Magic));
        PhSetListViewSubItem(Context->ListViewHandle, PVP_IMAGE_HEADER_INDEX_OPT_MAGIC, 1, value);

        string = PhaFormatString(L"%lu.%lu", imageNtHeader->OptionalHeader.MajorLinkerVersion, imageNtHeader->OptionalHeader.MinorLinkerVersion);
        PhSetListViewSubItem(Context->ListViewHandle, PVP_IMAGE_HEADER_INDEX_OPT_LINKERVERSION, 1, PhGetString(string));

        PhPrintPointer(value, UlongToPtr(imageNtHeader->OptionalHeader.SizeOfCode));
        string = PhaFormatString(L"%s (%s)", value, PhaFormatSize(imageNtHeader->OptionalHeader.SizeOfCode, ULONG_MAX)->Buffer);
        PhSetListViewSubItem(Context->ListViewHandle, PVP_IMAGE_HEADER_INDEX_OPT_SIZEOFCODE, 1, PhGetString(string));

        PhPrintPointer(value, UlongToPtr(imageNtHeader->OptionalHeader.SizeOfInitializedData));
        string = PhaFormatString(L"%s (%s)", value, PhaFormatSize(imageNtHeader->OptionalHeader.SizeOfInitializedData, ULONG_MAX)->Buffer);
        PhSetListViewSubItem(Context->ListViewHandle, PVP_IMAGE_HEADER_INDEX_OPT_INITSIZE, 1, PhGetString(string));

        PhPrintPointer(value, UlongToPtr(imageNtHeader->OptionalHeader.SizeOfUninitializedData));
        string = PhaFormatString(L"%s (%s)", value, PhaFormatSize(imageNtHeader->OptionalHeader.SizeOfUninitializedData, ULONG_MAX)->Buffer);
        PhSetListViewSubItem(Context->ListViewHandle, PVP_IMAGE_HEADER_INDEX_OPT_UNINITSIZE, 1, PhGetString(string));

        PhPrintPointer(value, UlongToPtr(imageNtHeader->OptionalHeader.AddressOfEntryPoint));
        PhSetListViewSubItem(Context->ListViewHandle, PVP_IMAGE_HEADER_INDEX_OPT_ENTRYPOINT, 1, value);

        PhPrintPointer(value, UlongToPtr(imageNtHeader->OptionalHeader.BaseOfCode));
        PhSetListViewSubItem(Context->ListViewHandle, PVP_IMAGE_HEADER_INDEX_OPT_BASEOFCODE, 1, value);

        //PhPrintPointer(value, UlongToPtr(imageNtHeader->OptionalHeader.BaseOfData));
        PhSetListViewSubItem(Context->ListViewHandle, PVP_IMAGE_HEADER_INDEX_OPT_BASEOFDATA, 1, L"N/A");

        PhPrintPointer(value, (PVOID)imageNtHeader->OptionalHeader.ImageBase);
        PhSetListViewSubItem(Context->ListViewHandle, PVP_IMAGE_HEADER_INDEX_OPT_IMAGEBASE, 1, value);

        PhPrintPointer(value, UlongToPtr(imageNtHeader->OptionalHeader.SectionAlignment));
        string = PhaFormatString(L"%s (%s)", value, PhaFormatSize(imageNtHeader->OptionalHeader.SectionAlignment, ULONG_MAX)->Buffer);
        PhSetListViewSubItem(Context->ListViewHandle, PVP_IMAGE_HEADER_INDEX_OPT_SECTIONALIGN, 1, PhGetString(string));

        PhPrintPointer(value, UlongToPtr(imageNtHeader->OptionalHeader.FileAlignment));
        string = PhaFormatString(L"%s (%s)", value, PhaFormatSize(imageNtHeader->OptionalHeader.FileAlignment, ULONG_MAX)->Buffer);
        PhSetListViewSubItem(Context->ListViewHandle, PVP_IMAGE_HEADER_INDEX_OPT_FILEALIGN, 1, PhGetString(string));

        string = PhaFormatString(L"%lu.%lu", imageNtHeader->OptionalHeader.MajorOperatingSystemVersion, imageNtHeader->OptionalHeader.MinorOperatingSystemVersion);
        PhSetListViewSubItem(Context->ListViewHandle, PVP_IMAGE_HEADER_INDEX_OPT_OSVERSION, 1, PhGetString(string));

        string = PhaFormatString(L"%lu.%lu", imageNtHeader->OptionalHeader.MajorImageVersion, imageNtHeader->OptionalHeader.MinorImageVersion);
        PhSetListViewSubItem(Context->ListViewHandle, PVP_IMAGE_HEADER_INDEX_OPT_IMGVERSION, 1, PhGetString(string));

        string = PhaFormatString(L"%lu.%lu", imageNtHeader->OptionalHeader.MajorSubsystemVersion, imageNtHeader->OptionalHeader.MinorSubsystemVersion);
        PhSetListViewSubItem(Context->ListViewHandle, PVP_IMAGE_HEADER_INDEX_OPT_SUBSYSTEMVERSION, 1, PhGetString(string));

        PhPrintPointer(value, UlongToPtr(imageNtHeader->OptionalHeader.Win32VersionValue));
        PhSetListViewSubItem(Context->ListViewHandle, PVP_IMAGE_HEADER_INDEX_OPT_WIN32VERSION, 1, value);

        PhPrintPointer(value, UlongToPtr(imageNtHeader->OptionalHeader.SizeOfImage));
        string = PhaFormatString(L"%s (%s)", value, PhaFormatSize(imageNtHeader->OptionalHeader.SizeOfImage, ULONG_MAX)->Buffer);
        PhSetListViewSubItem(Context->ListViewHandle, PVP_IMAGE_HEADER_INDEX_OPT_SIZEOFIMAGE, 1, PhGetString(string));

        PhPrintPointer(value, UlongToPtr(imageNtHeader->OptionalHeader.SizeOfHeaders));
        string = PhaFormatString(L"%s (%s)", value, PhaFormatSize(imageNtHeader->OptionalHeader.SizeOfHeaders, ULONG_MAX)->Buffer);
        PhSetListViewSubItem(Context->ListViewHandle, PVP_IMAGE_HEADER_INDEX_OPT_SIZEOFHEADERS, 1, PhGetString(string));

        PhPrintPointer(value, UlongToPtr(imageNtHeader->OptionalHeader.CheckSum));
        PhSetListViewSubItem(Context->ListViewHandle, PVP_IMAGE_HEADER_INDEX_OPT_CHECKSUM, 1, value);

        PhPrintUInt32(value, imageNtHeader->OptionalHeader.Subsystem);
        PhSetListViewSubItem(Context->ListViewHandle, PVP_IMAGE_HEADER_INDEX_OPT_SUBSYSTEM, 1, value);

        PhPrintPointer(value, UlongToPtr(imageNtHeader->OptionalHeader.DllCharacteristics));
        PhSetListViewSubItem(Context->ListViewHandle, PVP_IMAGE_HEADER_INDEX_OPT_DLLCHARACTERISTICS, 1, value);

        PhPrintPointer(value, (PVOID)imageNtHeader->OptionalHeader.SizeOfStackReserve);
        string = PhaFormatString(L"%s (%s)", value, PhaFormatSize(imageNtHeader->OptionalHeader.SizeOfStackReserve, ULONG_MAX)->Buffer);
        PhSetListViewSubItem(Context->ListViewHandle, PVP_IMAGE_HEADER_INDEX_OPT_SIZEOFSTACKRESERVE, 1, PhGetString(string));

        PhPrintPointer(value, (PVOID)imageNtHeader->OptionalHeader.SizeOfStackCommit);
        string = PhaFormatString(L"%s (%s)", value, PhaFormatSize(imageNtHeader->OptionalHeader.SizeOfStackCommit, ULONG_MAX)->Buffer);
        PhSetListViewSubItem(Context->ListViewHandle, PVP_IMAGE_HEADER_INDEX_OPT_SIZEOFSTACKCOMMIT, 1, PhGetString(string));

        PhPrintPointer(value, (PVOID)imageNtHeader->OptionalHeader.SizeOfHeapReserve);
        string = PhaFormatString(L"%s (%s)", value, PhaFormatSize(imageNtHeader->OptionalHeader.SizeOfHeapReserve, ULONG_MAX)->Buffer);
        PhSetListViewSubItem(Context->ListViewHandle, PVP_IMAGE_HEADER_INDEX_OPT_SIZEOFHEAPRESERVE, 1, PhGetString(string));

        PhPrintPointer(value, (PVOID)imageNtHeader->OptionalHeader.SizeOfHeapCommit);
        string = PhaFormatString(L"%s (%s)", value, PhaFormatSize(imageNtHeader->OptionalHeader.SizeOfHeapCommit, ULONG_MAX)->Buffer);
        PhSetListViewSubItem(Context->ListViewHandle, PVP_IMAGE_HEADER_INDEX_OPT_SIZEOFHEAPCOMMIT, 1, PhGetString(string));

        PhPrintPointer(value, UlongToPtr(imageNtHeader->OptionalHeader.LoaderFlags));
        PhSetListViewSubItem(Context->ListViewHandle, PVP_IMAGE_HEADER_INDEX_OPT_LOADERFLAGS, 1, value);

        PhPrintUInt32(value, imageNtHeader->OptionalHeader.NumberOfRvaAndSizes);
        PhSetListViewSubItem(Context->ListViewHandle, PVP_IMAGE_HEADER_INDEX_OPT_NUMBEROFRVA, 1, value);
    }
}

VOID PvSetPeImageOverlayHeaderProperties(
    _In_ PPVP_PE_HEADER_CONTEXT Context
    )
{
    ULONG lastRawDataAddress = 0;
    ULONG64 lastRawDataOffset = 0;

    for (ULONG i = 0; i < PvMappedImage.NumberOfSections; i++)
    {
        if (PvMappedImage.Sections[i].PointerToRawData > lastRawDataAddress)
        {
            lastRawDataAddress = PvMappedImage.Sections[i].PointerToRawData;
            lastRawDataOffset = (ULONG64)PTR_ADD_OFFSET(lastRawDataAddress, PvMappedImage.Sections[i].SizeOfRawData);
        }
    }

    if (PvMappedImage.Size != lastRawDataOffset)
    {
        PIMAGE_DATA_DIRECTORY dataDirectory;
        ULONG64 imageOverlayDataLength;
        PVOID imageOverlayData;
        WCHAR value[PH_PTR_STR_LEN_1];
        WCHAR size[PH_PTR_STR_LEN_1];
        PPH_STRING string;
        PPH_STRING hashString;

        if (NT_SUCCESS(PhGetMappedImageDataEntry(
            &PvMappedImage,
            IMAGE_DIRECTORY_ENTRY_SECURITY,
            &dataDirectory
            )))
        {
            if (
                (lastRawDataOffset + dataDirectory->Size == PvMappedImage.Size) &&
                (lastRawDataOffset == dataDirectory->VirtualAddress)
                )
            {
                return;
            }
        }

        imageOverlayDataLength = PvMappedImage.Size - lastRawDataOffset;
        imageOverlayData = PTR_ADD_OFFSET(PvMappedImage.ViewBase, lastRawDataOffset);

        PhPrintPointer(value, (PVOID)lastRawDataOffset);
        PhPrintPointer(size, PTR_ADD_OFFSET(lastRawDataOffset, imageOverlayDataLength));
        string = PhaFormatString(L"%s-%s", value, size);
        PhSetListViewSubItem(Context->ListViewHandle, PVP_IMAGE_HEADER_INDEX_PE_OVERLAY_SIZE, 1, PhaFormatString(
            L"%s (%s)",
            PhGetString(string),
            PhaFormatSize(imageOverlayDataLength, ULONG_MAX)->Buffer
            )->Buffer);

        __try
        {
            DOUBLE imageDosStubEntropy;
            DOUBLE imageDosStubMean = 0;
            PPH_STRING entropyString;
            PPH_STRING stringEntropy;
            PPH_STRING stringMean;

            imageDosStubEntropy = PvCalculateEntropyBuffer(
                imageOverlayData,
                (ULONG)imageOverlayDataLength,
                &imageDosStubMean
                );

            stringEntropy = PvFormatDoubleCropZero(imageDosStubEntropy, 6);
            stringMean = PvFormatDoubleCropZero(imageDosStubMean, 4);
            entropyString = PhFormatString(
                L"%s S (%s X)",
                PhGetStringOrEmpty(stringEntropy),
                PhGetStringOrEmpty(stringMean)
                );

            PhSetListViewSubItem(Context->ListViewHandle, PVP_IMAGE_HEADER_INDEX_PE_OVERLAY_ENTROPY, 1, PhGetStringOrEmpty(entropyString));

            PhDereferenceObject(entropyString);
            PhDereferenceObject(stringMean);
            PhDereferenceObject(stringEntropy);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            PPH_STRING message;

            if (message = PhGetWin32Message(PhNtStatusToDosError(GetExceptionCode())))
            {
                PhSetListViewSubItem(Context->ListViewHandle, PVP_IMAGE_HEADER_INDEX_PE_OVERLAY_ENTROPY, 1, PhGetString(message));
                PhDereferenceObject(message);
            }
        }

        if (hashString = PvHashBuffer(imageOverlayData, (ULONG)imageOverlayDataLength))
        {
            PhSetListViewSubItem(Context->ListViewHandle, PVP_IMAGE_HEADER_INDEX_PE_OVERLAY_HASH, 1, PhGetString(hashString));
            PhDereferenceObject(hashString);
        }
    }
}

VOID PvPeUpdateImageHeaderProperties(
    _In_ PPVP_PE_HEADER_CONTEXT Context
    )
{
    ExtendedListView_SetRedraw(Context->ListViewHandle, FALSE);
    ListView_DeleteAllItems(Context->ListViewHandle);

    // DOS Headers
    PhAddListViewGroupItem(Context->ListViewHandle, PVP_IMAGE_HEADER_CATEGORY_DOSHDR, PVP_IMAGE_HEADER_INDEX_DOS_MAGIC, L"Magic number", NULL);
    PhAddListViewGroupItem(Context->ListViewHandle, PVP_IMAGE_HEADER_CATEGORY_DOSHDR, PVP_IMAGE_HEADER_INDEX_DOS_BYTES, L"Bytes on last page of file", NULL);
    PhAddListViewGroupItem(Context->ListViewHandle, PVP_IMAGE_HEADER_CATEGORY_DOSHDR, PVP_IMAGE_HEADER_INDEX_DOS_PAGES, L"Pages in file", NULL);
    PhAddListViewGroupItem(Context->ListViewHandle, PVP_IMAGE_HEADER_CATEGORY_DOSHDR, PVP_IMAGE_HEADER_INDEX_DOS_RELOCATIONS, L"Relocations", NULL);
    PhAddListViewGroupItem(Context->ListViewHandle, PVP_IMAGE_HEADER_CATEGORY_DOSHDR, PVP_IMAGE_HEADER_INDEX_DOS_PARAGRAPH, L"Size of header in paragraphs", NULL);
    PhAddListViewGroupItem(Context->ListViewHandle, PVP_IMAGE_HEADER_CATEGORY_DOSHDR, PVP_IMAGE_HEADER_INDEX_DOS_MINPARA, L"Minimum extra paragraphs needed", NULL);
    PhAddListViewGroupItem(Context->ListViewHandle, PVP_IMAGE_HEADER_CATEGORY_DOSHDR, PVP_IMAGE_HEADER_INDEX_DOS_MAXPARA, L"Maximum extra paragraphs needed", NULL);
    PhAddListViewGroupItem(Context->ListViewHandle, PVP_IMAGE_HEADER_CATEGORY_DOSHDR, PVP_IMAGE_HEADER_INDEX_DOS_INITRELSS, L"Initial (relative) SS value", NULL);
    PhAddListViewGroupItem(Context->ListViewHandle, PVP_IMAGE_HEADER_CATEGORY_DOSHDR, PVP_IMAGE_HEADER_INDEX_DOS_INITRELSP, L"Initial SP value", NULL);
    PhAddListViewGroupItem(Context->ListViewHandle, PVP_IMAGE_HEADER_CATEGORY_DOSHDR, PVP_IMAGE_HEADER_INDEX_DOS_CHECKSUM, L"Checksum", NULL);
    PhAddListViewGroupItem(Context->ListViewHandle, PVP_IMAGE_HEADER_CATEGORY_DOSHDR, PVP_IMAGE_HEADER_INDEX_DOS_INITIP, L"Initial IP value", NULL);
    PhAddListViewGroupItem(Context->ListViewHandle, PVP_IMAGE_HEADER_CATEGORY_DOSHDR, PVP_IMAGE_HEADER_INDEX_DOS_INITCS, L"Initial (relative) CS value", NULL);
    PhAddListViewGroupItem(Context->ListViewHandle, PVP_IMAGE_HEADER_CATEGORY_DOSHDR, PVP_IMAGE_HEADER_INDEX_DOS_RELOCADDR, L"File address of relocation table", NULL);
    PhAddListViewGroupItem(Context->ListViewHandle, PVP_IMAGE_HEADER_CATEGORY_DOSHDR, PVP_IMAGE_HEADER_INDEX_DOS_OVERLAY, L"Overlay number", NULL);
    //PhAddListViewGroupItem(Context->ListViewHandle, PVP_IMAGE_HEADER_CATEGORY_DOSHDR, PVP_IMAGE_HEADER_INDEX_DOS_RESERVED1, L"Reserved words", NULL);
    PhAddListViewGroupItem(Context->ListViewHandle, PVP_IMAGE_HEADER_CATEGORY_DOSHDR, PVP_IMAGE_HEADER_INDEX_DOS_OEMID, L"OEM identifier (for e_oeminfo)", NULL);
    PhAddListViewGroupItem(Context->ListViewHandle, PVP_IMAGE_HEADER_CATEGORY_DOSHDR, PVP_IMAGE_HEADER_INDEX_DOS_OEMINFO, L"OEM information (e_oemid specific)", NULL);
    //PhAddListViewGroupItem(Context->ListViewHandle, PVP_IMAGE_HEADER_CATEGORY_DOSHDR, PVP_IMAGE_HEADER_INDEX_DOS_RESERVED2, L"Reserved words", NULL);
    PhAddListViewGroupItem(Context->ListViewHandle, PVP_IMAGE_HEADER_CATEGORY_DOSHDR, PVP_IMAGE_HEADER_INDEX_DOS_EXEHDRADDR, L"File address of new exe header", NULL);
    // DOS Stub
    PhAddListViewGroupItem(Context->ListViewHandle, PVP_IMAGE_HEADER_CATEGORY_DOSSTUB, PVP_IMAGE_HEADER_INDEX_DOS_STUBSIZE, L"Stub size", NULL);
    PhAddListViewGroupItem(Context->ListViewHandle, PVP_IMAGE_HEADER_CATEGORY_DOSSTUB, PVP_IMAGE_HEADER_INDEX_DOS_STUBENTROPY, L"Stub entropy", NULL);
    PhAddListViewGroupItem(Context->ListViewHandle, PVP_IMAGE_HEADER_CATEGORY_DOSSTUB, PVP_IMAGE_HEADER_INDEX_DOS_STUBHASH, L"Stub hash", NULL);
    PhAddListViewGroupItem(Context->ListViewHandle, PVP_IMAGE_HEADER_CATEGORY_DOSSTUB, PVP_IMAGE_HEADER_INDEX_DOS_RICHSIZE, L"Rich size", NULL);
    PhAddListViewGroupItem(Context->ListViewHandle, PVP_IMAGE_HEADER_CATEGORY_DOSSTUB, PVP_IMAGE_HEADER_INDEX_DOS_RICHENTROPY, L"Rich entropy", NULL);
    PhAddListViewGroupItem(Context->ListViewHandle, PVP_IMAGE_HEADER_CATEGORY_DOSSTUB, PVP_IMAGE_HEADER_INDEX_DOS_RICHHASH, L"Rich hash", NULL);
    PhAddListViewGroupItem(Context->ListViewHandle, PVP_IMAGE_HEADER_CATEGORY_DOSSTUB, PVP_IMAGE_HEADER_INDEX_DOS_SIZE, L"Total size", NULL);
    PhAddListViewGroupItem(Context->ListViewHandle, PVP_IMAGE_HEADER_CATEGORY_DOSSTUB, PVP_IMAGE_HEADER_INDEX_DOS_ENTROPY, L"Total entropy", NULL);
    PhAddListViewGroupItem(Context->ListViewHandle, PVP_IMAGE_HEADER_CATEGORY_DOSSTUB, PVP_IMAGE_HEADER_INDEX_DOS_HASH, L"Total hash", NULL);
    // File Headers
    PhAddListViewGroupItem(Context->ListViewHandle, PVP_IMAGE_HEADER_CATEGORY_FILEHDR, PVP_IMAGE_HEADER_INDEX_FILE_NTSIG, L"Signature", NULL);
    PhAddListViewGroupItem(Context->ListViewHandle, PVP_IMAGE_HEADER_CATEGORY_FILEHDR, PVP_IMAGE_HEADER_INDEX_FILE_MACHINE, L"Machine", NULL);
    PhAddListViewGroupItem(Context->ListViewHandle, PVP_IMAGE_HEADER_CATEGORY_FILEHDR, PVP_IMAGE_HEADER_INDEX_FILE_SECTIONS, L"NumberOfSections", NULL);
    PhAddListViewGroupItem(Context->ListViewHandle, PVP_IMAGE_HEADER_CATEGORY_FILEHDR, PVP_IMAGE_HEADER_INDEX_FILE_TIMESTAMP, L"Timestamp", NULL);
    PhAddListViewGroupItem(Context->ListViewHandle, PVP_IMAGE_HEADER_CATEGORY_FILEHDR, PVP_IMAGE_HEADER_INDEX_FILE_SYMTABLEADDR, L"PointerToSymbolTable", NULL);
    PhAddListViewGroupItem(Context->ListViewHandle, PVP_IMAGE_HEADER_CATEGORY_FILEHDR, PVP_IMAGE_HEADER_INDEX_FILE_SYMTABLECOUNT, L"NumberOfSymbols", NULL);
    PhAddListViewGroupItem(Context->ListViewHandle, PVP_IMAGE_HEADER_CATEGORY_FILEHDR, PVP_IMAGE_HEADER_INDEX_FILE_OPTHDRSIZE, L"SizeOfOptionalHeader", NULL);
    PhAddListViewGroupItem(Context->ListViewHandle, PVP_IMAGE_HEADER_CATEGORY_FILEHDR, PVP_IMAGE_HEADER_INDEX_FILE_CHARACTERISTICS, L"Characteristics", NULL);
    // Optional Headers
    PhAddListViewGroupItem(Context->ListViewHandle, PVP_IMAGE_HEADER_CATEGORY_OPTHDR, PVP_IMAGE_HEADER_INDEX_OPT_MAGIC, L"Magic", NULL);
    PhAddListViewGroupItem(Context->ListViewHandle, PVP_IMAGE_HEADER_CATEGORY_OPTHDR, PVP_IMAGE_HEADER_INDEX_OPT_LINKERVERSION, L"LinkerVersion", NULL);
    PhAddListViewGroupItem(Context->ListViewHandle, PVP_IMAGE_HEADER_CATEGORY_OPTHDR, PVP_IMAGE_HEADER_INDEX_OPT_SIZEOFCODE, L"SizeOfCode", NULL);
    PhAddListViewGroupItem(Context->ListViewHandle, PVP_IMAGE_HEADER_CATEGORY_OPTHDR, PVP_IMAGE_HEADER_INDEX_OPT_INITSIZE, L"SizeOfInitializedData", NULL);
    PhAddListViewGroupItem(Context->ListViewHandle, PVP_IMAGE_HEADER_CATEGORY_OPTHDR, PVP_IMAGE_HEADER_INDEX_OPT_UNINITSIZE, L"SizeOfUninitializedData", NULL);
    PhAddListViewGroupItem(Context->ListViewHandle, PVP_IMAGE_HEADER_CATEGORY_OPTHDR, PVP_IMAGE_HEADER_INDEX_OPT_ENTRYPOINT, L"AddressOfEntryPoint", NULL);
    PhAddListViewGroupItem(Context->ListViewHandle, PVP_IMAGE_HEADER_CATEGORY_OPTHDR, PVP_IMAGE_HEADER_INDEX_OPT_BASEOFCODE, L"BaseOfCode", NULL);
    PhAddListViewGroupItem(Context->ListViewHandle, PVP_IMAGE_HEADER_CATEGORY_OPTHDR, PVP_IMAGE_HEADER_INDEX_OPT_BASEOFDATA, L"BaseOfData", NULL);
    PhAddListViewGroupItem(Context->ListViewHandle, PVP_IMAGE_HEADER_CATEGORY_OPTHDR, PVP_IMAGE_HEADER_INDEX_OPT_IMAGEBASE, L"ImageBase", NULL);
    PhAddListViewGroupItem(Context->ListViewHandle, PVP_IMAGE_HEADER_CATEGORY_OPTHDR, PVP_IMAGE_HEADER_INDEX_OPT_SECTIONALIGN, L"SectionAlignment", NULL);
    PhAddListViewGroupItem(Context->ListViewHandle, PVP_IMAGE_HEADER_CATEGORY_OPTHDR, PVP_IMAGE_HEADER_INDEX_OPT_FILEALIGN, L"FileAlignment", NULL);
    PhAddListViewGroupItem(Context->ListViewHandle, PVP_IMAGE_HEADER_CATEGORY_OPTHDR, PVP_IMAGE_HEADER_INDEX_OPT_OSVERSION, L"OperatingSystemVersion", NULL);
    PhAddListViewGroupItem(Context->ListViewHandle, PVP_IMAGE_HEADER_CATEGORY_OPTHDR, PVP_IMAGE_HEADER_INDEX_OPT_IMGVERSION, L"ImageVersion", NULL);
    PhAddListViewGroupItem(Context->ListViewHandle, PVP_IMAGE_HEADER_CATEGORY_OPTHDR, PVP_IMAGE_HEADER_INDEX_OPT_SUBSYSTEMVERSION, L"SubsystemVersion", NULL);
    PhAddListViewGroupItem(Context->ListViewHandle, PVP_IMAGE_HEADER_CATEGORY_OPTHDR, PVP_IMAGE_HEADER_INDEX_OPT_WIN32VERSION, L"Win32VersionValue", NULL);
    PhAddListViewGroupItem(Context->ListViewHandle, PVP_IMAGE_HEADER_CATEGORY_OPTHDR, PVP_IMAGE_HEADER_INDEX_OPT_SIZEOFIMAGE, L"SizeOfImage", NULL);
    PhAddListViewGroupItem(Context->ListViewHandle, PVP_IMAGE_HEADER_CATEGORY_OPTHDR, PVP_IMAGE_HEADER_INDEX_OPT_SIZEOFHEADERS, L"SizeOfHeaders", NULL);
    PhAddListViewGroupItem(Context->ListViewHandle, PVP_IMAGE_HEADER_CATEGORY_OPTHDR, PVP_IMAGE_HEADER_INDEX_OPT_CHECKSUM, L"CheckSum", NULL);
    PhAddListViewGroupItem(Context->ListViewHandle, PVP_IMAGE_HEADER_CATEGORY_OPTHDR, PVP_IMAGE_HEADER_INDEX_OPT_SUBSYSTEM, L"Subsystem", NULL);
    PhAddListViewGroupItem(Context->ListViewHandle, PVP_IMAGE_HEADER_CATEGORY_OPTHDR, PVP_IMAGE_HEADER_INDEX_OPT_DLLCHARACTERISTICS, L"DllCharacteristics", NULL);
    PhAddListViewGroupItem(Context->ListViewHandle, PVP_IMAGE_HEADER_CATEGORY_OPTHDR, PVP_IMAGE_HEADER_INDEX_OPT_SIZEOFSTACKRESERVE, L"SizeOfStackReserve", NULL);
    PhAddListViewGroupItem(Context->ListViewHandle, PVP_IMAGE_HEADER_CATEGORY_OPTHDR, PVP_IMAGE_HEADER_INDEX_OPT_SIZEOFSTACKCOMMIT, L"SizeOfStackCommit", NULL);
    PhAddListViewGroupItem(Context->ListViewHandle, PVP_IMAGE_HEADER_CATEGORY_OPTHDR, PVP_IMAGE_HEADER_INDEX_OPT_SIZEOFHEAPRESERVE, L"SizeOfHeapReserve", NULL);
    PhAddListViewGroupItem(Context->ListViewHandle, PVP_IMAGE_HEADER_CATEGORY_OPTHDR, PVP_IMAGE_HEADER_INDEX_OPT_SIZEOFHEAPCOMMIT, L"SizeOfHeapCommit", NULL);
    PhAddListViewGroupItem(Context->ListViewHandle, PVP_IMAGE_HEADER_CATEGORY_OPTHDR, PVP_IMAGE_HEADER_INDEX_OPT_LOADERFLAGS, L"LoaderFlags", NULL);
    PhAddListViewGroupItem(Context->ListViewHandle, PVP_IMAGE_HEADER_CATEGORY_OPTHDR, PVP_IMAGE_HEADER_INDEX_OPT_NUMBEROFRVA, L"NumberOfRvaAndSizes", NULL);
    // Overlay Data
    PhAddListViewGroupItem(Context->ListViewHandle, PVP_IMAGE_HEADER_CATEGORY_OVERLAY, PVP_IMAGE_HEADER_INDEX_PE_OVERLAY_SIZE, L"Data size", NULL);
    PhAddListViewGroupItem(Context->ListViewHandle, PVP_IMAGE_HEADER_CATEGORY_OVERLAY, PVP_IMAGE_HEADER_INDEX_PE_OVERLAY_ENTROPY, L"Data entropy", NULL);
    PhAddListViewGroupItem(Context->ListViewHandle, PVP_IMAGE_HEADER_CATEGORY_OVERLAY, PVP_IMAGE_HEADER_INDEX_PE_OVERLAY_HASH, L"Data hash", NULL);

    // DOS Headers
    PvSetPeImageDosHeaderProperties(Context);
    // DOS Stub
    PvSetPeImageDosStubHeaderProperties(Context);
    // File Headers
    PvSetPeImageFileHeaderProperties(Context);
    // Optional Headers
    PvSetPeImageOptionalHeaderProperties(Context);
    // Overlay Data
    PvSetPeImageOverlayHeaderProperties(Context);

    ExtendedListView_SetRedraw(Context->ListViewHandle, TRUE);
}

VOID PvPeAddImageHeaderGroups(
    _In_ PPVP_PE_HEADER_CONTEXT Context
    )
{
    ExtendedListView_SetRedraw(Context->ListViewHandle, FALSE);

    ListView_EnableGroupView(Context->ListViewHandle, TRUE);
    PhAddListViewGroup(Context->ListViewHandle, PVP_IMAGE_HEADER_CATEGORY_DOSHDR, L"DOS Header");
    PhAddListViewGroup(Context->ListViewHandle, PVP_IMAGE_HEADER_CATEGORY_DOSSTUB, L"DOS Stub");
    PhAddListViewGroup(Context->ListViewHandle, PVP_IMAGE_HEADER_CATEGORY_FILEHDR, L"File Header");
    PhAddListViewGroup(Context->ListViewHandle, PVP_IMAGE_HEADER_CATEGORY_OPTHDR, L"Optional Header");
    PhAddListViewGroup(Context->ListViewHandle, PVP_IMAGE_HEADER_CATEGORY_OVERLAY, L"Overlay Stub");

    ExtendedListView_SetRedraw(Context->ListViewHandle, TRUE);
}

INT_PTR CALLBACK PvPeHeadersDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PPVP_PE_HEADER_CONTEXT context;

    if (uMsg == WM_INITDIALOG)
    {
        context = PhAllocateZero(sizeof(PVP_PE_HEADER_CONTEXT));
        PhSetWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT, context);

        if (lParam)
        {
            LPPROPSHEETPAGE propSheetPage = (LPPROPSHEETPAGE)lParam;
            context->PropSheetContext = (PPV_PROPPAGECONTEXT)propSheetPage->lParam;
        }
    }
    else
    {
        context = PhGetWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT);
    }

    if (!context)
        return FALSE;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            context->WindowHandle = hwndDlg;
            context->ListViewHandle = GetDlgItem(hwndDlg, IDC_LIST);

            PhSetListViewStyle(context->ListViewHandle, FALSE, TRUE);
            PhSetControlTheme(context->ListViewHandle, L"explorer");
            PhAddListViewColumn(context->ListViewHandle, 0, 0, 0, LVCFMT_LEFT, 220, L"Name");
            PhAddListViewColumn(context->ListViewHandle, 1, 1, 1, LVCFMT_LEFT, 170, L"Value");
            PhSetExtendedListView(context->ListViewHandle);
            PvPeAddImageHeaderGroups(context);
            PhLoadListViewColumnsFromSetting(L"ImageHeadersListViewColumns", context->ListViewHandle);
            PhLoadListViewGroupStatesFromSetting(L"ImageHeadersListViewGroupStates", context->ListViewHandle);
            PvConfigTreeBorders(context->ListViewHandle);

            PhInitializeLayoutManager(&context->LayoutManager, hwndDlg);
            PhAddLayoutItem(&context->LayoutManager, context->ListViewHandle, NULL, PH_ANCHOR_ALL);

            PvPeUpdateImageHeaderProperties(context);

            PhInitializeWindowTheme(hwndDlg, PeEnableThemeSupport);
        }
        break;
    case WM_DESTROY:
        {
            PhSaveListViewGroupStatesToSetting(L"ImageHeadersListViewGroupStates", context->ListViewHandle);
            PhSaveListViewColumnsToSetting(L"ImageHeadersListViewColumns", context->ListViewHandle);
            PhRemoveWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT);
            PhFree(context);
        }
        break;
    case WM_SHOWWINDOW:
        {
            if (context->PropSheetContext && !context->PropSheetContext->LayoutInitialized)
            {
                PvAddPropPageLayoutItem(hwndDlg, hwndDlg, PH_PROP_PAGE_TAB_CONTROL_PARENT, PH_ANCHOR_ALL);
                PvDoPropPageLayout(hwndDlg);

                context->PropSheetContext->LayoutInitialized = TRUE;
            }
        }
        break;
    case WM_SIZE:
        {
            PhLayoutManagerLayout(&context->LayoutManager);
        }
        break;
    case WM_NOTIFY:
        {
            PvHandleListViewNotifyForCopy(lParam, context->ListViewHandle);
        }
        break;
    case WM_CONTEXTMENU:
        {
            PvHandleListViewCommandCopy(hwndDlg, lParam, wParam, context->ListViewHandle);
        }
        break;
    case WM_CTLCOLORBTN:
    case WM_CTLCOLORDLG:
    case WM_CTLCOLORSTATIC:
    case WM_CTLCOLORLISTBOX:
        {
            SetBkMode((HDC)wParam, TRANSPARENT);
            SetTextColor((HDC)wParam, RGB(0, 0, 0));
            SetDCBrushColor((HDC)wParam, RGB(255, 255, 255));
            return (INT_PTR)GetStockBrush(DC_BRUSH);
        }
        break;
    }

    return FALSE;
}
