/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     dmex    2026
 *
 */

#include <peview.h>

#define RUNTIME_NATIVEAOT_RTR_SIGNATURE 0x00525452UL
#define RUNTIME_NATIVEAOT_RTR_MAX_SECTIONS 0x50UL
#define RUNTIME_NATIVEAOT_RTR_SECTION_TYPE_STRING_TABLE 200UL
#define RUNTIME_NATIVEAOT_RTR_SECTION_TYPE_GC_STATIC_REGION 201UL
#define RUNTIME_NATIVEAOT_RTR_SECTION_TYPE_THREAD_STATIC_REGION 202UL
#define RUNTIME_NATIVEAOT_RTR_SECTION_TYPE_TYPE_MANAGER_INDIRECTION 204UL
#define RUNTIME_NATIVEAOT_RTR_SECTION_TYPE_EAGER_CCTOR 205UL
#define RUNTIME_NATIVEAOT_RTR_SECTION_TYPE_FROZEN_OBJECT_REGION 206UL
#define RUNTIME_NATIVEAOT_RTR_SECTION_TYPE_THREAD_STATIC_OFFSET_REGION 208UL
#define RUNTIME_NATIVEAOT_RTR_SECTION_TYPE_IMPORT_ADDRESS_TABLES 212UL
#define RUNTIME_NATIVEAOT_RTR_SECTION_TYPE_MODULE_INITIALIZER_LIST 213UL
#define RUNTIME_NATIVEAOT_RTR_SECTION_SIZE32 0x10UL
#define RUNTIME_NATIVEAOT_RTR_SECTION_SIZE64 0x18UL

typedef struct _PV_RUNTIME_DEBUG_CONTEXT
{
    HWND ListViewHandle;
    PH_LAYOUT_MANAGER LayoutManager;
    PPV_PROPPAGECONTEXT PropSheetContext;
} PV_RUNTIME_DEBUG_CONTEXT, *PPV_RUNTIME_DEBUG_CONTEXT;

#pragma pack(push, 1)
typedef struct _CLR_NATIVEAOT_RTR_SECTION
{
    ULONG Type;
    ULONG Flags;
    ULONG_PTR Start;
    ULONG_PTR End;
} CLR_NATIVEAOT_RTR_SECTION, *PCLR_NATIVEAOT_RTR_SECTION;

typedef struct _CLR_NATIVEAOT_RTR_HEADER
{
    ULONG Signature;
    USHORT MajorVersion;
    USHORT MinorVersion;
    ULONG Attributes;
    USHORT NumberOfSections;
    UCHAR EntrySize;
    UCHAR EntryType;
    CLR_NATIVEAOT_RTR_SECTION Sections[1];
} CLR_NATIVEAOT_RTR_HEADER, *PCLR_NATIVEAOT_RTR_HEADER;
#pragma pack(pop)

static LONG PvpRuntimeDebugCurrentGroup;

VOID PvRuntimeDebugAdd(
    _In_ HWND ListViewHandle,
    _In_ PCWSTR Name,
    _In_ PCWSTR Value
    )
{
    LONG index = PhAddListViewGroupItem(ListViewHandle, PvpRuntimeDebugCurrentGroup, MAXINT, Name, NULL);
    PhSetListViewSubItem(ListViewHandle, index, 1, Value);
}

