/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2011-2016
 *     dmex    2017-2023
 *
 */

#include <phapp.h>
#include <phsettings.h>

#include <shellapi.h>
#include <malloc.h>

#include <mainwnd.h>
#include <phplug.h>
#include <procprv.h>
#include <settings.h>

#include <mainwndp.h>
#include <notifico.h>
#include <notificop.h>

BOOLEAN PhNfMiniInfoEnabled = FALSE;
BOOLEAN PhNfMiniInfoPinned = FALSE;
BOOLEAN PhNfTransparencyEnabled = FALSE;
BOOLEAN PhNfPersistTrayIconPositionEnabled = FALSE;
PPH_LIST PhTrayIconItemList = NULL;

PH_NF_POINTERS PhNfpPointers;
PH_CALLBACK_REGISTRATION PhNfpProcessesUpdatedRegistration;
PH_NF_BITMAP PhNfpDefaultBitmapContext = { 0 };
PH_NF_BITMAP PhNfpBlackBitmapContext = { 0 };
HBITMAP PhNfpBlackBitmap = NULL;
HICON PhNfpBlackIcon = NULL;
HFONT PhNfpTrayIconFont = NULL;
GUID PhNfpTrayIconItemGuids[PH_TRAY_ICON_GUID_MAXIMUM];

static POINT IconClickLocation;
static PH_NF_MSG_SHOWMINIINFOSECTION_DATA IconClickShowMiniInfoSectionData;
static BOOLEAN IconClickUpDueToDown = FALSE;
static BOOLEAN IconDisableHover = FALSE;
static HANDLE PhpTrayIconThreadHandle = NULL;
static HANDLE PhpTrayIconEventHandle = NULL;
#ifdef PH_NF_ENABLE_WORKQUEUE
static SLIST_HEADER PhpTrayIconWorkQueueListHead;
#endif

VOID PhNfLoadStage1(
    VOID
    )
{
    PhTrayIconItemList = PhCreateList(20);

    PhNfpPointers.BeginBitmap = PhNfpBeginBitmap;
#ifdef PH_NF_ENABLE_WORKQUEUE
    RtlInitializeSListHead(&PhpTrayIconWorkQueueListHead);
#endif
}

VOID PhNfLoadSettings(
    VOID
    )
{
    PPH_STRING settingsString;
    PH_STRINGREF remaining;

    settingsString = PhGetStringSetting(L"IconSettings");

    if (PhIsNullOrEmptyString(settingsString))
        return;

    remaining = PhGetStringRef(settingsString);

    while (remaining.Length != 0)
    {
        PH_STRINGREF idPart;
        PH_STRINGREF flagsPart;
        PH_STRINGREF pluginNamePart;
        ULONG64 idInteger;
        ULONG64 flagsInteger;

        PhSplitStringRefAtChar(&remaining, L'|', &idPart, &remaining);
        PhSplitStringRefAtChar(&remaining, L'|', &flagsPart, &remaining);
        PhSplitStringRefAtChar(&remaining, L'|', &pluginNamePart, &remaining);

        if (!PhStringToInteger64(&idPart, 10, &idInteger))
            break;
        if (!PhStringToInteger64(&flagsPart, 10, &flagsInteger))
            break;

        if (flagsInteger)
        {
            PPH_NF_ICON icon;

            if (pluginNamePart.Length)
            {
                if (icon = PhNfFindIcon(&pluginNamePart, (ULONG)idInteger))
                {
                    RtlInterlockedSetBits(&icon->Flags, PH_NF_ICON_ENABLED);
                }
            }
            else
            {
                if (icon = PhNfGetIconById((ULONG)idInteger))
                {
                    RtlInterlockedSetBits(&icon->Flags, PH_NF_ICON_ENABLED);
                }
            }
        }
    }

    PhDereferenceObject(settingsString);
}

VOID PhNfSaveSettings(
    VOID
    )
{
    PPH_STRING settingsString;
    PH_STRING_BUILDER iconListBuilder;

    PhInitializeStringBuilder(&iconListBuilder, 200);

    for (ULONG i = 0; i < PhTrayIconItemList->Count; i++)
    {
        PPH_NF_ICON icon = PhTrayIconItemList->Items[i];

        if (BooleanFlagOn(icon->Flags, PH_NF_ICON_ENABLED))
        {
            PH_FORMAT format[6];
            SIZE_T returnLength;
            WCHAR buffer[PH_INT64_STR_LEN_1];

            // %lu|%lu|%s|
            PhInitFormatU(&format[0], icon->SubId);
            PhInitFormatC(&format[1], L'|');
            PhInitFormatU(&format[2], TRUE);
            PhInitFormatC(&format[3], L'|');
            if (icon->Plugin)
                PhInitFormatSR(&format[4], icon->Plugin->Name);
            else
                PhInitFormatS(&format[4], L"");
            PhInitFormatC(&format[5], L'|');

            if (PhFormatToBuffer(format, RTL_NUMBER_OF(format), buffer, sizeof(buffer), &returnLength))
            {
                PhAppendStringBuilderEx(&iconListBuilder, buffer, returnLength - sizeof(UNICODE_NULL));
            }
            else
            {
                PhAppendFormatStringBuilder(
                    &iconListBuilder,
                    L"%lu|%lu|%s|",
                    icon->SubId,
                    TRUE,
                    icon->Plugin ? PhGetStringRefZ(&icon->Plugin->Name) : L""
                    );
            }
        }
    }

    if (iconListBuilder.String->Length != 0)
        PhRemoveEndStringBuilder(&iconListBuilder, 1);

    settingsString = PhFinalStringBuilderString(&iconListBuilder);
    PhSetStringSetting2(L"IconSettings", &settingsString->sr);
    PhDereferenceObject(settingsString);
}

VOID PhNfLoadGuids(
    VOID
    )
{
    PPH_STRING settingsString;
    PH_STRINGREF remaining;
    ULONG i;

    settingsString = PhGetStringSetting(L"IconTrayGuids");

    if (PhIsNullOrEmptyString(settingsString))
    {
        PH_STRING_BUILDER iconListBuilder;
        PPH_STRING iconGuid;

        PhInitializeStringBuilder(&iconListBuilder, 100);

        for (i = 0; i < RTL_NUMBER_OF(PhNfpTrayIconItemGuids); i++)
        {
            PhGenerateGuid(&PhNfpTrayIconItemGuids[i]);

            if (iconGuid = PhFormatGuid(&PhNfpTrayIconItemGuids[i]))
            {
                PhAppendFormatStringBuilder(
                    &iconListBuilder,
                    L"%s|",
                    iconGuid->Buffer
                    );
                PhDereferenceObject(iconGuid);
            }
        }

        if (iconListBuilder.String->Length != 0)
            PhRemoveEndStringBuilder(&iconListBuilder, 1);

        PhMoveReference(&settingsString, PhFinalStringBuilderString(&iconListBuilder));
        PhSetStringSetting2(L"IconTrayGuids", &settingsString->sr);
        PhDereferenceObject(settingsString);
    }
    else
    {
        remaining = PhGetStringRef(settingsString);

        for (i = 0; i < RTL_NUMBER_OF(PhNfpTrayIconItemGuids); i++)
        {
            PH_STRINGREF guidPart;
            GUID guid;

            if (remaining.Length == 0)
                continue;

            PhSplitStringRefAtChar(&remaining, L'|', &guidPart, &remaining);

            if (guidPart.Length == 0)
                continue;

            if (!NT_SUCCESS(PhStringToGuid(&guidPart, &guid)))
                PhGenerateGuid(&PhNfpTrayIconItemGuids[i]);
            else
                PhNfpTrayIconItemGuids[i] = guid;
        }

        PhDereferenceObject(settingsString);
    }
}

VOID PhNfCreateIconThreadDelayed(
    VOID
    )
{
    static PH_INITONCE initOnce = PH_INITONCE_INIT;
    PPH_NF_ICON staticIcon = NULL;
    ULONG iconCount = 0;

    for (ULONG i = 0; i < PhTrayIconItemList->Count; i++)
    {
        PPH_NF_ICON icon = PhTrayIconItemList->Items[i];

        if (!BooleanFlagOn(icon->Flags, PH_NF_ICON_ENABLED))
            continue;

        if (icon->SubId == PH_TRAY_ICON_ID_PLAIN_ICON)
            staticIcon = icon;

        iconCount++;
    }

    if (iconCount && PhBeginInitOnce(&initOnce))
    {
        if (NT_SUCCESS(NtCreateEvent(
            &PhpTrayIconEventHandle,
            EVENT_ALL_ACCESS,
            NULL,
            SynchronizationEvent,
            !PhGetIntegerSetting(L"IconTrayLazyStartDelay")
            )))
        {
            // Set the event when the only icon is the static icon. (dmex)
            if (iconCount == 1 && staticIcon)
            {
                NtSetEvent(PhpTrayIconEventHandle, NULL);
            }

            // Use a separate thread so we don't block the main GUI or
            // the provider threads when explorer is not responding. (dmex)
            PhCreateThreadEx(&PhpTrayIconThreadHandle, PhNfpTrayIconUpdateThread, NULL);
        }

        PhEndInitOnce(&initOnce);
    }
}

PPH_NF_ICON PhNfRegisterIcon(
    _In_opt_ struct _PH_PLUGIN* Plugin,
    _In_ ULONG Id,
    _In_ GUID Guid,
    _In_opt_ PVOID Context,
    _In_ PWSTR Text,
    _In_ ULONG Flags,
    _In_opt_ PPH_NF_ICON_UPDATE_CALLBACK UpdateCallback,
    _In_opt_ PPH_NF_ICON_MESSAGE_CALLBACK MessageCallback
    )
{
    PPH_NF_ICON icon;

    icon = PhAllocateZero(sizeof(PH_NF_ICON));
    icon->Plugin = Plugin;
    icon->SubId = Id;
    icon->Context = Context;
    icon->Pointers = &PhNfpPointers;
    icon->Text = Text;
    icon->Flags = Flags;
    icon->UpdateCallback = UpdateCallback;
    icon->MessageCallback = MessageCallback;
    icon->TextCache = PhReferenceEmptyString();
    icon->IconId = PhTrayIconItemList->Count + 1; // HACK
    icon->IconGuid = Guid;

    PhAddItemList(PhTrayIconItemList, icon);

    return icon;
}

