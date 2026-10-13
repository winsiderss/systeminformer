/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2010
 *     dmex    2018-2026
 *
 */

#include <phapp.h>
#include <emenu.h>
#include <phsettings.h>
#include <secedit.h>
#include <lsasup.h>
#include <winsta.h>

INT_PTR CALLBACK PhpSessionPropertiesDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

static CONST PH_KEY_VALUE_PAIR PhpConnectStatePairs[] =
{
    SIP(L"Active", State_Active),
    SIP(L"Connected", State_Connected),
    SIP(L"ConnectQuery", State_ConnectQuery),
    SIP(L"Shadow", State_Shadow),
    SIP(L"Disconnected", State_Disconnected),
    SIP(L"Idle", State_Idle),
    SIP(L"Listen", State_Listen),
    SIP(L"Reset", State_Reset),
    SIP(L"Down", State_Down),
    SIP(L"Init", State_Init)
};

VOID PhShowSessionProperties(
    _In_ HWND ParentWindowHandle,
    _In_ ULONG SessionId
    )
{
    PhDialogBox(
        PhInstanceHandle,
        MAKEINTRESOURCE(IDD_SESSION),
        PhCsForceNoParent ? NULL : ParentWindowHandle,
        PhpSessionPropertiesDlgProc,
        UlongToPtr(SessionId)
        );
}

#define WM_PH_SESSION_NLTEST_RESULT (WM_APP + 1)

typedef struct _PHP_SESSION_PROPERTIES_CONTEXT
{
    HWND ListViewHandle;
    ULONG SessionId;
    PPH_STRING UserName;
    PSECURITY_DESCRIPTOR SecurityDescriptor;
    PH_LAYOUT_MANAGER LayoutManager;
} PHP_SESSION_PROPERTIES_CONTEXT, *PPHP_SESSION_PROPERTIES_CONTEXT;

typedef struct _PHP_SESSION_NLTEST_CONTEXT
{
    HWND WindowHandle;
    PPH_STRING UserName;
} PHP_SESSION_NLTEST_CONTEXT, *PPHP_SESSION_NLTEST_CONTEXT;

static PPH_STRING PhpSessionNltestFormatSid(
    _In_ PSID Sid
    )
{
    PPH_STRING name;

    name = PhGetSidFullName(Sid, TRUE, NULL);

    if (name)
        return name;

    return PhSidToStringSid(Sid);
}

static PPH_STRING PhpSessionNltestFormatAccess(
    _In_ ACCESS_MASK AccessMask
    )
{
    PH_STRING_BUILDER sb;

    if ((AccessMask & USER_ALL_ACCESS) == USER_ALL_ACCESS)
        return PhFormatString(L"Full Control (0x%lx)", AccessMask);

    PhInitializeStringBuilder(&sb, 64);

    if ((AccessMask & USER_READ) == USER_READ)
        PhAppendStringBuilder2(&sb, L"Read/");
    if ((AccessMask & USER_WRITE) == USER_WRITE)
        PhAppendStringBuilder2(&sb, L"Write/");
    if ((AccessMask & USER_EXECUTE) == USER_EXECUTE)
        PhAppendStringBuilder2(&sb, L"Execute/");
    if (AccessMask & USER_LIST_GROUPS)
        PhAppendStringBuilder2(&sb, L"List Groups/");
    if (AccessMask & DELETE)
        PhAppendStringBuilder2(&sb, L"Delete/");
    if (AccessMask & READ_CONTROL)
        PhAppendStringBuilder2(&sb, L"Read Control/");
    if (AccessMask & WRITE_DAC)
        PhAppendStringBuilder2(&sb, L"Write DAC/");
    if (AccessMask & WRITE_OWNER)
        PhAppendStringBuilder2(&sb, L"Write Owner/");

    if (sb.String->Length != 0)
        PhRemoveEndStringBuilder(&sb, 1);
    else
        PhAppendStringBuilder2(&sb, L"Special");

    PhAppendFormatStringBuilder(&sb, L" (0x%lx)", AccessMask);

    return PhFinalStringBuilderString(&sb);
}

