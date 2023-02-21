/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2011-2016
 *     dmex    2016-2023
 *
 */

#include "usernotes.h"

INT_PTR CALLBACK ProcessCommentPageDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    LPPROPSHEETPAGE propSheetPage;
    PPH_PROCESS_PROPPAGECONTEXT propPageContext;
    PPH_PROCESS_ITEM processItem;
    PPROCESS_COMMENT_PAGE_CONTEXT context;

    if (PhPropPageDlgProcHeader(hwndDlg, uMsg, lParam, &propSheetPage, &propPageContext, &processItem))
    {
        context = propPageContext->Context;
    }
    else
    {
        return FALSE;
    }

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            PDB_OBJECT object;
            PPH_STRING comment;

            context = propPageContext->Context = PhAllocateZero(sizeof(PROCESS_COMMENT_PAGE_CONTEXT));
            context->CommentHandle = GetDlgItem(hwndDlg, IDC_COMMENT);
            context->RevertHandle = GetDlgItem(hwndDlg, IDC_REVERT);
            context->MatchCommandlineHandle = GetDlgItem(hwndDlg, IDC_MATCHCOMMANDLINE);

            // Load the comment.
            Edit_LimitText(context->CommentHandle, UNICODE_STRING_MAX_CHARS);

            LockDb();

            if (object = FindDbObjectForProcess(processItem, INTENT_PROCESS_COMMENT))
            {
                PhSetReference(&comment, object->Comment);

                if (processItem->CommandLine && (object = FindDbObject(COMMAND_LINE_TAG, &processItem->CommandLine->sr)) && object->Comment->Length != 0)
                {
                    Button_SetCheck(context->MatchCommandlineHandle, BST_CHECKED);
                }
            }
            else
            {
                comment = PhReferenceEmptyString();
            }

            UnlockDb();

            Edit_SetText(context->CommentHandle, comment->Buffer);
            context->OriginalComment = comment;

            if (!processItem->CommandLine)
                EnableWindow(context->MatchCommandlineHandle, FALSE);

            PhInitializeWindowTheme(hwndDlg, !!PhGetIntegerSetting(L"EnableThemeSupport"));
        }
        break;
    case WM_DESTROY:
        {
            PDB_OBJECT object;
            PPH_STRING comment;
            BOOLEAN matchCommandLine;
            BOOLEAN done = FALSE;

            comment = PH_AUTO(PhGetWindowText(context->CommentHandle));
            matchCommandLine = Button_GetCheck(context->MatchCommandlineHandle) == BST_CHECKED;

            if (!processItem->CommandLine)
                matchCommandLine = FALSE;

            LockDb();

            if (processItem->CommandLine && !matchCommandLine)
            {
                PDB_OBJECT objectForProcessName;
                PPH_STRING message = NULL;

                object = FindDbObject(COMMAND_LINE_TAG, &processItem->CommandLine->sr);
                objectForProcessName = FindDbObject(FILE_TAG, &processItem->ProcessName->sr);

                if (object && objectForProcessName && object->Comment->Length != 0 && objectForProcessName->Comment->Length != 0 &&
                    !PhEqualString(comment, objectForProcessName->Comment, FALSE))
                {
                    message = PhaFormatString(
                        L"Do you want to replace the comment for %s which is currently\n    \"%s\"\n"
                        L"with\n    \"%s\"?",
                        processItem->ProcessName->Buffer,
                        objectForProcessName->Comment->Buffer,
                        comment->Buffer
                        );
                }

                if (object)
                {
                    PhMoveReference(&object->Comment, PhReferenceEmptyString());
                    DeleteDbObjectForProcessIfUnused(object);
                }

                if (message)
                {
                    // Prevent deadlocks.
                    UnlockDb();

                    if (MessageBox(hwndDlg, message->Buffer, L"Comment", MB_ICONQUESTION | MB_YESNO) == IDNO)
                    {
                        done = TRUE;
                    }

                    LockDb();
                }
            }

            if (!done)
            {
                if (comment->Length != 0)
                {
                    if (matchCommandLine)
                        CreateDbObject(COMMAND_LINE_TAG, &processItem->CommandLine->sr, comment);
                    else
                        CreateDbObject(FILE_TAG, &processItem->ProcessName->sr, comment);
                }
                else
                {
                    if (
                        (!matchCommandLine && (object = FindDbObject(FILE_TAG, &processItem->ProcessName->sr))) ||
                        (matchCommandLine && (object = FindDbObject(COMMAND_LINE_TAG, &processItem->CommandLine->sr)))
                        )
                    {
                        PhMoveReference(&object->Comment, PhReferenceEmptyString());
                        DeleteDbObjectForProcessIfUnused(object);
                    }
                }
            }

            UnlockDb();

            PhDereferenceObject(context->OriginalComment);
            PhFree(context);

            SaveDb();
            InvalidateProcessComments();
        }
        break;
    case WM_SHOWWINDOW:
        {
            PPH_LAYOUT_ITEM dialogItem;

            if (dialogItem = PhBeginPropPageLayout(hwndDlg, propPageContext))
            {
                PhAddPropPageLayoutItem(hwndDlg, context->CommentHandle, dialogItem, PH_ANCHOR_ALL);
                PhAddPropPageLayoutItem(hwndDlg, context->RevertHandle, dialogItem, PH_ANCHOR_LEFT | PH_ANCHOR_BOTTOM);
                PhAddPropPageLayoutItem(hwndDlg, context->MatchCommandlineHandle, dialogItem, PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM);
                PhEndPropPageLayout(hwndDlg, propPageContext);
            }
        }
        break;
    case WM_COMMAND:
        {
            switch (GET_WM_COMMAND_ID(wParam, lParam))
            {
            case IDC_COMMENT:
                {
                    if (GET_WM_COMMAND_CMD(wParam, lParam) == EN_CHANGE)
                        EnableWindow(context->RevertHandle, TRUE);
                }
                break;
            case IDC_REVERT:
                {
                    Edit_SetText(context->CommentHandle, context->OriginalComment->Buffer);
                    SendMessage(context->CommentHandle, EM_SETSEL, 0, -1);
                    PhSetDialogFocus(hwndDlg, context->CommentHandle);
                    EnableWindow(context->RevertHandle, FALSE);
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
            case PSN_QUERYINITIALFOCUS:
                SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, (LONG_PTR)context->MatchCommandlineHandle);
                return TRUE;
            }
        }
        break;
    }

    return FALSE;
}