PPH_NF_ICON PhNfPluginRegisterIcon(
    _In_ struct _PH_PLUGIN* Plugin,
    _In_ ULONG SubId,
    _In_ GUID Guid,
    _In_opt_ PVOID Context,
    _In_ PWSTR Text,
    _In_ ULONG Flags,
    _In_ struct _PH_NF_ICON_REGISTRATION_DATA* RegistrationData
    )
{
    return PhNfRegisterIcon(
        Plugin,
        SubId,
        Guid,
        Context,
        Text,
        Flags,
        RegistrationData->UpdateCallback,
        RegistrationData->MessageCallback
        );
}

VOID PhNfLoadStage2(
    VOID
    )
{
    PhNfMiniInfoEnabled = !!PhGetIntegerSetting(L"MiniInfoWindowEnabled");
    PhNfTransparencyEnabled = !!PhGetIntegerSetting(L"IconTransparencyEnabled");
    PhNfPersistTrayIconPositionEnabled = !!PhGetIntegerSetting(L"IconTrayPersistGuidEnabled");

    if (PhNfPersistTrayIconPositionEnabled)
    {
        PhNfLoadGuids();
    }

    PhNfRegisterIcon(NULL, PH_TRAY_ICON_ID_CPU_USAGE, PhNfpTrayIconItemGuids[PH_TRAY_ICON_GUID_CPU_USAGE], NULL, L"CPU &usage", 0, PhNfpCpuUsageIconUpdateCallback, NULL);
    PhNfRegisterIcon(NULL, PH_TRAY_ICON_ID_CPU_HISTORY, PhNfpTrayIconItemGuids[PH_TRAY_ICON_GUID_CPU_HISTORY], NULL, L"CPU &history", 0, PhNfpCpuHistoryIconUpdateCallback, NULL);
    PhNfRegisterIcon(NULL, PH_TRAY_ICON_ID_IO_HISTORY, PhNfpTrayIconItemGuids[PH_TRAY_ICON_GUID_IO_HISTORY], NULL, L"&I/O history", 0, PhNfpIoHistoryIconUpdateCallback, NULL);
    PhNfRegisterIcon(NULL, PH_TRAY_ICON_ID_COMMIT_HISTORY, PhNfpTrayIconItemGuids[PH_TRAY_ICON_GUID_COMMIT_HISTORY], NULL, L"&Commit charge history", 0, PhNfpCommitHistoryIconUpdateCallback, NULL);
    PhNfRegisterIcon(NULL, PH_TRAY_ICON_ID_PHYSICAL_HISTORY, PhNfpTrayIconItemGuids[PH_TRAY_ICON_GUID_PHYSICAL_HISTORY], NULL, L"&Physical memory history", 0, PhNfpPhysicalHistoryIconUpdateCallback, NULL);
    PhNfRegisterIcon(NULL, PH_TRAY_ICON_ID_CPU_TEXT, PhNfpTrayIconItemGuids[PH_TRAY_ICON_GUID_CPU_TEXT], NULL, L"CPU usage (text)", 0, PhNfpCpuUsageTextIconUpdateCallback, NULL);
    PhNfRegisterIcon(NULL, PH_TRAY_ICON_ID_IO_TEXT, PhNfpTrayIconItemGuids[PH_TRAY_ICON_GUID_IO_TEXT], NULL, L"IO usage (text)", 0, PhNfpIoUsageTextIconUpdateCallback, NULL);
    PhNfRegisterIcon(NULL, PH_TRAY_ICON_ID_COMMIT_TEXT, PhNfpTrayIconItemGuids[PH_TRAY_ICON_GUID_COMMIT_TEXT], NULL, L"Commit usage (text)", 0, PhNfpCommitTextIconUpdateCallback, NULL);
    PhNfRegisterIcon(NULL, PH_TRAY_ICON_ID_PHYSICAL_TEXT, PhNfpTrayIconItemGuids[PH_TRAY_ICON_GUID_PHYSICAL_TEXT], NULL, L"Physical usage (text)", 0, PhNfpPhysicalUsageTextIconUpdateCallback, NULL);
    PhNfRegisterIcon(NULL, PH_TRAY_ICON_ID_PLAIN_ICON, PhNfpTrayIconItemGuids[PH_TRAY_ICON_GUID_PLAIN_ICON], NULL, L"System Informer icon (static)", 0, PhNfpPlainIconUpdateCallback, NULL);

    if (PhPluginsEnabled)
    {
        PH_TRAY_ICON_POINTERS pointers;

        pointers.RegisterTrayIcon = (PPH_REGISTER_TRAY_ICON)PhNfPluginRegisterIcon;

        PhInvokeCallback(PhGetGeneralCallback(GeneralCallbackTrayIconsInitializing), &pointers);
    }

    // Load tray icon settings.
    PhNfLoadSettings();

    //for (ULONG i = 0; i < PhTrayIconItemList->Count; i++)
    //{
    //    PPH_NF_ICON icon = PhTrayIconItemList->Items[i];
    //
    //    if (!BooleanFlagOn(icon->Flags, PH_NF_ICON_ENABLED))
    //        continue;
    //
    //    PhNfpAddNotifyIcon(icon);
    //}

    PhNfCreateIconThreadDelayed();

    PhRegisterCallback(
        PhGetGeneralCallback(GeneralCallbackProcessProviderUpdatedEvent),
        PhNfpProcessesUpdatedHandler,
        NULL,
        &PhNfpProcessesUpdatedRegistration
        );
}

VOID PhNfUninitialization(
    VOID
    )
{
#ifdef PH_NF_ENABLE_WORKQUEUE
    if (PhpTrayIconEventHandle)
        NtSetEvent(PhpTrayIconEventHandle, NULL);
#endif

    // Remove all icons to prevent them hanging around after we exit.

    for (ULONG i = 0; i < PhTrayIconItemList->Count; i++)
    {
        PPH_NF_ICON icon = PhTrayIconItemList->Items[i];

        if (!BooleanFlagOn(icon->Flags, PH_NF_ICON_ENABLED))
            continue;

        if (RtlInterlockedClearBits(&icon->Flags, PH_NF_ICON_ENABLED) == PH_NF_ICON_ENABLED)
        {
            PhNfpRemoveNotifyIcon(icon);
        }
    }

//#ifdef PH_NF_ENABLE_WORKQUEUE
//    if (PhpTrayIconThreadHandle)
//    {
//        NtWaitForSingleObject(PhpTrayIconThreadHandle, FALSE, PhTimeoutFromMillisecondsEx(1000));
//    }
//#endif
}

VOID PhNfForwardMessage(
    _In_ HWND WindowHandle,
    _In_ ULONG_PTR WParam,
    _In_ ULONG_PTR LParam
    )
{
    ULONG iconIndex = HIWORD(LParam);
    PPH_NF_ICON registeredIcon = NULL;

    if (iconIndex == 0)
        return;

    for (ULONG i = 0; i < PhTrayIconItemList->Count; i++)
    {
        PPH_NF_ICON icon = PhTrayIconItemList->Items[i];

        if (icon->IconId == iconIndex)
        {
            registeredIcon = icon;
            break;
        }
    }

    if (!registeredIcon)
        return;

    if (registeredIcon->MessageCallback)
    {
        if (registeredIcon->MessageCallback(
            registeredIcon,
            WParam,
            LParam,
            registeredIcon->Context
            ))
        {
            return;
        }
    }

    switch (LOWORD(LParam))
    {
    case WM_LBUTTONDOWN:
        {
            if (PhGetIntegerSetting(L"IconSingleClick"))
            {
                ProcessHacker_IconClick();
                PhNfpDisableHover();
            }
            else
            {
                IconClickUpDueToDown = TRUE;
            }
        }
        break;
    case WM_LBUTTONUP:
        {
            if (!PhGetIntegerSetting(L"IconSingleClick") && PhNfMiniInfoEnabled && IconClickUpDueToDown)
            {
                PH_NF_MSG_SHOWMINIINFOSECTION_DATA showMiniInfoSectionData;

                if (PhNfpGetShowMiniInfoSectionData(iconIndex, registeredIcon, &showMiniInfoSectionData))
                {
                    GetCursorPos(&IconClickLocation);

                    if (IconClickShowMiniInfoSectionData.SectionName)
                    {
                        PhFree(IconClickShowMiniInfoSectionData.SectionName);
                        IconClickShowMiniInfoSectionData.SectionName = NULL;
                    }

                    if (showMiniInfoSectionData.SectionName)
                    {
                        IconClickShowMiniInfoSectionData.SectionName = PhDuplicateStringZ(showMiniInfoSectionData.SectionName);
                    }

                    SetTimer(WindowHandle, TIMER_ICON_CLICK_ACTIVATE, GetDoubleClickTime() + NFP_ICON_CLICK_ACTIVATE_DELAY, PhNfpIconClickActivateTimerProc);
                }
                else
                {
                    KillTimer(WindowHandle, TIMER_ICON_CLICK_ACTIVATE);
                }
            }
        }
        break;
    case WM_LBUTTONDBLCLK:
        {
            if (!PhGetIntegerSetting(L"IconSingleClick"))
            {
                if (PhNfMiniInfoEnabled)
                {
                    // We will get another WM_LBUTTONUP message corresponding to the double-click,
                    // and we need to make sure that it doesn't start the activation timer again.
                    KillTimer(WindowHandle, TIMER_ICON_CLICK_ACTIVATE);
                    IconClickUpDueToDown = FALSE;
                    PhNfpDisableHover();
                }

                ProcessHacker_IconClick();
            }
        }
        break;
    case WM_RBUTTONUP:
    case WM_CONTEXTMENU:
        {
            POINT location;

            if (!PhGetIntegerSetting(L"IconSingleClick") && PhNfMiniInfoEnabled)
                KillTimer(WindowHandle, TIMER_ICON_CLICK_ACTIVATE);

            PhPinMiniInformation(MiniInfoIconPinType, -1, 0, 0, NULL, NULL);
            GetCursorPos(&location);
            PhShowIconContextMenu(location);
        }
        break;
    case NIN_KEYSELECT:
        // HACK: explorer seems to send two NIN_KEYSELECT messages when the user selects the icon and presses ENTER.
        if (GetForegroundWindow() != WindowHandle)
            ProcessHacker_IconClick();
        break;
    case NIN_BALLOONUSERCLICK:
        if (!PhGetIntegerSetting(L"IconIgnoreBalloonClick"))
            PhShowDetailsForIconNotification();
        break;
    case NIN_POPUPOPEN:
        {
            PH_NF_MSG_SHOWMINIINFOSECTION_DATA showMiniInfoSectionData;
            POINT location;

            if (PhNfMiniInfoEnabled && !IconDisableHover && PhNfpGetShowMiniInfoSectionData(iconIndex, registeredIcon, &showMiniInfoSectionData))
            {
                GetCursorPos(&location);
                PhPinMiniInformation(MiniInfoIconPinType, 1, 0, PH_MINIINFO_DONT_CHANGE_SECTION_IF_PINNED,
                    showMiniInfoSectionData.SectionName, &location);
            }
        }
        break;
    case NIN_POPUPCLOSE:
        PhPinMiniInformation(MiniInfoIconPinType, -1, 350, 0, NULL, NULL);
        break;
    }
}

