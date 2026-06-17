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
#include <oaidl.h>
#include <oleauto.h>

typedef struct _PV_RESOURCE_VIEWER_CONTEXT
{
    HWND WindowHandle;
    HWND ImageHandle;
    HWND TextHandle;
    HWND LabelHandle;
    PH_LAYOUT_MANAGER LayoutManager;
    RECT MinimumSize;

    PPH_STRING TypeName;
    PPH_STRING Text;
    BOOLEAN FixedFont;
    HICON IconHandle;
    HBITMAP BitmapHandle;
    HFONT FontHandle;
} PV_RESOURCE_VIEWER_CONTEXT, *PPV_RESOURCE_VIEWER_CONTEXT;

// Group icon/cursor directory structures (packed).
#include <pshpack2.h>
typedef struct _PV_GRPICONDIRENTRY
{
    UCHAR Width;
    UCHAR Height;
    UCHAR ColorCount;
    UCHAR Reserved;
    USHORT Planes;     // for cursors: HotspotX
    USHORT BitCount;   // for cursors: HotspotY
    ULONG BytesInRes;
    USHORT Id;
} PV_GRPICONDIRENTRY, *PPV_GRPICONDIRENTRY;

typedef struct _PV_GRPICONDIR
{
    USHORT Reserved;
    USHORT Type;       // 1 = icon, 2 = cursor
    USHORT Count;
    PV_GRPICONDIRENTRY Entries[1];
} PV_GRPICONDIR, *PPV_GRPICONDIR;
#include <poppack.h>

static PCWSTR PvpVarTypeToString(
    _In_ VARTYPE VarType
    )
{
    switch (VarType & VT_TYPEMASK)
    {
    case VT_EMPTY: return L"void";
    case VT_NULL: return L"null";
    case VT_I2: return L"short";
    case VT_I4: return L"long";
    case VT_R4: return L"single";
    case VT_R8: return L"double";
    case VT_CY: return L"CURRENCY";
    case VT_DATE: return L"DATE";
    case VT_BSTR: return L"BSTR";
    case VT_DISPATCH: return L"IDispatch*";
    case VT_ERROR: return L"SCODE";
    case VT_BOOL: return L"VARIANT_BOOL";
    case VT_VARIANT: return L"VARIANT";
    case VT_UNKNOWN: return L"IUnknown*";
    case VT_DECIMAL: return L"DECIMAL";
    case VT_I1: return L"char";
    case VT_UI1: return L"unsigned char";
    case VT_UI2: return L"unsigned short";
    case VT_UI4: return L"unsigned long";
    case VT_I8: return L"int64";
    case VT_UI8: return L"uint64";
    case VT_INT: return L"int";
    case VT_UINT: return L"unsigned int";
    case VT_VOID: return L"void";
    case VT_HRESULT: return L"HRESULT";
    case VT_PTR: return L"void*";
    case VT_SAFEARRAY: return L"SAFEARRAY";
    case VT_CARRAY: return L"array";
    case VT_USERDEFINED: return L"UDT";
    case VT_LPSTR: return L"LPSTR";
    case VT_LPWSTR: return L"LPWSTR";
    default: return L"VARIANT";
    }
}

static VOID PvpAppendTypeDescString(
    _Inout_ PPH_STRING_BUILDER StringBuilder,
    _In_ ITypeInfo* TypeInfo,
    _In_ TYPEDESC* TypeDesc
    )
{
    if (TypeDesc->vt == VT_PTR)
    {
        PvpAppendTypeDescString(StringBuilder, TypeInfo, TypeDesc->lptdesc);
        PhAppendStringBuilder2(StringBuilder, L"*");
    }
    else if (TypeDesc->vt == VT_USERDEFINED)
    {
        ITypeInfo* refTypeInfo;
        BSTR name = NULL;

        if (SUCCEEDED(ITypeInfo_GetRefTypeInfo(TypeInfo, TypeDesc->hreftype, &refTypeInfo)))
        {
            ITypeInfo_GetDocumentation(refTypeInfo, MEMBERID_NIL, &name, NULL, NULL, NULL);
            ITypeInfo_Release(refTypeInfo);
        }

        if (name)
        {
            PhAppendStringBuilder2(StringBuilder, name);
            SysFreeString(name);
        }
        else
        {
            PhAppendStringBuilder2(StringBuilder, L"UDT");
        }
    }
    else
    {
        PhAppendStringBuilder2(StringBuilder, (PWSTR)PvpVarTypeToString(TypeDesc->vt));
    }
}

