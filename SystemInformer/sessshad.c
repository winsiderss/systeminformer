/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2011-2016
 *     dmex    2017-2023
 */

#include <phapp.h>
#include <settings.h>
#include <winsta.h>

#define SIP(String, Integer) { (String), (PVOID)(Integer) }

static PH_KEY_VALUE_PAIR VirtualKeyPairs[] =
{
    SIP(L"0", '0'),
    SIP(L"1", '1'),
    SIP(L"2", '2'),
    SIP(L"3", '3'),
    SIP(L"4", '4'),
    SIP(L"5", '5'),
    SIP(L"6", '6'),
    SIP(L"7", '7'),
    SIP(L"8", '8'),
    SIP(L"9", '9'),
    SIP(L"A", 'A'),
    SIP(L"B", 'B'),
    SIP(L"C", 'C'),
    SIP(L"D", 'D'),
    SIP(L"E", 'E'),
    SIP(L"F", 'F'),
    SIP(L"G", 'G'),
    SIP(L"H", 'H'),
    SIP(L"I", 'I'),
    SIP(L"J", 'J'),
    SIP(L"K", 'K'),
    SIP(L"L", 'L'),
    SIP(L"M", 'M'),
    SIP(L"N", 'N'),
    SIP(L"O", 'O'),
    SIP(L"P", 'P'),
    SIP(L"Q", 'Q'),
    SIP(L"R", 'R'),
    SIP(L"S", 'S'),
    SIP(L"T", 'T'),
    SIP(L"U", 'U'),
    SIP(L"V", 'V'),
    SIP(L"W", 'W'),
    SIP(L"X", 'X'),
    SIP(L"Y", 'Y'),
    SIP(L"Z", 'Z'),
    SIP(L"{backspace}", VK_BACK),
    SIP(L"{delete}", VK_DELETE),
    SIP(L"{down}", VK_DOWN),
    SIP(L"{end}", VK_END),
    SIP(L"{enter}", VK_RETURN),
    SIP(L"{F2}", VK_F2),
    SIP(L"{F3}", VK_F3),
    SIP(L"{F4}", VK_F4),
    SIP(L"{F5}", VK_F5),
    SIP(L"{F6}", VK_F6),
    SIP(L"{F7}", VK_F7),
    SIP(L"{F8}", VK_F8),
    SIP(L"{F9}", VK_F9),
    SIP(L"{F10}", VK_F10),
    SIP(L"{F11}", VK_F11),
    SIP(L"{F12}", VK_F12),
    SIP(L"{home}", VK_HOME),
    SIP(L"{insert}", VK_INSERT),
    SIP(L"{left}", VK_LEFT),
    SIP(L"{-}", VK_SUBTRACT),
    SIP(L"{pagedown}", VK_NEXT),
    SIP(L"{pageup}", VK_PRIOR),
    SIP(L"{+}", VK_ADD),
    SIP(L"{prtscrn}", VK_SNAPSHOT),
    SIP(L"{right}", VK_RIGHT),
    SIP(L"{spacebar}", VK_SPACE),
    SIP(L"{*}", VK_MULTIPLY),
    SIP(L"{tab}", VK_TAB),
    SIP(L"{up}", VK_UP)
};

INT_PTR CALLBACK PhpSessionShadowDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

VOID PhShowSessionShadowDialog(
    _In_ HWND ParentWindowHandle,
    _In_ ULONG SessionId
    )
{
    ULONG sessionId = ULONG_MAX;

    PhGetProcessSessionId(NtCurrentProcess(), &sessionId);

    if (SessionId == sessionId)
    {
        PhShowError2(ParentWindowHandle, L"Unable to shadow session.", L"%s", L"You cannot remote control the current session.");
        return;
    }

    PhDialogBox(
        PhInstanceHandle,
        MAKEINTRESOURCE(IDD_SHADOWSESSION),
        ParentWindowHandle,
        PhpSessionShadowDlgProc,
        UlongToPtr(SessionId)
        );
}

INT_PTR CALLBACK PhpSessionShadowDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            HWND virtualKeyComboBox;
            PH_INTEGER_PAIR hotkey;
            ULONG i;
            PWSTR stringToSelect;

            PhSetWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT, UlongToPtr((ULONG)lParam));

            PhCenterWindow(hwndDlg, GetParent(hwndDlg));

            hotkey = PhGetIntegerPairSetting(L"SessionShadowHotkey");

            // Set up the hotkeys.

            virtualKeyComboBox = GetDlgItem(hwndDlg, IDC_VIRTUALKEY);
            stringToSelect = L"{*}";

            for (i = 0; i < sizeof(VirtualKeyPairs) / sizeof(PH_KEY_VALUE_PAIR); i++)
            {
                ComboBox_AddString(virtualKeyComboBox, VirtualKeyPairs[i].Key);

                if (PtrToUlong(VirtualKeyPairs[i].Value) == (ULONG)hotkey.X)
                {
                    stringToSelect = VirtualKeyPairs[i].Key;
                }
            }

            PhSelectComboBoxString(virtualKeyComboBox, stringToSelect, FALSE);

            // Set up the modifiers.

            Button_SetCheck(GetDlgItem(hwndDlg, IDC_SHIFT), hotkey.Y & KBDSHIFT);
            Button_SetCheck(GetDlgItem(hwndDlg, IDC_CTRL), hotkey.Y & KBDCTRL);
            Button_SetCheck(GetDlgItem(hwndDlg, IDC_ALT), hotkey.Y & KBDALT);
        }
        break;
    case WM_DESTROY:
        {
            PhRemoveWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT);
        }
        break;
    case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
            case IDCANCEL:
                EndDialog(hwndDlg, IDCANCEL);
                break;
            case IDOK:
                {
                    ULONG sessionId = PtrToUlong(PhGetWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT));
                    ULONG virtualKey;
                    ULONG modifiers;
                    PPH_STRING computerName;

                    virtualKey = VK_MULTIPLY;
                    PhFindIntegerSiKeyValuePairs(
                        VirtualKeyPairs,
                        sizeof(VirtualKeyPairs),
                        PhaGetDlgItemText(hwndDlg, IDC_VIRTUALKEY)->Buffer,
                        &virtualKey
                        );

                    modifiers = 0;

                    if (Button_GetCheck(GetDlgItem(hwndDlg, IDC_SHIFT)) == BST_CHECKED)
                        modifiers |= KBDSHIFT;
                    if (Button_GetCheck(GetDlgItem(hwndDlg, IDC_CTRL)) == BST_CHECKED)
                        modifiers |= KBDCTRL;
                    if (Button_GetCheck(GetDlgItem(hwndDlg, IDC_ALT)) == BST_CHECKED)
                        modifiers |= KBDALT;

                    if (computerName = PhGetActiveComputerName())
                    {
                        if (WinStationShadow(NULL, PhGetString(computerName), sessionId, (UCHAR)virtualKey, (USHORT)modifiers))
                        {
                            PH_INTEGER_PAIR hotkey;

                            hotkey.X = virtualKey;
                            hotkey.Y = modifiers;
                            PhSetIntegerPairSetting(L"SessionShadowHotkey", hotkey);

                            EndDialog(hwndDlg, IDOK);
                        }
                        else
                        {
                            PhShowStatus(hwndDlg, L"Unable to remote control the session", 0, GetLastError());
                        }

                        PhDereferenceObject(computerName);
                    }
                    else
                    {
                        PhShowError(hwndDlg, L"%s", L"The computer name is too long.");
                    }
                }
                break;
            }
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