VOID PhNfSetVisibleIcon(
    _In_ PPH_NF_ICON Icon,
    _In_ BOOLEAN Visible
    )
{
    if (Visible)
    {
#ifndef PH_NF_ENABLE_WORKQUEUE
        RtlInterlockedSetBits(&Icon->Flags, PH_NF_ICON_ENABLED);

        PhNfpAddNotifyIcon(Icon);
#else
        PPH_NF_WORKQUEUE_DATA data;

        data = PhAllocateZero(sizeof(PH_NF_WORKQUEUE_DATA));
        data->Add = TRUE;
        data->Icon = Icon;

        RtlInterlockedPushEntrySList(&PhpTrayIconWorkQueueListHead, &data->ListEntry);
#endif
    }
    else
    {
#ifndef PH_NF_ENABLE_WORKQUEUE
        PhNfpRemoveNotifyIcon(Icon);

        RtlInterlockedClearBits(&Icon->Flags, PH_NF_ICON_ENABLED);
#else
        PPH_NF_WORKQUEUE_DATA data;

        data = PhAllocateZero(sizeof(PH_NF_WORKQUEUE_DATA));
        data->Delete = TRUE;
        data->Icon = Icon;

        RtlInterlockedPushEntrySList(&PhpTrayIconWorkQueueListHead, &data->ListEntry);
#endif
    }
#ifdef PH_NF_ENABLE_WORKQUEUE
    PhNfCreateIconThreadDelayed();

    if (PhpTrayIconEventHandle)
        NtSetEvent(PhpTrayIconEventHandle, NULL);
#endif
}

BOOLEAN PhNfShowBalloonTip(
    _In_ PWSTR Title,
    _In_ PWSTR Text,
    _In_ ULONG Timeout
    )
{
    NOTIFYICONDATA notifyIcon;
    PPH_NF_ICON registeredIcon = NULL;

    for (ULONG i = 0; i < PhTrayIconItemList->Count; i++)
    {
        PPH_NF_ICON icon = PhTrayIconItemList->Items[i];

        if (!BooleanFlagOn(icon->Flags, PH_NF_ICON_ENABLED))
            continue;

        registeredIcon = icon;
        break;
    }

    if (!registeredIcon)
        return FALSE;

    memset(&notifyIcon, 0, sizeof(NOTIFYICONDATA));
    notifyIcon.cbSize = sizeof(NOTIFYICONDATA);
    notifyIcon.uFlags = NIF_INFO;
    notifyIcon.hWnd = PhMainWndHandle;
    notifyIcon.uID = registeredIcon->IconId;

    if (PhNfPersistTrayIconPositionEnabled)
    {
        notifyIcon.uFlags |= NIF_GUID;
        notifyIcon.guidItem = registeredIcon->IconGuid;
    }

    wcsncpy_s(notifyIcon.szInfoTitle, RTL_NUMBER_OF(notifyIcon.szInfoTitle), Title, _TRUNCATE);
    wcsncpy_s(notifyIcon.szInfo, RTL_NUMBER_OF(notifyIcon.szInfo), Text, _TRUNCATE);
    notifyIcon.uTimeout = Timeout;
    notifyIcon.dwInfoFlags = NIIF_INFO;

    Shell_NotifyIcon(NIM_MODIFY, &notifyIcon);

    return TRUE;
}

HICON PhNfBitmapToIcon(
    _In_ HBITMAP Bitmap
    )
{
    ICONINFO iconInfo;

    PhNfpGetBlackIcon();

    iconInfo.fIcon = TRUE;
    iconInfo.xHotspot = 0;
    iconInfo.yHotspot = 0;
    iconInfo.hbmMask = PhNfpBlackBitmap;
    iconInfo.hbmColor = Bitmap;

    return CreateIconIndirect(&iconInfo);
}

PPH_NF_ICON PhNfGetIconById(
    _In_ ULONG SubId
    )
{
    for (ULONG i = 0; i < PhTrayIconItemList->Count; i++)
    {
        PPH_NF_ICON icon = PhTrayIconItemList->Items[i];

        if (icon->SubId == SubId)
            return icon;
    }

    return NULL;
}

PPH_NF_ICON PhNfFindIcon(
    _In_ PPH_STRINGREF PluginName,
    _In_ ULONG SubId
    )
{
    for (ULONG i = 0; i < PhTrayIconItemList->Count; i++)
    {
        PPH_NF_ICON icon = PhTrayIconItemList->Items[i];

        if (icon->Plugin)
        {
            if (icon->SubId == SubId &&
                PhEqualStringRef(PluginName, &icon->Plugin->Name, TRUE))
            {
                return icon;
            }
        }
    }

    return NULL;
}

BOOLEAN PhNfIconsEnabled(
    VOID
    )
{
    BOOLEAN enabled = FALSE;

    for (ULONG i = 0; i < PhTrayIconItemList->Count; i++)
    {
        PPH_NF_ICON icon = PhTrayIconItemList->Items[i];

        if (BooleanFlagOn(icon->Flags, PH_NF_ICON_ENABLED))
        {
            enabled = TRUE;
            break;
        }
    }

    return enabled;
}

VOID PhNfNotifyMiniInfoPinned(
    _In_ BOOLEAN Pinned
    )
{
    if (PhNfMiniInfoPinned != Pinned)
    {
        PhNfMiniInfoPinned = Pinned;

        // Go through every icon and set/clear the NIF_SHOWTIP flag depending on whether the mini info window is
        // pinned. If it's pinned then we want to show normal tooltips, because the section doesn't change
        // automatically when the cursor hovers over an icon.

        for (ULONG i = 0; i < PhTrayIconItemList->Count; i++)
        {
            PPH_NF_ICON icon = PhTrayIconItemList->Items[i];

            if (!BooleanFlagOn(icon->Flags, PH_NF_ICON_ENABLED))
                continue;

            PhNfpModifyNotifyIcon(icon, NIF_TIP, icon->TextCache, NULL);
        }
    }
}

//VOID PhpNfCreateTransparentHdc(
//    _Inout_ PRECT Rect,
//    _Out_ HDC *Hdc,
//    _Out_ HDC *MaskDc,
//    _Out_ HBITMAP *Bitmap,
//    _Out_ HBITMAP *MaskBitmap,
//    _Out_ HBITMAP *OldBitmap,
//    _Out_ HBITMAP *OldMaskBitmap
//    )
//{
//    HDC hdc;
//    HDC bufferDc;
//    HDC bufferMaskDc;
//    HBITMAP bufferBitmap;
//    HBITMAP bufferMaskBitmap;
//    HBITMAP oldBufferBitmap;
//    HBITMAP oldBufferMaskBitmap;
//
//    hdc = GetDC(NULL);
//    bufferDc = CreateCompatibleDC(hdc);
//    bufferBitmap = CreateCompatibleBitmap(hdc, Rect->right, Rect->bottom);
//    oldBufferBitmap = SelectBitmap(bufferDc, bufferBitmap);
//
//    bufferMaskDc = CreateCompatibleDC(hdc);
//    bufferMaskBitmap = CreateBitmap(Rect->right, Rect->bottom, 1, 1, NULL);
//    oldBufferMaskBitmap = SelectBitmap(bufferMaskDc, bufferMaskBitmap);
//
//    *Hdc = bufferDc;
//    *MaskDc = bufferMaskDc;
//    *Bitmap = bufferBitmap;
//    *MaskBitmap = bufferMaskBitmap;
//    *OldBitmap = oldBufferBitmap;
//    *OldMaskBitmap = oldBufferMaskBitmap;
//}
//
//HICON PhpNfTransparentHdcToIcon(
//    _Inout_ PRECT Rect,
//    _In_ HDC Hdc,
//    _In_ HDC MaskDc,
//    _In_ HBITMAP Bitmap,
//    _In_ HBITMAP MaskBitmap,
//    _In_ HBITMAP OldBitmap,
//    _In_ HBITMAP OldMaskBitmap
//    )
//{
//    HICON iconHandle;
//    ICONINFO iconInfo;
//
//    SetBkColor(Hdc, RGB(0, 0, 0)); // Set transparent color and draw the mask
//    BitBlt(MaskDc, 0, 0, Rect->right, Rect->bottom, Hdc, 0, 0, SRCCOPY);
//
//    SelectBitmap(Hdc, Bitmap);
//    SelectBitmap(MaskDc, MaskBitmap);
//
//    DeleteDC(Hdc);
//    DeleteDC(MaskDc);
//    ReleaseDC(NULL, Hdc);
//
//    iconInfo.fIcon = TRUE;
//    iconInfo.xHotspot = 0;
//    iconInfo.yHotspot = 0;
//    iconInfo.hbmMask = MaskBitmap;
//    iconInfo.hbmColor = Bitmap;
//
//    iconHandle = CreateIconIndirect(&iconInfo); // Create transparent icon
//
//    DeleteBitmap(MaskBitmap);
//    DeleteBitmap(Bitmap);
//
//    return iconHandle;
//}

