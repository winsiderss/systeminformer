/*
 * Process Hacker - 
 *   process record properties
 * 
 * Copyright (C) 2010 wj32
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

typedef struct _PROCESS_RECORD_CONTEXT
{
    PPH_PROCESS_RECORD Record;
} PROCESS_RECORD_CONTEXT, *PPROCESS_RECORD_CONTEXT;

INT_PTR CALLBACK PhpProcessRecordDlgProc(
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    );

VOID PhShowProcessRecordDialog(
    __in HWND ParentWindowHandle,
    __in PPH_PROCESS_RECORD Record
    )
{
    PROCESS_RECORD_CONTEXT context;

    context.Record = Record;

    DialogBoxParam(
        PhInstanceHandle,
        MAKEINTRESOURCE(IDD_PROCRECORD),
        ParentWindowHandle,
        PhpProcessRecordDlgProc,
        (LPARAM)&context
        );
}

PPH_STRING PhapGetRelativeTimeString(
    __in PLARGE_INTEGER Time
    )
{
    LARGE_INTEGER time;
    LARGE_INTEGER currentTime;
    SYSTEMTIME timeFields;
    PPH_STRING timeRelativeString;
    PPH_STRING timeString;

    time = *Time;
    PhQuerySystemTime(&currentTime);
    timeRelativeString = PHA_DEREFERENCE(PhFormatTimeSpanRelative(currentTime.QuadPart - time.QuadPart));

    PhLargeIntegerToLocalSystemTime(&timeFields, &time);
    timeString = PhaFormatDateTime(&timeFields);

    return PhaFormatString(L"%s (%s)", timeRelativeString->Buffer, timeString->Buffer);
}

FORCEINLINE PWSTR PhpGetStringOrNa(
    __in PPH_STRING String
    )
{
    if (String)
        return String->Buffer;
    else
        return L"N/A";
}

INT_PTR CALLBACK PhpProcessRecordDlgProc(
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    )
{
    PPROCESS_RECORD_CONTEXT context = NULL;

    if (uMsg == WM_INITDIALOG)
    {
        context = (PPROCESS_RECORD_CONTEXT)lParam;
        SetProp(hwndDlg, L"Context", (HANDLE)context);
    }
    else
    {
        context = (PPROCESS_RECORD_CONTEXT)GetProp(hwndDlg, L"Context");

        if (uMsg == WM_DESTROY)
        {
            RemoveProp(hwndDlg, L"Context");
        }
    }

    if (!context)
        return FALSE;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            PH_IMAGE_VERSION_INFO versionInfo;
            SHFILEINFO fileInfo;
            HICON icon;
            PPH_STRING processNameString;

            processNameString = PhaFormatString(L"%s (%u)",
                context->Record->ProcessName->Buffer, (ULONG)context->Record->ProcessId);

            PhCenterWindow(hwndDlg, GetParent(hwndDlg));
            SetWindowText(hwndDlg, processNameString->Buffer);

            SetDlgItemText(hwndDlg, IDC_PROCESSNAME, processNameString->Buffer);
            SetDlgItemText(hwndDlg, IDC_PARENT, PhaFormatString(L"Unknown Process (%u)",
                (ULONG)context->Record->ParentProcessId)->Buffer);

            memset(&versionInfo, 0, sizeof(PH_IMAGE_VERSION_INFO));
            icon = NULL;

            if (context->Record->FileName)
            {
                PhInitializeImageVersionInfo(&versionInfo, context->Record->FileName->Buffer);

                if (SHGetFileInfo(
                    context->Record->FileName->Buffer,
                    0,
                    &fileInfo,
                    sizeof(SHFILEINFO),
                    SHGFI_ICON | SHGFI_LARGEICON
                    ))
                {
                    icon = fileInfo.hIcon;
                }
            }

            if (!icon)
            {
                if (SHGetFileInfo(
                    L".exe",
                    FILE_ATTRIBUTE_NORMAL,
                    &fileInfo,
                    sizeof(SHFILEINFO),
                    SHGFI_ICON | SHGFI_LARGEICON | SHGFI_USEFILEATTRIBUTES
                    ))
                    icon = fileInfo.hIcon;
            }

            SendMessage(GetDlgItem(hwndDlg, IDC_OPENFILENAME), BM_SETIMAGE, IMAGE_BITMAP,
                (LPARAM)PH_LOAD_SHARED_IMAGE(MAKEINTRESOURCE(IDB_FOLDER), IMAGE_BITMAP));
            SendMessage(GetDlgItem(hwndDlg, IDC_FILEICON), STM_SETICON,
                (WPARAM)icon, 0);

            SetDlgItemText(hwndDlg, IDC_NAME, PhpGetStringOrNa(versionInfo.FileDescription));
            SetDlgItemText(hwndDlg, IDC_COMPANYNAME, PhpGetStringOrNa(versionInfo.CompanyName));
            SetDlgItemText(hwndDlg, IDC_VERSION, PhpGetStringOrNa(versionInfo.FileVersion));
            SetDlgItemText(hwndDlg, IDC_FILENAME, PhpGetStringOrNa(context->Record->FileName));

            if (!context->Record->FileName)
                EnableWindow(GetDlgItem(hwndDlg, IDC_OPENFILENAME), FALSE);

            SetDlgItemText(hwndDlg, IDC_CMDLINE, PhpGetStringOrNa(context->Record->CommandLine));

            if (context->Record->CreateTime.QuadPart != 0)
                SetDlgItemText(hwndDlg, IDC_STARTED, PhapGetRelativeTimeString(&context->Record->CreateTime)->Buffer);
            else
                SetDlgItemText(hwndDlg, IDC_STARTED, L"N/A");

            if (context->Record->ExitTime.QuadPart != 0)
                SetDlgItemText(hwndDlg, IDC_TERMINATED, PhapGetRelativeTimeString(&context->Record->ExitTime)->Buffer);
            else
                SetDlgItemText(hwndDlg, IDC_TERMINATED, L"N/A");

            SetDlgItemInt(hwndDlg, IDC_SESSIONID, context->Record->SessionId, FALSE);
        }
        break;
    case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
            case IDCANCEL:
            case IDOK:
                {
                    EndDialog(hwndDlg, IDOK);
                }
                break;
            case IDC_OPENFILENAME:
                {
                    if (context->Record->FileName)
                        PhShellExploreFile(hwndDlg, context->Record->FileName->Buffer);
                }
                break;
            }
        }
        break;
    }

    return FALSE;
}
