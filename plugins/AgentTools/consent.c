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

#include "agenttools.h"

typedef struct _AT_CONSENT_REQUEST
{
    LONG RefCount;
    LONG Abandoned;
    PH_EVENT CompletedEvent;
    HWND DialogHandle;
    LONG Shown;
    ULONG64 ShownTick;
    ULONG64 SubmitTick;
    BOOLEAN Raised;

    PAT_CONNECTION Connection;
    BOOLEAN ConnectionRequest;

    PCAT_ACTION_INFO Action;
    HICON MainIcon;
    PPH_STRING Instruction;
    PPH_STRING Content;
    PPH_STRING Footer;
    PPH_STRING FooterUpdate;
    BOOLEAN OfferPolicies;
    BOOLEAN OfferDelegate;
    PCWSTR AcceptText;
    PCWSTR DeclineText;
    HWND ComboHandle;

    BOOLEAN Allowed;
    AT_SESSION_POLICY Policy;
    BOOLEAN TimedOut;
    BOOLEAN Failed;
} AT_CONSENT_REQUEST, *PAT_CONSENT_REQUEST;

static PH_WORK_QUEUE AtConsentWorkQueue;

VOID AtConsentInitialize(
    VOID
    )
{
    PhInitializeWorkQueue(&AtConsentWorkQueue, 0, 1, 1000);
}

VOID AtConsentUninitialize(
    VOID
    )
{
    PhDeleteWorkQueue(&AtConsentWorkQueue);
}

VOID AtpCreateSessionPolicyControls(
    _In_ HWND DialogHandle,
    _In_ PAT_CONSENT_REQUEST Request
    );

VOID AtpCenterWindowOnUserMonitor(
    _In_ HWND WindowHandle
    )
{
    HWND foregroundWindow;
    HMONITOR monitor = NULL;
    MONITORINFO monitorInfo;
    RECT rect;
    PH_RECTANGLE rectangle;
    PH_RECTANGLE bounds;

    if (foregroundWindow = GetForegroundWindow())
        monitor = MonitorFromWindow(foregroundWindow, MONITOR_DEFAULTTONULL);

    if (!monitor)
    {
        POINT cursor;

        if (GetCursorPos(&cursor))
            monitor = MonitorFromPoint(cursor, MONITOR_DEFAULTTONEAREST);
    }

    memset(&monitorInfo, 0, sizeof(MONITORINFO));
    monitorInfo.cbSize = sizeof(MONITORINFO);

    if (!monitor || !GetMonitorInfo(monitor, &monitorInfo))
    {
        PhCenterWindow(WindowHandle, NULL);
        return;
    }

    if (!PhGetWindowRect(WindowHandle, &rect))
        return;

    PhRectToRectangle(&rectangle, &rect);
    PhRectToRectangle(&bounds, &monitorInfo.rcWork);
    PhCenterRectangle(&rectangle, &bounds);

    MoveWindow(WindowHandle, rectangle.Left, rectangle.Top, rectangle.Width, rectangle.Height, FALSE);
}

PPH_STRING AtpFormatConnectionRequester(
    _In_ PAT_CONNECTION Connection
    );

VOID AtpDereferenceConsentRequest(
    _In_ PAT_CONSENT_REQUEST Request
    )
{
    if (InterlockedDecrement(&Request->RefCount) == 0)
    {
        if (Request->MainIcon)
            DestroyIcon(Request->MainIcon);

        PhClearReference(&Request->Instruction);
        PhClearReference(&Request->Content);
        PhClearReference(&Request->Footer);
        PhClearReference(&Request->FooterUpdate);

        if (Request->Connection)
            PhDereferenceObject(Request->Connection);

        PhFree(Request);
    }
}

VOID AtpEnsureLauncherVerified(
    _In_ PAT_CONNECTION Connection
    )
{
    PPH_STRING launcher = NULL;
    PPH_STRING signer = NULL;
    VERIFY_RESULT result;

    PhAcquireQueuedLockExclusive(&Connection->Lock);
    if (!Connection->LauncherVerifyChecked && Connection->LauncherImageName)
        launcher = PhReferenceObject(Connection->LauncherImageName);
    PhReleaseQueuedLockExclusive(&Connection->Lock);

    if (!launcher)
        return;

    result = PhVerifyFile(PhGetString(launcher), &signer);
    PhDereferenceObject(launcher);

    PhAcquireQueuedLockExclusive(&Connection->Lock);
    if (!Connection->LauncherVerifyChecked)
    {
        Connection->LauncherVerifyChecked = TRUE;
        Connection->LauncherVerifyResult = result;
        PhMoveReference(&Connection->LauncherSignerName, signer);
        signer = NULL;
    }
    PhReleaseQueuedLockExclusive(&Connection->Lock);

    PhClearReference(&signer);
}

PPH_STRING AtpFormatRequester(
    _In_ PAT_CONNECTION Connection
    )
{
    PH_STRING_BUILDER builder;
    PPH_STRING launcher = NULL;

    AtpEnsureLauncherVerified(Connection);

    PhInitializeStringBuilder(&builder, 128);

    PhAcquireQueuedLockExclusive(&Connection->Lock);

    PhAppendStringBuilder2(&builder, L"Requested by ");

    if (Connection->ClientName)
    {
        PhAppendStringBuilder(&builder, &Connection->ClientName->sr);

        if (Connection->ClientVersion)
        {
            PhAppendCharStringBuilder(&builder, L' ');
            PhAppendStringBuilder(&builder, &Connection->ClientVersion->sr);
        }
    }
    else
    {
        PhAppendStringBuilder2(&builder, L"an unidentified client");
    }

    if (Connection->LauncherImageName)
        launcher = PhGetBaseName(Connection->LauncherImageName);

    PhAppendFormatStringBuilder(&builder, L" via %s", PhGetStringOrDefault(launcher, L"an unknown process"));
    PhClearReference(&launcher);

    if (Connection->LauncherVerifyChecked && Connection->LauncherVerifyResult == VrTrusted)
        PhAppendFormatStringBuilder(&builder, L"\nSigner: %s (trusted)", PhGetStringOrDefault(Connection->LauncherSignerName, L"unknown"));
    else if (!Connection->LauncherVerifyChecked || Connection->LauncherVerifyResult == VrUnknown)
        PhAppendStringBuilder2(&builder, L"\nSigner: not verified");
    else
        PhAppendStringBuilder2(&builder, L"\nSigner: not trusted");

    PhAppendFormatStringBuilder(&builder, L"\nUser: %s", PhGetStringOrDefault(Connection->UserName, L"unknown"));

    PhReleaseQueuedLockExclusive(&Connection->Lock);

    return PhFinalStringBuilderString(&builder);
}