HICON PhNfpGetBlackIcon(
    VOID
    )
{
    if (!PhNfpBlackIcon)
    {
        ULONG width;
        ULONG height;
        PVOID bits;
        HDC hdc;
        HBITMAP oldBitmap;
        ICONINFO iconInfo;

        PhNfpBeginBitmap2(&PhNfpBlackBitmapContext, &width, &height, &PhNfpBlackBitmap, &bits, &hdc, &oldBitmap);
        memset(bits, PhNfTransparencyEnabled ? 1 : 0, width * height * sizeof(RGBQUAD));

        iconInfo.fIcon = TRUE;
        iconInfo.xHotspot = 0;
        iconInfo.yHotspot = 0;
        iconInfo.hbmMask = PhNfpBlackBitmap;
        iconInfo.hbmColor = PhNfpBlackBitmap;
        PhNfpBlackIcon = CreateIconIndirect(&iconInfo);

        SelectBitmap(hdc, oldBitmap);
    }

    return PhNfpBlackIcon;
}

HFONT PhNfGetTrayIconFont(
    _In_opt_ LONG DpiValue
    )
{
    if (DpiValue != 0 && PhNfpTrayIconFont)
    {
        DeleteFont(PhNfpTrayIconFont);
        PhNfpTrayIconFont = NULL;
    }

    if (!PhNfpTrayIconFont)
    {
        if (DpiValue == 0)
        {
            DpiValue = PhGetTaskbarDpi();
        }

        PhNfpTrayIconFont = CreateFont(
            PhGetDpi(-11, DpiValue),
            0,
            0,
            0,
            FW_NORMAL,
            FALSE,
            FALSE,
            FALSE,
            ANSI_CHARSET,
            OUT_DEFAULT_PRECIS,
            CLIP_DEFAULT_PRECIS,
            ANTIALIASED_QUALITY,
            DEFAULT_PITCH,
            L"Tahoma"
            );
    }

    return PhNfpTrayIconFont;
}

BOOLEAN PhNfpAddNotifyIcon(
    _In_ PPH_NF_ICON Icon
    )
{
    NOTIFYICONDATA notifyIcon;

    if (PhMainWndExiting)
        return FALSE;

    memset(&notifyIcon, 0, sizeof(NOTIFYICONDATA));
    notifyIcon.cbSize = sizeof(NOTIFYICONDATA);
    notifyIcon.hWnd = PhMainWndHandle;
    notifyIcon.uID = Icon->IconId;
    notifyIcon.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    notifyIcon.uCallbackMessage = WM_PH_NOTIFY_ICON_MESSAGE;

    if (PhNfPersistTrayIconPositionEnabled)
    {
        notifyIcon.uFlags |= NIF_GUID;
        notifyIcon.guidItem = Icon->IconGuid;
    }

    wcsncpy_s(
        notifyIcon.szTip, RTL_NUMBER_OF(notifyIcon.szTip),
        PhGetStringOrDefault(Icon->TextCache, PhApplicationName),
        _TRUNCATE
        );
    notifyIcon.hIcon = PhNfpGetBlackIcon();

    if (!PhNfMiniInfoEnabled || PhNfMiniInfoPinned || (Icon->Flags & PH_NF_ICON_NOSHOW_MINIINFO))
        notifyIcon.uFlags |= NIF_SHOWTIP;

    Shell_NotifyIcon(NIM_ADD, &notifyIcon);
    notifyIcon.uVersion = NOTIFYICON_VERSION_4;
    Shell_NotifyIcon(NIM_SETVERSION, &notifyIcon);

    return TRUE;
}

BOOLEAN PhNfpRemoveNotifyIcon(
    _In_ PPH_NF_ICON Icon
    )
{
    NOTIFYICONDATA notifyIcon;

    memset(&notifyIcon, 0, sizeof(NOTIFYICONDATA));
    notifyIcon.cbSize = sizeof(NOTIFYICONDATA);
    notifyIcon.hWnd = PhMainWndHandle;
    notifyIcon.uID = Icon->IconId;

    if (PhNfPersistTrayIconPositionEnabled)
    {
        notifyIcon.uFlags |= NIF_GUID;
        notifyIcon.guidItem = Icon->IconGuid;
    }

    Shell_NotifyIcon(NIM_DELETE, &notifyIcon);

    return TRUE;
}

BOOLEAN PhNfpModifyNotifyIcon(
    _In_ PPH_NF_ICON Icon,
    _In_ ULONG Flags,
    _In_opt_ PPH_STRING Text,
    _In_opt_ HICON IconHandle
    )
{
    NOTIFYICONDATA notifyIcon;

    if (PhMainWndExiting)
        return FALSE;
    if (BooleanFlagOn(Icon->Flags, PH_NF_ICON_UNAVAILABLE))
        return FALSE;

    memset(&notifyIcon, 0, sizeof(NOTIFYICONDATA));
    notifyIcon.cbSize = sizeof(NOTIFYICONDATA);
    notifyIcon.uFlags = Flags;
    notifyIcon.hWnd = PhMainWndHandle;
    notifyIcon.uID = Icon->IconId;
    notifyIcon.hIcon = IconHandle;

    if (PhNfPersistTrayIconPositionEnabled)
    {
        notifyIcon.uFlags |= NIF_GUID;
        notifyIcon.guidItem = Icon->IconGuid;
    }

    if (!PhNfMiniInfoEnabled || PhNfMiniInfoPinned || BooleanFlagOn(Icon->Flags, PH_NF_ICON_NOSHOW_MINIINFO))
        notifyIcon.uFlags |= NIF_SHOWTIP;

    if (Flags & NIF_TIP)
    {
        PhSwapReference(&Icon->TextCache, Text);
        wcsncpy_s(
            notifyIcon.szTip, RTL_NUMBER_OF(notifyIcon.szTip),
            PhGetStringOrDefault(Text, PhApplicationName),
            _TRUNCATE
            );
    }

    if (!Shell_NotifyIcon(NIM_MODIFY, &notifyIcon))
    {
        // Explorer probably died and we lost our icon. Try to add the icon, and try again.
        PhNfpAddNotifyIcon(Icon);
        Shell_NotifyIcon(NIM_MODIFY, &notifyIcon);
    }

    return TRUE;
}

//BOOLEAN PhNfpGetNotifyIconRect(
//    _In_ ULONG Id,
//    _In_opt_ PPH_RECTANGLE IconRectangle
//    )
//{
//    NOTIFYICONIDENTIFIER notifyIconId = { sizeof(NOTIFYICONIDENTIFIER) };
//    PPH_NF_ICON icon;
//    RECT notifyRect;
//
//    if (PhMainWndExiting)
//        return FALSE;
//    if ((icon = PhNfGetIconById(Id)) && (icon->Flags & PH_NF_ICON_UNAVAILABLE))
//        return FALSE;
//
//    notifyIconId.hWnd = PhMainWndHandle;
//    notifyIconId.uID = Id;
//
//    if (SUCCEEDED(Shell_NotifyIconGetRect(&notifyIconId, &notifyRect)))
//    {
//        *IconRectangle = PhRectToRectangle(notifyRect);
//        return TRUE;
//    }
//
//    return FALSE;
//}

#ifdef PH_NF_ENABLE_WORKQUEUE
VOID PhNfTrayIconFlushWorkQueueData(
    VOID
    )
{
    PSLIST_ENTRY entry;
    PPH_NF_WORKQUEUE_DATA data;

    entry = RtlInterlockedFlushSList(&PhpTrayIconWorkQueueListHead);

    while (entry)
    {
        data = CONTAINING_RECORD(entry, PH_NF_WORKQUEUE_DATA, ListEntry);
        entry = entry->Next;

        if (PhMainWndExiting)
            break;

        if (data->Add)
        {
            RtlInterlockedSetBits(&data->Icon->Flags, PH_NF_ICON_ENABLED);

            PhNfpAddNotifyIcon(data->Icon);
        }

        if (data->Delete)
        {
            PhNfpRemoveNotifyIcon(data->Icon);

            RtlInterlockedClearBits(&data->Icon->Flags, PH_NF_ICON_ENABLED);
        }

        PhFree(data);
    }
}
#endif

NTSTATUS PhNfpTrayIconUpdateThread(
    _In_opt_ PVOID Context
    )
{
    PhSetThreadName(NtCurrentThread(), L"TrayIconUpdateThread");

    for (ULONG i = 0; i < PhTrayIconItemList->Count; i++)
    {
        PPH_NF_ICON icon = PhTrayIconItemList->Items[i];

        if (!BooleanFlagOn(icon->Flags, PH_NF_ICON_ENABLED))
            continue;

        PhNfpAddNotifyIcon(icon);
    }

    while (TRUE)
    {
        if (PhMainWndExiting)
            break;

        if (!(PhpTrayIconEventHandle && PhNfIconsEnabled()))
        {
            PhDelayExecution(1000);
            continue;
        }

        if (NT_SUCCESS(NtWaitForSingleObject(PhpTrayIconEventHandle, FALSE, NULL)))
        {
#ifdef PH_NF_ENABLE_WORKQUEUE
            PhNfTrayIconFlushWorkQueueData();
#endif
            for (ULONG i = 0; i < PhTrayIconItemList->Count; i++)
            {
                PPH_NF_ICON icon = PhTrayIconItemList->Items[i];

                if (!BooleanFlagOn(icon->Flags, PH_NF_ICON_ENABLED))
                    continue;

                if (PhMainWndExiting)
                    break;

                PhNfpUpdateRegisteredIcon(icon);
            }
        }
    }

#ifdef PH_NF_ENABLE_WORKQUEUE
    // Remove all icons to prevent them hanging around after we exit.

    for (ULONG i = 0; i < PhTrayIconItemList->Count; i++)
    {
        PPH_NF_ICON icon = PhTrayIconItemList->Items[i];

        if (!BooleanFlagOn(icon->Flags, PH_NF_ICON_ENABLED))
            continue;

        if (RtlInterlockedClearBits(&icon->Flags, PH_NF_ICON_ENABLED) == PH_NF_ICON_ENABLED)
        {
            PhNfpRemoveNotifyIcon(icon);
        }
    }
#endif

    return STATUS_SUCCESS;
}

