/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     jxy-s   2026
 *
 */

#include "onlnchk.h"

#define PCRE2_CODE_UNIT_WIDTH 16
#include "../../tools/thirdparty/pcre/pcre2.h"

static PH_QUEUED_LOCK ScanExcludeLock = PH_QUEUED_LOCK_INIT;
static PPH_LIST ScanExcludeList = NULL;

static VOID ScanClearExcludeListUnderLock(
    VOID
    )
{
    if (!ScanExcludeList)
        return;

    for (ULONG i = 0; i < ScanExcludeList->Count; i++)
        pcre2_code_free(ScanExcludeList->Items[i]);

    PhClearList(ScanExcludeList);
}

VOID ScanLoadExclusions(
    VOID
    )
{
    static const PH_STRINGREF trimSet = PH_STRINGREF_INIT(L" \t\r");
    PPH_STRING setting;
    PH_STRINGREF remaining;

    setting = PhGetStringSetting(SETTING_NAME_SCAN_EXCLUDE_LIST);

    PhAcquireQueuedLockExclusive(&ScanExcludeLock);

    if (!ScanExcludeList)
        ScanExcludeList = PhCreateList(4);

    ScanClearExcludeListUnderLock();

    remaining = setting->sr;

    while (remaining.Length != 0)
    {
        PH_STRINGREF line;
        pcre2_code* code;
        int errorCode;
        PCRE2_SIZE errorOffset;

        PhSplitStringRefAtChar(&remaining, L'\n', &line, &remaining);
        PhTrimStringRef(&line, &trimSet, 0);

        if (line.Length == 0)
            continue;

        code = pcre2_compile(
            (PCRE2_SPTR)line.Buffer,
            line.Length / sizeof(WCHAR),
            PCRE2_CASELESS,
            &errorCode,
            &errorOffset,
            NULL
            );

        if (code)
            PhAddItemList(ScanExcludeList, code);
    }

    PhReleaseQueuedLockExclusive(&ScanExcludeLock);

    PhDereferenceObject(setting);
}

BOOLEAN ScanIsFileExcluded(
    _In_opt_ PPH_STRING FileName
    )
{
    BOOLEAN excluded = FALSE;

    if (PhIsNullOrEmptyString(FileName))
        return FALSE;

    PhAcquireQueuedLockShared(&ScanExcludeLock);

    if (ScanExcludeList)
    {
        for (ULONG i = 0; i < ScanExcludeList->Count; i++)
        {
            pcre2_code* code = ScanExcludeList->Items[i];
            pcre2_match_data* matchData;

            matchData = pcre2_match_data_create_from_pattern(code, NULL);

            if (pcre2_match(
                code,
                (PCRE2_SPTR)FileName->Buffer,
                FileName->Length / sizeof(WCHAR),
                0,
                0,
                matchData,
                NULL
                ) >= 0)
            {
                excluded = TRUE;
            }

            pcre2_match_data_free(matchData);

            if (excluded)
                break;
        }
    }

    PhReleaseQueuedLockShared(&ScanExcludeLock);

    return excluded;
}

static PPH_STRING ExcludeGetListBoxItemText(
    _In_ HWND ListBoxHandle,
    _In_ INT Index
    )
{
    INT length;
    PPH_STRING string;

    length = ListBox_GetTextLen(ListBoxHandle, Index);

    if (length <= 0)
        return NULL;

    string = PhCreateStringEx(NULL, length * sizeof(WCHAR));
    ListBox_GetText(ListBoxHandle, Index, string->Buffer);

    return string;
}

static BOOLEAN ExcludeValidateRegex(
    _In_ PPH_STRINGREF Pattern,
    _Out_writes_(MessageCount) PWSTR Message,
    _In_ SIZE_T MessageCount
    )
{
    pcre2_code* code;
    int errorCode;
    PCRE2_SIZE errorOffset;

    code = pcre2_compile(
        (PCRE2_SPTR)Pattern->Buffer,
        Pattern->Length / sizeof(WCHAR),
        PCRE2_CASELESS,
        &errorCode,
        &errorOffset,
        NULL
        );

    if (code)
    {
        pcre2_code_free(code);
        return TRUE;
    }

    pcre2_get_error_message(errorCode, (PCRE2_UCHAR*)Message, MessageCount);
    return FALSE;
}

VOID ScanExclusionsPopulateListBox(
    _In_ HWND ListBoxHandle
    )
{
    static const PH_STRINGREF trimSet = PH_STRINGREF_INIT(L" \t\r");
    PPH_STRING setting;
    PH_STRINGREF remaining;

    ListBox_ResetContent(ListBoxHandle);

    setting = PhGetStringSetting(SETTING_NAME_SCAN_EXCLUDE_LIST);
    remaining = setting->sr;

    while (remaining.Length != 0)
    {
        PH_STRINGREF line;

        PhSplitStringRefAtChar(&remaining, L'\n', &line, &remaining);
        PhTrimStringRef(&line, &trimSet, 0);

        if (line.Length != 0)
        {
            PPH_STRING entry = PhCreateString2(&line);
            ListBox_AddString(ListBoxHandle, entry->Buffer);
            PhDereferenceObject(entry);
        }
    }

    PhDereferenceObject(setting);
}

