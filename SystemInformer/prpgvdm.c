/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     dmex    2021-2022
 *
 */

#ifdef _M_IX86
#include <phapp.h>
#include <phplug.h>
#include <phsettings.h>
#include <procprp.h>
#include <procprv.h>
#include <settings.h>
#include <emenu.h>

#include <vdmdbg.h>

#define WM_PH_VDMDBG_UPDATE (WM_APP + 251)

typedef struct _PH_NTVDM_CONTEXT
{
    PH_CALLBACK_REGISTRATION ProcessesUpdatedRegistration;
    HWND WindowHandle;
    HWND ListViewHandle;
    BOOLEAN Enabled;
    PPH_LIST VdmHostProcessList;
} PH_NTVDM_CONTEXT, *PPH_NTVDM_CONTEXT;

typedef struct _PH_NTVDM_ENTRY
{
    ULONG ThreadId;
    USHORT Mod16;
    USHORT Task16;
    PPH_STRING ModuleName;
    PPH_STRING FileName;
} PH_NTVDM_ENTRY, *PPH_NTVDM_ENTRY;

static INT (WINAPI* VDMEnumTaskWOWEx_I)(
    _In_ ULONG ProcessId,
    _In_ TASKENUMPROCEX fp,
    _In_ LPARAM lparam
    ) = NULL;
static BOOL (WINAPI* VDMTerminateTaskWOW_I)(
    _In_ ULONG ProcessId,
    _In_ USHORT TaskId
    ) = NULL;

PVOID PhpGetVdmDbgDllBase(
    VOID
    )
{
    static PH_INITONCE initOnce = PH_INITONCE_INIT;
    static PVOID imageBaseAddress = NULL;

    if (PhBeginInitOnce(&initOnce))
    {
        PPH_STRING systemDirectory;
        PPH_STRING systemFileName;

        if (systemDirectory = PhGetSystemDirectory())
        {
            if (systemFileName = PhConcatStringRefZ(&systemDirectory->sr, L"\\vdmdbg.dll"))
            {
                if (!(imageBaseAddress = PhGetLoaderEntryStringRefDllBase(&systemFileName->sr, NULL)))
                    imageBaseAddress = PhLoadLibrary(PhGetString(systemFileName));

                PhDereferenceObject(systemFileName);
            }

            PhDereferenceObject(systemDirectory);
        }

        PhEndInitOnce(&initOnce);
    }

    return imageBaseAddress;
}

BOOL WINAPI PhpVDMEnumTaskWOWCallback(
    _In_ ULONG ThreadId,
    _In_ USHORT Mod16,
    _In_ USHORT Task16,
    _In_ PSTR ModName,
    _In_ PSTR FileName,
    _In_ LPARAM Context
    )
{
    PPH_LIST vdmTaskList = (PPH_LIST)Context;
    PPH_NTVDM_ENTRY entry;

    entry = PhAllocateZero(sizeof(PH_NTVDM_ENTRY));
    entry->ThreadId = ThreadId;
    entry->Mod16 = Mod16;
    entry->Task16 = Task16;
    entry->ModuleName = PhZeroExtendToUtf16(ModName);
    entry->FileName = PhZeroExtendToUtf16(FileName);

    PhAddItemList(vdmTaskList, entry);

    return FALSE;
}

PPH_LIST PhpQueryVdmHostProcess(
    _In_ PPH_PROCESS_ITEM ProcessItem
    )
{
    PPH_LIST vdmTaskList;

    if (!VDMEnumTaskWOWEx_I)
    {
        PVOID imageBaseAddress;

        if (imageBaseAddress = PhpGetVdmDbgDllBase())
        {
            VDMEnumTaskWOWEx_I = PhGetDllBaseProcedureAddress(imageBaseAddress, "VDMEnumTaskWOWEx", 0);
        }
    }

    if (!VDMEnumTaskWOWEx_I)
        return NULL;

    vdmTaskList = PhCreateList(1);

    VDMEnumTaskWOWEx_I(
        HandleToUlong(ProcessItem->ProcessId),
        PhpVDMEnumTaskWOWCallback,
        (LPARAM)vdmTaskList
        );

    return vdmTaskList;
}

