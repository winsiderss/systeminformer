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

#define STR_OR_DEFAULT(String, Default) ((String) ? (String) : (Default))

static HWND PluginsLv;
static PPH_PLUGIN SelectedPlugin;
static PPH_LIST DisabledPluginInstances; // fake PH_PLUGIN structures for disabled plugins

INT_PTR CALLBACK PhpPluginsDlgProc(
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    );

VOID PhShowPluginsDialog(
    __in HWND ParentWindowHandle
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
    __in PPH_PLUGIN Plugin
    )
{
    PWSTR baseName;

    if (Plugin->FileName)
    {
        baseName = wcsrchr(Plugin->FileName->Buffer, '\\');

        if (baseName)
            baseName++; // skip the backslash
        else
            baseName = Plugin->FileName->Buffer;
    }
    else
    {
        // Fake disabled plugin.
        baseName = Plugin->Name;
    }

    return baseName;
}

PWSTR PhpGetPluginDisableButtonText(
    __in PWSTR BaseName
    )
{
    PH_STRINGREF baseName;

    PhInitializeStringRef(&baseName, BaseName);

    if (PhIsPluginDisabled(&baseName))
        return L"Enable";
    else
        return L"Disable";
}

VOID PhpRefreshPluginDetails(
    __in HWND hwndDlg
    )
{
    PPH_STRING fileName;
    PH_IMAGE_VERSION_INFO versionInfo;

    if (SelectedPlugin && SelectedPlugin->FileName) // if there's no FileName, then it's a fake disabled plugin instance
    {
        fileName = SelectedPlugin->FileName;

        SetDlgItemText(hwndDlg, IDC_NAME, SelectedPlugin->Information.DisplayName ? SelectedPlugin->Information.DisplayName : L"(unnamed)");
        SetDlgItemText(hwndDlg, IDC_INTERNALNAME, SelectedPlugin->Name);
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
            SetDlgItemText(hwndDlg, IDC_DISABLE, PhpGetPluginDisableButtonText(SelectedPlugin->Name));
        }
        else
        {
            EnableWindow(GetDlgItem(hwndDlg, IDC_DISABLE), FALSE);
            SetDlgItemText(hwndDlg, IDC_DISABLE, L"Disable");
        }

        EnableWindow(GetDlgItem(hwndDlg, IDC_OPTIONS), FALSE);
    }
}

PPH_PLUGIN PhpCreateDisabledPlugin(
    __in PPH_STRING BaseName
    )
{
    PPH_PLUGIN plugin;

    plugin = PhAllocate(sizeof(PH_PLUGIN));
    memset(plugin, 0, sizeof(PH_PLUGIN));

    plugin->Name = PhAllocateCopy(BaseName->Buffer, BaseName->Length + sizeof(WCHAR));

    return plugin;
}

VOID PhpFreeDisabledPlugin(
    __in PPH_PLUGIN Plugin
    )
{
    PhFree(Plugin->Name);
    PhFree(Plugin);
}

VOID PhpAddDisabledPlugins()
{
    PPH_STRING disabled;
    ULONG i;
    ULONG length;
    ULONG endOfPart;
    PPH_STRING part;
    PPH_PLUGIN disabledPlugin;
    INT lvItemIndex;

    disabled = PhGetStringSetting(L"DisabledPlugins");
    i = 0;
    length = disabled->Length / 2;

    while (i < length)
    {
        endOfPart = PhFindCharInString(disabled, i, '|');

        if (endOfPart == -1)
            endOfPart = length;

        part = PhSubstring(disabled, i, endOfPart - i);

        disabledPlugin = PhpCreateDisabledPlugin(part);
        PhAddItemList(DisabledPluginInstances, disabledPlugin);
        lvItemIndex = PhAddListViewItem(PluginsLv, MAXINT, part->Buffer, disabledPlugin);
        PhDereferenceObject(part);
        PhSetListViewSubItem(PluginsLv, lvItemIndex, 1, L"(Disabled)");

        i = endOfPart + 1;
    }

    PhDereferenceObject(disabled);
}

static COLORREF PhpPluginColorFunction(
    __in INT Index,
    __in PVOID Param,
    __in_opt PVOID Context
    )
{
    PPH_PLUGIN plugin = Param;

    if (plugin->Flags & PH_PLUGIN_FLAG_IS_CLR)
        return RGB(0xde, 0xff, 0x00);
    if (!plugin->FileName)
        return RGB(0x77, 0x77, 0x77); // fake disabled plugin

    return PhSysWindowColor;
}

INT_PTR CALLBACK PhpPluginsDlgProc(
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
            PPH_AVL_LINKS links;

            PluginsLv = GetDlgItem(hwndDlg, IDC_LIST);
            PhSetListViewStyle(PluginsLv, FALSE, TRUE);
            PhSetControlTheme(PluginsLv, L"explorer");
            PhAddListViewColumn(PluginsLv, 0, 0, 0, LVCFMT_LEFT, 200, L"Name");
            PhAddListViewColumn(PluginsLv, 1, 1, 1, LVCFMT_LEFT, 160, L"Author");
            PhSetExtendedListView(PluginsLv);
            ExtendedListView_SetItemColorFunction(PluginsLv, PhpPluginColorFunction);

            links = PhMinimumElementAvlTree(&PhPluginsByName);

            while (links)
            {
                PPH_PLUGIN plugin = CONTAINING_RECORD(links, PH_PLUGIN, Links);
                INT lvItemIndex;

                lvItemIndex = PhAddListViewItem(PluginsLv, MAXINT,
                    plugin->Information.DisplayName ? plugin->Information.DisplayName : plugin->Name, plugin);

                if (plugin->Information.Author)
                    PhSetListViewSubItem(PluginsLv, lvItemIndex, 1, plugin->Information.Author);

                links = PhSuccessorElementAvlTree(links);
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

                        baseName = PhpGetPluginBaseName(SelectedPlugin);
                        PhInitializeStringRef(&baseNameRef, baseName);
                        PhSetPluginDisabled(&baseNameRef, !PhIsPluginDisabled(&baseNameRef));

                        SetDlgItemText(hwndDlg, IDC_DISABLE, PhpGetPluginDisableButtonText(baseName));
                    }
                }
                break;
            case IDC_OPTIONS:
                {
                    if (SelectedPlugin)
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
                    if (SelectedPlugin)
                    {
                    }
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
                        if (SelectedPlugin)
                            PhShellExecute(hwndDlg, SelectedPlugin->Information.Url, NULL);
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
