/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2010-2013
 *     dmex    2015-2026
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
    HFONT TitleFontHandle;
    HFONT FontHandle;

    HBRUSH BrushNormal;
    HBRUSH BrushHover;
    HBRUSH BrushHot;
    COLORREF TextColor;
} PH_CHOICE_DIALOG_CONTEXT, *PPH_CHOICE_DIALOG_CONTEXT;

/**
 * Creates the brushes used for owner-drawn combo box items.
 *
 * \param Context The choice dialog context.
 */
static VOID PhpChoiceDialogInitializeBrushes(
    _Inout_ PPH_CHOICE_DIALOG_CONTEXT Context
    )
{
    if (PhEnableThemeSupport)
    {
        Context->BrushNormal = CreateSolidBrush(PhThemeWindowBackgroundColor);
        Context->BrushHover = CreateSolidBrush(PhThemeWindowBackground2Color);
        Context->BrushHot = CreateSolidBrush(PhThemeWindowHighlightColor);
        Context->TextColor = PhThemeWindowTextColor;
    }
    else
    {
        Context->BrushNormal = GetSysColorBrush(COLOR_WINDOW);
        Context->BrushHover = CreateSolidBrush(RGB(229, 243, 255));
        Context->BrushHot = CreateSolidBrush(RGB(145, 201, 247));
        Context->TextColor = GetSysColor(COLOR_WINDOWTEXT);
    }
}

/**
 * Deletes the brushes used for owner-drawn combo box items.
 *
 * \param Context The choice dialog context.
 */
static VOID PhpChoiceDialogDeleteBrushes(
    _Inout_ PPH_CHOICE_DIALOG_CONTEXT Context
    )
{
    if (Context->BrushNormal)
        DeleteBrush(Context->BrushNormal);
    if (Context->BrushHover)
        DeleteBrush(Context->BrushHover);
    if (Context->BrushHot)
        DeleteBrush(Context->BrushHot);
}

/**
 * Draws an owner-drawn combo box item with hot (mouse-over) and selection theming.
 *
 * \param Context The choice dialog context.
 * \param DrawInfo The owner-draw information.
 * \return TRUE if the item was drawn, FALSE to use default drawing.
 */
static BOOLEAN PhpChoiceDialogDrawComboBoxItem(
    _In_ PPH_CHOICE_DIALOG_CONTEXT Context,
    _In_ LPDRAWITEMSTRUCT DrawInfo
    )
{
    HDC bufferDc;
    HBITMAP bufferBitmap;
    HBITMAP oldBufferBitmap;
    PPH_STRING string;
    RECT bufferRect =
    {
        0, 0,
        DrawInfo->rcItem.right - DrawInfo->rcItem.left,
        DrawInfo->rcItem.bottom - DrawInfo->rcItem.top
    };
    BOOLEAN isSelected = (DrawInfo->itemState & ODS_SELECTED) == ODS_SELECTED;
    BOOLEAN isEditField = (DrawInfo->itemState & ODS_COMBOBOXEDIT) == ODS_COMBOBOXEDIT;

    if (DrawInfo->itemID == UINT_MAX)
    {
        // Empty selection field.
        FillRect(DrawInfo->hDC, &DrawInfo->rcItem, Context->BrushNormal);
        return TRUE;
    }

    string = PhGetComboBoxString(DrawInfo->hwndItem, DrawInfo->itemID);

    if (PhIsNullOrEmptyString(string))
    {
        PhClearReference(&string);
        return FALSE;
    }

    bufferDc = CreateCompatibleDC(DrawInfo->hDC);
    bufferBitmap = CreateCompatibleBitmap(DrawInfo->hDC, bufferRect.right, bufferRect.bottom);
    oldBufferBitmap = SelectBitmap(bufferDc, bufferBitmap);

    SelectFont(bufferDc, GetWindowFont(DrawInfo->hwndItem));
    SetBkMode(bufferDc, TRANSPARENT);

    if (isEditField)
    {
        // The selection field renders with the normal background.
        FillRect(bufferDc, &bufferRect, Context->BrushNormal);
    }
    else if (isSelected)
    {
        // The drop-down list highlights the item under the mouse as selected.
        FillRect(bufferDc, &bufferRect, Context->BrushHover);
    }
    else
    {
        FillRect(bufferDc, &bufferRect, Context->BrushNormal);
    }

    SetTextColor(bufferDc, Context->TextColor);

    bufferRect.left += 5;
    DrawText(
        bufferDc,
        string->Buffer,
        (UINT)string->Length / sizeof(WCHAR),
        &bufferRect,
        DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOCLIP
        );
    bufferRect.left -= 5;

    BitBlt(
        DrawInfo->hDC,
        DrawInfo->rcItem.left,
        DrawInfo->rcItem.top,
        DrawInfo->rcItem.right,
        DrawInfo->rcItem.bottom,
        bufferDc,
        0,
        0,
        SRCCOPY
        );

    SelectBitmap(bufferDc, oldBufferBitmap);
    DeleteBitmap(bufferBitmap);
    DeleteDC(bufferDc);
    PhClearReference(&string);

    return TRUE;
}

