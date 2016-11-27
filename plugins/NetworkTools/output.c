/*
 * Process Hacker Network Tools -
 *   output dialog
 *
 * Copyright (C) 2010-2015 wj32
 * Copyright (C) 2012-2015 dmex
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

#include "nettools.h"
#include <richedit.h>

VOID RichEditAppendText(
    _In_ HWND RichEditHandle,
    _In_ PWSTR Text
    )
{
    CHARFORMAT2 cf;

    memset(&cf, 0, sizeof(CHARFORMAT2));
    cf.cbSize = sizeof(CHARFORMAT2);
    //cf.dwMask = CFM_COLOR | CFM_EFFECTS;
    //cf.dwEffects = CFE_BOLD;
    //cf.crTextColor = RGB(0xff, 0xff, 0xff);
    //cf.crBackColor = RGB(0, 0, 0);

    SendMessage(RichEditHandle, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf);
    SendMessage(RichEditHandle, EM_REPLACESEL, FALSE, (LPARAM)Text);
}

INT_PTR CALLBACK NetworkOutputDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PNETWORK_OUTPUT_CONTEXT context;

    if (uMsg == WM_INITDIALOG)
    {
        context = (PNETWORK_OUTPUT_CONTEXT)lParam;
        SetProp(hwndDlg, L"Context", (HANDLE)context);
    }
    else
    {
        context = (PNETWORK_OUTPUT_CONTEXT)GetProp(hwndDlg, L"Context");

        if (uMsg == WM_DESTROY)
        {
            PhSaveWindowPlacementToSetting(SETTING_NAME_OUTPUT_WINDOW_POSITION, SETTING_NAME_OUTPUT_WINDOW_SIZE, hwndDlg);
            PhDeleteLayoutManager(&context->LayoutManager);
            RemoveProp(hwndDlg, L"Context");
            PhFree(context);

            PostQuitMessage(0);
        }
    }

    if (!context)
        return FALSE;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            PH_RECTANGLE windowRectangle;
            HANDLE dialogThread;

            context->WindowHandle = hwndDlg;
            context->WhoisHandle = GetDlgItem(hwndDlg, IDC_NETOUTPUTEDIT);

           

            //SendMessage(context->OutputHandle, EM_SETBKGNDCOLOR, RGB(0, 0, 0), 0);
            SendMessage(context->WhoisHandle, EM_SETEVENTMASK, 0, SendMessage(context->WhoisHandle, EM_GETEVENTMASK, 0, 0) | ENM_LINK);
            SendMessage(context->WhoisHandle, EM_AUTOURLDETECT, AURL_ENABLEURL, 0);
            SendMessage(context->WhoisHandle, EM_SETWORDWRAPMODE, WBF_WORDWRAP, 0);

            PhInitializeLayoutManager(&context->LayoutManager, hwndDlg);
            PhAddLayoutItem(&context->LayoutManager, context->WhoisHandle, NULL, PH_ANCHOR_ALL);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDCANCEL), NULL, PH_ANCHOR_BOTTOM | PH_ANCHOR_RIGHT);

            windowRectangle.Position = PhGetIntegerPairSetting(SETTING_NAME_OUTPUT_WINDOW_POSITION);
            windowRectangle.Size = PhGetScalableIntegerPairSetting(SETTING_NAME_OUTPUT_WINDOW_SIZE, TRUE).Pair;
            if (windowRectangle.Position.X != 0 || windowRectangle.Position.Y != 0)
                PhLoadWindowPlacementFromSetting(SETTING_NAME_OUTPUT_WINDOW_POSITION, SETTING_NAME_OUTPUT_WINDOW_SIZE, hwndDlg);
            else
                PhCenterWindow(hwndDlg, GetParent(hwndDlg));

            if (context->RemoteEndpoint.Address.Type == PH_IPV4_NETWORK_TYPE)
                RtlIpv4AddressToString(&context->RemoteEndpoint.Address.InAddr, context->IpAddressString);
            else
                RtlIpv6AddressToString(&context->RemoteEndpoint.Address.In6Addr, context->IpAddressString);

            SetWindowText(context->WindowHandle, PhaFormatString(L"Whois %s...", context->IpAddressString)->Buffer);
            RichEditAppendText(context->WhoisHandle, L"whois.iana.org found the following authoritative answer from: ");

            if (dialogThread = PhCreateThread(0, NetworkWhoisThreadStart, (PVOID)context))
                NtClose(dialogThread);
        }
        break;
    case WM_COMMAND:
        {
            switch (GET_WM_COMMAND_ID(wParam, lParam))
            {
            case IDCANCEL:
            case IDOK:
                DestroyWindow(hwndDlg);
                break;
            }
        }
        break;
    case WM_SIZE:
        PhLayoutManagerLayout(&context->LayoutManager);
        break;
    case NTM_RECEIVEDWHOIS:
        {
            PH_STRING_BUILDER receivedString;
            PPH_STRING convertedString = (PPH_STRING)lParam;

            PhInitializeStringBuilder(&receivedString, PAGE_SIZE);

            for (SIZE_T i = 0; i < convertedString->Length / sizeof(WCHAR); i++)
            {
                if (convertedString->Buffer[i] == '\n' && convertedString->Buffer[i + 1] == '\n')
                {
                    if (i < convertedString->Length - 2 && 
                        convertedString->Buffer[i] == '\n' &&
                        convertedString->Buffer[i + 1] == '\n' &&
                        convertedString->Buffer[i + 2] == '\n')
                    {
                        NOTHING;
                    }
                    else
                    {
                        PhAppendStringBuilder2(&receivedString, L"\r\n");
                    }
                }
                else
                {
                    PhAppendCharStringBuilder(&receivedString, convertedString->Buffer[i]);
                }
            }

            if (receivedString.String->Length >= 2 * 2 &&
                receivedString.String->Buffer[0] == '\n' &&
                receivedString.String->Buffer[1] == '\n')
            {
                PhRemoveStringBuilder(&receivedString, 0, 2);
            }

            RichEditAppendText(context->WhoisHandle, receivedString.String->Buffer);

            PhDeleteStringBuilder(&receivedString);
            PhDereferenceObject(convertedString);
        }
        break;
    case NTM_RECEIVEDFINISH:
        {
            PPH_STRING windowText = PH_AUTO(PhGetWindowText(context->WindowHandle));

            if (windowText)
            {
                Static_SetText(
                    context->WindowHandle, 
                    PhaFormatString(L"%s Finished.", windowText->Buffer)->Buffer
                    );
            }
        }
        break;
    }

    return FALSE;
}

NTSTATUS PhNetworkOutputDialogThreadStart(
    _In_ PVOID Parameter
    )
{
    static PH_INITONCE initOnce = PH_INITONCE_INIT;
    PNETWORK_OUTPUT_CONTEXT context;
    BOOL result;
    MSG message;
    HWND windowHandle;
    PH_AUTO_POOL autoPool;

    context = (PNETWORK_OUTPUT_CONTEXT)Parameter;

    if (PhBeginInitOnce(&initOnce))
    {
        LoadLibrary(L"msftedit.dll");
        PhEndInitOnce(&initOnce);
    }

    PhInitializeAutoPool(&autoPool);

    windowHandle = CreateDialogParam(
        (HINSTANCE)PluginInstance->DllBase,
        MAKEINTRESOURCE(IDD_OUTPUT),
        NULL,
        NetworkOutputDlgProc,
        (LPARAM)Parameter
        );

    ShowWindow(windowHandle, SW_SHOW);
    SetForegroundWindow(windowHandle);

    while (result = GetMessage(&message, NULL, 0, 0))
    {
        if (result == -1)
            break;

        if (!IsDialogMessage(context->WindowHandle, &message))
        {
            TranslateMessage(&message);
            DispatchMessage(&message);
        }

        PhDrainAutoPool(&autoPool);
    }

    PhDeleteAutoPool(&autoPool);

    return STATUS_SUCCESS;
}

VOID PerformNetworkAction(
    _In_ PH_NETWORK_ACTION Action,
    _In_ PPH_NETWORK_ITEM NetworkItem
    )
{
    HANDLE dialogThread = INVALID_HANDLE_VALUE;
    PNETWORK_OUTPUT_CONTEXT context;

    context = (PNETWORK_OUTPUT_CONTEXT)PhAllocate(sizeof(NETWORK_OUTPUT_CONTEXT));
    memset(context, 0, sizeof(NETWORK_OUTPUT_CONTEXT));

    context->Action = Action;
    context->RemoteEndpoint = NetworkItem->RemoteEndpoint;

    if (context->Action == NETWORK_ACTION_PING)
    {
        if (dialogThread = PhCreateThread(0, PhNetworkPingDialogThreadStart, (PVOID)context))
            NtClose(dialogThread);
    }
    else
    {
        if (dialogThread = PhCreateThread(0, PhNetworkOutputDialogThreadStart, (PVOID)context))
            NtClose(dialogThread);
    }
}

VOID PerformTracertAction(
    _In_ PH_NETWORK_ACTION Action,
    _In_ PH_IP_ENDPOINT RemoteEndpoint
    )
{
    HANDLE dialogThread = INVALID_HANDLE_VALUE;
    PNETWORK_OUTPUT_CONTEXT context;

    context = (PNETWORK_OUTPUT_CONTEXT)PhAllocate(sizeof(NETWORK_OUTPUT_CONTEXT));
    memset(context, 0, sizeof(NETWORK_OUTPUT_CONTEXT));

    context->Action = Action;
    context->RemoteEndpoint = RemoteEndpoint;

    if (context->Action == NETWORK_ACTION_PING)
    {
        if (dialogThread = PhCreateThread(0, PhNetworkPingDialogThreadStart, (PVOID)context))
            NtClose(dialogThread);
    }
    else
    {
        if (dialogThread = PhCreateThread(0, PhNetworkOutputDialogThreadStart, (PVOID)context))
            NtClose(dialogThread);
    }
}