static VOID PhpSessionNltestAddAcl(
    _In_ PPH_LIST List,
    _In_ PACL Acl,
    _In_ BOOLEAN Sacl
    )
{
    for (ULONG i = 0; i < Acl->AceCount; i++)
    {
        PACE_HEADER ace;
        PSID sid;
        ACCESS_MASK mask;
        PCWSTR typeName;
        PPH_STRING sidName;
        PPH_STRING accessString;

        if (!NT_SUCCESS(RtlGetAce(Acl, i, &ace)))
            break;

        switch (ace->AceType)
        {
        case ACCESS_ALLOWED_ACE_TYPE:
            typeName = L"Allow";
            break;
        case ACCESS_DENIED_ACE_TYPE:
            typeName = L"Deny";
            break;
        case SYSTEM_AUDIT_ACE_TYPE:
            typeName = L"Audit";
            break;
        case SYSTEM_MANDATORY_LABEL_ACE_TYPE:
            typeName = L"Label";
            break;
        default:
            PhAddItemList(List, PhCreateString(Sacl ? L"Audit" : L"Allow"));
            PhAddItemList(List, PhFormatString(L"Unsupported ACE type %u", ace->AceType));
            continue;
        }

        // ACCESS_ALLOWED_ACE, ACCESS_DENIED_ACE, SYSTEM_AUDIT_ACE and SYSTEM_MANDATORY_LABEL_ACE share a layout.
        mask = ((PACCESS_ALLOWED_ACE)ace)->Mask;
        sid = (PSID)&((PACCESS_ALLOWED_ACE)ace)->SidStart;
        sidName = PhpSessionNltestFormatSid(sid);
        accessString = PhpSessionNltestFormatAccess(mask);

        PhAddItemList(List, PhCreateString(typeName));

        if (ace->AceType == SYSTEM_AUDIT_ACE_TYPE)
        {
            PhAddItemList(List, PhFormatString(
                L"%s: %s%s%s",
                PhGetStringOrDefault(sidName, L"Unknown"),
                PhGetString(accessString),
                (ace->AceFlags & SUCCESSFUL_ACCESS_ACE_FLAG) ? L", Success" : L"",
                (ace->AceFlags & FAILED_ACCESS_ACE_FLAG) ? L", Failure" : L""
                ));
        }
        else
        {
            PhAddItemList(List, PhFormatString(
                L"%s: %s",
                PhGetStringOrDefault(sidName, L"Unknown"),
                PhGetString(accessString)
                ));
        }

        PhClearReference(&sidName);
        PhDereferenceObject(accessString);
    }
}

/**
 * Decodes the nltest hex dump of a self-relative security descriptor into key/value rows.
 *
 * \param Value The space-separated list of little-endian ULONG hex words.
 * \param List The list that receives alternating key and value strings.
 * \param SecurityDescriptor Receives a copy of the security descriptor. Free with PhFree.
 * \return TRUE if the descriptor was decoded, FALSE if it is invalid or truncated.
 */