/**
 * Updates the fonts used in the choice dialog based on the current DPI value.
 *
 * \param WindowHandle The handle to the dialog window.
 * \param Context The choice dialog context.
 * \param DpiValue The current DPI value.
 */
static VOID PhpChoiceDialogUpdateFonts(
    _In_ HWND WindowHandle,
    _Inout_ PPH_CHOICE_DIALOG_CONTEXT Context,
    _In_ LONG DpiValue
    )
{
    NONCLIENTMETRICS metrics = { sizeof(NONCLIENTMETRICS) };

    if (PhGetSystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(metrics), &metrics, DpiValue))
    {
        HFONT fontHandle;

        metrics.lfMessageFont.lfHeight = 4 * metrics.lfMessageFont.lfHeight / 3;

        if (fontHandle = CreateFontIndirect(&metrics.lfMessageFont))
            PhSwapReferenceFont(&Context->TitleFontHandle, GetDlgItem(WindowHandle, IDC_TITLE), fontHandle, FALSE);

        metrics.lfMessageFont.lfHeight = PhScaleToDisplay(-14, DpiValue);

        if (fontHandle = CreateFontIndirect(&metrics.lfMessageFont))
            PhSwapReferenceFont(&Context->FontHandle, Context->ComboBoxHandle, fontHandle, FALSE);

        ComboBox_SetItemHeight(Context->ComboBoxHandle, -1, PhScaleToDisplay(22, DpiValue));
        ComboBox_SetItemHeight(Context->ComboBoxHandle, 0, PhScaleToDisplay(20, DpiValue));
    }
}

/**
 * The dialog procedure for the standard choice dialog.
 *
 * \param WindowHandle The handle to the dialog window.
 * \param WindowMessage The window message.
 * \param wParam Additional message-specific information.
 * \param lParam Additional message-specific information.
 * \return INT_PTR Dialog return value.
 */
