/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2009-2016
 *     dmex    2016-2023
 *
 */

#include <phapp.h>
#include <procprp.h>
#include <procprpp.h>

#include <mapimg.h>
#include <secedit.h>
#include <verify.h>

#include <actions.h>
#include <mainwnd.h>
#include <phsettings.h>
#include <procmtgn.h>
#include <procprv.h>
#include <settings.h>

#include <shellapi.h>

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

            // Isolated User Mode (IUM) (dmex)
            if (ProcessItem->Protection.Type == PsProtectedTypeNone && ProcessItem->IsSecureProcess)
                return PhConcatStrings2(L"Secure (IUM)", signer);

            return PhConcatStrings2(type, signer);
        }
        else
        {
            if (ProcessItem->IsSecureProcess)
                return PhCreateString(L"Secure (IUM)");

            if (ProcessItem->IsProtectedProcess)
                return PhCreateString(L"Yes");

            return PhCreateString(L"None");
        }
    }

    return PhCreateString(L"N/A");
}

PPH_STRING PhGetProcessItemImageTypeText(
    _In_ PPH_PROCESS_ITEM ProcessItem
    )
{
    USHORT architecture = IMAGE_FILE_MACHINE_UNKNOWN;
    PWSTR arch = L"";
    PWSTR bits = L"";

    {
        USHORT processArchitecture;

        if (
            WindowsVersion >= WINDOWS_11 && ProcessItem->QueryHandle && 
            NT_SUCCESS(PhGetProcessArchitecture(ProcessItem->QueryHandle, &processArchitecture))
            )
        {
            architecture = processArchitecture;
        }
        else if (ProcessItem->FileName)
        {
            PH_MAPPED_IMAGE mappedImage;

            if (NT_SUCCESS(PhLoadMappedImageHeaderPageSize(&ProcessItem->FileName->sr, NULL, &mappedImage)))
            {
                architecture = mappedImage.NtHeaders->FileHeader.Machine;
                PhUnloadMappedImage(&mappedImage);
            }
        }
    }

    switch (architecture)
    {
    case IMAGE_FILE_MACHINE_I386:
        arch = L"I386 ";
        break;
    case IMAGE_FILE_MACHINE_AMD64:
        arch = L"AMD64 ";
        break;
    case IMAGE_FILE_MACHINE_ARMNT:
        arch = L"ARM ";
        break;
    case IMAGE_FILE_MACHINE_ARM64:
        arch = L"ARM64 ";
        break;
    }

#if _WIN64
    bits = ProcessItem->IsWow64 ? L"(32-bit)" : L"(64-bit)";
#else
    bits = L"(32-bit)";
#endif

    if (arch[0] == UNICODE_NULL && bits[0] == UNICODE_NULL)
        return PhCreateString(L"N/A");

    return PhConcatStrings2(arch, bits);
}

NTSTATUS PhpProcessGeneralOpenProcess(
    _Out_ PHANDLE Handle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_opt_ PVOID Context
    )
{
    if (Context)
        return PhOpenProcess(Handle, DesiredAccess, (HANDLE)Context);
    return STATUS_UNSUCCESSFUL;
}