// This method is a rough equivalent to TerminateProcess() in Win32.
// It should be avoided, if possible. It does not give the task a chance to cleanly exit, so data may be lost.
// Unlike Win32, the WowExec is not guaranteed to clean up after a terminated task.
// This can leave the VDM corrupt and unusable.
// To terminate the task cleanly, send a WM_CLOSE to its top-level window.
BOOLEAN PhpTerminateVdmTask(
    _In_ PPH_PROCESS_ITEM ProcessItem,
    _In_ USHORT TaskId
    )
{
    if (!VDMTerminateTaskWOW_I)
    {
        PVOID imageBaseAddress;

        if (imageBaseAddress = PhpGetVdmDbgDllBase())
        {
            VDMTerminateTaskWOW_I = PhGetDllBaseProcedureAddress(imageBaseAddress, "VDMTerminateTaskWOW", 0);
        }
    }

    if (!VDMTerminateTaskWOW_I)
        return FALSE;

    return !!VDMTerminateTaskWOW_I(HandleToUlong(ProcessItem->ProcessId), TaskId);
}

static VOID NTAPI PhpVdmHostProcessUpdateHandler(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    PPH_NTVDM_CONTEXT context = (PPH_NTVDM_CONTEXT)Context;

    if (context && context->Enabled)
    {
        PostMessage(context->WindowHandle, WM_PH_VDMDBG_UPDATE, 0, 0);
    }
}

VOID PhpClearVdmHostProcessItems(
    _Inout_ PPH_NTVDM_CONTEXT Context
    )
{
    ULONG i;
    PPH_NTVDM_ENTRY entry;

    for (i = 0; i < Context->VdmHostProcessList->Count; i++)
    {
        entry = Context->VdmHostProcessList->Items[i];

        if (entry->FileName)
            PhDereferenceObject(entry->FileName);
        if (entry->ModuleName)
            PhDereferenceObject(entry->ModuleName);

        PhFree(entry);
    }

    PhClearList(Context->VdmHostProcessList);
}

VOID PhpRefreshVdmHostProcess(
    _In_ HWND hwndDlg,
    _Inout_ PPH_NTVDM_CONTEXT Context,
    _In_ PPH_PROCESS_ITEM ProcessItem
    )
{
    PVOID selectedIndex;
    PPH_LIST providerList;

    ExtendedListView_SetRedraw(Context->ListViewHandle, FALSE);

    selectedIndex = PhGetSelectedListViewItemParam(Context->ListViewHandle);
    ListView_DeleteAllItems(Context->ListViewHandle);
    PhpClearVdmHostProcessItems(Context);

    if (providerList = PhpQueryVdmHostProcess(ProcessItem))
    {
        for (ULONG i = 0; i < providerList->Count; i++)
        {
            PPH_NTVDM_ENTRY entry = providerList->Items[i];
            INT lvItemIndex;

            lvItemIndex = PhAddListViewItem(
                Context->ListViewHandle,
                MAXINT,
                PhGetStringOrEmpty(entry->ModuleName),
                UlongToPtr(Context->VdmHostProcessList->Count + 1)
                );
            PhSetListViewSubItem(Context->ListViewHandle, lvItemIndex, 1, PhGetStringOrEmpty(entry->FileName));
            PhSetListViewSubItem(Context->ListViewHandle, lvItemIndex, 2, PhaFormatUInt64(entry->ThreadId, FALSE)->Buffer);
            PhSetListViewSubItem(Context->ListViewHandle, lvItemIndex, 3, PhaFormatUInt64(entry->Task16, FALSE)->Buffer);

            PhAddItemList(Context->VdmHostProcessList, entry);
        }

        PhDereferenceObject(providerList);
    }

    if (selectedIndex)
    {
        ListView_SetItemState(Context->ListViewHandle, PtrToUlong(selectedIndex) - 1,
            LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);
        ListView_EnsureVisible(Context->ListViewHandle, PtrToUlong(selectedIndex) - 1, FALSE);
    }

    ExtendedListView_SetRedraw(Context->ListViewHandle, TRUE);
}

