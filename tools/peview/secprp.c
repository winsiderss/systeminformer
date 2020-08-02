/*
 * Process Hacker -
 *   PE viewer
 *
 * Copyright (C) 2020 dmex
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

#include <peview.h>
#include <cryptuiapi.h>

BOOLEAN PvpPeAddCertificateInfo(
    _In_ HWND ListViewHandle,
    _In_ INT ListViewItemIndex,
    _In_ PCCERT_CONTEXT CertificateContext
    )
{
    ULONG dataLength = 0;

    if (dataLength = CertGetNameString(
        CertificateContext,
        CERT_NAME_FRIENDLY_DISPLAY_TYPE,
        0,
        NULL,
        NULL,
        0
        ))
    {
        PWSTR data = PhAllocateZero(dataLength * sizeof(TCHAR));

        if (CertGetNameString(
            CertificateContext,
            CERT_NAME_FRIENDLY_DISPLAY_TYPE,
            0,
            NULL,
            data,
            dataLength
            ))
        {
            PhSetListViewSubItem(ListViewHandle, ListViewItemIndex, 1, data);
        }

        PhFree(data);
    }

    if (dataLength = CertGetNameString(
        CertificateContext,
        CERT_NAME_SIMPLE_DISPLAY_TYPE,
        CERT_NAME_ISSUER_FLAG,
        NULL,
        NULL,
        0
        ))
    {
        PWSTR data = PhAllocateZero(dataLength * sizeof(TCHAR));

        if (CertGetNameString(
            CertificateContext,
            CERT_NAME_SIMPLE_DISPLAY_TYPE,
            CERT_NAME_ISSUER_FLAG,
            NULL,
            data,
            dataLength
            ))
        {
            PhSetListViewSubItem(ListViewHandle, ListViewItemIndex, 2, data);
        }

        PhFree(data);
    }

    {
        LARGE_INTEGER fileTime;
        SYSTEMTIME systemTime;

        fileTime.QuadPart = 0;
        fileTime.LowPart = CertificateContext->pCertInfo->NotAfter.dwLowDateTime;
        fileTime.HighPart = CertificateContext->pCertInfo->NotAfter.dwHighDateTime;

        PhLargeIntegerToLocalSystemTime(&systemTime, &fileTime);
        PhSetListViewSubItem(ListViewHandle, ListViewItemIndex, 3, PhaFormatDateTime(&systemTime)->Buffer);

        //fileTime.QuadPart = 0;
        //fileTime.LowPart = CertificateContext->pCertInfo->NotBefore.dwLowDateTime;
        //fileTime.HighPart = CertificateContext->pCertInfo->NotBefore.dwHighDateTime;
        //PhSetListViewSubItem(ListViewHandle, ListViewItemIndex, 4, PvpGetRelativeTimeString(&fileTime)->Buffer);
    }

    {
        dataLength = 0;

        CertGetCertificateContextProperty(CertificateContext, CERT_HASH_PROP_ID, NULL, &dataLength);

        if (dataLength)
        {
            PUCHAR hash;
            PPH_STRING string;

            hash = PhAllocateZero(dataLength);
            CertGetCertificateContextProperty(CertificateContext, CERT_HASH_PROP_ID, hash, &dataLength);

            string = PhBufferToHexString(hash, dataLength);
            PhSetListViewSubItem(ListViewHandle, ListViewItemIndex, 4, string->Buffer);

            PhDereferenceObject(string);
            PhFree(hash);
        }
    }

    return TRUE;
}

PCMSG_SIGNER_INFO PvpPeGetSignerInfo(
    _In_ HCRYPTMSG CryptMessageHandle
    )
{
    ULONG signerInfoLength;
    PCMSG_SIGNER_INFO signerInfo;

    if (!CryptMsgGetParam(
        CryptMessageHandle,
        CMSG_SIGNER_INFO_PARAM,
        0,
        NULL,
        &signerInfoLength
        ))
    {
        return NULL;
    }

    signerInfo = PhAllocateZero(signerInfoLength);

    if (!CryptMsgGetParam(
        CryptMessageHandle,
        CMSG_SIGNER_INFO_PARAM,
        0,
        signerInfo,
        &signerInfoLength
        ))
    {
        PhFree(signerInfo);
        return NULL;
    }

    return signerInfo;
}

VOID PvpPeEnumerateNestedSignatures(
    _In_ HWND ListViewHandle,
    _In_ PULONG Count,
    _In_ PCMSG_SIGNER_INFO SignerInfo
    )
{
    HCERTSTORE cryptStoreHandle = NULL;
    HCRYPTMSG cryptMessageHandle = NULL;
    PCCERT_CONTEXT certificateContext = NULL;
    PCMSG_SIGNER_INFO cryptMessageSignerInfo = NULL;
    ULONG certificateEncoding;
    ULONG certificateContentType;
    ULONG certificateFormatType;
    ULONG index = ULONG_MAX;

    for (ULONG i = 0; i < SignerInfo->UnauthAttrs.cAttr; i++)
    {
        if (PhEqualBytesZ(SignerInfo->UnauthAttrs.rgAttr[i].pszObjId, szOID_NESTED_SIGNATURE, FALSE))
        {
            index = i;
            break;
        }
    }

    if (index == ULONG_MAX)
        return;

    if (CryptQueryObject(
        CERT_QUERY_OBJECT_BLOB,
        SignerInfo->UnauthAttrs.rgAttr[index].rgValue,
        CERT_QUERY_CONTENT_FLAG_PKCS7_SIGNED,
        CERT_QUERY_FORMAT_FLAG_BINARY,
        0,
        &certificateEncoding,
        &certificateContentType,
        &certificateFormatType,
        &cryptStoreHandle,
        &cryptMessageHandle,
        NULL
        ))
    {
        while (certificateContext = CertEnumCertificatesInStore(cryptStoreHandle, certificateContext))
        {
            INT lvItemIndex;
            WCHAR number[PH_INT32_STR_LEN_1];

            PhPrintUInt32(number, ++(*Count));
            lvItemIndex = PhAddListViewItem(
                ListViewHandle,
                MAXINT,
                number,
                (PVOID)certificateContext
                );

            PvpPeAddCertificateInfo(ListViewHandle, lvItemIndex, certificateContext);
        }

        if (cryptMessageSignerInfo = PvpPeGetSignerInfo(cryptMessageHandle))
        {
            PvpPeEnumerateNestedSignatures(ListViewHandle, Count, cryptMessageSignerInfo);

            PhFree(cryptMessageSignerInfo);
        }

        //if (certificateContext) CertFreeCertificateContext(certificateContext);
        //if (cryptStoreHandle) CertCloseStore(cryptStoreHandle, 0);
        //if (cryptMessageHandle) CryptMsgClose(cryptMessageHandle);
    }
}

VOID PvpPeEnumerateFileCertificates(
    _In_ HWND ListViewHandle
    )
{
    HCERTSTORE cryptStoreHandle = NULL;
    PCCERT_CONTEXT certificateContext = NULL;
    HCRYPTMSG cryptMessageHandle = NULL;
    PCMSG_SIGNER_INFO cryptMessageSignerInfo = NULL;
    ULONG certificateEncoding;
    ULONG certificateContentType;
    ULONG certificateFormatType;
    ULONG count = 0;

    if (CryptQueryObject(
        CERT_QUERY_OBJECT_FILE,
        PhGetString(PvFileName),
        CERT_QUERY_CONTENT_FLAG_PKCS7_SIGNED_EMBED,
        CERT_QUERY_FORMAT_FLAG_BINARY,
        0,
        &certificateEncoding,
        &certificateContentType,
        &certificateFormatType,
        &cryptStoreHandle,
        &cryptMessageHandle,
        NULL
        ))
    {
        while (certificateContext = CertEnumCertificatesInStore(cryptStoreHandle, certificateContext))
        {
            INT lvItemIndex;
            WCHAR number[PH_INT32_STR_LEN_1];

            PhPrintUInt32(number, ++count);
            lvItemIndex = PhAddListViewItem(
                ListViewHandle,
                MAXINT,
                number,
                (PVOID)certificateContext
                );

            PvpPeAddCertificateInfo(ListViewHandle, lvItemIndex, certificateContext);
        }

        if (cryptMessageSignerInfo = PvpPeGetSignerInfo(cryptMessageHandle))
        {
            PvpPeEnumerateNestedSignatures(ListViewHandle, &count, cryptMessageSignerInfo);

            PhFree(cryptMessageSignerInfo);
        }

        //if (certificateContext) CertFreeCertificateContext(certificateContext);
        //if (cryptStoreHandle) CertCloseStore(cryptStoreHandle, 0);
        //if (cryptMessageHandle) CryptMsgClose(cryptMessageHandle);
    }
}

VOID PvpPeViewCertificateContext(
    _In_ HWND WindowHandle,
    _In_ PCCERT_CONTEXT CertContext
    )
{
    CRYPTUI_VIEWCERTIFICATE_STRUCT cryptViewCertInfo;

    memset(&cryptViewCertInfo, 0, sizeof(CRYPTUI_VIEWCERTIFICATE_STRUCT));
    cryptViewCertInfo.dwSize = sizeof(CRYPTUI_VIEWCERTIFICATE_STRUCT);
    cryptViewCertInfo.dwFlags = CRYPTUI_ENABLE_REVOCATION_CHECKING | CRYPTUI_ENABLE_REVOCATION_CHECK_END_CERT;
    cryptViewCertInfo.hwndParent = WindowHandle;
    cryptViewCertInfo.pCertContext = CertContext;

    HCERTSTORE certStoreHandle = CertContext->hCertStore;
    cryptViewCertInfo.cStores = 1;
    cryptViewCertInfo.rghStores = &certStoreHandle;

    if (CryptUIDlgViewCertificate)
    {
        CryptUIDlgViewCertificate(&cryptViewCertInfo, NULL);
    }
}

typedef struct _PV_PE_CERTIFICATE_CONTEXT
{
    HWND WindowHandle;
    HWND ListViewHandle;
    HIMAGELIST ListViewImageList;
} PV_PE_CERTIFICATE_CONTEXT, *PPV_PE_CERTIFICATE_CONTEXT;

INT_PTR CALLBACK PvpPeSecurityDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    LPPROPSHEETPAGE propSheetPage;
    PPV_PROPPAGECONTEXT propPageContext;
    PPV_PE_CERTIFICATE_CONTEXT context;

    if (!PvPropPageDlgProcHeader(hwndDlg, uMsg, lParam, &propSheetPage, &propPageContext))
        return FALSE;

    if (uMsg == WM_INITDIALOG)
    {
        context = propPageContext->Context = PhAllocate(sizeof(PV_PE_CERTIFICATE_CONTEXT));
        memset(context, 0, sizeof(PV_PE_CERTIFICATE_CONTEXT));
    }
    else
    {
        context = propPageContext->Context;
    }

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            context->WindowHandle = hwndDlg;
            context->ListViewHandle = GetDlgItem(hwndDlg, IDC_LIST);
            
            PhSetListViewStyle(context->ListViewHandle, TRUE, TRUE);
            PhSetControlTheme(context->ListViewHandle, L"explorer");
            PhAddListViewColumn(context->ListViewHandle, 0, 0, 0, LVCFMT_LEFT, 40, L"#");
            PhAddListViewColumn(context->ListViewHandle, 1, 1, 1, LVCFMT_LEFT, 100, L"Issued to");
            PhAddListViewColumn(context->ListViewHandle, 2, 2, 2, LVCFMT_LEFT, 100, L"Issued by");
            PhAddListViewColumn(context->ListViewHandle, 3, 3, 3, LVCFMT_LEFT, 100, L"Expires");
            PhAddListViewColumn(context->ListViewHandle, 4, 4, 4, LVCFMT_LEFT, 100, L"Thumbprint");
            PhSetExtendedListView(context->ListViewHandle);
            PhLoadListViewColumnsFromSetting(L"ImageSecurityListViewColumns", context->ListViewHandle);
            PhLoadListViewSortColumnsFromSetting(L"ImageSecurityListViewSort", context->ListViewHandle);

            if (context->ListViewImageList = ImageList_Create(2, 20, ILC_COLOR, 1, 1))
                ListView_SetImageList(context->ListViewHandle, context->ListViewImageList, LVSIL_SMALL);

            PvpPeEnumerateFileCertificates(context->ListViewHandle);
        }
        break;
    case WM_DESTROY:
        {
            PhSaveListViewSortColumnsToSetting(L"ImageSecurityListViewSort", context->ListViewHandle);
            PhSaveListViewColumnsToSetting(L"ImageSecurityListViewColumns", context->ListViewHandle);

            if (context->ListViewImageList)
                ImageList_Destroy(context->ListViewImageList);

            PhFree(context);
        }
        break;
    case WM_SHOWWINDOW:
        {
            if (!propPageContext->LayoutInitialized)
            {
                PPH_LAYOUT_ITEM dialogItem;

                dialogItem = PvAddPropPageLayoutItem(hwndDlg, hwndDlg, PH_PROP_PAGE_TAB_CONTROL_PARENT, PH_ANCHOR_ALL);
                PvAddPropPageLayoutItem(hwndDlg, context->ListViewHandle, dialogItem, PH_ANCHOR_ALL);

                PvDoPropPageLayout(hwndDlg);

                propPageContext->LayoutInitialized = TRUE;
            }
        }
        break;
    case WM_NOTIFY:
        {
            LPNMHDR header = (LPNMHDR)lParam;

            PvHandleListViewNotifyForCopy(lParam, context->ListViewHandle);

            switch (header->code)
            {
            case NM_DBLCLK:
                {
                    if (header->hwndFrom == context->ListViewHandle)
                    {
                        PVOID* listviewItems;
                        ULONG numberOfItems;

                        PhGetSelectedListViewItemParams(context->ListViewHandle, &listviewItems, &numberOfItems);

                        if (numberOfItems != 0)
                        {
                            PvpPeViewCertificateContext(hwndDlg, listviewItems[0]);
                        }

                        PhFree(listviewItems);
                    }
                }
                break;
            }
        }
        break;
    case WM_CONTEXTMENU:
        {
            if ((HWND)wParam == context->ListViewHandle)
            {
                POINT point;
                PPH_EMENU menu;
                PPH_EMENU item;
                PVOID* listviewItems;
                ULONG numberOfItems;

                point.x = GET_X_LPARAM(lParam);
                point.y = GET_Y_LPARAM(lParam);

                if (point.x == -1 && point.y == -1)
                    PvGetListViewContextMenuPoint((HWND)wParam, &point);

                PhGetSelectedListViewItemParams((HWND)wParam, &listviewItems, &numberOfItems);

                if (numberOfItems != 0)
                {
                    menu = PhCreateEMenu();
                    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, 1, L"View certificate", NULL, NULL), ULONG_MAX);
                    PhInsertEMenuItem(menu, PhCreateEMenuSeparator(), ULONG_MAX);
                    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, USHRT_MAX, L"&Copy", NULL, NULL), ULONG_MAX);
                    PvInsertCopyListViewEMenuItem(menu, USHRT_MAX, (HWND)wParam);

                    item = PhShowEMenu(
                        menu,
                        hwndDlg,
                        PH_EMENU_SHOW_SEND_COMMAND | PH_EMENU_SHOW_LEFTRIGHT,
                        PH_ALIGN_LEFT | PH_ALIGN_TOP,
                        point.x,
                        point.y
                        );

                    if (item)
                    {
                        if (!PvHandleCopyListViewEMenuItem(item))
                        {
                            switch (item->Id)
                            {
                            case 1:
                                PvpPeViewCertificateContext(hwndDlg, listviewItems[0]);                   
                                break;
                            case USHRT_MAX:
                                PvCopyListView((HWND)wParam);
                                break;
                            }
                        }
                    }

                    PhDestroyEMenu(menu);
                }

                PhFree(listviewItems);
            }
        }
        break;
    }

    return FALSE;
}