static BOOLEAN PhpSessionNltestAddSecurityDescriptor(
    _In_ PCPH_STRINGREF Value,
    _In_ PPH_LIST List,
    _Out_ PSECURITY_DESCRIPTOR* SecurityDescriptor
    )
{
    BOOLEAN result = FALSE;
    PH_STRINGREF remaining = *Value;
    PH_STRINGREF token;
    PULONG buffer;
    ULONG count = 0;
    ULONG capacity;
    PSID sid;
    BOOLEAN present;
    BOOLEAN defaulted;
    PACL acl;

    capacity = (ULONG)(Value->Length / sizeof(WCHAR) / 8) + 1;
    buffer = PhAllocate(capacity * sizeof(ULONG));

    while (remaining.Length != 0 && count < capacity)
    {
        LONG64 word;

        PhSplitStringRefAtChar(&remaining, L' ', &token, &remaining);

        if (token.Length == 0)
            continue;
        if (token.Length != 8 * sizeof(WCHAR)) // truncated word
            break;
        if (!PhStringToInteger64(&token, 16, &word))
            break;

        buffer[count++] = (ULONG)word;
    }

    if (count == 0 || !RtlValidRelativeSecurityDescriptor(buffer, count * sizeof(ULONG), 0))
        goto CleanupExit;

    if (NT_SUCCESS(RtlGetOwnerSecurityDescriptor(buffer, &sid, &defaulted)) && sid)
    {
        PhAddItemList(List, PhCreateString(L"Owner"));
        PhAddItemList(List, PhpSessionNltestFormatSid(sid));
    }

    if (NT_SUCCESS(RtlGetGroupSecurityDescriptor(buffer, &sid, &defaulted)) && sid)
    {
        PhAddItemList(List, PhCreateString(L"Group"));
        PhAddItemList(List, PhpSessionNltestFormatSid(sid));
    }

    if (NT_SUCCESS(RtlGetDaclSecurityDescriptor(buffer, &present, &acl, &defaulted)) && present)
    {
        if (acl)
        {
            PhpSessionNltestAddAcl(List, acl, FALSE);
        }
        else
        {
            PhAddItemList(List, PhCreateString(L"Allow"));
            PhAddItemList(List, PhCreateString(L"NULL DACL (Everyone: Full Control)"));
        }
    }

    if (NT_SUCCESS(RtlGetSaclSecurityDescriptor(buffer, &present, &acl, &defaulted)) && present && acl)
    {
        PhpSessionNltestAddAcl(List, acl, TRUE);
    }

    *SecurityDescriptor = buffer;
    buffer = NULL;
    result = TRUE;

CleanupExit:
    if (buffer)
        PhFree(buffer);

    return result;
}

_Function_class_(USER_THREAD_START_ROUTINE)
NTSTATUS PhpSessionNltestThreadStart(
    _In_ PVOID Parameter
    )
{
    PPHP_SESSION_NLTEST_CONTEXT context = Parameter;
    PPH_STRING systemDirectory;
    PPH_STRING fileName;
    PPH_STRING commandLine;
    PPH_STRING commandOutput = NULL;
    PPH_LIST list = NULL;
    PSECURITY_DESCRIPTOR securityDescriptor = NULL;

    systemDirectory = PhGetSystemDirectory();

    if (systemDirectory)
    {
        // nltest.exe is only present on Server SKUs or with RSAT installed. (STATUS_OBJECT_PATH_NOT_FOUND)
        fileName = PhConcatStringRefZ(&systemDirectory->sr, L"\\nltest.exe");

        if (!PhDoesFileExistWin32(PhGetString(fileName)))
        {
            PhDereferenceObject(fileName);
            PhDereferenceObject(systemDirectory);
            goto CleanupExit;
        }

        commandLine = PhFormatString(
            L"\"%s\" /USER:%s",
            fileName->Buffer,
            context->UserName->Buffer
            );

        if (NT_SUCCESS(PhCreateProcessRedirection(&fileName->sr, &commandLine->sr, NULL, &commandOutput)) && commandOutput)
        {
            static CONST PH_STRINGREF whitespace = PH_STRINGREF_INIT(L" \t\r");
            PH_STRINGREF remaining = commandOutput->sr;
            PH_STRINGREF line;
            PH_STRINGREF key;
            PH_STRINGREF value;

            list = PhCreateList(64);

            while (remaining.Length != 0)
            {
                PhSplitStringRefAtChar(&remaining, L'\n', &line, &remaining);

                if (!PhSplitStringRefAtChar(&line, L':', &key, &value))
                    continue;

                PhTrimStringRef(&key, &whitespace, 0);
                PhTrimStringRef(&value, &whitespace, 0);

                if (key.Length == 0)
                    continue;

                // Replace the raw hex dump with decoded owner/group/ACE rows.
                if (
                    !securityDescriptor &&
                    PhEqualStringRef2(&key, L"SecurityDescriptor", TRUE) &&
                    PhpSessionNltestAddSecurityDescriptor(&value, list, &securityDescriptor)
                    )
                {
                    continue;
                }

                PhAddItemList(list, PhCreateString2(&key));

                // Timestamps are formatted as "<low> <high> = <date>". Show the date first
                // followed by the raw 64-bit value. (dmex)
                {
                    static CONST PH_STRINGREF separator = PH_STRINGREF_INIT(L" = ");
                    PH_STRINGREF rawPart;
                    PH_STRINGREF datePart;
                    PH_STRINGREF lowPart;
                    PH_STRINGREF highPart;

                    if (
                        PhSplitStringRefAtString(&value, &separator, FALSE, &rawPart, &datePart) &&
                        PhSplitStringRefAtChar(&rawPart, L' ', &lowPart, &highPart)
                        )
                    {
                        if (PhEqualStringRef2(&rawPart, L"ffffffff 7fffffff", TRUE))
                        {
                            PhAddItemList(list, PhCreateString(L"Never (0x7fffffffffffffff)"));
                        }
                        else
                        {
                            PhAddItemList(list, PhFormatString(
                                L"%.*s (0x%.*s%.*s)",
                                (ULONG)(datePart.Length / sizeof(WCHAR)), datePart.Buffer,
                                (ULONG)(highPart.Length / sizeof(WCHAR)), highPart.Buffer,
                                (ULONG)(lowPart.Length / sizeof(WCHAR)), lowPart.Buffer
                                ));
                        }

                        continue;
                    }
                }

                PhAddItemList(list, PhCreateString2(&value));
            }

            PhDereferenceObject(commandOutput);
        }

        PhDereferenceObject(fileName);
        PhDereferenceObject(commandLine);
        PhDereferenceObject(systemDirectory);
    }

    if (list && !PostMessage(context->WindowHandle, WM_PH_SESSION_NLTEST_RESULT, (WPARAM)securityDescriptor, (LPARAM)list))
    {
        PhDereferenceObjects(list->Items, list->Count);
        PhDereferenceObject(list);
        list = NULL;
    }

    if (!list && securityDescriptor)
        PhFree(securityDescriptor);

CleanupExit:
    PhDereferenceObject(context->UserName);
    PhFree(context);

    return STATUS_SUCCESS;
}