static PCWSTR PvpTypeKindToString(
    _In_ TYPEKIND TypeKind
    )
{
    switch (TypeKind)
    {
    case TKIND_ENUM: return L"enum";
    case TKIND_RECORD: return L"struct";
    case TKIND_MODULE: return L"module";
    case TKIND_INTERFACE: return L"interface";
    case TKIND_DISPATCH: return L"dispinterface";
    case TKIND_COCLASS: return L"coclass";
    case TKIND_ALIAS: return L"typedef";
    case TKIND_UNION: return L"union";
    default: return L"type";
    }
}

static VOID PvpAppendTypeInfo(
    _Inout_ PPH_STRING_BUILDER StringBuilder,
    _In_ ITypeInfo* TypeInfo
    )
{
    TYPEATTR* typeAttr;
    BSTR typeName = NULL;
    BSTR typeDoc = NULL;

    if (FAILED(ITypeInfo_GetTypeAttr(TypeInfo, &typeAttr)))
        return;

    ITypeInfo_GetDocumentation(TypeInfo, MEMBERID_NIL, &typeName, &typeDoc, NULL, NULL);

    PhAppendFormatStringBuilder(
        StringBuilder,
        L"%s %s",
        PvpTypeKindToString(typeAttr->typekind),
        typeName ? typeName : L"<unnamed>"
        );

    if (typeDoc && typeDoc[0])
        PhAppendFormatStringBuilder(StringBuilder, L"   // %s", typeDoc);

    PhAppendStringBuilder2(StringBuilder, L"\r\n{\r\n");

    // Variables / enum members / fields.
    for (ULONG i = 0; i < typeAttr->cVars; i++)
    {
        VARDESC* varDesc;
        BSTR varName = NULL;

        if (FAILED(ITypeInfo_GetVarDesc(TypeInfo, i, &varDesc)))
            continue;

        ITypeInfo_GetDocumentation(TypeInfo, varDesc->memid, &varName, NULL, NULL, NULL);

        PhAppendStringBuilder2(StringBuilder, L"    ");

        if (typeAttr->typekind == TKIND_ENUM)
        {
            PhAppendStringBuilder2(StringBuilder, varName ? varName : L"<member>");

            if (varDesc->varkind == VAR_CONST && varDesc->lpvarValue)
            {
                VARIANT value;
                VariantInit(&value);

                if (SUCCEEDED(VariantChangeType(&value, varDesc->lpvarValue, 0, VT_I8)))
                {
                    PhAppendFormatStringBuilder(StringBuilder, L" = %lld", value.llVal);
                    VariantClear(&value);
                }
            }
        }
        else
        {
            PvpAppendTypeDescString(StringBuilder, TypeInfo, &varDesc->elemdescVar.tdesc);
            PhAppendFormatStringBuilder(StringBuilder, L" %s", varName ? varName : L"<field>");
        }

        PhAppendStringBuilder2(StringBuilder, L";\r\n");

        if (varName)
            SysFreeString(varName);
        ITypeInfo_ReleaseVarDesc(TypeInfo, varDesc);
    }

    // Functions / methods.
    for (ULONG i = 0; i < typeAttr->cFuncs; i++)
    {
        FUNCDESC* funcDesc;
        BSTR names[64] = { 0 };
        UINT nameCount = 0;

        if (FAILED(ITypeInfo_GetFuncDesc(TypeInfo, i, &funcDesc)))
            continue;

        ITypeInfo_GetNames(TypeInfo, funcDesc->memid, names, RTL_NUMBER_OF(names), &nameCount);

        PhAppendStringBuilder2(StringBuilder, L"    ");

        if (funcDesc->memid != MEMBERID_NIL && (funcDesc->invkind & (INVOKE_PROPERTYGET | INVOKE_PROPERTYPUT | INVOKE_PROPERTYPUTREF)))
        {
            if (funcDesc->invkind == INVOKE_PROPERTYGET)
                PhAppendStringBuilder2(StringBuilder, L"[propget] ");
            else
                PhAppendStringBuilder2(StringBuilder, L"[propput] ");
        }

        PvpAppendTypeDescString(StringBuilder, TypeInfo, &funcDesc->elemdescFunc.tdesc);
        PhAppendFormatStringBuilder(StringBuilder, L" %s(", nameCount > 0 && names[0] ? names[0] : L"<method>");

        for (SHORT p = 0; p < funcDesc->cParams; p++)
        {
            if (p != 0)
                PhAppendStringBuilder2(StringBuilder, L", ");

            PvpAppendTypeDescString(StringBuilder, TypeInfo, &funcDesc->lprgelemdescParam[p].tdesc);

            // Parameter names follow the method name in the GetNames array.
            if ((UINT)(p + 1) < nameCount && names[p + 1])
                PhAppendFormatStringBuilder(StringBuilder, L" %s", names[p + 1]);
        }

        PhAppendFormatStringBuilder(StringBuilder, L");   // dispid 0x%lx\r\n", (ULONG)funcDesc->memid);

        for (UINT n = 0; n < nameCount; n++)
        {
            if (names[n])
                SysFreeString(names[n]);
        }

        ITypeInfo_ReleaseFuncDesc(TypeInfo, funcDesc);
    }

    PhAppendStringBuilder2(StringBuilder, L"}\r\n\r\n");

    if (typeName)
        SysFreeString(typeName);
    if (typeDoc)
        SysFreeString(typeDoc);
    ITypeInfo_ReleaseTypeAttr(TypeInfo, typeAttr);
}

