/*
 * Process Hacker -
 *   about dialog
 *
 * Copyright (C) 2010-2011 wj32
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

#include <phapp.h>
#include <phappres.h>

static INT_PTR CALLBACK PhpAboutDlgProc(
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    )
{
    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            PPH_STRING appName;

#if (PHAPP_VERSION_REVISION != 0)
            appName = PhFormatString(
                L"Process Hacker %u.%u (r%u)",
                PHAPP_VERSION_MAJOR,
                PHAPP_VERSION_MINOR,
                PHAPP_VERSION_REVISION
                );
#else
            appName = PhFormatString(
                L"Process Hacker %u.%u",
                PHAPP_VERSION_MAJOR,
                PHAPP_VERSION_MINOR
                );
#endif

            SetDlgItemText(hwndDlg, IDC_ABOUT_NAME, appName->Buffer);
            PhDereferenceObject(appName);

            SetDlgItemText(hwndDlg, IDC_CREDITS,
                L"    Installer by XhmikosR\n"
                L"    Driver signed by the ReactOS Foundation\n"
                L"Thanks to:\n"
                L"    dmex\n"
                L"    Donators - thank you for your support!\n"
                L"    <a href=\"http://forum.sysinternals.com\">Sysinternals Forums</a>\n"
                L"    <a href=\"http://www.reactos.org\">ReactOS</a>\n"
                L"Process Hacker uses the following components:\n"
                L"    <a href=\"http://www.minixml.org\">Mini-XML</a> by Michael Sweet\n"
                L"    <a href=\"http://www.pcre.org\">PCRE</a>\n"
                L"    MD5 code by Jouni Malinen\n"
                L"    SHA1 code by Filip Navara, based on code by Steve Reid\n"
                L"    <a href=\"http://www.famfamfam.com/lab/icons/silk\">Silk icons</a>\n"
                );
        }
        break;
    case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
            case IDCANCEL:
            case IDOK:
                EndDialog(hwndDlg, IDOK);
                break;
            case IDC_DIAGNOSTICS:
                {
                    PPH_STRING diagnosticsString = PhGetDiagnosticsString();

                    PhShowInformationDialog(hwndDlg, diagnosticsString->Buffer);
                    PhDereferenceObject(diagnosticsString);
                }
                break;
            }
        }
        break;
    case WM_NOTIFY:
        {
            LPNMHDR header = (LPNMHDR)lParam;

            switch (header->code)
            {
            case NM_CLICK:
                {
                    switch (header->idFrom)
                    {
                    case IDC_CREDITS:
                    case IDC_LINK_SF:
                        PhShellExecute(hwndDlg, ((PNMLINK)header)->item.szUrl, NULL);
                        break;
                    }
                }
                break;
            }
        }
        break;
    }

    return FALSE;
}

VOID PhShowAboutDialog(
    __in HWND ParentWindowHandle
    )
{
    DialogBox(
        PhInstanceHandle,
        MAKEINTRESOURCE(IDD_ABOUT),
        ParentWindowHandle,
        PhpAboutDlgProc
        );
}

FORCEINLINE ULONG PhpGetObjectTypeObjectCount(
    __in PPH_OBJECT_TYPE ObjectType
    )
{
    PH_OBJECT_TYPE_INFORMATION info;

    PhGetObjectTypeInformation(ObjectType, &info);

    return info.NumberOfObjects;
}

PPH_STRING PhGetDiagnosticsString(
    VOID
    )
{
    PH_STRING_BUILDER stringBuilder;

    PhInitializeStringBuilder(&stringBuilder, 50);

    PhAppendFormatStringBuilder(&stringBuilder, L"OBJECT INFORMATION\r\n");

#define OBJECT_TYPE_COUNT(Type) PhAppendFormatStringBuilder(&stringBuilder, \
    L#Type L": %u objects\r\n", PhpGetObjectTypeObjectCount(Type))

    // ref
    OBJECT_TYPE_COUNT(PhObjectTypeObject);

    // basesup
    OBJECT_TYPE_COUNT(PhStringType);
    OBJECT_TYPE_COUNT(PhAnsiStringType);
    OBJECT_TYPE_COUNT(PhListType);
    OBJECT_TYPE_COUNT(PhPointerListType);
    OBJECT_TYPE_COUNT(PhHashtableType);
    OBJECT_TYPE_COUNT(PhFileStreamType);

    // ph
    OBJECT_TYPE_COUNT(PhSymbolProviderType);
    OBJECT_TYPE_COUNT(PhProcessItemType);
    OBJECT_TYPE_COUNT(PhServiceItemType);
    OBJECT_TYPE_COUNT(PhNetworkItemType);
    OBJECT_TYPE_COUNT(PhModuleProviderType);
    OBJECT_TYPE_COUNT(PhModuleItemType);
    OBJECT_TYPE_COUNT(PhThreadProviderType);
    OBJECT_TYPE_COUNT(PhThreadItemType);
    OBJECT_TYPE_COUNT(PhHandleProviderType);
    OBJECT_TYPE_COUNT(PhHandleItemType);
    OBJECT_TYPE_COUNT(PhMemoryItemType);

    return PhFinalStringBuilderString(&stringBuilder);
}