PCWSTR PvRuntimeDebugSectionName(
    _In_ ULONG Type
    )
{
    switch (Type)
    {
    case RUNTIME_NATIVEAOT_RTR_SECTION_TYPE_STRING_TABLE: return L"String table";
    case RUNTIME_NATIVEAOT_RTR_SECTION_TYPE_GC_STATIC_REGION: return L"GC static region";
    case RUNTIME_NATIVEAOT_RTR_SECTION_TYPE_THREAD_STATIC_REGION: return L"Thread static region";
    case RUNTIME_NATIVEAOT_RTR_SECTION_TYPE_TYPE_MANAGER_INDIRECTION: return L"Type manager indirection";
    case RUNTIME_NATIVEAOT_RTR_SECTION_TYPE_EAGER_CCTOR: return L"Eager cctor";
    case RUNTIME_NATIVEAOT_RTR_SECTION_TYPE_FROZEN_OBJECT_REGION: return L"Frozen object region";
    case RUNTIME_NATIVEAOT_RTR_SECTION_TYPE_THREAD_STATIC_OFFSET_REGION: return L"Thread static offset region";
    case RUNTIME_NATIVEAOT_RTR_SECTION_TYPE_IMPORT_ADDRESS_TABLES: return L"Import address tables";
    case RUNTIME_NATIVEAOT_RTR_SECTION_TYPE_MODULE_INITIALIZER_LIST: return L"ModuleInitializerList";
    case 301: return L"TypeMap";
    case 302: return L"InvokeMap";
    case 304: return L"TypeMetadataMap";
    case 305: return L"StackTraceMetadataMap";
    case 306: return L"ArrayMap";
    case 307: return L"FieldAccessMap";
    case 308: return L"CCWTemplateData";
    case 309: return L"BlobResources";
    case 310: return L"DefaultConstructorMap";
    case 311: return L"StructMarshallingStubMap";
    case 312: return L"DelegateMarshallingStubMap";
    case 313: return L"GenericVirtualMethodTable";
    case 321: return L"InterfaceGenericVirtualMethodTable";
    case 322: return L"GenericMethodsHashtable";
    case 324: return L"GenericMethodsTemplateMap";
    case 325: return L"GenericTypesTemplateMap";
    case 327: return L"NativeLayoutInfo";
    case 330: return L"ExactMethodInstantiationHashtable";
    case 331: return L"GenericTypeHashtable";
    case 332: return L"TypeGenericInfoMap";
    case 333: return L"StaticsInfoHashtable";
    case 334: return L"ReflectionInvokeMap";
    case 335: return L"ClassConstructorContextMap";
    case 336: return L"EmbeddedMetadata";
    default: return L"Unknown";
    }
}

