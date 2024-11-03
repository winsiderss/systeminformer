/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2010-2013
 *     dmex    2015-2024
 *
 */

#include <phapp.h>
#include <phsettings.h>
#include <settings.h>
#include <vsstyle.h>
#include <vssym32.h>

typedef struct _PH_CHOICE_DIALOG_CONTEXT
{
    PCWSTR Title;
    PCWSTR Message;
    PCWSTR *Choices;
    ULONG NumberOfChoices;
    PCWSTR Option;
    ULONG Flags;
    PPH_STRING *SelectedChoice;
    PBOOLEAN SelectedOption;
    PCWSTR SavedChoicesSettingName;

    HWND ComboBoxHandle;
} PH_CHOICE_DIALOG_CONTEXT, *PPH_CHOICE_DIALOG_CONTEXT;

INT_PTR CALLBACK PhpChoiceDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

/**
 * Prompts the user for input.
 *
 * \remarks If \c PH_CHOICE_DIALOG_PASSWORD is specified, the string
 * returned in \a SelectedChoice is NOT auto-dereferenced.
 */
BOOLEAN PhaChoiceDialog(
    _In_ HWND ParentWindowHandle,
    _In_ PCWSTR Title,
    _In_ PCWSTR Message,
    _In_opt_ PCWSTR*Choices,
    _In_opt_ ULONG NumberOfChoices,
    _In_opt_ PCWSTR Option,
    _In_ ULONG Flags,
    _Inout_ PPH_STRING *SelectedChoice,
    _Inout_opt_ PBOOLEAN SelectedOption,
    _In_opt_ PCWSTR SavedChoicesSettingName
    )
{
    PH_CHOICE_DIALOG_CONTEXT context;

    memset(&context, 0, sizeof(PH_CHOICE_DIALOG_CONTEXT));
    context.Title = Title;
    context.Message = Message;
    context.Choices = Choices;
    context.NumberOfChoices = NumberOfChoices;
    context.Option = Option;
    context.Flags = Flags;
    context.SelectedChoice = SelectedChoice;
    context.SelectedOption = SelectedOption;
    context.SavedChoicesSettingName = SavedChoicesSettingName;

    return PhDialogBox(
        PhInstanceHandle,
        MAKEINTRESOURCE(IDD_CHOOSE),
        ParentWindowHandle,
        PhpChoiceDlgProc,
        &context
        ) == IDOK;
}