VOID PhNfpProcessesUpdatedHandler(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    static ULONG processesUpdatedCount = 0;

    // Update the tray icons on a separate thread so we don't block the main GUI or
    // the provider threads when explorer is not responding.

    if (processesUpdatedCount != 3)
    {
        processesUpdatedCount++;
        return;
    }

    if (PhpTrayIconEventHandle && PhNfIconsEnabled())
    {
        NtSetEvent(PhpTrayIconEventHandle, NULL);
    }
}

VOID PhNfpUpdateRegisteredIcon(
    _In_ PPH_NF_ICON Icon
    )
{
    PVOID newIconOrBitmap;
    ULONG updateFlags;
    PPH_STRING newText;
    HICON newIcon;
    ULONG flags;

    if (!Icon->UpdateCallback)
        return;

    newIconOrBitmap = NULL;
    newText = NULL;
    newIcon = NULL;
    flags = 0;

    Icon->UpdateCallback(
        Icon,
        &newIconOrBitmap,
        &updateFlags,
        &newText,
        Icon->Context
        );

    if (newIconOrBitmap)
    {
        if (updateFlags & PH_NF_UPDATE_IS_BITMAP)
            newIcon = PhNfBitmapToIcon(newIconOrBitmap);
        else
            newIcon = newIconOrBitmap;

        flags |= NIF_ICON;
    }

    if (newText)
        flags |= NIF_TIP;

    if (flags != 0)
        PhNfpModifyNotifyIcon(Icon, flags, newText, newIcon);

    if (newIcon && (updateFlags & PH_NF_UPDATE_IS_BITMAP))
        DestroyIcon(newIcon);

    if (newIconOrBitmap && (updateFlags & PH_NF_UPDATE_DESTROY_RESOURCE))
    {
        if (updateFlags & PH_NF_UPDATE_IS_BITMAP)
            DeleteObject(newIconOrBitmap);
        else
            DestroyIcon(newIconOrBitmap);
    }

    if (newText)
        PhDereferenceObject(newText);
}

VOID PhNfpBeginBitmap(
    _Out_ PULONG Width,
    _Out_ PULONG Height,
    _Out_ HBITMAP *Bitmap,
    _Out_opt_ PVOID *Bits,
    _Out_ HDC *Hdc,
    _Out_ HBITMAP *OldBitmap
    )
{
    PhNfpBeginBitmap2(&PhNfpDefaultBitmapContext, Width, Height, Bitmap, Bits, Hdc, OldBitmap);
}

VOID PhNfpBeginBitmap2(
    _Inout_ PPH_NF_BITMAP Context,
    _Out_ PULONG Width,
    _Out_ PULONG Height,
    _Out_ HBITMAP *Bitmap,
    _Out_opt_ PVOID *Bits,
    _Out_ HDC *Hdc,
    _Out_ HBITMAP *OldBitmap
    )
{
    LONG dpiValue = PhGetTaskbarDpi();

    // Initialize and cache the current system metrics. (dmex)

    if (Context->TaskbarDpi == 0 || Context->TaskbarDpi != dpiValue)
    {
        Context->Width = PhGetSystemMetrics(SM_CXSMICON, dpiValue);
        Context->Height = PhGetSystemMetrics(SM_CXSMICON, dpiValue);

        // Re-initialize fonts with updated DPI (only when there's an existing handle). (dmex)

        if (PhNfpTrayIconFont)
        {
            PhNfGetTrayIconFont(dpiValue);
        }
    }

    // Cleanup existing resources when the DPI changes so we're able to re-initialize with updated DPI. (dmex)

    if (Context->TaskbarDpi != 0 && Context->TaskbarDpi != dpiValue)
    {
        if (PhNfpBlackIcon)
        {
            DestroyIcon(PhNfpBlackIcon);
            PhNfpBlackIcon = NULL;
        }

        if (Context->Hdc)
        {
            DeleteDC(Context->Hdc);
            Context->Hdc = NULL;
        }

        if (Context->Bitmap)
        {
            DeleteBitmap(Context->Bitmap);
            Context->Bitmap = NULL;
        }

        Context->Initialized = FALSE;
    }

    if (!Context->Initialized)
    {
        HDC screenHdc;
        BITMAPINFO bitmapInfo;

        memset(&bitmapInfo, 0, sizeof(BITMAPINFO));
        bitmapInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bitmapInfo.bmiHeader.biPlanes = 1;
        bitmapInfo.bmiHeader.biCompression = BI_RGB;
        bitmapInfo.bmiHeader.biWidth = Context->Width;
        bitmapInfo.bmiHeader.biHeight = Context->Height;
        bitmapInfo.bmiHeader.biBitCount = 32;

        screenHdc = GetDC(NULL);
        Context->Hdc = CreateCompatibleDC(screenHdc);
        Context->Bitmap = CreateDIBSection(screenHdc, &bitmapInfo, DIB_RGB_COLORS, &Context->Bits, NULL, 0);
        ReleaseDC(NULL, screenHdc);

        Context->TaskbarDpi = dpiValue;
        Context->Initialized = TRUE;
    }

    *Width = Context->Width;
    *Height = Context->Height;
    *Bitmap = Context->Bitmap;
    if (Bits) *Bits = Context->Bits;
    *Hdc = Context->Hdc;
    if (Context->Bitmap) *OldBitmap = SelectBitmap(Context->Hdc, Context->Bitmap);
}

VOID PhNfpCpuHistoryIconUpdateCallback(
    _In_ struct _PH_NF_ICON *Icon,
    _Out_ PVOID *NewIconOrBitmap,
    _Out_ PULONG Flags,
    _Out_ PPH_STRING *NewText,
    _In_opt_ PVOID Context
    )
{
    static PH_GRAPH_DRAW_INFO drawInfo =
    {
        16,
        16,
        PH_GRAPH_USE_LINE_2,
        2,
        RGB(0x00, 0x00, 0x00),
        16,
        NULL,
        NULL,
        0,
        0,
        0,
        0
    };
    ULONG maxDataCount;
    ULONG lineDataCount;
    PFLOAT lineData1;
    PFLOAT lineData2;
    HBITMAP bitmap;
    PVOID bits;
    HDC hdc;
    HBITMAP oldBitmap;
    HANDLE maxCpuProcessId;
    PPH_PROCESS_ITEM maxCpuProcessItem;
    PH_FORMAT format[8];

    // Icon

    Icon->Pointers->BeginBitmap(&drawInfo.Width, &drawInfo.Height, &bitmap, &bits, &hdc, &oldBitmap);
    maxDataCount = drawInfo.Width / 2 + 1;
    lineData1 = _malloca(maxDataCount * sizeof(FLOAT));
    lineData2 = _malloca(maxDataCount * sizeof(FLOAT));

    lineDataCount = min(maxDataCount, PhCpuKernelHistory.Count);
    PhCopyCircularBuffer_FLOAT(&PhCpuKernelHistory, lineData1, lineDataCount);
    PhCopyCircularBuffer_FLOAT(&PhCpuUserHistory, lineData2, lineDataCount);

    drawInfo.LineDataCount = lineDataCount;
    drawInfo.LineData1 = lineData1;
    drawInfo.LineData2 = lineData2;
    drawInfo.LineColor1 = PhCsColorCpuKernel;
    drawInfo.LineColor2 = PhCsColorCpuUser;
    drawInfo.LineBackColor1 = PhHalveColorBrightness(PhCsColorCpuKernel);
    drawInfo.LineBackColor2 = PhHalveColorBrightness(PhCsColorCpuUser);
    PhDrawGraphDirect(hdc, bits, &drawInfo);

    if (PhNfTransparencyEnabled)
    {
        PhBitmapSetAlpha(bits, drawInfo.Width, drawInfo.Height);
    }

    SelectBitmap(hdc, oldBitmap);
    *NewIconOrBitmap = bitmap;
    *Flags = PH_NF_UPDATE_IS_BITMAP;

    // Text

    if (PhMaxCpuHistory.Count != 0)
        maxCpuProcessId = UlongToHandle(PhGetItemCircularBuffer_ULONG(&PhMaxCpuHistory, 0));
    else
        maxCpuProcessId = NULL;

    if (maxCpuProcessId)
        maxCpuProcessItem = PhReferenceProcessItem(maxCpuProcessId);
    else
        maxCpuProcessItem = NULL;

    PhInitFormatS(&format[0], L"CPU history: ");
    PhInitFormatF(&format[1], ((DOUBLE)PhCpuKernelUsage + PhCpuUserUsage) * 100, PhMaxPrecisionUnit);
    PhInitFormatC(&format[2], '%');

    if (maxCpuProcessItem)
    {
        PhInitFormatC(&format[3], '\n');
        PhInitFormatSR(&format[4], maxCpuProcessItem->ProcessName->sr);
        PhInitFormatS(&format[5], L": ");
        PhInitFormatF(&format[6], (DOUBLE)maxCpuProcessItem->CpuUsage * 100, PhMaxPrecisionUnit);
        PhInitFormatC(&format[7], '%');
    }

    *NewText = PhFormat(format, maxCpuProcessItem ? 8 : 3, 128);
    if (maxCpuProcessItem) PhDereferenceObject(maxCpuProcessItem);

    _freea(lineData2);
    _freea(lineData1);
}