static BOOLEAN ExcludeLooksLikePath(
    _In_ PCPH_STRINGREF Pattern
    )
{
    SIZE_T count = Pattern->Length / sizeof(WCHAR);
    WCHAR drive;

    if (count < 3)
        return FALSE;

    drive = Pattern->Buffer[0];

    if (!((drive >= L'A' && drive <= L'Z') || (drive >= L'a' && drive <= L'z')))
        return FALSE;
    if (Pattern->Buffer[1] != L':' || Pattern->Buffer[2] != L'\\')
        return FALSE;
    if (count >= 4 && Pattern->Buffer[3] == L'\\') // already escaped
        return FALSE;

    return TRUE;
}

static PPH_STRING ExcludePathToRegex(
    _In_ PCPH_STRINGREF Input
    )
{
    PH_STRING_BUILDER stringBuilder;
    SIZE_T count = Input->Length / sizeof(WCHAR);

    PhInitializeStringBuilder(&stringBuilder, (ULONG)(count * 2 + 8) * sizeof(WCHAR));

    for (SIZE_T i = 0; i < count; i++)
    {
        WCHAR c = Input->Buffer[i];

        switch (c)
        {
        case L'*':
            PhAppendStringBuilder2(&stringBuilder, L".*");
            break;
        case L'?':
            PhAppendCharStringBuilder(&stringBuilder, L'.');
            break;
        case L'\\':
        case L'.':
        case L'^':
        case L'$':
        case L'+':
        case L'(':
        case L')':
        case L'[':
        case L']':
        case L'{':
        case L'}':
        case L'|':
            PhAppendCharStringBuilder(&stringBuilder, L'\\');
            PhAppendCharStringBuilder(&stringBuilder, c);
            break;
        default:
            PhAppendCharStringBuilder(&stringBuilder, c);
            break;
        }
    }

    return PhFinalStringBuilderString(&stringBuilder);
}

VOID ScanExclusionsAddFromEdit(
    _In_ HWND WindowHandle,
    _In_ HWND ListBoxHandle,
    _In_ HWND EditHandle
    )
{
    static const PH_STRINGREF trimSet = PH_STRINGREF_INIT(L" \t\r");
    PPH_STRING text;
    PH_STRINGREF trimmed;
    PPH_STRING pattern;
    WCHAR message[256];

    text = PhGetWindowText(EditHandle);

    if (!text)
        return;

    trimmed = text->sr;
    PhTrimStringRef(&trimmed, &trimSet, 0);

    if (trimmed.Length == 0)
    {
        PhClearReference(&text);
        return;
    }

    pattern = PhCreateString2(&trimmed);

    if (ExcludeLooksLikePath(&pattern->sr))
    {
        if (PhShowMessage2(
            WindowHandle,
            TD_YES_BUTTON | TD_NO_BUTTON,
            TD_INFORMATION_ICON,
            L"This looks like a wildcard path, not a regular expression.",
            L"Convert it to a regular expression? Backslashes will be escaped and wildcards expanded."
            ) == IDYES)
        {
            PhMoveReference(&pattern, ExcludePathToRegex(&pattern->sr));
        }
    }

    if (ExcludeValidateRegex(&pattern->sr, message, RTL_NUMBER_OF(message)))
    {
        ListBox_AddString(ListBoxHandle, pattern->Buffer);
        SetWindowText(EditHandle, L"");
        PhSetDialogFocus(WindowHandle, EditHandle);
    }
    else
    {
        PhShowError2(WindowHandle, L"The regular expression could not be compiled.", L"%s", message);
    }

    PhDereferenceObject(pattern);
    PhClearReference(&text);
}

VOID ScanExclusionsSaveListBox(
    _In_ HWND ListBoxHandle
    )
{
    INT count = ListBox_GetCount(ListBoxHandle);
    PH_STRING_BUILDER stringBuilder;

    PhInitializeStringBuilder(&stringBuilder, 64);

    for (INT i = 0; i < count; i++)
    {
        PPH_STRING entry = ExcludeGetListBoxItemText(ListBoxHandle, i);

        if (entry)
        {
            PhAppendStringBuilder(&stringBuilder, &entry->sr);
            PhAppendCharStringBuilder(&stringBuilder, L'\n');
            PhDereferenceObject(entry);
        }
    }

    PhSetStringSetting2(SETTING_NAME_SCAN_EXCLUDE_LIST, &stringBuilder.String->sr);
    PhDeleteStringBuilder(&stringBuilder);
}