_Function_class_(PH_GET_OBJECT_SECURITY)
static NTSTATUS NTAPI PhpSessionNltestGetSecurity(
    _Out_ PSECURITY_DESCRIPTOR *SecurityDescriptor,
    _In_ SECURITY_INFORMATION SecurityInformation,
    _In_opt_ PVOID Context
    )
{
    PPH_STD_OBJECT_SECURITY objectSecurity = Context;
    PPHP_SESSION_PROPERTIES_CONTEXT context;

    UNREFERENCED_PARAMETER(SecurityInformation);

    if (!objectSecurity)
        return STATUS_INVALID_PARAMETER;

    context = objectSecurity->Context;

    if (!context || !context->SecurityDescriptor)
        return STATUS_INVALID_PARAMETER;

    *SecurityDescriptor = PhAllocateCopy(
        context->SecurityDescriptor,
        PhLengthSecurityDescriptor(context->SecurityDescriptor)
        );

    return STATUS_SUCCESS;
}

_Function_class_(PH_SET_OBJECT_SECURITY)
static NTSTATUS NTAPI PhpSessionNltestSetSecurity(
    _In_ PSECURITY_DESCRIPTOR SecurityDescriptor,
    _In_ SECURITY_INFORMATION SecurityInformation,
    _In_opt_ PVOID Context
    )
{
    UNREFERENCED_PARAMETER(SecurityDescriptor);
    UNREFERENCED_PARAMETER(SecurityInformation);
    UNREFERENCED_PARAMETER(Context);

    // The descriptor is a snapshot from nltest output; changes cannot be applied.
    return STATUS_ACCESS_DENIED;
}