FORCEINLINE PWSTR PhpGetStringOrNa(
    _In_opt_ _Maybenull_ PPH_STRING String
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

            // HACK: Show System process CET mitigation (dmex)
            if (ProcessItem->ProcessId == SYSTEM_PROCESS_ID)
            {
                SYSTEM_SHADOW_STACK_INFORMATION shadowStackInformation;

                if (NT_SUCCESS(PhGetSystemShadowStackInformation(&shadowStackInformation)))
                {
                    PROCESS_MITIGATION_USER_SHADOW_STACK_POLICY policyData;

                    memset(&policyData, 0, sizeof(PROCESS_MITIGATION_USER_SHADOW_STACK_POLICY));
                    policyData.EnableUserShadowStack = shadowStackInformation.KernelCetEnabled;
                    policyData.EnableUserShadowStackStrictMode = shadowStackInformation.KernelCetEnabled;
                    policyData.AuditUserShadowStack = shadowStackInformation.KernelCetAuditModeEnabled;

                    if (PhDescribeProcessMitigationPolicy(
                        ProcessUserShadowStackPolicy,
                        &policyData,
                        &shortDescription,
                        NULL
                        ))
                    {
                        PhAppendStringBuilder(&sb, &shortDescription->sr);
                        PhAppendStringBuilder2(&sb, L"; ");
                        PhDereferenceObject(shortDescription);
                    }
                }
            }

            if (sb.String->Length != 0)
            {
                PhRemoveEndStringBuilder(&sb, 2);
                PhSetDialogItemText(hwndDlg, IDC_MITIGATION, PhFinalStringBuilderString(&sb)->Buffer);
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

typedef struct _PH_PROCGENERAL_CONTEXT
{
    HWND WindowHandle;
    HWND StartedLabelHandle;
    BOOLEAN Enabled;
    HICON ProgramIcon;
} PH_PROCGENERAL_CONTEXT, *PPH_PROCGENERAL_CONTEXT;

VOID PphProcessGeneralDlgUpdateIcons(
    _In_ HWND hwndDlg
    )
{
    HICON folder;
    HICON magnifier;
    LONG dpiValue;

    dpiValue = PhGetWindowDpi(hwndDlg);

    folder = PH_LOAD_SHARED_ICON_SMALL(PhInstanceHandle, MAKEINTRESOURCE(IDI_FOLDER), dpiValue);
    magnifier = PH_LOAD_SHARED_ICON_SMALL(PhInstanceHandle, MAKEINTRESOURCE(IDI_MAGNIFIER), dpiValue);

    SET_BUTTON_ICON(IDC_INSPECT, magnifier);
    SET_BUTTON_ICON(IDC_OPENFILENAME, folder);
    SET_BUTTON_ICON(IDC_INSPECT2, magnifier);
    SET_BUTTON_ICON(IDC_OPENFILENAME2, folder);
    SET_BUTTON_ICON(IDC_VIEWCOMMANDLINE, magnifier);
    SET_BUTTON_ICON(IDC_VIEWPARENTPROCESS, magnifier);
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
    PPH_PROCGENERAL_CONTEXT context;

    if (PhPropPageDlgProcHeader(hwndDlg, uMsg, lParam, &propSheetPage, &propPageContext, &processItem))
    {
        context = (PPH_PROCGENERAL_CONTEXT)propPageContext->Context;
    }
    else
    {
        return FALSE;
    }

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            HANDLE processHandle = NULL;
            PPH_STRING curDir = NULL;
            PPH_PROCESS_ITEM parentProcess;
            CLIENT_ID clientId;

            context = propPageContext->Context = PhAllocateZero(sizeof(PH_PROCGENERAL_CONTEXT));
            context->WindowHandle = hwndDlg;
            context->StartedLabelHandle = GetDlgItem(hwndDlg, IDC_STARTED);
            context->Enabled = TRUE;

            PphProcessGeneralDlgUpdateIcons(hwndDlg);

            // File

            context->ProgramIcon = PhGetImageListIcon(processItem->LargeIconIndex, TRUE);
            Static_SetIcon(GetDlgItem(hwndDlg, IDC_FILEICON), context->ProgramIcon);

            if (PH_IS_REAL_PROCESS_ID(processItem->ProcessId))
            {
                PhSetDialogItemText(hwndDlg, IDC_NAME, PhpGetStringOrNa(processItem->VersionInfo.FileDescription));
            }
            else
            {
                PhSetDialogItemText(hwndDlg, IDC_NAME, processItem->ProcessName->Buffer);
            }

            PhSetDialogItemText(hwndDlg, IDC_VERSION, PhpGetStringOrNa(processItem->VersionInfo.FileVersion));
            PhSetDialogItemText(hwndDlg, IDC_FILENAME, PhpGetStringOrNa(processItem->FileNameWin32));
            PhSetDialogItemText(hwndDlg, IDC_FILENAMEWIN32, PhpGetStringOrNa(processItem->FileName));

            {
                PPH_STRING inspectExecutables;

                if (PhIsNullOrEmptyString(processItem->FileName))
                {
                    EnableWindow(GetDlgItem(hwndDlg, IDC_OPENFILENAME), FALSE);
                    EnableWindow(GetDlgItem(hwndDlg, IDC_INSPECT), FALSE);
                }
                else
                {
                    if (!PhDoesFileExist(&processItem->FileName->sr))
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
                // Tell the function to get the WOW64 current directory, because that's the one that actually gets updated.
                if (NT_SUCCESS(PhGetProcessCurrentDirectory(
                    processHandle,
                    !!processItem->IsWow64,
                    &curDir
                    )))
                {
                    PH_AUTO(curDir);
                }

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

                PhSetWindowText(context->StartedLabelHandle, PhaFormatString(
                    L"%s ago (%s)",
                    startTimeRelativeString->Buffer,
                    startTimeString->Buffer
                    )->Buffer);
            }
            else
            {
                PhSetWindowText(context->StartedLabelHandle, L"N/A");
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
                if (processItem->ProcessId == SYSTEM_IDLE_PROCESS_ID)
                {
                    PhSetDialogItemText(hwndDlg, IDC_PARENTPROCESS, L"N/A");
                }
                else
                {
                    PhSetDialogItemText(hwndDlg, IDC_PARENTPROCESS, PhaFormatString(
                        L"Non-existent process (%lu)", HandleToUlong(processItem->ParentProcessId))->Buffer);
                }

                EnableWindow(GetDlgItem(hwndDlg, IDC_VIEWPARENTPROCESS), FALSE);
            }

            // Parent console

            //if (NT_SUCCESS(PhGetProcessConsoleHostProcessId(processItem->QueryHandle, &consoleParentProcessId)))
            //{
            //    ProcessItem->ParentProcessId = consoleParentProcessId;
            //    PhPrintUInt32(ProcessItem->ParentProcessIdString, HandleToUlong(consoleParentProcessId));
            //}

            if (parentProcess = PhReferenceProcessItem((HANDLE)((ULONG_PTR)processItem->ConsoleHostProcessId & ~3)))
            {
                if (parentProcess->ProcessId == SYSTEM_IDLE_PROCESS_ID)
                {
                    PhSetDialogItemText(hwndDlg, IDC_PARENTCONSOLE, L"N/A");
                }
                else
                {
                    clientId.UniqueProcess = parentProcess->ProcessId;
                    clientId.UniqueThread = NULL;

                    PhSetDialogItemText(
                        hwndDlg,
                        IDC_PARENTCONSOLE,
                        PH_AUTO_T(PH_STRING, PhGetClientIdNameEx(&clientId, parentProcess->ProcessName))->Buffer
                        );
                }

                PhDereferenceObject(parentProcess);
            }
            else
            {
                PhSetDialogItemText(hwndDlg, IDC_PARENTCONSOLE, L"N/A");
            }

            // Mitigation policies

            PhpUpdateProcessMitigationPolicies(hwndDlg, processItem);

            // Protection

            PhSetDialogItemText(hwndDlg, IDC_PROTECTION, PH_AUTO_T(PH_STRING, PhGetProcessItemProtectionText(processItem))->Buffer);

            // Image type

            PhSetDialogItemText(hwndDlg, IDC_PROCESSTYPETEXT, PH_AUTO_T(PH_STRING, PhGetProcessItemImageTypeText(processItem))->Buffer);

            PhInitializeWindowTheme(hwndDlg, PhEnableThemeSupport);

            SetTimer(hwndDlg, 1, 1000, NULL);
        }
        break;
    case WM_DESTROY:
        {
            KillTimer(hwndDlg, 1);

            if (context->ProgramIcon)
            {
                DestroyIcon(context->ProgramIcon);
            }

            PhFree(context);
        }
        break;
    case WM_DPICHANGED_AFTERPARENT:
        {
            PphProcessGeneralDlgUpdateIcons(hwndDlg);
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
                PhAddPropPageLayoutItem(hwndDlg, GetDlgItem(hwndDlg, IDC_FILENAME), dialogItem, PH_ANCHOR_LEFT | PH_ANCHOR_TOP | PH_ANCHOR_RIGHT);
                PhAddPropPageLayoutItem(hwndDlg, GetDlgItem(hwndDlg, IDC_INSPECT), dialogItem, PH_ANCHOR_TOP | PH_ANCHOR_RIGHT);
                PhAddPropPageLayoutItem(hwndDlg, GetDlgItem(hwndDlg, IDC_OPENFILENAME), dialogItem, PH_ANCHOR_TOP | PH_ANCHOR_RIGHT);
                PhAddPropPageLayoutItem(hwndDlg, GetDlgItem(hwndDlg, IDC_FILENAMEWIN32), dialogItem, PH_ANCHOR_LEFT | PH_ANCHOR_TOP | PH_ANCHOR_RIGHT);
                PhAddPropPageLayoutItem(hwndDlg, GetDlgItem(hwndDlg, IDC_INSPECT2), dialogItem, PH_ANCHOR_TOP | PH_ANCHOR_RIGHT);
                PhAddPropPageLayoutItem(hwndDlg, GetDlgItem(hwndDlg, IDC_OPENFILENAME2), dialogItem, PH_ANCHOR_TOP | PH_ANCHOR_RIGHT);
                PhAddPropPageLayoutItem(hwndDlg, GetDlgItem(hwndDlg, IDC_CMDLINE), dialogItem, PH_ANCHOR_LEFT | PH_ANCHOR_TOP | PH_ANCHOR_RIGHT);
                PhAddPropPageLayoutItem(hwndDlg, GetDlgItem(hwndDlg, IDC_VIEWCOMMANDLINE), dialogItem, PH_ANCHOR_TOP | PH_ANCHOR_RIGHT);
                PhAddPropPageLayoutItem(hwndDlg, GetDlgItem(hwndDlg, IDC_CURDIR), dialogItem, PH_ANCHOR_LEFT | PH_ANCHOR_TOP | PH_ANCHOR_RIGHT);
                PhAddPropPageLayoutItem(hwndDlg, context->StartedLabelHandle, dialogItem, PH_ANCHOR_LEFT | PH_ANCHOR_TOP | PH_ANCHOR_RIGHT);
                PhAddPropPageLayoutItem(hwndDlg, GetDlgItem(hwndDlg, IDC_PARENTCONSOLE), dialogItem, PH_ANCHOR_LEFT | PH_ANCHOR_TOP | PH_ANCHOR_RIGHT);
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
                    if (processItem->FileNameWin32)
                    {
                        PhShellExecuteUserString(
                            hwndDlg,
                            L"ProgramInspectExecutables",
                            processItem->FileNameWin32->Buffer,
                            FALSE,
                            L"Make sure the PE Viewer executable file is present."
                            );
                    }
                }
                break;
            case IDC_OPENFILENAME:
                {
                    if (processItem->FileNameWin32)
                    {
                        PhShellExecuteUserString(
                            hwndDlg,
                            L"FileBrowseExecutable",
                            processItem->FileNameWin32->Buffer,
                            FALSE,
                            L"Make sure the Explorer executable file is present."
                            );
                    }
                }
                break;
            case IDC_VIEWCOMMANDLINE:
                {
                    if (processItem->CommandLine)
                    {
                        PPH_STRING commandLineString;
                        INT stringArgCount;
                        PWSTR* stringArgList;

                        if (stringArgList = CommandLineToArgvW(processItem->CommandLine->Buffer, &stringArgCount))
                        {
                            PH_STRING_BUILDER sb;

                            PhInitializeStringBuilder(&sb, 260);

                            for (INT i = 0; i < stringArgCount; i++)
                            {
                                PhAppendFormatStringBuilder(&sb, L"[%d] %s\r\n\r\n", i, stringArgList[i]);
                            }

                            PhAppendFormatStringBuilder(&sb, L"[FULL] %s\r\n", processItem->CommandLine->Buffer);

                            commandLineString = PhFinalStringBuilderString(&sb);

                            LocalFree(stringArgList);
                        }
                        else
                        {
                            commandLineString = PhReferenceObject(processItem->CommandLine);
                        }

                        PhShowInformationDialog(hwndDlg, commandLineString->Buffer, 0);

                        PhDereferenceObject(commandLineString);
                    }
                }
                break;
            case IDC_VIEWPARENTPROCESS:
                {
                    PPH_PROCESS_ITEM parentProcessItem;

                    if (parentProcessItem = PhReferenceProcessItem(processItem->ParentProcessId))
                    {
                        ProcessHacker_ShowProcessProperties(parentProcessItem);
                        PhDereferenceObject(parentProcessItem);
                    }
                    else
                    {
                        PhShowError(hwndDlg, L"%s", L"The process does not exist.");
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
            case PSN_SETACTIVE:
                context->Enabled = TRUE;
                break;
            case PSN_KILLACTIVE:
                context->Enabled = FALSE;
                break;
            case NM_CLICK:
                {
                    switch (header->idFrom)
                    {
                    case IDC_COMPANYNAME_LINK:
                        {
                            if (processItem->FileName)
                            {
                                NTSTATUS status;
                                HANDLE fileHandle;

                                status = PhCreateFile(
                                    &fileHandle,
                                    &processItem->FileName->sr,
                                    FILE_READ_DATA | FILE_READ_ATTRIBUTES | SYNCHRONIZE,
                                    FILE_ATTRIBUTE_NORMAL,
                                    FILE_SHARE_READ | FILE_SHARE_DELETE,
                                    FILE_OPEN,
                                    FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT
                                    );

                                if (NT_SUCCESS(status))
                                {
                                    PH_VERIFY_FILE_INFO info;

                                    memset(&info, 0, sizeof(PH_VERIFY_FILE_INFO));
                                    info.FileHandle = fileHandle;
                                    info.Flags = PH_VERIFY_VIEW_PROPERTIES;
                                    info.hWnd = hwndDlg;
                                    PhVerifyFileWithAdditionalCatalog(
                                        &info,
                                        processItem->PackageFullName,
                                        NULL
                                        );

                                    NtClose(fileHandle);
                                }
                                else
                                {
                                    PhShowStatus(hwndDlg, L"Unable to perform the operation.", status, 0);
                                }
                            }
                        }
                        break;
                    }
                }
                break;
            }
        }
        break;
    case WM_TIMER:
        {
            if (!(context->Enabled && GetFocus() != context->StartedLabelHandle))
                break;

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

                PhSetWindowText(context->StartedLabelHandle, PhaFormatString(
                    L"%s ago (%s)",
                    startTimeRelativeString->Buffer,
                    startTimeString->Buffer
                    )->Buffer);
            }
            else
            {
                PhSetWindowText(context->StartedLabelHandle, L"N/A");
            }
        }
        break;
    }

    return FALSE;
}
