/*
 * Process Hacker - 
 *   choice dialog
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
#include <settings.h>
#include <windowsx.h>

typedef struct _CHOICE_DIALOG_CONTEXT
{
    PWSTR Title;
    PWSTR *Choices;
    ULONG NumberOfChoices;
    PWSTR Option;
    ULONG Flags;
    PPH_STRING *SelectedChoice;
    PBOOLEAN SelectedOption;
    PWSTR SavedChoicesSettingName;

    HWND ComboBoxHandle;
} CHOICE_DIALOG_CONTEXT, *PCHOICE_DIALOG_CONTEXT;

INT_PTR CALLBACK PhpChoiceDlgProc(
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    );

BOOLEAN PhaChoiceDialog(
    __in HWND ParentWindowHandle,
    __in PWSTR Title,
    __in_opt PWSTR *Choices,
    __in_opt ULONG NumberOfChoices,
    __in_opt PWSTR Option,
    __in ULONG Flags,
    __inout PPH_STRING *SelectedChoice,
    __inout_opt PBOOLEAN SelectedOption,
    __in_opt PWSTR SavedChoicesSettingName
    )
{
    CHOICE_DIALOG_CONTEXT context;

    context.Title = Title;
    context.Choices = Choices;
    context.NumberOfChoices = NumberOfChoices;
    context.Option = Option;
    context.Flags = Flags;
    context.SelectedChoice = SelectedChoice;
    context.SelectedOption = SelectedOption;
    context.SavedChoicesSettingName = SavedChoicesSettingName;

    return DialogBoxParam(
        PhInstanceHandle,
        MAKEINTRESOURCE(IDD_CHOOSE),
        ParentWindowHandle,
        PhpChoiceDlgProc,
        (LPARAM)&context
        ) == IDOK;
}

INT_PTR CALLBACK PhpChoiceDlgProc(
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
            PCHOICE_DIALOG_CONTEXT context = (PCHOICE_DIALOG_CONTEXT)lParam;
            ULONG i;
            HWND comboBoxHandle;
            HWND checkBoxHandle;
            RECT checkBoxRect;
            RECT rect;
            ULONG diff;

            SetProp(hwndDlg, L"Context", (HANDLE)context);
            PhCenterWindow(hwndDlg, GetParent(hwndDlg));

            SetWindowText(hwndDlg, context->Title);

            // Select the combo box to show, depending on whether we allow 
            // the user to enter a choice. This is because it is impossible to 
            // change the style of the combo box after it is created.
            if (!(context->Flags & PH_CHOICE_DIALOG_USER_CHOICE))
            {
                comboBoxHandle = GetDlgItem(hwndDlg, IDC_CHOICE);
                ShowWindow(GetDlgItem(hwndDlg, IDC_CHOICEUSER), SW_HIDE);
            }
            else
            {
                comboBoxHandle = GetDlgItem(hwndDlg, IDC_CHOICEUSER);
                ShowWindow(GetDlgItem(hwndDlg, IDC_CHOICE), SW_HIDE);
            }

            context->ComboBoxHandle = comboBoxHandle;

            checkBoxHandle = GetDlgItem(hwndDlg, IDC_OPTION);

            // If we're allowing user choices and a setting name was specified, load 
            // the saved choices.
            if (!((context->Flags & PH_CHOICE_DIALOG_USER_CHOICE) && context->SavedChoicesSettingName))
            {
                for (i = 0; i < context->NumberOfChoices; i++)
                {
                    ComboBox_AddString(comboBoxHandle, context->Choices[i]);
                }

                context->SavedChoicesSettingName = NULL; // make sure we don't try to save the choices
            }
            else
            {
                PPH_STRING savedChoices = PhGetStringSetting(context->SavedChoicesSettingName);
                ULONG i;
                ULONG indexOfNewLine;
                PPH_STRING savedChoice;

                i = 0;

                // Split the saved choices using the newline character.
                while (i < (ULONG)savedChoices->Length / 2)
                {
                    indexOfNewLine = PhStringIndexOfChar(savedChoices, i, '\n');

                    if (indexOfNewLine == -1)
                        indexOfNewLine = savedChoices->Length / 2;

                    savedChoice = PhSubstring(savedChoices, i, indexOfNewLine);
                    ComboBox_AddString(comboBoxHandle, savedChoice->Buffer);
                    PhDereferenceObject(savedChoice);

                    i = indexOfNewLine + 1;
                }

                PhDereferenceObject(savedChoices);
            }

            // If we failed to choose a default choice based on what was specified, 
            // select the first one if possible.
            if ((!(*context->SelectedChoice) || ComboBox_SelectString(
                comboBoxHandle, -1, (*context->SelectedChoice)->Buffer) == CB_ERR) &&
                !(context->Flags & PH_CHOICE_DIALOG_USER_CHOICE) && context->NumberOfChoices != 0)
            {
                ComboBox_SelectString(comboBoxHandle, -1, context->Choices[0]);
            }

            if (context->Option)
            {
                SetWindowText(checkBoxHandle, context->Option);

                if (context->SelectedOption)
                    Button_SetCheck(checkBoxHandle, *context->SelectedOption ? BST_CHECKED : BST_UNCHECKED);
            }
            else
            {
                // Hide the check box and move the buttons up.

                ShowWindow(checkBoxHandle, SW_HIDE);
                GetWindowRect(checkBoxHandle, &checkBoxRect);
                MapWindowPoints(NULL, hwndDlg, (POINT *)&checkBoxRect, 2);
                GetWindowRect(GetDlgItem(hwndDlg, IDOK), &rect);
                MapWindowPoints(NULL, hwndDlg, (POINT *)&rect, 2);
                diff = rect.top - checkBoxRect.top;

                // OK
                rect.top -= diff;
                rect.bottom -= diff;
                SetWindowPos(GetDlgItem(hwndDlg, IDOK), NULL, rect.left, rect.top,
                    rect.right - rect.left, rect.bottom - rect.top,
                    SWP_NOACTIVATE | SWP_NOZORDER);

                // Cancel
                GetWindowRect(GetDlgItem(hwndDlg, IDCANCEL), &rect);
                MapWindowPoints(NULL, hwndDlg, (POINT *)&rect, 2);
                rect.top -= diff;
                rect.bottom -= diff;
                SetWindowPos(GetDlgItem(hwndDlg, IDCANCEL), NULL, rect.left, rect.top,
                    rect.right - rect.left, rect.bottom - rect.top,
                    SWP_NOACTIVATE | SWP_NOZORDER);

                // Window
                GetWindowRect(hwndDlg, &rect);
                rect.bottom -= diff;
                SetWindowPos(hwndDlg, NULL, rect.left, rect.top,
                    rect.right - rect.left, rect.bottom - rect.top,
                    SWP_NOACTIVATE | SWP_NOZORDER);
            }

            SetFocus(comboBoxHandle);
        }
        break;
    case WM_DESTROY:
        {
            RemoveProp(hwndDlg, L"Context");
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
                    PCHOICE_DIALOG_CONTEXT context = (PCHOICE_DIALOG_CONTEXT)GetProp(hwndDlg, L"Context");
                    PPH_STRING selectedChoice;

                    selectedChoice = PHA_DEREFERENCE(PhGetWindowText(context->ComboBoxHandle));
                    *context->SelectedChoice = selectedChoice;

                    if (context->Option && context->SelectedOption)
                        *context->SelectedOption = Button_GetCheck(GetDlgItem(hwndDlg, IDC_OPTION)) == BST_CHECKED;

                    if (context->SavedChoicesSettingName)
                    {
                        PPH_STRING_BUILDER savedChoices;
                        ULONG i;
                        ULONG choicesToSave = PH_CHOICE_DIALOG_SAVED_CHOICES;
                        PPH_STRING choice;

                        savedChoices = PhCreateStringBuilder(100);

                        // Push the selected choice to the top, then save the others.

                        PhStringBuilderAppend(savedChoices, selectedChoice);
                        PhStringBuilderAppendChar(savedChoices, '\n');

                        for (i = 1; i < choicesToSave; i++)
                        {
                            choice = PhGetComboBoxString(context->ComboBoxHandle, i);

                            if (!choice)
                                break;

                            // Don't save the choice if it's the same as the one 
                            // entered by the user.
                            if (PhStringEquals(choice, selectedChoice, FALSE))
                            {
                                PhDereferenceObject(choice);
                                choicesToSave++; // useless for now, but may be needed in the future
                                continue;
                            }

                            PhStringBuilderAppend(savedChoices, choice);
                            PhDereferenceObject(choice);

                            PhStringBuilderAppendChar(savedChoices, '\n');
                        }

                        if (PhStringEndsWith2(savedChoices->String, L"\n", FALSE))
                            PhStringBuilderRemove(savedChoices, savedChoices->String->Length / 2 - 1, 1);

                        PhSetStringSetting2(context->SavedChoicesSettingName, &savedChoices->String->sr);
                        PhDereferenceObject(savedChoices);
                    }

                    EndDialog(hwndDlg, IDOK);
                }
                break;
            }
        }
        break;
    }

    return FALSE;
}