VOID PhpSessionPropertiesQueryWinStationInfo(
    _In_ PPHP_SESSION_PROPERTIES_CONTEXT Context,
    _In_ ULONG SessionId
    )
{
    WINSTATIONINFORMATION winStationInfo;
    BOOLEAN haveWinStationInfo;
    WINSTATIONCLIENT clientInfo;
    BOOLEAN haveClientInfo;
    ULONG returnLength;
    PWSTR stateString;

    // Query basic session information

    haveWinStationInfo = WinStationQueryInformationW(
        WINSTATION_CURRENT_SERVER,
        SessionId,
        WinStationInformation,
        &winStationInfo,
        sizeof(WINSTATIONINFORMATION),
        &returnLength
        );

    // Query client information

    haveClientInfo = WinStationQueryInformationW(
        WINSTATION_CURRENT_SERVER,
        SessionId,
        WinStationClient,
        &clientInfo,
        sizeof(WINSTATIONCLIENT),
        &returnLength
        );

    if (haveWinStationInfo)
    {
        PH_FORMAT format[3];
        WCHAR formatBuffer[256];

        PhInitFormatS(&format[0], winStationInfo.Domain);
        PhInitFormatC(&format[1], OBJ_NAME_PATH_SEPARATOR);
        PhInitFormatS(&format[2], winStationInfo.UserName);

        if (PhFormatToBuffer(
            format,
            RTL_NUMBER_OF(format),
            formatBuffer,
            sizeof(formatBuffer),
            NULL
            ))
        {
            PhSetListViewSubItem(Context->ListViewHandle, 0, 1, formatBuffer);
        }

        if (winStationInfo.UserName[0] != UNICODE_NULL)
        {
            PhMoveReference(&Context->UserName, PhCreateString(winStationInfo.UserName));
        }
    }

    PhSetListViewSubItem(Context->ListViewHandle, 1, 1, PhaFormatUInt64(SessionId, FALSE)->Buffer);

    if (haveWinStationInfo)
    {
        if (PhFindStringSiKeyValuePairs(
            PhpConnectStatePairs,
            sizeof(PhpConnectStatePairs),
            winStationInfo.ConnectState,
            &stateString
            ))
        {
            PhSetListViewSubItem(Context->ListViewHandle, 2, 1, stateString);
        }
    }

    if (haveWinStationInfo && winStationInfo.LogonTime.QuadPart != 0)
    {
        SYSTEMTIME systemTime;
        PPH_STRING time;

        PhLargeIntegerToLocalSystemTime(&systemTime, &winStationInfo.LogonTime);
        time = PhFormatDateTime(&systemTime);
        PhSetListViewSubItem(Context->ListViewHandle, 3, 1, time->Buffer);
        PhDereferenceObject(time);
    }

    if (haveWinStationInfo && winStationInfo.ConnectTime.QuadPart != 0)
    {
        SYSTEMTIME systemTime;
        PPH_STRING time;

        PhLargeIntegerToLocalSystemTime(&systemTime, &winStationInfo.ConnectTime);
        time = PhFormatDateTime(&systemTime);
        PhSetListViewSubItem(Context->ListViewHandle, 4, 1, time->Buffer);
        PhDereferenceObject(time);
    }

    if (haveWinStationInfo && winStationInfo.DisconnectTime.QuadPart != 0)
    {
        SYSTEMTIME systemTime;
        PPH_STRING time;

        PhLargeIntegerToLocalSystemTime(&systemTime, &winStationInfo.DisconnectTime);
        time = PhFormatDateTime(&systemTime);
        PhSetListViewSubItem(Context->ListViewHandle, 5, 1, time->Buffer);
        PhDereferenceObject(time);
    }

    if (haveWinStationInfo && winStationInfo.LastInputTime.QuadPart != 0)
    {
        SYSTEMTIME systemTime;
        PPH_STRING time;

        PhLargeIntegerToLocalSystemTime(&systemTime, &winStationInfo.LastInputTime);
        time = PhFormatDateTime(&systemTime);
        PhSetListViewSubItem(Context->ListViewHandle, 6, 1, time->Buffer);
        PhDereferenceObject(time);
    }

    if (haveClientInfo && clientInfo.ClientName[0] != UNICODE_NULL)
    {
        WCHAR addressString[INET6_ADDRSTRLEN + 1];

        PhSetListViewSubItem(Context->ListViewHandle, 7, 1, clientInfo.ClientName);

        if (clientInfo.ClientAddressFamily == AF_INET6)
        {
            ULONG addressLength = RTL_NUMBER_OF(addressString);
            IN6_ADDR address;
            ULONG i;
            PUSHORT in;
            PUSHORT out;

            // IPv6 is special - the client address data is a reversed version of
            // the real address.

            in = (PUSHORT)clientInfo.ClientAddress;
            out = (PUSHORT)address.u.Word;

            for (i = 8; i != 0; i--)
            {
                *out = _byteswap_ushort(*in);
                in++;
                out++;
            }

            RtlIpv6AddressToStringEx(&address, 0, 0, addressString, &addressLength);
        }
        else
        {
            wcscpy_s(addressString, 65, clientInfo.ClientAddress);
        }

        PhSetListViewSubItem(Context->ListViewHandle, 8, 1, addressString);
        PhSetListViewSubItem(Context->ListViewHandle, 9, 1,
            PhaFormatString(L"%ux%u@%u", clientInfo.HRes, clientInfo.VRes, clientInfo.ColorDepth)->Buffer
            );
    }
}