static PPH_STRING PvpDecodeTypeLib(
    _In_ ULONG_PTR Name
    )
{
    static PH_INITONCE initOnce = PH_INITONCE_INIT;
    static HRESULT (WINAPI* LoadTypeLibEx_I)(LPCOLESTR, REGKIND, ITypeLib**) = NULL;
    PPH_STRING result = NULL;
    PPH_STRING moduleSpec;
    ITypeLib* typeLib = NULL;
    PH_STRING_BUILDER stringBuilder;
    UINT typeInfoCount;

    if (PhBeginInitOnce(&initOnce))
    {
        PVOID oleaut32 = PhLoadLibrary(L"oleaut32.dll");

        if (oleaut32)
            LoadTypeLibEx_I = PhGetDllBaseProcedureAddress(oleaut32, "LoadTypeLibEx", 0);

        PhEndInitOnce(&initOnce);
    }

    if (!LoadTypeLibEx_I)
        return NULL;

    // The type-library loader accepts a "path\index" form to read an embedded TYPELIB resource.
    if (IS_INTRESOURCE(Name))
        moduleSpec = PhFormatString(L"%s\\%lu", PhGetString(PvFileName), (ULONG)Name);
    else
        moduleSpec = PhReferenceObject(PvFileName);

    if (FAILED(LoadTypeLibEx_I(PhGetString(moduleSpec), REGKIND_NONE, &typeLib)))
    {
        PhDereferenceObject(moduleSpec);
        return NULL;
    }

    PhInitializeStringBuilder(&stringBuilder, 0x400);

    typeInfoCount = ITypeLib_GetTypeInfoCount(typeLib);

    {
        BSTR libName = NULL;
        BSTR libDoc = NULL;

        ITypeLib_GetDocumentation(typeLib, -1, &libName, &libDoc, NULL, NULL);

        if (libName)
        {
            PhAppendFormatStringBuilder(&stringBuilder, L"library %s", libName);

            if (libDoc && libDoc[0])
                PhAppendFormatStringBuilder(&stringBuilder, L"   // %s", libDoc);

            PhAppendStringBuilder2(&stringBuilder, L"\r\n\r\n");
            SysFreeString(libName);
        }

        if (libDoc)
            SysFreeString(libDoc);
    }

    for (UINT i = 0; i < typeInfoCount; i++)
    {
        ITypeInfo* typeInfo;

        if (SUCCEEDED(ITypeLib_GetTypeInfo(typeLib, i, &typeInfo)))
        {
            PvpAppendTypeInfo(&stringBuilder, typeInfo);
            ITypeInfo_Release(typeInfo);
        }
    }

    ITypeLib_Release(typeLib);
    PhDereferenceObject(moduleSpec);

    result = PhFinalStringBuilderString(&stringBuilder);

    if (PhIsNullOrEmptyString(result))
    {
        PhClearReference(&result);
        return NULL;
    }

    return result;
}