INT_PTR CALLBACK ServiceCommentPageDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PSERVICE_COMMENT_PAGE_CONTEXT context;

    if (uMsg == WM_INITDIALOG)
    {
        context = PhAllocate(sizeof(SERVICE_COMMENT_PAGE_CONTEXT));
        memset(context, 0, sizeof(SERVICE_COMMENT_PAGE_CONTEXT));

        PhSetWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT, context);
    }
    else
    {
        context = PhGetWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT);

        if (uMsg == WM_DESTROY)
            PhRemoveWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT);
    }

    if (!context)
        return FALSE;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            LPPROPSHEETPAGE propSheetPage = (LPPROPSHEETPAGE)lParam;
            PPH_SERVICE_ITEM serviceItem = (PPH_SERVICE_ITEM)propSheetPage->lParam;
            PDB_OBJECT object;
            PPH_STRING comment;

            context->ServiceItem = serviceItem;

            PhInitializeLayoutManager(&context->LayoutManager, hwndDlg);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDC_COMMENT), NULL, PH_ANCHOR_ALL);

            // Load the comment.

            SendMessage(GetDlgItem(hwndDlg, IDC_COMMENT), EM_SETLIMITTEXT, UNICODE_STRING_MAX_CHARS, 0);

            LockDb();

            if (object = FindDbObject(SERVICE_TAG, &serviceItem->Name->sr))
                comment = object->Comment;
            else
                comment = PH_AUTO(PhReferenceEmptyString());

            UnlockDb();

            PhSetDialogItemText(hwndDlg, IDC_COMMENT, comment->Buffer);

            PhInitializeWindowTheme(hwndDlg, !!PhGetIntegerSetting(L"EnableThemeSupport"));
        }
        break;
    case WM_DESTROY:
        {
            PhDeleteLayoutManager(&context->LayoutManager);
            PhFree(context);
        }
        break;
    case WM_SIZE:
        {
            PhLayoutManagerLayout(&context->LayoutManager);
        }
        break;
    case WM_COMMAND:
        {
            switch (GET_WM_COMMAND_ID(wParam, lParam))
            {
            case IDC_COMMENT:
                {
                    if (GET_WM_COMMAND_CMD(wParam, lParam) == EN_CHANGE)
                        EnableWindow(GetDlgItem(hwndDlg, IDC_REVERT), TRUE);
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
            case PSN_KILLACTIVE:
                {
                    SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, FALSE);
                }
                return TRUE;
            case PSN_APPLY:
                {
                    PDB_OBJECT object;
                    PPH_STRING comment;

                    comment = PH_AUTO(PhGetWindowText(GetDlgItem(hwndDlg, IDC_COMMENT)));

                    LockDb();

                    if (comment->Length != 0)
                    {
                        CreateDbObject(SERVICE_TAG, &context->ServiceItem->Name->sr, comment);
                    }
                    else
                    {
                        if (object = FindDbObject(SERVICE_TAG, &context->ServiceItem->Name->sr))
                            DeleteDbObject(object);
                    }

                    UnlockDb();

                    SaveDb();
                    InvalidateServiceComments();

                    SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, PSNRET_NOERROR);
                }
                return TRUE;
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

UINT_PTR CALLBACK ColorDlgHookProc(
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
            PhCenterWindow(hwndDlg, GetParent(hwndDlg));

            PhInitializeWindowTheme(hwndDlg, !!PhGetIntegerSetting(L"EnableThemeSupport"));
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