INT_PTR CALLBACK PhpSessionPropertiesDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PPHP_SESSION_PROPERTIES_CONTEXT context;

    if (uMsg == WM_INITDIALOG)
    {
        context = PhAllocateZero(sizeof(PHP_SESSION_PROPERTIES_CONTEXT));
        context->SessionId = (ULONG)lParam;

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
            context->ListViewHandle = GetDlgItem(hwndDlg, IDC_LIST);

            PhSetApplicationWindowIcon(hwndDlg);

            PhSetListViewStyle(context->ListViewHandle, FALSE, TRUE);
            PhSetControlTheme(context->ListViewHandle, L"explorer");
            PhAddListViewColumn(context->ListViewHandle, 0, 0, 0, LVCFMT_LEFT, 150, L"Name");
            PhAddListViewColumn(context->ListViewHandle, 1, 1, 1, LVCFMT_LEFT, 250, L"Value");
            PhSetExtendedListView(context->ListViewHandle);

            ListView_EnableGroupView(context->ListViewHandle, TRUE);
            PhAddListViewGroup(context->ListViewHandle, 0, L"User");

            PhAddListViewGroupItem(context->ListViewHandle, 0, 0, L"User name", NULL);
            PhAddListViewGroupItem(context->ListViewHandle, 0, 1, L"Session ID", NULL);
            PhAddListViewGroupItem(context->ListViewHandle, 0, 2, L"State", NULL);
            PhAddListViewGroupItem(context->ListViewHandle, 0, 3, L"Logon time", NULL);
            PhAddListViewGroupItem(context->ListViewHandle, 0, 4, L"Connect time", NULL);
            PhAddListViewGroupItem(context->ListViewHandle, 0, 5, L"Disconnect time", NULL);
            PhAddListViewGroupItem(context->ListViewHandle, 0, 6, L"Last input time", NULL);
            PhAddListViewGroupItem(context->ListViewHandle, 0, 7, L"Client name", NULL);
            PhAddListViewGroupItem(context->ListViewHandle, 0, 8, L"Client address", NULL);
            PhAddListViewGroupItem(context->ListViewHandle, 0, 9, L"Client display", NULL);


            PhInitializeLayoutManager(&context->LayoutManager, hwndDlg);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDC_LIST), NULL, PH_ANCHOR_ALL);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDOK), NULL, PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDC_PERMISSIONS), NULL, PH_ANCHOR_LEFT | PH_ANCHOR_BOTTOM);

            EnableWindow(GetDlgItem(hwndDlg, IDC_PERMISSIONS), FALSE);

            PhpSessionPropertiesQueryWinStationInfo(context, context->SessionId);

            if (context->UserName)
            {
                PPHP_SESSION_NLTEST_CONTEXT threadContext;

                threadContext = PhAllocateZero(sizeof(PHP_SESSION_NLTEST_CONTEXT));
                threadContext->WindowHandle = hwndDlg;
                threadContext->UserName = PhReferenceObject(context->UserName);

                if (!NT_SUCCESS(PhCreateThread2(PhpSessionNltestThreadStart, threadContext)))
                {
                    PhDereferenceObject(threadContext->UserName);
                    PhFree(threadContext);
                }
            }

            PhSetDialogFocus(hwndDlg, GetDlgItem(hwndDlg, IDOK));

            PhCenterWindow(hwndDlg, GetParent(hwndDlg));

            PhInitializeWindowTheme(hwndDlg, PhEnableThemeSupport); // HACK
        }
        break;
    case WM_DESTROY:
        {
            PhRemoveWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT);

            PhDeleteLayoutManager(&context->LayoutManager);
            PhClearReference(&context->UserName);
            if (context->SecurityDescriptor)
                PhFree(context->SecurityDescriptor);
            PhFree(context);
        }
        break;
    case WM_PH_SESSION_NLTEST_RESULT:
        {
            PPH_LIST list = (PPH_LIST)lParam;
            PSECURITY_DESCRIPTOR securityDescriptor = (PSECURITY_DESCRIPTOR)wParam;

            if (securityDescriptor)
            {
                if (context->SecurityDescriptor)
                    PhFree(context->SecurityDescriptor);

                context->SecurityDescriptor = securityDescriptor;
                EnableWindow(GetDlgItem(hwndDlg, IDC_PERMISSIONS), TRUE);
            }

            if (list->Count != 0)
            {
                ExtendedListView_SetRedraw(context->ListViewHandle, FALSE);
                PhAddListViewGroup(context->ListViewHandle, 1, L"Account (nltest)");

                for (ULONG i = 0; i + 1 < list->Count; i += 2)
                {
                    PPH_STRING key = list->Items[i];
                    PPH_STRING value = list->Items[i + 1];
                    LONG index;

                    index = PhAddListViewGroupItem(context->ListViewHandle, 1, MAXINT, key->Buffer, NULL);

                    if (index != INT_ERROR)
                        PhSetListViewSubItem(context->ListViewHandle, index, 1, value->Buffer);
                }

                ExtendedListView_SetRedraw(context->ListViewHandle, TRUE);
            }

            PhDereferenceObjects(list->Items, list->Count);
            PhDereferenceObject(list);
        }
        break;
    case WM_COMMAND:
        {
            switch (GET_WM_COMMAND_ID(wParam, lParam))
            {
            case IDCANCEL:
            case IDOK:
                EndDialog(hwndDlg, IDOK);
                break;
            case IDC_PERMISSIONS:
                {
                    if (context->SecurityDescriptor)
                    {
                        PhEditSecurityEx(
                            hwndDlg,
                            PhGetString(context->UserName),
                            L"SamUser",
                            NULL,
                            NULL,
                            PhpSessionNltestGetSecurity,
                            PhpSessionNltestSetSecurity,
                            context
                            );
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
                    PhGetListViewContextMenuPoint(context->ListViewHandle, &point);

                if (PhGetSelectedListViewItemParams(context->ListViewHandle, &listviewItems, &numberOfItems))
                {
                    menu = PhCreateEMenu();
                    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, IDC_COPY, L"&Copy", NULL, NULL), ULONG_MAX);
                    PhInsertCopyListViewEMenuItem(menu, IDC_COPY, context->ListViewHandle);

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
                        BOOLEAN handled = FALSE;

                        handled = PhHandleCopyListViewEMenuItem(item);

                        //if (!handled && PhPluginsEnabled)
                        //    handled = PhPluginTriggerEMenuItem(&menuInfo, item);

                        if (!handled)
                        {
                            switch (item->Id)
                            {
                            case IDC_COPY:
                                {
                                    PhCopyListView(context->ListViewHandle);
                                }
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
    case WM_SIZE:
        {
            PhLayoutManagerLayout(&context->LayoutManager);
        }
        break;
    case WM_CTLCOLORBTN:
        return HANDLE_WM_CTLCOLORBTN(hwndDlg, wParam, lParam, PhWindowThemeControlColor);
    case WM_CTLCOLORDLG:
        return HANDLE_WM_CTLCOLORDLG(hwndDlg, wParam, lParam, PhWindowThemeControlColor);
    case WM_CTLCOLORSTATIC:
        return HANDLE_WM_CTLCOLORSTATIC(hwndDlg, wParam, lParam, PhWindowThemeControlColor);
    }

    return FALSE;
}