HICON AtpCreateRequestIcon(
    _In_ PAT_CONNECTION Connection
    )
{
    PPH_STRING launcher = NULL;
    LONG dpi;
    LONG largeSize;
    LONG smallSize;
    HICON launcherIcon = NULL;
    HICON unusedIcon = NULL;
    HBITMAP launcherBitmap = NULL;
    HBITMAP shieldBitmap = NULL;
    HBITMAP compositeBitmap = NULL;
    HBITMAP maskBitmap = NULL;
    HDC compositeDc = NULL;
    HDC sourceDc = NULL;
    HGDIOBJ oldComposite = NULL;
    HGDIOBJ oldSource = NULL;
    BITMAPINFO bitmapInfo;
    PVOID bits;
    BLENDFUNCTION blend = { AC_SRC_OVER, 0, 255, AC_SRC_ALPHA };
    ICONINFO iconInfo;
    HICON result = NULL;

    PhAcquireQueuedLockExclusive(&Connection->Lock);
    if (Connection->LauncherImageName)
        launcher = PhReferenceObject(Connection->LauncherImageName);
    PhReleaseQueuedLockExclusive(&Connection->Lock);

    if (!launcher)
        return NULL;

    dpi = PhGetWindowDpi(SystemInformer_GetWindowHandle());
    largeSize = PhGetSystemMetrics(SM_CXICON, dpi);
    smallSize = PhGetSystemMetrics(SM_CXSMICON, dpi);

    PhExtractIconEx(&launcher->sr, FALSE, 0, largeSize, largeSize, smallSize, smallSize, &launcherIcon, &unusedIcon);
    PhDereferenceObject(launcher);

    if (unusedIcon)
        DestroyIcon(unusedIcon);
    if (!launcherIcon)
        return NULL;

    // Both helpers yield premultiplied 32-bit bitmaps, so a plain alpha blend composes them.
    launcherBitmap = PhIconToBitmap(launcherIcon, largeSize, largeSize);
    DestroyIcon(launcherIcon);

    if (!launcherBitmap)
        return NULL;

    memset(&bitmapInfo, 0, sizeof(BITMAPINFO));
    bitmapInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bitmapInfo.bmiHeader.biWidth = largeSize;
    bitmapInfo.bmiHeader.biHeight = -largeSize;
    bitmapInfo.bmiHeader.biPlanes = 1;
    bitmapInfo.bmiHeader.biBitCount = 32;
    bitmapInfo.bmiHeader.biCompression = BI_RGB;

    if ((shieldBitmap = PhGetShieldBitmap(dpi, smallSize, smallSize)) &&
        (compositeDc = CreateCompatibleDC(NULL)) &&
        (sourceDc = CreateCompatibleDC(NULL)) &&
        (compositeBitmap = CreateDIBSection(compositeDc, &bitmapInfo, DIB_RGB_COLORS, &bits, NULL, 0)) &&
        (maskBitmap = CreateBitmap(largeSize, largeSize, 1, 1, NULL)))
    {
        oldComposite = SelectObject(compositeDc, compositeBitmap);

        oldSource = SelectObject(sourceDc, launcherBitmap);
        GdiAlphaBlend(compositeDc, 0, 0, largeSize, largeSize, sourceDc, 0, 0, largeSize, largeSize, blend);

        SelectObject(sourceDc, shieldBitmap);
        GdiAlphaBlend(compositeDc, largeSize - smallSize, largeSize - smallSize, smallSize, smallSize, sourceDc, 0, 0, smallSize, smallSize, blend);

        SelectObject(sourceDc, oldSource);
        SelectObject(compositeDc, oldComposite);

        memset(&iconInfo, 0, sizeof(ICONINFO));
        iconInfo.fIcon = TRUE;
        iconInfo.hbmMask = maskBitmap;
        iconInfo.hbmColor = compositeBitmap;
        result = CreateIconIndirect(&iconInfo);
    }

    if (maskBitmap)
        DeleteBitmap(maskBitmap);
    if (compositeBitmap)
        DeleteBitmap(compositeBitmap);
    if (sourceDc)
        DeleteDC(sourceDc);
    if (compositeDc)
        DeleteDC(compositeDc);
    if (shieldBitmap)
        DeleteBitmap(shieldBitmap);

    DeleteBitmap(launcherBitmap);

    return result;
}

PPH_STRING AtFormatCallerDescription(
    _In_ PAT_CONNECTION Connection
    )
{
    PH_STRING_BUILDER builder;

    PhInitializeStringBuilder(&builder, 256);

    PhAcquireQueuedLockExclusive(&Connection->Lock);

    if (Connection->ClientName)
    {
        PhAppendStringBuilder(&builder, &Connection->ClientName->sr);

        if (Connection->ClientVersion)
        {
            PhAppendCharStringBuilder(&builder, L' ');
            PhAppendStringBuilder(&builder, &Connection->ClientVersion->sr);
        }
    }
    else
    {
        PhAppendStringBuilder2(&builder, L"an MCP client that did not identify itself");
    }

    PhAppendStringBuilder2(&builder, L" (unverified) launched via ");
    PhAppendStringBuilder2(&builder, PhGetStringOrDefault(Connection->LauncherImageName, L"an unknown process"));
    PhAppendStringBuilder2(&builder, L" (unverified)");

    PhAppendFormatStringBuilder(
        &builder,
        L"; running as %s at %s integrity",
        PhGetStringOrDefault(Connection->UserName, L"the current user"),
        Connection->IntegrityString ? Connection->IntegrityString : L"unknown"
        );

    PhReleaseQueuedLockExclusive(&Connection->Lock);

    return PhFinalStringBuilderString(&builder);
}