static PPH_STRING PvpFormatHexDump(
    _In_reads_bytes_(Length) PUCHAR Buffer,
    _In_ ULONG Length
    )
{
    PH_STRING_BUILDER stringBuilder;
    ULONG displayLength;

    // Cap the dump so we don't choke the edit control on huge resources.
    displayLength = min(Length, 0x10000);

    PhInitializeStringBuilder(&stringBuilder, displayLength * 4);

    for (ULONG offset = 0; offset < displayLength; offset += 16)
    {
        WCHAR line[PH_PTR_STR_LEN_1];
        ULONG count = min(16, displayLength - offset);

        PhPrintPointer(line, UlongToPtr(offset));
        PhAppendStringBuilder2(&stringBuilder, line);
        PhAppendStringBuilder2(&stringBuilder, L"  ");

        for (ULONG i = 0; i < 16; i++)
        {
            if (i < count)
                PhAppendFormatStringBuilder(&stringBuilder, L"%02x ", Buffer[offset + i]);
            else
                PhAppendStringBuilder2(&stringBuilder, L"   ");
        }

        PhAppendStringBuilder2(&stringBuilder, L" ");

        for (ULONG i = 0; i < count; i++)
        {
            UCHAR value = Buffer[offset + i];
            WCHAR character = (value >= 0x20 && value < 0x7f) ? (WCHAR)value : L'.';
            PhAppendCharStringBuilder(&stringBuilder, character);
        }

        PhAppendStringBuilder2(&stringBuilder, L"\r\n");
    }

    if (Length > displayLength)
        PhAppendFormatStringBuilder(&stringBuilder, L"\r\n... (%lu bytes truncated)\r\n", Length - displayLength);

    return PhFinalStringBuilderString(&stringBuilder);
}

static PPH_STRING PvpFormatTextResource(
    _In_reads_bytes_(Length) PUCHAR Buffer,
    _In_ ULONG Length
    )
{
    // UTF-16 LE BOM.
    if (Length >= 2 && Buffer[0] == 0xff && Buffer[1] == 0xfe)
        return PhCreateStringEx((PWCHAR)(Buffer + 2), Length - 2);

    // UTF-8 BOM.
    if (Length >= 3 && Buffer[0] == 0xef && Buffer[1] == 0xbb && Buffer[2] == 0xbf)
        return PhConvertUtf8ToUtf16Ex((PCSTR)(Buffer + 3), Length - 3);

    return PhConvertUtf8ToUtf16Ex((PCSTR)Buffer, Length);
}

static HICON PvpCreateIconFromResource(
    _In_ PPH_MAPPED_IMAGE MappedImage,
    _In_ PPH_MAPPED_IMAGE_RESOURCES Resources,
    _In_reads_bytes_(Length) PVOID Buffer,
    _In_ ULONG Length,
    _In_ BOOLEAN IsGroup,
    _In_ BOOLEAN IsIcon
    )
{
    PVOID iconBuffer = Buffer;
    ULONG iconLength = Length;

    if (IsGroup)
    {
        LONG iconId;
        PVOID resolvedBuffer = NULL;
        ULONG resolvedLength = 0;

        if (!(iconId = LookupIconIdFromDirectoryEx(Buffer, IsIcon, 0, 0, LR_DEFAULTCOLOR)))
            return NULL;

        // Locate the matching RT_ICON/RT_CURSOR resource by Name == iconId.
        for (SIZE_T i = 0; i < Resources->NumberOfEntries; i++)
        {
            PH_IMAGE_RESOURCE_ENTRY entry = Resources->ResourceEntries[i];
            PVOID data;

            if (!IS_INTRESOURCE(entry.Name) || (ULONG)entry.Name != (ULONG)iconId)
                continue;
            if (entry.Type != (ULONG_PTR)(IsIcon ? RT_ICON : RT_CURSOR))
                continue;

            if (NT_SUCCESS(PhMappedImageRvaToVa(MappedImage, entry.Offset, &data)))
            {
                resolvedBuffer = data;
                resolvedLength = entry.Size;
            }
            break;
        }

        if (!resolvedBuffer)
            return NULL;

        iconBuffer = resolvedBuffer;
        iconLength = resolvedLength;
    }

    return CreateIconFromResourceEx(
        iconBuffer,
        iconLength,
        IsIcon,
        0x30000,
        0,
        0,
        LR_DEFAULTSIZE | LR_DEFAULTCOLOR
        );
}