VOID PvRuntimeDebugAddNativeBuildInfo(
    _In_ HWND ListViewHandle
    )
{
    NTSTATUS status;
    PH_MAPPED_IMAGE_RESOURCES resources;
    ULONG i;

    PvpRuntimeDebugCurrentGroup = 2;
    PhAddListViewGroup(ListViewHandle, 2, L"NETNATIVEBUILDINFO");

    if (!NT_SUCCESS(PhGetMappedImageResources(&resources, &PvMappedImage)))
    {
        PvRuntimeDebugAdd(ListViewHandle, L"Status", L"Resource directory is unavailable or malformed.");
        return;
    }

    for (i = 0; i < resources.NumberOfEntries; i++)
    {
        PH_IMAGE_RESOURCE_ENTRY entry = resources.ResourceEntries[i];
        PIMAGE_RESOURCE_DIR_STRING_U typeString;
        PH_STRINGREF typeRef;
        PIMAGE_RESOURCE_DATA_ENTRY dataEntry;
        PVOID data = NULL;
        ULONG resourceLength = 0;
        PPH_STRING text;
        ULONG previewLength;
        ULONG resourceRva;
        PIMAGE_SECTION_HEADER section;
        WCHAR value[PH_INT64_STR_LEN_1];

        if (IS_INTRESOURCE(entry.Type))
            continue;

        typeString = (PIMAGE_RESOURCE_DIR_STRING_U)entry.Type;
        typeRef.Buffer = typeString->NameString;
        typeRef.Length = typeString->Length * sizeof(WCHAR);

        if (!PhEqualStringRef2(&typeRef, L"NETNATIVEBUILDINFO", TRUE))
            continue;

        if (!NT_SUCCESS(status = PhMappedImageRvaToVa(&PvMappedImage, entry.Offset, &dataEntry)))
        {
            PvRuntimeDebugAdd(ListViewHandle, L"Status", L"Resource data is outside the mapped image.");
            continue;
        }

        if (!NT_SUCCESS(status = PhMappedImageRvaToVa(&PvMappedImage, dataEntry->OffsetToData, &data)))
        {
            PvRuntimeDebugAdd(ListViewHandle, L"Status", L"Resource data is outside the mapped image.");
            continue;
        }

        if ((ULONG_PTR)dataEntry->OffsetToData >= PvMappedImage.ViewSize ||
            (ULONG_PTR)entry.Size > PvMappedImage.ViewSize - dataEntry->OffsetToData)
        {
            PvRuntimeDebugAdd(ListViewHandle, L"Status", L"Resource data is outside the mapped image.");
            continue;
        }

        status = PhGetMappedImageResource(
            &PvMappedImage,
            (PCWSTR)entry.Name,
            L"NETNATIVEBUILDINFO",
            (USHORT)entry.Language,
            &resourceLength,
            &data
            );

        if (!NT_SUCCESS(status) || !data || resourceLength != entry.Size)
        {
            PvRuntimeDebugAdd(ListViewHandle, L"Status", L"Resource data could not be read.");
            continue;
        }

        previewLength = min(resourceLength, 8192UL);
        resourceRva = dataEntry->OffsetToData;

        PvRuntimeDebugAdd(ListViewHandle, L"Language", PhaFormatString(L"0x%04x", (USHORT)entry.Language)->Buffer);
        PvRuntimeDebugAdd(ListViewHandle, L"Code page", PhaFormatString(L"%lu", entry.CodePage)->Buffer);
        PvRuntimeDebugAdd(ListViewHandle, L"Size", PhaFormatSize(entry.Size, ULONG_MAX)->Buffer);
        PvRuntimeDebugAdd(ListViewHandle, L"RVA", PhaFormatString(L"0x%lx", resourceRva)->Buffer);

        if (NT_SUCCESS(PhMappedImageRvaToSection(&PvMappedImage, resourceRva, &section)))
        {
            SIZE_T returnCount = 0;
            WCHAR nameBuffer[IMAGE_SIZEOF_SHORT_NAME + 1] = { 0 };

            status = PhCopyStringZFromUtf8(
                (PCSTR)section->Name,
                IMAGE_SIZEOF_SHORT_NAME,
                nameBuffer,
                IMAGE_SIZEOF_SHORT_NAME,
                &returnCount
                );

            if (NT_SUCCESS(status))
            {
                PvRuntimeDebugAdd(ListViewHandle, L"Section", nameBuffer);
            }
        }

        if (!previewLength)
        {
            PvRuntimeDebugAdd(ListViewHandle, L"Text", L"(empty)");
            continue;
        }

        text = PhConvertUtf8ToUtf16Ex((PCSTR)data, previewLength);
        if (text)
        {
            PvRuntimeDebugAdd(ListViewHandle, L"Text", text->Buffer);
            PhDereferenceObject(text);
        }
        else
        {
            PPH_STRING bytes = PhBufferToHexStringEx(data, min(previewLength, 64UL), FALSE);
            PvRuntimeDebugAdd(ListViewHandle, L"Text", L"Payload is not valid UTF-8.");
            PvRuntimeDebugAdd(ListViewHandle, L"Hex preview", bytes->Buffer);
            PhDereferenceObject(bytes);
        }

        if (entry.Size > previewLength)
        {
            PhPrintUInt32(value, entry.Size - previewLength);
            PvRuntimeDebugAdd(ListViewHandle, L"Preview truncated (bytes)", value);
        }

        PhFree(resources.ResourceEntries);
        PvpRuntimeDebugCurrentGroup = 0;
        return;
    }

    PhFree(resources.ResourceEntries);
    PvRuntimeDebugAdd(ListViewHandle, L"Status", L"Resource is not present.");
    PvpRuntimeDebugCurrentGroup = 0;
}