VOID AtAudit(
    _In_ PAT_CONNECTION Connection,
    _In_ PCAT_ACTION_INFO Action,
    _In_opt_ PAT_TARGET Target,
    _In_ PCWSTR Outcome
    )
{
    PPH_STRING message;
    PPH_STRING client;
    PPH_STRING target = NULL;

    PhAcquireQueuedLockExclusive(&Connection->Lock);
    client = Connection->ClientName ? PhReferenceObject(Connection->ClientName) : NULL;
    PhReleaseQueuedLockExclusive(&Connection->Lock);

    if (Target && Target->Kind != AtTargetNone)
        target = AtFormatTargetAudit(Target);

    if (target)
    {
        message = PhFormatString(
            L"AgentTools: connection %u (%s, client %s) %s on %s%s%s: %s",
            Connection->ConnectionId,
            PhGetStringOrDefault(Connection->UserName, L"unknown user"),
            PhGetStringOrDefault(client, L"unidentified"),
            Action->AuditName,
            PhGetString(target),
            Target->Parameter ? L" to " : L"",
            Target->Parameter ? PhGetString(Target->Parameter) : L"",
            Outcome
            );
    }
    else
    {
        message = PhFormatString(
            L"AgentTools: connection %u (%s, client %s) %s: %s",
            Connection->ConnectionId,
            PhGetStringOrDefault(Connection->UserName, L"unknown user"),
            PhGetStringOrDefault(client, L"unidentified"),
            Action->AuditName,
            Outcome
            );
    }

    if (message)
    {
        PhLogMessageEntry(PH_LOG_ENTRY_MESSAGE, message);
        PhDereferenceObject(message);
    }

    PhClearReference(&target);
    PhClearReference(&client);
}

HRESULT CALLBACK AtpConsentDialogCallback(
    _In_ HWND WindowHandle,
    _In_ UINT Notification,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam,
    _In_ LONG_PTR Context
    )
{
    PAT_CONSENT_REQUEST request = (PAT_CONSENT_REQUEST)Context;

    switch (Notification)
    {
    case TDN_CREATED:
        {
            WritePointerRelease(&request->DialogHandle, WindowHandle);
            PhSetApplicationWindowIcon(WindowHandle);

            // The agent must not be able to read its own consent prompt through a screenshot or a
            // capture API; screen readers are unaffected.
            SetWindowDisplayAffinity(WindowHandle, WDA_EXCLUDEFROMCAPTURE);

            // Where the user is looking, not where the main window sits: a consent prompt that
            // comes up on another monitor is a consent prompt that gets missed.
            AtpCenterWindowOnUserMonitor(WindowHandle);

            // A custom main icon suppresses the task dialog's own sound.
            MessageBeep(MB_ICONEXCLAMATION);

            // Topmost because the foreground cannot be relied on: a worker thread may not take it
            // while the user is typing elsewhere. Raised without taking the input focus.
            SetWindowPos(WindowHandle, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);

            // The countdown and the waiter's bound both run from here, not from submission.
            request->ShownTick = NtGetTickCount64();
            WriteRelease(&request->Shown, 1);

            if (request->OfferPolicies)
                AtpCreateSessionPolicyControls(WindowHandle, request);

            if (ReadAcquire(&request->Abandoned))
                SendMessage(WindowHandle, TDM_CLICK_BUTTON, IDNO, 0);
        }
        break;
    case TDN_DESTROYED:
        // Cleared here rather than after the dialog call returns, so nothing can post to a handle
        // the system is free to recycle.
        WritePointerRelease(&request->DialogHandle, NULL);
        break;
    case TDN_BUTTON_CLICKED:
        {
            // The drop-down only means something with the action button.
            if (wParam == IDYES && request->ComboHandle)
            {
                LONG index = ComboBox_GetCurSel(request->ComboHandle);

                if (index != CB_ERR)
                    request->Policy = (AT_SESSION_POLICY)ComboBox_GetItemData(request->ComboHandle, index);
            }
        }
        break;
    case TDN_TIMER:
        {
            ULONG elapsed = (ULONG)wParam;

            // Raise the dialog once it is on screen. A worker thread may not take the foreground
            // while the user is typing elsewhere; then the taskbar button flashes instead.
            if (!request->Raised)
            {
                request->Raised = TRUE;
                SetForegroundWindow(WindowHandle);

                if (GetForegroundWindow() != WindowHandle)
                {
                    FLASHWINFO flash;

                    memset(&flash, 0, sizeof(FLASHWINFO));
                    flash.cbSize = sizeof(FLASHWINFO);
                    flash.hwnd = WindowHandle;
                    flash.dwFlags = FLASHW_ALL | FLASHW_TIMERNOFG;
                    FlashWindowEx(&flash);
                }
            }

            // Bounded wait: deny when nobody answers.
            if (elapsed >= AT_CONSENT_TIMEOUT_MS || ReadAcquire(&request->Abandoned))
            {
                if (elapsed >= AT_CONSENT_TIMEOUT_MS)
                    request->TimedOut = TRUE;

                SendMessage(WindowHandle, TDM_CLICK_BUTTON, IDNO, 0);
            }
            else
            {
                PH_FORMAT format[3];
                PPH_STRING footer;

                PhInitFormatS(&format[0], L"Automatically denied in ");
                PhInitFormatU(&format[1], (AT_CONSENT_TIMEOUT_MS - elapsed + 999) / 1000);
                PhInitFormatS(&format[2], L" seconds.");
                footer = PhFormat(format, RTL_NUMBER_OF(format), 64);
                SendMessage(WindowHandle, TDM_UPDATE_ELEMENT_TEXT, TDE_FOOTER, (LPARAM)footer->Buffer);

                // Footer is what config.pszFooter points at for the life of the dialog; the
                // countdown replaces this one instead.
                PhMoveReference(&request->FooterUpdate, footer);
            }
        }
        break;
    }

    return S_OK;
}