static HBITMAP PvpCreateBitmapFromResource(
    _In_reads_bytes_(Length) PVOID Buffer,
    _In_ ULONG Length
    )
{
    PBITMAPINFOHEADER header = Buffer;
    HBITMAP bitmapHandle = NULL;
    HDC hdc;
    ULONG paletteBytes = 0;
    ULONG headerSize;

    if (Length < sizeof(BITMAPINFOHEADER) || header->biSize < sizeof(BITMAPINFOHEADER))
        return NULL;

    headerSize = header->biSize;

    if (header->biBitCount <= 8)
    {
        ULONG colorCount = header->biClrUsed ? header->biClrUsed : (1ul << header->biBitCount);
        paletteBytes = colorCount * sizeof(RGBQUAD);
    }
    else if (header->biCompression == BI_BITFIELDS)
    {
        paletteBytes = 3 * sizeof(ULONG);
    }

    if (headerSize + paletteBytes > Length)
        return NULL;

    if (hdc = GetDC(NULL))
    {
        bitmapHandle = CreateDIBitmap(
            hdc,
            header,
            CBM_INIT,
            (PUCHAR)Buffer + headerSize + paletteBytes,
            (PBITMAPINFO)header,
            DIB_RGB_COLORS
            );

        ReleaseDC(NULL, hdc);
    }

    return bitmapHandle;
}

static BOOLEAN PvpResourceNameEquals(
    _In_ ULONG_PTR Type,
    _In_ PCWSTR Name
    )
{
    if (IS_INTRESOURCE(Type))
        return FALSE;

    {
        PIMAGE_RESOURCE_DIR_STRING_U resourceString = (PIMAGE_RESOURCE_DIR_STRING_U)Type;
        PH_STRINGREF nameSr;
        PH_STRINGREF compareSr;

        nameSr.Buffer = resourceString->NameString;
        nameSr.Length = resourceString->Length * sizeof(WCHAR);
        PhInitializeStringRef(&compareSr, Name);

        return PhEqualStringRef(&nameSr, &compareSr, TRUE);
    }
}

