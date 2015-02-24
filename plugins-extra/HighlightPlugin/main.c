/*
 * Process Hacker Extra Plugins -
 *   Highlight Plugin
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

#include "main.h"

static PPH_LIST ProcessHighlightList = NULL;
static COLORREF ProcessCustomColors[16] = 
{ 
    RGB(255, 255, 255),
    RGB(255, 255, 255),
    RGB(255, 255, 255),
    RGB(255, 255, 255),
    RGB(255, 255, 255),
    RGB(255, 255, 255),
    RGB(255, 255, 255),
    RGB(255, 255, 255),
    RGB(255, 255, 255),
    RGB(255, 255, 255),
    RGB(255, 255, 255),
    RGB(255, 255, 255),
    RGB(255, 255, 255),
    RGB(255, 255, 255),
    RGB(255, 255, 255),
    RGB(255, 255, 255)
};

static PPH_PLUGIN PluginInstance = NULL;
static PH_CALLBACK_REGISTRATION PluginLoadCallbackRegistration;
static PH_CALLBACK_REGISTRATION PluginUnloadCallbackRegistration;
static PH_CALLBACK_REGISTRATION PluginShowOptionsCallbackRegistration;
static PH_CALLBACK_REGISTRATION PluginMenuItemCallbackRegistration;
static PH_CALLBACK_REGISTRATION MainWindowShowingCallbackRegistration;
static PH_CALLBACK_REGISTRATION GetProcessHighlightingColorCallbackRegistration;
static PH_CALLBACK_REGISTRATION ProcessMenuInitializingCallbackRegistration;
static PH_CALLBACK_REGISTRATION ProcessTreeNewInitializingCallbackRegistration;

static VOID LoadColorList(
    _Inout_ PPH_LIST ColorList,
    _In_ PPH_STRING String
    )
{
    PH_STRINGREF remaining = String->sr;

    while (remaining.Length != 0)
    {
        ULONG64 integer;
        PH_STRINGREF part;
        PH_STRING_BUILDER stringBuilder;
        PITEM_COLOR colorEntry;

        colorEntry = (PITEM_COLOR)PhAllocate(sizeof(ITEM_COLOR));
        memset(colorEntry, 0, sizeof(ITEM_COLOR));

        PhInitializeStringBuilder(&stringBuilder, 20);
        PhSplitStringRefAtChar(&remaining, ',', &part, &remaining);

        for (SIZE_T i = 0; i < part.Length / sizeof(WCHAR); i++)
        {
            PhAppendCharStringBuilder(&stringBuilder, part.Buffer[i]);
        }

        PhSplitStringRefAtChar(&remaining, ',', &part, &remaining);

        if (PhStringToInteger64(&part, 10, &integer))
        {
            colorEntry->BackColor = (COLORREF)integer;
        }

        colorEntry->ProcessName = PhFinalStringBuilderString(&stringBuilder);

        PhAddItemList(ColorList, colorEntry);
    }
}

static PPH_STRING SaveColorList(
    _Inout_ PPH_LIST ColorList                                          
    )
{
    PH_STRING_BUILDER stringBuilder;

    PhInitializeStringBuilder(&stringBuilder, 100);

    for (SIZE_T i = 0; i < ColorList->Count; i++)
    {
        PITEM_COLOR colorEntry = (PITEM_COLOR)ColorList->Items[i];
        
        PhAppendFormatStringBuilder(
            &stringBuilder,
            L"%s,%u,",
            colorEntry->ProcessName->Buffer,
            colorEntry->BackColor
            );
    }
        
    if (stringBuilder.String->Length != 0)
        PhRemoveStringBuilder(&stringBuilder, stringBuilder.String->Length / 2 - 1, 1);

    return PhFinalStringBuilderString(&stringBuilder);
}

static VOID LoadCustomColors(
    _In_ PPH_STRING String
    )
{
    PH_STRINGREF part;
    PH_STRINGREF remaining = String->sr;

    for (SIZE_T i = 0; i < _countof(ProcessCustomColors); i++)
    {
        ULONG64 integer = 0;

        PhSplitStringRefAtChar(&remaining, ',', &part, &remaining);

        if (PhStringToInteger64(&part, 10, &integer))
        {
            ProcessCustomColors[i] = (COLORREF)integer;
        }
    }
}

static PPH_STRING SaveCustomColors(
    VOID
    )
{
    PH_STRING_BUILDER stringBuilder;

    PhInitializeStringBuilder(&stringBuilder, 100);

    for (SIZE_T i = 0; i < _countof(ProcessCustomColors); i++)
    {
        PhAppendFormatStringBuilder(&stringBuilder, L"%u,", ProcessCustomColors[i]);
    }

    if (stringBuilder.String->Length != 0)
        PhRemoveStringBuilder(&stringBuilder, stringBuilder.String->Length / 2 - 1, 1);

    return PhFinalStringBuilderString(&stringBuilder);
}

static UINT_PTR CALLBACK ColorWndHookProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    switch (uMsg)
    {
    case WM_INITDIALOG:
        PhCenterWindow(hwndDlg, PhMainWndHandle);
        break;
    case WM_CTLCOLORBTN:
    case WM_CTLCOLORDLG:
    case WM_CTLCOLORSTATIC:
        {
            HDC hDC = (HDC)wParam;
            HWND hwndChild = (HWND)lParam;

            // Set a transparent background for the control backcolor.
            SetBkMode(hDC, TRANSPARENT);

            // Set window background color.
            return (INT_PTR)GetSysColorBrush(COLOR_WINDOW);
        }
        break;
    }

    return FALSE;
}

static VOID LoadCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    PPH_STRING colorsString = NULL;
    PPH_STRING customColorsString = NULL;

    ProcessHighlightList = PhCreateList(1);

    colorsString = PhGetStringSetting(SETTING_NAME_COLOR_LIST);
    LoadColorList(ProcessHighlightList, colorsString);
    PhDereferenceObject(colorsString);

    customColorsString = PhGetStringSetting(SETTING_NAME_CUSTOM_COLOR_LIST);
    LoadCustomColors(customColorsString);
    PhDereferenceObject(customColorsString);
}

static VOID NTAPI UnloadCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    PPH_STRING colorsString = NULL;
    PPH_STRING customColorsString = NULL;

    colorsString = SaveColorList(ProcessHighlightList);
    PhSetStringSetting2(SETTING_NAME_COLOR_LIST, &colorsString->sr);
    PhDereferenceObject(colorsString);

    customColorsString = SaveCustomColors();
    PhSetStringSetting2(SETTING_NAME_CUSTOM_COLOR_LIST, &customColorsString->sr);
    PhDereferenceObject(customColorsString);
}

static VOID ShowOptionsCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{

}

static VOID MenuItemCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    PPH_PLUGIN_MENU_ITEM menuItem = Parameter;

    switch (menuItem->Id)
    {
    case PLUGIN_MENU_ADD_BK_ITEM:
        {
            PPH_PROCESS_ITEM processItem = (PPH_PROCESS_ITEM)menuItem->Context;

            if (processItem == NULL)
                break;

            CHOOSECOLOR chooseColor = { sizeof(CHOOSECOLOR) };
            chooseColor.hwndOwner = PhMainWndHandle;
            chooseColor.lpCustColors = ProcessCustomColors;
            chooseColor.lpfnHook = ColorWndHookProc;
            chooseColor.Flags = CC_ANYCOLOR | CC_FULLOPEN | CC_RGBINIT | CC_SOLIDCOLOR | CC_ENABLEHOOK;

            if (ChooseColor(&chooseColor))
            {
                BOOLEAN itemFound = FALSE;
                PITEM_COLOR itemHighlight = NULL;

                for (ULONG i = 0; i < ProcessHighlightList->Count; i++)
                {
                    itemHighlight = (PITEM_COLOR)ProcessHighlightList->Items[i];

                    if (PhEqualString(processItem->ProcessName, itemHighlight->ProcessName, TRUE))
                    {
                        itemFound = TRUE;
                        break;
                    }
                }

                if (itemFound)
                {
                    itemHighlight->BackColor = chooseColor.rgbResult;
                }
                else
                {
                    PITEM_COLOR itemHighlight;

                    itemHighlight = (PITEM_COLOR)PhAllocate(sizeof(ITEM_COLOR));
                    memset(itemHighlight, 0, sizeof(ITEM_COLOR));

                    itemHighlight->ProcessName = PhFormatString(L"%s", processItem->ProcessName->Buffer);
                    itemHighlight->BackColor = chooseColor.rgbResult;

                    PhAddItemList(ProcessHighlightList, itemHighlight);
                }
            }

            PhInvalidateAllProcessNodes();
        }
        break;
    case PLUGIN_MENU_REMOVE_BK_ITEM:
        {
            PPH_PROCESS_ITEM processItem = (PPH_PROCESS_ITEM)menuItem->Context;

            for (ULONG i = 0; i < ProcessHighlightList->Count; i++)
            {
                PITEM_COLOR itemHighlight = (PITEM_COLOR)ProcessHighlightList->Items[i];

                if (PhEqualString(processItem->ProcessName, itemHighlight->ProcessName, TRUE))
                {
                    PhRemoveItemList(ProcessHighlightList, i);

                    if (itemHighlight->ProcessName)
                        PhDereferenceObject(itemHighlight->ProcessName);

                    PhFree(itemHighlight);
                }
            }

            PhInvalidateAllProcessNodes();
        }
        break;
    }
}

static VOID ProcessMenuInitializingCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    PPH_PLUGIN_MENU_INFORMATION menuInfo = Parameter;
    PPH_EMENU_ITEM miscMenuItem;
    PPH_EMENU_ITEM highlightMenuItem;
    PPH_PROCESS_ITEM processItem;

    //PPH_EMENU_ITEM backgroundMenuItem;
    //PPH_EMENU_ITEM foregroundMenuItem;

    miscMenuItem = PhFindEMenuItem(menuInfo->Menu, 0, L"Miscellaneous", 0);

    if (!miscMenuItem)
        return;

    processItem = menuInfo->u.Process.NumberOfProcesses == 1 ? menuInfo->u.Process.Processes[0] : NULL;

    if (processItem)
    {
        if (ProcessHighlightList->Count == 0)
        {
            highlightMenuItem = PhPluginCreateEMenuItem(PluginInstance, 0, PLUGIN_MENU_ADD_BK_ITEM, L"Highlight Process", processItem);
            //backgroundMenuItem = PhPluginCreateEMenuItem(PluginInstance, 0, PLUGIN_MENU_ADD_BK_ITEM, L"Background", processItem);
            //foregroundMenuItem = PhPluginCreateEMenuItem(PluginInstance, 0, PLUGIN_MENU_ADD_FG_ITEM, L"Foreground", processItem);

            //PhInsertEMenuItem(highlightMenuItem, backgroundMenuItem, -1);
            //PhInsertEMenuItem(highlightMenuItem, foregroundMenuItem, -1);
            PhInsertEMenuItem(miscMenuItem, highlightMenuItem, 0);
        }
        else
        {
            BOOLEAN itemFound = FALSE;

            for (ULONG i = 0; i < ProcessHighlightList->Count; i++)
            {
                PITEM_COLOR itemHighlight = (PITEM_COLOR)ProcessHighlightList->Items[i];

                if (PhEqualString(processItem->ProcessName, itemHighlight->ProcessName, TRUE))
                {
                    itemFound = TRUE;
                    break;
                }
            }

            highlightMenuItem = PhPluginCreateEMenuItem(PluginInstance, 0, itemFound ? PLUGIN_MENU_REMOVE_BK_ITEM : PLUGIN_MENU_ADD_BK_ITEM, L"Highlight Process", processItem);
            //backgroundMenuItem = PhPluginCreateEMenuItem(PluginInstance, 0, itemFound ? PLUGIN_MENU_REMOVE_BK_ITEM : PLUGIN_MENU_ADD_BK_ITEM, L"Background", processItem);
            //foregroundMenuItem = PhPluginCreateEMenuItem(PluginInstance, 0, itemFound ? PLUGIN_MENU_REMOVE_FG_ITEM : PLUGIN_MENU_ADD_FG_ITEM, L"Foreground", processItem);

            //PhInsertEMenuItem(highlightMenuItem, backgroundMenuItem, -1);
            //PhInsertEMenuItem(highlightMenuItem, foregroundMenuItem, -1);
            PhInsertEMenuItem(miscMenuItem, highlightMenuItem, 0);

            if (itemFound)
            {
                highlightMenuItem->Flags |= PH_EMENU_CHECKED;
            }
        }
    }
}

static VOID GetProcessHighlightingColorCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    PPH_PLUGIN_GET_HIGHLIGHTING_COLOR getHighlightingColor = Parameter;
    PPH_PROCESS_ITEM processItem = (PPH_PROCESS_ITEM)getHighlightingColor->Parameter;

    if (getHighlightingColor->Handled)
        return;

    for (ULONG i = 0; i < ProcessHighlightList->Count; i++)
    {
        PITEM_COLOR itemHighlight = (PITEM_COLOR)ProcessHighlightList->Items[i];

        if (PhEqualString(processItem->ProcessName, itemHighlight->ProcessName, TRUE))
        {
            getHighlightingColor->BackColor = itemHighlight->BackColor;
            //getHighlightingColor->ForeColor = itemHighlight->BackColor;
            getHighlightingColor->Cache = TRUE;
            getHighlightingColor->Handled = TRUE;
        }
    }
}

LOGICAL DllMain(
    _In_ HINSTANCE Instance,
    _In_ ULONG Reason,
    _Reserved_ PVOID Reserved
    )
{
    switch (Reason)
    {
    case DLL_PROCESS_ATTACH:
        {
            PPH_PLUGIN_INFORMATION info;
            PH_SETTING_CREATE settings[] =
            {
                { StringSettingType, SETTING_NAME_COLOR_LIST, L"" },
                { StringSettingType, SETTING_NAME_CUSTOM_COLOR_LIST, L"" }
            };

            PluginInstance = PhRegisterPlugin(SETTING_PREFIX, Instance, &info);

            if (!PluginInstance)
                return FALSE;

            info->DisplayName = L"Process Highlight";
            info->Author = L"dmex";
            info->Description = L"Plugin for individual process highlighting via the Process > Miscellaneous menu.";
            info->HasOptions = FALSE;

            PhRegisterCallback(
                PhGetPluginCallback(PluginInstance, PluginCallbackLoad),
                LoadCallback,
                NULL,
                &PluginLoadCallbackRegistration
                );      
            PhRegisterCallback(
                PhGetPluginCallback(PluginInstance, PluginCallbackUnload),
                UnloadCallback,
                NULL,
                &PluginUnloadCallbackRegistration
                );
            PhRegisterCallback(
                PhGetPluginCallback(PluginInstance, PluginCallbackShowOptions),
                ShowOptionsCallback,
                NULL,
                &PluginShowOptionsCallbackRegistration
                );
            PhRegisterCallback(
                PhGetPluginCallback(PluginInstance, PluginCallbackMenuItem),
                MenuItemCallback,
                NULL,
                &PluginMenuItemCallbackRegistration
                );
            PhRegisterCallback(
                PhGetGeneralCallback(GeneralCallbackGetProcessHighlightingColor),
                GetProcessHighlightingColorCallback,
                NULL,
                &GetProcessHighlightingColorCallbackRegistration
                );
            PhRegisterCallback(
                PhGetGeneralCallback(GeneralCallbackProcessMenuInitializing),
                ProcessMenuInitializingCallback,
                NULL,
                &ProcessMenuInitializingCallbackRegistration
                );

            PhAddSettings(settings, _countof(settings));
        }
        break;
    }

    return TRUE;
}