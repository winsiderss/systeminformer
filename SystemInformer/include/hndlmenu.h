#ifndef PH_HNDLMENU_H
#define PH_HNDLMENU_H

typedef struct _PH_HANDLE_ITEM_INFO
{
    HANDLE ProcessId;
    HANDLE Handle;
    PPH_STRING TypeName;
    PPH_STRING BestObjectName;
} PH_HANDLE_ITEM_INFO, *PPH_HANDLE_ITEM_INFO;

#define PhaAppendCtrlEnter(Text, Enable) ((Enable) ? PhaConcatStrings2((Text), L"\tCtrl+Enter")->Buffer : (Text))

VOID PhInsertHandleObjectPropertiesEMenuItems(
    _In_ struct _PH_EMENU_ITEM *Menu,
    _In_ ULONG InsertBeforeId,
    _In_ BOOLEAN EnableShortcut,
    _In_ PPH_HANDLE_ITEM_INFO Info
    );

#define PH_MAX_SECTION_EDIT_SIZE (32 * 1024 * 1024) // 32 MB

VOID PhShowHandleObjectProperties1(
    _In_ HWND hWnd,
    _In_ PPH_HANDLE_ITEM_INFO Info
    );

VOID PhShowHandleObjectProperties2(
    _In_ HWND hWnd,
    _In_ PPH_HANDLE_ITEM_INFO Info
    );

#endif