INT_PTR CALLBACK PvpResourceViewerDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PPV_RESOURCE_VIEWER_CONTEXT context;

    if (uMsg == WM_INITDIALOG)
    {
        context = (PPV_RESOURCE_VIEWER_CONTEXT)lParam;
        PhSetWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT, context);
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
            HICON smallIcon;
            HICON largeIcon;

            context->WindowHandle = hwndDlg;
            context->LabelHandle = GetDlgItem(hwndDlg, IDC_RESTYPE);
            context->ImageHandle = GetDlgItem(hwndDlg, IDC_PREVIEW);
            context->TextHandle = GetDlgItem(hwndDlg, IDC_TEXT);

            PhCenterWindow(hwndDlg, GetParent(hwndDlg));

            PhGetStockApplicationIcon(&smallIcon, &largeIcon, PhGetWindowDpi(hwndDlg));
            SendMessage(hwndDlg, WM_SETICON, ICON_SMALL, (LPARAM)smallIcon);
            SendMessage(hwndDlg, WM_SETICON, ICON_BIG, (LPARAM)largeIcon);

            if (context->TypeName)
                PhSetWindowText(context->LabelHandle, PhGetString(context->TypeName));

            if (context->IconHandle)
            {
                ShowWindow(context->TextHandle, SW_HIDE);
                PhSetWindowStyle(context->ImageHandle, SS_TYPEMASK | SS_CENTERIMAGE | SS_REALSIZECONTROL, SS_ICON | SS_CENTERIMAGE);
                SendMessage(context->ImageHandle, STM_SETIMAGE, IMAGE_ICON, (LPARAM)context->IconHandle);
            }
            else if (context->BitmapHandle)
            {
                ShowWindow(context->TextHandle, SW_HIDE);
                PhSetWindowStyle(context->ImageHandle, SS_TYPEMASK | SS_CENTERIMAGE | SS_REALSIZECONTROL, SS_BITMAP | SS_CENTERIMAGE);
                SendMessage(context->ImageHandle, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)context->BitmapHandle);
            }
            else
            {
                ShowWindow(context->ImageHandle, SW_HIDE);
                ShowWindow(context->TextHandle, SW_SHOW);

                SendMessage(context->TextHandle, EM_SETLIMITTEXT, ULONG_MAX, 0);

                if (context->FixedFont)
                {
                    context->FontHandle = CreateFont(
                        -PhMultiplyDivideSigned(9, PhGetWindowDpi(hwndDlg), 72),
                        0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                        ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                        PhFontQuality, FIXED_PITCH | FF_MODERN, L"Consolas"
                        );

                    if (context->FontHandle)
                        SetWindowFont(context->TextHandle, context->FontHandle, TRUE);
                }

                if (context->Text)
                    PhSetWindowText(context->TextHandle, PhGetString(context->Text));
            }

            PhInitializeLayoutManager(&context->LayoutManager, hwndDlg);
            PhAddLayoutItem(&context->LayoutManager, context->LabelHandle, NULL, PH_ANCHOR_TOP | PH_ANCHOR_LEFT | PH_ANCHOR_RIGHT);
            PhAddLayoutItem(&context->LayoutManager, context->ImageHandle, NULL, PH_ANCHOR_ALL);
            PhAddLayoutItem(&context->LayoutManager, context->TextHandle, NULL, PH_ANCHOR_ALL);
            PhLayoutManagerLayout(&context->LayoutManager);

            context->MinimumSize.left = 0;
            context->MinimumSize.top = 0;
            context->MinimumSize.right = 200;
            context->MinimumSize.bottom = 150;
            MapDialogRect(hwndDlg, &context->MinimumSize);

            PhInitializeWindowTheme(hwndDlg, PhEnableThemeSupport);
        }
        break;
    case WM_DESTROY:
        {
            PhDeleteLayoutManager(&context->LayoutManager);

            if (context->IconHandle)
                DestroyIcon(context->IconHandle);
            if (context->BitmapHandle)
                DeleteBitmap(context->BitmapHandle);
            if (context->FontHandle)
                DeleteFont(context->FontHandle);
            if (context->TypeName)
                PhDereferenceObject(context->TypeName);
            if (context->Text)
                PhDereferenceObject(context->Text);

            PhRemoveWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT);
            PhFree(context);
        }
        break;
    case WM_DPICHANGED:
        {
            PhLayoutManagerUpdate(&context->LayoutManager, LOWORD(wParam));
            PhLayoutManagerLayout(&context->LayoutManager);
        }
        break;
    case WM_SIZE:
        {
            PhLayoutManagerLayout(&context->LayoutManager);
        }
        break;
    case WM_SIZING:
        {
            PhResizingMinimumSize((PRECT)lParam, wParam, context->MinimumSize.right, context->MinimumSize.bottom);
        }
        break;
    case WM_COMMAND:
        {
            switch (GET_WM_COMMAND_ID(wParam, lParam))
            {
            case IDCANCEL:
            case IDOK:
                DestroyWindow(hwndDlg);
                return TRUE;
            }
        }
        break;
    case WM_CLOSE:
        {
            DestroyWindow(hwndDlg);
        }
        return TRUE;
    case WM_CTLCOLORBTN:
    case WM_CTLCOLORDLG:
    case WM_CTLCOLORSTATIC:
        {
            SetBkMode((HDC)wParam, TRANSPARENT);
            SetTextColor((HDC)wParam, RGB(0, 0, 0));
            SetDCBrushColor((HDC)wParam, RGB(255, 255, 255));
            return (INT_PTR)PhGetStockBrush(DC_BRUSH);
        }
        break;
    }

    return FALSE;
}