PCLR_NATIVEAOT_RTR_HEADER PvpFindRtrHeader(
    _Out_opt_ PULONG Rva
    )
{
    for (ULONG i = 0; i < PvMappedImage.NumberOfSections; i++)
    {
        PIMAGE_SECTION_HEADER section = &PvMappedImage.Sections[i];
        ULONG size = max(section->Misc.VirtualSize, section->SizeOfRawData);

        if (!(section->Characteristics & IMAGE_SCN_MEM_READ) || (section->Characteristics & IMAGE_SCN_MEM_EXECUTE) || size < UFIELD_OFFSET(CLR_NATIVEAOT_RTR_HEADER, Sections))
            continue;

        for (ULONG offset = 0; offset + UFIELD_OFFSET(CLR_NATIVEAOT_RTR_HEADER, Sections) <= size; offset += sizeof(ULONG))
        {
            PVOID address;
            PCLR_NATIVEAOT_RTR_HEADER header;

            if (!NT_SUCCESS(PhMappedImageRvaToVa(&PvMappedImage, section->VirtualAddress + offset, &address)))
                break;

            header = (PCLR_NATIVEAOT_RTR_HEADER)address;

            if (header->Signature != RUNTIME_NATIVEAOT_RTR_SIGNATURE || header->NumberOfSections > RUNTIME_NATIVEAOT_RTR_MAX_SECTIONS)
                continue;

            if (header->EntrySize < (PvMappedImage.Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC ? RUNTIME_NATIVEAOT_RTR_SECTION_SIZE32 : RUNTIME_NATIVEAOT_RTR_SECTION_SIZE64))
                continue;

            ULONG_PTR tableSize = (ULONG_PTR)header->NumberOfSections * header->EntrySize;
            if (tableSize / header->EntrySize != header->NumberOfSections || tableSize > size - offset - UFIELD_OFFSET(CLR_NATIVEAOT_RTR_HEADER, Sections))
                continue;

            if (Rva)
                *Rva = section->VirtualAddress + offset;
            return header;
        }
    }

    return NULL;
}