VOID PhNfpIoHistoryIconUpdateCallback(
    _In_ struct _PH_NF_ICON *Icon,
    _Out_ PVOID *NewIconOrBitmap,
    _Out_ PULONG Flags,
    _Out_ PPH_STRING *NewText,
    _In_opt_ PVOID Context
    )
{
    static PH_GRAPH_DRAW_INFO drawInfo =
    {
        16,
        16,
        PH_GRAPH_USE_LINE_2,
        2,
        RGB(0x00, 0x00, 0x00),
        16,
        NULL,
        NULL,
        0,
        0,
        0,
        0
    };
    ULONG maxDataCount;
    ULONG lineDataCount;
    PFLOAT lineData1;
    PFLOAT lineData2;
    FLOAT max;
    ULONG i;
    HBITMAP bitmap;
    PVOID bits;
    HDC hdc;
    HBITMAP oldBitmap;
    HANDLE maxIoProcessId;
    PPH_PROCESS_ITEM maxIoProcessItem;
    PH_FORMAT format[8];

    // Icon

    Icon->Pointers->BeginBitmap(&drawInfo.Width, &drawInfo.Height, &bitmap, &bits, &hdc, &oldBitmap);
    maxDataCount = drawInfo.Width / 2 + 1;
    lineData1 = _malloca(maxDataCount * sizeof(FLOAT));
    lineData2 = _malloca(maxDataCount * sizeof(FLOAT));

    lineDataCount = min(maxDataCount, PhIoReadHistory.Count);
    max = 1024 * 1024; // minimum scaling of 1 MB.

    for (i = 0; i < lineDataCount; i++)
    {
        lineData1[i] =
            (FLOAT)PhGetItemCircularBuffer_ULONG64(&PhIoReadHistory, i) +
            (FLOAT)PhGetItemCircularBuffer_ULONG64(&PhIoOtherHistory, i);
        lineData2[i] =
            (FLOAT)PhGetItemCircularBuffer_ULONG64(&PhIoWriteHistory, i);

        if (max < lineData1[i] + lineData2[i])
            max = lineData1[i] + lineData2[i];
    }

    PhDivideSinglesBySingle(lineData1, max, lineDataCount);
    PhDivideSinglesBySingle(lineData2, max, lineDataCount);

    drawInfo.LineDataCount = lineDataCount;
    drawInfo.LineData1 = lineData1;
    drawInfo.LineData2 = lineData2;
    drawInfo.LineColor1 = PhCsColorIoReadOther;
    drawInfo.LineColor2 = PhCsColorIoWrite;
    drawInfo.LineBackColor1 = PhHalveColorBrightness(PhCsColorIoReadOther);
    drawInfo.LineBackColor2 = PhHalveColorBrightness(PhCsColorIoWrite);
    PhDrawGraphDirect(hdc, bits, &drawInfo);

    if (PhNfTransparencyEnabled)
    {
        PhBitmapSetAlpha(bits, drawInfo.Width, drawInfo.Height);
    }

    SelectBitmap(hdc, oldBitmap);
    *NewIconOrBitmap = bitmap;
    *Flags = PH_NF_UPDATE_IS_BITMAP;

    // Text

    if (PhMaxIoHistory.Count != 0)
        maxIoProcessId = UlongToHandle(PhGetItemCircularBuffer_ULONG(&PhMaxIoHistory, 0));
    else
        maxIoProcessId = NULL;

    if (maxIoProcessId)
        maxIoProcessItem = PhReferenceProcessItem(maxIoProcessId);
    else
        maxIoProcessItem = NULL;

    PhInitFormatS(&format[0], L"I/O\nR: ");
    PhInitFormatSize(&format[1], PhIoReadDelta.Delta);
    PhInitFormatS(&format[2], L"\nW: ");
    PhInitFormatSize(&format[3], PhIoWriteDelta.Delta);
    PhInitFormatS(&format[4], L"\nO: ");
    PhInitFormatSize(&format[5], PhIoOtherDelta.Delta);

    if (maxIoProcessItem)
    {
        PhInitFormatC(&format[6], '\n');
        PhInitFormatSR(&format[7], maxIoProcessItem->ProcessName->sr);
    }

    *NewText = PhFormat(format, maxIoProcessItem ? 8 : 6, 128);
    if (maxIoProcessItem) PhDereferenceObject(maxIoProcessItem);

    _freea(lineData2);
    _freea(lineData1);
}

VOID PhNfpCommitHistoryIconUpdateCallback(
    _In_ struct _PH_NF_ICON *Icon,
    _Out_ PVOID *NewIconOrBitmap,
    _Out_ PULONG Flags,
    _Out_ PPH_STRING *NewText,
    _In_opt_ PVOID Context
    )
{
    static PH_GRAPH_DRAW_INFO drawInfo =
    {
        16,
        16,
        0,
        2,
        RGB(0x00, 0x00, 0x00),
        16,
        NULL,
        NULL,
        0,
        0,
        0,
        0
    };
    ULONG maxDataCount;
    ULONG lineDataCount;
    PFLOAT lineData1;
    ULONG i;
    HBITMAP bitmap;
    PVOID bits;
    HDC hdc;
    HBITMAP oldBitmap;
    DOUBLE commitFraction;
    PH_FORMAT format[5];

    // Icon

    Icon->Pointers->BeginBitmap(&drawInfo.Width, &drawInfo.Height, &bitmap, &bits, &hdc, &oldBitmap);
    maxDataCount = drawInfo.Width / 2 + 1;
    lineData1 = _malloca(maxDataCount * sizeof(FLOAT));

    lineDataCount = min(maxDataCount, PhCommitHistory.Count);

    for (i = 0; i < lineDataCount; i++)
        lineData1[i] = (FLOAT)PhGetItemCircularBuffer_ULONG(&PhCommitHistory, i);

    PhDivideSinglesBySingle(lineData1, (FLOAT)PhPerfInformation.CommitLimit, lineDataCount);

    drawInfo.LineDataCount = lineDataCount;
    drawInfo.LineData1 = lineData1;
    drawInfo.LineColor1 = PhCsColorPrivate;
    drawInfo.LineBackColor1 = PhHalveColorBrightness(PhCsColorPrivate);
    PhDrawGraphDirect(hdc, bits, &drawInfo);

    if (PhNfTransparencyEnabled)
    {
        PhBitmapSetAlpha(bits, drawInfo.Width, drawInfo.Height);
    }

    SelectBitmap(hdc, oldBitmap);
    *NewIconOrBitmap = bitmap;
    *Flags = PH_NF_UPDATE_IS_BITMAP;

    // Text

    commitFraction = (DOUBLE)PhPerfInformation.CommittedPages / PhPerfInformation.CommitLimit;

    PhInitFormatS(&format[0], L"Commit: ");
    PhInitFormatSize(&format[1], UInt32x32To64(PhPerfInformation.CommittedPages, PAGE_SIZE));
    PhInitFormatS(&format[2], L" (");
    PhInitFormatF(&format[3], commitFraction * 100, PhMaxPrecisionUnit);
    PhInitFormatS(&format[4], L"%)");

    *NewText = PhFormat(format, 5, 96);

    _freea(lineData1);
}

VOID PhNfpPhysicalHistoryIconUpdateCallback(
    _In_ struct _PH_NF_ICON *Icon,
    _Out_ PVOID *NewIconOrBitmap,
    _Out_ PULONG Flags,
    _Out_ PPH_STRING *NewText,
    _In_opt_ PVOID Context
    )
{
    static PH_GRAPH_DRAW_INFO drawInfo =
    {
        16,
        16,
        0,
        2,
        RGB(0x00, 0x00, 0x00),
        16,
        NULL,
        NULL,
        0,
        0,
        0,
        0
    };
    ULONG maxDataCount;
    ULONG lineDataCount;
    PFLOAT lineData1;
    ULONG i;
    HBITMAP bitmap;
    PVOID bits;
    HDC hdc;
    HBITMAP oldBitmap;
    ULONG physicalUsage;
    DOUBLE physicalFraction;
    PH_FORMAT format[5];

    // Icon

    Icon->Pointers->BeginBitmap(&drawInfo.Width, &drawInfo.Height, &bitmap, &bits, &hdc, &oldBitmap);
    maxDataCount = drawInfo.Width / 2 + 1;
    lineData1 = _malloca(maxDataCount * sizeof(FLOAT));

    lineDataCount = min(maxDataCount, PhPhysicalHistory.Count);

    for (i = 0; i < lineDataCount; i++)
        lineData1[i] = (FLOAT)PhGetItemCircularBuffer_ULONG(&PhPhysicalHistory, i);

    PhDivideSinglesBySingle(lineData1, (FLOAT)PhSystemBasicInformation.NumberOfPhysicalPages, lineDataCount);

    drawInfo.LineDataCount = lineDataCount;
    drawInfo.LineData1 = lineData1;
    drawInfo.LineColor1 = PhCsColorPhysical;
    drawInfo.LineBackColor1 = PhHalveColorBrightness(PhCsColorPhysical);
    PhDrawGraphDirect(hdc, bits, &drawInfo);

    if (PhNfTransparencyEnabled)
    {
        PhBitmapSetAlpha(bits, drawInfo.Width, drawInfo.Height);
    }

    SelectBitmap(hdc, oldBitmap);
    *NewIconOrBitmap = bitmap;
    *Flags = PH_NF_UPDATE_IS_BITMAP;

    // Text

    physicalUsage = PhSystemBasicInformation.NumberOfPhysicalPages - PhPerfInformation.AvailablePages;
    physicalFraction = (DOUBLE)physicalUsage / PhSystemBasicInformation.NumberOfPhysicalPages;

    PhInitFormatS(&format[0], L"Physical memory: ");
    PhInitFormatSize(&format[1], UInt32x32To64(physicalUsage, PAGE_SIZE));
    PhInitFormatS(&format[2], L" (");
    PhInitFormatF(&format[3], physicalFraction * 100, PhMaxPrecisionUnit);
    PhInitFormatS(&format[4], L"%)");

    *NewText = PhFormat(format, 5, 96);

    _freea(lineData1);
}

