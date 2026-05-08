/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     jxy-s   2025
 *
 */

#include <peview.h>
#include <xmllite.h>
#include <shlwapi.h>

typedef struct _PV_APP_MANIFEST_CONTEXT
{
    HWND WindowHandle;
    HWND EditWindow;
    HIMAGELIST ListViewImageList;
    PH_LAYOUT_MANAGER LayoutManager;
    PPV_PROPPAGECONTEXT PropSheetContext;
} PV_APP_MANIFEST_CONTEXT, *PPV_APP_MANIFEST_CONTEXT;

VOID PvpShowAppManifest(
    _In_ PPV_APP_MANIFEST_CONTEXT Context
    )
{
    static PH_INITONCE initOnce = PH_INITONCE_INIT;
    static typeof(&CreateXmlReader) CreateXmlReader_I = NULL;
    static typeof(&CreateXmlWriter) CreateXmlWriter_I = NULL;
    static typeof(&SHCreateMemStream) SHCreateMemStream_I = NULL;
    static typeof(&CreateStreamOnHGlobal) CreateStreamOnHGlobal_I = NULL;
    ULONG manifestLength;
    PVOID manifestBuffer;

    if (PhBeginInitOnce(&initOnce))
    {
        PVOID xmllite = PhLoadLibrary(L"xmllite.dll");
        PVOID shlwapi = PhLoadLibrary(L"shlwapi.dll");
        PVOID ole32 = PhLoadLibrary(L"ole32.dll");

        if (xmllite)
        {
            CreateXmlReader_I = PhGetDllBaseProcedureAddress(xmllite, "CreateXmlReader", 0);
            CreateXmlWriter_I = PhGetDllBaseProcedureAddress(xmllite, "CreateXmlWriter", 0);
        }

        if (shlwapi)
        {
            SHCreateMemStream_I = PhGetDllBaseProcedureAddress(shlwapi, "SHCreateMemStream", 0);
        }

        if (ole32)
        {
            CreateStreamOnHGlobal_I = PhGetDllBaseProcedureAddress(ole32, "CreateStreamOnHGlobal", 0);
        }

        PhEndInitOnce(&initOnce);
    }

    if (NT_SUCCESS(PhGetMappedImageResource(
        &PvMappedImage,
        CREATEPROCESS_MANIFEST_RESOURCE_ID,
        RT_MANIFEST,
        0,
        &manifestLength,
        &manifestBuffer
        )))
    {
        IStream* dataInputStream = NULL;
        IStream* dataOutputStream = NULL;
        IXmlReader* streamReader = NULL;
        IXmlWriter* streamWriter = NULL;
        STATSTG statstg;
        LARGE_INTEGER move;

        move.QuadPart = 0;

        if (!(CreateXmlReader_I && CreateXmlWriter_I && SHCreateMemStream_I && CreateStreamOnHGlobal_I))
            goto Fallback;

        dataInputStream = SHCreateMemStream_I((const BYTE*)manifestBuffer, manifestLength);
        if (!dataInputStream)
            goto Fallback;

        if (FAILED(CreateXmlReader_I(&IID_IXmlReader, (void**)&streamReader, NULL)))
            goto Fallback;

        if (FAILED(IXmlReader_SetInput(streamReader, (IUnknown*)dataInputStream)))
            goto Fallback;

        if (FAILED(CreateStreamOnHGlobal_I(NULL, TRUE, &dataOutputStream)))
            goto Fallback;

        if (FAILED(CreateXmlWriter_I(&IID_IXmlWriter, (void**)&streamWriter, NULL)))
            goto Fallback;

        if (FAILED(IXmlWriter_SetOutput(streamWriter, (IUnknown*)dataOutputStream)))
            goto Fallback;

        IXmlWriter_SetProperty(streamWriter, XmlWriterProperty_Indent, TRUE);

        while (IXmlReader_Read(streamReader, NULL) == S_OK)
        {
            IXmlWriter_WriteNode(streamWriter, streamReader, TRUE);
        }

        IXmlWriter_Flush(streamWriter);

        if (SUCCEEDED(IStream_Stat(dataOutputStream, &statstg, STATFLAG_NONAME)))
        {
            ULONG bufferLength = statstg.cbSize.LowPart;
            PVOID buffer = PhAllocate(bufferLength);

            if (buffer)
            {
                IStream_Seek(dataOutputStream, move, STREAM_SEEK_SET, NULL);

                if (SUCCEEDED(dataOutputStream->lpVtbl->Read(dataOutputStream, buffer, bufferLength, NULL)))
                {
                    PPH_STRING manifestString;

                    if (manifestString = PhConvertUtf8ToUtf16Ex((PCSTR)buffer, bufferLength))
                    {
                        SendMessage(Context->EditWindow, WM_SETREDRAW, FALSE, 0);
                        SendMessage(Context->EditWindow, WM_SETTEXT, FALSE, (LPARAM)manifestString->Buffer);
                        SendMessage(Context->EditWindow, WM_SETREDRAW, TRUE, 0);
                        PhDereferenceObject(manifestString);
                    }
                }
                PhFree(buffer);
            }
        }

    Cleanup:
        if (streamWriter) IXmlWriter_Release(streamWriter);
        if (dataOutputStream) dataOutputStream->lpVtbl->Release(dataOutputStream);
        if (streamReader) IXmlReader_Release(streamReader);
        if (dataInputStream) dataInputStream->lpVtbl->Release(dataInputStream);
        return;

    Fallback:
        {
            PPH_STRING manifestString;

            if (manifestString = PhConvertUtf8ToUtf16Ex(manifestBuffer, manifestLength))
            {
                SendMessage(Context->EditWindow, WM_SETREDRAW, FALSE, 0);
                SendMessage(Context->EditWindow, WM_SETTEXT, FALSE, (LPARAM)manifestString->Buffer);
                SendMessage(Context->EditWindow, WM_SETREDRAW, TRUE, 0);
                PhDereferenceObject(manifestString);
            }
        }
        goto Cleanup;
    }
}

INT_PTR CALLBACK PvPeAppManifestDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PPV_APP_MANIFEST_CONTEXT context;

    if (uMsg == WM_INITDIALOG)
    {
        context = PhAllocateZero(sizeof(PV_APP_MANIFEST_CONTEXT));
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
            context->EditWindow = GetDlgItem(hwndDlg, IDC_PREVIEW);

            SendMessage(context->EditWindow, EM_SETLIMITTEXT, ULONG_MAX, 0);
            PvConfigTreeBorders(context->EditWindow);

            PhInitializeLayoutManager(&context->LayoutManager, hwndDlg);
            PhAddLayoutItem(&context->LayoutManager, context->EditWindow, NULL, PH_ANCHOR_ALL);

            PvpShowAppManifest(context);

            PhInitializeWindowTheme(hwndDlg, PhEnableThemeSupport);
        }
        break;
    case WM_DESTROY:
        {
            PhDeleteLayoutManager(&context->LayoutManager);

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
            LPNMHDR header = (LPNMHDR)lParam;

            switch (header->code)
            {
            case PSN_QUERYINITIALFOCUS:
                SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, (LONG_PTR)context->EditWindow);
                return TRUE;
            }
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
            return (INT_PTR)PhGetStockBrush(DC_BRUSH);
        }
        break;
    }

    return FALSE;
}