INT_PTR CALLBACK PhpChoiceDlgProc(
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
            PPH_CHOICE_DIALOG_CONTEXT context = (PPH_CHOICE_DIALOG_CONTEXT)lParam;
            ULONG type;
            SIZE_T i;
            HWND comboBoxHandle;
            HWND checkBoxHandle;
            RECT checkBoxRect;
            RECT rect;
            ULONG diff;

            PhSetWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT, context);
            PhCenterWindow(hwndDlg, GetParent(hwndDlg));

            PhSetWindowText(hwndDlg, context->Title);
            PhSetWindowText(GetDlgItem(hwndDlg, IDC_MESSAGE), context->Message);

            type = context->Flags & PH_CHOICE_DIALOG_TYPE_MASK;

            // Select the control to show, depending on the type. This is
            // because it is impossible to change the style of the combo box
            // after it is created.
            switch (type)
            {
            case PH_CHOICE_DIALOG_USER_CHOICE:
                comboBoxHandle = GetDlgItem(hwndDlg, IDC_CHOICEUSER);
                ShowWindow(GetDlgItem(hwndDlg, IDC_CHOICEUSER), SW_SHOW);
                break;
            case PH_CHOICE_DIALOG_PASSWORD:
                comboBoxHandle = GetDlgItem(hwndDlg, IDC_CHOICESIMPLE);
                ShowWindow(GetDlgItem(hwndDlg, IDC_CHOICESIMPLE), SW_SHOW);

                // Disable combo box features since it isn't a combo box.
                context->SavedChoicesSettingName = NULL;
                break;
            case PH_CHOICE_DIALOG_CHOICE:
            default:
                comboBoxHandle = GetDlgItem(hwndDlg, IDC_CHOICE);
                ShowWindow(GetDlgItem(hwndDlg, IDC_CHOICE), SW_SHOW);
                break;
            }

            context->ComboBoxHandle = comboBoxHandle;

            checkBoxHandle = GetDlgItem(hwndDlg, IDC_OPTION);

            if (type == PH_CHOICE_DIALOG_PASSWORD)
            {
                // Nothing
            }
            else if (type == PH_CHOICE_DIALOG_USER_CHOICE && context->SavedChoicesSettingName)
            {
                PPH_STRING savedChoices = PhGetStringSetting(context->SavedChoicesSettingName);
                ULONG_PTR indexOfDelim;
                PPH_STRING savedChoice;

                i = 0;

                // Split the saved choices using the delimiter.
                while (i < savedChoices->Length / sizeof(WCHAR))
                {
                    // BUG BUG BUG - what if the user saves "\s"?
                    indexOfDelim = PhFindStringInString(savedChoices, i, L"\\s");

                    if (indexOfDelim == SIZE_MAX)
                        indexOfDelim = savedChoices->Length / sizeof(WCHAR);

                    savedChoice = PhSubstring(savedChoices, i, indexOfDelim - i);

                    if (savedChoice->Length != 0)
                    {
                        PPH_STRING unescaped;

                        unescaped = PhUnescapeStringForDelimiter(savedChoice, L'\\');
                        ComboBox_InsertString(comboBoxHandle, -1, unescaped->Buffer);
                        PhDereferenceObject(unescaped);
                    }

                    PhDereferenceObject(savedChoice);

                    i = indexOfDelim + 2;
                }

                PhDereferenceObject(savedChoices);
            }
            else
            {
                for (i = 0; i < context->NumberOfChoices; i++)
                {
                    ComboBox_AddString(comboBoxHandle, context->Choices[i]);
                }

                context->SavedChoicesSettingName = NULL; // make sure we don't try to save the choices
            }

            if (type == PH_CHOICE_DIALOG_PASSWORD)
            {
                if (*context->SelectedChoice)
                    PhSetWindowText(comboBoxHandle, (*context->SelectedChoice)->Buffer);

                Edit_SetSel(comboBoxHandle, 0, -1);
            }
            else if (type == PH_CHOICE_DIALOG_USER_CHOICE || type == PH_CHOICE_DIALOG_CHOICE)
            {
                // If we failed to choose a default choice based on what was specified,
                // select the first one if possible, or set the text directly.
                if (!(*context->SelectedChoice) || PhSelectComboBoxString(
                    comboBoxHandle, (*context->SelectedChoice)->Buffer, FALSE) == CB_ERR)
                {
                    if (type == PH_CHOICE_DIALOG_USER_CHOICE && *context->SelectedChoice)
                    {
                        PhSetWindowText(comboBoxHandle, (*context->SelectedChoice)->Buffer);
                    }
                    else if (type == PH_CHOICE_DIALOG_CHOICE && context->NumberOfChoices != 0)
                    {
                        ComboBox_SetCurSel(comboBoxHandle, 0);
                    }
                }

                if (type == PH_CHOICE_DIALOG_USER_CHOICE)
                    ComboBox_SetEditSel(comboBoxHandle, 0, -1);
            }

            if (context->Option)
            {
                PhSetWindowText(checkBoxHandle, context->Option);

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

            PhInitializeWindowTheme(hwndDlg, PhEnableThemeSupport);

            PhSetDialogFocus(hwndDlg, comboBoxHandle);
        }
        break;
    case WM_DESTROY:
        {
            PhRemoveWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT);
        }
        break;
    case WM_COMMAND:
        {
            switch (GET_WM_COMMAND_ID(wParam, lParam))
            {
            case IDCANCEL:
                EndDialog(hwndDlg, IDCANCEL);
                break;
            case IDOK:
                {
                    PPH_CHOICE_DIALOG_CONTEXT context = PhGetWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT);
                    PPH_STRING selectedChoice;

                    if ((context->Flags & PH_CHOICE_DIALOG_TYPE_MASK) != PH_CHOICE_DIALOG_PASSWORD)
                    {
                        selectedChoice = PH_AUTO(PhGetWindowText(context->ComboBoxHandle));
                        *context->SelectedChoice = selectedChoice;
                    }
                    else
                    {
                        // Password values are never auto-dereferenced.
                        selectedChoice = PhGetWindowText(context->ComboBoxHandle);
                        *context->SelectedChoice = selectedChoice;
                    }

                    if (context->Option && context->SelectedOption)
                        *context->SelectedOption = Button_GetCheck(GetDlgItem(hwndDlg, IDC_OPTION)) == BST_CHECKED;

                    if (context->SavedChoicesSettingName)
                    {
                        PH_STRING_BUILDER savedChoices;
                        ULONG i;
                        ULONG choicesToSave = PH_CHOICE_DIALOG_SAVED_CHOICES;
                        PPH_STRING choice;
                        PPH_STRING escaped;

                        PhInitializeStringBuilder(&savedChoices, 100);

                        // Push the selected choice to the top, then save the others.

                        if (selectedChoice->Length != 0)
                        {
                            escaped = PhEscapeStringForDelimiter(selectedChoice, L'\\');
                            PhAppendStringBuilder(&savedChoices, &escaped->sr);
                            PhDereferenceObject(escaped);
                            PhAppendStringBuilder2(&savedChoices, L"\\s");
                        }

                        for (i = 1; i < choicesToSave; i++)
                        {
                            choice = PhGetComboBoxString(context->ComboBoxHandle, i - 1);

                            if (!choice)
                                break;

                            // Don't save the choice if it's the same as the one
                            // entered by the user (since we already saved it above).
                            if (PhEqualString(choice, selectedChoice, FALSE))
                            {
                                PhDereferenceObject(choice);
                                choicesToSave++; // useless for now, but may be needed in the future
                                continue;
                            }

                            escaped = PhEscapeStringForDelimiter(choice, L'\\');
                            PhAppendStringBuilder(&savedChoices, &escaped->sr);
                            PhDereferenceObject(escaped);
                            PhDereferenceObject(choice);

                            PhAppendStringBuilder2(&savedChoices, L"\\s");
                        }

                        if (PhEndsWithString2(savedChoices.String, L"\\s", FALSE))
                            PhRemoveEndStringBuilder(&savedChoices, 2);

                        PhSetStringSetting2(context->SavedChoicesSettingName, &savedChoices.String->sr);
                        PhDeleteStringBuilder(&savedChoices);
                    }

                    EndDialog(hwndDlg, IDOK);
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

INT_PTR CALLBACK PhChooseNewPageDlgProc(
    _In_ HWND WindowHandle,
    _In_ UINT WindowMessage,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    switch (WindowMessage)
    {
    case WM_INITDIALOG:
        {
            PPH_CHOICE_DIALOG_CONTEXT context = (PPH_CHOICE_DIALOG_CONTEXT)lParam;
            HFONT fontHandle;
            NONCLIENTMETRICS metrics = { sizeof(NONCLIENTMETRICS) };
            LONG dpi = PhGetWindowDpi(WindowHandle);

            context->ComboBoxHandle = GetDlgItem(WindowHandle, IDC_INPUT);

            PhSetWindowContext(WindowHandle, PH_WINDOW_CONTEXT_DEFAULT, context);
            PhCenterWindow(WindowHandle, GetParent(WindowHandle));
            PhSetApplicationWindowIcon(WindowHandle);

            PhSetWindowText(WindowHandle, PhApplicationName);
            PhSetWindowText(GetDlgItem(WindowHandle, IDC_TITLE), context->Title);
            PhSetWindowText(GetDlgItem(WindowHandle, IDC_TEXT), context->Message);

            {
                if (PhGetSystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(metrics), &metrics, dpi))
                {
                    metrics.lfMessageFont.lfHeight = 4 * metrics.lfMessageFont.lfHeight / 3;

                    if (fontHandle = CreateFontIndirect(&metrics.lfMessageFont))
                    {
                        SetWindowFont(GetDlgItem(WindowHandle, IDC_TITLE), fontHandle, FALSE);
                    }

                    metrics.lfMessageFont.lfHeight = -14;

                    if (fontHandle = CreateFontIndirect(&metrics.lfMessageFont))
                    {
                        SetWindowFont(context->ComboBoxHandle, fontHandle, FALSE);
                    }
                }
            }

            {
                if ((context->Flags & PH_CHOICE_DIALOG_TYPE_MASK) == PH_CHOICE_DIALOG_PASSWORD)
                {
                    NOTHING;
                }
                else if ((context->Flags & PH_CHOICE_DIALOG_TYPE_MASK) == PH_CHOICE_DIALOG_USER_CHOICE && context->SavedChoicesSettingName)
                {
                    PPH_STRING savedChoices = PhGetStringSetting(context->SavedChoicesSettingName);
                    PPH_STRING savedChoice;
                    ULONG_PTR indexOfDelim;
                    ULONG_PTR i = 0;

                    // Split the saved choices using the delimiter.
                    while (i < savedChoices->Length / sizeof(WCHAR))
                    {
                        // BUG BUG BUG - what if the user saves "\s"?
                        indexOfDelim = PhFindStringInString(savedChoices, i, L"\\s");

                        if (indexOfDelim == SIZE_MAX)
                            indexOfDelim = savedChoices->Length / sizeof(WCHAR);

                        savedChoice = PhSubstring(savedChoices, i, indexOfDelim - i);

                        if (savedChoice->Length)
                        {
                            PPH_STRING unescaped;

                            unescaped = PhUnescapeStringForDelimiter(savedChoice, L'\\');
                            ComboBox_InsertString(context->ComboBoxHandle, UINT_MAX, unescaped->Buffer);
                            PhDereferenceObject(unescaped);
                        }

                        PhDereferenceObject(savedChoice);

                        i = indexOfDelim + 2;
                    }

                    PhDereferenceObject(savedChoices);
                }
                else
                {
                    for (ULONG i = 0; i < context->NumberOfChoices; i++)
                    {
                        ComboBox_AddString(context->ComboBoxHandle, context->Choices[i]);
                    }

                    context->SavedChoicesSettingName = NULL; // make sure we don't try to save the choices
                }
            }

            PhSetDialogFocus(WindowHandle, context->ComboBoxHandle);
        }
        break;
    case WM_DESTROY:
        {
            PhRemoveWindowContext(WindowHandle, PH_WINDOW_CONTEXT_DEFAULT);
        }
        break;
    case WM_COMMAND:
        {
            switch (GET_WM_COMMAND_ID(wParam, lParam))
            {
            case IDCANCEL:
                {
                    EndDialog(WindowHandle, IDCANCEL);
                }
                break;
            case IDOK:
                {
                    PPH_CHOICE_DIALOG_CONTEXT context = PhGetWindowContext(WindowHandle, PH_WINDOW_CONTEXT_DEFAULT);
                    PPH_STRING selectedChoice;

                    if ((context->Flags & PH_CHOICE_DIALOG_TYPE_MASK) != PH_CHOICE_DIALOG_PASSWORD)
                    {
                        selectedChoice = PH_AUTO(PhGetWindowText(context->ComboBoxHandle));
                        *context->SelectedChoice = selectedChoice;
                    }
                    else
                    {
                        // Password values are never auto-dereferenced.
                        selectedChoice = PhGetWindowText(context->ComboBoxHandle);
                        *context->SelectedChoice = selectedChoice;
                    }

                    if (context->SavedChoicesSettingName)
                    {
                        PH_STRING_BUILDER savedChoices;
                        ULONG i;
                        ULONG choicesToSave = PH_CHOICE_DIALOG_SAVED_CHOICES;
                        PPH_STRING choice;
                        PPH_STRING escaped;

                        PhInitializeStringBuilder(&savedChoices, 100);

                        // Push the selected choice to the top, then save the others.

                        if (selectedChoice->Length != 0)
                        {
                            escaped = PhEscapeStringForDelimiter(selectedChoice, L'\\');
                            PhAppendStringBuilder(&savedChoices, &escaped->sr);
                            PhDereferenceObject(escaped);
                            PhAppendStringBuilder2(&savedChoices, L"\\s");
                        }

                        for (i = 1; i < choicesToSave; i++)
                        {
                            choice = PhGetComboBoxString(context->ComboBoxHandle, i - 1);

                            if (!choice)
                                break;

                            // Don't save the choice if it's the same as the one
                            // entered by the user (since we already saved it above).
                            if (PhEqualString(choice, selectedChoice, FALSE))
                            {
                                PhDereferenceObject(choice);
                                choicesToSave++; // useless for now, but may be needed in the future
                                continue;
                            }

                            escaped = PhEscapeStringForDelimiter(choice, L'\\');
                            PhAppendStringBuilder(&savedChoices, &escaped->sr);
                            PhDereferenceObject(escaped);
                            PhDereferenceObject(choice);

                            PhAppendStringBuilder2(&savedChoices, L"\\s");
                        }

                        if (PhEndsWithString2(savedChoices.String, L"\\s", FALSE))
                            PhRemoveEndStringBuilder(&savedChoices, 2);

                        PhSetStringSetting2(context->SavedChoicesSettingName, &savedChoices.String->sr);
                        PhDeleteStringBuilder(&savedChoices);
                    }

                    EndDialog(WindowHandle, IDOK);
                }
                break;
            }
        }
        break;
    case WM_ERASEBKGND:
        {
            LONG dpi = PhGetWindowDpi(WindowHandle);
            HDC hdc = (HDC)wParam;
            RECT clientRect;

            if (!GetClientRect(WindowHandle, &clientRect))
                break;

            SetBkMode(hdc, TRANSPARENT);

            clientRect.bottom -= PhGetDpi(50, dpi);
            FillRect(hdc, &clientRect, PhEnableThemeSupport ? PhThemeWindowBackgroundBrush : GetSysColorBrush(COLOR_WINDOW));

            clientRect.top = clientRect.bottom;
            clientRect.bottom = clientRect.top + PhGetDpi(50, dpi);
            FillRect(hdc, &clientRect, PhEnableThemeSupport ? PhThemeWindowBackgroundBrush : GetSysColorBrush(COLOR_3DFACE));

            SetWindowLongPtr(WindowHandle, DWLP_MSGRESULT, TRUE);
        }
        return TRUE;
    case WM_CTLCOLORSTATIC:
        {
            HDC hdc = (HDC)wParam;

            if (PhEnableThemeSupport)
                break;

            SetBkMode(hdc, TRANSPARENT);

            if ((HWND)lParam == GetDlgItem(WindowHandle, IDC_TITLE))
            {
                static COLORREF color = 0;

                if (color == 0)
                {
                    HTHEME theme;

                    if (theme = PhOpenThemeData(NULL, VSCLASS_TASKDIALOG, 0))
                    {
                        PhGetThemeColor(theme, TDLG_MAININSTRUCTIONPANE, 0, TMT_TEXTCOLOR, &color);
                        PhCloseThemeData(theme);
                    }
                }

                SetTextColor(hdc, color);
            }

            return (INT_PTR)PhGetStockBrush(WHITE_BRUSH);
        }
        break;
    }

    return FALSE;
}

BOOLEAN PhChoiceDialog(
    _In_ HWND ParentWindowHandle,
    _In_ PCWSTR Title,
    _In_ PCWSTR Message,
    _In_opt_ PCWSTR*Choices,
    _In_opt_ ULONG NumberOfChoices,
    _In_opt_ PCWSTR Option,
    _In_ ULONG Flags,
    _Inout_ PPH_STRING *SelectedChoice,
    _Inout_opt_ PBOOLEAN SelectedOption,
    _In_opt_ PCWSTR SavedChoicesSettingName
    )
{
    PH_CHOICE_DIALOG_CONTEXT context;

    memset(&context, 0, sizeof(PH_CHOICE_DIALOG_CONTEXT));
    context.Title = Title;
    context.Message = Message;
    context.Choices = Choices;
    context.NumberOfChoices = NumberOfChoices;
    context.Option = Option;
    context.Flags = Flags;
    context.SelectedChoice = SelectedChoice;
    context.SelectedOption = SelectedOption;
    context.SavedChoicesSettingName = SavedChoicesSettingName;

    return PhDialogBox(
        PhInstanceHandle,
        MAKEINTRESOURCE(IDD_CHOOSENEW),
        PhCsForceNoParent ? NULL : ParentWindowHandle,
        PhChooseNewPageDlgProc,
        &context
        ) == IDOK;
}
