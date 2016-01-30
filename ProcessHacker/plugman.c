/*
 * Process Hacker -
 *   plugins
 *
 * Copyright (C) 2010-2011 wj32
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
#include <phplug.h>

#define IS_PLUGIN_LOADED(Plugin) (!!(Plugin)->AppContext.AppName.Buffer)
#define STR_OR_DEFAULT(String, Default) ((String) ? (String) : (Default))

static HWND PluginsLv;
static PPH_PLUGIN SelectedPlugin;
static PPH_LIST DisabledPluginInstances; // fake PH_PLUGIN structures for disabled plugins
static PPH_HASHTABLE DisabledPluginLookup; // list of all disabled plugins (including fake structures) by PH_PLUGIN address

INT_PTR CALLBACK PhpPluginsDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

VOID PhShowPluginsDialog(
    _In_ HWND ParentWindowHandle
    )
{
    if (PhPluginsEnabled)
    {
        DialogBox(
            PhInstanceHandle,
            MAKEINTRESOURCE(IDD_PLUGINS),
            ParentWindowHandle,
            PhpPluginsDlgProc
            );
    }
    else
    {
        PhShowInformation(ParentWindowHandle,
            L"Plugins are not enabled. To use plugins enable them in Options and restart Process Hacker.");
    }
}

PWSTR PhpGetPluginBaseName(
    _In_ PPH_PLUGIN Plugin
    )
{
    if (Plugin->FileName)
    {
        PH_STRINGREF pathNamePart;
        PH_STRINGREF baseNamePart;

        if (PhSplitStringRefAtLastChar(&Plugin->FileName->sr, '\\', &pathNamePart, &baseNamePart))
            return baseNamePart.Buffer;
        else
            return Plugin->FileName->Buffer;
    }
    else
    {
        // Fake disabled plugin.
        return Plugin->Name.Buffer;
    }
}

PWSTR PhpGetPluginDisableButtonText(
    _In_ PWSTR BaseName
    )
{
    PH_STRINGREF baseName;

    PhInitializeStringRefLongHint(&baseName, BaseName);

    if (PhIsPluginDisabled(&baseName))
        return L"Enable";
    else
        return L"Disable";
}

VOID PhpRefreshPluginDetails(
    _In_ HWND hwndDlg
    )
{
    PPH_STRING fileName;
    PH_IMAGE_VERSION_INFO versionInfo;

    if (SelectedPlugin && SelectedPlugin->FileName) // if there's no FileName, then it's a fake disabled plugin instance
    {
        fileName = SelectedPlugin->FileName;

        SetDlgItemText(hwndDlg, IDC_NAME, SelectedPlugin->Information.DisplayName ? SelectedPlugin->Information.DisplayName : L"(unnamed)");
        SetDlgItemText(hwndDlg, IDC_INTERNALNAME, SelectedPlugin->Name.Buffer);
        SetDlgItemText(hwndDlg, IDC_AUTHOR, SelectedPlugin->Information.Author);
        SetDlgItemText(hwndDlg, IDC_FILENAME, fileName->Buffer);
        SetDlgItemText(hwndDlg, IDC_DESCRIPTION, SelectedPlugin->Information.Description);
        SetDlgItemText(hwndDlg, IDC_URL, SelectedPlugin->Information.Url);

        if (PhInitializeImageVersionInfo(&versionInfo, fileName->Buffer))
        {
            SetDlgItemText(hwndDlg, IDC_VERSION, PhGetStringOrDefault(versionInfo.FileVersion, L"Unknown"));
            PhDeleteImageVersionInfo(&versionInfo);
        }
        else
        {
            SetDlgItemText(hwndDlg, IDC_VERSION, L"Unknown");
        }

        ShowWindow(GetDlgItem(hwndDlg, IDC_OPENURL), SelectedPlugin->Information.Url ? SW_SHOW : SW_HIDE);
        EnableWindow(GetDlgItem(hwndDlg, IDC_DISABLE), TRUE);
        SetDlgItemText(hwndDlg, IDC_DISABLE, PhpGetPluginDisableButtonText(PhpGetPluginBaseName(SelectedPlugin)));
        EnableWindow(GetDlgItem(hwndDlg, IDC_OPTIONS), SelectedPlugin->Information.HasOptions);
    }
    else
    {
        SetDlgItemText(hwndDlg, IDC_NAME, L"N/A");
        SetDlgItemText(hwndDlg, IDC_VERSION, L"N/A");
        SetDlgItemText(hwndDlg, IDC_INTERNALNAME, L"N/A");
        SetDlgItemText(hwndDlg, IDC_AUTHOR, L"N/A");
        SetDlgItemText(hwndDlg, IDC_URL, L"N/A");
        SetDlgItemText(hwndDlg, IDC_FILENAME, L"N/A");
        SetDlgItemText(hwndDlg, IDC_DESCRIPTION, L"N/A");

        ShowWindow(GetDlgItem(hwndDlg, IDC_OPENURL), SW_HIDE);

        if (SelectedPlugin)
        {
            // This is a disabled plugin.
            EnableWindow(GetDlgItem(hwndDlg, IDC_DISABLE), TRUE);
            SetDlgItemText(hwndDlg, IDC_DISABLE, PhpGetPluginDisableButtonText(SelectedPlugin->Name.Buffer));
        }
        else
        {
            EnableWindow(GetDlgItem(hwndDlg, IDC_DISABLE), FALSE);
            SetDlgItemText(hwndDlg, IDC_DISABLE, L"Disable");
        }

        EnableWindow(GetDlgItem(hwndDlg, IDC_OPTIONS), FALSE);
    }
}

BOOLEAN PhpIsPluginLoadedByBaseName(
    _In_ PPH_STRINGREF BaseName
    )
{
    PPH_AVL_LINKS links;

    // Extremely inefficient code follows.
    // TODO: Make this better.

    for (links = PhMinimumElementAvlTree(&PhPluginsByName); links; links = PhSuccessorElementAvlTree(links))
    {
        PPH_PLUGIN plugin = CONTAINING_RECORD(links, PH_PLUGIN, Links);
        PH_STRINGREF pluginBaseName;

        PhInitializeStringRefLongHint(&pluginBaseName, PhpGetPluginBaseName(plugin));

        if (PhEqualStringRef(&pluginBaseName, BaseName, TRUE))
            return TRUE;
    }

    return FALSE;
}

PPH_PLUGIN PhpCreateDisabledPlugin(
    _In_ PPH_STRINGREF BaseName
    )
{
    PPH_PLUGIN plugin;

    plugin = PhAllocate(sizeof(PH_PLUGIN));
    memset(plugin, 0, sizeof(PH_PLUGIN));

    plugin->Name.Length = BaseName->Length;
    plugin->Name.Buffer = PhAllocate(BaseName->Length + sizeof(WCHAR));
    memcpy(plugin->Name.Buffer, BaseName->Buffer, BaseName->Length);
    plugin->Name.Buffer[BaseName->Length / 2] = 0;

    return plugin;
}

VOID PhpFreeDisabledPlugin(
    _In_ PPH_PLUGIN Plugin
    )
{
    PhFree(Plugin->Name.Buffer);
    PhFree(Plugin);
}

VOID PhpAddDisabledPlugins(
    VOID
    )
{
    PPH_STRING disabled;
    PH_STRINGREF remainingPart;
    PH_STRINGREF part;
    PPH_PLUGIN disabledPlugin;
    PPH_STRING displayText;
    INT lvItemIndex;

    disabled = PhGetStringSetting(L"DisabledPlugins");
    remainingPart = disabled->sr;

    while (remainingPart.Length != 0)
    {
        PhSplitStringRefAtChar(&remainingPart, '|', &part, &remainingPart);

        if (part.Length != 0)
        {
            if (!PhpIsPluginLoadedByBaseName(&part))
            {
                disabledPlugin = PhpCreateDisabledPlugin(&part);
                PhAddItemList(DisabledPluginInstances, disabledPlugin);
                PhAddItemSimpleHashtable(DisabledPluginLookup, disabledPlugin, NULL);

                displayText = PhCreateString2(&part);
                lvItemIndex = PhAddListViewItem(PluginsLv, MAXINT, displayText->Buffer, disabledPlugin);
                PhDereferenceObject(displayText);
            }
        }
    }

    PhDereferenceObject(disabled);
}

VOID PhpUpdateDisabledPlugin(
    _In_ HWND hwndDlg,
    _In_ INT ItemIndex,
    _In_ PPH_PLUGIN Plugin,
    _In_ BOOLEAN NewDisabledState
    )
{
    if (NewDisabledState)
    {
        PhAddItemSimpleHashtable(DisabledPluginLookup, Plugin, NULL);
    }
    else
    {
        PhRemoveItemSimpleHashtable(DisabledPluginLookup, Plugin);
    }

    if (!IS_PLUGIN_LOADED(Plugin))
    {
        assert(!NewDisabledState);
        ListView_DeleteItem(PluginsLv, ItemIndex);
    }

    InvalidateRect(PluginsLv, NULL, TRUE);

    ShowWindow(GetDlgItem(hwndDlg, IDC_INSTRUCTION), SW_SHOW);
}

static COLORREF PhpPluginColorFunction(
    _In_ INT Index,
    _In_ PVOID Param,
    _In_opt_ PVOID Context
    )
{
    PPH_PLUGIN plugin = Param;

    if (PhFindItemSimpleHashtable(DisabledPluginLookup, plugin))
        return RGB(0x77, 0x77, 0x77); // fake disabled plugin

    return GetSysColor(COLOR_WINDOW);
}

INT_PTR CALLBACK PhpPluginsDlgProc(
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
            PPH_AVL_LINKS links;

            PluginsLv = GetDlgItem(hwndDlg, IDC_LIST);
            PhSetListViewStyle(PluginsLv, FALSE, TRUE);
            PhSetControlTheme(PluginsLv, L"explorer");
            PhAddListViewColumn(PluginsLv, 0, 0, 0, LVCFMT_LEFT, 100, L"File");
            PhAddListViewColumn(PluginsLv, 1, 1, 1, LVCFMT_LEFT, 150, L"Name");
            PhAddListViewColumn(PluginsLv, 2, 2, 2, LVCFMT_LEFT, 100, L"Author");
            PhSetExtendedListView(PluginsLv);
            ExtendedListView_SetItemColorFunction(PluginsLv, PhpPluginColorFunction);

            DisabledPluginLookup = PhCreateSimpleHashtable(10);

            for (links = PhMinimumElementAvlTree(&PhPluginsByName); links; links = PhSuccessorElementAvlTree(links))
            {
                PPH_PLUGIN plugin = CONTAINING_RECORD(links, PH_PLUGIN, Links);
                INT lvItemIndex;
                PH_STRINGREF baseNameSr;

                lvItemIndex = PhAddListViewItem(PluginsLv, MAXINT, PhpGetPluginBaseName(plugin), plugin);

                PhSetListViewSubItem(PluginsLv, lvItemIndex, 1, plugin->Information.DisplayName ? plugin->Information.DisplayName : plugin->Name.Buffer);

                if (plugin->Information.Author)
                    PhSetListViewSubItem(PluginsLv, lvItemIndex, 2, plugin->Information.Author);

                PhInitializeStringRefLongHint(&baseNameSr, PhpGetPluginBaseName(plugin));

                if (PhIsPluginDisabled(&baseNameSr))
                    PhAddItemSimpleHashtable(DisabledPluginLookup, plugin, NULL);
            }

            DisabledPluginInstances = PhCreateList(10);
            PhpAddDisabledPlugins();

            ExtendedListView_SortItems(PluginsLv);

            SelectedPlugin = NULL;
            PhpRefreshPluginDetails(hwndDlg);
        }
        break;
    case WM_DESTROY:
        {
            ULONG i;

            for (i = 0; i < DisabledPluginInstances->Count; i++)
                PhpFreeDisabledPlugin(DisabledPluginInstances->Items[i]);

            PhDereferenceObject(DisabledPluginInstances);
            PhDereferenceObject(DisabledPluginLookup);
        }
        break;
    case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
            case IDCANCEL:
            case IDOK:
                EndDialog(hwndDlg, IDOK);
                break;
            case IDC_DISABLE:
                {
                    if (SelectedPlugin)
                    {
                        PWSTR baseName;
                        PH_STRINGREF baseNameRef;
                        BOOLEAN newDisabledState;

                        baseName = PhpGetPluginBaseName(SelectedPlugin);
                        PhInitializeStringRef(&baseNameRef, baseName);
                        newDisabledState = !PhIsPluginDisabled(&baseNameRef);
                        PhSetPluginDisabled(&baseNameRef, newDisabledState);
                        PhpUpdateDisabledPlugin(hwndDlg, PhFindListViewItemByFlags(PluginsLv, -1, LVNI_SELECTED), SelectedPlugin, newDisabledState);

                        SetDlgItemText(hwndDlg, IDC_DISABLE, PhpGetPluginDisableButtonText(baseName));
                    }
                }
                break;
            case IDC_OPTIONS:
                {
                    if (SelectedPlugin && IS_PLUGIN_LOADED(SelectedPlugin))
                    {
                        PhInvokeCallback(PhGetPluginCallback(SelectedPlugin, PluginCallbackShowOptions), hwndDlg);
                    }
                }
                break;
            case IDC_CLEANUP:
                {
                    if (PhShowMessage(hwndDlg, MB_ICONQUESTION | MB_YESNO,
                        L"Do you want to clean up unused plugin settings?") == IDYES)
                    {
                        PhClearIgnoredSettings();
                    }
                }
                break;
            case IDC_OPENURL:
                {
                    NOTHING;
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
            case LVN_ITEMCHANGED:
                {
                    if (header->hwndFrom == PluginsLv)
                    {
                        if (ListView_GetSelectedCount(PluginsLv) == 1)
                            SelectedPlugin = PhGetSelectedListViewItemParam(PluginsLv);
                        else
                            SelectedPlugin = NULL;

                        PhpRefreshPluginDetails(hwndDlg);
                    }
                }
                break;
            case NM_CLICK:
                {
                    if (header->hwndFrom == GetDlgItem(hwndDlg, IDC_OPENURL))
                    {
                        if (SelectedPlugin && IS_PLUGIN_LOADED(SelectedPlugin))
                            PhShellExecute(hwndDlg, SelectedPlugin->Information.Url, NULL);
                    }
                }
                break;
            case NM_DBLCLK:
                {
                    if (header->hwndFrom == PluginsLv)
                    {
                        if (SelectedPlugin && IS_PLUGIN_LOADED(SelectedPlugin))
                        {
                            PhInvokeCallback(PhGetPluginCallback(SelectedPlugin, PluginCallbackShowOptions), hwndDlg);
                        }
                    }
                }
                break;
            }
        }
        break;
    }

    REFLECT_MESSAGE_DLG(hwndDlg, PluginsLv, uMsg, wParam, lParam);

    return FALSE;
}
