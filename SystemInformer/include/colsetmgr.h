#ifndef PH_COLSETMGR_H
#define PH_COLSETMGR_H

typedef struct _PH_COLUMN_SET_ENTRY
{
    PPH_STRING Name;
    PPH_STRING Setting;
    PPH_STRING Sorting;
} PH_COLUMN_SET_ENTRY, *PPH_COLUMN_SET_ENTRY;

PPH_LIST PhInitializeColumnSetList(
    _In_ PWSTR SettingName
    );

VOID PhDeleteColumnSetList(
    _In_ PPH_LIST ColumnSetList
    );

_Success_(return)
BOOLEAN PhLoadSettingsColumnSet(
    _In_ PWSTR SettingName,
    _In_ PPH_STRING ColumnSetName,
    _Out_ PPH_STRING *TreeListSettings,
    _Out_ PPH_STRING *TreeSortSettings
    );

VOID PhSaveSettingsColumnSet(
    _In_ PWSTR SettingName,
    _In_ PPH_STRING ColumnSetName,
    _In_ PPH_STRING TreeListSettings,
    _In_ PPH_STRING TreeSortSettings
    );

// Column Set Editor Dialog

VOID PhShowColumnSetEditorDialog(
    _In_ HWND ParentWindowHandle,
    _In_ PWSTR SettingName
    );

#endif