typedef struct _AT_BUTTON_ROW
{
    RECT Rect;
    HWND ButtonHandle;
    HWND DialogHandle;
} AT_BUTTON_ROW, *PAT_BUTTON_ROW;

BOOL CALLBACK AtpFindLeftmostButton(
    _In_ HWND WindowHandle,
    _In_ LPARAM Context
    )
{
    PAT_BUTTON_ROW row = (PAT_BUTTON_ROW)Context;
    WCHAR className[16];
    RECT rect;

    // TDN_CREATED arrives before the dialog is shown, so test the style bit, not IsWindowVisible.
    if ((GetWindowLongPtr(WindowHandle, GWL_STYLE) & WS_VISIBLE) &&
        GetClassName(WindowHandle, className, RTL_NUMBER_OF(className)) &&
        PhEqualStringZ(className, L"Button", TRUE) &&
        (GetWindowLongPtr(WindowHandle, GWL_STYLE) & BS_TYPEMASK) <= BS_DEFPUSHBUTTON)
    {
        GetWindowRect(WindowHandle, &rect);
        MapWindowPoints(NULL, row->DialogHandle, (PPOINT)&rect, 2);

        if (!row->ButtonHandle || rect.left < row->Rect.left)
        {
            row->Rect = rect;
            row->ButtonHandle = WindowHandle;
        }
    }

    return TRUE;
}