VOID PhNfpCpuUsageIconUpdateCallback(
    _In_ struct _PH_NF_ICON *Icon,
    _Out_ PVOID *NewIconOrBitmap,
    _Out_ PULONG Flags,
    _Out_ PPH_STRING *NewText,
    _In_opt_ PVOID Context
    )
{
    ULONG width;
    ULONG height;
    HBITMAP bitmap;
    PVOID bits;
    HDC hdc;
    HBITMAP oldBitmap;
    PH_FORMAT format[5];
    HANDLE maxCpuProcessId;
    PPH_PROCESS_ITEM maxCpuProcessItem;
    PPH_STRING maxCpuText = NULL;

    // Icon

    Icon->Pointers->BeginBitmap(&width, &height, &bitmap, &bits, &hdc, &oldBitmap);

    // This stuff is copied from CpuUsageIcon.cs (PH 1.x).
    {
        COLORREF kColor = PhCsColorCpuKernel;
        COLORREF uColor = PhCsColorCpuUser;
        COLORREF kbColor = PhHalveColorBrightness(PhCsColorCpuKernel);
        COLORREF ubColor = PhHalveColorBrightness(PhCsColorCpuUser);
        FLOAT k = PhCpuKernelUsage;
        FLOAT u = PhCpuUserUsage;
        LONG kl = (LONG)(k * height);
        LONG ul = (LONG)(u * height);
        RECT rect;
        HBRUSH dcBrush;
        HPEN dcPen;
        POINT points[2];

        dcBrush = GetStockBrush(DC_BRUSH);
        dcPen = GetStockPen(DC_PEN);
        rect.left = 0;
        rect.top = 0;
        rect.right = width;
        rect.bottom = height;
        SetDCBrushColor(hdc, RGB(0x00, 0x00, 0x00));
        FillRect(hdc, &rect, dcBrush);

        // Draw the base line.
        if (kl + ul == 0)
        {
            SelectPen(hdc, dcPen);
            SetDCPenColor(hdc, uColor);
            points[0].x = 0;
            points[0].y = height - 1;
            points[1].x = width;
            points[1].y = height - 1;
            Polyline(hdc, points, 2);
        }
        else
        {
            rect.left = 0;
            rect.top = height - ul - kl;
            rect.right = width;
            rect.bottom = height - kl;
            SetDCBrushColor(hdc, ubColor);
            FillRect(hdc, &rect, dcBrush);

            points[0].x = 0;
            points[0].y = height - 1 - ul - kl;
            if (points[0].y < 0) points[0].y = 0;
            points[1].x = width;
            points[1].y = points[0].y;
            SelectPen(hdc, dcPen);
            SetDCPenColor(hdc, uColor);
            Polyline(hdc, points, 2);

            if (kl != 0)
            {
                rect.left = 0;
                rect.top = height - kl;
                rect.right = width;
                rect.bottom = height;
                SetDCBrushColor(hdc, kbColor);
                FillRect(hdc, &rect, dcBrush);

                points[0].x = 0;
                points[0].y = height - 1 - kl;
                if (points[0].y < 0) points[0].y = 0;
                points[1].x = width;
                points[1].y = points[0].y;
                SelectPen(hdc, dcPen);
                SetDCPenColor(hdc, kColor);
                Polyline(hdc, points, 2);
            }
        }
    }

    if (PhNfTransparencyEnabled)
    {
        PhBitmapSetAlpha(bits, width, height);
    }

    SelectBitmap(hdc, oldBitmap);
    *NewIconOrBitmap = bitmap;
    *Flags = PH_NF_UPDATE_IS_BITMAP;

    // Text

    if (PhMaxCpuHistory.Count != 0)
        maxCpuProcessId = UlongToHandle(PhGetItemCircularBuffer_ULONG(&PhMaxCpuHistory, 0));
    else
        maxCpuProcessId = NULL;

    if (maxCpuProcessId)
    {
        if (maxCpuProcessItem = PhReferenceProcessItem(maxCpuProcessId))
        {
            // \n%s: %.2f%%
            PhInitFormatC(&format[0], L'\n');
            PhInitFormatSR(&format[1], maxCpuProcessItem->ProcessName->sr);
            PhInitFormatS(&format[2], L": ");
            PhInitFormatF(&format[3], (DOUBLE)maxCpuProcessItem->CpuUsage * 100, PhMaxPrecisionUnit);
            PhInitFormatC(&format[4], L'%');

            maxCpuText = PhFormat(format, 5, 128);
            //maxCpuText = PhFormatString(
            //    L"\n%s: %.2f%%",
            //    maxCpuProcessItem->ProcessName->Buffer,
            //    maxCpuProcessItem->CpuUsage * 100
            //    );
            PhDereferenceObject(maxCpuProcessItem);
        }
    }

    PhInitFormatS(&format[0], L"CPU usage: ");
    PhInitFormatF(&format[1], (DOUBLE)(PhCpuKernelUsage + PhCpuUserUsage) * 100, PhMaxPrecisionUnit);
    PhInitFormatC(&format[2], L'%');
    if (maxCpuText) PhInitFormatSR(&format[3], maxCpuText->sr);
    else PhInitFormatC(&format[3], L' ');

    *NewText = PhFormat(format, 4, 128);
    //*NewText = PhFormatString(L"CPU usage: %.2f%%%s", (PhCpuKernelUsage + PhCpuUserUsage) * 100, PhGetStringOrEmpty(maxCpuText));
    if (maxCpuText) PhDereferenceObject(maxCpuText);
}

// Text icons

VOID PhNfpCpuUsageTextIconUpdateCallback(
    _In_ struct _PH_NF_ICON *Icon,
    _Out_ PVOID *NewIconOrBitmap,
    _Out_ PULONG Flags,
    _Out_ PPH_STRING *NewText,
    _In_opt_ PVOID Context
    )
{
    static PH_GRAPH_DRAW_INFO drawInfo =
    {
        16,
        16,
        0,
        2,
        RGB(0x00, 0x00, 0x00),
        16,
        NULL,
        NULL,
        0,
        0,
        0,
        0
    };
    HBITMAP bitmap;
    PVOID bits;
    HDC hdc;
    HBITMAP oldBitmap;
    PH_FORMAT format[5];
    HANDLE maxCpuProcessId;
    PPH_PROCESS_ITEM maxCpuProcessItem;
    PPH_STRING maxCpuText = NULL;
    PPH_STRING text;

    // Icon

    Icon->Pointers->BeginBitmap(&drawInfo.Width, &drawInfo.Height, &bitmap, &bits, &hdc, &oldBitmap);

    PhInitFormatF(&format[0], (DOUBLE)(PhCpuKernelUsage + PhCpuUserUsage) * 100, 0);
    text = PhFormat(format, 1, 10);

    drawInfo.TextFont = PhNfGetTrayIconFont(0);
    drawInfo.TextColor = PhCsColorCpuKernel;
    PhDrawTrayIconText(hdc, bits, &drawInfo, &text->sr);
    PhDereferenceObject(text);

    if (PhNfTransparencyEnabled)
    {
        PhBitmapSetAlpha(bits, drawInfo.Width, drawInfo.Height);
    }

    SelectBitmap(hdc, oldBitmap);
    *NewIconOrBitmap = bitmap;
    *Flags = PH_NF_UPDATE_IS_BITMAP;

    // Text

    if (PhMaxCpuHistory.Count != 0)
        maxCpuProcessId = UlongToHandle(PhGetItemCircularBuffer_ULONG(&PhMaxCpuHistory, 0));
    else
        maxCpuProcessId = NULL;

    if (maxCpuProcessId)
    {
        if (maxCpuProcessItem = PhReferenceProcessItem(maxCpuProcessId))
        {
            PhInitFormatC(&format[0], L'\n');
            PhInitFormatSR(&format[1], maxCpuProcessItem->ProcessName->sr);
            PhInitFormatS(&format[2], L": ");
            PhInitFormatF(&format[3], (DOUBLE)maxCpuProcessItem->CpuUsage * 100, PhMaxPrecisionUnit);
            PhInitFormatC(&format[4], L'%');

            maxCpuText = PhFormat(format, 5, 128);
            //maxCpuText = PhFormatString(
            //    L"\n%s: %.2f%%",
            //    maxCpuProcessItem->ProcessName->Buffer,
            //    maxCpuProcessItem->CpuUsage * 100
            //    );
            PhDereferenceObject(maxCpuProcessItem);
        }
    }

    PhInitFormatS(&format[0], L"CPU usage: ");
    PhInitFormatF(&format[1], (DOUBLE)(PhCpuKernelUsage + PhCpuUserUsage) * 100, PhMaxPrecisionUnit);
    PhInitFormatC(&format[2], L'%');
    if (maxCpuText) PhInitFormatSR(&format[3], maxCpuText->sr);
    else PhInitFormatC(&format[3], L' ');

    *NewText = PhFormat(format, 4, 128);
    //*NewText = PhFormatString(L"CPU usage: %.2f%%%s", (PhCpuKernelUsage + PhCpuUserUsage) * 100, PhGetStringOrEmpty(maxCpuText));
    if (maxCpuText) PhDereferenceObject(maxCpuText);
}

VOID PhNfpIoUsageTextIconUpdateCallback(
    _In_ struct _PH_NF_ICON *Icon,
    _Out_ PVOID *NewIconOrBitmap,
    _Out_ PULONG Flags,
    _Out_ PPH_STRING *NewText,
    _In_opt_ PVOID Context
    )
{
    static PH_GRAPH_DRAW_INFO drawInfo =
    {
        16,
        16,
        0,
        0,
        RGB(0x00, 0x00, 0x00),
        0,
        NULL,
        NULL,
        0,
        0,
        0,
        0
    };
    HBITMAP bitmap;
    PVOID bits;
    HDC hdc;
    HBITMAP oldBitmap;
    HANDLE maxIoProcessId;
    PPH_PROCESS_ITEM maxIoProcessItem;
    PH_FORMAT format[8];
    PPH_STRING text;
    static ULONG64 maxValue = 100000 * 1024; // minimum scaling of 100 MB.

    // TODO: Reset maxValue every X amount of time.

    // Icon

    Icon->Pointers->BeginBitmap(&drawInfo.Width, &drawInfo.Height, &bitmap, &bits, &hdc, &oldBitmap);

    if (maxValue < (PhIoReadDelta.Delta + PhIoWriteDelta.Delta + PhIoOtherDelta.Delta))
        maxValue = (PhIoReadDelta.Delta + PhIoWriteDelta.Delta + PhIoOtherDelta.Delta);

    PhInitFormatF(&format[0], (DOUBLE)(PhIoReadDelta.Delta + PhIoWriteDelta.Delta + PhIoOtherDelta.Delta) / maxValue * 100, 0);
    text = PhFormat(format, 1, 10);

    drawInfo.TextFont = PhNfGetTrayIconFont(0);
    drawInfo.TextColor = PhCsColorIoReadOther;
    PhDrawTrayIconText(hdc, bits, &drawInfo, &text->sr);
    PhDereferenceObject(text);

    if (PhNfTransparencyEnabled)
    {
        PhBitmapSetAlpha(bits, drawInfo.Width, drawInfo.Height);
    }

    SelectBitmap(hdc, oldBitmap);
    *NewIconOrBitmap = bitmap;
    *Flags = PH_NF_UPDATE_IS_BITMAP;

    // Text

    if (PhMaxIoHistory.Count != 0)
        maxIoProcessId = UlongToHandle(PhGetItemCircularBuffer_ULONG(&PhMaxIoHistory, 0));
    else
        maxIoProcessId = NULL;

    if (maxIoProcessId)
        maxIoProcessItem = PhReferenceProcessItem(maxIoProcessId);
    else
        maxIoProcessItem = NULL;

    PhInitFormatS(&format[0], L"I/O\nR: ");
    PhInitFormatSize(&format[1], PhIoReadDelta.Delta);
    PhInitFormatS(&format[2], L"\nW: ");
    PhInitFormatSize(&format[3], PhIoWriteDelta.Delta);
    PhInitFormatS(&format[4], L"\nO: ");
    PhInitFormatSize(&format[5], PhIoOtherDelta.Delta);

    if (maxIoProcessItem)
    {
        PhInitFormatC(&format[6], '\n');
        PhInitFormatSR(&format[7], maxIoProcessItem->ProcessName->sr);
    }

    *NewText = PhFormat(format, maxIoProcessItem ? 8 : 6, 128);
    if (maxIoProcessItem) PhDereferenceObject(maxIoProcessItem);
}

