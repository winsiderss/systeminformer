/*
 * Process Hacker Extra Plugins -
 *   Performance Monitor Plugin
 *
 * Copyright (C) 2015 dmex
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

#include "perfmon.h"

static VOID FreeCounterEntry(
    _In_ PPH_PERFMON_ENTRY Entry
    )
{
    if (Entry->Name)
    {
        PhDereferenceObject(Entry->Name);
    }

    PhFree(Entry);
}

VOID ClearCounterList(
    _Inout_ PPH_LIST FilterList
    )
{
    for (ULONG i = 0; i < FilterList->Count; i++)
    {
        FreeCounterEntry((PPH_PERFMON_ENTRY)FilterList->Items[i]);
    }

    PhClearList(FilterList);
}

VOID CopyCounterList(
    _Inout_ PPH_LIST Destination,
    _In_ PPH_LIST Source
    )
{
    for (ULONG i = 0; i < Source->Count; i++)
    {
        PPH_PERFMON_ENTRY entry = (PPH_PERFMON_ENTRY)Source->Items[i];
        PPH_PERFMON_ENTRY newEntry;

        newEntry = (PPH_PERFMON_ENTRY)PhAllocate(sizeof(PH_PERFMON_ENTRY));

        PhReferenceObject(entry->Name);
        newEntry->Name = entry->Name;

        PhAddItemList(Destination, newEntry);
    }
}

VOID LoadCounterList(
    _Inout_ PPH_LIST FilterList,
    _In_ PPH_STRING String
    )
{
    PH_STRING_BUILDER stringBuilder;
    PPH_PERFMON_ENTRY entry = NULL; 

    PH_STRINGREF part;
    PH_STRINGREF remaining = String->sr;

    while (remaining.Length != 0)
    {
        entry = (PPH_PERFMON_ENTRY)PhAllocate(sizeof(PH_PERFMON_ENTRY));
        memset(entry, 0, sizeof(PH_PERFMON_ENTRY));
       
        PhInitializeStringBuilder(&stringBuilder, 20);
        PhSplitStringRefAtChar(&remaining, ',', &part, &remaining);

        for (SIZE_T i = 0; i < part.Length / sizeof(WCHAR); i++)
        {
            if (part.Buffer[i] == '\\') 
            {
                if (i != part.Length - 1)
                {
                    i++;
                    PhAppendCharStringBuilder(&stringBuilder, part.Buffer[i]);
                } 
                else
                {
                    // Unescape backslashes - Just ignore chars.
                    break;
                }
            }
            else 
            {
                PhAppendCharStringBuilder(&stringBuilder, part.Buffer[i]);
            }
        }

        entry->Name = PhCreateString(stringBuilder.String->Buffer);

        PhAddItemList(FilterList, entry);
        PhDeleteStringBuilder(&stringBuilder);
    }
}

PPH_STRING SaveCounterList(
    _Inout_ PPH_LIST FilterList
    )
{
    PH_STRING_BUILDER stringBuilder;
    WCHAR temp[2];

    PhInitializeStringBuilder(&stringBuilder, 100);

    temp[0] = '\\';

    for (SIZE_T i = 0; i < FilterList->Count; i++)
    {
        PPH_PERFMON_ENTRY entry = (PPH_PERFMON_ENTRY)FilterList->Items[i];
        
        SIZE_T length = entry->Name->Length / 2;

        for (SIZE_T ii = 0; ii < length; ii++)
        {
            if (entry->Name->Buffer[ii] == '\\') // escape backslashes
            {
                temp[1] = entry->Name->Buffer[ii];
                PhAppendStringBuilderEx(&stringBuilder, temp, 4);
            }
            else
            {
                PhAppendCharStringBuilder(&stringBuilder, entry->Name->Buffer[ii]);
            }
        }

        PhAppendCharStringBuilder(&stringBuilder, ',');
    }

    if (stringBuilder.String->Length != 0)
        PhRemoveStringBuilder(&stringBuilder, stringBuilder.String->Length / 2 - 1, 1);

    return PhFinalStringBuilderString(&stringBuilder);
}

static VOID AddCounterToListView(
    _In_ PPH_PERFMON_CONTEXT Context,
    _In_ PWSTR CounterName
    )
{
    PPH_PERFMON_ENTRY entry;

    entry = (PPH_PERFMON_ENTRY)PhAllocate(sizeof(PH_PERFMON_ENTRY));
    memset(entry, 0, sizeof(PH_PERFMON_ENTRY));

    entry->Name = PhCreateString(CounterName);

    PhAddListViewItem(
        Context->ListViewHandle,
        MAXINT,
        entry->Name->Buffer,
        entry
        );

    PhAddItemList(Context->CountersListEdited, entry);
}

static VOID LoadCountersToListView(
    _In_ PPH_PERFMON_CONTEXT Context,
    _In_ PPH_LIST Source
    )
{ 
    for (ULONG i = 0; i < Source->Count; i++)
    {
        PPH_PERFMON_ENTRY entry = (PPH_PERFMON_ENTRY)Source->Items[i];
            
        PhAddListViewItem(
            Context->ListViewHandle, 
            MAXINT, 
            entry->Name->Buffer, 
            entry
            );
    }
}

static INT_PTR CALLBACK OptionsDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PPH_PERFMON_CONTEXT context = NULL;

    if (uMsg == WM_INITDIALOG)
    {
        context = (PPH_PERFMON_CONTEXT)PhAllocate(sizeof(PH_PERFMON_CONTEXT));
        memset(context, 0, sizeof(PH_PERFMON_CONTEXT));

        SetProp(hwndDlg, L"Context", (HANDLE)context);
    }
    else
    {
        context = (PPH_PERFMON_CONTEXT)GetProp(hwndDlg, L"Context");

        if (uMsg == WM_NCDESTROY)
        {
            PPH_STRING string;

            ClearCounterList(CountersList);
            CopyCounterList(CountersList, context->CountersListEdited);
            PhDereferenceObject(context->CountersListEdited);

            string = SaveCounterList(CountersList);
            PhSetStringSetting2(SETTING_NAME_PERFMON_LIST, &string->sr);
            PhDereferenceObject(string);
            
            RemoveProp(hwndDlg, L"Context");
            PhFree(context);
        }
    }

    if (context == NULL)
        return FALSE;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            context->CountersListEdited = PhCreateList(2);
            context->ListViewHandle = GetDlgItem(hwndDlg, IDC_PERFCOUNTER_LISTVIEW);

            PhSetListViewStyle(context->ListViewHandle, FALSE, TRUE);
            PhSetControlTheme(context->ListViewHandle, L"explorer");
            PhAddListViewColumn(context->ListViewHandle, 0, 0, 0, LVCFMT_LEFT, 400, L"Counter");
            PhSetExtendedListView(context->ListViewHandle);

            ClearCounterList(context->CountersListEdited);
            CopyCounterList(context->CountersListEdited, CountersList);
            LoadCountersToListView(context, context->CountersListEdited);
        }
        break;
    case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
            case IDC_ADD_BUTTON:
                {
                    PDH_STATUS counterStatus = 0;
                    PPH_STRING counterPathString = NULL;
                    PPH_STRING counterWildCardString = NULL;
                    PDH_BROWSE_DLG_CONFIG browseConfig = { 0 };
                    WCHAR counterPathBuffer[PDH_MAX_COUNTER_PATH] = L"";

                    browseConfig.bIncludeInstanceIndex = FALSE;
                    browseConfig.bSingleCounterPerAdd = FALSE;// Fix empty CounterPathBuffer
                    browseConfig.bSingleCounterPerDialog = TRUE;
                    browseConfig.bLocalCountersOnly = FALSE;
                    browseConfig.bWildCardInstances = TRUE; // Seems to cause a lot of crashes
                    browseConfig.bHideDetailBox = TRUE;
                    browseConfig.bInitializePath = FALSE;
                    browseConfig.bDisableMachineSelection = FALSE;
                    browseConfig.bIncludeCostlyObjects = FALSE;
                    browseConfig.bShowObjectBrowser = FALSE;
                    browseConfig.hWndOwner = hwndDlg;
                    browseConfig.szReturnPathBuffer = counterPathBuffer;
                    browseConfig.cchReturnPathLength = PDH_MAX_COUNTER_PATH;
                    browseConfig.CallBackStatus = ERROR_SUCCESS;
                    browseConfig.dwDefaultDetailLevel = PERF_DETAIL_WIZARD;
                    browseConfig.szDialogBoxCaption = L"Select a counter to monitor.";

                    __try
                    {
                        // Display the counter browser window. 
                        if ((counterStatus = PdhBrowseCounters(&browseConfig)) != ERROR_SUCCESS)
                        {
                            if (counterStatus != PDH_DIALOG_CANCELLED)
                            {
                                PhShowError(hwndDlg, L"PdhBrowseCounters failed with status 0x%x.", counterStatus);
                            }

                            __leave;
                        }
                        else if (wcslen(counterPathBuffer) == 0)
                        {
                            // This gets called when pressing the X on the BrowseCounters dialog.
                            __leave;
                        }

                        counterPathString = PhCreateString(counterPathBuffer);

                        // Check if we need to expand any wildcards...
                        if (PhFindCharInString(counterPathString, 0, '*') != -1)
                        {
                            ULONG counterWildCardLength = 0;

                            // Query WildCard buffer length...
                            PdhExpandWildCardPath(
                                NULL,
                                counterPathString->Buffer,
                                NULL,
                                &counterWildCardLength,
                                0
                                );

                            counterWildCardString = PhCreateStringEx(NULL, counterWildCardLength * sizeof(WCHAR));

                            if ((counterStatus = PdhExpandWildCardPath(
                                NULL,
                                counterPathString->Buffer,
                                counterWildCardString->Buffer,
                                &counterWildCardLength,
                                0
                                )) == ERROR_SUCCESS)
                            {
                                PH_STRINGREF part;
                                PH_STRINGREF remaining = counterWildCardString->sr;

                                while (remaining.Length != 0)
                                {
                                    // Split the results
                                    if (!PhSplitStringRefAtChar(&remaining, '\0', &part, &remaining))
                                        break;
                                    if (remaining.Length == 0)
                                        break;

                                    if ((counterStatus = PdhValidatePath(part.Buffer)) != ERROR_SUCCESS)
                                    {
                                        PhShowError(hwndDlg, L"PdhValidatePath failed with status 0x%x.", counterStatus);
                                        __leave;
                                    }

                                    AddCounterToListView(context, part.Buffer);
                                }
                            }
                            else
                            {
                                PhShowError(hwndDlg, L"PdhExpandWildCardPath failed with status 0x%x.", counterStatus);
                            }
                        }
                        else
                        {
                            if ((counterStatus = PdhValidatePath(counterPathString->Buffer)) != ERROR_SUCCESS)
                            {
                                PhShowError(hwndDlg, L"PdhValidatePath failed with status 0x%x.", counterStatus);
                                __leave;
                            }

                            AddCounterToListView(context, counterPathString->Buffer);
                        }
                    }
                    __finally
                    {
                        if (counterWildCardString)
                            PhDereferenceObject(counterWildCardString);

                        if (counterPathString)
                            PhDereferenceObject(counterPathString);
                    }
                }
                break;
            case IDC_REMOVE_BUTTON:
                {
                    INT itemIndex;

                    // Get the first selected item
                    itemIndex = ListView_GetNextItem(context->ListViewHandle, -1, LVNI_SELECTED);

                    while (itemIndex != -1)
                    {
                        PPH_PERFMON_ENTRY entry;

                        if (PhGetListViewItemParam(context->ListViewHandle, itemIndex, (PPVOID)&entry))
                        {
                            ULONG index = PhFindItemList(context->CountersListEdited, entry);

                            if (index != -1)
                            {
                                FreeCounterEntry(entry);
                                PhRemoveItemList(context->CountersListEdited, index);
                                PhRemoveListViewItem(context->ListViewHandle, itemIndex);
                            }
                        }

                        // Get the next selected item
                        itemIndex = ListView_GetNextItem(context->ListViewHandle, -1, LVNI_SELECTED);
                    }
                }
                break;
            case IDCANCEL:
                EndDialog(hwndDlg, IDCANCEL);
                break;
            case IDOK:
                EndDialog(hwndDlg, IDOK);
                break;
            }
        }
        break;
    }

    return FALSE;
}

VOID ShowOptionsDialog(
    _In_ HWND ParentHandle
    )
{
    DialogBox(
        (HINSTANCE)PluginInstance->DllBase,
        MAKEINTRESOURCE(IDD_PERFMON_OPTIONS),
        ParentHandle,
        OptionsDlgProc
        );
}