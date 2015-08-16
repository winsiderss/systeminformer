/*
 * Process Hacker -
 *   session shadow configuration
 *
 * Copyright (C) 2011-2013 wj32
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
#include <settings.h>
#include <windowsx.h>
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
    if (SessionId == NtCurrentPeb()->SessionId)
    {
        PhShowError(ParentWindowHandle, L"You cannot remote control the current session.");
        return;
    }

    DialogBoxParam(
        PhInstanceHandle,
        MAKEINTRESOURCE(IDD_SHADOWSESSION),
        ParentWindowHandle,
        PhpSessionShadowDlgProc,
        (LPARAM)SessionId
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

            SetProp(hwndDlg, L"SessionId", UlongToHandle((ULONG)lParam));
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
            RemoveProp(hwndDlg, L"SessionId");
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
                    ULONG sessionId = HandleToUlong(GetProp(hwndDlg, L"SessionId"));
                    ULONG virtualKey;
                    ULONG modifiers;
                    WCHAR computerName[64];
                    ULONG computerNameLength = 64;

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

                    if (GetComputerName(computerName, &computerNameLength))
                    {
                        if (WinStationShadow(NULL, computerName, sessionId, (UCHAR)virtualKey, (USHORT)modifiers))
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
                    }
                    else
                    {
                        PhShowError(hwndDlg, L"The computer name is too long.");
                    }
                }
                break;
            }
        }
        break;
    }

    return FALSE;
}