VOID AtpCreateSessionPolicyControls(
    _In_ HWND DialogHandle,
    _In_ PAT_CONSENT_REQUEST Request
    )
{
    static PCWSTR longLabel = L"Authorization this session:";
    static PCWSTR shortLabel = L"This session:";
    static PCWSTR items[] = { L"Ask every time", L"Allow for this session", L"Delegate to client" };
    AT_BUTTON_ROW row;
    PCWSTR labelText;
    HFONT font;
    LONG dpi;
    HDC hdc;
    HGDIOBJ oldFont = NULL;
    RECT textRect;
    LONG margin;
    LONG gap;
    LONG labelWidth;
    LONG comboWidth;
    LONG itemWidth = 0;
    LONG available;
    ULONG i;
    HWND label;
    HWND combo;
    LONG index;

    memset(&row, 0, sizeof(AT_BUTTON_ROW));
    row.DialogHandle = DialogHandle;
    EnumChildWindows(DialogHandle, AtpFindLeftmostButton, (LPARAM)&row);

    if (!row.ButtonHandle)
        return;

    font = GetWindowFont(row.ButtonHandle);
    dpi = PhGetWindowDpi(DialogHandle);
    margin = PhMultiplyDivideSigned(12, dpi, USER_DEFAULT_SCREEN_DPI);
    gap = PhMultiplyDivideSigned(6, dpi, USER_DEFAULT_SCREEN_DPI);
    available = row.Rect.left - margin - PhMultiplyDivideSigned(16, dpi, USER_DEFAULT_SCREEN_DPI);

    hdc = GetDC(DialogHandle);
    if (font)
        oldFont = SelectObject(hdc, font);

    // The drop-down is as wide as its widest item plus the arrow and the edit padding.
    for (i = 0; i < RTL_NUMBER_OF(items); i++)
    {
        memset(&textRect, 0, sizeof(RECT));
        DrawText(hdc, items[i], -1, &textRect, DT_CALCRECT | DT_SINGLELINE | DT_NOPREFIX);
        itemWidth = max(itemWidth, textRect.right);
    }

    comboWidth = itemWidth + PhGetSystemMetrics(SM_CXVSCROLL, dpi) + PhMultiplyDivideSigned(14, dpi, USER_DEFAULT_SCREEN_DPI);

    // The long label when the row has room for it and the drop-down, else the short one.
    labelText = longLabel;
    memset(&textRect, 0, sizeof(RECT));
    DrawText(hdc, labelText, -1, &textRect, DT_CALCRECT | DT_SINGLELINE | DT_NOPREFIX);
    labelWidth = textRect.right;

    if (labelWidth + gap + comboWidth > available)
    {
        labelText = shortLabel;
        memset(&textRect, 0, sizeof(RECT));
        DrawText(hdc, labelText, -1, &textRect, DT_CALCRECT | DT_SINGLELINE | DT_NOPREFIX);
        labelWidth = textRect.right;
    }

    if (oldFont)
        SelectObject(hdc, oldFont);
    ReleaseDC(DialogHandle, hdc);

    comboWidth = min(comboWidth, available - labelWidth - gap);

    if (comboWidth < PhMultiplyDivideSigned(80, dpi, USER_DEFAULT_SCREEN_DPI))
        return;

    label = CreateWindowEx(
        0,
        WC_STATIC,
        labelText,
        WS_CHILD | WS_VISIBLE | SS_LEFT | SS_CENTERIMAGE,
        margin,
        row.Rect.top,
        labelWidth,
        row.Rect.bottom - row.Rect.top,
        DialogHandle,
        NULL,
        PluginInstance->DllBase,
        NULL
        );
    combo = CreateWindowEx(
        0,
        WC_COMBOBOX,
        NULL,
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_VSCROLL | CBS_DROPDOWNLIST,
        margin + labelWidth + gap,
        row.Rect.top,
        comboWidth,
        (row.Rect.bottom - row.Rect.top) + PhMultiplyDivideSigned(80, dpi, USER_DEFAULT_SCREEN_DPI), // includes the list
        DialogHandle,
        NULL,
        PluginInstance->DllBase,
        NULL
        );

    if (!label || !combo)
        return;

    if (font)
    {
        SetWindowFont(label, font, FALSE);
        SetWindowFont(combo, font, FALSE);
    }

    // Above the DirectUI surface, which clips its siblings.
    SetWindowPos(label, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
    SetWindowPos(combo, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);

    // The choice lasts for this connection; the persistent setting lives in Options.
    index = ComboBox_AddString(combo, items[0]);
    ComboBox_SetItemData(combo, index, AtSessionAsk);
    index = ComboBox_AddString(combo, items[1]);
    ComboBox_SetItemData(combo, index, AtSessionAllow);

    if (Request->OfferDelegate)
    {
        index = ComboBox_AddString(combo, items[2]);
        ComboBox_SetItemData(combo, index, AtSessionDelegate);
    }

    ComboBox_SetCurSel(combo, 0);

    // The dialog was themed before these controls existed. In dark mode, run theming again so the
    // drop-down gets its subclass and the label's parent answers WM_CTLCOLORSTATIC in theme colors.
    if (PhGetIntegerSetting(SETTING_ENABLE_THEME_SUPPORT))
        PhInitializeWindowTheme(DialogHandle, TRUE);

    Request->ComboHandle = combo;
}

VOID AtpCompleteConnectionRequest(
    _In_ PAT_CONSENT_REQUEST Request
    )
{
    PAT_CONNECTION connection = Request->Connection;
    PCWSTR reason;

    // The connection went away before anyone answered: nothing to record.
    if (ReadAcquire(&Request->Abandoned) && !Request->TimedOut)
        return;

    if (Request->Allowed)
    {
        WriteRelease((PLONG)&connection->Approval, AtApprovalAllowed);
        AtAudit(connection, &AtActionInfo[AtActionConnect], NULL, L"allowed by the user");
        return;
    }

    WriteRelease((PLONG)&connection->Approval, AtApprovalDenied);

    if (Request->Failed)
        reason = L"denied (the confirmation could not be shown)";
    else if (Request->TimedOut)
        reason = L"denied (no answer in time)";
    else
        reason = L"denied by the user";

    AtAudit(connection, &AtActionInfo[AtActionConnect], NULL, reason);

    if (Request->TimedOut)
    {
        PhShowIconNotification(
            L"Agent connection denied",
            L"An agent connected to System Informer and nobody allowed it in time. It was disconnected."
            );
    }

    AtConnectionClose(connection, SimcpCloseRejected,
        Request->Failed ? SimcpHelloRejectedInternal : SimcpHelloRejectedByUser);
}

_Function_class_(USER_THREAD_START_ROUTINE)
NTSTATUS NTAPI AtpConsentDialogWorker(
    _In_ PVOID Parameter
    )
{
    PAT_CONSENT_REQUEST request = Parameter;
    TASKDIALOGCONFIG config;
    TASKDIALOG_BUTTON buttons[2];
    ULONG button = IDNO;

    if (!ReadAcquire(&request->Abandoned))
    {
        // Built now rather than at the handshake: by the time the dialog comes up the client has
        // usually sent initialize, so it can be named.
        if (request->ConnectionRequest)
            PhMoveReference(&request->Content, AtpFormatConnectionRequester(request->Connection));

        // No owner on purpose: an owner is disabled for the dialog's lifetime, and the user may
        // want to inspect the target in the main window before answering.
        memset(&config, 0, sizeof(TASKDIALOGCONFIG));
        config.cbSize = sizeof(TASKDIALOGCONFIG);
        config.hInstance = PluginInstance->DllBase;
        config.dwFlags = TDF_ALLOW_DIALOG_CANCELLATION | TDF_CALLBACK_TIMER | TDF_SIZE_TO_CONTENT;
        config.pszWindowTitle = L"System Informer";

        // The requester's icon badged with a shield; the stock shield when there is none.
        if (request->MainIcon)
        {
            config.dwFlags |= TDF_USE_HICON_MAIN;
            config.hMainIcon = request->MainIcon;
        }
        else
        {
            config.pszMainIcon = TD_SHIELD_ICON;
        }
        config.pszMainInstruction = PhGetString(request->Instruction);
        config.pszContent = PhGetString(request->Content);
        config.pszFooter = PhGetString(request->Footer);
        config.pfCallback = AtpConsentDialogCallback;
        config.lpCallbackData = (LONG_PTR)request;
        config.cxWidth = 250;


        buttons[0].nButtonID = IDYES;
        buttons[0].pszButtonText = request->AcceptText ? request->AcceptText : L"Approve";
        buttons[1].nButtonID = IDNO;
        buttons[1].pszButtonText = request->DeclineText ? request->DeclineText : L"Deny";
        config.cButtons = RTL_NUMBER_OF(buttons);
        config.pButtons = buttons;
        config.nDefaultButton = IDNO;

        // A dialog that could not be shown is not a decision, and must not be reported as one.
        if (!PhShowTaskDialog(&config, &button, NULL, NULL))
        {
            request->Failed = TRUE;
            button = IDNO;
        }
    }

    // A late answer is not a decision: the waiter already denied.
    if (!ReadAcquire(&request->Abandoned))
        request->Allowed = button == IDYES;

    request->ComboHandle = NULL;

    WritePointerRelease(&request->DialogHandle, NULL);

    if (request->ConnectionRequest)
        AtpCompleteConnectionRequest(request);

    PhSetEvent(&request->CompletedEvent);

    return STATUS_SUCCESS;
}

VOID NTAPI AtpConsentDialogDeleteItem(
    _In_ PUSER_THREAD_START_ROUTINE Function,
    _In_ PVOID Context
    )
{
    AtpDereferenceConsentRequest(Context);
}

PAT_CONSENT_REQUEST AtpCreateConsentRequest(
    _In_ PCAT_ACTION_INFO Action,
    _In_ PAT_CONNECTION Connection
    )
{
    PAT_CONSENT_REQUEST request;

    request = PhAllocateZero(sizeof(AT_CONSENT_REQUEST));
    request->RefCount = 2; // the caller and the dialog work item
    PhInitializeEvent(&request->CompletedEvent);
    request->Action = Action;
    request->MainIcon = AtpCreateRequestIcon(Connection);
    request->Footer = PhFormatString(L"Automatically denied in %u seconds.", AT_CONSENT_TIMEOUT_MS / 1000);

    return request;
}

VOID AtpSubmitConsentRequest(
    _In_ PAT_CONSENT_REQUEST Request
    )
{
    Request->SubmitTick = NtGetTickCount64();
    PhQueueItemWorkQueueEx(&AtConsentWorkQueue, AtpConsentDialogWorker, Request, AtpConsentDialogDeleteItem, NULL);
}

AT_CONSENT_RESULT AtpWaitForConsentRequest(
    _In_ PAT_CONNECTION Connection,
    _In_ PAT_CONSENT_REQUEST Request,
    _In_ BOOLEAN AbandonOnCancel,
    _Out_ AT_SESSION_POLICY* Policy
    )
{
    AT_CONSENT_RESULT result = AtConsentTimeout;
    LARGE_INTEGER timeout;
    HWND dialogHandle;

    *Policy = AtSessionAsk;

    while (TRUE)
    {
        if (PhWaitForEvent(&Request->CompletedEvent, PhTimeoutFromMilliseconds(&timeout, 250)))
        {
            if (Request->TimedOut)
                result = AtConsentTimeout;
            else if (Request->Failed)
                result = AtConsentFailed;
            else
                result = Request->Allowed ? AtConsentAllowed : AtConsentDenied;

            *Policy = Request->Policy;
            break;
        }

        if (AtMcpPumpDuringWait(Connection) || AtConnectionIsClosing(Connection))
        {
            // The client cancelled the call (or went away): close the dialog, nothing to send.
            if (AbandonOnCancel || AtConnectionIsClosing(Connection))
            {
                WriteRelease(&Request->Abandoned, 1);
                if (dialogHandle = ReadPointerAcquire(&Request->DialogHandle))
                    PostMessage(dialogHandle, TDM_CLICK_BUTTON, IDNO, 0);
            }

            result = AtConsentCancelled;
            break;
        }

        // The dialog denies itself at the bound once shown; until then it may be queued behind
        // other requests, so the wait is bounded from submission as well.
        if (ReadAcquire(&Request->Shown) ?
            NtGetTickCount64() - Request->ShownTick > AT_CONSENT_TIMEOUT_MS + 5000 :
            NtGetTickCount64() - Request->SubmitTick > AT_CONSENT_QUEUE_TIMEOUT_MS)
        {
            Request->TimedOut = TRUE;
            WriteRelease(&Request->Abandoned, 1);
            if (dialogHandle = ReadPointerAcquire(&Request->DialogHandle))
                PostMessage(dialogHandle, TDM_CLICK_BUTTON, IDNO, 0);
            result = AtConsentTimeout;
            break;
        }
    }

    return result;
}

AT_CONSENT_RESULT AtpAskUser(
    _In_ PAT_TOOL_CALL Call,
    _In_ PCAT_ACTION_INFO Action,
    _In_opt_ PAT_TARGET Target,
    _Out_ AT_SESSION_POLICY* Policy
    )
{
    PAT_CONNECTION connection = Call->Connection;
    PAT_CONSENT_REQUEST request;
    PPH_STRING requester;
    AT_CONSENT_RESULT result;

    requester = AtpFormatRequester(connection);
    request = AtpCreateConsentRequest(Action, connection);

    // The headline is the one thing to check: the action, its target and, when the action sets a
    // value, that value. The body is who asks.
    if (Target && Target->Kind != AtTargetNone)
    {
        PPH_STRING headline = AtFormatTargetHeadline(Target);

        request->Instruction = PhFormatString(
            L"%s %s%s%s?",
            Action->Headline,
            PhGetString(headline),
            Target->Parameter ? L" to " : L"",
            Target->Parameter ? PhGetString(Target->Parameter) : L""
            );
        PhDereferenceObject(headline);
    }
    else
    {
        request->Instruction = PhFormatString(L"%s?", Action->Headline);
    }

    // Every tool asks the same way; the drop-down decides whether this connection is asked again.
    request->OfferPolicies = TRUE;
    request->OfferDelegate = Call->ClientElicitation;

    if (Action->Tier == AtTierNetworkEgress)
        request->Content = PhFormatString(L"%s\n\nThis sends the request off this machine to a service on the internet.", PhGetString(requester));
    else
        request->Content = PhReferenceObject(requester);

    PhDereferenceObject(requester);

    AtpSubmitConsentRequest(request);
    result = AtpWaitForConsentRequest(connection, request, TRUE, Policy);

    if (result == AtConsentTimeout)
    {
        PhShowIconNotification(
            L"Agent request denied",
            L"An agent asked System Informer to perform an action and nobody confirmed it in time. The request was denied."
            );
    }

    AtpDereferenceConsentRequest(request);

    return result;
}

/**
 * Confirms a standing grant on its own, after the request that offered it was approved.
 *
 * \return TRUE if the grant was confirmed. A refusal, a timeout, or a dialog that could not be
 * shown all return FALSE, which withholds the grant without affecting the approved request.
 */
BOOLEAN AtpConfirmSessionPolicy(
    _In_ PAT_TOOL_CALL Call,
    _In_ PCAT_ACTION_INFO Action,
    _In_ AT_SESSION_POLICY Policy
    )
{
    static CONST PH_STRINGREF undo = PH_STRINGREF_INIT(
        L"\n\nRevoke grants on the Agents options page, or disconnecting the agent, undoes it.");
    PAT_CONNECTION connection = Call->Connection;
    PAT_CONSENT_REQUEST request;
    AT_SESSION_POLICY policy;
    AT_CONSENT_RESULT result;
    PPH_STRING content;
    PCWSTR scope;

    request = AtpCreateConsentRequest(Action, connection);
    PhMoveReference(&request->Footer,
        PhFormatString(L"No answer in %u seconds keeps asking each time.", AT_CONSENT_TIMEOUT_MS / 1000));

    if (Policy == AtSessionDelegate)
    {
        request->Instruction = PhCreateString(L"Let the client confirm this from now on?");
        content = PhFormatString(
            L"Later requests to %s are confirmed by the connected client instead of by System "
            L"Informer, which cannot check how the client presents them.",
            Action->Verb
            );
        request->AcceptText = L"Let the client ask";
        request->DeclineText = L"Keep asking here";
    }
    else
    {
        request->Instruction = PhCreateString(L"Stop asking about this for this connection?");

        // A classed grant covers every tool in the class, and the description says so.
        if (scope = AtConsentClassDescription(Action->Class))
            content = PhFormatString(L"This connection will be able to %s without being asked again.", scope);
        else
            content = PhFormatString(L"This connection will be able to %s, against any target, without being asked again.", Action->Verb);

        request->AcceptText = L"Stop asking";
        request->DeclineText = L"Ask each time";
    }

    request->Content = PhConcatStringRef2(&content->sr, &undo);
    PhDereferenceObject(content);

    AtpSubmitConsentRequest(request);
    result = AtpWaitForConsentRequest(connection, request, TRUE, &policy);
    AtpDereferenceConsentRequest(request);

    return result == AtConsentAllowed;
}

PPH_STRING AtpFormatConnectionRequester(
    _In_ PAT_CONNECTION Connection
    )
{
    PH_STRING_BUILDER builder;
    PPH_STRING launcher = NULL;

    AtpEnsureLauncherVerified(Connection);

    PhInitializeStringBuilder(&builder, 256);

    PhAcquireQueuedLockExclusive(&Connection->Lock);

    // The client's own name when it has sent initialize already; the verified launcher always.
    if (Connection->ClientName)
    {
        PhAppendStringBuilder2(&builder, L"Requested by ");
        PhAppendStringBuilder(&builder, &Connection->ClientName->sr);

        if (Connection->ClientVersion)
        {
            PhAppendCharStringBuilder(&builder, L' ');
            PhAppendStringBuilder(&builder, &Connection->ClientVersion->sr);
        }

        PhAppendCharStringBuilder(&builder, L'\n');
    }

    if (Connection->LauncherImageName)
        launcher = PhGetBaseName(Connection->LauncherImageName);

    PhAppendFormatStringBuilder(
        &builder,
        L"Process: %s (PID %lu)",
        PhGetStringOrDefault(launcher, L"unknown"),
        Connection->LauncherProcessId
        );
    PhClearReference(&launcher);

    if (Connection->LauncherVerifyChecked && Connection->LauncherVerifyResult == VrTrusted)
        PhAppendFormatStringBuilder(&builder, L"\nSigner: %s (trusted)", PhGetStringOrDefault(Connection->LauncherSignerName, L"unknown"));
    else if (!Connection->LauncherVerifyChecked || Connection->LauncherVerifyResult == VrUnknown)
        PhAppendStringBuilder2(&builder, L"\nSigner: not verified");
    else
        PhAppendStringBuilder2(&builder, L"\nSigner: not trusted");

    PhAppendFormatStringBuilder(&builder, L"\nUser: %s", PhGetStringOrDefault(Connection->UserName, L"unknown"));

    PhReleaseQueuedLockExclusive(&Connection->Lock);

    return PhFinalStringBuilderString(&builder);
}

VOID AtConsentRequestConnection(
    _In_ PAT_CONNECTION Connection
    )
{
    PCAT_ACTION_INFO action = &AtActionInfo[AtActionConnect];
    PAT_CONSENT_REQUEST request;

    if (PhGetIntegerSetting(action->ConfirmSetting) == AT_CONFIRM_NONE)
    {
        WriteRelease((PLONG)&Connection->Approval, AtApprovalAllowed);
        return;
    }

    request = AtpCreateConsentRequest(action, Connection);
    request->ConnectionRequest = TRUE;
    request->Connection = PhReferenceObject(Connection);
    request->Instruction = PhFormatString(L"%s?", action->Headline);
    // Content is built when the dialog comes up, so the client's name can be included.

    Connection->ApprovalRequest = request;
    AtpSubmitConsentRequest(request);
}

AT_CONSENT_RESULT AtConsentWaitForConnection(
    _In_ PAT_CONNECTION Connection
    )
{
    PAT_CONSENT_REQUEST request = Connection->ApprovalRequest;
    AT_CONSENT_RESULT result;
    AT_SESSION_POLICY policy;

    if (request)
    {
        result = AtpWaitForConsentRequest(Connection, request, FALSE, &policy);

        // The call was cancelled while the dialog is still up: keep it for the next call.
        if (result == AtConsentCancelled && !AtConnectionIsClosing(Connection))
            return AtConsentCancelled;

        Connection->ApprovalRequest = NULL;
        AtpDereferenceConsentRequest(request);
    }

    return ReadAcquire((PLONG)&Connection->Approval) == AtApprovalAllowed ? AtConsentAllowed : AtConsentDenied;
}

VOID AtConsentReleaseConnection(
    _In_ PAT_CONNECTION Connection
    )
{
    PAT_CONSENT_REQUEST request = Connection->ApprovalRequest;
    HWND dialogHandle;

    if (!request)
        return;

    WriteRelease(&request->Abandoned, 1);
    if (dialogHandle = ReadPointerAcquire(&request->DialogHandle))
        PostMessage(dialogHandle, TDM_CLICK_BUTTON, IDNO, 0);

    Connection->ApprovalRequest = NULL;
    AtpDereferenceConsentRequest(request);
}

PCWSTR AtConsentClassDescription(
    _In_ AT_CONSENT_CLASS Class
    )
{
    switch (Class)
    {
    case AtConsentClassHandleNames:
        return L"reading the names of the objects behind handles, in any process";
    case AtConsentClassThreadStacks:
        return L"reading thread stacks and their symbols, in any process";
    case AtConsentClassProcessMemory:
        return L"reading the contents of process memory, in any process";
    }

    return NULL;
}

AT_SESSION_POLICY AtpGetSessionPolicy(
    _In_ PAT_CONNECTION Connection,
    _In_ PCAT_ACTION_INFO Action
    )
{
    AT_SESSION_POLICY policy;

    PhAcquireQueuedLockExclusive(&Connection->Lock);

    if (Action->Class != AtConsentClassNone)
        policy = Connection->ClassPolicy[Action->Class];
    else
        policy = Connection->SessionPolicy[Action->Action];

    PhReleaseQueuedLockExclusive(&Connection->Lock);

    return policy;
}

VOID AtpSetSessionPolicy(
    _In_ PAT_CONNECTION Connection,
    _In_ PCAT_ACTION_INFO Action,
    _In_ AT_SESSION_POLICY Policy
    )
{
    PhAcquireQueuedLockExclusive(&Connection->Lock);

    if (Action->Class != AtConsentClassNone)
        Connection->ClassPolicy[Action->Class] = Policy;
    else
        Connection->SessionPolicy[Action->Action] = Policy;

    PhReleaseQueuedLockExclusive(&Connection->Lock);
}

VOID AtConsentRevokeGrants(
    _In_ ULONG ConnectionId
    )
{
    PPH_LIST connections;
    ULONG i;

    connections = AtServerSnapshotConnections();

    if (!connections)
        return;

    for (i = 0; i < connections->Count; i++)
    {
        PAT_CONNECTION connection = connections->Items[i];

        if (connection->ConnectionId != ConnectionId)
            continue;

        PhAcquireQueuedLockExclusive(&connection->Lock);
        memset(connection->SessionPolicy, 0, sizeof(connection->SessionPolicy));
        memset(connection->ClassPolicy, 0, sizeof(connection->ClassPolicy));
        PhReleaseQueuedLockExclusive(&connection->Lock);

        AtAudit(connection, &AtActionInfo[AtActionConnect], NULL, L"session grants revoked");
    }

    PhDereferenceObject(connections);
}

AT_CONSENT_RESULT AtConsentGate(
    _In_ PAT_TOOL_CALL Call,
    _In_ PCAT_ACTION_INFO Action,
    _In_opt_ PAT_TARGET Target
    )
{
    PAT_CONNECTION connection = Call->Connection;
    AT_CONSENT_RESULT result;
    AT_SESSION_POLICY policy;
    ULONG confirm;

    confirm = PhGetIntegerSetting(Action->ConfirmSetting);

    // Authorization not required: chosen per tool by the user in Options. Writes and sensitive
    // reads are still audited by the caller.
    if (confirm == AT_CONFIRM_NONE)
        return AtConsentAllowed;

    // A grant already held by this connection, chosen in the dialog (or, for reads, through the
    // client's prompt). Revoke grants, or Disconnect, clears it from the options page.
    policy = AtpGetSessionPolicy(connection, Action);

    if (policy == AtSessionAllow)
        return AtConsentAllowed;

    if (confirm == AT_CONFIRM_ALWAYS && policy != AtSessionDelegate)
    {
        result = AtpAskUser(Call, Action, Target, &policy);

        // A standing grant is broader than the request that was just approved, so it is confirmed
        // on its own. Declining withholds only the grant: this call was already allowed.
        if (result == AtConsentAllowed && policy != AtSessionAsk)
        {
            if (AtpConfirmSessionPolicy(Call, Action, policy))
            {
                // The choice lasts for this connection only; Revoke grants voids it.
                AtpSetSessionPolicy(connection, Action, policy);
                AtAudit(
                    connection,
                    Action,
                    NULL,
                    policy == AtSessionAllow ? L"granted for this connection" : L"delegated to the client's prompt for this connection"
                    );
            }
            else
            {
                policy = AtSessionAsk;
                AtAudit(connection, Action, NULL, L"standing grant not confirmed; this call only");
            }
        }
    }
    else
    {
        // Dialogs are off for this action: the client's elicitation UI is the human check, and
        // without one the call is refused.
        result = AtMcpElicitConsent(Call, Action, Target);

        // The client's prompt offers no session choice. Its message says a read is granted for
        // the rest of the session, so honour that; writes are asked every time.
        if (result == AtConsentAllowed &&
            (Action->Tier == AtTierRead || Action->Tier == AtTierSensitiveRead))
        {
            AtpSetSessionPolicy(connection, Action, AtSessionAllow);
        }
    }

    switch (result)
    {
    case AtConsentAllowed:
        AtAudit(connection, Action, Target, L"allowed");
        break;
    case AtConsentDenied:
        AtAudit(connection, Action, Target, L"denied by the user");
        break;
    case AtConsentTimeout:
        AtAudit(connection, Action, Target, L"denied (no answer in time)");
        break;
    case AtConsentDeclined:
        AtAudit(connection, Action, Target, L"declined through the client");
        break;
    case AtConsentCancelled:
        AtAudit(connection, Action, Target, L"cancelled by the client");
        break;
    case AtConsentElicitationRequired:
        AtAudit(connection, Action, Target, L"refused (confirmation delegated to a client without elicitation)");
        break;
    case AtConsentInputRequired:
        AtAudit(connection, Action, Target, L"confirmation requested through the client");
        break;
    default:
        AtAudit(connection, Action, Target, L"confirmation failed");
        break;
    }

    return result;
}