VOID PvShowResourceViewerDialog(
    _In_ HWND ParentWindowHandle,
    _In_ PPH_MAPPED_IMAGE MappedImage,
    _In_ ULONG ResourceOffset
    )
{
    PH_MAPPED_IMAGE_RESOURCES resources = { 0 };
    PPV_RESOURCE_VIEWER_CONTEXT context;
    HWND dialogHandle;
    BOOLEAN found = FALSE;
    PH_IMAGE_RESOURCE_ENTRY entry = { 0 };
    PVOID data = NULL;

    if (!NT_SUCCESS(PhGetMappedImageResources(&resources, MappedImage)))
    {
        PhShowError(ParentWindowHandle, L"%s", L"Unable to enumerate the image resources.");
        return;
    }

    for (SIZE_T i = 0; i < resources.NumberOfEntries; i++)
    {
        if (resources.ResourceEntries[i].Offset == ResourceOffset)
        {
            entry = resources.ResourceEntries[i];
            found = TRUE;
            break;
        }
    }

    if (!found || entry.Size == 0 || !NT_SUCCESS(PhMappedImageRvaToVa(MappedImage, entry.Offset, &data)) || !data)
    {
        PhFree(resources.ResourceEntries);
        PhShowError(ParentWindowHandle, L"%s", L"Unable to locate the resource data.");
        return;
    }

    context = PhAllocateZero(sizeof(PV_RESOURCE_VIEWER_CONTEXT));

    if (IS_INTRESOURCE(entry.Type))
    {
        context->TypeName = PvpGetResourceTypeString(entry.Type);
    }
    else
    {
        PIMAGE_RESOURCE_DIR_STRING_U resourceString = (PIMAGE_RESOURCE_DIR_STRING_U)entry.Type;
        context->TypeName = PhCreateStringEx(resourceString->NameString, resourceString->Length * sizeof(WCHAR));
    }

    __try
    {
        if (entry.Type == (ULONG_PTR)RT_ICON || entry.Type == (ULONG_PTR)RT_GROUP_ICON)
        {
            context->IconHandle = PvpCreateIconFromResource(
                MappedImage, &resources, data, entry.Size,
                entry.Type == (ULONG_PTR)RT_GROUP_ICON, TRUE);
        }
        else if (entry.Type == (ULONG_PTR)RT_CURSOR || entry.Type == (ULONG_PTR)RT_GROUP_CURSOR)
        {
            context->IconHandle = PvpCreateIconFromResource(
                MappedImage, &resources, data, entry.Size,
                entry.Type == (ULONG_PTR)RT_GROUP_CURSOR, FALSE);
        }
        else if (entry.Type == (ULONG_PTR)RT_BITMAP)
        {
            context->BitmapHandle = PvpCreateBitmapFromResource(data, entry.Size);
        }
        else if (PvpResourceNameEquals(entry.Type, L"TYPELIB"))
        {
            context->Text = PvpDecodeTypeLib(entry.Name);
            context->FixedFont = TRUE;
        }
        else if (
            entry.Type == (ULONG_PTR)RT_HTML ||
            entry.Type == (ULONG_PTR)RT_MANIFEST ||
            PvpResourceNameEquals(entry.Type, L"UIFILE") ||
            PvpResourceNameEquals(entry.Type, L"WEVT_TEMPLATE")
            )
        {
            context->Text = PvpFormatTextResource(data, entry.Size);
        }

        // Fall back to a hex dump for anything we couldn't render above.
        if (!context->IconHandle && !context->BitmapHandle && !context->Text)
        {
            context->Text = PvpFormatHexDump(data, entry.Size);
            context->FixedFont = TRUE;
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        NOTHING;
    }

    PhFree(resources.ResourceEntries);

    dialogHandle = PhCreateDialog(
        PhInstanceHandle,
        MAKEINTRESOURCE(IDD_PERESOURCEVIEW),
        ParentWindowHandle,
        PvpResourceViewerDlgProc,
        context
        );

    if (!dialogHandle)
    {
        if (context->IconHandle)
            DestroyIcon(context->IconHandle);
        if (context->BitmapHandle)
            DeleteBitmap(context->BitmapHandle);
        if (context->TypeName)
            PhDereferenceObject(context->TypeName);
        if (context->Text)
            PhDereferenceObject(context->Text);
        PhFree(context);
        return;
    }

    ShowWindow(dialogHandle, SW_SHOW);
}