VOID PhNfpCommitTextIconUpdateCallback(
    _In_ struct _PH_NF_ICON *Icon,
    _Out_ PVOID *NewIconOrBitmap,
    _Out_ PULONG Flags,
    _Out_ PPH_STRING *NewText,
    _In_opt_ PVOID Context
    )
{
    static PH_GRAPH_DRAW_INFO drawInfo =
    {
        16,
        16,
        0,
        0,
        RGB(0x00, 0x00, 0x00),
        0,
        NULL,
        NULL,
        0,
        0,
        0,
        0
    };
    HBITMAP bitmap;
    PVOID bits;
    HDC hdc;
    HBITMAP oldBitmap;
    DOUBLE commitFraction;
    PH_FORMAT format[5];
    PPH_STRING text;

    // Icon

    Icon->Pointers->BeginBitmap(&drawInfo.Width, &drawInfo.Height, &bitmap, &bits, &hdc, &oldBitmap);

    PhInitFormatF(&format[0], (DOUBLE)PhPerfInformation.CommittedPages / PhPerfInformation.CommitLimit * 100, 0);
    text = PhFormat(format, 1, 10);

    drawInfo.TextFont = PhNfGetTrayIconFont(0);
    drawInfo.TextColor = PhCsColorPrivate;
    PhDrawTrayIconText(hdc, bits, &drawInfo, &text->sr);
    PhDereferenceObject(text);

    if (PhNfTransparencyEnabled)
    {
        PhBitmapSetAlpha(bits, drawInfo.Width, drawInfo.Height);
    }

    SelectBitmap(hdc, oldBitmap);
    *NewIconOrBitmap = bitmap;
    *Flags = PH_NF_UPDATE_IS_BITMAP;

    // Text

    commitFraction = (DOUBLE)PhPerfInformation.CommittedPages / PhPerfInformation.CommitLimit;

    PhInitFormatS(&format[0], L"Commit: ");
    PhInitFormatSize(&format[1], UInt32x32To64(PhPerfInformation.CommittedPages, PAGE_SIZE));
    PhInitFormatS(&format[2], L" (");
    PhInitFormatF(&format[3], commitFraction * 100, PhMaxPrecisionUnit);
    PhInitFormatS(&format[4], L"%)");

    *NewText = PhFormat(format, 5, 96);
}

VOID PhNfpPhysicalUsageTextIconUpdateCallback(
    _In_ struct _PH_NF_ICON *Icon,
    _Out_ PVOID *NewIconOrBitmap,
    _Out_ PULONG Flags,
    _Out_ PPH_STRING *NewText,
    _In_opt_ PVOID Context
    )
{
    static PH_GRAPH_DRAW_INFO drawInfo =
    {
        16,
        16,
        0,
        0,
        RGB(0x00, 0x00, 0x00),
        0,
        NULL,
        NULL,
        0,
        0,
        0,
        0
    };
    HBITMAP bitmap;
    PVOID bits;
    HDC hdc;
    HBITMAP oldBitmap;
    ULONG physicalUsage;
    DOUBLE physicalFraction;
    PH_FORMAT format[5];
    PPH_STRING text;

    // Icon

    Icon->Pointers->BeginBitmap(&drawInfo.Width, &drawInfo.Height, &bitmap, &bits, &hdc, &oldBitmap);

    physicalUsage = PhSystemBasicInformation.NumberOfPhysicalPages - PhPerfInformation.AvailablePages;
    physicalFraction = (DOUBLE)physicalUsage / PhSystemBasicInformation.NumberOfPhysicalPages;

    PhInitFormatF(&format[0], physicalFraction * 100, 0);
    text = PhFormat(format, 1, 10);

    drawInfo.TextFont = PhNfGetTrayIconFont(0);
    drawInfo.TextColor = PhCsColorPhysical;
    PhDrawTrayIconText(hdc, bits, &drawInfo, &text->sr);
    PhDereferenceObject(text);

    if (PhNfTransparencyEnabled)
    {
        PhBitmapSetAlpha(bits, drawInfo.Width, drawInfo.Height);
    }

    SelectBitmap(hdc, oldBitmap);
    *NewIconOrBitmap = bitmap;
    *Flags = PH_NF_UPDATE_IS_BITMAP;

    // Text

    physicalUsage = PhSystemBasicInformation.NumberOfPhysicalPages - PhPerfInformation.AvailablePages;
    physicalFraction = (DOUBLE)physicalUsage / PhSystemBasicInformation.NumberOfPhysicalPages;

    PhInitFormatS(&format[0], L"Physical memory: ");
    PhInitFormatSize(&format[1], UInt32x32To64(physicalUsage, PAGE_SIZE));
    PhInitFormatS(&format[2], L" (");
    PhInitFormatF(&format[3], physicalFraction * 100, PhMaxPrecisionUnit);
    PhInitFormatS(&format[4], L"%)");

    *NewText = PhFormat(format, 5, 96);
}

VOID PhNfpPlainIconUpdateCallback(
    _In_ struct _PH_NF_ICON *Icon,
    _Out_ PVOID *NewIconOrBitmap,
    _Out_ PULONG Flags,
    _Out_ PPH_STRING *NewText,
    _In_opt_ PVOID Context
    )
{
    *NewIconOrBitmap = PhGetApplicationIcon(TRUE);
    *NewText = PhCreateString(L"System Informer");
    *Flags = 0;
}

_Success_(return)
BOOLEAN PhNfpGetShowMiniInfoSectionData(
    _In_ ULONG IconIndex,
    _In_ PPH_NF_ICON RegisteredIcon,
    _Out_ PPH_NF_MSG_SHOWMINIINFOSECTION_DATA Data
    )
{
    BOOLEAN showMiniInfo = FALSE;

    if (RegisteredIcon && RegisteredIcon->MessageCallback)
    {
        Data->SectionName = NULL;

        if (!BooleanFlagOn(RegisteredIcon->Flags, PH_NF_ICON_NOSHOW_MINIINFO))
        {
            if (RegisteredIcon->MessageCallback)
            {
                RegisteredIcon->MessageCallback(
                    RegisteredIcon,
                    (ULONG_PTR)Data,
                    MAKELPARAM(PH_NF_MSG_SHOWMINIINFOSECTION, 0),
                    RegisteredIcon->Context
                    );
            }

            showMiniInfo = TRUE;
        }
    }
    else
    {
        showMiniInfo = TRUE;

        switch (IconIndex)
        {
        case PH_TRAY_ICON_ID_CPU_HISTORY:
        case PH_TRAY_ICON_ID_CPU_USAGE:
        case PH_TRAY_ICON_ID_CPU_TEXT:
        case PH_TRAY_ICON_ID_PLAIN_ICON:
            Data->SectionName = L"CPU";
            break;
        case PH_TRAY_ICON_ID_IO_HISTORY:
        case PH_TRAY_ICON_ID_IO_TEXT:
            Data->SectionName = L"I/O";
            break;
        case PH_TRAY_ICON_ID_COMMIT_HISTORY:
        case PH_TRAY_ICON_ID_COMMIT_TEXT:
            Data->SectionName = L"Commit charge";
            break;
        case PH_TRAY_ICON_ID_PHYSICAL_HISTORY:
        case PH_TRAY_ICON_ID_PHYSICAL_TEXT:
            Data->SectionName = L"Physical memory";
            break;
        default:
            showMiniInfo = FALSE;
            break;
        }
    }

    return showMiniInfo;
}

VOID PhNfpIconClickActivateTimerProc(
    _In_ HWND hwnd,
    _In_ UINT uMsg,
    _In_ UINT_PTR idEvent,
    _In_ ULONG dwTime
    )
{
    PhPinMiniInformation(MiniInfoActivePinType, 1, 0,
        PH_MINIINFO_ACTIVATE_WINDOW | PH_MINIINFO_DONT_CHANGE_SECTION_IF_PINNED,
        IconClickShowMiniInfoSectionData.SectionName, &IconClickLocation);
    KillTimer(PhMainWndHandle, TIMER_ICON_CLICK_ACTIVATE);
}

VOID PhNfpDisableHover(
    VOID
    )
{
    IconDisableHover = TRUE;
    SetTimer(PhMainWndHandle, TIMER_ICON_RESTORE_HOVER, NFP_ICON_RESTORE_HOVER_DELAY, PhNfpIconRestoreHoverTimerProc);
}

VOID PhNfpIconRestoreHoverTimerProc(
    _In_ HWND hwnd,
    _In_ UINT uMsg,
    _In_ UINT_PTR idEvent,
    _In_ ULONG dwTime
    )
{
    IconDisableHover = FALSE;
    KillTimer(PhMainWndHandle, TIMER_ICON_RESTORE_HOVER);
}