INT_PTR CALLBACK PvPeRuntimeDebugDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PPV_RUNTIME_DEBUG_CONTEXT context;

    if (uMsg == WM_INITDIALOG)
    {
        context = PhAllocateZero(sizeof(PV_RUNTIME_DEBUG_CONTEXT));
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
            PH_MAPPED_IMAGE_EXPORTS exports;
            PH_MAPPED_IMAGE_EXPORT_FUNCTION function;
            PVOID mappedAddress = NULL;
            PIMAGE_SECTION_HEADER section = NULL;
            ULONG_PTR rva;
            WCHAR buffer[PH_INT64_STR_LEN_1];

            context->ListViewHandle = GetDlgItem(hwndDlg, IDC_LIST);

            PhSetListViewStyle(context->ListViewHandle, TRUE, TRUE);
            PvConfigListViewFont(hwndDlg, context->ListViewHandle);
            PhAddListViewColumn(context->ListViewHandle, 0, 0, 0, LVCFMT_LEFT, 180, L"Name");
            PhAddListViewColumn(context->ListViewHandle, 1, 1, 1, LVCFMT_LEFT, 350, L"Value");
            PhSetExtendedListView(context->ListViewHandle);

            ListView_EnableGroupView(context->ListViewHandle, TRUE);
            PhAddListViewGroup(context->ListViewHandle, 0, L"NativeAOT Runtime Debug");
            PhAddListViewGroup(context->ListViewHandle, 1, L"ReadyToRun Header");

            PvConfigTreeBorders(context->ListViewHandle);
            PvSetListViewImageList(hwndDlg, context->ListViewHandle);

            //PhLoadListViewColumnsFromSetting(L"ImageRuntimeDebugListViewColumns", context->ListViewHandle);
            PhInitializeLayoutManager(&context->LayoutManager, hwndDlg);
            PhAddLayoutItem(&context->LayoutManager, context->ListViewHandle, NULL, PH_ANCHOR_ALL);

            PvRuntimeDebugAddNativeBuildInfo(context->ListViewHandle);

            if (!NT_SUCCESS(PhGetMappedImageExports(&exports, &PvMappedImage)) ||
                !NT_SUCCESS(PhGetMappedImageExportFunction(&exports, "DotNetRuntimeDebugHeader", 0, &function)) ||
                !function.Function)
            {
                PvRuntimeDebugAdd(context->ListViewHandle, L"Status", L"DotNetRuntimeDebugHeader is not available.");
            }
            else
            {
                // PhGetMappedImageExportFunction returns an RVA, not a VA.
                rva = (ULONG_PTR)function.Function;
                PhPrintPointer(buffer, (PVOID)rva);
                PvRuntimeDebugAdd(context->ListViewHandle, L"Export RVA", buffer);

                if (NT_SUCCESS(PhMappedImageRvaToVa(&PvMappedImage, rva, &mappedAddress)) &&
                    NT_SUCCESS(PhMappedImageRvaToSection(&PvMappedImage, rva, &section)))
                {
                    PCLR_NATIVEAOT_RTR_HEADER header = (PCLR_NATIVEAOT_RTR_HEADER)mappedAddress;
                    PPH_STRING bytes;
                    ULONG previewLength = 64;

                    if (section)
                    {
                        CHAR nameBuffer[IMAGE_SIZEOF_SHORT_NAME + 1] = { 0 };
                        PPH_STRING sectionName;
                        memcpy(nameBuffer, section->Name, IMAGE_SIZEOF_SHORT_NAME);
                        sectionName = PhConvertUtf8ToUtf16(nameBuffer);
                        PvRuntimeDebugAdd(context->ListViewHandle, L"Section", sectionName->Buffer);
                        PhDereferenceObject(sectionName);
                    }

                    if (rva < PvMappedImage.ViewSize)
                        previewLength = (ULONG)min((ULONG_PTR)previewLength, PvMappedImage.ViewSize - rva);
                    else
                        previewLength = 0;

                    PvRuntimeDebugAdd(context->ListViewHandle, L"Architecture",
                        PvMappedImage.Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC ? L"32-bit" : L"64-bit");
                    PvRuntimeDebugAdd(context->ListViewHandle, L"Data size", PhaFormatSize(previewLength, ULONG_MAX)->Buffer);

                    // NativeAOT ReadyToRun directory (the structure exposed by the
                    // runtime debug export) follows the layout used by ida-nativeaot.
                    if (previewLength >= UFIELD_OFFSET(CLR_NATIVEAOT_RTR_HEADER, Sections))
                    {
                        BOOLEAN is64 = PvMappedImage.Magic != IMAGE_NT_OPTIONAL_HDR32_MAGIC;
                        USHORT numberOfSections = header->NumberOfSections;
                        UCHAR entrySize = header->EntrySize;
                        UCHAR entryType = header->EntryType;
                        ULONG_PTR requiredSize;
                        WCHAR value[PH_INT64_STR_LEN_1];

                        PvRuntimeDebugAdd(context->ListViewHandle, L"Signature", PhaFormatString(L"0x%lx", header->Signature)->Buffer);
                        PvRuntimeDebugAdd(context->ListViewHandle, L"Major version", PhaFormatString(L"%u", header->MajorVersion)->Buffer);
                        PvRuntimeDebugAdd(context->ListViewHandle, L"Minor version", PhaFormatString(L"%u", header->MinorVersion)->Buffer);
                        PvRuntimeDebugAdd(context->ListViewHandle, L"Attributes", PhaFormatString(L"0x%lx", header->Attributes)->Buffer);
                        PvRuntimeDebugAdd(context->ListViewHandle, L"Number of sections", PhaFormatString(L"%u", numberOfSections)->Buffer);
                        PvRuntimeDebugAdd(context->ListViewHandle, L"Entry size", PhaFormatString(L"%u", entrySize)->Buffer);
                        PvRuntimeDebugAdd(context->ListViewHandle, L"Entry type", PhaFormatString(L"%u", entryType)->Buffer);
                        PvRuntimeDebugAdd(context->ListViewHandle, L"Layout",
                            header->Signature == RUNTIME_NATIVEAOT_RTR_SIGNATURE ?
                            (header->MajorVersion <= 8 ? L".NET 7 layout" : L".NET 8+ layout") :
                            L"NativeAOT runtime debug header");

                        requiredSize = (ULONG_PTR)numberOfSections * entrySize;
                        if (header->Signature == RUNTIME_NATIVEAOT_RTR_SIGNATURE && numberOfSections <= RUNTIME_NATIVEAOT_RTR_MAX_SECTIONS && entrySize >= (is64 ? RUNTIME_NATIVEAOT_RTR_SECTION_SIZE64 : RUNTIME_NATIVEAOT_RTR_SECTION_SIZE32) &&
                            (entrySize == 0 || requiredSize / entrySize == numberOfSections) &&
                            requiredSize <= (ULONG_PTR)previewLength - FIELD_OFFSET(CLR_NATIVEAOT_RTR_HEADER, Sections))
                        {
                            for (USHORT i = 0; i < numberOfSections; i++)
                            {
                                PCLR_NATIVEAOT_RTR_SECTION row = (PCLR_NATIVEAOT_RTR_SECTION)PTR_ADD_OFFSET(header->Sections, ((ULONG_PTR)i * entrySize));

                                PvpRuntimeDebugCurrentGroup = 100 + i;
                                PhAddListViewGroup(context->ListViewHandle, PvpRuntimeDebugCurrentGroup, PhaFormatString(
                                    L"RTR Section %u - %s",
                                    i,
                                    PvRuntimeDebugSectionName(row->Type)
                                    )->Buffer);

                                PhPrintPointer(value, (PVOID)row->Start);
                                PvRuntimeDebugAdd(context->ListViewHandle, PhaFormatString(L"Section %u type", i)->Buffer, PhaFormatString(L"0x%lx", row->Type)->Buffer);
                                PvRuntimeDebugAdd(context->ListViewHandle, PhaFormatString(L"Section %u name", i)->Buffer, PvRuntimeDebugSectionName(row->Type));
                                PvRuntimeDebugAdd(context->ListViewHandle, PhaFormatString(L"Section %u flags", i)->Buffer, PhaFormatString(L"0x%lx", row->Flags)->Buffer);
                                PvRuntimeDebugAdd(context->ListViewHandle, PhaFormatString(L"Section %u start", i)->Buffer, value);
                                PhPrintPointer(value, (PVOID)row->End);
                                PvRuntimeDebugAdd(context->ListViewHandle, PhaFormatString(L"Section %u end", i)->Buffer, value);
                            }
                        }
                        else if (header->Signature == RUNTIME_NATIVEAOT_RTR_SIGNATURE)
                        {
                            PvRuntimeDebugAdd(context->ListViewHandle, L"RTR sections", L"Invalid or truncated section table.");
                        }
                    }

                    if (header->Signature != RUNTIME_NATIVEAOT_RTR_SIGNATURE)
                    {
                        ULONG rtrRva;
                        PCLR_NATIVEAOT_RTR_HEADER rtrHeader = PvpFindRtrHeader(&rtrRva);

                        if (rtrHeader)
                        {
                            PvpRuntimeDebugCurrentGroup = 1;
                            PvRuntimeDebugAdd(context->ListViewHandle, L"RTR header RVA", PhaFormatString(L"0x%lx", rtrRva)->Buffer);
                            PvRuntimeDebugAdd(context->ListViewHandle, L"RTR signature", L"RTR");
                            PvRuntimeDebugAdd(context->ListViewHandle, L"RTR version", PhaFormatString(L"%u.%u", rtrHeader->MajorVersion, rtrHeader->MinorVersion)->Buffer);
                            PvRuntimeDebugAdd(context->ListViewHandle, L"RTR attributes", PhaFormatString(L"0x%lx", rtrHeader->Attributes)->Buffer);
                            PvRuntimeDebugAdd(context->ListViewHandle, L"RTR entry size", PhaFormatString(L"%u", rtrHeader->EntrySize)->Buffer);
                            PvRuntimeDebugAdd(context->ListViewHandle, L"RTR entry type", PhaFormatString(L"%u", rtrHeader->EntryType)->Buffer);
                            PvRuntimeDebugAdd(context->ListViewHandle, L"RTR sections", PhaFormatString(L"%u", rtrHeader->NumberOfSections)->Buffer);

                            for (USHORT i = 0; i < rtrHeader->NumberOfSections; i++)
                            {
                                PCLR_NATIVEAOT_RTR_SECTION row = (PCLR_NATIVEAOT_RTR_SECTION)PTR_ADD_OFFSET(rtrHeader->Sections, ((ULONG_PTR)i * rtrHeader->EntrySize));
                                WCHAR value[PH_INT64_STR_LEN_1];

                                PvpRuntimeDebugCurrentGroup = 100 + i;
                                PhAddListViewGroup(context->ListViewHandle, PvpRuntimeDebugCurrentGroup, PhaFormatString(
                                    L"RTR Section %u - %s",
                                    i,
                                    PvRuntimeDebugSectionName(row->Type)
                                    )->Buffer);

                                PvRuntimeDebugAdd(context->ListViewHandle, PhaFormatString(L"RTR section %u type", i)->Buffer, PhaFormatString(L"0x%lx", row->Type)->Buffer);
                                PvRuntimeDebugAdd(context->ListViewHandle, PhaFormatString(L"RTR section %u name", i)->Buffer, PvRuntimeDebugSectionName(row->Type));
                                PvRuntimeDebugAdd(context->ListViewHandle, PhaFormatString(L"RTR section %u flags", i)->Buffer, PhaFormatString(L"0x%lx", row->Flags)->Buffer);
                                PhPrintPointer(value, (PVOID)row->Start);
                                PvRuntimeDebugAdd(context->ListViewHandle, PhaFormatString(L"RTR section %u start", i)->Buffer, value);
                                PhPrintPointer(value, (PVOID)row->End);
                                PvRuntimeDebugAdd(context->ListViewHandle, PhaFormatString(L"RTR section %u end", i)->Buffer, value);
                            }
                            PvpRuntimeDebugCurrentGroup = 0;
                        }
                        else
                        {
                            PvRuntimeDebugAdd(context->ListViewHandle, L"RTR header", L"Not found in readable mapped sections.");
                        }
                    }

                    if (previewLength)
                    {
                        bytes = PhBufferToHexStringEx(mappedAddress, previewLength, FALSE);
                        PvRuntimeDebugAdd(context->ListViewHandle, L"Header bytes (preview)", bytes->Buffer);
                        PhDereferenceObject(bytes);
                    }
                }
                else
                {
                    PvRuntimeDebugAdd(context->ListViewHandle, L"Status", L"Export address is outside the mapped image.");
                }
            }

            PvThemeInitializePageDialog(hwndDlg, PhEnableThemeSupport);
            PvThemeApplyListView(context->ListViewHandle);
        }
        break;
    case WM_THEMECHANGED:
        PvThemeApplyListView(context->ListViewHandle);
        break;
    case WM_SIZE:
        PhLayoutManagerLayout(&context->LayoutManager);
        break;
    case WM_NOTIFY:
        PvHandleListViewNotifyForCopy(lParam, context->ListViewHandle);
        break;
    case WM_DESTROY:
        PhSaveListViewColumnsToSetting(L"ImageRuntimeDebugListViewColumns", context->ListViewHandle);
        PhDeleteLayoutManager(&context->LayoutManager);
        PhRemoveWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT);
        PhFree(context);
        break;
    }

    return FALSE;
}
