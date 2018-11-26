/*
 * Process Hacker -
 *   Process properties: General page
 *
 * Copyright (C) 2009-2016 wj32
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
#include <procprp.h>
#include <procprpp.h>

#include <secedit.h>
#include <verify.h>

#include <actions.h>
#include <mainwnd.h>
#include <phsettings.h>
#include <procmtgn.h>
#include <procprv.h>
#include <settings.h>

static PWSTR ProtectedSignerStrings[] =
    { L"", L" (Authenticode)", L" (CodeGen)", L" (Antimalware)", L" (Lsa)", L" (Windows)", L" (WinTcb)", L" (WinSystem)", L" (StoreApp)" };

PPH_STRING PhGetProcessItemProtectionText(
    _In_ PPH_PROCESS_ITEM ProcessItem
    )
{
    if (ProcessItem->Protection.Level != UCHAR_MAX)
    {
        if (WindowsVersion >= WINDOWS_8_1)
        {
            PWSTR type;
            PWSTR signer;

            switch (ProcessItem->Protection.Type)
            {
            case PsProtectedTypeNone:
                type = L"None";
                break;
            case PsProtectedTypeProtectedLight:
                type = L"Light";
                break;
            case PsProtectedTypeProtected:
                type = L"Full";
                break;
            default:
                type = L"Unknown";
                break;
            }

            if (ProcessItem->Protection.Signer < sizeof(ProtectedSignerStrings) / sizeof(PWSTR))
                signer = ProtectedSignerStrings[ProcessItem->Protection.Signer];
            else
                signer = L"";

            return PhConcatStrings2(type, signer);
        }
        else
        {
            return PhCreateString(ProcessItem->IsProtectedProcess ? L"Yes" : L"None");
        }
    }

    return PhCreateString(L"N/A");
}

NTSTATUS PhpProcessGeneralOpenProcess(
    _Out_ PHANDLE Handle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_opt_ PVOID Context
    )
{
    return PhOpenProcess(Handle, DesiredAccess, (HANDLE)Context);
}

FORCEINLINE PWSTR PhpGetStringOrNa(
    _In_ PPH_STRING String
    )
{
    if (String)
        return String->Buffer;
    else
        return L"N/A";
}

VOID PhpUpdateProcessMitigationPolicies(
    _In_ HWND hwndDlg,
    _In_ PPH_PROCESS_ITEM ProcessItem
    )
{
    NTSTATUS status;
    HANDLE processHandle;
    PH_PROCESS_MITIGATION_POLICY_ALL_INFORMATION information;

    PhSetDialogItemText(hwndDlg, IDC_MITIGATION, L"N/A");

    if (NT_SUCCESS(status = PhOpenProcess(
        &processHandle,
        PROCESS_QUERY_INFORMATION,
        ProcessItem->ProcessId
        )))
    {
        if (NT_SUCCESS(status = PhGetProcessMitigationPolicy(processHandle, &information)))
        {
            PH_STRING_BUILDER sb;
            PROCESS_MITIGATION_POLICY policy;
            PPH_STRING shortDescription;

            PhInitializeStringBuilder(&sb, 100);

            for (policy = 0; policy < MaxProcessMitigationPolicy; policy++)
            {
                if (information.Pointers[policy] && PhDescribeProcessMitigationPolicy(
                    policy,
                    information.Pointers[policy],
                    &shortDescription,
                    NULL
                    ))
                {
                    PhAppendStringBuilder(&sb, &shortDescription->sr);
                    PhAppendStringBuilder2(&sb, L"; ");
                    PhDereferenceObject(shortDescription);
                }
            }

            if (sb.String->Length != 0)
            {
                PhRemoveEndStringBuilder(&sb, 2);
                PhSetDialogItemText(hwndDlg, IDC_MITIGATION, sb.String->Buffer);
            }
            else
            {
                PhSetDialogItemText(hwndDlg, IDC_MITIGATION, L"None");
            }

            PhDeleteStringBuilder(&sb);
        }

        NtClose(processHandle);
    }

    if (!NT_SUCCESS(status))
        EnableWindow(GetDlgItem(hwndDlg, IDC_VIEWMITIGATION), FALSE);
}

INT_PTR CALLBACK PhpProcessGeneralDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    LPPROPSHEETPAGE propSheetPage;
    PPH_PROCESS_PROPPAGECONTEXT propPageContext;
    PPH_PROCESS_ITEM processItem;

    if (!PhPropPageDlgProcHeader(hwndDlg, uMsg, lParam, &propSheetPage, &propPageContext, &processItem))
        return FALSE;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            HANDLE processHandle = NULL;
            PPH_STRING curDir = NULL;
            PROCESS_BASIC_INFORMATION basicInfo;
#ifdef _WIN64
            PVOID peb32;
#endif
            PPH_PROCESS_ITEM parentProcess;
            CLIENT_ID clientId;
            HICON folder;
            HICON magnifier;

            folder = PH_LOAD_SHARED_ICON_SMALL(PhInstanceHandle, MAKEINTRESOURCE(IDI_FOLDER));
            magnifier = PH_LOAD_SHARED_ICON_SMALL(PhInstanceHandle, MAKEINTRESOURCE(IDI_MAGNIFIER));

            SET_BUTTON_ICON(IDC_INSPECT, magnifier);
            SET_BUTTON_ICON(IDC_OPENFILENAME, folder);
            SET_BUTTON_ICON(IDC_VIEWCOMMANDLINE, magnifier);
            SET_BUTTON_ICON(IDC_VIEWPARENTPROCESS, magnifier);

            // File

            if (processItem->LargeIcon)
            {
                Static_SetIcon(GetDlgItem(hwndDlg, IDC_FILEICON), processItem->LargeIcon);
            }
            else
            {
                HICON iconLarge;

                PhGetStockApplicationIcon(NULL, &iconLarge);
                Static_SetIcon(GetDlgItem(hwndDlg, IDC_FILEICON), iconLarge);
            }

            if (PH_IS_REAL_PROCESS_ID(processItem->ProcessId))
            {
                PhSetDialogItemText(hwndDlg, IDC_NAME, PhpGetStringOrNa(processItem->VersionInfo.FileDescription));
            }
            else
            {
                PhSetDialogItemText(hwndDlg, IDC_NAME, processItem->ProcessName->Buffer);
            }

            PhSetDialogItemText(hwndDlg, IDC_VERSION, PhpGetStringOrNa(processItem->VersionInfo.FileVersion));
            PhSetDialogItemText(hwndDlg, IDC_FILENAME, PhpGetStringOrNa(processItem->FileName));

            {
                PPH_STRING inspectExecutables;

                if (PhIsNullOrEmptyString(processItem->FileName))
                {
                    EnableWindow(GetDlgItem(hwndDlg, IDC_OPENFILENAME), FALSE);
                    EnableWindow(GetDlgItem(hwndDlg, IDC_INSPECT), FALSE);
                }
                else
                {
                    if (!RtlDoesFileExists_U(PhGetString(processItem->FileName)))
                    {
                        EnableWindow(GetDlgItem(hwndDlg, IDC_OPENFILENAME), FALSE);
                        EnableWindow(GetDlgItem(hwndDlg, IDC_INSPECT), FALSE);
                    }
                }

                inspectExecutables = PhaGetStringSetting(L"ProgramInspectExecutables");

                if (PhIsNullOrEmptyString(inspectExecutables))
                {
                    EnableWindow(GetDlgItem(hwndDlg, IDC_INSPECT), FALSE);
                }
            }

            if (processItem->VerifyResult == VrTrusted)
            {
                if (processItem->VerifySignerName)
                {
                    PhSetDialogItemText(hwndDlg, IDC_COMPANYNAME_LINK,
                        PhaFormatString(L"<a>(Verified) %s</a>", processItem->VerifySignerName->Buffer)->Buffer);
                    ShowWindow(GetDlgItem(hwndDlg, IDC_COMPANYNAME), SW_HIDE);
                    ShowWindow(GetDlgItem(hwndDlg, IDC_COMPANYNAME_LINK), SW_SHOW);
                }
                else
                {
                    PhSetDialogItemText(hwndDlg, IDC_COMPANYNAME,
                        PhaConcatStrings2(
                        L"(Verified) ",
                        PhGetStringOrEmpty(processItem->VersionInfo.CompanyName)
                        )->Buffer);
                }
            }
            else if (processItem->VerifyResult != VrUnknown)
            {
                PhSetDialogItemText(hwndDlg, IDC_COMPANYNAME,
                    PhaConcatStrings2(
                    L"(UNVERIFIED) ",
                    PhGetStringOrEmpty(processItem->VersionInfo.CompanyName)
                    )->Buffer);
            }
            else
            {
                PhSetDialogItemText(hwndDlg, IDC_COMPANYNAME,
                    PhpGetStringOrNa(processItem->VersionInfo.CompanyName));
            }

            // Command Line

            PhSetDialogItemText(hwndDlg, IDC_CMDLINE, PhpGetStringOrNa(processItem->CommandLine));

            if (!processItem->CommandLine)
                EnableWindow(GetDlgItem(hwndDlg, IDC_VIEWCOMMANDLINE), FALSE);

            // Current Directory

            if (NT_SUCCESS(PhOpenProcess(
                &processHandle,
                PROCESS_QUERY_LIMITED_INFORMATION | PROCESS_VM_READ,
                processItem->ProcessId
                )))
            {
                PH_PEB_OFFSET pebOffset;

                pebOffset = PhpoCurrentDirectory;

#ifdef _WIN64
                // Tell the function to get the WOW64 current directory, because that's the one that
                // actually gets updated.
                if (processItem->IsWow64)
                    pebOffset |= PhpoWow64;
#endif

                PhGetProcessPebString(
                    processHandle,
                    pebOffset,
                    &curDir
                    );
                PH_AUTO(curDir);

                NtClose(processHandle);
                processHandle = NULL;
            }

            PhSetDialogItemText(hwndDlg, IDC_CURDIR, PhpGetStringOrNa(curDir));

            // Started

            if (processItem->CreateTime.QuadPart != 0)
            {
                LARGE_INTEGER startTime;
                LARGE_INTEGER currentTime;
                SYSTEMTIME startTimeFields;
                PPH_STRING startTimeRelativeString;
                PPH_STRING startTimeString;

                startTime = processItem->CreateTime;
                PhQuerySystemTime(&currentTime);
                startTimeRelativeString = PH_AUTO(PhFormatTimeSpanRelative(currentTime.QuadPart - startTime.QuadPart));

                PhLargeIntegerToLocalSystemTime(&startTimeFields, &startTime);
                startTimeString = PhaFormatDateTime(&startTimeFields);

                PhSetDialogItemText(hwndDlg, IDC_STARTED,
                    PhaFormatString(L"%s ago (%s)", startTimeRelativeString->Buffer, startTimeString->Buffer)->Buffer);
            }
            else
            {
                PhSetDialogItemText(hwndDlg, IDC_STARTED, L"N/A");
            }

            // Parent

            if (parentProcess = PhReferenceProcessItemForParent(processItem))
            {
                clientId.UniqueProcess = parentProcess->ProcessId;
                clientId.UniqueThread = NULL;

                PhSetDialogItemText(hwndDlg, IDC_PARENTPROCESS,
                    PH_AUTO_T(PH_STRING, PhGetClientIdNameEx(&clientId, parentProcess->ProcessName))->Buffer);

                PhDereferenceObject(parentProcess);
            }
            else
            {
                PhSetDialogItemText(hwndDlg, IDC_PARENTPROCESS,
                    PhaFormatString(L"Non-existent process (%u)", HandleToUlong(processItem->ParentProcessId))->Buffer);
                EnableWindow(GetDlgItem(hwndDlg, IDC_VIEWPARENTPROCESS), FALSE);
            }

            // Mitigation policies

            PhpUpdateProcessMitigationPolicies(hwndDlg, processItem);

            // PEB address

            PhSetDialogItemText(hwndDlg, IDC_PEBADDRESS, L"N/A");

            if (NT_SUCCESS(PhOpenProcess(
                &processHandle,
                PROCESS_QUERY_LIMITED_INFORMATION,
                processItem->ProcessId
                )))
            {
                PhGetProcessBasicInformation(processHandle, &basicInfo);
#ifdef _WIN64
                if (processItem->IsWow64)
                {
                    PhGetProcessPeb32(processHandle, &peb32);
                    PhSetDialogItemText(hwndDlg, IDC_PEBADDRESS,
                        PhaFormatString(L"0x%Ix (32-bit: 0x%x)", basicInfo.PebBaseAddress, PtrToUlong(peb32))->Buffer);
                }
                else
                {
#endif

                PhSetDialogItemText(hwndDlg, IDC_PEBADDRESS,
                    PhaFormatString(L"0x%Ix", basicInfo.PebBaseAddress)->Buffer);
#ifdef _WIN64
                }
#endif
                NtClose(processHandle);
                processHandle = NULL;
            }

            // Protection

            PhSetDialogItemText(hwndDlg, IDC_PROTECTION, PH_AUTO_T(PH_STRING, PhGetProcessItemProtectionText(processItem))->Buffer);

#ifdef _WIN64
            if (processItem->IsWow64Valid)
            {
                if (processItem->IsWow64)
                    PhSetDialogItemText(hwndDlg, IDC_PROCESSTYPETEXT, L"32-bit");
                else
                    PhSetDialogItemText(hwndDlg, IDC_PROCESSTYPETEXT, L"64-bit");
            }
            else
            {
                PhSetDialogItemText(hwndDlg, IDC_PROCESSTYPETEXT, L"N/A");
            }

            ShowWindow(GetDlgItem(hwndDlg, IDC_PROCESSTYPELABEL), SW_SHOW);
            ShowWindow(GetDlgItem(hwndDlg, IDC_PROCESSTYPETEXT), SW_SHOW);
#endif
            PhInitializeWindowTheme(hwndDlg, PhEnableThemeSupport);
        }
        break;
    case WM_SHOWWINDOW:
        {
            PPH_LAYOUT_ITEM dialogItem;

            if (dialogItem = PhBeginPropPageLayout(hwndDlg, propPageContext))
            {
                PhAddPropPageLayoutItem(hwndDlg, GetDlgItem(hwndDlg, IDC_FILE), dialogItem, PH_ANCHOR_LEFT | PH_ANCHOR_TOP | PH_ANCHOR_RIGHT);
                PhAddPropPageLayoutItem(hwndDlg, GetDlgItem(hwndDlg, IDC_NAME), dialogItem, PH_ANCHOR_LEFT | PH_ANCHOR_TOP | PH_ANCHOR_RIGHT);
                PhAddPropPageLayoutItem(hwndDlg, GetDlgItem(hwndDlg, IDC_COMPANYNAME), dialogItem, PH_ANCHOR_LEFT | PH_ANCHOR_TOP | PH_ANCHOR_RIGHT);
                PhAddPropPageLayoutItem(hwndDlg, GetDlgItem(hwndDlg, IDC_COMPANYNAME_LINK), dialogItem, PH_ANCHOR_LEFT | PH_ANCHOR_TOP | PH_ANCHOR_RIGHT);
                PhAddPropPageLayoutItem(hwndDlg, GetDlgItem(hwndDlg, IDC_VERSION), dialogItem, PH_ANCHOR_LEFT | PH_ANCHOR_TOP | PH_ANCHOR_RIGHT);
                PhAddPropPageLayoutItem(hwndDlg, GetDlgItem(hwndDlg, IDC_PROCESSTYPELABEL), dialogItem, PH_ANCHOR_RIGHT | PH_ANCHOR_TOP);
                PhAddPropPageLayoutItem(hwndDlg, GetDlgItem(hwndDlg, IDC_PROCESSTYPETEXT), dialogItem, PH_ANCHOR_RIGHT | PH_ANCHOR_TOP);
                PhAddPropPageLayoutItem(hwndDlg, GetDlgItem(hwndDlg, IDC_FILENAME), dialogItem, PH_ANCHOR_LEFT | PH_ANCHOR_TOP | PH_ANCHOR_RIGHT);
                PhAddPropPageLayoutItem(hwndDlg, GetDlgItem(hwndDlg, IDC_INSPECT), dialogItem, PH_ANCHOR_TOP | PH_ANCHOR_RIGHT);
                PhAddPropPageLayoutItem(hwndDlg, GetDlgItem(hwndDlg, IDC_OPENFILENAME), dialogItem, PH_ANCHOR_TOP | PH_ANCHOR_RIGHT);
                PhAddPropPageLayoutItem(hwndDlg, GetDlgItem(hwndDlg, IDC_CMDLINE), dialogItem, PH_ANCHOR_LEFT | PH_ANCHOR_TOP | PH_ANCHOR_RIGHT);
                PhAddPropPageLayoutItem(hwndDlg, GetDlgItem(hwndDlg, IDC_VIEWCOMMANDLINE), dialogItem, PH_ANCHOR_TOP | PH_ANCHOR_RIGHT);
                PhAddPropPageLayoutItem(hwndDlg, GetDlgItem(hwndDlg, IDC_CURDIR), dialogItem, PH_ANCHOR_LEFT | PH_ANCHOR_TOP | PH_ANCHOR_RIGHT);
                PhAddPropPageLayoutItem(hwndDlg, GetDlgItem(hwndDlg, IDC_STARTED), dialogItem, PH_ANCHOR_LEFT | PH_ANCHOR_TOP | PH_ANCHOR_RIGHT);
                PhAddPropPageLayoutItem(hwndDlg, GetDlgItem(hwndDlg, IDC_PEBADDRESS), dialogItem, PH_ANCHOR_LEFT | PH_ANCHOR_TOP | PH_ANCHOR_RIGHT);
                PhAddPropPageLayoutItem(hwndDlg, GetDlgItem(hwndDlg, IDC_PARENTPROCESS), dialogItem, PH_ANCHOR_LEFT | PH_ANCHOR_TOP | PH_ANCHOR_RIGHT);
                PhAddPropPageLayoutItem(hwndDlg, GetDlgItem(hwndDlg, IDC_VIEWPARENTPROCESS), dialogItem, PH_ANCHOR_TOP | PH_ANCHOR_RIGHT);
                PhAddPropPageLayoutItem(hwndDlg, GetDlgItem(hwndDlg, IDC_MITIGATION), dialogItem, PH_ANCHOR_LEFT | PH_ANCHOR_TOP | PH_ANCHOR_RIGHT);
                PhAddPropPageLayoutItem(hwndDlg, GetDlgItem(hwndDlg, IDC_VIEWMITIGATION), dialogItem, PH_ANCHOR_TOP | PH_ANCHOR_RIGHT);
                PhAddPropPageLayoutItem(hwndDlg, GetDlgItem(hwndDlg, IDC_TERMINATE), dialogItem, PH_ANCHOR_RIGHT | PH_ANCHOR_TOP);
                PhAddPropPageLayoutItem(hwndDlg, GetDlgItem(hwndDlg, IDC_PERMISSIONS), dialogItem, PH_ANCHOR_RIGHT | PH_ANCHOR_TOP);
                PhAddPropPageLayoutItem(hwndDlg, GetDlgItem(hwndDlg, IDC_PROCESS), dialogItem, PH_ANCHOR_ALL);
                PhEndPropPageLayout(hwndDlg, propPageContext);
            }
        }
        break;
    case WM_COMMAND:
        {
            switch (GET_WM_COMMAND_ID(wParam, lParam))
            {
            case IDC_INSPECT:
                {
                    if (processItem->FileName)
                    {
                        PhShellExecuteUserString(
                            hwndDlg,
                            L"ProgramInspectExecutables",
                            processItem->FileName->Buffer,
                            FALSE,
                            L"Make sure the PE Viewer executable file is present."
                            );
                    }
                }
                break;
            case IDC_OPENFILENAME:
                {
                    if (processItem->FileName)
                    {
                        PhShellExecuteUserString(
                            hwndDlg,
                            L"FileBrowseExecutable",
                            processItem->FileName->Buffer,
                            FALSE,
                            L"Make sure the Explorer executable file is present."
                            );
                    }
                }
                break;
            case IDC_VIEWCOMMANDLINE:
                {
                    if (processItem->CommandLine)
                        PhShowInformationDialog(hwndDlg, processItem->CommandLine->Buffer, 0);
                }
                break;
            case IDC_VIEWPARENTPROCESS:
                {
                    PPH_PROCESS_ITEM parentProcessItem;

                    if (parentProcessItem = PhReferenceProcessItem(processItem->ParentProcessId))
                    {
                        ProcessHacker_ShowProcessProperties(PhMainWndHandle, parentProcessItem);
                        PhDereferenceObject(parentProcessItem);
                    }
                    else
                    {
                        PhShowError(hwndDlg, L"The process does not exist.");
                    }
                }
                break;
            case IDC_VIEWMITIGATION:
                {
                    PhShowProcessMitigationPolicyDialog(hwndDlg, processItem->ProcessId);
                }
                break;
            case IDC_TERMINATE:
                {
                    PhUiTerminateProcesses(
                        hwndDlg,
                        &processItem,
                        1
                        );
                }
                break;
            case IDC_PERMISSIONS:
                {
                    PhEditSecurity(
                        PhCsForceNoParent ? NULL : hwndDlg,
                        PhGetStringOrEmpty(processItem->ProcessName),
                        L"Process",
                        PhpProcessGeneralOpenProcess,
                        NULL,
                        processItem->ProcessId
                        );
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
                    case IDC_COMPANYNAME_LINK:
                        {
                            if (processItem->FileName)
                            {
                                PH_VERIFY_FILE_INFO info;

                                memset(&info, 0, sizeof(PH_VERIFY_FILE_INFO));
                                info.FileName = processItem->FileName->Buffer;
                                info.Flags = PH_VERIFY_VIEW_PROPERTIES;
                                info.hWnd = hwndDlg;
                                PhVerifyFileWithAdditionalCatalog(
                                    &info,
                                    processItem->PackageFullName,
                                    NULL
                                    );
                            }
                        }
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