INT_PTR CALLBACK PhpProcessVdmHostProcessDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    LPPROPSHEETPAGE propSheetPage;
    PPH_PROCESS_PROPPAGECONTEXT propPageContext;
    PPH_PROCESS_ITEM processItem;
    PPH_NTVDM_CONTEXT context;

    if (PhPropPageDlgProcHeader(hwndDlg, uMsg, lParam, &propSheetPage, &propPageContext, &processItem))
    {
        context = (PPH_NTVDM_CONTEXT)propPageContext->Context;
    }
    else
    {
        return FALSE;
    }

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            context = propPageContext->Context = PhAllocateZero(sizeof(PH_NTVDM_CONTEXT));
            context->WindowHandle = hwndDlg;
            context->ListViewHandle = GetDlgItem(hwndDlg, IDC_LIST);
            context->Enabled = TRUE;
            context->VdmHostProcessList = PhCreateList(1);

            PhSetListViewStyle(context->ListViewHandle, FALSE, TRUE);
            PhSetControlTheme(context->ListViewHandle, L"explorer");
            PhAddListViewColumn(context->ListViewHandle, 0, 0, 0, LVCFMT_LEFT, 140, L"Module name");
            PhAddListViewColumn(context->ListViewHandle, 1, 1, 1, LVCFMT_LEFT, 180, L"File name");
            PhAddListViewColumn(context->ListViewHandle, 2, 2, 2, LVCFMT_LEFT, 80, L"Thread Id");
            PhAddListViewColumn(context->ListViewHandle, 3, 3, 3, LVCFMT_LEFT, 80, L"Task Id");
            PhSetExtendedListView(context->ListViewHandle);
            PhLoadListViewColumnsFromSetting(L"VdmHostListViewColumns", context->ListViewHandle);

            PhpRefreshVdmHostProcess(hwndDlg, context, processItem);

            PhRegisterCallback(
                PhGetGeneralCallback(GeneralCallbackProcessProviderUpdatedEvent),
                PhpVdmHostProcessUpdateHandler,
                context,
                &context->ProcessesUpdatedRegistration
                );

            PhInitializeWindowTheme(hwndDlg, PhEnableThemeSupport);
        }
        break;
    case WM_DESTROY:
        {
            PhUnregisterCallback(PhGetGeneralCallback(GeneralCallbackProcessProviderUpdatedEvent), &context->ProcessesUpdatedRegistration);

            PhSaveListViewColumnsToSetting(L"VdmHostListViewColumns", context->ListViewHandle);

            PhpClearVdmHostProcessItems(context);
            PhDereferenceObject(context->VdmHostProcessList);

            PhFree(context);
        }
        break;
    case WM_SHOWWINDOW:
        {
            PPH_LAYOUT_ITEM dialogItem;

            if (dialogItem = PhBeginPropPageLayout(hwndDlg, propPageContext))
            {
                PhAddPropPageLayoutItem(hwndDlg, context->ListViewHandle, dialogItem, PH_ANCHOR_ALL);
                PhEndPropPageLayout(hwndDlg, propPageContext);
            }
        }
        break;
    case WM_NOTIFY:
        {
            LPNMHDR header = (LPNMHDR)lParam;

            switch (header->code)
            {
            case PSN_SETACTIVE:
                context->Enabled = TRUE;
                break;
            case PSN_KILLACTIVE:
                context->Enabled = FALSE;
                break;
            }

            PhHandleListViewNotifyForCopy(lParam, context->ListViewHandle);
        }
        break;
    case WM_CONTEXTMENU:
        {
            if ((HWND)wParam == context->ListViewHandle)
            {
                POINT point;
                PVOID index;
                PPH_EMENU_ITEM selectedItem;

                point.x = GET_X_LPARAM(lParam);
                point.y = GET_Y_LPARAM(lParam);

                if (point.x == -1 && point.y == -1)
                    PhGetListViewContextMenuPoint((HWND)wParam, &point);

                if (index = PhGetSelectedListViewItemParam(context->ListViewHandle))
                {
                    PPH_EMENU menu;

                    menu = PhCreateEMenu();
                    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, 1, L"Terminate", NULL, NULL), ULONG_MAX);
                    PhInsertEMenuItem(menu, PhCreateEMenuSeparator(), ULONG_MAX);
                    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, 4, L"Open &file location", NULL, NULL), ULONG_MAX);
                    PhInsertEMenuItem(menu, PhCreateEMenuSeparator(), ULONG_MAX);
                    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, 5, L"&Inspect", NULL, NULL), ULONG_MAX);
                    PhInsertEMenuItem(menu, PhCreateEMenuSeparator(), ULONG_MAX);
                    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, IDC_COPY, L"&Copy", NULL, NULL), ULONG_MAX);
                    PhInsertCopyListViewEMenuItem(menu, IDC_COPY, context->ListViewHandle);

                    selectedItem = PhShowEMenu(
                        menu,
                        hwndDlg,
                        PH_EMENU_SHOW_LEFTRIGHT,
                        PH_ALIGN_LEFT | PH_ALIGN_TOP,
                        point.x,
                        point.y
                        );
                    PhDestroyEMenu(menu);

                    if (selectedItem && selectedItem->Id != ULONG_MAX)
                    {
                        PPH_NTVDM_ENTRY entry;
                        BOOLEAN handled;

                        handled = PhHandleCopyListViewEMenuItem(selectedItem);

                        //if (!handled && PhPluginsEnabled)
                        //    handled = PhPluginTriggerEMenuItem(&menuInfo, item);

                        if (handled)
                            break;

                        entry = context->VdmHostProcessList->Items[PtrToUlong(index) - 1];

                        switch (selectedItem->Id)
                        {
                        case 1:
                            {
                                if (!PhpTerminateVdmTask(processItem, entry->Task16))
                                {
                                    PhShowError(hwndDlg, L"%s", L"Unable to terminate the task.");
                                }
                            }
                            break;
                        case 4:
                            {
                                if (!PhIsNullOrEmptyString(entry->FileName) && PhDoesFileExistWin32(PhGetString(entry->FileName)))
                                {
                                    PhShellExecuteUserString(
                                        hwndDlg,
                                        L"FileBrowseExecutable",
                                        PhGetString(entry->FileName),
                                        FALSE,
                                        L"Make sure the Explorer executable file is present."
                                        );
                                }
                            }
                            break;
                        case 5:
                            {
                                if (!PhIsNullOrEmptyString(entry->FileName) && PhDoesFileExistWin32(PhGetString(entry->FileName)))
                                {
                                    PhShellExecuteUserString(
                                        hwndDlg,
                                        L"ProgramInspectExecutables",
                                        PhGetString(entry->FileName),
                                        FALSE,
                                        L"Make sure the PE Viewer executable file is present."
                                        );
                                }
                            }
                            break;
                        case IDC_COPY:
                            {
                                PhCopyListView(context->ListViewHandle);
                            }
                            break;
                        }
                    }
                }
            }
        }
        break;
    case WM_PH_VDMDBG_UPDATE:
        {
            PhpRefreshVdmHostProcess(hwndDlg, context, processItem);
        }
        break;
    }

    return FALSE;
}

#endif