INT_PTR CALLBACK PhChoiceDlgProc(
    _In_ HWND WindowHandle,
    _In_ UINT WindowMessage,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PPH_CHOICE_DIALOG_CONTEXT context;

    if (WindowMessage == WM_INITDIALOG)
    {
        context = (PPH_CHOICE_DIALOG_CONTEXT)lParam;
        PhSetWindowContext(WindowHandle, PH_WINDOW_CONTEXT_DEFAULT, context);
    }
    else
    {
        context = PhGetWindowContext(WindowHandle, PH_WINDOW_CONTEXT_DEFAULT);
    }

    if (!context)
        return FALSE;

    switch (WindowMessage)
    {
    case WM_INITDIALOG:
        {
            ULONG type;
            SIZE_T i;
            HWND comboBoxHandle;
            HWND checkBoxHandle;
            RECT checkBoxRect;
            RECT rect;
            ULONG diff;

            checkBoxHandle = GetDlgItem(WindowHandle, IDC_OPTION);

            PhSetWindowText(WindowHandle, context->Title);
            PhSetWindowText(GetDlgItem(WindowHandle, IDC_MESSAGE), context->Message);
            PhCenterWindow(WindowHandle, GetParent(WindowHandle));

            type = context->Flags & PH_CHOICE_DIALOG_TYPE_MASK;

            // Select the control to show, depending on the type. This is
            // because it is impossible to change the style of the combo box
            // after it is created.
            switch (type)
            {
            case PH_CHOICE_DIALOG_USER_CHOICE:
                comboBoxHandle = GetDlgItem(WindowHandle, IDC_CHOICEUSER);
                break;
            case PH_CHOICE_DIALOG_PASSWORD:
                comboBoxHandle = GetDlgItem(WindowHandle, IDC_CHOICESIMPLE);

                // Disable combo box features since it isn't a combo box.
                context->SavedChoicesSettingName = NULL;
                break;
            case PH_CHOICE_DIALOG_CHOICE:
            default:
                comboBoxHandle = GetDlgItem(WindowHandle, IDC_CHOICE);
                break;
            }

            context->ComboBoxHandle = comboBoxHandle;
            ShowWindow(context->ComboBoxHandle, SW_SHOW);

            PhpChoiceDialogInitializeBrushes(context);

            if (type != PH_CHOICE_DIALOG_PASSWORD)
            {
                LONG dpiValue = PhGetWindowDpi(WindowHandle);

                ComboBox_SetItemHeight(comboBoxHandle, 0, PhScaleToDisplay(16, dpiValue));
            }

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
                if (!PhIsNullOrEmptyString(*context->SelectedChoice))
                {
                    PhSetWindowText(comboBoxHandle, PhGetString(*context->SelectedChoice));
                }

                Edit_SetSel(comboBoxHandle, 0, -1);
            }
            else if (type == PH_CHOICE_DIALOG_USER_CHOICE || type == PH_CHOICE_DIALOG_CHOICE)
            {
                // If we failed to choose a default choice based on what was specified,
                // select the first one if possible, or set the text directly.
                if (
                    !PhIsNullOrEmptyString(*context->SelectedChoice) ||
                    PhSelectComboBoxString(comboBoxHandle, PhGetString(*context->SelectedChoice), FALSE) == CB_ERR
                    )
                {
                    if (type == PH_CHOICE_DIALOG_USER_CHOICE && !PhIsNullOrEmptyString(*context->SelectedChoice))
                    {
                        PhSetWindowText(comboBoxHandle, PhGetString(*context->SelectedChoice));
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
                PhGetWindowRect(checkBoxHandle, &checkBoxRect);
                MapWindowRect(NULL, WindowHandle, &checkBoxRect);
                PhGetWindowRect(GetDlgItem(WindowHandle, IDOK), &rect);
                MapWindowRect(NULL, WindowHandle, &rect);
                diff = rect.top - checkBoxRect.top;

                // OK
                rect.top -= diff;
                rect.bottom -= diff;
                SetWindowPos(GetDlgItem(WindowHandle, IDOK), NULL, rect.left, rect.top,
                    rect.right - rect.left, rect.bottom - rect.top,
                    SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOOWNERZORDER);

                // Cancel
                PhGetWindowRect(GetDlgItem(WindowHandle, IDCANCEL), &rect);
                MapWindowRect(NULL, WindowHandle, &rect);
                rect.top -= diff;
                rect.bottom -= diff;
                SetWindowPos(GetDlgItem(WindowHandle, IDCANCEL), NULL, rect.left, rect.top,
                    rect.right - rect.left, rect.bottom - rect.top,
                    SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOOWNERZORDER);

                // Window
                PhGetWindowRect(WindowHandle, &rect);
                rect.bottom -= diff;
                SetWindowPos(WindowHandle, NULL, rect.left, rect.top,
                    rect.right - rect.left, rect.bottom - rect.top,
                    SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOOWNERZORDER);
            }

            PhInitializeWindowTheme(WindowHandle, PhEnableThemeSupport);

            PhSetDialogFocus(WindowHandle, comboBoxHandle);
        }
        break;
    case WM_DESTROY:
        {
            PhpChoiceDialogDeleteBrushes(context);

            PhRemoveWindowContext(WindowHandle, PH_WINDOW_CONTEXT_DEFAULT);
        }
        break;
    case WM_DRAWITEM:
        {
            LPDRAWITEMSTRUCT drawInfo = (LPDRAWITEMSTRUCT)lParam;

            if (drawInfo->hwndItem == context->ComboBoxHandle)
                return PhpChoiceDialogDrawComboBoxItem(context, drawInfo);
        }
        break;
    case WM_COMMAND:
        {
            switch (GET_WM_COMMAND_ID(wParam, lParam))
            {
            case IDCANCEL:
                EndDialog(WindowHandle, IDCANCEL);
                break;
            case IDOK:
                {
                    PPH_STRING selectedChoice;

                    if (FlagOn(context->Flags, PH_CHOICE_DIALOG_TYPE_MASK) != PH_CHOICE_DIALOG_PASSWORD)
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
                    {
                        *context->SelectedOption = Button_GetCheck(GetDlgItem(WindowHandle, IDC_OPTION)) == BST_CHECKED;
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
                            escaped = PH_AUTO(PhEscapeStringForDelimiter(selectedChoice, L'\\'));
                            PhAppendStringBuilder(&savedChoices, &escaped->sr);
                            PhAppendStringBuilder2(&savedChoices, L"\\s");
                        }

                        for (i = 1; i < choicesToSave; i++)
                        {
                            choice = PH_AUTO(PhGetComboBoxString(context->ComboBoxHandle, i - 1));

                            if (PhIsNullOrEmptyString(choice))
                                break;

                            // Don't save the choice if it's the same as the one
                            // entered by the user (since we already saved it above).
                            if (PhEqualString(choice, selectedChoice, FALSE))
                            {
                                choicesToSave++; // useless for now, but may be needed in the future
                                continue;
                            }

                            escaped = PH_AUTO(PhEscapeStringForDelimiter(choice, L'\\'));
                            PhAppendStringBuilder(&savedChoices, &escaped->sr);
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
    case WM_CTLCOLORBTN:
        return HANDLE_WM_CTLCOLORBTN(WindowHandle, wParam, lParam, PhWindowThemeControlColor);
    case WM_CTLCOLORDLG:
        return HANDLE_WM_CTLCOLORDLG(WindowHandle, wParam, lParam, PhWindowThemeControlColor);
    case WM_CTLCOLORSTATIC:
        return HANDLE_WM_CTLCOLORSTATIC(WindowHandle, wParam, lParam, PhWindowThemeControlColor);
    }

    return FALSE;
}

/**
 * The dialog procedure for the new style choice dialog.
 *
 * \param WindowHandle The handle to the dialog window.
 * \param WindowMessage The window message.
 * \param wParam Additional message-specific information.
 * \param lParam Additional message-specific information.
 * \return INT_PTR Dialog return value.
 */
INT_PTR CALLBACK PhChooseNewPageDlgProc(
    _In_ HWND WindowHandle,
    _In_ UINT WindowMessage,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PPH_CHOICE_DIALOG_CONTEXT context;

    if (WindowMessage == WM_INITDIALOG)
    {
        context = (PPH_CHOICE_DIALOG_CONTEXT)lParam;
        PhSetWindowContext(WindowHandle, PH_WINDOW_CONTEXT_DEFAULT, context);
    }
    else
    {
        context = PhGetWindowContext(WindowHandle, PH_WINDOW_CONTEXT_DEFAULT);
    }

    if (!context)
        return FALSE;

    switch (WindowMessage)
    {
    case WM_INITDIALOG:
        {
            context->ComboBoxHandle = GetDlgItem(WindowHandle, IDC_INPUT);

            PhpChoiceDialogInitializeBrushes(context);

            PhCenterWindow(WindowHandle, GetParent(WindowHandle));
            PhSetApplicationWindowIcon(WindowHandle);

            PhSetWindowText(WindowHandle, PhApplicationName);
            PhSetWindowText(GetDlgItem(WindowHandle, IDC_TITLE), context->Title);
            PhSetWindowText(GetDlgItem(WindowHandle, IDC_TEXT), context->Message);

            PhpChoiceDialogUpdateFonts(WindowHandle, context, PhGetWindowDpi(WindowHandle));

            {
                if (FlagOn(context->Flags, PH_CHOICE_DIALOG_TYPE_MASK) == PH_CHOICE_DIALOG_PASSWORD)
                {
                    NOTHING;
                }
                else if (FlagOn(context->Flags, PH_CHOICE_DIALOG_TYPE_MASK) == PH_CHOICE_DIALOG_USER_CHOICE && context->SavedChoicesSettingName)
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

            if (PhEnableThemeSupport)
                ShowWindow(GetDlgItem(WindowHandle, IDC_SIZE_), SW_HIDE);
            PhInitializeWindowTheme(WindowHandle, PhEnableThemeSupport);
        }
        break;
    case WM_DESTROY:
        {
            PhRemoveWindowContext(WindowHandle, PH_WINDOW_CONTEXT_DEFAULT);

            PhpChoiceDialogDeleteBrushes(context);

            if (context->FontHandle)
            {
                DeleteFont(context->FontHandle);
            }

            if (context->TitleFontHandle)
            {
                DeleteFont(context->TitleFontHandle);
            }
        }
        break;
    case WM_DPICHANGED:
        {
            PhpChoiceDialogUpdateFonts(WindowHandle, context, LOWORD(wParam));
        }
        break;
    case WM_DRAWITEM:
        {
            LPDRAWITEMSTRUCT drawInfo = (LPDRAWITEMSTRUCT)lParam;

            if (drawInfo->hwndItem == context->ComboBoxHandle)
                return PhpChoiceDialogDrawComboBoxItem(context, drawInfo);
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
                    PPH_STRING selectedChoice;

                    if (FlagOn(context->Flags, PH_CHOICE_DIALOG_TYPE_MASK) != PH_CHOICE_DIALOG_PASSWORD)
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
                            escaped = PH_AUTO(PhEscapeStringForDelimiter(selectedChoice, L'\\'));
                            PhAppendStringBuilder(&savedChoices, &escaped->sr);
                            PhAppendStringBuilder2(&savedChoices, L"\\s");
                        }

                        for (i = 1; i < choicesToSave; i++)
                        {
                            choice = PH_AUTO(PhGetComboBoxString(context->ComboBoxHandle, i - 1));

                            if (PhIsNullOrEmptyString(choice))
                                break;

                            // Don't save the choice if it's the same as the one
                            // entered by the user (since we already saved it above).
                            if (PhEqualString(choice, selectedChoice, FALSE))
                            {
                                choicesToSave++; // useless for now, but may be needed in the future
                                continue;
                            }

                            escaped = PH_AUTO(PhEscapeStringForDelimiter(choice, L'\\'));
                            PhAppendStringBuilder(&savedChoices, &escaped->sr);
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

            if (!PhGetClientRect(WindowHandle, &clientRect))
                break;

            SetBkMode(hdc, TRANSPARENT);

            clientRect.bottom -= PhScaleToDisplay(50, dpi);
            FillRect(hdc, &clientRect, PhEnableThemeSupport ? PhThemeWindowBackgroundBrush : GetSysColorBrush(COLOR_WINDOW));

            clientRect.top = clientRect.bottom;
            clientRect.bottom = clientRect.top + PhScaleToDisplay(50, dpi);

            if (PhEnableThemeSupport)
            {
                SetDCBrushColor(hdc, RGB(50, 50, 50));
                FillRect(hdc, &clientRect, PhGetStockBrush(DC_BRUSH));
                clientRect.bottom = clientRect.top + 1;
                SetDCBrushColor(hdc, PhThemeWindowForegroundColor);
                FillRect(hdc, &clientRect, PhGetStockBrush(DC_BRUSH));
                PhOffsetRect(&clientRect, 0, 1);
                SetDCBrushColor(hdc, PhThemeWindowBackground2Color);
                FillRect(hdc, &clientRect, PhGetStockBrush(DC_BRUSH));
            }
            else
            {
                FillRect(hdc, &clientRect, GetSysColorBrush(COLOR_3DFACE));
            }

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
        PhChoiceDlgProc,
        &context
        ) == IDOK;
}

/**
 * Prompts the user for input using the new dialog style.
 *
 * \param ParentWindowHandle Parent window for the dialog.
 * \param Title The title of the dialog.
 * \param Message The message displayed in the dialog.
 * \param Choices Optional list of choices to populate the combo box.
 * \param NumberOfChoices Number of choices in the Choices array.
 * \param Option Optional check box label to show.
 * \param Flags Dialog configuration flags.
 * \param SelectedChoice Receives the selected choice string.
 * \param SelectedOption Receives the state of the check box if checked.
 * \param SavedChoicesSettingName Optional setting name to save the choices list.
 * \return BOOLEAN TRUE if the user pressed OK, FALSE otherwise.
 */
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
